#include "DsscHDF5TrimmingDataReader.h"
#include "DsscHDF5TrimmingDataWriter.h"

using namespace std;

DsscHDF5TrimmingDataReader::DsscHDF5TrimmingDataReader(const std::string & fileName)
  : DsscHDF5Reader(fileName),
    m_fileVersion(0),
    m_measurementName("unknown"),
    m_mainNode("INSTRUMENT/DSSC/TrimmingData/")
{
  if(isGroupExisting(m_mainNode+"fileVersion")){
    std::string node = m_mainNode+"fileVersion";
    readValue(node,m_fileVersion);
    if(m_fileVersion>0){
      node = m_mainNode+"MeasurementName";
      if(!isGroupExisting(node)){
        node = m_mainNode+"measurementName";
        readDataString(node,m_measurementName,100);
      }else{
        readDataString(node,m_measurementName,100);
      }
      cout << "TrimmingDataReader: Measurement Name = " << m_measurementName << endl;
      if(m_fileVersion>1){
        checkModuleInfo(m_mainNode);
      }
    }
  }
  cout << "TrimmingDataReader: File Version = " << m_fileVersion << endl;
}


DsscHDF5TrimmingDataReader::~DsscHDF5TrimmingDataReader()
{
}


std::vector<uint32_t> DsscHDF5TrimmingDataReader::readPixels()
{
  std::vector<uint32_t> pixels;

  if(!loaded) return pixels;

  const std::string chipPartsNode = m_mainNode+"RunCHIPParts";
  const std::string dataHistoPixelsNode = m_mainNode+"SpectrumHistos";

  if(isGroupExisting(dataHistoPixelsNode))
  {
    std::string pixelString = "0";
    readDataString(dataHistoPixelsNode + "/pixels",pixelString,2e6);
    pixels = utils::positionListToVector<uint32_t>(pixelString);
  }
  else if(m_fileVersion>0)
  {
    if(m_measurementName == "BinningMeasurement"){
      const std::vector<uint32_t> asics = utils::positionListToVector<uint32_t>("0-15");

      std::vector<std::string> chipParts;
      readStringVector(chipPartsNode,chipParts);
      for(auto && part : chipParts){
        std::string partNode = m_mainNode+"RUN_"+part+"_DNLHistograms";

        const auto partPixels = utils::getSendingColumnPixels(asics,part,true);
        if(isGroupExisting(partNode)){
          pixels.insert(pixels.end(),partPixels.begin(),partPixels.end());
        }
      }
      std::sort(pixels.begin(),pixels.end());
    }else if(m_measurementName == "ADCGainMapSummary"){
      return utils::positionListToVector<uint32_t>("0-65535");
    }else{
      std::string pixelsNode = m_mainNode+"pixels";
      if(isGroupExisting(pixelsNode)){
        std::string pixelStr;
        readDataString(pixelsNode,pixelStr,50000);
        pixels = utils::positionListToVector(pixelStr);
      }
    }
  }
  else if(isGroupExisting(chipPartsNode))
  {
    std::vector<std::string> chipParts;
    readStringVector(chipPartsNode,chipParts);
    std::vector<uint32_t> asics = utils::positionListToVector<uint32_t>("0-15");
    for(auto && part : chipParts){
      std::string partNode = m_mainNode+"RUN_"+part+"_DNLHistograms";

      const auto partPixels = utils::getSendingColumnPixels(asics,part,true);
      if(isGroupExisting(partNode)){
        pixels.insert(pixels.end(),partPixels.begin(),partPixels.end());
      }
    }
    std::sort(pixels.begin(),pixels.end());
  }else if(m_fileVersion == 0){
    const std::string oldHistoPixelsNode = m_mainNode+"SpektrumData";
    if(isGroupExisting(oldHistoPixelsNode))
    {
      std::string pixelString = "0";
      readDataString(oldHistoPixelsNode + "/pixels",pixelString,2e6);
      pixels = utils::positionListToVector<uint32_t>(pixelString);
    }
  }
  return pixels;
}


