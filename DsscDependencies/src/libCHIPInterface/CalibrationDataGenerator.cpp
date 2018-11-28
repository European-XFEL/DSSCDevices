#include <stdlib.h>
#include "CalibrationDataGenerator.h"
#include "CHIPGainConfigurator.h"
#include "ConfigReg.h"
#include "Sequencer.h"

//#include <DsscCalibrationDB.h>

using namespace SuS;
using namespace std;


CalibrationDataGenerator::CalibrationDataGenerator(int numPixels) :
  m_pixelRegisters(nullptr),
  m_pxInjCalibMode("CHARGE_BUSINJ"),
  m_numPixels(numPixels),
  //m_noiseEnc(m_numPixels,0.0),
  m_noiseSigma(m_numPixels,0.0),
  m_pxGain(m_numPixels,0.0),
  m_slopes(m_numPixels),
  m_currentIrampSettings(m_numPixels,0),
  m_spectrumFitResults(m_numPixels,utils::SpectrumFitResult()),
  m_adcGainFitParamsLoaded(false),
  m_spectrumFitResultsLoaded(false),
  m_calibDataConfigLoaded(false),
  m_dnlValuesLoaded(false),
  m_measuredGainMapLoaded(false),
  m_pixelInjectionSlopesLoaded(false),
  m_pxInjectionCalibFactorsValid(false),
  m_meanSigma(0.0),
  m_noiseThresh(1.5)
//  m_db(nullptr)
{
  m_pxDelays.resize(m_numPixels);
  m_sCurves.resize(m_numPixels);
  m_gridHistoMaps.resize(2);
  useAllPixels();
  initGainMapParams();
}


void CalibrationDataGenerator::initGainMapParams()
{
#ifdef F1IO
  m_gainParamsMap["D0_EnMimCap"]     = 0;
  m_gainParamsMap["FCF_EnIntRes"]    = 1;
  m_gainParamsMap["FCF_EnCap"]       = 1;
  m_gainParamsMap["IntegrationTime"] = 30;
#else
  m_gainParamsMap["CSA_FbCap"]       = 3;
  m_gainParamsMap["CSA_Resistor"]    = 1;
  m_gainParamsMap["FCF_EnCap"]       = 1;
  m_gainParamsMap["IntegrationTime"] = 30;
#endif
}

void CalibrationDataGenerator::setPixelRegisters(ConfigReg *pxRegs)
{
  m_pixelRegisters=pxRegs;
  m_currentIrampSettings = m_pixelRegisters->getSignalValues("Control register","all","RmpFineTrm");
  setGainParamsMap();
  m_calibDataConfigLoaded=true;
}

void CalibrationDataGenerator::setGainParamsMap()
{
#ifdef F1IO
  m_gainParamsMap["D0_EnMimCap"]     = m_pixelRegisters->getSignalValue("Control register","0","D0_EnMimCap");
  m_gainParamsMap["FCF_EnIntRes"]    = m_pixelRegisters->getSignalValue("Control register","0","FCF_EnIntRes");
  m_gainParamsMap["FCF_EnCap"]       = m_pixelRegisters->getSignalValue("Control register","0","FCF_EnCap");
#else
  m_gainParamsMap["CSA_FbCap"]       = m_pixelRegisters->getSignalValue("Control register","0","CSA_FbCap");
  m_gainParamsMap["CSA_Resistor"]    = m_pixelRegisters->getSignalValue("Control register","0","CSA_Resistor");
  m_gainParamsMap["FCF_EnCap"]       = m_pixelRegisters->getSignalValue("Control register","0","FCF_EnCap");
#endif
}

void CalibrationDataGenerator::setIntegrationTime(int intTime)
{
  m_gainParamsMap["IntegrationTime"] = intTime;
}

void CalibrationDataGenerator::setPixelRegisterValue(const std::string & moduleStr, const std::string & signalName, uint32_t value)
{
  m_pixelRegisters->setSignalValue("Control register",moduleStr,signalName,value);
}


uint32_t CalibrationDataGenerator::getCoarseGainSetting(const std::string & signalName)
{
  if(!m_calibDataConfigLoaded) return 0;
  if(!m_pixelRegisters) return 0;

  return m_pixelRegisters->getSignalValue("Control register","0",signalName);
}


void CalibrationDataGenerator::calcPxDelaysFromIrmpGrid(int currDouble)
{
  if (m_gridHistoMaps[currDouble].empty()) {
    cout << "WARNING: no pxDelays calculated, no grid histograms loaded." << std::endl;
  }
//#pragma omp parallel for
  for (auto px=0; px<(int)m_currentPixels.size(); ++px) {
    m_pxDelays[px] = utils::calcPxDelaySteps(px,m_gridHistoMaps[currDouble]);
    if (m_pxDelays[px].empty()) {
      cout << "ERROR calculating pxDelays for px " << px << std::endl;
    }
  }
  cout << "CalibrationDataGenerator: calculated pixel delays for " << m_currentPixels.size() << " pixels." << std::endl;
}


