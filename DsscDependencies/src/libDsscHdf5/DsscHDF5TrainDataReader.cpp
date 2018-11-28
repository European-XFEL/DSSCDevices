#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include "DsscHDF5TrainDataReader.h"
#include "utils.h"
#include "DataHisto.h"
#include "DsscTrainDataProcessor.h"

using namespace std;

// #define DEBUG

uint16_t * DsscHDF5TrainDataReader::imageData = nullptr;
uint16_t * DsscHDF5TrainDataReader::pixelData = nullptr;
utils::DsscTrainData * DsscHDF5TrainDataReader::trainData = nullptr;

DsscHDF5TrainDataReader::DsscHDF5TrainDataReader()
  : DsscHDF5Reader()
{
}


DsscHDF5TrainDataReader::DsscHDF5TrainDataReader(const string & readFileName)
  : DsscHDF5Reader(readFileName),
    dimX(64),
    dimY(64),
    numFrames(800),
    numASICs(1),
    maxASIC(0)
{
  if(!loaded) return;

  readAcquisitionTime();

  // old data version
  m_dataFormat = DATAFORMAT::ASIC;

  if(!checkMattiaVersion()){
    readTrainID();
    readPPTData();
    readAvailableASICs();
    readImageDataVersion();
   // readSIBData();
   readASICTrailerData();
   if(version >=4){
     readImageFormat();
   }
  }else{
    readTrainIDMattia();
    asicTrailerData.resize(256,0);
  }
}


DsscHDF5TrainDataReader::~DsscHDF5TrainDataReader()
{
}


bool DsscHDF5TrainDataReader::checkMattiaVersion()
{
  //cout << "---> check data format - ignore errors <---" << endl;

  dataset_id = H5Dopen2(h5File, "INSTRUMENT/DSSC/trainData/trainId", H5P_DEFAULT);
  isMattiaHDF5Format = (dataset_id<0);
  if(dataset_id>=0){
    H5Dclose(dataset_id);
  }
  if(isMattiaHDF5Format){
    maxASIC = 15;
    cout << "---> Mattia Ladder HDF5 Found <---" << endl;
    availableASICs.clear();
    for(int i=0; i<16; i++){
      availableASICs.push_back(i);
    }
  }
  return isMattiaHDF5Format;
}


void DsscHDF5TrainDataReader::readTrainID()
{
  dataset_id = H5Dopen2(h5File, "INSTRUMENT/DSSC/trainData/trainId", H5P_DEFAULT);
  H5Dread(dataset_id, H5T_STD_U64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT,&trainID);
  H5Dclose(dataset_id);

  cout << "## HDF5 File Reader: Read File with Train ID = " << trainID << endl;
}


void DsscHDF5TrainDataReader::readImageDataVersion()
{
  version = 1;
  if(isGroupExisting("INSTRUMENT/DSSC/trainData/version")){
    dataset_id = H5Dopen2(h5File, "INSTRUMENT/DSSC/trainData/version", H5P_DEFAULT);
    if(dataset_id>=0){
      H5Dread(dataset_id, H5T_STD_U32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT,&version);
      H5Dclose(dataset_id);
    }
  }
}


void DsscHDF5TrainDataReader::readImageFormat()
{
  string imageFormat;
  readDataString("INSTRUMENT/DSSC/imageData/imageFormat",imageFormat);
  m_dataFormat = utils::DsscTrainData::getFormat(imageFormat);
  cout << "Data found in " << imageFormat << endl;
}


