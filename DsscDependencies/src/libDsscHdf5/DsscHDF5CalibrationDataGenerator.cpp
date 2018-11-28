#include <iostream>
#include "DataHisto.h"
#include "CHIPGainConfigurator.h"

#include "DsscHDF5CalibrationDataGenerator.h"

using namespace std;


DsscHDF5CalibrationDataGenerator::DsscHDF5CalibrationDataGenerator()
  : CalibrationDataGenerator()
{
}


bool DsscHDF5CalibrationDataGenerator::importPxInjectionCalibFactors(const std::string & calibFileName)
{
  if(!utils::checkFileExists(calibFileName)){
    cout << "File does not exist:" << calibFileName << endl;
    return false;
  }

  DsscHDF5TrimmingDataReader dataReader(calibFileName);

  if(dataReader.getMeasurementName() == "PixelInjectionCalibrationFactors")
  {
    m_pxInjCalibMode = dataReader.getStringData("pxInjCalibMode");
    const auto pxInjectionCalibFactors = dataReader.getDataVector("pxInjectionCalibFactors");
    setPixelInjectionFactors(pxInjectionCalibFactors);
  }
  else
  {
    cout << "CalibrationDataGenerator Error: PixelInjectionCalibrationFactors can not loaded from this type of file: " << dataReader.getMeasurementName() << endl;
  }

  return isPixelInjectionSlopesLoaded();
}

void DsscHDF5CalibrationDataGenerator::compareInjectionSlopes(const std::string & slopeFileName)
{
  if(!utils::checkFileExists(slopeFileName)){
    cout << "File does not exist:" << slopeFileName << endl;
    return;
  }

  if(!isPixelInjectionSlopesLoaded()){
    cout << "Error: DsscHDF5CalibrationDataGenerator, can not compare injection slopes if no injection slopes are loaded" << endl;
    return;
  }

  const auto modInfoRem = DsscHDF5Writer::s_writerModuleInfo;

  DsscHDF5TrimmingDataReader dataReader(slopeFileName);
  const auto measurementType = dataReader.getMeasurementName();
  if(measurementType == "CHARGE_BUSINJ Sweep")
  {
    const double numMeasurements = dataReader.readDataValue<double>("numMeasurements");
    const auto gainParamsMap     = dataReader.readGainParamsMap();
    const auto irampSettings     = dataReader.readRmpFineTrmSettings();
    const auto activePixels      = dataReader.readPixels();
    const auto measurementSlopes = dataReader.getDataVector("MeasurementSlopes");
    const auto measurementSettings = dataReader.getDataVector("MeasurementSettings");
    std::string otherCalibMode = measurementType.substr(0,measurementType.find_first_of(' '));
    if(measurementSettings.size() != (size_t)numMeasurements){
      cout << "ERROR: Measurement settings do not fit to number of settings, should be 1: " <<measurementSettings.size() << endl;
    }
    if(otherCalibMode != m_pxInjCalibMode){
      cout << "WARNING: files have been generated with different injection modes " <<  otherCalibMode << "/" << m_pxInjCalibMode << endl;
    }

    if(modInfoRem != DsscHDF5Writer::s_writerModuleInfo){
      cout << "WARNING: files have been generated with different Modules " << modInfoRem.getInfoStr()  <<  DsscHDF5Writer::s_writerModuleInfo.getInfoStr() << endl;
    }
    updatePixelInjectionSlopeCompareFactors(measurementSlopes,activePixels,gainParamsMap);
  }
}