void CalibrationDataGenerator::importGrid(int currDouble, string filename)
{
  m_gridHistoMaps[currDouble] = utils::DataHisto::importHistogramsFromASCII(filename, m_currentPixels, m_numPixels);
  cout << "CalibrationDataGenerator: read in " << m_gridHistoMaps[currDouble].size() << " histograms for "
    << m_currentPixels.size() << " pixels." << std::endl;
}


void CalibrationDataGenerator::exportPxDelays(string filename)
{
  vector<int> pxDelaySettings;
  string fullFilename = m_outputDir + "/" + filename;
  for (auto s=0; s<c_numPxDelaySteps; ++s) pxDelaySettings.push_back(s);
  if (m_pxDelays.empty()) {
    cout << "WARNING: exportPxDelays(): no pixel delays calculated yet, nothing exported." << endl;
    return;
  }
  for (auto px : m_currentPixels) {
    std::vector<int> pixels(c_numPxDelaySteps,px);
    utils::exportDataFile(fullFilename, "#px \t delaySet \t delay", pixels, pxDelaySettings, m_pxDelays[px], px!=0);
  }
  cout << "Exported pxDelays to " << fullFilename << endl;
}


void CalibrationDataGenerator::calcNoiseFromInvErrFunc()
{
  //m_encHisto.clear();
  m_sigmaHisto.clear();
  cout << "INFO: using gridHistoMap to calculate noise." << std::endl;
  if (m_pixelRegisters) {
//#pragma omp parallel for
    for (auto px=0; px<(int)m_currentPixels.size(); ++px) {
      int irmp = m_pixelRegisters->getSignalValue("Control register", std::to_string(px),"RmpFineTrm");
      int cd   = m_pixelRegisters->getSignalValue("Control register", std::to_string(px),"RmpCurrDouble");
      std::vector<utils::DataHisto> histos;
      auto & histoMapToUse = m_gridHistoMaps[cd];
      for (int p=0; p<16; ++p) {
        //int idx = currPixel*(16*64) + setting1 + setting2*64;
        histos.push_back(histoMapToUse[px*16*64+irmp+p*64]);
      }
      m_sCurves[px] = utils::DataHisto::calcStepBoarderFractions(histos,false);
      //double enc = calcNoiseFromInvErrFunc(m_pxDelays[px],m_sCurves[px]) * m_gain;
      //m_noiseEnc[px] = enc;
      double sigma = calcNoiseFromInvErrFunc(m_pxDelays[px],m_sCurves[px]);
      m_noiseSigma[px] = sigma;
      //if (enc < m_noiseThresh) m_encHisto.add(enc);
      if (sigma < m_noiseThresh) m_sigmaHisto.add(sigma);
    }
  } else {
    cout << "WARNING: could not calcNoiseFromInvErrFunc, no pixelRegisterFile loaded." << std::endl;
  }
  cout << "INFO: Noise calculation finished." << std::endl;
  //m_meanENC = m_encHisto.calcMean();
  //cout << "INFO: meanENC = " << m_meanENC << std::endl;
  m_meanSigma= m_sigmaHisto.calcMean();
  cout << "INFO: meanSigma = " << m_meanSigma << std::endl;
}


void CalibrationDataGenerator::calcNoiseFromPxDelayHistos(const std::vector<std::vector<utils::DataHisto> > & pxDelayHistos)
{
  for (auto px : m_currentPixels) {
    m_sCurves[px] = utils::DataHisto::calcStepBoarderFractions(pxDelayHistos[px],false);
    double sigma = calcNoiseFromInvErrFunc(m_pxDelays[px],m_sCurves[px]);
    //double enc = sigma * m_gain;
    m_noiseSigma[px] = (std::isnan(sigma)) ? 0.0 : sigma;
    //m_noiseEnc[px] = sigma * m_gain;
    //if (enc < m_noiseThresh) m_encHisto.add(enc);
    if (sigma < m_noiseThresh) m_sigmaHisto.add(sigma);
  }
  cout << "INFO: Noise calculation finished." << std::endl;
  //m_meanENC = m_encHisto.calcMean();
  //cout << "INFO: meanENC = " << m_meanENC << std::endl;
  m_meanSigma= m_sigmaHisto.calcMean();
  cout << "INFO: meanSigma = " << m_meanSigma << std::endl;
}