void DsscHDF5TrainDataReader::readTrainIDMattia()
{
  dataset_id = H5Dopen2(h5File, "INSTRUMENT/DSSC/infotrain", H5P_DEFAULT);
  if(dataset_id<0){
    cout << "cant find: INSTRUMENT/DSSC/infotrain" << endl;
    return;
  }
/*
  cout << "Storage Size "<< H5Dget_storage_size(dataset_id) << endl;
  auto datatype = H5Dget_type(dataset_id);
  cout << "Type "<< datatype << endl;
  cout << "Type "<< H5Tget_tag(datatype) << endl;
  cout << "Size "<< H5Tget_size(datatype) << endl;
  cout << "Is String:" << H5Tis_variable_str(datatype) << endl;

  auto dataspace = H5Dget_space(dataset_id );
  int nDims = H5Sget_simple_extent_ndims(dataspace);
  hsize_t dims[nDims];
  hsize_t maxDims[nDims];
  H5Sget_simple_extent_dims(dataspace,dims,maxDims);

  for(int i=0; i<nDims; i++){
    cout << "dims["<<i<<"] = " << dims[i] << endl;
  }

  //H5Dread(dataset_id, H5T_STD_U8LE, H5S_ALL, H5S_ALL, H5P_DEFAULT,traininfo);

  H5Tclose(datatype);
  H5Sclose(dataspace);
*/
  H5Dclose(dataset_id);

  trainID = 0;
}


void DsscHDF5TrainDataReader::readSIBData()
{
  readVectorData("INSTRUMENT/DSSC/specificData/sibData",sibData,H5T_STD_U16LE);
}

void DsscHDF5TrainDataReader::readPPTData()
{
  readVectorData("INSTRUMENT/DSSC/specificData/pptData",pptData,H5T_STD_U16LE);
}

void DsscHDF5TrainDataReader::readASICTrailerData()
{
  readVectorData("INSTRUMENT/DSSC/specificData/asicTrailer",asicTrailerData,H5T_STD_U8LE);
}

void DsscHDF5TrainDataReader::readAvailableASICs()
{
  readVectorData("INSTRUMENT/DSSC/imageData/availableASICs",availableASICs,H5T_STD_U32LE);
  maxASIC = *std::max_element(availableASICs.begin(),availableASICs.end());
}


void DsscHDF5TrainDataReader::readAcquisitionTime()
{
  struct stat t_stat;
  stat(fileName.c_str(), &t_stat);
  struct tm * timeinfo = localtime(&t_stat.st_ctime);
  string timeStr(asctime(timeinfo));
  acTime = timeStr;
}


int DsscHDF5TrainDataReader::getTempADCValue(int ASIC)
{
  return utils::convertGCC(getASICTrailerWord(ASIC,0));
}


bool DsscHDF5TrainDataReader::isLadderMode()
{
  if(!loaded){
    cout << "file not loaded, can not read data" << endl;
    return false;
  }

  if(maxASIC == 0){
    return false;
  }

  return true;
}