utils::DataHisto DsscHDF5TrimmingDataReader::readPixelDataHisto(uint32_t pixel)
{
  const std::string chipPartsNode = m_mainNode+"RunCHIPParts";
  std::string dataHistoPixelsNode = m_mainNode+"SpectrumHistos/pixelHisto_" + to_string(pixel);

  if(isGroupExisting(dataHistoPixelsNode))
  {
    utils::DataHisto histo;
    readDataHisto(dataHistoPixelsNode,histo);
    return histo;
  }
  else if(m_fileVersion>0)
  {
    if(m_measurementName == "BinningMeasurement"){
      const std::vector<uint32_t> asics = utils::positionListToVector<uint32_t>("0-15");

      std::vector<std::string> chipParts;
      readStringVector(chipPartsNode,chipParts);
      for(auto && part : chipParts){
        std::string partNode = m_mainNode+"RUN_"+part+"_DNLHistograms";

        const auto partPixels = utils::getSendingColumnPixels(asics,part,true);
        // check if pixel is contained in current chip part section
        if(std::find(partPixels.begin(),partPixels.end(),pixel) != partPixels.end())
        {
          std::string node = partNode + "/pixelHisto_" + to_string(pixel);
          if(isGroupExisting(node)){
            utils::DataHisto histo;
            readDataHisto(node,histo);
            return histo;
          }
        }
      }
    }
  }
  else if(isGroupExisting(chipPartsNode))
  {
    std::vector<std::string> chipParts;
    readStringVector(chipPartsNode,chipParts);
    std::vector<uint32_t> asics = utils::positionListToVector<uint32_t>("0-15");
    for(auto && part : chipParts){
      std::string partNode = m_mainNode+"RUN_"+part+"_DNLHistograms";

      const auto partPixels = utils::getSendingColumnPixels(asics,part,true);
      // check if pixel is contained in current chip part section
      if(std::find(partPixels.begin(),partPixels.end(),pixel) != partPixels.end())
      {
        std::string node = partNode + "/pixelHisto_" + to_string(pixel);
        if(isGroupExisting(node)){
          utils::DataHisto histo;
          readDataHisto(node,histo);
          return histo;
        }
      }
    }
  }else if(m_fileVersion == 0){
    const std::string oldHistoPixelsNode = m_mainNode+"SpektrumData";
    if(isGroupExisting(oldHistoPixelsNode))
    {
      std::string node = oldHistoPixelsNode + "/pixelHisto_" + to_string(pixel);
      if(isGroupExisting(node)){
        utils::DataHisto histo;
        readDataHisto(node,histo);
        return histo;
      }
    }
  }
  cout << "Trimming Data Reader Error: no histogram node found" << endl;
  return utils::DataHisto();
}


utils::DataHistoMap DsscHDF5TrimmingDataReader::readPixelDataHistos(const std::vector<uint32_t> & pixels)
{
  utils::DataHistoMap histoMap;
  if(!loaded) return histoMap;

  for(auto && pixel : pixels){
    histoMap[pixel] = readPixelDataHisto(pixel);
  }
  return histoMap;
}

//faster version to read all histograms since singe pixel histo reading has lots of overhead
//to check if all nodes are existing
utils::DataHistoMap DsscHDF5TrimmingDataReader::readPixelDataHistos()
{
  cout << "Trimming Data Reader: Read DataHistograms... please wait" << endl;

  utils::DataHistoMap histoMap;

  const std::string chipPartsNode = m_mainNode+"RunCHIPParts";
  const std::string dataHistoPixelsNode = m_mainNode + "SpectrumHistos";

  if(isGroupExisting(dataHistoPixelsNode))
  {
    const auto pixels = readPixels();
    for(auto && pixel : pixels){
      std::string node = dataHistoPixelsNode +"/pixelHisto_" + to_string(pixel);
      readDataHisto(node,histoMap[pixel]);
    }
  }
  else if(m_fileVersion>0)
  {
    if(m_measurementName == "BinningMeasurement"){
      const std::vector<uint32_t> asics = utils::positionListToVector<uint32_t>("0-15");

      std::vector<std::string> chipParts;
      readStringVector(chipPartsNode,chipParts);
      for(auto && part : chipParts){
        std::string partNode = m_mainNode+"RUN_"+part+"_DNLHistograms";
        const auto partPixels = utils::getSendingColumnPixels(asics,part,true);
        for(auto && pixel : partPixels){
          std::string node = partNode + "/pixelHisto_" + to_string(pixel);
          if(isGroupExisting(node)){
            readDataHisto(node,histoMap[pixel]);
          }
        }
      }
    }
  }
  else if(isGroupExisting(chipPartsNode))
  {
    std::vector<std::string> chipParts;
    readStringVector(chipPartsNode,chipParts);
    std::vector<uint32_t> asics = utils::positionListToVector<uint32_t>("0-15");
    for(auto && part : chipParts){
      std::string partNode = m_mainNode+"RUN_"+part+"_DNLHistograms";
      const auto partPixels = utils::getSendingColumnPixels(asics,part,true);
      for(auto && pixel : partPixels){
        std::string node = partNode + "/pixelHisto_" + to_string(pixel);
        if(isGroupExisting(node)){
          readDataHisto(node,histoMap[pixel]);
        }
      }
    }
  }
  else if(m_fileVersion == 0)
  {
    const std::string oldHistoPixelsNode = m_mainNode+"SpektrumData";
    if(isGroupExisting(oldHistoPixelsNode))
    {
      // old histograms have a bug in pixel to image pixel conversion, can easily be fixed
      const auto pixels = readPixels();
      for(auto && pixel : pixels){
        std::string node = oldHistoPixelsNode+"/pixelHisto_" + to_string(pixel);
        uint32_t asic       = pixel/4096;
        uint32_t asicPixel  = pixel%4096;
        uint32_t imagePixel = utils::calcImagePixel(asic,asicPixel);
        readDataHisto(node,histoMap[imagePixel]);
      }
    }
  }

  return histoMap;
}