void CalibrationDataGenerator::exportNoiseInvErrFunc(string filename)
{
  cout << "INFO: exporting noise values." << std::endl;
  std::vector<int> pixels(m_numPixels,0);
  for (auto px=0; px<m_numPixels; ++px) pixels[px] = px;
  //utils::exportDataFile(m_outputDir + "/" + filename, "#px \t enc \t", pixels, m_noiseEnc);
  utils::exportDataFile(m_outputDir + "/" + filename, "#px \t enc \t", pixels, m_noiseSigma);
  cout << "INFO: exporting noise finished." << std::endl;
}


void CalibrationDataGenerator::useAllPixels()
{
  m_currentPixels.resize(m_numPixels);
  for (auto px=0; px<m_numPixels; ++px) m_currentPixels[px] = px;
}


double CalibrationDataGenerator::calcNoiseFromDelaySweepHistos(const std::vector<double> & delayValues, const utils::DataHistoVec & pixelHistograms)
{
  const auto stepBoarderValues = utils::DataHisto::calcStepBoarderFractions(pixelHistograms);
  return calcNoiseFromInvErrFunc(delayValues,stepBoarderValues);
}


double CalibrationDataGenerator::calcNoiseFromInvErrFunc(const std::vector<double> & delayValues, const std::vector<double> & sCurve)
{
  static constexpr double SQRT2 = sqrt(2.0);
  const int numValues = sCurve.size();
  if (sCurve.size() != delayValues.size()) {
    cout << "WARNING: calcNoiseFromInvErrFunc(), vector sizes do not match " <<  delayValues.size() << "/" << sCurve.size() << "." << std::endl;
    return 0.0;
  }

  vector<double> xValues;
  vector<double> yValues;

  // inverseError Function good between -0.0 and +0.9
  for(int i=0; i<numValues; i++){
    double corrStepValue = 2.0 * sCurve[i] - 1.0;
    if(fabs(corrStepValue) < 0.9){
      xValues.push_back(delayValues[i]);
      yValues.push_back(SQRT2 * utils::inverseErf(corrStepValue));
    }
  }

  double sigma = 1.0 / utils::linearRegression(xValues,yValues);
  // returning abs(sigma) also allows for inputting the "mirrored" sCurve
  return abs(sigma);
}


void CalibrationDataGenerator::importPxDelays(string filename)
{
  ifstream in(filename);
  string line;
  int px = 0;
  int pxDelaySetting = 0;
  double pxDelay = 0.0;
  int lineCnt = 0;
  while(getline(in,line)) {
    lineCnt++;
    stringstream iss(line);
    iss >> px;
    iss >> pxDelaySetting;
    iss >> pxDelay;
    if (pxDelaySetting==0) m_pxDelays[px].resize(c_numPxDelaySteps);
    m_pxDelays[px][pxDelaySetting] = pxDelay;
  }
  cout << "INFO: importPxDelays(): imported " << lineCnt << " lines." << std::endl;
}


void CalibrationDataGenerator::importPxGains(string filename)
{
  ifstream in(filename);
  string line;
  int px = 0;
  double gain = 0.0;
  int lineCnt = 0;
  while(getline(in,line)) {
    lineCnt++;
    stringstream iss(line);
    iss >> px;
    iss >> gain;
    iss >> gain;  // gain in 2nd col
    m_pxGain[px] = gain;
  }
  cout << "INFO: importPxGains(): imported " << lineCnt << " lines." << std::endl;
}


bool CalibrationDataGenerator::isNoiseOutlier(int px)
{
  //return (abs(m_noiseEnc[px]-m_meanENC) > m_meanENC/10);
  return (abs(m_noiseSigma[px]-m_meanSigma) > m_meanSigma/10);
}


void CalibrationDataGenerator::exportPxDelaysToDB()
{
/*
  if (!m_db) {
    cout << "ERROR: no database loaded." << endl;
    return;
  }
  // Add Calibration Parameters as 1 dim vectors
  auto activeConfig = m_db->getActiveCalibrationSet();

  // would it not be better to retrieve a reference of a vector that we can directly write into
  // instead of copying the vector again?
  std::vector<double> pixelDelays(4096*16,0.0);
  int i = 0;
  for (auto px : m_pxDelays) {
    for (auto pd : px) {
      pixelDelays[i++] = pd;
    }
  }
  activeConfig->addCalibrationParamSet("PixelDelays",pixelDelays);
  */
}


void CalibrationDataGenerator::importPxDelaysFromDB()
{
  /*
  auto & pixelDelays = m_db->getActiveCalibrationSet()->getCalibrationParamSet("PixelDelays");
  int i = 0;
  for (auto pd : pixelDelays) {
    m_pxDelays[i/c_numPxDelaySteps][i%c_numPxDelaySteps] = pd;
  }
  cout << "INFO: importPxDelays(): imported " << i << " pixel delays from DB " << m_db->getFilename() << "." << std::endl;
  */
}