bool DsscHDF5CalibrationDataGenerator::importPixelInjectionSlopes(const std::string & slopeFileName)
{
  if(!utils::checkFileExists(slopeFileName)){
    cout << "File does not exist:" << slopeFileName << endl;
    return false;
  }

  DsscHDF5TrimmingDataReader dataReader(slopeFileName);
  const auto measurementType = dataReader.getMeasurementName();
  if(measurementType == "CHARGE_BUSINJ Sweep")
  {
    const double numMeasurements = dataReader.readDataValue<double>("numMeasurements");
    const auto gainParamsMap     = dataReader.readGainParamsMap();
    const auto irampSettings     = dataReader.readRmpFineTrmSettings();
    const auto activePixels      = dataReader.readPixels();
    const auto measurementSlopes = dataReader.getDataVector("MeasurementSlopes");
    const auto measurementSettings = dataReader.getDataVector("MeasurementSettings");
    m_pxInjCalibMode = measurementType.substr(0,measurementType.find_first_of(' '));
    if(measurementSettings.size() != (size_t)numMeasurements){
      cout << "ERROR: Measurement settings do not fit to number of settings, should be 1: " <<measurementSettings.size() << endl;
    }

    if(isCalibrationDataConfigLoaded()){
      compareToLoadedGainSettings(gainParamsMap,irampSettings,activePixels);
    }else{
      updateGainParamsMap(gainParamsMap);
      setCurrentIrampSettings(irampSettings,activePixels);
    }
    setPixelInjectionSlopes(measurementSlopes,activePixels);
  }
  else
  {
    cout << "CalibrationDataGenerator Error: PixelInjectionSlopes can not loaded from this type of file: " << measurementType << endl;
  }

  return isPixelInjectionSlopesLoaded();
}


bool DsscHDF5CalibrationDataGenerator::importMeasuredGainMap(const std::string & gainMapFileName)
{
  if(!utils::checkFileExists(gainMapFileName)){
    cout << "File does not exist:" << gainMapFileName << endl;
    return false;
  }

  DsscHDF5TrimmingDataReader dataReader(gainMapFileName);

  if(dataReader.getMeasurementName() == "ADCGainMapMeasurementSummary")
  {
    const auto irampSettings = dataReader.readRmpFineTrmSettings();
    const auto measuredADCGainMap = dataReader.getDataVector("ADCGainMap_Gain");

    setMeasuredSpectrumGains(measuredADCGainMap,irampSettings);
  }
  else
  {
    cout << "CalibrationDataGenerator Error: Measured Gain Map can not loaded from this type of file: " << dataReader.getMeasurementName() << endl;
  }

  return isMeasuredGainMapLoaded();
}


bool DsscHDF5CalibrationDataGenerator::importADCGainMapFile(const std::string & adcGainMapFileName)
{
  if(!utils::checkFileExists(adcGainMapFileName)){
    cout << "File does not exist:" << adcGainMapFileName << endl;
    return false;
  }

  DsscHDF5TrimmingDataReader dataReader(adcGainMapFileName);
  const auto adcGainMapParams = dataReader.readADCGainFitParameterVec();
  setADCGainMap(adcGainMapParams);
  if(isADCGainMapLoaded()){
    cout << "Loaded " << adcGainMapParams.size() << " ADCGain Fit Parameters" << endl;
    setLastOutput(adcGainMapFileName);
  }
  return isADCGainMapLoaded();
}


bool DsscHDF5CalibrationDataGenerator::importSpektrumFitResultsFile(const std::string & sprectrumFitResultsFileName)
{
  if(!utils::checkFileExists(sprectrumFitResultsFileName)){
    cout << "File does not exist:" << sprectrumFitResultsFileName << endl;
    return false;
  }

  DsscHDF5TrimmingDataReader dataReader(sprectrumFitResultsFileName);
  const auto spectrumFitResults = dataReader.readPixelFitResultsVec();
  setSpectrumFitResults(spectrumFitResults);
  if(isSpectrumFitResultsLoaded()){
    cout << "Loaded " << spectrumFitResults.size() << " Spectrum Fit Results" << endl;
    setLastOutput(sprectrumFitResultsFileName);
  }

  return isSpectrumFitResultsLoaded();
}


bool DsscHDF5CalibrationDataGenerator::importBinningInformationFile(const std::string & binningInfoFileName)
{
  if(!utils::checkFileExists(binningInfoFileName)){
    cout << "File does not exist:" << binningInfoFileName << endl;
    return false;
  }

  const auto newMap = utils::DataHisto::importDNLEvaluationsMap(binningInfoFileName);
  setDNLValuesMap(newMap);

  if(isDNLMapLoaded()){
    cout << "Loaded " << getDNLInformation().size() << " Binning information" << endl;
    cout << "Contained bin range: " << utils::computeBinningRangeStr(getDNLInformation()) << endl;
    setLastOutput(binningInfoFileName);
  }

  return isDNLMapLoaded();
}