const uint16_t * DsscHDF5TrainDataReader::getData(const std::vector<int> & pixelsToExport) throw (std::string)
{
  if(!loaded){
    cout << "file not loaded, can not read data" << endl;
    return nullptr;
  }

  std::string imageDataLocation = "/INSTRUMENT/DSSC/imageData/data";
  if(isMattiaHDF5Format){
    imageDataLocation = "/INSTRUMENT/DSSC/images/image";
  }

  hid_t dataset_id = H5Dopen2(h5File,imageDataLocation.c_str() , H5P_DEFAULT);
  H5D_space_status_t status;
  H5Dget_space_status(dataset_id,&status);

  hid_t dataspace = H5Dget_space(dataset_id);
  int nDims = H5Sget_simple_extent_ndims(dataspace);
  hsize_t dims[nDims];
  hsize_t maxDims[nDims];
  H5Sget_simple_extent_dims(dataspace,dims,maxDims);

  if(nDims != 4){
    cout << "Error HDF5 Image data wrongly configured expected dimensions frames - asics - dimX - dimY" << endl;
    cout << "Got "<< nDims << " Dimensions : ";
    for(int i=0; i<nDims; i++){
      cout << dims[i] << " - ";
    }
    cout << endl;
    throw "Invalid dimensions in HDF5 file found";
  }

  if(m_dataFormat == DATAFORMAT::PIXEL){
    numASICs  = (int)dims[0];
    dimY      = (int)dims[1];
    dimX      = (int)dims[2];
    numFrames = (int)dims[3];
  }else if(m_dataFormat == DATAFORMAT::ASIC){
    numFrames = (int)dims[0];
    numASICs  = (int)dims[1];
    dimY      = (int)dims[2];
    dimX      = (int)dims[3];
  }else{
    numFrames = (int)dims[1];
    numASICs  = (int)16;
    dimY      = (int)64;
    dimX      = (int)64;
  }

  if(!imageData){
    imageData = new uint16_t[numFrames*numASICs*dimX*dimY];
  }

  if(!pixelData){
    pixelData = new uint16_t[800*16*dimX*dimY];
  }

  if(m_dataFormat == DATAFORMAT::PIXEL && numFrames == 800 && numASICs == 16){
    // nothing to sort
    H5Dread(dataset_id, H5T_STD_U16LE, H5S_ALL, H5S_ALL, H5P_DEFAULT,pixelData);
  }else{
    // all other cases are catched here in the sort function
    H5Dread(dataset_id, H5T_STD_U16LE, H5S_ALL, H5S_ALL, H5P_DEFAULT,imageData);
    sortData(pixelsToExport);
  }

  H5Sclose(dataspace);
  H5Dclose(dataset_id);

  return pixelData;
}


const uint16_t * DsscHDF5TrainDataReader::loadData() throw (std::string)
{
  if(!loaded){
    cout << "file not loaded, can not read data" << endl;
    return nullptr;
  }

  utils::Timer timer("HDF5TrainDataReader::loadData");

  std::string imageDataLocation = "/INSTRUMENT/DSSC/imageData/data";
  if(isMattiaHDF5Format){
    imageDataLocation = "/INSTRUMENT/DSSC/images/image";
  }

  hid_t dataset_id = H5Dopen2(h5File,imageDataLocation.c_str() , H5P_DEFAULT);
  H5D_space_status_t status;
  H5Dget_space_status(dataset_id,&status);

  hid_t dataspace = H5Dget_space(dataset_id);
  int nDims = H5Sget_simple_extent_ndims(dataspace);
  hsize_t dims[nDims];
  hsize_t maxDims[nDims];
  H5Sget_simple_extent_dims(dataspace,dims,maxDims);

  if(nDims != 4){
    cout << "Error HDF5 Image data wrongly configured expected dimensions frames - asics - dimX - dimY" << endl;
    cout << "Got "<< nDims << " Dimensions : ";
    for(int i=0; i<nDims; i++){
      cout << dims[i] << " - ";
    }
    cout << endl;
    throw "invalid dimensions in HDF5 file found";
  }
  if(m_dataFormat == DATAFORMAT::PIXEL){
    numASICs  = (int)dims[0];
    dimY      = (int)dims[1];
    dimX      = (int)dims[2];
    numFrames = (int)dims[3];
  }else if(m_dataFormat == DATAFORMAT::ASIC){
    numFrames = (int)dims[0];
    numASICs  = (int)dims[1];
    dimY      = (int)dims[2];
    dimX      = (int)dims[3];
  }else{
    numASICs  = (int)16;
    numFrames = (int)dims[1];
    dimY      = (int)64;
    dimX      = (int)64;
  }

  if(!imageData){
    imageData = new uint16_t[numFrames*numASICs*dimX*dimY];
  }

  H5Dread(dataset_id, H5T_STD_U16LE, H5S_ALL, H5S_ALL, H5P_DEFAULT,imageData);

  H5Sclose(dataspace);
  H5Dclose(dataset_id);

  return imageData;
}


const utils::DsscTrainData * DsscHDF5TrainDataReader::getTrainData()
{
  if(!loaded){
    cout << "file not loaded, can not rad data" << endl;
    return nullptr;
  }

  if(!trainData){
    trainData = new utils::DsscTrainData(utils::DsscTrainData::s_trainSize);
  }

  fillTrainData(trainData);

  return trainData;
}