void CalibrationDataGenerator::loadDB(string filename)
{
//  if (m_db) delete m_db;
//  m_db = new DsscCalibrationDB(filename);
}


void CalibrationDataGenerator::genNewCalibSetConfig()
{
  //string component = "DSSC_F2_Test";
  //string version = "v0.0.0.1";
  //int minorVersion = 1;

  // initialize Configuration for Calibration Set
//  SuS::DsscCalibrationSetConfig config;
//  config.setParamValue("numPixels","4096");
//  config.setParamValue("active","J");

  // add new Calibration Set with date time version and componentID (ASIC or Sensor...)
//  m_db->addNewCalibrationSetAndSetActive(SuS::DsscCalibrationDB::generateNewConfigID(version,minorVersion),
//      config.getParameterValues());
}


std::vector<double> CalibrationDataGenerator::genNoiseEnc() const
{
  std::vector<double> noiseEnc;
  noiseEnc.resize(m_numPixels);
  for (auto px : m_currentPixels) {
    noiseEnc[px] = m_noiseSigma[px] * m_pxGain[px];
  }
  return noiseEnc;
}


std::string CalibrationDataGenerator::saveSpectrumFitResults(const std::string & exportFilename)
{
  string fullFilename = m_outputDir + "/" + exportFilename + ".txt";
  utils::dumpFitResultsToASCII(fullFilename, getAllPixels(), m_spectrumFitResults);
  m_lastOutput = fullFilename;
  return m_lastOutput;
}


void CalibrationDataGenerator::fitSpectra(utils::DataHistoVec & pixelHistograms, const string &exportFilename, const utils::DataHisto::DNLEvalMap & dnlEvalMap)
{
  const bool dnlsValid = !dnlEvalMap.empty();
  const size_t numPixels = m_currentPixels.size();

  if(pixelHistograms.size() != numPixels){
    cout << "CalibrationDataGenerator ERROR: PixelHistograms.size() does not fit to pixels size!!" << pixelHistograms.size() <<"/" << numPixels << endl;
    //return;
  }

  std::atomic_int pixelCnt;
  pixelCnt = 0;

  if(dnlsValid){
    const auto dnlValuesMapVec = utils::DataHisto::toDnlValuesMapVec(dnlEvalMap);
#pragma omp parallel for
    for(size_t idx=0; idx<numPixels; idx++){
      int pixel = m_currentPixels[idx];
      m_spectrumFitResults[pixel] = pixelHistograms[idx].fitSpectrum(dnlValuesMapVec[pixel]);
      cout << "++++ Fitted Spectrum "  << std::setw(5) << pixelCnt++ << "/" << numPixels << endl;
    }
  }
  else
  {
#pragma omp parallel for
    for(size_t idx=0; idx<numPixels; idx++)
    {
      int pixel = m_currentPixels[idx];
      m_spectrumFitResults[pixel] = pixelHistograms[idx].fitSpectrum();
      cout << "++++ Fitted Spectrum "  << std::setw(5) << pixelCnt++ << "/" << numPixels << endl;
    }
  }

  saveSpectrumFitResults(exportFilename);
  m_spectrumFitResultsLoaded = true;
}


// m_currentPixels must match pixelHistograms (map)
void CalibrationDataGenerator::fitSpectra(utils::DataHistoMap & pixelHistograms,const string & exportFilename, const utils::DataHisto::DNLEvalMap & dnlEvalMap)
{
  const bool dnlsValid = !dnlEvalMap.empty();
  const size_t numPixels = m_currentPixels.size();

  if(pixelHistograms.size() != numPixels){
    cout << "CalibrationDataGenerator ERROR: PixelHistograms.size() does not fit to pixels size!!" << pixelHistograms.size() <<"/" << numPixels << endl;
    //return;
  }

  std::atomic_int pixelCnt;
  pixelCnt = 0;

  if(dnlsValid){
    const auto dnlValuesMapVec = utils::DataHisto::toDnlValuesMapVec(dnlEvalMap);
#pragma omp parallel for
    for(size_t idx=0; idx<numPixels; idx++){
      int pixel = m_currentPixels[idx];
      m_spectrumFitResults[pixel] = pixelHistograms[pixel].fitSpectrum(dnlValuesMapVec[pixel]);
      utils::CoutColorKeeper keeper(utils::STDCYAN);
      cout << "++++ Fitted Spectrum "  << std::setw(5) << pixelCnt++ << "/" << numPixels << endl;
    }
  }
  else
  {
#pragma omp parallel for
    for(size_t idx=0; idx<numPixels; idx++)
    {
      int pixel = m_currentPixels[idx];
      m_spectrumFitResults[pixel] = pixelHistograms[pixel].fitSpectrum();
      utils::CoutColorKeeper keeper(utils::STDCYAN);
      cout << "++++ Fitted Spectrum "  << std::setw(5) << pixelCnt++ << "/" << numPixels << endl;
    }
  }

  saveSpectrumFitResults(exportFilename);
  m_spectrumFitResultsLoaded = true;
}