bool DsscHDF5CalibrationDataGenerator::loadHistoDataAndFit(const string &histogramFileName)
{
  DsscHDF5TrimmingDataReader reader(histogramFileName);
  auto dataHistoMap = reader.readPixelDataHistos();
  return loadHistoDataAndFit(dataHistoMap);
}


//filePath where SpectrumFitResults are stored is saved in m_lastOutput variable and canbe used to load
// output file in next step
bool DsscHDF5CalibrationDataGenerator::loadHistoDataAndFit(utils::DataHistoMap &dataHistoMap)
{
  if(isDNLMapLoaded()){
    fitSpectra(dataHistoMap,"SpectrumFitResults",getDNLInformation());
  }else{
    fitSpectra(dataHistoMap,"SpectrumFitResults");
  }
  return !getSpectrumFitResults().empty();
}


std::string DsscHDF5CalibrationDataGenerator::saveSpectrumFitResults(const std::string & exportFilename)
{
  SuS::CalibrationDataGenerator::saveSpectrumFitResults(exportFilename);

  const std::string exportFilePath =  getOutputDir() + "/" + exportFilename;
  DsscHDF5TrimmingDataWriter dataWriter(exportFilePath);
  dataWriter.setMeasurementName("SpectrumFitResults");
  dataWriter.addCurrentTargetInfo();
  dataWriter.addFitResultData("SpectrumFitResults",getSpectrumFitResults());

  if(isCalibrationDataConfigLoaded()){
    dataWriter.addGainParamsMap(getGainParamsMap());
  }

  if(isDNLMapLoaded()){
    dataWriter.addStringData("dnlApplied","YES");
  }else{
    dataWriter.addStringData("dnlApplied","NO");
  }

  cout << "SpectrumFitResults saved to " << exportFilePath << endl;
  setLastOutput(exportFilePath);

  return exportFilePath;
}


bool DsscHDF5CalibrationDataGenerator::computeTargetGainADCConfiguration(const std::string & gainString)
{
  bool ok = true;
  if(!isADCGainMapLoaded()){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "Can not compute TargetADCGain Configuration without ADC Gain Map" << endl;
    ok = false;
  }

  if(!isSpectrumFitResultsLoaded()){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "Can not compute TargetADCGain Configuration without SpectrumFitresults for current Gain Information" << endl;
    ok = false;
  }

  if(!ok) return false;

  double targetGain = SuS::CHIPGainConfigurator::getGainModeEV(gainString);

  computeADCGainConfiguration(targetGain);

  saveCalibratedIrampSettings(gainString);

  return ok;
}


std::string DsscHDF5CalibrationDataGenerator::saveCalibratedIrampSettings(const std::string & gainStr)
{
  const auto & currentIrampSettings = getCurrentPixelsIrampSettings();

  DsscHDF5TrimmingDataWriter dataWriter(getOutputDir()+"/CalibratedIrampSettings_"+ utils::stringReplace(gainStr,'/','_'));
  dataWriter.setMeasurementName("CalibratedIrampSettings");
  dataWriter.addStringData("GainSetting",gainStr);
  dataWriter.addCurrentTargetInfo();
  dataWriter.addStringData("pixels","0-"+to_string(getNumPixels()));
  dataWriter.addValueData("numPixels",getNumPixels());
  dataWriter.addValueData("numLimitPixels",getNumLimitPixels());
  dataWriter.addImageData("RmpFineTrmSettings",512,utils::convertVectorType<uint32_t,double>(currentIrampSettings));

  if(isCalibrationDataConfigLoaded()){
    dataWriter.addGainParamsMap(getGainParamsMap());
  }

  if(isDNLMapLoaded()){
    dataWriter.addStringData("dnlApplied","YES");
  }else{
    dataWriter.addStringData("dnlApplied","NO");
  }

  utils::DataHisto irampHisto;
  for(auto && value : currentIrampSettings){
    irampHisto.add(value);
  }
  dataWriter.addHistoData("IRampSettingsHisto",irampHisto,1.0);

  std::string hdf5FileName = dataWriter.getFileName();

  setLastOutput(hdf5FileName);

  return hdf5FileName;
}


