#include "CHIPTrimmer.h"
#include "CHIPInterface.h"
#include "DsscProgressBar.h"
//#include "DsscCalibrationDB.h"
#include "utils.h"
#include <iostream>

using namespace std;
using namespace SuS;

#define log_id() "CHIPTrimmer"
#define SuS_LOG_STREAM(type,id,output)\
  std::cout <<"-- ["#type"  ] [CHIPTrimmer  ] " << output << std::endl; std::cout.flush();

#ifdef HAVE_HDF5
#include "DsscHDF5TrimmingDataWriter.h"
#endif

//#ifdef HAVE_HDF5
//DsscHDF5CalibrationDB CHIPTrimmer::s_calibrationDB("DsscCalibrationDB.h5");
//#else
//SuS::DsscCalibrationDB CHIPTrimmer::s_calibrationDB("DsscCalibrationDB.txt");
//#endif

bool CHIPTrimmer::s_saveSpektrumData = false;
bool CHIPTrimmer::s_saveImageData = false;
string CHIPTrimmer::s_saveOutDir = ".";
std::vector<std::thread> CHIPTrimmer::s_saveThreads;


CHIPTrimmer::CHIPTrimmer(CHIPInterface *chip) :
  m_chip(chip),
  m_startDacVal(4000),
  m_endDacVal(7000),
  m_numDacVals(5),
  m_startMemAddr(0),
  m_endMemAddr(799),
  m_asicWise(true),
  m_iterations(1),
  m_numRuns(1),
  m_runTrimming(true),
  m_untrimmablePixelsFound(false),
  m_trimCurrentDoubleMode(TCDM::DOUBLE),
  m_outputDir(""),
  m_pxInjCalibfactorsValid(false),
  m_calibInjMode("CHARGE_BUSINJ"),
  m_gainConfigurator(m_chip)
{
  initMemberVectorSizes();
  cout << "CHIPTrimmer initialized" << endl;
}

void CHIPTrimmer::setChipInterface(CHIPInterface *chip)
{
  m_chip = chip;
  m_gainConfigurator.setChipInterface(m_chip);
  initMemberVectorSizes();
}

void CHIPTrimmer::setPixelInjectionCalibrationFactors(const std::vector<double> &pxInjCalibFactors, const std::string & calibGainMode)
{
  m_pxInjCalibfactorsValid = false;
  if(pxInjCalibFactors.size() != (size_t)m_chip->totalNumPxs){
    return;
  }

  m_pxInjCalibFactors = pxInjCalibFactors;
  m_calibInjMode = calibGainMode;

  m_pxInjCalibfactorsValid=true;
}

std::vector<uint32_t> CHIPTrimmer::getAllPixels() const
{
  std::vector<uint32_t> allPixels;
  for (auto s : m_chipParts){
    auto colPixels = m_chip->getSendingPixels<uint32_t>(s);
    allPixels.insert(allPixels.end(),colPixels.begin(),colPixels.end());
  }

  sort(allPixels.begin(),allPixels.end());

  return allPixels;
}

void CHIPTrimmer::initMemberVectorSizes()
{
  m_runTrimming = true;

  m_chip->checkLadderReadout();
  m_finalSlopes.assign(m_chip->totalNumPxs,0.0);
  m_curvatureValues.assign(m_chip->totalNumPxs,0.0);
  m_untrimmablePixels.assign(m_chip->totalNumPxs,false);
  m_untrimmablePixelsFound = false;

  m_chip->updatePoweredPixels();
}

//top iramp trimming function
bool CHIPTrimmer::calibratePxsIrampSetting(const double targetSlope, const double maxAbsDiff, bool checkOnly)
{
  SuS_LOG_STREAM(warning, log_id(), "Using CHIPTrimmer::calibratePxsIrampSetting()");

  initMemberVectorSizes();

  if((uint32_t)m_endMemAddr >= m_chip->getNumFramesToSend()){
    if (!(m_chip->fullFReadout && (m_chip->numOfPxs == 4096))){
      m_startMemAddr = 0;
      m_endMemAddr = m_chip->getNumFramesToSend();
      SuS_LOG_STREAM(warning, log_id(), "Iramp Trimming: End address (" << m_endMemAddr << ") larger than numFramesToSend  (" << m_chip->getNumFramesToSend() << ")");
      SuS_LOG_STREAM(warning, log_id(), "getNumWordsToReceive(): " << m_chip->getNumWordsToReceive());
      SuS_LOG_STREAM(warning, log_id(), "End address set to " << m_chip->getNumFramesToSend() << " and m_startMemAddr set to 0");
    }
  }

  m_dacValues = utils::getSettingsVector(m_startDacVal,m_endDacVal,m_numDacVals);
  if(m_dacValues.size() < 4){
    cout << "ERROR: Dac Values range too short, will not produce useful results - exit" << endl;
    return false;
  } else {
    cout << "++++++++ DacRange = " << m_startDacVal << " - " << m_endDacVal << ", " << m_numDacVals << " points." << endl;
  }

  // ASIC Wise Pixel Trimming was here inserted but never used

  bool ok = true;

  cout << "++++++++ Start Trimming " << m_chip->getPoweredPixels().size() << " Pixels.... ++++++++" << endl;

  {// Output of ChipParts
    SuS_LOG_STREAM(debug, log_id(), "Trimming ADC gain for the following chipParts:");
    for (auto s : m_chipParts) SuS_LOG_STREAM(debug, log_id(), s);
  }

  initChipProgressBar("Trimming Iramp Settings",m_chipParts.size());

  for (auto cp : m_chipParts)
  {
    m_chip->enableMonBusCols(cp);
    m_currentRunPixels = m_chip->getSendingPixels<uint32_t>(cp);

    SuS_LOG_STREAM(info, log_id(), "----------------------------------------------------------");
    SuS_LOG_STREAM(info, log_id(), "-- Current trimmingChipPart = " << cp << " - "
        << m_currentRunPixels.size() << " pixels.");
    SuS_LOG_STREAM(info, log_id(), "----------------------------------------------------------");

    if (m_currentRunPixels.empty()) {
      SuS_LOG_STREAM(info, log_id(), "No pixels in current run.");
      continue;
    }

    //SuS_LOG_STREAM(debug, log_id(), "First pixel is " << m_currentRunPixels[0]);

    m_chip->removeUnpoweredPixels(m_currentRunPixels);

    m_chip->enableInjection(false,"all",false);
    m_chip->enableInjection(true, utils::positionVectorToList(m_currentRunPixels));

    m_chip->findAndSetInjectionEventOffset(m_currentRunPixels,m_dacValues.back(), m_startMemAddr);

    findUntrimmablePixels(m_currentRunPixels);

    if(checkOnly) {
      vector<uint32_t> unsettledPxs;
      checkIrampCalibration(unsettledPxs,cp,targetSlope,maxAbsDiff);
    } else {
      ok &= calibrateAndCheckPxsIrampSetting(cp,targetSlope,maxAbsDiff);
      if(!ok){
        SuS_LOG_STREAM(warning, log_id(), "calibrateAndCheckPxsIrampSetting() high number of untrimmable pixels found for run: " << cp);
        //break;
      } else {
        SuS_LOG_STREAM(info, log_id(), "+++++ ChipPart " << cp << " done +++++");
      }
    }

    if (m_chipParts.empty()) {
      SuS_LOG_STREAM(warning, log_id(), "chipParts are empty.");
    }

    if(!m_runTrimming){
      break;
    }

    //restorePoweredPixels();
  }

  showTrimmingStats();

  m_chip->enableInjection(false,"all");
  return ok;
}