std::string DsscHDF5TrimmingDataReader::getStringData(const std::string & node)
{
  std::string dataNode = m_mainNode+node;
  std::string dataStr;
  if(isGroupExisting(dataNode)){
    readDataString(dataNode,dataStr);
  }else{
    cout << "TrimmingdataReader Error: node not found: " << dataNode << endl;
  }
  return dataStr;
}

std::vector<double> DsscHDF5TrimmingDataReader::getDataVector(const std::string & node)
{
  std::vector<double> dataVector;
  std::string dataNode = m_mainNode+node;
  if(isGroupExisting(dataNode)){
    readVectorData(dataNode,dataVector,H5T_IEEE_F64LE);
  }else{
    cout << "TrimmingdataReader Error: node not found: " << dataNode << endl;
  }
  return dataVector;
}


std::vector<uint32_t> DsscHDF5TrimmingDataReader::readRmpFineTrmSettings()
{
  std::vector<uint32_t> rmpFineTrmSettings;
  std::string irampSettingsNode = m_mainNode+"RmpFineTrmSettings";
  if(isGroupExisting(irampSettingsNode)){
    readVectorData(irampSettingsNode,rmpFineTrmSettings,H5T_STD_U32LE);
  }else{
    cout << "TrimmingdataReader Error: node not found: " << irampSettingsNode << endl;
  }
  return rmpFineTrmSettings;
}


utils::ADCGainFitResultsVec DsscHDF5TrimmingDataReader::readADCGainFitParameterVec()
{
  utils::ADCGainFitResultsVec fitResultsVector;
  const std::string node = m_mainNode + "ADCGainFitParameters";

  // if it is a parameter file load the data from file
  // else generate from ADCGainMapSummary file with parameters
  if(isGroupExisting(node))
  {
    cout << "Data Node found: " << node << endl;
    std::vector<std::vector<double>> dataVector;
    readVectorData2(node,dataVector,H5T_IEEE_F64LE,false);

    size_t numPixels = dataVector.size();
    if(numPixels == 0){
      std::cout << "TrimmingDataReader Error: no fit parameter data found in file" << std::endl;
      return fitResultsVector;
    }
    size_t numValues = dataVector.front().size();
    if(numValues != utils::ADCGainFitResults::NUMVALS) {
      std::cout << "TrimmingDataReader Error: ADCGainFitParameters do not have the correct dimenstions" << std::endl;
      return fitResultsVector;
    }

    fitResultsVector.resize(numPixels);

    //fill data into fit results array structure
#pragma omp parallel for
    for(size_t idx = 0; idx<numPixels; idx++){
      const auto & pixelVector = dataVector[idx];
      auto & pixelResArray = fitResultsVector[idx];
      std::copy(pixelVector.begin(),pixelVector.end(),pixelResArray.begin());
    }

    const std::string meanNode = m_mainNode + "MeanFitParameters";

    std::vector<double> meanParams;
    readVectorData(meanNode,meanParams,H5T_IEEE_F64LE);
    utils::ADCGainFitResult meanParamsRes;
    meanParamsRes[0] = 65536; //meanValue
    std::copy(meanParams.begin(),meanParams.end(),meanParamsRes.begin()+1);
    fitResultsVector.push_back(meanParamsRes);

    return fitResultsVector;
  }
  else
  {
    cout << "Data Node not found: " << node << endl;
    const auto path = utils::getFilePath(fileName);
    if(m_measurementName != "ADCGainMap"){
      const std::string gainMapNode = m_mainNode + "ADCGainMap";
      if(!isGroupExisting(gainMapNode) && m_fileVersion == 0){
        cout << "ERROR: ADCGainRatio node not found in file" << endl;
        return fitResultsVector;
      }
    }

    const std::string gainMapNode = m_mainNode + "ADCGainMap";
    if(!isGroupExisting(gainMapNode)){
      std::cout << "TrimmingDataReader Error: could not find ADCGainRatio Node in ADCGainMap file" << std::endl;
      return fitResultsVector;
    }

    std::cout << "TrimmingDataReader Info: GainRatio node found, read data" << std::endl;

    std::vector<double> dataVector;
    readVectorData(gainMapNode,dataVector,H5T_IEEE_F64LE); // numPixels x numSettings
    if(dataVector.size() == 0) return fitResultsVector;


    size_t numPixels = 65536;
    size_t numSettings = dataVector.size()/numPixels;

    std::cout << "TrimmingDataReader Info: dimension are " << numPixels << " x " << numSettings << std::endl;

    // fill rampfinetrm settings vector from file or guess parameters
    std::vector<double> rmpFineTrimVec;
    const std::string settingsNode = m_mainNode + "RmpFineTrmSettings";
    if(isGroupExisting(settingsNode)){
      readVectorData(settingsNode,rmpFineTrimVec,H5T_IEEE_F64LE);
    }else{
      for(size_t i=0; i<numSettings; i++){
        rmpFineTrimVec.push_back(i);
      }
    }

    if(rmpFineTrimVec.size() != numSettings){
      std::cout << "TrimmingDataReader Error: ADCGainMap rmpFineTrm settings vector size does not fit to number of Values" << std::endl;
    }

    fitResultsVector = DsscHDF5TrimmingDataWriter::savePixelADCGainValues(path,"ADCGainFitParameters.h5",dataVector,utils::convertVectorType<double,uint32_t>(rmpFineTrimVec));
  }

  return fitResultsVector;
}