std::string DsscHDF5CalibrationDataGenerator::generateTargetGainConfiguration(double targetGain, const utils::SpectrumGainMapFitResultVec & spectrumGainFitResultsVec, const std::vector<uint32_t> & irampSettings)
{
  const size_t totalNumPxs = 65536;

  std::vector<uint32_t> irampSettingsMap(64,0);
  std::vector<std::vector<double>> adcGainDiffMap(totalNumPxs,std::vector<double>(irampSettings.size(),0.0));
  std::vector<utils::FitResultsVec> sortedSpectrumFitResults(totalNumPxs,utils::FitResultsVec(irampSettings.size()));
  const size_t numElems = spectrumGainFitResultsVec.size();

  for(size_t idx=0; idx < irampSettings.size(); idx++){
    irampSettingsMap[irampSettings[idx]] = idx;
  }

#pragma omp parallel for
  for(size_t idx=0; idx < numElems; idx++){
    const auto & fitResults = spectrumGainFitResultsVec[idx];
    const auto pixel        = fitResults.pixel;
    const auto irampSetting = fitResults.gainSetting;

    auto & pixelResults     = sortedSpectrumFitResults[pixel];
    auto & pixelGainDiffMap = adcGainDiffMap[pixel];

    pixelGainDiffMap[irampSettingsMap[irampSetting]] = fabs(fitResults.getGain()-targetGain);
    pixelResults[irampSettingsMap[irampSetting]] = fitResults;
  }

  m_spectrumFitResults.resize(totalNumPxs);
  std::vector<uint32_t> calibratedSettingsVec(totalNumPxs,0);
  std::vector<double>   calibratedGainVec(totalNumPxs,0.0);
  std::vector<double>   calibratedNoiseVec(totalNumPxs,0.0);

#pragma omp parallel for
  for(size_t px=0; px < totalNumPxs; px++){
    const auto & pixelResults  = sortedSpectrumFitResults[px];
    const auto & pixelDiffMap  = adcGainDiffMap[px];
    int bestSettingIdx = std::distance(pixelDiffMap.begin(),std::min_element(pixelDiffMap.begin(),pixelDiffMap.end()));
    calibratedSettingsVec[px] = irampSettings[bestSettingIdx];

    const auto & bestResult  = pixelResults[bestSettingIdx];
    m_spectrumFitResults[px] = bestResult;
    calibratedGainVec[px]    = bestResult.getGain();
    calibratedNoiseVec[px]   = bestResult.getENC();
  }

  std::string resultsFileName = "Calib_SpectrumFitResults_TargetGain" + to_string((int)targetGain) + "eV.h5";
  saveSpectrumFitResults(resultsFileName);

  std::string hdf5FileName = getOutputDir() + "/CalibADCSettingConfigs_TargetGain" + to_string((int)targetGain) + "eV.h5";

  DsscHDF5TrimmingDataWriter dataWriter(hdf5FileName);
  dataWriter.setMeasurementName("CalibratedIrampSettings");
  dataWriter.addValueData("TargetGain",targetGain);
  dataWriter.addCurrentTargetInfo();
  dataWriter.addStringData("pixels","0-65535");
  dataWriter.addValueData("numPixels",totalNumPxs);
  dataWriter.addImageData("RmpFineTrmSettings",512,utils::convertVectorType<uint32_t,double>(calibratedSettingsVec));
  dataWriter.addImageData("MeasuredGainValues",512,calibratedGainVec);
  dataWriter.addImageData("MeasuredNoiseValues",512,calibratedNoiseVec);

  return hdf5FileName;
}