bool CHIPTrimmer::calibrateAndCheckPxsIrampSetting(const string & runInfo, double targetSlope, const double maxAbsDiff)
{
  vector<uint32_t> unsettledPxs = m_currentRunPixels;
  SuS_LOG_STREAM(info, log_id(), "calibrateAndCheckPxsIrampSetting(): starting with " << unsettledPxs.size() << " pixels.");

  m_errorCode = false;
  bool ok = true;

  uint32_t it=0;
  // main iramp trimming loop
  while(/*!m_errorCode &&*/ ok && unsettledPxs.size()>0 && it<m_numRuns && m_runTrimming)
  {
    ok = calibratePxsIrampSetting(unsettledPxs,targetSlope,maxAbsDiff);

    string status = runInfo + " it " + to_string(it) + " results: ";
    SuS_LOG_STREAM(info, log_id(), "Unsettled pixels.size() = " << unsettledPxs.size());
    checkIrampCalibration(unsettledPxs,status,targetSlope,maxAbsDiff);

    it++;

    if(m_errorCode){
      SuS_LOG_STREAM(info, log_id(), "+++++ Warning Got ErrorCode in Run " << it << " because of Range Error +++++");
    }

    const uint32_t numUntrimmable =  getUntrimmablePixels().size();

    SuS_LOG_STREAM(info, log_id(), "+++++ After Run " << it << ": " << numUntrimmable << " untrimmable pixels found. +++++");
    if(numUntrimmable > 2222){
      SuS_LOG_STREAM(info, log_id(), "+++++ Too many untrimmable pixels found - will abort trimming +++++");
      return false;
    }


    if(unsettledPxs.size() > 0){
      if(it < m_numRuns){
        SuS_LOG_STREAM(info, log_id(), "+++++ After Run " << it << ": " << unsettledPxs.size() << " will go into next trimming round: "
            << utils::positionVectorToList(unsettledPxs) << " +++++");
      }else{
        SuS_LOG_STREAM(info, log_id(), "+++++ After Run " << it << ": " << unsettledPxs.size() << " could not be trimmed to aimrange: "
            << utils::positionVectorToList(unsettledPxs) << " +++++");
      }
    }else{
      SuS_LOG_STREAM(info, log_id(), "+++++ All sucessfully trimed into aim range to max difference of "
          << utils::setPrecision(utils::calcPerCent(maxAbsDiff,targetSlope),2) << "% finished after run " << it << " +++++");
    }
  }

  addValuesToCalibrationDB("IrampTrimmingSlopes",m_chip->getFinalSlopes());

  return ok;
}


bool CHIPTrimmer::calibratePxsIrampSetting(const vector<uint32_t> & unsettledPxs, const double targetSlope, const double maxAbsDiff)
{
  if(m_trimCurrentDoubleMode == TCDM::BOTH) {
    SuS_LOG_STREAM(info, log_id(), "Trimming with both CD settings.");
  } else if (m_trimCurrentDoubleMode == TCDM::SLOW) {
    SuS_LOG_STREAM(info, log_id(), "Trimming with CD = 0 only.");
  } else if (m_trimCurrentDoubleMode == TCDM::DOUBLE) {
    SuS_LOG_STREAM(info, log_id(), "Trimming with CD = 1 only.");
  }
  // trimCurrentDoubleMode: SLOW   = trim only for CurrentDouble 0
  //                        DOUBLE = trim only for currentDouble 1
  //                        BOTH   = trim for currentDouble 0 & 1
  // only in BOTH mode, the first run has to be checked if there was found
  // a better configuration. also the initial fine trim configuration has
  // to be set to a large value before double setting is changed to avoid error
  // codes if ramp length is too small
  bool ok = true;
  if(itWillBeHardToGetValidValues(unsettledPxs)){
    cout << "+++++ Set all RmpFineTrims to 63 because of hard to handle settings" << endl;
    m_chip->setPixelRegisterValue(utils::positionVectorToList(unsettledPxs),"RmpFineTrm",63);
  }

  vector<double> finalAbsDiffsDouble(m_chip->totalNumPxs);
  vector<uint32_t> irampSettingsDouble(m_chip->totalNumPxs);
  if(m_trimCurrentDoubleMode != TCDM::SLOW){
    ok &= calibratePxsIrampSettingForDouble(unsettledPxs,finalAbsDiffsDouble,irampSettingsDouble,targetSlope,maxAbsDiff,1);
    //if(!ok) return ok;
  }

  if(m_trimCurrentDoubleMode == TCDM::BOTH)
  { //set fine trim to high value before switch to double = 0 to avoid error codes
    m_chip->setPixelRegisterValue(utils::positionVectorToList(unsettledPxs),"RmpFineTrm",63);
  }

  vector<double> finalAbsDiffs(m_chip->totalNumPxs);
  vector<uint32_t> irampSettings(m_chip->totalNumPxs);
  if(m_trimCurrentDoubleMode != TCDM::DOUBLE){
    ok &= calibratePxsIrampSettingForDouble(unsettledPxs,finalAbsDiffs,irampSettings,targetSlope,maxAbsDiff,0);
    //if(!ok) return ok;
  }

  if(m_trimCurrentDoubleMode == TCDM::BOTH)
  {
    for(int px=0; px<m_chip->totalNumPxs; px++){
      if(finalAbsDiffsDouble[px] < finalAbsDiffs[px]){
        m_chip->setPixelRegisterValue(to_string(px),"RmpCurrDouble",1);
        m_chip->setPixelRegisterValue(to_string(px),"RmpFineTrm",irampSettingsDouble[px]);
      }
    }
  }

  m_chip->programPixelRegs();

  return ok;
}



void CHIPTrimmer::findUntrimmablePixels(vector<uint32_t> & pixels)
{
  m_chip->checkLadderReadout();

  m_curvatureValues = m_chip->measureDacSweepForCurvature(pixels,m_dacValues,m_startMemAddr,m_endMemAddr);
  int numDiscardedPx = 0;

#pragma omp parallel for
  for(size_t idx=0; idx<pixels.size(); idx++)
  {
    int px = pixels[idx];
    auto curvature = m_curvatureValues[px];
    if (curvature > 1.5 || curvature < 0.5) {
      m_untrimmablePixels[px] = true;
      numDiscardedPx++;
    }
  }

  cout << "++++++++ " << numDiscardedPx << " pixels set untrimmable because of bad curvature in the DAC sweep." << endl;
}