// pixel histos must be in order of m_currentPixels
void CalibrationDataGenerator::calcSlopes(const std::vector<double> & settings, const std::vector<std::vector<utils::DataHistoVec> > & pxHistos)
{
//  cout << "INFO: Calculating measurement slopes for " << m_currentPixels.size() << " pixels ." << endl;
//  double s_x = 0.0;
//  double s_xx = 0.0;
//
//  for(unsigned i=0; i<settings.size(); i++){
//    s_x  += settings[i];
//    s_xx += settings[i]*settings[i];
//  }
//
//#pragma omp parallel for
//  for (auto i=0; i<m_currentPixels.size(); ++i) {
//    int px = m_currentPixels[i];
//    for (auto measVec : pxHistos) {
//      m_slopes[px].clear();
//      std::vector<double> yVals;
//      yVals.reserve(measVec.size());
//      for (auto histo : measVec)
//        yVals.push_back(histo.calcMean());
//      m_slopes[px].push_back(utils::linearRegression(s_x, s_xx, settings, yVals));
//    }
//  }
}


std::vector<int> CalibrationDataGenerator::getAllPixels() const
{
  std::vector<int> allPx(m_numPixels,0);
  for (int p=0; p<m_numPixels; ++p) allPx[p] = p;
  return allPx;
}

int CalibrationDataGenerator::calcNewADCSettingUsingMeanFitParams(int setting, double relativeGainChange)
{
  return calcNewADCSetting(m_meanADCGainFitParameters,setting,relativeGainChange);
}


double CalibrationDataGenerator::calcGainForSetting(const utils::ADCGainFitResult & fitResult, double refGainEv, int refSetting, int newSetting)
{
  const double * params = fitResult.data()+1;
  double set = refSetting;
  double refGainValue = utils::hyperbel(&set,params);

  set = newSetting;
  double newGainValue = utils::hyperbel(&set,params);

  double relGainChange = refGainValue/newGainValue;

  // Iramp sweep gain map can not be used to compute GainEv Values
  return refGainEv * relGainChange;
}


int CalibrationDataGenerator::calcNewADCSetting(const utils::ADCGainFitResult & fitResult, int setting, double relativeGainChange)
{
  const double * params = fitResult.data()+1;
  double set = setting;
  double currentGain = utils::hyperbel(&set,params);
  double targetValue = currentGain * relativeGainChange;
  double newSetting = utils::inverseHyperbel(targetValue,params);

  //cout << "New Setting for pixel " << fitResult[0] << " is " << newSetting << " gainChange = " << relativeGainChange << " current gain = " << currentGain << " for setting " << setting << endl;
  int bestSetting = round(newSetting);
  return bestSetting;
}


void CalibrationDataGenerator::setADCGainMap(const utils::ADCGainFitResultsVec &gainMap)
{
  m_pixelADCGainFitParameters = gainMap;
  if(gainMap.size() == 65537){
    m_meanADCGainFitParameters = gainMap.back();
    m_pixelADCGainFitParameters.pop_back();
  }else{
    int numGoodPixels;
    const auto meanRmsVec = utils::calcMeanRMSADCGainFitParams(m_pixelADCGainFitParameters,numGoodPixels);
    // fill mean fit params
    m_meanADCGainFitParameters[0] = 65536;
    for(size_t i=0; i<meanRmsVec.size(); i++){
      m_meanADCGainFitParameters[i+1] = meanRmsVec[i].mean;
    }
    cout << "CalibrationDataGenerator: Mean ADCGain FitParameters loaded" << endl;
  }
  m_adcGainFitParamsLoaded = !m_pixelADCGainFitParameters.empty();
}


void CalibrationDataGenerator::setDNLValuesMap(const utils::DataHisto::DNLEvalMap & dnlEvalMap)
{
  m_dnlEvalsMap = dnlEvalMap;
  m_dnlValuesLoaded = !dnlEvalMap.empty();
}


void CalibrationDataGenerator::setSpectrumFitResults(const utils::FitResultsVec & fitResults){
  m_spectrumFitResults = fitResults;
  m_spectrumFitResultsLoaded = !fitResults.empty();
}


