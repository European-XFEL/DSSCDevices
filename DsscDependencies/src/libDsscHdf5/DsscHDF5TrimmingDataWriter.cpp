#include "DsscHDF5TrimmingDataWriter.h"

using namespace std;

/*
Version 1: version information
MeasurementName and ClockDeskew for BinningMeasurement
Version 2: Module Info added
*/

const unsigned int DsscHDF5TrimmingDataWriter::fileVersion = 2;

DsscHDF5TrimmingDataWriter::DsscHDF5TrimmingDataWriter(const std::string & fileName)
  : m_fileName(fileName)
{
  if(m_fileName.rfind(".h5") == std::string::npos){
    m_fileName += ".h5";
  }

  m_fileHDF5 = H5Fcreate(m_fileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  createNewGroup(m_fileHDF5, "/INSTRUMENT");
  createNewGroup(m_fileHDF5, "/INSTRUMENT/DSSC");
  m_trimmingDirName = "/INSTRUMENT/DSSC/TrimmingData/";
  createNewGroup(m_fileHDF5, m_trimmingDirName);
  writeDataString(m_fileHDF5,m_trimmingDirName,"timestamp",utils::getLocalTimeStr());

  hsize_t singleValue = 1;
  writeData<uint32_t>(m_fileHDF5,m_trimmingDirName,"fileVersion",H5T_STD_U32LE,&singleValue,&fileVersion);

  addCurrentModuleInfo();
}


DsscHDF5TrimmingDataWriter::~DsscHDF5TrimmingDataWriter()
{
  cout << "Save DsscHDF5TrimmingDataWriter File: "<< m_fileName << endl;
  H5Fclose(m_fileHDF5);
}

void DsscHDF5TrimmingDataWriter::setMeasurementName(const std::string & measurementName)
{
  writeDataString(m_fileHDF5,m_trimmingDirName,"MeasurementName",measurementName);
}


void DsscHDF5TrimmingDataWriter::addNewGroup(const std::string & groupName)
{
   createNewGroup(m_fileHDF5, m_trimmingDirName+groupName);
}

void DsscHDF5TrimmingDataWriter::setCurrentGroup(const std::string & groupName)
{
   m_trimmingDirName = groupName;
   if(m_trimmingDirName.back() != '/'){
     m_trimmingDirName += "/";
   }
}


void DsscHDF5TrimmingDataWriter::addValueData(const std::string & paramName, double value)
{
  hsize_t singleValue = 1;
  writeData<double>(m_fileHDF5,m_trimmingDirName,paramName,H5T_IEEE_F64LE,&singleValue,&value);
}


void DsscHDF5TrimmingDataWriter::addStringData(const std::string & paramName, const std::string & value)
{
  writeDataString(m_fileHDF5,m_trimmingDirName,paramName,value);
}


void DsscHDF5TrimmingDataWriter::addHistoData(const std::string & groupName, const utils::DataHistoVec & dataHistoVec)
{
  hsize_t singleValue = 1;
  const string trimingDirRem = m_trimmingDirName;

  m_trimmingDirName += groupName;

  createNewGroup(m_fileHDF5,m_trimmingDirName);

  m_trimmingDirName += "/";

  unsigned int numPixels = dataHistoVec.size();

  string pixelString = "0-"+ to_string(numPixels-1);
  writeDataString(m_fileHDF5,m_trimmingDirName,"pixels",pixelString);
  writeData<unsigned int> (m_fileHDF5,m_trimmingDirName,"numPixels",H5T_STD_U32LE,&singleValue,&numPixels);

  int idx = 0;
  for(auto && pixelHisto : dataHistoVec){
    const string name = "pixelHisto_" + to_string(idx++);
    addHistoData(name,pixelHisto);
  }

  m_trimmingDirName = trimingDirRem;
}


void DsscHDF5TrimmingDataWriter::addHistoData(const std::string & groupName, const utils::DataHistoVec & dataHistoVec, const std::vector<uint32_t> & containedPixels)
{
  hsize_t singleValue = 1;
  const string trimingDirRem = m_trimmingDirName;

  m_trimmingDirName += groupName;

  createNewGroup(m_fileHDF5,m_trimmingDirName);

  m_trimmingDirName += "/";

  unsigned int numPixels = containedPixels.size();

  string pixelString = utils::positionVectorToList(containedPixels);
  writeDataString(m_fileHDF5,m_trimmingDirName,"pixels",pixelString);
  writeData<unsigned int> (m_fileHDF5,m_trimmingDirName,"numPixels",H5T_STD_U32LE,&singleValue,&numPixels);

  for(auto && imagePixel : containedPixels){
    const string name = "pixelHisto_" + to_string(imagePixel);
    addHistoData(name,dataHistoVec[imagePixel]);
  }

  m_trimmingDirName = trimingDirRem;
}


void DsscHDF5TrimmingDataWriter::addHistoData(const std::string & histoName, const utils::DataHisto & dataHisto, double scalefactor)
{
  hsize_t singleValue = 1;

  const string trimingDirRem = m_trimmingDirName;

  m_trimmingDirName += histoName;

  createNewGroup(m_fileHDF5,m_trimmingDirName);

  m_trimmingDirName += "/";
  uint64_t numCounts = dataHisto.getCount();
  const auto bins = dataHisto.getBins();
  const auto binValues = dataHisto.getBinValues();

  size_t numValues = bins.size();
  std::vector<std::vector<double>> values(numValues,std::vector<double>(2,0));
  for(size_t i=0; i<numValues; i++){
    values[i] = {(double)bins[i],(double)binValues[i]};
  }

  writeData<uint64_t> (m_fileHDF5,m_trimmingDirName,"numEntries",H5T_IEEE_F64LE,&singleValue,&numCounts);
  writeData<double> (m_fileHDF5,m_trimmingDirName,"scaleFactor",H5T_IEEE_F64LE,&singleValue,&scalefactor);
  addDualVectorData("histoBins",values);

  m_trimmingDirName = trimingDirRem;
}


void DsscHDF5TrimmingDataWriter::addVectorData(const std::string & paramName, const std::vector<uint32_t> & values)
{
  hsize_t numValues = values.size();
  writeData<uint32_t> (m_fileHDF5,m_trimmingDirName,paramName,H5T_STD_U32LE,&numValues,values.data());
}


void DsscHDF5TrimmingDataWriter::addVectorData(const std::string & paramName, const std::vector<double> & values)
{
  hsize_t numValues = values.size();
  writeData<double> (m_fileHDF5,m_trimmingDirName,paramName,H5T_IEEE_F64LE,&numValues,values.data());
}


void DsscHDF5TrimmingDataWriter::addImageData(const std::string & paramName, hsize_t width, const std::vector<double> & values)
{
  hsize_t numValues = values.size();
  hsize_t height = numValues/width;

  constexpr int numDims = 2;

  hsize_t dimsf[numDims]{height, width};

  hid_t datatype = H5Tcopy(H5T_IEEE_F64LE);

  hid_t dataspace = H5Screate_simple(numDims,dimsf,NULL);

  hid_t dataset = H5Dcreate2(m_fileHDF5,(m_trimmingDirName + "/" + paramName).c_str(),datatype,dataspace,
                              H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);

  herr_t status = H5Dwrite(dataset,H5T_IEEE_F64LE,H5S_ALL,H5S_ALL,H5P_DEFAULT,values.data());
  if(status<0) std::cout << "HDF5Writer: Could not write double vector data " << paramName << " to HDF5 file" <<  std::endl;

  H5Sclose(dataspace);
  H5Tclose(datatype);
  H5Dclose(dataset);
}


void DsscHDF5TrimmingDataWriter::addDualVectorData(const std::string & paramName, const std::vector<std::vector<double>> & values)
{
  addDoubleVectorData(m_fileHDF5, m_trimmingDirName, paramName,values);
}


void DsscHDF5TrimmingDataWriter::addStringVectorData(const std::string & paramName, const std::vector<std::string> & values)
{
  writeStringVector(m_fileHDF5,m_trimmingDirName,paramName,values);
}


void DsscHDF5TrimmingDataWriter::addCurrentTargetInfo()
{
  if(isGroupExisting("Target")){
    cout << "Target info already existing" << endl;
    return;
  }

  addNewGroup("Target");
  addValueData("Target/PhotonEnergy",utils::SpectrumFitResult::s_currentTarget.photonEnergy);
  addStringData("Target/Name",utils::SpectrumFitResult::s_currentTarget.name);
  addValueData("Target/KAB_ERatio",utils::SpectrumFitResult::s_currentTarget.kab_Eratio);
  addValueData("Target/KAB_Iatio",utils::SpectrumFitResult::s_currentTarget.kab_Iratio);
}


void DsscHDF5TrimmingDataWriter::addGainParamsMap(const SuS::DsscGainParamMap & gainParams, const std::string & node)
{
  if(isGroupExisting(node)){
    cout << "GainParams already existing" << endl;
    return;
  }

  addNewGroup(node);
  for(auto && elem : gainParams){
    addValueData(node + "/" + elem.first,elem.second);
  }
}


void DsscHDF5TrimmingDataWriter::addFitResultData(const std::string & paramName, const utils::FitResultsVec & fitResults)
{
  const auto dataNames = utils::SpectrumFitResult::getDataNames();
  unsigned int numPixels = fitResults.size();
  const size_t numValues = dataNames.size();
  std::vector<std::vector<double>> exportVectors(numPixels,std::vector<double>(numValues));

  std::vector<uint32_t> badPixels;
  for(size_t px=0; px<numPixels; px++){
    const auto & result = fitResults[px];
    if(!result.isValid()){
      badPixels.push_back(px);
    }
    const auto pixelResData = result.getDataVec();
    auto & pxExport = exportVectors[px];
    std::copy(pixelResData.begin(),pixelResData.end(),pxExport.begin());
  }

  if(!badPixels.empty()){
    cout << "WARNING: Found BadSpectrumFitResults in " << badPixels.size() << " pixels" << endl;
  }

  string pixelString = "0-"+ to_string(numPixels-1);
  addDualVectorData(paramName,exportVectors);
  if(!isGroupExisting("pixels")){
    addStringData("pixels",pixelString);
  }
  if(!isGroupExisting("numPixels")){
    addValueData("numPixels",numPixels);
  }

  if(!isGroupExisting(paramName+"_FitErrorPixels")){
    addValueData(paramName+"_FitErrorPixelCnt",badPixels.size());
    addVectorData(paramName+"_FitErrorPixels",badPixels);
  }

  if(numPixels == 65536 || numPixels == 4096){
    addFitResultsImageMaps(fitResults);
  }
}


void DsscHDF5TrimmingDataWriter::addSpectrumGainMapFitResultData(const std::string & paramName, const utils::SpectrumGainMapFitResultVec & fitResults)
{
  const auto dataNames = utils::SpectrumGainMapFitResult::getDataNames();
  unsigned int numPixels = fitResults.size();
  const size_t numValues = dataNames.size();
  std::vector<std::vector<double>> exportVectors(numPixels,std::vector<double>(numValues));
  for(size_t px=0; px<numPixels; px++)
  {
    const auto pixelResData = fitResults[px].getDataVec();
    auto & pxExport = exportVectors[px];
    std::copy(pixelResData.begin(),pixelResData.end(),pxExport.begin());
  }

  string pixelString = "0-"+ to_string(numPixels-1);
  addDualVectorData(paramName,exportVectors);

  if(!isGroupExisting("pixels")) addStringData("pixels",pixelString);
  if(!isGroupExisting("resultDataNames")) addStringVectorData("resultDataNames",dataNames);
  if(!isGroupExisting("numPixels")) addValueData("numPixels",numPixels);

  if(numPixels == 65536 || numPixels == 4096){
    addSpectrumGainMapFitResultsImageMaps(paramName,fitResults);
  }
}





void DsscHDF5TrimmingDataWriter::addFitResultsImageMaps(const utils::FitResultsVec & fitResults)
{
   if(isGroupExisting("GainMap")) {
     cout << "WARNING Fit Results Image Maps already existing" << endl;
     return;
   }

  const size_t numPixels = fitResults.size();
  std::vector<std::vector<double>> fitResultMaps(13,std::vector<double>(numPixels));
  //Transpose fitResultsValues to generate Parameter Maps
  // Errors are not added to overview images
#pragma omp parallel for
  for(size_t pxIdx=0; pxIdx < numPixels; pxIdx++)
  {
    auto & result = fitResults[pxIdx];
    int idx = 0;
    fitResultMaps[idx++][pxIdx] = result.getGain();
    fitResultMaps[idx++][pxIdx] = result.getKaDistance();
    fitResultMaps[idx++][pxIdx] = result.getENC();
    fitResultMaps[idx++][pxIdx] = result.roiNoiseStart;
    fitResultMaps[idx++][pxIdx] = result.roiNoiseEnd;
    fitResultMaps[idx++][pxIdx] = result.roiSignalStart;
    fitResultMaps[idx++][pxIdx] = result.roiSignalEnd;
    fitResultMaps[idx++][pxIdx] = result.noiseAmp;
    fitResultMaps[idx++][pxIdx] = result.noiseMean;
    fitResultMaps[idx++][pxIdx] = result.noiseSigma;
    fitResultMaps[idx++][pxIdx] = result.kaPeakAmp;
    fitResultMaps[idx++][pxIdx] = result.kaPeakMean;
    fitResultMaps[idx++][pxIdx] = result.troughrat;
  }

  int width = (fitResults.size() == 4096)? 64 : 512;

  int idx = 0;
  addImageData("GainMap",width,fitResultMaps[idx++]);
  addImageData("KaDistanceMap",width,fitResultMaps[idx++]);
  addImageData("ENCMap",width,fitResultMaps[idx++]);
  addImageData("RoiNoiseStartMap",width,fitResultMaps[idx++]);
  addImageData("RoiNoiseEndMap",width,fitResultMaps[idx++]);
  addImageData("RoiSignalStartMap",width,fitResultMaps[idx++]);
  addImageData("RoiSignalEndMap",width,fitResultMaps[idx++]);
  addImageData("NoiseAmpMap",width,fitResultMaps[idx++]);
  addImageData("NoiseMeanMap",width,fitResultMaps[idx++]);
  addImageData("NoiseSigmaMap",width,fitResultMaps[idx++]);
  addImageData("KaPeakAmpMap",width,fitResultMaps[idx++]);
  addImageData("KaPeakMeanMap",width,fitResultMaps[idx++]);
  addImageData("TroughRatMap",width,fitResultMaps[idx++]);
}


void DsscHDF5TrimmingDataWriter::addSpectrumGainMapFitResultsImageMaps(const std::string & subDirName, const utils::SpectrumGainMapFitResultVec & fitResults)
{
  if(isGroupExisting(subDirName)){
    cout << "Error: subDirName info already existing" << endl;
    return;
  }

  addNewGroup(subDirName);

  const size_t numPixels = fitResults.size();
  std::vector<std::vector<double>> fitResultMaps(14,std::vector<double>(numPixels));
  //Transpose fitResultsValues to generate Parameter Maps

#pragma omp parallel for
  for(size_t pxIdx=0; pxIdx < numPixels; pxIdx++)
  {
    auto & result = fitResults[pxIdx];
    int idx = 0;
    fitResultMaps[idx++][pxIdx] = result.gainSetting;
    fitResultMaps[idx++][pxIdx] = result.getGain();
    fitResultMaps[idx++][pxIdx] = result.getKaDistance();
    fitResultMaps[idx++][pxIdx] = result.getENC();
    fitResultMaps[idx++][pxIdx] = result.roiNoiseStart;
    fitResultMaps[idx++][pxIdx] = result.roiNoiseEnd;
    fitResultMaps[idx++][pxIdx] = result.roiSignalStart;
    fitResultMaps[idx++][pxIdx] = result.roiSignalEnd;
    fitResultMaps[idx++][pxIdx] = result.noiseAmp;
    fitResultMaps[idx++][pxIdx] = result.noiseMean;
    fitResultMaps[idx++][pxIdx] = result.noiseSigma;
    fitResultMaps[idx++][pxIdx] = result.kaPeakAmp;
    fitResultMaps[idx++][pxIdx] = result.kaPeakMean;
    fitResultMaps[idx++][pxIdx] = result.troughrat;
  }

  int width = (fitResults.size() == 4096)? 64 : 512;

  int idx = 0;
  addImageData(subDirName+"/gainSetting",width,fitResultMaps[idx++]);
  addImageData(subDirName+"/GainMap",width,fitResultMaps[idx++]);
  addImageData(subDirName+"/KaDistanceMap",width,fitResultMaps[idx++]);
  addImageData(subDirName+"/ENCMap",width,fitResultMaps[idx++]);
  addImageData(subDirName+"/RoiNoiseStartMap",width,fitResultMaps[idx++]);
  addImageData(subDirName+"/RoiNoiseEndMap",width,fitResultMaps[idx++]);
  addImageData(subDirName+"/RoiSignalStartMap",width,fitResultMaps[idx++]);
  addImageData(subDirName+"/RoiSignalEndMap",width,fitResultMaps[idx++]);
  addImageData(subDirName+"/NoiseAmpMap",width,fitResultMaps[idx++]);
  addImageData(subDirName+"/NoiseMeanMap",width,fitResultMaps[idx++]);
  addImageData(subDirName+"/NoiseSigmaMap",width,fitResultMaps[idx++]);
  addImageData(subDirName+"/KaPeakAmpMap",width,fitResultMaps[idx++]);
  addImageData(subDirName+"/KaPeakMeanMap",width,fitResultMaps[idx++]);
  addImageData(subDirName+"/TroughRatMap",width,fitResultMaps[idx++]);
}


utils::ADCGainFitResult DsscHDF5TrimmingDataWriter::addADCGainMapFitSlopesData(const utils::ADCGainFitResultsVec & resultsVec)
{
  size_t numPixels = resultsVec.size();

  std::vector<double> exportVector(numPixels*utils::ADCGainFitResults::NUMVALS);

  std::atomic_int pixelCnt;
  pixelCnt = 0;
  // generate fit results from loaded data and store it in a new file.
  // At the same time fill the adcfitResultsVector for return
  #pragma omp parallel for
  for(size_t idx = 0; idx<numPixels; idx++)
  {
    auto & adcGainFitResult = resultsVec[idx];
    auto * pixelResArray = exportVector.data() + idx * utils::ADCGainFitResults::NUMVALS;
    std::copy(adcGainFitResult.begin(),adcGainFitResult.end(),pixelResArray);
  }

  hsize_t width = utils::ADCGainFitResults::NUMVALS;
  addStringVectorData("ADCGainFitParamNames",utils::ADCGainFitResults::getFitParamNames());
  addImageData("ADCGainFitParameters",width,exportVector);

  int numGoodValues;
  const auto meanRmsVec = utils::calcMeanRMSADCGainFitParams(resultsVec,numGoodValues);

  std::vector<double> meanParam(utils::ADCGainFitResults::NUMVALS-1,0);
  std::vector<double> rmsParam(utils::ADCGainFitResults::NUMVALS-1,0);

  for(size_t i=0; i<meanParam.size(); i++){
    meanParam[i] = meanRmsVec[i].mean;
    rmsParam[i] = meanRmsVec[i].rms;
  }

  addVectorData("MeanFitParameters",meanParam);
  addVectorData("MeanFitParametersRMS",rmsParam);
  addValueData("NumBadFits",numPixels-numGoodValues);

  utils::ADCGainFitResult meanResult;
  meanResult[0] = 65536;
  std::copy(meanParam.begin(),meanParam.end(),meanResult.begin()+1);
  return meanResult;
}


void DsscHDF5TrimmingDataWriter::addInjectionCalibrationData(const utils::InjectionCalibrationData & calibrationData)
{
  size_t numPixels = calibrationData.size();
  std::vector<uint32_t> pixels(numPixels);
  std::array<std::vector<double>,6> calibFactors;
  for(auto && factVec : calibFactors){
    factVec.resize(numPixels);
  }

#pragma omp parallel for
  for(size_t px=0; px<numPixels; px++){
    const auto & pixelCalibData = calibrationData[px];
    pixels[px] = pixelCalibData[0];
    calibFactors[0][px] = pixelCalibData[1];
    calibFactors[1][px] = pixelCalibData[2];
    calibFactors[2][px] = pixelCalibData[3];
    calibFactors[3][px] = pixelCalibData[4];
    calibFactors[4][px] = pixelCalibData[5];
    calibFactors[5][px] = pixelCalibData[6];
  }

  std::sort(pixels.begin(),pixels.end());
  addStringData("pixels",utils::positionVectorToList(pixels));
  addVectorData("pxInjectionCalibFactors",calibFactors[0]);
  addVectorData("calibFactorErr",calibFactors[1]);
  addVectorData("slope",calibFactors[2]);
  addVectorData("slopeErr",calibFactors[3]);
  addVectorData("gain",calibFactors[4]);
  addVectorData("gainErr",calibFactors[5]);
}


// input: numPixels x numSettings
utils::ADCGainFitResultsVec DsscHDF5TrimmingDataWriter::savePixelADCGainValues(const std::string & outputDir, const std::string & fileName, const std::vector<double> &pixelADCGainValues, const std::vector<uint32_t> & irampSettings)
{
  utils::ADCGainFitResultsVec fitResultsVector;
  // transpose settings and fill one dimensional vector
  size_t numSettings = irampSettings.size();
  size_t totalNumPxs = pixelADCGainValues.size()/numSettings;

  if(numSettings == 0){
    cout << "ERROR: pixelADCGainValues vector does not contain any settings: size() = " << numSettings << endl;
    return fitResultsVector;
  }

  utils::makePath(outputDir);


  fitResultsVector = utils::generateADCGainFitResults(pixelADCGainValues,irampSettings);

  // fill measurement data into hdf5 file structure
  DsscHDF5TrimmingDataWriter dataWriter(outputDir+"/"+fileName);
  dataWriter.setMeasurementName("ADCGainMapSummary");
  dataWriter.addImageData("ADCGainMap", numSettings, pixelADCGainValues);
  dataWriter.addCurrentTargetInfo();
  dataWriter.addVectorData("RmpFineTrmSettings",irampSettings);
  dataWriter.addADCGainMapFitSlopesData(fitResultsVector);

  //also text file generation for additional analysis methods
  std::string asciiFileName = outputDir+"/"+utils::changeFileExtension(fileName,".txt");
  const auto pixelsVec = utils::positionListToVector<uint32_t>("0-" +to_string(totalNumPxs-1));
  utils::dumpFitResultsToASCII(asciiFileName,pixelsVec,fitResultsVector);

  return fitResultsVector;
}


std::string DsscHDF5TrimmingDataWriter::saveSpectrumFitResults(const std::string & outputDir, const utils::SpectrumGainMapFitResultVec & spectrumFitResults, const std::vector<uint32_t> & irampSettings, std::vector<double> & measuredPixelGainValues)
{
  const size_t numSettings = irampSettings.size();
  const size_t numPixels   = 65536;
  const size_t numEntries  = spectrumFitResults.size();

  measuredPixelGainValues.resize(numSettings * numPixels,0.0);
  std::vector<double> measuredPixelKaDistance(numSettings * numPixels,0.0);
  std::vector<double> measuredPixelENC(numSettings * numPixels,0.0);

  std::vector<uint32_t> gainSettingIdxVec(64,0);
  for(size_t idx = 0; idx < numSettings; idx++){
    gainSettingIdxVec[irampSettings[idx]] = idx;
  }

#pragma omp parallel for
  for(size_t idx=0; idx<numEntries; idx++){
    const auto & pixelFitResult = spectrumFitResults[idx];
    auto & pixel = pixelFitResult.pixel;
    auto & gainSetting = pixelFitResult.gainSetting;

    uint32_t gainSettingIdx = gainSettingIdxVec[gainSetting];
    size_t offs = pixel * numSettings + gainSettingIdx;

    measuredPixelGainValues[offs] = pixelFitResult.getGain();
    measuredPixelKaDistance[offs] = pixelFitResult.getKaDistance();
    measuredPixelENC[offs]        = pixelFitResult.getENC();
  }

  std::string hdf5FileName = outputDir+"/ADCGainMap_MeasuredValues_Iramp"+utils::positionVectorToList(irampSettings)+".h5";
  DsscHDF5TrimmingDataWriter dataWriter(hdf5FileName);
  dataWriter.setMeasurementName("ADCGainMapMeasurementSummary");
  dataWriter.addStringData("pixels","0-65535");
  dataWriter.addVectorData("RmpFineTrmSettings",irampSettings);
  dataWriter.addImageData("ADCGainMap_Gain",numSettings,measuredPixelGainValues);
  dataWriter.addImageData("ADCGainMap_KaDistance",numSettings,measuredPixelKaDistance);
  dataWriter.addImageData("ADCGainMap_ENC",numSettings,measuredPixelENC);
  dataWriter.addCurrentTargetInfo();

  return hdf5FileName;
}


void DsscHDF5TrimmingDataWriter::addCurrentModuleInfo()
{
  if(isGroupExisting("ModuleInfo")){
    cout << "ModuleInfo info already existing" << endl;
    return;
  }
  addModuleInfo(m_fileHDF5,m_trimmingDirName);
}