utils::FitResultsVec DsscHDF5TrimmingDataReader::readPixelFitResultsVec()
{
  const std::string node = m_mainNode + "SpectrumFitResults";

  utils::FitResultsVec fitResults;

  if(!isGroupExisting(node))
  {
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "Import Error: No Spectrum fit results entry found" << endl;
    return fitResults;
  }

  cout << "Data Node found: " << node << endl;
  std::vector<std::vector<double>> dataVector;
  readVectorData2(node,dataVector,H5T_IEEE_F64LE,false);

  size_t numPixels = dataVector.size();

  fitResults.resize(numPixels);

#pragma omp parallel for
  for(size_t idx=0; idx<numPixels; idx++){
    fitResults[idx].fill(dataVector[idx]);
  }

  return fitResults;
}


utils::FitResultsMap DsscHDF5TrimmingDataReader::readPixelFitResultsMap()
{
  const std::string node = m_mainNode + "SpectrumFitResults";

  utils::FitResultsMap fitResults;
  // if it is a parameter file load the data from file
  // else generate from ADCGainMapSummary file with parameters
  if(isGroupExisting(node))
  {
    cout << "Data Node found: " << node << endl;
    std::vector<std::vector<double>> dataVector;
    readVectorData2(node,dataVector,H5T_IEEE_F64LE,false);

    size_t numPixels = dataVector.size();

#pragma omp parallel for
    for(size_t idx=0; idx<numPixels; idx++){
      utils::SpectrumFitResult result;
      result.fill(dataVector[idx]);
      fitResults[idx] = result;
    }
  }else{
    cout << "Node not found : " << node << endl;
  }
  return fitResults;
}


utils::SpectrumGainMapFitResultVec DsscHDF5TrimmingDataReader::readPixelGainMapFitResultsVec(const string &paramName)
{
  const std::string node = m_mainNode + paramName;

  utils::SpectrumGainMapFitResultVec fitResults;
  // if it is a parameter file load the data from file
  // else generate from ADCGainMapSummary file with parameters
  if(isGroupExisting(node))
  {
    cout << "Data Node found: " << node << endl;
    std::vector<std::vector<double>> dataVector;
    readVectorData2(node,dataVector,H5T_IEEE_F64LE,false);

    size_t numPixels = dataVector.size();

    fitResults.resize(numPixels);

#pragma omp parallel for
    for(size_t idx=0; idx<numPixels; idx++){
      fitResults[idx].fill(dataVector[idx]);
    }
  }
  return fitResults;
}

SuS::DsscGainParamMap DsscHDF5TrimmingDataReader::readGainParamsMap()
{
  std::string paramNode = m_mainNode+"GainParams";

  if(!isGroupExisting(paramNode)){
    return SuS::DsscGainParamMap();
  }

  SuS::DsscGainParamMap gainParams;
  for(auto && paramName : SuS::DsscGainParamMap::s_gainModeParamNames){
    double value;
    readValue(paramNode+"/"+paramName,value);
    gainParams[paramName] = value;
  }
  return gainParams;
}