void CalibrationDataGenerator::setMeasuredSpectrumGains(const std::vector<double> & measuredGains, const std::vector<uint32_t> & irampSettings)
{
  const size_t numSettings = irampSettings.size();
  const size_t numPixels   = measuredGains.size() / numSettings;

  m_measuredGainMap.resize(numPixels);
// vector is parallel save, map probably not but each map is handled in the same thread
#pragma omp parallel for
  for(size_t px=0; px<numPixels; px++){
    auto & pxGainMap = m_measuredGainMap[px];
    pxGainMap.clear();
    for(size_t set=0; set<numSettings; set++){
      size_t idx = px*numSettings + set;
      int setting = irampSettings[set];
      pxGainMap[setting] = measuredGains[idx];
    }
  }

  m_measuredGainMapLoaded = !m_measuredGainMap.empty();
}


std::vector<uint32_t> CalibrationDataGenerator::getCurrentPixelsIrampSettings()
{
  return m_currentIrampSettings;
}


std::vector<uint32_t> & CalibrationDataGenerator::computeADCGainConfiguration(double targetGain)
{
  if(!m_spectrumFitResultsLoaded)
  {
    cout << "m_spectrumFitResultsLoaded not loaded" << endl;
    return m_currentIrampSettings;
  }

  const size_t numPixels = m_spectrumFitResults.size();
  std::vector<double> pixelGainValues(numPixels,targetGain);
#pragma omp parallel for
  for(size_t pixel=0; pixel<numPixels; pixel++)
  {
    auto & pxFitResult = m_spectrumFitResults[pixel];
    pixelGainValues[pixel] = pxFitResult.getGain();
  }
  return computeADCGainConfiguration(targetGain,pixelGainValues);
}


std::vector<uint32_t> & CalibrationDataGenerator::computeADCGainConfiguration(double targetGain, const std::vector<double> & pixelGainValues)
{
  cout << "Current Target Type Information = " << endl;
  utils::SpectrumFitResult::s_currentTarget.print();

  if(!isCalibrationDataConfigLoaded()){
    cout << "Can not compute calibrated iramp configuration without information about ADCGainSetting of spectrumFitResults" << endl;
    return m_currentIrampSettings;
  }

  if(m_pixelADCGainFitParameters.size() != m_currentIrampSettings.size()){
    cout << "m_pixelADCGainFitParameters.size() != rmpFineTrimSettings.size()" << endl;
    return m_currentIrampSettings;
  }

  if(m_spectrumFitResults.size() != m_currentIrampSettings.size())
  {
    cout << "m_spectrumFitResults.size() != rmpFineTrimSettings.size()" << endl;
    return m_currentIrampSettings;
  }

  // will change configuration
  setPixelRegistersChanged();

  m_irampLimitPixels.clear();
  m_targetGainComputed.clear();
  m_targetGainMeasured.clear();
  m_targetGainComputed.resize(65536,0.0);
  m_targetGainMeasured.resize(65536,0.0);

  const size_t numPixels = m_currentIrampSettings.size();

#pragma omp parallel for
  for(size_t pixel=0; pixel<numPixels; pixel++)
  {
    /* first try to use mean values for fit params
    const auto & fitParams =  m_pixelADCGainFitParameters[pixel];
    if(pixel != fitParams[0]){
      cout << "ERROR: FitParam Pixel != pixel: " <<  fitParams[0] << "/" << pixel << endl;
      //break;
    }
    */

    //high keV Value is a low gain value 1.0keV/Bin measured --> 2.0keV/Bin target --> gain by half
    double pixelGaineEv = pixelGainValues[pixel];
    if(pixelGaineEv != 0.0)
    {
      double gainChange = pixelGaineEv/targetGain;
      int gainSetting   = m_currentIrampSettings[pixel];
     //int newSetting   = calcNewADCSettingUsingMeanFitParams(gainSetting,gainChange);
      int newSetting   = calcNewADCSetting(m_pixelADCGainFitParameters[pixel],gainSetting,gainChange);
      if(newSetting<0 || newSetting>63)
      {
#pragma omp critical
        m_irampLimitPixels.push_back(pixel);
        newSetting = std::min(63,std::max(0,newSetting));
      }

      //m_targetGainComputed[pixel] = calcGainForSetting(m_meanADCGainFitParameters,pixelGaineEv,gainSetting,newSetting);
      m_targetGainComputed[pixel] = calcGainForSetting(m_pixelADCGainFitParameters[pixel],pixelGaineEv,gainSetting,newSetting);
      m_targetGainMeasured[pixel] = getMeasuredGainForSetting(pixel,newSetting);

      m_currentIrampSettings[pixel] = newSetting;
    }
  }

  if(m_pixelRegisters){
    for(int pixel = 0; pixel<getNumPixels(); pixel++){
      setPixelRegisterValue(to_string(pixel),"RmpFineTrm",m_currentIrampSettings[pixel]);
    }
  }

  std::sort(m_irampLimitPixels.begin(),m_irampLimitPixels.end());
  if(!m_irampLimitPixels.empty()){
    cout << "Warning: Calibration for gain " << targetGain << "eV/Bin probably not optimal: "<< endl;
    cout << "Warning:" << m_irampLimitPixels.size() << " pixels are at setting limit: " << utils::positionVectorToList(m_irampLimitPixels).substr(0,200) + " ..." << endl;
  }

  return m_currentIrampSettings;
}