int CHIPTrimmer::checkIrampCalibration(vector<uint32_t> & unsettledPxs, string & status, double targetSlope, double maxAbsDiff)
{
  if(m_chip->numOfPxs == 4096 && !m_chip->pixelRegisters->signalIsVarious("Control register","RmpFineTrm","all")){
    status = "Iramp not fully trimmed - all registers same values";
    cout << status << endl;
  }

  string fileName;
  int stat = 0;
  if(CHIPInterface::isNormMode(m_chip->getInjectionMode()))
  {
    stat = checkIrampCalibrationUsingSpektrum(status,targetSlope,maxAbsDiff);
    fileName = m_outputDir + "TrimmedIRampSettingsSpectrumCheck.txt";
  }
  else
  {
    stat = checkIrampCalibrationUsingInjection(status,targetSlope,maxAbsDiff);
    fileName = m_outputDir + "TrimmedIRampSettingsInjectionCheck.txt";
  }

  if(stat<0){
    return stat;
  }

  writeIrampCalibrateLog(targetSlope,fileName);

  // check slopes and get overall calibration status
  stat =  updateIrampCalibStatus(unsettledPxs,status,targetSlope,maxAbsDiff);

  m_trimmingStats.push_back(status);

  m_chip->m_progressBar.addValue();

  return stat;
}


int CHIPTrimmer::checkIrampCalibrationUsingInjection(string & status, double targetSlope, double maxAbsDiff)
{
  const uint32_t LINSUM_X  = std::accumulate(m_dacValues.begin(), m_dacValues.end(), 0.0);
  const uint32_t LINSUM_XX = std::inner_product(m_dacValues.begin(), m_dacValues.end(), m_dacValues.begin(), 0.0);

  vector<vector<double> > binValues(m_chip->totalNumPxs);
  for(auto && vals : binValues){
    vals.resize(m_dacValues.size());
  }

  vector<uint32_t> activePixels = m_currentRunPixels;

  cout << endl << endl << "++++++++ Checking Iramp Trimming target: " << targetSlope << " +- " << maxAbsDiff << " +++++++++ "<< endl;
  cout << "++++++++ Checking " << activePixels.size() << " Pixels : " << utils::positionVectorToList(activePixels).substr(0,100) << " ... " << endl;

  if(!m_chip->measureIntDacSweepforSlopes(m_dacValues,activePixels,binValues,m_startMemAddr,m_endMemAddr)){
    status = "DataReceiver Timeout!";
    return -10;
  }

  // if problems in sweep data occures active pixels will be changed.
  // Only good pixels will be checked. Bad pixels will not get a new finalSlope value
  //int idx = 0;
  const size_t numPxs = activePixels.size();

#pragma omp parallel for
  for(size_t idx=0; idx<numPxs; idx++)
  {
    int px = activePixels[idx];
    double slope = utils::linearRegression(LINSUM_X,LINSUM_XX,m_dacValues,binValues[px]);
    m_finalSlopes[px] = (m_pxInjCalibfactorsValid)? calcGainValueFromSlope(px,slope) : slope;
    double curvature = m_chip->calcCurvatureGraph(m_dacValues,binValues[px]);
    m_curvatureValues[px] = curvature;

// print measurement values during checking
//    if(idx++ < 10){
//      cout << px << " : slope " << slope << " - ";
//      for(const auto & value : binValues[px]){
//        cout << setw(8) << value << " ";
//      }
//      cout << endl;
//    }
  }

  cout << "CHIPTRimmer: Copy final slopes to CHIPInterface" << endl;
  m_chip->setFinalSlopes(m_finalSlopes);

  return 0;
}


double CHIPTrimmer::calcGainValueFromSlope(int pixel,double slope)
{
  return m_pxInjCalibFactors[pixel]/std::max(1E-5,slope);
}


int CHIPTrimmer::checkIrampCalibrationUsingSpektrum(string & status, double targetSlope, double maxAbsDiff)
{
  vector<uint32_t> activePixels = m_currentRunPixels;

  cout << endl << endl << "++++++++ Checking Iramp Trimming target: " << targetSlope << " +- " << maxAbsDiff << " +++++++++ "<< endl;
  cout << "++++++++ Checking " << activePixels.size() << " Pixels : " << utils::positionVectorToList(activePixels) << endl;

  // const removed because binBoundaries can be calculated and stored in the dataHisto in the
  // fitting function (for plotting the histogram later)
  auto spektrumHistograms = m_chip->measureHistoData(activePixels,m_startMemAddr,m_endMemAddr);

  if(s_saveSpektrumData){
    saveHistograms(activePixels,spektrumHistograms);
  }

  const auto fitResults = utils::fitSpectra(spektrumHistograms);

  // if problems in sweep data occures active pixels will be changed.
  // Only good pixels will be checked. Bad pixels will not get a new finalSlope value

  const size_t numPxs = activePixels.size();
#pragma omp parallel for
  for(size_t idx=0; idx<numPxs; idx++){
    m_finalSlopes[activePixels[idx]] = fitResults[idx].getGain();
  }

  status = "All Ok";

  return 0;
}


vector<uint32_t> CHIPTrimmer::getUntrimmablePixels() const
{
  vector<uint32_t> unTrimPixels;
  int px = 0;
  for(const auto & untrimPix : m_untrimmablePixels){
    if(untrimPix){
      unTrimPixels.push_back(px);
    }
    px++;
  }
  return unTrimPixels;
}


std::vector<double> CHIPTrimmer::calcSlopesFromIntDacSweep(const std::vector<uint32_t> & unsettledPxs,
                                                           uint32_t LINSUM_X, uint32_t LINSUM_XX,
                                                           const std::vector<uint32_t> & dacValues,
                                                           const std::vector<std::vector<double>> & binValues)
{
  const size_t numPxs = unsettledPxs.size();

  vector<double> slopeVec(numPxs,0.0);

#pragma omp parallel for
  for(size_t idx = 0; idx < numPxs; idx++)
  {
    int px = unsettledPxs[idx];
    slopeVec[idx] = utils::linearRegression(LINSUM_X,LINSUM_XX,dacValues,binValues[px]) ;
  }

  return slopeVec;
}


bool CHIPTrimmer::calibratePxsIrampSettingForDouble(vector<uint32_t>  unsettledPxs,
                                                    vector<double> & finalAbsDiffs,
                                                    std::vector<uint32_t> &irampSettings,
                                                    const double targetSlope,
                                                    const double maxAbsDiff,
                                                    int currentDouble)
{
  bool ok = true;

  if(CHIPInterface::isNormMode(m_chip->getInjectionMode()))
  {
    ok = calibratePxsIrampSettingForDoubleUsingSpektrum(unsettledPxs,finalAbsDiffs,irampSettings,
                                                        targetSlope,maxAbsDiff,currentDouble);
  }
  else
  {
    ok = calibratePxsIrampSettingForDoubleUsingInjection(unsettledPxs,finalAbsDiffs,irampSettings,
                                                         targetSlope,maxAbsDiff,currentDouble);
  }
  return ok;
}


