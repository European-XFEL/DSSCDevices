
#include "DsscHDF5Writer.h"
#define DSSCTRAINDATA_INIT
#include "DsscTrainData.hh"
#include "DsscModuleInfo.h"
#include "DsscTrainDataProcessor.h"
//#define DEBUG


#define RUNBASEDIR "/RUN/DSSC/configuration/"
#define DATAVERSION 5
// Version 5 = new 'ModuleInfo' section added
// Version 4 = new 'imageFormat' filed and variable pixel sort modes for faster data access
// Version 3 = new 'is8BitMode' field

bool DsscHDF5Writer::enHDF5Compression = false;
int  DsscHDF5Writer::hdf5CompressionLevel = 1;
bool DsscHDF5Writer::en8BitMode = false;

DsscHDF5Writer::WriterModuleInfo DsscHDF5Writer::s_writerModuleInfo {"Qx",1,0xDDDD};

const uint64_t DsscHDF5Writer::m_numBytesInTrain = 104870912;

//8 bit conversion never tested
VectorQueue<uint8_t> DsscHDF5Writer::s_8BitStorages(4,800*16*4096);

void DsscHDF5Writer::createNewGroup(hid_t & fileHDF5, const std::string & groupName)
{
  hid_t  group_id = H5Gcreate2(fileHDF5, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  /* Close the group. */
  H5Gclose(group_id);
}


void DsscHDF5Writer::createGroupStructureConfig(hid_t & fileHDF5)
{
  createNewGroup(fileHDF5, "/RUN");
  createNewGroup(fileHDF5, "/RUN/DSSC");

  createNewGroup(fileHDF5, RUNBASEDIR );
  createNewGroup(fileHDF5, RUNBASEDIR "TrainBuilderDataSize");
  createNewGroup(fileHDF5, RUNBASEDIR "checksumSize");
  createNewGroup(fileHDF5, RUNBASEDIR "detectorDataSize");
  createNewGroup(fileHDF5, RUNBASEDIR "disable");
  createNewGroup(fileHDF5, RUNBASEDIR "magicNumberSize");
  createNewGroup(fileHDF5, RUNBASEDIR "minorVersion");
  createNewGroup(fileHDF5, RUNBASEDIR "moduleHeight");
  createNewGroup(fileHDF5, RUNBASEDIR "moduleWidth");
}


void DsscHDF5Writer::createGroupStructureConfig(hid_t & fileHDF5, const DsscHDF5ConfigData & configData)
{
  createGroupStructureConfig(fileHDF5);

  createGroupStructureRegister(fileHDF5,configData.pixelRegisterDataVec);
  createGroupStructureRegister(fileHDF5,configData.jtagRegisterDataVec);
  createGroupStructureRegister(fileHDF5,configData.iobRegisterData);
  createGroupStructureRegister(fileHDF5,configData.epcRegisterData);

  createGroupStructureSequence(fileHDF5,"Sequencer",configData.sequencerData);
  createGroupStructureSequence(fileHDF5,"ControlSequence",configData.controlSequenceData);
}


void DsscHDF5Writer::createGroupStructureRegister(hid_t &fileHDF5, const DsscHDF5RegisterConfigVec &registerConfigVec)
{
  for(auto && registerConfig : registerConfigVec){
    createGroupStructureRegister(fileHDF5,registerConfig);
  }
}


void DsscHDF5Writer::createGroupStructureRegister(hid_t &fileHDF5, const DsscHDF5RegisterConfig &registerConfig)
{
  if(registerConfig.numModuleSets == 0) return;

  const std::string baseName = RUNBASEDIR + registerConfig.registerName;

  createNewGroup(fileHDF5, baseName);

  int modSet = 0;
  for(const auto & modSetName : registerConfig.moduleSets){
    createNewGroup(fileHDF5, baseName + "/"+ modSetName);

    for(const auto & signalName : registerConfig.signalNames[modSet]){
      const std::string sigDirName = baseName + "/"+ modSetName + "/" + signalName;
      createNewGroup(fileHDF5, sigDirName);
    }
    modSet++;
  }
}


void DsscHDF5Writer::createGroupStructureSequence(hid_t &fileHDF5, const std::string & node, const DsscHDF5SequenceData &sequenceData)
{
  if(sequenceData.empty()) return;

  createNewGroup(fileHDF5, RUNBASEDIR + node);
}


void DsscHDF5Writer::createGroupStructure(hid_t & fileHDF5)
{
  createNewGroup(fileHDF5, "/INSTRUMENT");
  createNewGroup(fileHDF5, "/INSTRUMENT/DSSC");

  createNewGroup(fileHDF5, "/INSTRUMENT/DSSC/specificData");
  createNewGroup(fileHDF5, "/INSTRUMENT/DSSC/pulseData");
  createNewGroup(fileHDF5, "/INSTRUMENT/DSSC/imageData");
  createNewGroup(fileHDF5, "/INSTRUMENT/DSSC/trainData");
}

std::array<hsize_t,4> DsscHDF5Writer::getDims(utils::DsscTrainData * trainData)
{
  hsize_t numASICs = (hsize_t)trainData->availableASICs.size();
  hsize_t numFrames = (hsize_t)trainData->pulseCnt;

  std::array<hsize_t,4> dimsf {numASICs,64,64,numFrames};
  switch(trainData->getFormat()){
    case utils::DsscTrainData::DATAFORMAT::PIXEL : dimsf = {numASICs,64,64,800}; break;
    case utils::DsscTrainData::DATAFORMAT::ASIC  : dimsf = {numFrames,numASICs,64,64}; break;
    case utils::DsscTrainData::DATAFORMAT::IMAGE : dimsf = {1,numFrames,128,512}; break;
  }

  return dimsf;
}


void DsscHDF5Writer::addImages(hid_t & fileHDF5, utils::DsscTrainData * trainData, hsize_t numRows, hsize_t numColumns)
{
  std::string node = "/INSTRUMENT/DSSC/imageData/";
  hid_t properties = H5Pcreate(H5P_DATASET_CREATE);

  hsize_t numASICs = (hsize_t)trainData->availableASICs.size();

  hsize_t numFrames = (hsize_t)trainData->pulseCnt;

  const auto dimsf = getDims(trainData);

  // set compression
  if(enHDF5Compression){
  //chunked images allow fast access to image tiles (parts of the image ( 8x8 or so
    H5Pset_chunk(properties,dimsf.size(),dimsf.data());
    H5Pset_deflate(properties,hdf5CompressionLevel);
  }


  hsize_t dim = 1;
  uint8_t is8BitMode = (en8BitMode)? 1 : 0;
  writeData<uint8_t>(fileHDF5,node,"is8BitMode",H5T_STD_U8LE,&dim,&is8BitMode);
//experimental 8 bit mode, not tested
  if(en8BitMode)
  {
    size_t numValues = numFrames*numASICs*numRows*numColumns;

    hid_t datatype = H5Tcopy(H5T_STD_U8LE);
    H5Tset_order(datatype, H5T_ORDER_LE);

    hid_t dataspace = H5Screate_simple(dimsf.size(),dimsf.data(),NULL);

    hid_t dataset = H5Dcreate2(fileHDF5,"/INSTRUMENT/DSSC/imageData/data",datatype,dataspace,
                                H5P_DEFAULT,properties,H5P_DEFAULT);

    s_8BitStorages.resize(numValues);
    auto next8BitStorage = s_8BitStorages.pop_front();
    {
      utils::Timer timer;
      // thread safe queue that stores 8 data vectors allocates enough memory for parallel usage of 8 image writers

      utils::reduceTo8BitData(trainData->imageData(),next8BitStorage,numValues);
      std::cout << "DsscHDF5Writer DEBUG: data reduction to 8 bit finished after: " << std::endl;
    }

    herr_t status = H5Dwrite(dataset,H5T_STD_U8LE,H5S_ALL,H5S_ALL,H5P_DEFAULT,next8BitStorage);
    if(status<0) std::cout << "HDF5Writer: Could not write 8 bit images to HDF5 file" <<  std::endl;

    H5Sclose(dataspace);
    H5Tclose(datatype);
    H5Dclose(dataset);

    s_8BitStorages.push_back(next8BitStorage);
  }
  else
  {
    hid_t datatype = H5Tcopy(H5T_STD_U16LE);
    H5Tset_order(datatype, H5T_ORDER_LE);

    hid_t dataspace = H5Screate_simple(dimsf.size(),dimsf.data(),NULL);

    hid_t dataset = H5Dcreate2(fileHDF5,"/INSTRUMENT/DSSC/imageData/data",datatype,dataspace,
                                H5P_DEFAULT,properties,H5P_DEFAULT);

    herr_t status = H5Dwrite(dataset,H5T_STD_U16LE,H5S_ALL,H5S_ALL,H5P_DEFAULT,trainData->imageData());
    if(status<0) std::cout << "HDF5Writer: Could not write images to HDF5 file" <<  std::endl;

    H5Sclose(dataspace);
    H5Tclose(datatype);
    H5Dclose(dataset);
  }

  H5Pclose(properties);

  writeData<uint32_t>(fileHDF5,node,"availableASICs" ,H5T_STD_U32LE,&numASICs,trainData->availableASICs.data());

  const auto imageFormat = trainData->getFormatStr();
  writeDataString(fileHDF5,node,"imageFormat",imageFormat);
  //std::cout << "DsscHDF5 Writer: debug write image data in " << imageFormat << " format" << std::endl;
}


void DsscHDF5Writer::addMapData(hid_t & fileHDF5, const std::string & node, const std::map<std::string,uint32_t> & mapData)
{
  hsize_t dim = 1;

  for(const auto & item: mapData){
    writeData<uint32_t>(fileHDF5,node,item.first.c_str(),H5T_STD_U32LE,&dim,&item.second);
  }
}


void DsscHDF5Writer::addConfiguration(hid_t & fileHDF5)
{
  hsize_t dim = 1;

  const std::string node = "/RUN/DSSC/configuration";

  writeDataString(fileHDF5,node,"TrainBuilderDataSize/type","UINT64");
  writeData<uint64_t>(fileHDF5,node,"TrainBuilderDataSize/value",H5T_STD_U64LE,&dim,&m_numBytesInTrain);

  uint16_t value=16;
  writeDataString(fileHDF5,node,"checksumSize/type","UINT16");
  writeData<uint16_t>(fileHDF5,node,"checksumSize/value",H5T_STD_U16LE,&dim,&value);

  writeDataString(fileHDF5,node,"detectorDataSize/type","UINT64");
  writeData<uint64_t>(fileHDF5,node,"detectorDataSize/value",H5T_STD_U64LE,&dim,&m_numBytesInTrain);

  value=8;
  writeDataString(fileHDF5,node,"magicNumberSize/type","UINT16");
  writeData<uint16_t>(fileHDF5,node,"magicNumberSize/value",H5T_STD_U16LE,&dim,&value);

  value=2;
  writeDataString(fileHDF5,node,"minorVersion/type","UINT16");
  writeData<uint16_t>(fileHDF5,node,"minorVersion/value",H5T_STD_U16LE,&dim,&value);

  value=128;
  writeDataString(fileHDF5,node,"moduleHeight/type","UINT16");
  writeData<uint16_t>(fileHDF5,node,"moduleHeight/value",H5T_STD_U16LE,&dim,&value);

  value=512;
  writeDataString(fileHDF5,node,"moduleWidth/type","UINT16");
  writeData<uint16_t>(fileHDF5,node,"moduleWidth/value",H5T_STD_U16LE,&dim,&value);
}


void DsscHDF5Writer::addConfiguration(hid_t & fileHDF5, const DsscHDF5ConfigData & configData)
{
  addConfiguration(fileHDF5);

  hsize_t dim = 1;
  uint16_t numRegisters = configData.getNumRegisters();
  const std::string node = "/RUN/DSSC/configuration";
  writeData<uint16_t>(fileHDF5,node,"NumRegisters",H5T_STD_U16LE,&dim,&numRegisters);
  writeDataString(fileHDF5,node,"RegisterNames",configData.getRegisterNames());
  writeDataString(fileHDF5,node,"timestamp",configData.timestamp);

  addConfiguration(fileHDF5,configData.pixelRegisterDataVec);
  addConfiguration(fileHDF5,configData.jtagRegisterDataVec);
  addConfiguration(fileHDF5,configData.iobRegisterData);
  addConfiguration(fileHDF5,configData.epcRegisterData);

  addConfiguration(fileHDF5,"Sequencer",configData.sequencerData);
  addConfiguration(fileHDF5,"ControlSequence",configData.controlSequenceData);
}


void DsscHDF5Writer::addConfiguration(hid_t & fileHDF5, const DsscHDF5RegisterConfigVec & registerConfigVec)
{
  for(auto && registerConfig : registerConfigVec){
    addConfiguration(fileHDF5,registerConfig);
  }
}


void DsscHDF5Writer::addConfiguration(hid_t & fileHDF5, const DsscHDF5RegisterConfig & registerConfig)
{
  if(registerConfig.numModuleSets == 0) return;

  hsize_t dim1 = 1;

  const std::string baseName = RUNBASEDIR + registerConfig.registerName;

  writeData<uint32_t>(fileHDF5,baseName,"NumModuleSets",H5T_STD_U32LE,&dim1,&registerConfig.numModuleSets);

  std::string moduleSetsStr;

  for(const auto & modSetStr : registerConfig.moduleSets){
    moduleSetsStr += modSetStr + ";";
  }
  writeDataString(fileHDF5,baseName,"ModuleSets",moduleSetsStr);


  int modSet = 0;
  for(const auto & modSetName : registerConfig.moduleSets){
    const std::string setDirName = baseName + "/"+ modSetName;

    hsize_t dimValues = registerConfig.numberOfModules[modSet];

    writeData<uint32_t>(fileHDF5,setDirName,"Modules",H5T_STD_U32LE,&dimValues,registerConfig.modules[modSet].data());
    writeData<uint32_t>(fileHDF5,setDirName,"Outputs",H5T_STD_U32LE,&dimValues,registerConfig.outputs[modSet].data());

    writeData<uint32_t>(fileHDF5,setDirName,"NumBitsPerModule",H5T_STD_U32LE,&dim1,&registerConfig.numBitsPerModule[modSet]);
    writeData<uint32_t>(fileHDF5,setDirName,"Address",H5T_STD_U32LE,&dim1,&registerConfig.addresses[modSet]);
    writeData<uint32_t>(fileHDF5,setDirName,"SetIsReverse",H5T_STD_U32LE,&dim1,&registerConfig.setIsReverse[modSet]);
    writeData<uint32_t>(fileHDF5,setDirName,"NumSignals",H5T_STD_U32LE,&dim1,&registerConfig.numSignals[modSet]);
    writeData<uint32_t>(fileHDF5,setDirName,"NumModules",H5T_STD_U32LE,&dim1,&registerConfig.numberOfModules[modSet]);


    int sig = 0;
    for(const auto & signalName : registerConfig.signalNames[modSet]){
      const std::string sigDirName = setDirName + "/" + signalName;
      writeDataString(fileHDF5,sigDirName,"BitPositions",registerConfig.bitPositions[modSet][sig]);

      writeData<uint32_t>(fileHDF5,sigDirName,"ReadOnly",H5T_STD_U32LE,&dim1,&registerConfig.readOnly[modSet][sig]);
      writeData<uint32_t>(fileHDF5,sigDirName,"ActiveLow",H5T_STD_U32LE,&dim1,&registerConfig.activeLow[modSet][sig]);
      writeData<uint32_t>(fileHDF5,sigDirName,"AccessLevel",H5T_STD_U32LE,&dim1,&registerConfig.accessLevels[modSet][sig]);
      writeData<uint32_t>(fileHDF5,sigDirName,"ConfigValues",H5T_STD_U32LE,&dimValues,registerConfig.registerData[modSet][sig].data());

      sig++;
    }
    modSet++;
  }
}

void DsscHDF5Writer::addConfiguration(hid_t & fileHDF5, const std::string & node,  const DsscHDF5SequenceData & sequenceData)
{
  if(sequenceData.empty()) return;

  const std::string baseNode = RUNBASEDIR + node;
  hsize_t dim = 1;

  for(const auto & mapItem : sequenceData){
    writeData<uint32_t>(fileHDF5,baseNode,mapItem.first,H5T_STD_U32LE,&dim,&mapItem.second);
  }
}

void DsscHDF5Writer::addDescriptors(hid_t & fileHDF5, utils::DsscTrainData *trainData)
{
  static const std::vector<uint32_t> statusVec(800,0);
  static const std::vector<uint32_t> lengthVec(800,0x20000);

  hsize_t pulseCntPtr = (hsize_t)trainData->pulseCnt;
  std::vector<uint64_t> trainIDVec(trainData->pulseCnt,trainData->trainId);

  std::string node = "/INSTRUMENT/DSSC/pulseData";
  writeData<uint16_t>(fileHDF5,node,"cellId" ,H5T_STD_U16LE,&pulseCntPtr,trainData->cellIds.data());
  writeData<uint32_t>(fileHDF5,node,"length" ,H5T_STD_U32LE,&pulseCntPtr,lengthVec.data());
  writeData<uint64_t>(fileHDF5,node,"pulseId",H5T_STD_U64LE,&pulseCntPtr,trainData->pulseIds.data());
  writeData<uint32_t>(fileHDF5,node,"status" ,H5T_STD_U32LE,&pulseCntPtr,statusVec.data());
  writeData<uint64_t>(fileHDF5,node,"trainId",H5T_STD_U64LE,&pulseCntPtr,trainIDVec.data());

  node = "/INSTRUMENT/DSSC/imageData";
  writeData<uint64_t>(fileHDF5,node,"pulseId",H5T_STD_U64LE,&pulseCntPtr,trainData->pulseIds.data());
  writeData<uint64_t>(fileHDF5,node,"trainId",H5T_STD_U64LE,&pulseCntPtr,trainIDVec.data());
}


void DsscHDF5Writer::addSpecific(hid_t & fileHDF5, utils::DsscTrainData *trainData)
{
  hsize_t numPPTData = (hsize_t)trainData->pptData.size();
  hsize_t numSIBData = (hsize_t)trainData->sibData.size();
  hsize_t numASICTrailer = (hsize_t)trainData->asicTrailerData.size();

  std::string node = "/INSTRUMENT/DSSC/specificData";
  writeData<uint16_t>(fileHDF5,node,"pptData",    H5T_STD_U16LE,&numPPTData,trainData->pptData.data());
  writeData<uint16_t>(fileHDF5,node,"sibData",    H5T_STD_U16LE,&numSIBData,trainData->sibData.data());
  writeData<uint8_t> (fileHDF5,node,"asicTrailer",H5T_STD_U8LE,&numASICTrailer,trainData->asicTrailerData.data());
}


void DsscHDF5Writer::addTrain(hid_t & fileHDF5, utils::DsscTrainData * trainData)
{
  hsize_t valPtr = 1;
  uint32_t version = DATAVERSION;
  const std::string node = "/INSTRUMENT/DSSC/trainData";
  writeData<uint32_t>(fileHDF5,node,"dataId"           ,H5T_STD_U32LE,&valPtr,&trainData->dataId);
  writeData<uint32_t>(fileHDF5,node,"imageCount"       ,H5T_STD_U32LE,&valPtr,&trainData->pulseCnt);
  writeData<uint32_t>(fileHDF5,node,"detLinkId"        ,H5T_STD_U32LE,&valPtr,&trainData->detLinkId);
  writeData<uint32_t>(fileHDF5,node,"tbLinkId"         ,H5T_STD_U32LE,&valPtr,&trainData->tbLinkId);
  writeData<uint64_t>(fileHDF5,node,"trainId"          ,H5T_STD_U64LE,&valPtr,&trainData->trainId);
  writeData<uint32_t>(fileHDF5,node,"detSpecificLength",H5T_STD_U32LE,&valPtr,&trainData->detSpecificLength);
  writeData<uint32_t>(fileHDF5,node,"tbSpecificLength" ,H5T_STD_U32LE,&valPtr,&trainData->tbSpecificLength);
  writeData<uint32_t>(fileHDF5,node,"version"          ,H5T_STD_U32LE,&valPtr,&version);
}


void DsscHDF5Writer::addDoubleVectorData(hid_t & fileHDF5, const std::string & node, const std::string & name, const std::vector<std::vector<double>>& doubleVector)
{
  constexpr int numDims = 2;

  hsize_t numPixels = doubleVector.size();

  if(numPixels == 0) return;

  hsize_t numPixelValues = doubleVector.front().size();

  std::vector<double> singleDataVector(numPixels*numPixelValues);

#pragma omp parallel for
  for(size_t pxIdx = 0; pxIdx < numPixels; pxIdx++ ){
    const auto & pxValues = doubleVector[pxIdx];
    std::copy(pxValues.begin(),pxValues.end(),singleDataVector.begin() + (pxIdx*numPixelValues));
  }

  hsize_t dimsf[numDims]{numPixels, numPixelValues};

  hid_t datatype = H5Tcopy(H5T_IEEE_F64LE);

  hid_t dataspace = H5Screate_simple(numDims,dimsf,NULL);

  hid_t dataset = H5Dcreate2(fileHDF5,(node + "/" + name).c_str(),datatype,dataspace,
                              H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);

  herr_t status = H5Dwrite(dataset,H5T_IEEE_F64LE,H5S_ALL,H5S_ALL,H5P_DEFAULT,singleDataVector.data());
  if(status<0) std::cout << "HDF5Writer: Could not write double vector data " << name << " to HDF5 file" <<  std::endl;

  H5Sclose(dataspace);
  H5Tclose(datatype);
  H5Dclose(dataset);
}


void DsscHDF5Writer::addCorrectionData(hid_t & fileHDF5, const std::string & node, const std::string & name, const std::vector<std::vector<float>>& correctionMap)
{
  hsize_t numPixels = correctionMap.size();

  constexpr int numDims = 2;

  hsize_t dimsf[numDims]{numPixels, 800};

  hid_t datatype = H5Tcopy(H5T_IEEE_F32LE);

  hid_t dataspace = H5Screate_simple(numDims,dimsf,NULL);

  hid_t dataset = H5Dcreate2(fileHDF5,(node + "/" + name).c_str(),datatype,dataspace,
                              H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);

  herr_t status = H5Dwrite(dataset,H5T_IEEE_F32LE,H5S_ALL,H5S_ALL,H5P_DEFAULT,correctionMap.data());
  if(status<0) std::cout << "HDF5Writer: Could not write correction data to HDF5 file" <<  std::endl;

  H5Sclose(dataspace);
  H5Tclose(datatype);
  H5Dclose(dataset);
}


void DsscHDF5Writer::saveConfiguration(std::string fileName)
{
  if(fileName.rfind(".h5") == std::string::npos){
    fileName += ".h5";
  }

  hid_t fileHDF5 = H5Fcreate(fileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

  createGroupStructureConfig(fileHDF5);

  addConfiguration(fileHDF5);

  addModuleInfo(fileHDF5);

  H5Fclose(fileHDF5);
}


void DsscHDF5Writer::saveConfiguration(std::string fileName, const DsscHDF5ConfigData & h5ConfigData)
{
  if(fileName.rfind(".h5") == std::string::npos){
    fileName += ".h5";
  }
  hid_t fileHDF5 = H5Fcreate(fileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

  createGroupStructureConfig(fileHDF5,h5ConfigData);

  addConfiguration(fileHDF5,h5ConfigData);

  addModuleInfo(fileHDF5);

  H5Fclose(fileHDF5);

  std::cout << "HDF5 Configuration saved in " << fileName << std::endl;
}


utils::MeanRMSVectors DsscHDF5Writer::saveToFile(std::string fileName, const std::vector<utils::StatsAcc> & dataStatsAcc, const std::vector<uint32_t> & activeASICs, uint32_t numTrains)
{
  if(dataStatsAcc.empty()){
    std::cout << "Can not write Statsfile, input vector empty"<< std::endl;
    return utils::MeanRMSVectors();
  }

  hsize_t numPixels = dataStatsAcc.size();

  std::vector<float> meanVec(numPixels);
  std::vector<float> rmsVec(numPixels);

  int idx = 0;
  for(const auto & statsAcc : dataStatsAcc){
     const auto stats = statsAcc.calcStats();
     meanVec[idx] = stats.mean;
     rmsVec[idx] = stats.rms;
     idx++;
  }

  if(fileName.rfind(".h5") == std::string::npos){
    fileName += ".h5";
  }

  hid_t fileHDF5 = H5Fcreate(fileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

  createNewGroup(fileHDF5, "/INSTRUMENT");
  createNewGroup(fileHDF5, "/INSTRUMENT/DSSC");

  std::string node = "/INSTRUMENT/DSSC/trainStats";
  createNewGroup(fileHDF5,node);

  hsize_t cnt1 = (hsize_t)1;
  hsize_t numASICs = (hsize_t)activeASICs.size();
  uint16_t asicCnt = activeASICs.size();
  uint64_t numAccValues = dataStatsAcc.front().numValues;
  writeData<uint16_t>(fileHDF5,node,"numASICs",   H5T_STD_U16LE,&cnt1,&asicCnt);
  writeData<uint32_t>(fileHDF5,node,"acitveASICs",H5T_STD_U32LE,&numASICs,activeASICs.data());
  writeData<uint64_t>(fileHDF5,node,"numValues",H5T_STD_U64LE,&cnt1,&numAccValues);
  writeData<uint32_t>(fileHDF5,node,"numTrains",H5T_STD_U32LE,&cnt1,&numTrains);

  writeData<float> (fileHDF5,node,"meanValues",H5T_IEEE_F32LE,&numPixels,meanVec.data());
  writeData<float> (fileHDF5,node,"rmsValues",H5T_IEEE_F32LE,&numPixels,rmsVec.data());

  addModuleInfo(fileHDF5);

  H5Fclose(fileHDF5);

  utils::MeanRMSVectors meanRmsVecs;
  meanRmsVecs.meanValues = std::move(meanVec);
  meanRmsVecs.rmsValues = std::move(rmsVec);
  return meanRmsVecs;
}


void DsscHDF5Writer::saveBaselineAndSramCorrection(const std::string & fileName, const std::vector<double> & meanSramValues, const std::vector<uint32_t> & activeASICs, uint32_t numTrains)
{
  std::vector<float> backgroundValues(utils::s_totalNumPxs);
  std::vector<std::vector<float>> sramCorrectionValues(utils::s_totalNumPxs,std::vector<float>(utils::s_numSram,0));

#pragma omp parallel for
  for(uint px=0; px<utils::s_totalNumPxs; px++){
    const double * pxMeanValues = meanSramValues.data() + px*utils::s_numSram;
    const auto stats = utils::getMeandAndRMS(pxMeanValues,utils::s_numSram);
    backgroundValues[px] = stats.mean;

    auto & pxSramCorrection = sramCorrectionValues[px];
    for(uint sram=0; sram<utils::s_numSram; sram++){
      pxSramCorrection[sram] = pxMeanValues[sram] - stats.mean;
    }
  }

  saveBaselineAndSramCorrection(fileName,backgroundValues,sramCorrectionValues,activeASICs,numTrains);
}


void DsscHDF5Writer::saveBaselineAndSramCorrection(std::string fileName, const std::vector<float> & backGroundValues, const std::vector<std::vector<float>> & sramCorrectionValues,const std::vector<uint32_t> & activeASICs, uint32_t numTrains)
{
  if(backGroundValues.empty()){
    std::cout << "Can not write SRamCorrection File, input vector empty"<< std::endl;
    return;
  }

  uint numPixels = backGroundValues.size();
  uint numSram = sramCorrectionValues.front().size();

  if(fileName.rfind(".h5") == std::string::npos){
    fileName += ".h5";
  }

  hid_t fileHDF5 = H5Fcreate(fileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

  createNewGroup(fileHDF5, "/INSTRUMENT");
  createNewGroup(fileHDF5, "/INSTRUMENT/DSSC");

  std::string node = "/INSTRUMENT/DSSC/SramCorrection";
  createNewGroup(fileHDF5,node);

  hsize_t cnt1 = (hsize_t)1;

  uint16_t asicCnt = activeASICs.size();
  writeData<uint16_t>(fileHDF5,node,"numASICs",   H5T_STD_U16LE,&cnt1,&asicCnt);

  hsize_t numASICs = (hsize_t)activeASICs.size();
  writeData<uint32_t>(fileHDF5,node,"acitveASICs",H5T_STD_U32LE,&numASICs,activeASICs.data());

  uint64_t numAccValues = numTrains*numSram;
  writeData<uint64_t>(fileHDF5,node,"numValues",H5T_STD_U64LE,&cnt1,&numAccValues);

  writeData<uint32_t>(fileHDF5,node,"numTrains",H5T_STD_U32LE,&cnt1,&numTrains);

  hsize_t numBackgroundValues = numPixels;
  writeData<float> (fileHDF5,node,"baselineData",H5T_IEEE_F32LE,&numBackgroundValues,backGroundValues.data());


  std::vector<float> correctionValues(numSram*numPixels);

#pragma omp parallel for
  for(uint px=0; px<numPixels; px++){
    const auto & pixelCorrectionValues = sramCorrectionValues[px];
    auto * pxValues = correctionValues.data() + px * numSram;
    std::copy(pixelCorrectionValues.begin(),pixelCorrectionValues.end(),pxValues);
  }

  hsize_t numValues = correctionValues.size();
  writeData<float>(fileHDF5,node,"correctionData",H5T_IEEE_F32LE,&numValues,correctionValues.data());

  const auto sramSlopes = utils::DsscTrainDataProcessor::computeSramSlopeVectorFromMeanSramMatrix(sramCorrectionValues);
  numValues = sramSlopes.size();
  writeData<double>(fileHDF5,node,"sramSlopes",H5T_IEEE_F64LE,&numValues,sramSlopes.data());

  const auto sramRMSValues = utils::DsscTrainDataProcessor::computeMeanSramRMSMatrix(sramCorrectionValues);
  numValues = sramRMSValues.size();
  writeData<double>(fileHDF5,node,"meanSramRMSValues",H5T_IEEE_F64LE,&numValues,sramSlopes.data());

  addModuleInfo(fileHDF5);

  H5Fclose(fileHDF5);

  std::cout << "Background and SramCorrection Image saved to " << fileName << std::endl;
}


void DsscHDF5Writer::saveToFile(std::string fileName, utils::DsscTrainData * trainData, int numRows, int numColumns)
{
#ifdef DEBUG
  std::cout << "HDF5Writer: Save HDF5 to " << fileName << std::endl;
#endif
  if(fileName.rfind(".h5") == std::string::npos){
    fileName += ".h5";
  }

  hid_t fileHDF5 = H5Fcreate(fileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

  createGroupStructure(fileHDF5);

  const auto specificData =trainData->getSpecificData();
  s_writerModuleInfo.quadrantId = specificData.userSpecific1;
  s_writerModuleInfo.moduleNr   = specificData.moduleNr;
  s_writerModuleInfo.iobSerial  = specificData.iobSerial;

  addImages(fileHDF5,trainData,numRows,numColumns);

  addDescriptors(fileHDF5,trainData);

  addSpecific(fileHDF5,trainData);

  addTrain(fileHDF5,trainData);

  addModuleInfo(fileHDF5);

  H5Fclose(fileHDF5);
}


bool DsscHDF5Writer::isGroupExisting(hid_t fileHDF5, const std::string & group)
{
  auto res = H5Lexists(fileHDF5,group.c_str(),H5P_DEFAULT);
  return res>0;
}


void DsscHDF5Writer::updateModuleInfo(const std::string &quadrantId, uint moduleNr, uint iobSerial)
{
  if(s_writerModuleInfo.quadrantId != "Qx"){
    utils::DsscModuleInfo::setIOBSerial(quadrantId,moduleNr,iobSerial);
    std::cout << "DsscHDF5Writer Info: Module Info is overwritten" << std::endl;
  }else{
    std::cout << "DsscHDF5Writer Info: Module Info is updated" << std::endl;
  }

  s_writerModuleInfo.quadrantId = quadrantId;
  s_writerModuleInfo.moduleNr   = moduleNr;
  s_writerModuleInfo.iobSerial  = iobSerial;
}


bool DsscHDF5Writer::checkModuleInfo(const std::string & quadrantId, uint moduleNr, uint iobSerial)
{
  if(s_writerModuleInfo.quadrantId == "Qx"){
    updateModuleInfo(quadrantId,moduleNr,iobSerial);
    return true;
  }

  if(s_writerModuleInfo.quadrantId != quadrantId){
    std::cout << "DsscHDF5Writer Warning: Quadrant Id is different: " << quadrantId << " / " << s_writerModuleInfo.quadrantId << std::endl;
    return false;
  }

  if(s_writerModuleInfo.moduleNr != moduleNr){
    std::cout << "DsscHDF5Writer Warning: ModuleNr is different: " << moduleNr << " / " << s_writerModuleInfo.moduleNr << std::endl;
    return false;
  }

  if(s_writerModuleInfo.iobSerial != iobSerial){
    std::cout << "DsscHDF5Writer Warning: IOBSerial is different: " << iobSerial << " / " << s_writerModuleInfo.iobSerial << std::endl;
    //return false; // should not cause false, since IOB can be changed without effect on data
  }
  return true;
}


void DsscHDF5Writer::addModuleInfo(hid_t fileHDF5, const std::string & node)
{
  if(isGroupExisting(fileHDF5,node + "ModuleInfo")){
    std::cout << "ModuleInfo info already existing" << std::endl;
    return;
  }

  createNewGroup(fileHDF5, node+"ModuleInfo");
  writeDataString(fileHDF5,node+"ModuleInfo","quadrantId",s_writerModuleInfo.quadrantId);
  hsize_t singleValue = 1;
  writeData(fileHDF5,node+"ModuleInfo","moduleNr",H5T_STD_U16LE,&singleValue,&s_writerModuleInfo.moduleNr);
  std::string moduleName = utils::DsscModuleInfo::getModuleName(s_writerModuleInfo.quadrantId,s_writerModuleInfo.moduleNr);
  writeDataString(fileHDF5,node+"ModuleInfo","moduleName",moduleName);
  writeData(fileHDF5,node+"ModuleInfo","iobSerial",H5T_STD_U32LE,&singleValue,&s_writerModuleInfo.iobSerial);
}