void DsscHDF5TrainDataReader::fillTrainData(utils::DsscTrainData * trainDataToFill)
{
  imageData = trainDataToFill->imageData();
  loadData();

  trainDataToFill->status = 0;
  trainDataToFill->availableASICs = availableASICs;
  trainDataToFill->pulseCnt = getNumFrames();
  trainDataToFill->trainId = getTrainID();

  trainDataToFill->setFormat(m_dataFormat);
  trainDataToFill->asicTrailerData = asicTrailerData;

  readVectorData("INSTRUMENT/DSSC/specificData/pptData",trainDataToFill->pptData,H5T_STD_U16LE);
  readVectorData("INSTRUMENT/DSSC/specificData/sibData",trainDataToFill->sibData,H5T_STD_U16LE);


  readVectorData("INSTRUMENT/DSSC/pulseData/cellId",trainDataToFill->cellIds,H5T_STD_U32LE);
  readVectorData("INSTRUMENT/DSSC/pulseData/pulseId",trainDataToFill->pulseIds,H5T_STD_U32LE);

  readValue("/INSTRUMENT/DSSC/trainData/dataId",trainDataToFill->dataId);
  readValue("/INSTRUMENT/DSSC/trainData/detLinkId",trainDataToFill->detLinkId);
  readValue("/INSTRUMENT/DSSC/trainData/tbLinkId",trainDataToFill->tbLinkId);
  readValue("/INSTRUMENT/DSSC/trainData/detSpecificLength",trainDataToFill->detSpecificLength);
  readValue("/INSTRUMENT/DSSC/trainData/tbSpecificLength",trainDataToFill->tbSpecificLength);

  cout << "TrainDataSorter: read from file TrainId = " << getTrainID() << endl;
}


int DsscHDF5TrainDataReader::getHDF5ASICID(uint32_t asic)
{
  if(isMattiaHDF5Format) return asic;

  auto vecElem = std::find(availableASICs.begin(), availableASICs.end(), asic);
  if(vecElem != availableASICs.end()){
    return vecElem - availableASICs.begin();
  }
  return 0;
}

// in hdf5 not all asics have to be included in the data array, also the upper ASIC row is rotated
// in root the data array expects to contain always 16 asics, and the upper row is not rotated (will maybe changed some day?)
void DsscHDF5TrainDataReader::sortData(const std::vector<int> &pixelsToExport)
{
  if(m_dataFormat == DATAFORMAT::PIXEL){
    utils::DsscTrainDataProcessor::sortPixelToPixelWise(imageData,pixelData,availableASICs,numFrames);
  }
  else
  {
    if(version < 4){
      sortDataPreVer4(pixelsToExport);
    }else{
      utils::DsscTrainDataProcessor::sortAsicToPixelWise(imageData,pixelData,availableASICs,numFrames);
    }
  }
}