std::string DsscHDF5CalibrationDataGenerator::computeTargetGainADCStatistics(double targetGain, const utils::SpectrumGainMapFitResultVec & spectrumGainFitResultsVec)
{
  const size_t numPixels = 65536;
  utils::DataHistoVec irampSettingsHistograms(numPixels);

  for(auto && fitResult : spectrumGainFitResultsVec)
  {
    auto pixel = fitResult.pixel;
    auto iramp = fitResult.gainSetting;

    double gainChange = fitResult.getGain() / targetGain;
    int newSetting = calcNewADCSettingUsingMeanFitParams(iramp,gainChange);
    newSetting = std::max(0,std::min(newSetting,63));
    irampSettingsHistograms[pixel].add(newSetting);
  }

  std::vector<uint32_t> calibIrampSettings(numPixels,0);
#pragma omp parallel for
  for(size_t px=0; px<numPixels; px++){
    calibIrampSettings[px] = irampSettingsHistograms[px].getMaxBin();
  }

  std::string hdf5FileName = getOutputDir() + "/CalibADCSettingHistos_TargetGain" + to_string((int)targetGain) + "eV.h5";

  DsscHDF5TrimmingDataWriter dataWriter(hdf5FileName);
  dataWriter.setMeasurementName("ADCSettingsHistos");
  dataWriter.addHistoData("ADCSettingHistograms",irampSettingsHistograms);
  dataWriter.addValueData("TargetGain",targetGain);
  dataWriter.addStringData("pixels","0-65535");
  dataWriter.addImageData("RmpFineTrmSettings",512,utils::convertVectorType<uint32_t,double>(calibIrampSettings));

  if(isMeasuredGainMapLoaded()){
    std::vector<double> measuredGainValues(numPixels,0.0);
#pragma omp parallel for
    for(size_t px=0; px<numPixels; px++){
      measuredGainValues[px] = getMeasuredGainForSetting(px,calibIrampSettings[px]);
    }
    dataWriter.addImageData("MeasuredGainValues",512,measuredGainValues);
  }

  if(isCalibrationDataConfigLoaded()){
    dataWriter.addGainParamsMap(getGainParamsMap());
  }

  return hdf5FileName;
}


std::vector<double> DsscHDF5CalibrationDataGenerator::computePixelInjectionCalibrationFactors()
{
  const auto & calibData = CalibrationDataGenerator::computePixelInjectionCalibrationData();

  DsscHDF5TrimmingDataWriter dataWriter(getOutputDir() + "/PixelInjectionCalibrationFactors.h5");
  dataWriter.setMeasurementName("PixelInjectionCalibrationFactors");
  dataWriter.addStringData("pxInjCalibMode",m_pxInjCalibMode);
  dataWriter.addValueData("QInjEn10fF",getCoarseGainSetting("QInjEn10fF"));
  dataWriter.addValueData("CSA_Cin_200fF",getCoarseGainSetting("CSA_Cin_200fF"));
  dataWriter.addInjectionCalibrationData(calibData);
  dataWriter.addVectorData("RmpFineTrmSettings",getCurrentPixelsIrampSettings());
  if(isCalibrationDataConfigLoaded()){
    dataWriter.addGainParamsMap(getGainParamsMap());
  }

  if(!m_pxInjectionSlopeCompareFactors.empty()){
    dataWriter.addVectorData("InjectionSlopeCompareFactors",m_pxInjectionSlopeCompareFactors);
    dataWriter.addGainParamsMap(getCompareGainParamsMap(),"InjectionSlopeCompareGainParams");
  }

  std::vector<uint32_t> badPixels;
  std::vector<double> calibFactors;
  for(auto && factor : calibData){
    if(factor[1]==0.0){
      badPixels.push_back(factor[0]);
    }
    calibFactors.push_back(factor[1]);
  }

  dataWriter.addVectorData("invalidPixels",badPixels);
  dataWriter.addValueData("numInvalidPixels",badPixels.size());

  uint32_t gainSetting = getCoarseGainSetting("QInjEn10fF");
  utils::dumpInjectionCalibrationData(getOutputDir() + "/PixelInjectionCalibrationFactors.txt",gainSetting,m_pxInjCalibMode,calibData);

  return calibFactors;
}


std::pair<std::string,std::vector<double>> DsscHDF5CalibrationDataGenerator::loadPxInjCalibrationFactors(const std::string & calibFileName)
{
  DsscHDF5CalibrationDataGenerator calibDataGenerator;
  calibDataGenerator.importPxInjectionCalibFactors(calibFileName);
  return {calibDataGenerator.getPxInjCalibMode(),calibDataGenerator.getPxInjectionCalibFactors()};
}