double CalibrationDataGenerator::getMeasuredGainForSetting(uint32_t pixel, uint32_t setting)
{
  if(m_measuredGainMapLoaded){
    if(m_measuredGainMap.size() > pixel){
      auto & pxGainMap = m_measuredGainMap[pixel];
      if(pxGainMap.find(setting) != pxGainMap.end()){
        return pxGainMap[setting];
      }
    }
  }
  //cout << "Warning: setting out of measurement range - no gain value was measured for pixel " << setw(5) << pixel << "/" << setw(2) << setting << endl;
  //cout << "Will compute target gain setting" << endl;
  return 0.0;//calcGainForSetting(m_meanADCGainFitParameters,setting);
}

DsscGainParamMap CalibrationDataGenerator::getGainParamsMap(ConfigReg * pixelReg, int integrationTime)
{
  if(!pixelReg->moduleSetExists("Control register")) return DsscGainParamMap();
  const auto names = DsscGainParamMap::s_gainModeParamNames;

  std::vector<uint32_t> paramValues;
  paramValues.push_back(pixelReg->getSignalValue("Control register","0",names[0]));
  paramValues.push_back(pixelReg->getSignalValue("Control register","0",names[1]));
  paramValues.push_back(pixelReg->getSignalValue("Control register","0",names[2]));
  paramValues.push_back(integrationTime);

  return DsscGainParamMap(paramValues);
}


void CalibrationDataGenerator::setCurrentIrampSettings(const std::vector<uint32_t> & irampSettings, const std::vector<uint32_t> & activePixels)
{
  if(m_currentIrampSettings.size() != (size_t)m_numPixels){
    m_currentIrampSettings.resize(m_numPixels,0);
  }

  if(irampSettings.size() != activePixels.size()){
    cout << "ERROR: irampSettings.size() != pixels.size() : " << irampSettings.size() << " / " << activePixels.size() << endl;
    return;
  }
  const size_t numPixels = activePixels.size();
#pragma omp parallel for
  for(size_t idx = 0; idx < numPixels; idx++){
    m_currentIrampSettings[activePixels[idx]] = irampSettings[idx];
  }
  m_calibDataConfigLoaded = true;
}


void CalibrationDataGenerator::updateGainParamsMap(const SuS::DsscGainParamMap & gainParams)
{
  cout << "CalibrationDataGenerator gain ParamsMap updated" << endl;
  for(auto && elem : gainParams){
    m_gainParamsMap[elem.first] = elem.second;
  }
}


void CalibrationDataGenerator::setPixelInjectionFactors(const std::vector<double> & pxInjectionFactors)
{
  m_pxInjectionCalibFactorsValid = false;

  if(m_pxInjectionCalibFactors.size() != (size_t)m_numPixels){
    m_pxInjectionCalibFactors.resize(m_numPixels,0.0);
  }

  if(pxInjectionFactors.size() != (size_t)m_numPixels){
    return;
  }

  std::copy(pxInjectionFactors.begin(),pxInjectionFactors.end(),m_pxInjectionCalibFactors.begin());

  m_pxInjectionCalibFactorsValid = true;
}


void CalibrationDataGenerator::setPixelInjectionSlopes(const std::vector<double> & pxInjectionSlopes, const std::vector<uint32_t> & activePixels)
{
  m_pixelInjectionSlopesLoaded = false;

  if(m_pxInjectionSlopes.size() != (size_t)m_numPixels){
    m_pxInjectionSlopes.resize(m_numPixels,0.0);
  }

  if(pxInjectionSlopes.size() != activePixels.size()){
    cout << "ERROR: irampSettings.size() != pixels.size() : " << pxInjectionSlopes.size() << " / " << activePixels.size() << endl;
    return;
  }
  const size_t numPixels = activePixels.size();
#pragma omp parallel for
  for(size_t idx = 0; idx < numPixels; idx++){
    m_pxInjectionSlopes[activePixels[idx]] = pxInjectionSlopes[idx];
  }

  m_pixelInjectionSlopesLoaded = true;
}