bool CHIPTrimmer::calibratePxsIrampSettingForDoubleUsingInjection(vector<uint32_t>  unsettledPxs,
                                                                  vector<double> & finalAbsDiffs,
                                                                  std::vector<uint32_t> &irampSettings,
                                                                  const double targetSlope,
                                                                  const double maxAbsDiff,
                                                                  int currentDouble)
{
  SuS_LOG_STREAM(info, log_id(), "");
  SuS_LOG_STREAM(info, log_id(), "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  SuS_LOG_STREAM(info, log_id(), "++++++++ Trimming IRamp with Injection using Current Double Setting " << currentDouble << " ++++++++");
  SuS_LOG_STREAM(info, log_id(), "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

  m_chip->setPixelRegisterValue(utils::positionVectorToList(unsettledPxs),"RmpCurrDouble",currentDouble);
  irampSettings = m_chip->pixelRegisters->getSignalValues("Control register","all","RmpFineTrm");

  //same for all pixels since only xValues are included
  const uint32_t LINSUM_X  = std::accumulate(m_dacValues.begin(), m_dacValues.end(), 0.0);
  const uint32_t LINSUM_XX = std::inner_product(m_dacValues.begin(), m_dacValues.end(), m_dacValues.begin(), 0.0);

  vector<double> lastSlopeDiff(m_chip->totalNumPxs,targetSlope);
  vector<double> lastSlopeDiffAbs(m_chip->totalNumPxs,targetSlope);
  vector<int>    lastIrampSettings(m_chip->totalNumPxs);

  vector<vector<double> > binValues(m_chip->totalNumPxs,vector<double>(m_dacValues.size(),0));
  vector<uint32_t> untrimPxs;
  const double nomSlope  = targetSlope;

  // check if calibration factors are existing
  bool useCalibFactors = false;
  if(m_pxInjCalibfactorsValid && m_calibInjMode == m_chip->getInjectionModeName(m_chip->getInjectionMode())){
    useCalibFactors = true;
    SuS_LOG_STREAM(info, log_id(), "Valid PixelInjection Calibration Factors found, will apply during trimming\n");
    std::vector<uint32_t> invalidParamsPx;
    for (int i=unsettledPxs.size()-1; i>=0; --i)
    {
      int px = unsettledPxs[i];
      if(m_pxInjCalibFactors[px] == 0.0){
        invalidParamsPx.push_back(px);
        untrimPxs.push_back(px);
        unsettledPxs.erase(unsettledPxs.begin()+i);
      }
    }

    if(!invalidParamsPx.empty()){
      SuS_LOG_STREAM(warning, log_id(), invalidParamsPx.size() << " pixels do not have a valid calibration factor:");
      SuS_LOG_STREAM(warning, log_id(), "No-Factor Pixels:" << utils::positionVectorToList(invalidParamsPx).substr(0,200) << "...");
    }
  }

  size_t numStartPixels = unsettledPxs.size();
  int attempts = 0;

  while(unsettledPxs.size() != 0 && (attempts < 25) && m_runTrimming )
  {
    m_chip->programPixelRegs();
    if(!m_chip->measureIntDacSweepforSlopes(m_dacValues,unsettledPxs,binValues,m_startMemAddr,m_endMemAddr)){
      return false;
    }

    // unsettledPxs may be changed during sweep measurememt. If the sweep data contains error codes etc.
    // these pixels will be removed and not further be trimmed.

    // keep for debugging
    //    for (uint32_t i=0; (i<unsettledPxs.size() && i<10); i++)
    //    {
    //      const int px = unsettledPxs[i];

    //      std::cout << "Pixel "<< px;
    //      for(int j=0; j<5; j++){
    //        std:: cout << " binValues[px][" << to_string(j) << "]:" << setw(7) << binValues[px][j];
    //      }
    //      std::cout << std::endl;
    //    }

    const auto slopeVector = calcSlopesFromIntDacSweep(unsettledPxs,LINSUM_X,LINSUM_XX,m_dacValues,binValues);

    for (int i=unsettledPxs.size()-1; i>=0; --i)
    {
      int px       = unsettledPxs[i];
      int rampDiff = 0;
      int irampSettingToShow = irampSettings[px];
      bool showInfo = false;

      // using calibFactors slope becomes gain in eV
      double slope  = (useCalibFactors)? calcGainValueFromSlope(px,slopeVector[i]) : slopeVector[i];

      double slopeDiff    = slope-nomSlope;
      double slopeRat     = (useCalibFactors)? nomSlope/slope : slope/nomSlope;
      double slopeDiffAbs = fabs(slopeDiff);

      //maybe aimslope is not reached while minimum is found ->  proceed only if last step was larger than 1
      //closest setting can only be found with sign change
      int lastRampDiff = irampSettings[px] - lastIrampSettings[px];
      bool singleStep = abs(lastRampDiff)==1;
      bool signChanged = ((slopeDiff*lastSlopeDiff[px]) < 0);

      lastSlopeDiff[px] = slopeDiff;

      bool aimSlopeReached = (lastSlopeDiffAbs[px] <= maxAbsDiff) || singleStep;
      bool minimumReached  = (lastSlopeDiffAbs[px] < slopeDiffAbs) || fabs(slopeRat < 0.01);

      if (attempts > 0 && minimumReached && aimSlopeReached && signChanged && singleStep)
      {
        unsettledPxs.erase(unsettledPxs.begin()+i);
        irampSettings[px]  = lastIrampSettings[px];
        irampSettingToShow = irampSettings[px];
        // write best setting directly into
        //smaller than last is not always a good criteria, if data is bad, slope should at least be close to nomSlope
        if(lastSlopeDiffAbs[px] > maxAbsDiff){
          cout << utils::STDMAGENTA <<"BAD :";
          showInfo = true;
        }else if(i<10){
          cout << utils::STDGREEN   << "DONE:";
          showInfo = true;
        }
      }
      else
      {
        //last run will produce second best slope, so just update final slope up to second last run
        //if calibFactors are used final slopes contain gain ev values
        m_finalSlopes[px] = slope;

        //rampDiff = calcRampDiff(slopeDiff,targetSlope);
        rampDiff = calcRampDiffUsingRatio(slopeRat,nomSlope,lastRampDiff);
        if(minimumReached){
          // if we get here and minimum range is bad, try next
          rampDiff = rampDiff/fabs(rampDiff);
        }

        lastSlopeDiffAbs[px]  = slopeDiffAbs;
        lastIrampSettings[px] = irampSettings[px];

        irampSettings[px] += rampDiff;

        if (slopeRat > 4 || slopeRat < 0.25) {
          untrimPxs.push_back(px);
          unsettledPxs.erase(unsettledPxs.begin()+i);

          if(untrimPxs.size()<10){
            cout << utils::STDRED <<"#FAIL";
            showInfo = true;
          }
        } else if(irampSettings[px]<64 && irampSettings[px]>=0) {
          if(i<10){
            cout << utils::STDBLUE << "CONT:";
            showInfo = true;
          }
        }else{
          // if next setting out of range set to limits, if limits have already been tested
          // pixel is set untrimmable
          irampSettings[px] = (irampSettings[px]<0)? 0 : 63;
          if(lastIrampSettings[px] == 0 || lastIrampSettings[px] == 63) {
            untrimPxs.push_back(px);
            unsettledPxs.erase(unsettledPxs.begin()+i);

            if(untrimPxs.size()<10){
              cout << utils::STDRED <<"#FAIL";
              showInfo = true;
            }
          }else{
            if(i<10){
              cout << utils::STDBLUE << "PROC:";
              showInfo = true;
            }
          }

          if(minimumReached){
            if(!showInfo){
              cout << utils::STDMAGENTA << "STEP:";
              showInfo = true;
            }
          }
        }
      }

      m_chip->setPixelRegisterValue( to_string(px), "RmpFineTrm", irampSettings[px]);

      if(showInfo){

        std::string info = (useCalibFactors)? "GainEV" : "slope";

        stringstream ss;
        ss << " Pixel "                   << setw(5) << px;
        ss << " irampSettings[px] = "     << setw(2) << irampSettingToShow << " +- " << setw(3) << rampDiff;
        ss << " "<<info<<" = "            << setprecision(4) << setw(9)  << slope << " / " << setprecision(4) << setw( 9) << nomSlope;
        ss << " "<<info<<"Diff = "        << setprecision(4) << setw(10) << slopeDiff ;
        ss << " last"<<info<<"DiffAbs = " << setprecision(4) << setw(10) << lastSlopeDiff[px];
        ss << " ( " << setw(4) << utils::setPrecision(utils::calcPerCent(lastSlopeDiffAbs[px],nomSlope),2) <<" %) " << utils::STDNORM;
        cout << ss.str() << endl;
      }
    }

    SuS_LOG_STREAM(info, log_id(), "Attempt: "<< attempts << ". Number of unsettled Pixels left: " << unsettledPxs.size());
    if(untrimPxs.size()>0){
      SuS_LOG_STREAM(warning, log_id(), " Number of untrimmable pixels in this round (currentDouble = "<< currentDouble <<"): " << untrimPxs.size());
    }
    attempts++;
  }

  if (unsettledPxs.size()>0) {
    SuS_LOG_STREAM(warning, log_id(), unsettledPxs.size() << " pixels have not settled.");
    SuS_LOG_STREAM(warning, log_id(), "Unsettled pixels are: " << utils::positionVectorToList(unsettledPxs));
  }else{
    SuS_LOG_STREAM(info, log_id(), "++++++ All pixels settled after " << attempts << " attempts");
  }

  if(untrimPxs.size()>0){
    SuS_LOG_STREAM(warning, log_id(), untrimPxs.size() << " Untrimmable pixels found in this round");
  }

  finalAbsDiffs = lastSlopeDiffAbs;

  const string fileName = m_outputDir + "TrimmedIrampSettingAfterTrim_CD"+to_string(currentDouble)+".txt";
  writeIrampCalibrateLog(targetSlope,fileName);

  if(unsettledPxs.size() != 0){
    SuS_LOG_STREAM(warning, log_id(), unsettledPxs.size() << " unsettled pixels found in this round");
    SuS_LOG_STREAM(warning, log_id(), "Fraction of unsettled pixels was: " << unsettledPxs.size() / numStartPixels);
  }

  bool highNumberOfUnsettledPixels = (unsettledPxs.size() / numStartPixels) > 0.2;
  return highNumberOfUnsettledPixels;
}


bool CHIPTrimmer::calibratePxsIrampSettingForDoubleUsingSpektrum(vector<uint32_t>  unsettledPxs,
                                                                 vector<double> & finalAbsDiffs,
                                                                 std::vector<uint32_t> &irampSettings,
                                                                 const double targetGain,
                                                                 const double maxAbsDiff,
                                                                 int currentDouble)
{

  SuS_LOG_STREAM(info, log_id(), "");
  SuS_LOG_STREAM(info, log_id(), "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  SuS_LOG_STREAM(info, log_id(), "++++++++ IRamp Trimming with Spectrum using Current Double Setting " << currentDouble << " ++++++++");
  SuS_LOG_STREAM(info, log_id(), "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

  m_chip->setPixelRegisterValue(utils::positionVectorToList(unsettledPxs),"RmpCurrDouble",currentDouble);
  irampSettings = m_chip->pixelRegisters->getSignalValues("Control register","all","RmpFineTrm");

  vector<double> lastGainDiff(m_chip->totalNumPxs,targetGain);
  vector<double> lastGainDiffAbs(m_chip->totalNumPxs,targetGain);
  vector<int>    lastIrampSettings(m_chip->totalNumPxs);

  const double nomGain  = targetGain;

  vector<uint32_t> untrimPxs;
  size_t numStartPixels = unsettledPxs.size();

  int attempts = 0;

  while(unsettledPxs.size() != 0 && (attempts < 25) && m_runTrimming )
  {
    m_chip->programPixelRegs();
    auto spektrumHistograms = m_chip->measureHistoData(unsettledPxs,m_startMemAddr,m_endMemAddr);

    if(s_saveSpektrumData){
      saveHistograms(unsettledPxs,spektrumHistograms);
    }

    const auto fitResults = utils::fitSpectra(spektrumHistograms);

    for (int i=unsettledPxs.size()-1; i>=0; --i)
    {
      int px       = unsettledPxs[i];
      int rampDiff = 0;
      int irampSettingToShow = irampSettings[px];
      bool showInfo = false;
      double gain  = fitResults[i].getGain();

      double gainDiff    = gain - nomGain;
      double gainDiffAbs = fabs(gainDiff);

      m_finalSlopes[px] = gain;

      //maybe aimslope is not reached while minimum is found ->  proceed only if last step was larger than 1
      //closest setting can only be found with sign change
      bool singleStep = (abs(irampSettings[px] - lastIrampSettings[px])==1);
      bool signChanged = ((gainDiff*lastGainDiffAbs[px]) < 0);

      lastGainDiff[px] = gainDiff;

      bool aimSlopeReached = (lastGainDiffAbs[px] <= maxAbsDiff) || singleStep;
      bool minimumReached  = (lastGainDiffAbs[px] < gainDiffAbs);
      if (attempts > 0 && minimumReached && aimSlopeReached && signChanged && singleStep)
      {
        unsettledPxs.erase(unsettledPxs.begin()+i);
        irampSettings[px]  = lastIrampSettings[px];
        irampSettingToShow = irampSettings[px];
        // write best setting directly into
        //smaller than last is not always a good criteria, if data is bad, slope should at least be close to nomSlope
        if(lastGainDiffAbs[px] > maxAbsDiff){
          cout << utils::STDMAGENTA <<"BAD :";
          showInfo = true;
        }else if(i<10){
          cout << utils::STDGREEN   << "DONE:";
          showInfo = true;
        }
      }
      else
      {
        rampDiff = calcRampDiffSpektrum(gainDiff,targetGain);
        if(minimumReached){
          // if we get here and minimum range is bad, try next
          rampDiff = rampDiff/fabs(rampDiff);
        }

        lastGainDiffAbs[px]  = gainDiffAbs;
        lastIrampSettings[px] = irampSettings[px];

        irampSettings[px] += rampDiff;

        if(irampSettings[px]<64 && irampSettings[px]>=0)
        {
          if(i<10){
            cout << utils::STDBLUE << "CONT:";
            showInfo = true;
          }
        }else{
          // if next setting out of range set to limits, if limits have already been tested
          // pixel is set untrimmable
          irampSettings[px] = (irampSettings[px]<0)? 0 : 63;
          if(lastIrampSettings[px] == 0 || lastIrampSettings[px] == 63){
            untrimPxs.push_back(px);
            unsettledPxs.erase(unsettledPxs.begin()+i);

            if(untrimPxs.size()<10){
              cout << utils::STDRED <<"#FAIL";
              showInfo = true;
            }
          }else{
            if(i<10){
              cout << utils::STDBLUE << "PROC:";
              showInfo = true;
            }
          }

          if(minimumReached){
            if(!showInfo){
              cout << utils::STDMAGENTA << "STEP:";
              showInfo = true;
            }
          }
        }
      }

      m_chip->setPixelRegisterValue( to_string(px), "RmpFineTrm", irampSettings[px]);

      if(showInfo){
        stringstream ss;
        ss << " Pixel "              << setw(5) << px;
        ss << " irampSettings[px] = "<< setw(2) << irampSettingToShow << " +- " << setw(3) << rampDiff;
        ss << " slope = "            << setprecision(4) << setw(9)  << gain << " / " << setprecision(4) << setw( 9) << nomGain;
        ss << " slopeDiff = "        << setprecision(4) << setw(10) << gainDiff ;
        ss << " lastSlopeDiffAbs = " << setprecision(4) << setw(10) << lastGainDiff[px];
        ss << " ( " << setw(4) << utils::setPrecision(utils::calcPerCent(lastGainDiffAbs[px],nomGain),2) <<" %) " << utils::STDNORM;
        SuS_LOG_STREAM(debug, log_id(),ss.str());
      }
    }

    SuS_LOG_STREAM(info, log_id(), "Attempt: "<< attempts << ". Number of unsettled Pixels left: " << unsettledPxs.size());
    if(untrimPxs.size()>0){
      SuS_LOG_STREAM(warning, log_id(), " Number of untrimmable pixels in this round (currentDouble = "<< currentDouble <<"): " << untrimPxs.size());
    }
    attempts++;
  }


  if (unsettledPxs.size()>0) {
    SuS_LOG_STREAM(warning, log_id(), unsettledPxs.size() << " pixels have not settled.");
    SuS_LOG_STREAM(warning, log_id(), "Fraction of unsettled pixels was: " << unsettledPxs.size() / numStartPixels);
    SuS_LOG_STREAM(warning, log_id(), "Unsettled pixels are: " << utils::positionVectorToList(unsettledPxs));
  }else{
    SuS_LOG_STREAM(info, log_id(), "++++++ All pixels settled after " << attempts << " attempts");
  }

  if(untrimPxs.size()>0){
    SuS_LOG_STREAM(warning, log_id(), untrimPxs.size() << " Untrimmable pixels found in this round");
  }

  finalAbsDiffs = lastGainDiffAbs;

  const string fileName = utils::getLocalTimeStr() + "TrimmedIrampSettingAfterSpektrumTrim_CD"+to_string(currentDouble)+".txt";
  writeIrampCalibrateLog(targetGain,fileName,true);

  if(unsettledPxs.size() != 0){
    SuS_LOG_STREAM(warning, log_id(), untrimPxs.size() << " Untrimmable pixels found in this round");
  }
  return unsettledPxs.size()==0;
}


void CHIPTrimmer::saveHistograms(const std::vector<uint32_t> & activePixels, const utils::DataHistoVec & pixelHistograms)
{
  const string outputFile = s_saveOutDir + "/" + utils::getLocalTimeStr() + "_SpektrumTrimmingHistos.txt";
  utils::DataHisto::dumpHistogramsToASCII(outputFile,activePixels,pixelHistograms,0);
}

// real slope change per setting = 0.00059
// larger change decreases speed but will improve result
// because both sides of the minimum will be tested
int CHIPTrimmer::calcRampDiff(double slopeDiff, double targetSlope) const
{
  double slopeFactor = 100.0 / 4.0; // 4% difference gives one iramp Setting
  int rampDiff = slopeFactor * slopeDiff/targetSlope;
  if(rampDiff==0){
    rampDiff = (slopeDiff < 0)? -1 : 1;
  }
  return rampDiff;
}

// real slope change per setting = 0.00059
// larger change decreases speed but will improve result
// because both sides of the minimum will be tested
int CHIPTrimmer::calcRampDiffSpektrum(double gainDiff, double targetGain) const
{
  //TODO: not checked for spektrum measurements
  // find parameters to accelerate paramter finding
  double gainFactor = 100.0 / 4.0; // 4% difference gives one iramp Setting
  int rampDiff = gainFactor * gainDiff/targetGain;
  if(rampDiff==0){
    rampDiff = (gainDiff < 0)? -1 : 1;
  }
  return rampDiff;
}


int CHIPTrimmer::calcRampDiffUsingRatio(double slopeRat, double targetSlope, int lastRampDiff) const
{
  int rampDiff = fabs(1-(slopeRat))*100/2;
  //if (rampDiff < 1) SuS_LOG_STREAM(error, log_id(), "slopeRat = " << slopeRat << " rampDiff = " << rampDiff);

  // steps should become always smaller
  if (abs(rampDiff)>abs(lastRampDiff)) rampDiff = fabs(lastRampDiff);

  // sign correction from above analysis
  if (slopeRat < 1 && rampDiff > 0) rampDiff *= -1;
  // rampdiff minimum is 1
  return (rampDiff!=0) ? rampDiff : slopeRat<1 ? -1 : 1;
}


void CHIPTrimmer::showTrimmingStats()
{
  SuS_LOG_STREAM(info, log_id(), "Trimming stats:");
  for(const auto & stat : m_trimmingStats){
    cout << stat << endl;
  }
  m_trimmingStats.clear();

  for(auto && thr : s_saveThreads){
    if(thr.joinable()){
      thr.join();
    }
  }
  s_saveThreads.clear();
}


int CHIPTrimmer::updateIrampCalibStatus(vector<uint32_t> &unsettledPxs, string & status, const double targetSlope,const double maxAbsDiff)
{
  double minSlope = targetSlope;
  double maxSlope = targetSlope;

  bool someTrimmed = false;
  bool allTrimmed = true;

  double slopeRMS = 0.0;

  unsettledPxs.clear();

  int cnt = 0;
  //for(const auto & slope : m_finalSlopes)
  for(auto && px : m_currentRunPixels)
  {
    auto slope = m_finalSlopes[px];

    //0.0 means unpowered or untrimmable
    if(utils::almost_equal(slope,0.0))
    {//untrimmable = powered but finalSlope = 0.0
      if(m_chip->poweredPixels[px]){
        m_untrimmablePixels[px]  = true;
        m_untrimmablePixelsFound = true;
      }
    }
    else
    {
      slopeRMS += pow((targetSlope-slope),2);
      if(slope < minSlope){ minSlope = slope; }
      if(slope > maxSlope){ maxSlope = slope; }

      if( fabs(slope-targetSlope) > maxAbsDiff)
      { // if powered and slope out of range but not 0.0, pixel has already be trimmed to minimum
        // but slope is now out of range, try again and find better setting
        if(m_chip->poweredPixels[px]){
          unsettledPxs.push_back(px);
          allTrimmed = false;
        }
      }else{
        someTrimmed = true;
      }
      cnt++;
    }
  }

  slopeRMS = 100.0 * sqrt(slopeRMS / (cnt-1)) / targetSlope;

  stringstream ss;
  ss << cnt << " pixels ";
  ss << "in Range ";
  ss << setw(4) << utils::setPrecision(utils::calcDiffPerCent(minSlope,targetSlope),2) << "% - ";
  ss << setw(4) << utils::setPrecision(utils::calcDiffPerCent(maxSlope,targetSlope),2) << "% ( mean = ";
  ss << targetSlope << " -/+ "<< utils::setPrecision(slopeRMS,3) << " %)";

  const string rangeStr = ss.str();

  cout << getUntrimmablePixels().size() << " Untrimmable Pixels found, " << rangeStr << endl;

  if(allTrimmed){
    status += "All pixels trimmed. " + rangeStr;
    return 2;
  }

  if(someTrimmed){
    status += to_string(getUntrimmablePixels().size()) + " Untrimmable Pixels found: " +  rangeStr;
    return 1;
  }

  status += "Not trimmed. " + rangeStr;

  return -1;
}


void CHIPTrimmer::writeIrampCalibrateLog(const double targetSlope, const string & fileName, bool gainNotSlope)
{
  ofstream out(fileName,ofstream::out);
  if (!out.is_open()){
    SuS_LOG_STREAM(error, log_id(), "File error: file not opened!");
    return;
  }

  out << left;
  out << "################################" << endl;
  out << "#### Iramp Trimming Results ####" << endl;
  out << "################################" << endl;
  out << "#Statistics:\t" << utils::getVectorStatsStr(m_finalSlopes) << endl;
  out << "#### Columns\t" << ((m_chip->numOfPxs>505)? "64" : "8") << endl;
  out << setw(10) << "#Pixel\t";
  out << setw(10) << "ASIC\t";
  out << setw(10) << "RmpFineTrim\t";
  out << setw(10) << "RmpCurrDouble\t";
  out << setw(10) << "CompCoarse\t";
  out << setw(10) << ((gainNotSlope)? "Gain[keV/Bin]\t" : "Slope\t");
  out << setw(10) << "Diff\t" << endl;
  out << setw(10) << "Curvature\t" << endl;

  for (int px=0; px<m_chip->totalNumPxs; ++px) {
    const int asic   = m_chip->getASICNumberFromPixel(px);
    const int val    = m_chip->getPixelRegisterValue(to_string(px),"RmpFineTrm");
    const int dub    = m_chip->getPixelRegisterValue(to_string(px),"RmpCurrDouble");
    //const int coarse = m_chip->getPixelRegisterValue(to_string(px),"CompCoarse");
    const int coarse = 0;
    out << setw(10) << px     << "\t";
    out << setw(10) << asic   << "\t";
    out << setw(10) << val    << "\t";
    out << setw(10) << dub    << "\t";
    out << setw(10) << coarse << "\t";
    out << setw(10) << m_finalSlopes[px]<< "\t";
    out << setw(10) << utils::setPrecision(utils::calcDiffPerCent(m_finalSlopes[px],targetSlope),2) << "%"<< "\t";
    out << setw(10) << m_curvatureValues[px] << "\t";
    out << "\n";
  }
}


bool CHIPTrimmer::itWillBeHardToGetValidValues(const vector<uint32_t> & unsettledPxs) const
{
  // short ramp length large signal and high adc gain will produce error codes
  double hardCnt = m_dacValues.back()*100;
  hardCnt = m_chip->sequencer->getRampLength();
  if(!m_chip->pixelRegistersSignalIsVarious("RmpFineTrm",utils::positionVectorToList(unsettledPxs))){
    uint32_t setting = m_chip->getPixelRegisterValue(to_string(unsettledPxs.front()),"RmpFineTrm");
    hardCnt /= (setting+1);
  }else{
    hardCnt /=1;
  }
  return hardCnt > 1000;
}


void CHIPTrimmer::setChipParts(std::string chipParts)
{
  SuS_LOG_STREAM(debug,log_id(),"setting chipParts = " << chipParts);
  m_chipParts = utils::getChipParts(chipParts);
}


std::string CHIPTrimmer::getGoodTrimmedPixelsStr(std::vector<double> & trimmedSlopes, string & rangeStr, const double aimSlope, const double maxAbsDiff, int & goodSlopesRel) const
{
  trimmedSlopes.clear();
  int goodSlopes = 0;
  for(const double & slope : m_finalSlopes)
  {
    //if(slope != 0.0)
    if(slope > 0.0)
    {
      trimmedSlopes.push_back(slope);

      if(fabs(slope-aimSlope) < maxAbsDiff){
        goodSlopes++;
      }
    }
  }

  int numTrimmed = trimmedSlopes.size();
  goodSlopesRel = 100.0*goodSlopes/numTrimmed;

  stringstream rangess;
  rangess << numTrimmed << " pixels trimmed - ";
  rangess << goodSlopes << " (" << goodSlopesRel << "%) in " << utils::setPrecision(utils::calcPerCent(maxAbsDiff,aimSlope),2) << "% range. ";
  rangeStr = rangess.str();

  stringstream ss;
  ss << m_chip->getPoweredPixels().size() << " pixels powered. ";
  ss << rangeStr;
  ss << getUntrimmablePixels().size() << " untrimmable pixels found";

  return ss.str();
}


void CHIPTrimmer::setOutputDir(std::string outputDir)
{
  m_outputDir = outputDir;
  if (m_outputDir.back()!='/') m_outputDir.append("/");

  utils::makePath(m_outputDir);
}


void CHIPTrimmer::addValuesToCalibrationDB(const std::string & name ,const std::vector<double> & values)
{
  //s_calibrationDB.addCalibrationSet(name,values);
}

void CHIPTrimmer::updateCalibrationDBGainConfig()
{
  /*
  int numPixels      = m_chip->getTotalNumPixels();
  string componentId = m_chip->getActiveComponentId();
  string gainStr     = m_gainConfigurator.getCurrentGainMode();
  auto gainParams    = m_gainConfigurator.getActiveConfigMap();

  //s_calibrationDB.initializeActiveCalibrationConfig(numPixels,componentId,gainStr,gainParams);
  */
}


void CHIPTrimmer::generateBinningInformation(const std::vector<uint32_t> & dacSettings, const std::vector<int> & rmpFineTrmSettings)
{
  std::string outDirReminder = m_outputDir;
  utils::makePath(outDirReminder);

  addValuesToCalibrationDB("FittedBinningIrampsettings",utils::convertVectorType<int,double>(rmpFineTrmSettings));

  auto allColPixels = getAllPixels();
  m_chip->removeUnpoweredPixels(allColPixels);
  const auto allColPixelsStr = utils::positionVectorToList(allColPixels);

  for(auto && rmpFineTrim : rmpFineTrmSettings)
  {
    m_outputDir = outDirReminder + "/RmpFineTrm" + to_string(rmpFineTrim);
    utils::makePath(m_outputDir);

    cout << "Measure RmpFineTrm " << rmpFineTrim << endl;
    m_chip->setPixelRegisterValue(allColPixelsStr,"RmpFineTrm",rmpFineTrim);

    // gets dnl information also for not measured pixels in order to be compatible for any pixel selection
    // during later operations
    //const auto dnlEvalsVec = generateBinningInformationForChipParts();

    //int pxIdx = 0;
   // for(auto && dnlEval : dnlEvalsVec){
   //   const auto calibParamName = DsscCalibrationDB::getBinningParamName(pxIdx++,rmpFineTrim);
   //   addValuesToCalibrationDB(calibParamName,utils::dnlValuesToSingleVec(dnlEval.dnlMap));
   // }
  }

  m_outputDir = outDirReminder;
}

void CHIPTrimmer::initChipProgressBar(const std::string & title, uint64_t numValues)
{
  m_chip->m_progressBar.setTitle(title);
  m_chip->m_progressBar.setRange(numValues);
}

const utils::DataHisto::DNLEvalVec & CHIPTrimmer::generateBinningInformationForChipParts()
{
  CHIPInterface::InjectionModeKeeper keeper(m_chip,CHIPInterface::ADC_INJ);

  m_dacValues = utils::getSettingsVector(m_startDacVal,m_endDacVal,m_numDacVals);

#ifdef HAVE_HDF5
  const string fileName = m_outputDir + "/" + utils::getLocalTimeStr() + "_DNLDataHistos.h5";
  utils::makePath(m_outputDir);
  // display one dimensional vector as 2 dimensional image
  DsscHDF5TrimmingDataWriter dataWriter(fileName);
  dataWriter.setMeasurementName("BinningMeasurement");
  dataWriter.addValueData("ClockDeskew",m_chip->getJTAGParam("Global Control Register","0","ClkDeskew_0"));
  dataWriter.addStringVectorData("RunCHIPParts",m_chipParts);
#endif

  initChipProgressBar("Measure Binning Values",m_chipParts.size() * m_numDacVals);

  SuS_LOG_STREAM(info, log_id(), "Measure Binning Values for following chipParts:");
  for (auto s : m_chipParts) SuS_LOG_STREAM(info, log_id(), s);
  for (auto p : m_chipParts) {
    SuS_LOG_STREAM(info, log_id(), "--- Current trimmingChipPart = " << p);
    m_chip->enableMonBusCols(p);
    m_currentRunPixels = m_chip->getSendingPixels<uint32_t>(p);  // TODO : remove unpowered pixels
    m_chip->enableInjection(true,utils::positionVectorToList(m_currentRunPixels));

    //measure all dnlValues, for all active pixels
    auto runDataHistos = m_chip->measureDNLValues(m_currentRunPixels,m_dacValues,m_startMemAddr,m_endMemAddr);

#ifdef HAVE_HDF5
    dataWriter.addHistoData("RUN_"+p+"_DNLHistograms",runDataHistos,m_currentRunPixels);
#endif

    if(!m_chip->runTrimming){
      break;
    }
  }

  const utils::DataHisto::DNLEvalVec & dnlEvalsVec = m_chip->getDNLEvalsVector();
  utils::DataHisto::exportDNLEvaluationsVec(dnlEvalsVec,m_outputDir + "/DNLEvaluation.txt",m_chip->getJTAGParam("Global Control Register","0","ClkDeskew_0"));
  return dnlEvalsVec;
}

// DIM numSettings x numPixels
std::vector<std::vector<double>> CHIPTrimmer::generateADCGainMap(const std::vector<uint32_t> irampSettings)
{
  std::vector<std::vector<double>> pixelADCGainSlopes;

#ifdef HAVE_HDF5
  utils::makePath(m_outputDir);
#endif

  CHIPInterface::InjectionModeKeeper keeper(m_chip,CHIPInterface::ADC_INJ);

  //const size_t numSettings = irampSettings.size();
  for(auto && iramp : irampSettings)
  {
    cout << endl << endl;
    cout << "#############***New Setting***##################" << endl;
    cout << "### Measuring RmpFineTrim Sweep Setting " << iramp << endl;
    cout << "################################################" << endl;

    m_chip->setPixelRegisterValue("all","RmpFineTrm",iramp);
    m_chip->programPixelRegs();

    generateSlopeInformationForChipParts();

    pixelADCGainSlopes.push_back(m_finalSlopes);

    // const auto calibParamName = DsscCalibrationDB::getADCSlopeParamName(iramp);
    // addValuesToCalibrationDB(calibParamName,m_finalSlopes);
    // addValuesToCalibrationDB(calibParamName,m_finalSlopes);

#ifdef HAVE_HDF5
      const string fileName = m_outputDir + "/" + utils::getLocalTimeStr() + "_ADCGainMap_RmpFineTrm"+ to_string(iramp) +".h5";
      DsscHDF5TrimmingDataWriter dataWriter(fileName);
      dataWriter.setMeasurementName("ADCGainMap");
      dataWriter.addVectorData("ADCGainMap",m_finalSlopes);
      dataWriter.addVectorData("IrampSetting",std::vector<double>(1,iramp));
      dataWriter.addVectorData("NumPixels",std::vector<double>(1,m_chip->totalNumPxs));
#endif

    if(!m_chip->runTrimming){
      break;
    }
  }

  return pixelADCGainSlopes;
}

std::vector<std::vector<double>> CHIPTrimmer::generateSlopeInformationForChipParts()
{
  m_dacValues = utils::getSettingsVector(m_startDacVal,m_endDacVal,m_numDacVals);

  const uint32_t LINSUM_X  = std::accumulate(m_dacValues.begin(), m_dacValues.end(), 0.0);
  const uint32_t LINSUM_XX = std::inner_product(m_dacValues.begin(), m_dacValues.end(), m_dacValues.begin(), 0.0);

  vector<vector<double> > binValues(m_chip->totalNumPxs,vector<double>(m_dacValues.size(),0.0));

  SuS_LOG_STREAM(info, log_id(), "Measure Injection Slope Values for following chipParts:");
  for (auto s : m_chipParts) SuS_LOG_STREAM(info, log_id(), s);
  for (auto p : m_chipParts) {
    SuS_LOG_STREAM(info, log_id(), "Current trimmingChipPart = " << p);
    m_chip->enableMonBusCols(p);
    m_currentRunPixels = m_chip->getSendingPixels<uint32_t>(p);  // TODO : remove unpowered pixels
    m_chip->enableInjection(true,utils::positionVectorToList(m_currentRunPixels));

    //measure all dnlValues, for all active pixels
    m_chip->measureIntDacSweepforSlopes(m_dacValues,m_currentRunPixels,binValues,m_startMemAddr,m_endMemAddr);

    if(!m_chip->runTrimming){
      break;
    }
  }

  const auto allColPixels = getAllPixels();
  const size_t numPxs = allColPixels.size();

#pragma omp parallel for
  for(size_t idx=0; idx<numPxs; idx++)
  {
    int px = allColPixels[idx];
    double slope = utils::linearRegression(LINSUM_X,LINSUM_XX,m_dacValues,binValues[px]);
    m_finalSlopes[px] = slope;
    double curvature = m_chip->calcCurvatureGraph(m_dacValues,binValues[px]);
    m_curvatureValues[px] = curvature;

// print measurement values during checking
//    if(idx++ < 10){
//      cout << px << " : slope " << slope << " - ";
//      for(const auto & value : binValues[px]){
//        cout << setw(8) << value << " ";
//      }
//      cout << endl;
//    }
  }

  m_chip->setFinalSlopes(m_finalSlopes);
  m_chip->setCurvatureValues(m_curvatureValues);

  return binValues;
}