void DsscHDF5TrainDataReader::sortDataPreVer4(const std::vector<int> &pixelsToExport)
{
  int numPixels = utils::s_numAsicPixels;

  int numPixelsToExport = pixelsToExport.size();

  std::vector<int> hdf5pixelAsic(numPixelsToExport);
  std::vector<int> hdf5AsicPixel(numPixelsToExport);
  std::vector<int> rootPixelAsic(numPixelsToExport);
  std::vector<int> rootAsicPixel(numPixelsToExport);

  bool rotate = isMattiaHDF5Format || (!isLadderMode() && (version<2));

  int idx=0;
  for(const auto & pixel : pixelsToExport){
    rootAsicPixel[idx] = getAsicPixelNumber(pixel,false,maxASIC); // not rotated Asic pixel number 0-4095
    hdf5AsicPixel[idx] = getAsicPixelNumber(pixel,rotate,maxASIC);// rotated Asic pixel number 0-4095 (rotated only if asic number 8-15)
    rootPixelAsic[idx] = getAsicOfPixel(pixel,false,maxASIC);     // asic number 0-15, upper row not inverted
    hdf5pixelAsic[idx] = getHDF5ASICID(getAsicOfPixel(pixel,isMattiaHDF5Format,maxASIC)); // in hdf5 not all asics have to be included so the index is needed in mattias format (depr.) upper asic row is inverted inverted
    idx++;
  }

  // sorts pixel data in asic wise frame wise format, not image wise format.
  //#pragma omp parallel for
  for(int frame=0; frame<numFrames; frame++){
  #pragma omp parallel for
    for(int idx=0; idx<numPixelsToExport; idx++){
      const int pxDataIdx = rootPixelAsic[idx]*numPixels*800 + rootAsicPixel[idx]*800 + frame;
  #ifdef DEBUG
      pixelData[pxDataIdx] = (frame<10)? frame : pixelsToExport[idx];
  #else
      const int imagePxIdx = frame*numPixels*numASICs + hdf5pixelAsic[idx]*numPixels + hdf5AsicPixel[idx];
      auto value = imageData[imagePxIdx];
      pixelData[pxDataIdx] = value;
  #endif
    }
  }
}


uint16_t DsscHDF5TrainDataReader::getASICTrailerWord(int asic, int wordNum)
{
  constexpr size_t numTrailerBytes = 8*16*2;
  if(asicTrailerData.size() != numTrailerBytes){
    cout << "DsscHDF5TrainDataReader: Trailerdata has wrong size: " << asicTrailerData.size() << "/" << numTrailerBytes << endl;
    return 0;
  }

  int asicDO =(asic>7)? 15-asic + 8 : asic;
  int byteOffs = (wordNum*16 + asicDO)*2;

  const uint8_t *data  = asicTrailerData.data();
  uint16_t value = (*((uint16_t *)&data[byteOffs]));
  return value;
}


// sorts data asic wise, frame wise, not image wise (1-16,4096)
std::vector<uint16_t> DsscHDF5TrainDataReader::fillDataHistoFromTrainDataFile(const std::string & readFileName, utils::DataHistoVec & pixelHistograms, uint16_t asicsToPack)
{
  std::vector<uint16_t> packedASICs;
  std::vector<uint16_t> hdf5AsicIdxVec;

  DsscHDF5TrainDataReader trainReader(readFileName);

  if(!trainReader.isValid()) return packedASICs;

  auto avialableASICs = trainReader.getAvailableASICs();

  int hdf5AsicIdx = 0;
  for(auto && asic : avialableASICs)
  {
    if((asicsToPack & 1<<asic) != 0){
      packedASICs.push_back(asic);
      hdf5AsicIdxVec.push_back(hdf5AsicIdx);
    }
    hdf5AsicIdx++;
  }

  const size_t numAsicsToPack = packedASICs.size();
  const size_t numPixels = trainReader.isLadderMode()? numAsicsToPack*4096 : 4096;
  const size_t frameOffs = avialableASICs.size()*4096;

  if(pixelHistograms.size() != numPixels){
    pixelHistograms.resize(numPixels);
  }

  const int numFrames = trainReader.getNumFrames();

  //hdf5 asic wise image wise format
  const auto imageData = trainReader.loadData();

  for(int frame = 0; frame<numFrames; frame++){

    auto frameData = imageData + frame*frameOffs;

#pragma omp parallel for
    for(size_t asicIdx =0; asicIdx<numAsicsToPack; asicIdx++){
      auto asicFrameData = frameData + hdf5AsicIdxVec[asicIdx]*4096;

      size_t packASICOffs = asicIdx * 4096;

      auto asicPixelHistograms = pixelHistograms.begin() + packASICOffs;
      for(int px=0; px<4096; px++){
        uint16_t value = asicFrameData[px];
        asicPixelHistograms[px].add(value);
      }
    }
  }

  return packedASICs;
}