void CalibrationDataGenerator::compareToLoadedGainSettings(const SuS::DsscGainParamMap & gainParams,const std::vector<uint32_t> & irampSettings, const std::vector<uint32_t> & activePixels)
{
  bool gainMapSame = (m_gainParamsMap == gainParams);

  if(!gainMapSame){
    cout << "################################" << endl;
    cout << "WARNING: Gain Maps are not equal" << endl;
  }

  size_t numActivePixels = activePixels.size();
  bool irampsSame;
  if(numActivePixels != m_currentPixels.size()){
    std::vector<uint32_t> activePixelsSettings(numActivePixels);
    for(size_t idx=0; idx<numActivePixels; idx++){
      activePixelsSettings[idx] = m_currentIrampSettings[activePixels[idx]];
    }
    irampsSame = utils::vectors_are_identical(irampSettings,activePixelsSettings);
  }else{
    irampsSame = utils::vectors_are_identical(irampSettings,m_currentIrampSettings);
  }

  if(!irampsSame){
    cout << "################################" << endl;
    cout << "WARNING: Iramp Vectors are not equal" << endl;
  }
}


void CalibrationDataGenerator::updatePixelInjectionSlopeCompareFactors(const std::vector<double> & measurementSlopes,  const std::vector<uint32_t> & activePixels, const SuS::DsscGainParamMap & gainParamsMap)
{
  if(!m_pixelInjectionSlopesLoaded){
    cout << "####Error can not compute pixel injection slope compare factors without pixel injection slope inforamtion" << endl;
    return;
  }
  if(m_pxInjectionSlopeCompareGain != gainParamsMap)
  {
    std::cout << "CalibDataGenerator: WARNING gain parameters have changed" << std::endl;
    std::cout << "from:\n";
    m_pxInjectionSlopeCompareGain.print();
    std::cout << "to:\n";
    gainParamsMap.print();
  }

  m_pxInjectionSlopeCompareGain = gainParamsMap;
  m_pxInjectionSlopeCompareFactors.resize(utils::s_totalNumPxs);

  const uint numPixels = activePixels.size();
  for(size_t idx = 0; idx < numPixels; idx++){
    uint imagePixel = activePixels[idx];
    m_pxInjectionSlopeCompareFactors[imagePixel] = m_pxInjectionSlopes[imagePixel] / measurementSlopes[idx];
  }
}


utils::InjectionCalibrationData CalibrationDataGenerator::computePixelInjectionCalibrationData()
{
  m_pxInjectionCalibFactorsValid = false;

  if(m_pxInjectionCalibFactors.size() != (size_t)m_numPixels){
    m_pxInjectionCalibFactors.resize(m_numPixels,0.0);
  }

  utils::InjectionCalibrationData calibData(m_numPixels);

  if(!m_pixelInjectionSlopesLoaded){
    cout << "####Error can not compute pixel injection calibration factors without pixel injection slope inforamtion" << endl;
    return calibData;
  }

  if(!m_spectrumFitResultsLoaded){
    cout << "####Error can not compute pixel injection calibration factors without spectrum fit results for injection sweep configuration loaded" << endl;
    return calibData;
  }

#pragma omp parallel for
  for(int pixel = 0; pixel < m_numPixels; pixel++){
    const auto & pixelFitResult = m_spectrumFitResults[pixel];
    const double & pixelSlope = m_pxInjectionSlopes[pixel];
    auto & pixelCalibData = calibData[pixel];
    pixelCalibData[0] = pixel;
    if(pixelSlope != 0.0){
      double pixelGain     = pixelFitResult.getGain();
      double pixelGainErr  = pixelFitResult.getGainError();
      double pixelSlopeErr = 0.1*pixelSlope;

      // CurrentGain = CalibGain * CalibSlope / CurrentSlope
      //           --> CalibFactor = CalibGain * CalibSlope
      // if CurrentGain is higher than during measurement, eV value must become smaller:
      //           1000eV/Bin * 1.0 / 1.2 = 833ev/Bin
      //
      // ATTENTION: for the trimming algorithm, the inverse must be used
      //
      double calibFactor    = pixelGain*pixelSlope;
      double calibFactorErr = calibFactor * sqrt(pow(pixelGainErr/pixelGain,2)+pow(0.1,2));
      pixelCalibData[2] = calibFactorErr;
      pixelCalibData[3] = pixelSlope;
      pixelCalibData[4] = pixelSlopeErr;
      pixelCalibData[5] = pixelGain;
      pixelCalibData[6] = pixelGainErr;

      if(calibFactor > 1.0 && calibFactor < 1000)
      {
        m_pxInjectionCalibFactors[pixel] = calibFactor;
        pixelCalibData[1] = calibFactor;
      }else{
        m_pxInjectionCalibFactors[pixel] = 0;
        pixelCalibData[1] = 0;
      }
    }
  }

  if(isPixelInjectionSlopeCompareFactorsValid())
  {
#pragma omp parallel for
    for(int pixel = 0; pixel < m_numPixels; pixel++){+
      m_pxInjectionCalibFactors[pixel] *= m_pxInjectionSlopeCompareFactors[pixel];
    }
  }

  m_pxInjectionCalibFactorsValid = true;
  return calibData;
}


