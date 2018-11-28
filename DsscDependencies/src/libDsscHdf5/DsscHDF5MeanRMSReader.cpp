#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>

#include "DsscHDF5MeanRMSReader.h"
#include "utils.h"

using namespace std;

// #define DEBUG

uint16_t * DsscHDF5MeanRMSReader::pixelData = nullptr;
uint64_t DsscHDF5MeanRMSReader::trainID = 0;
DsscHDF5MeanRMSReader::DsscHDF5MeanRMSReader()
  : DsscHDF5Reader(),
    dimX(64),
    dimY(64)
{
}


DsscHDF5MeanRMSReader::DsscHDF5MeanRMSReader(const string & readFileName)
  : DsscHDF5Reader(readFileName),
    dimX(64),
    dimY(64),
    maxASIC(0)
{
  checkModuleInfo();

  readAcquisitionTime();

  readNumASICs();
  readNumTrains();
  readNumValues();
  readAvailableASICs();

  readMeanValues();

}


DsscHDF5MeanRMSReader::~DsscHDF5MeanRMSReader()
{
}


void DsscHDF5MeanRMSReader::readNumASICs()
{
  readValue("INSTRUMENT/DSSC/trainStats/numASICs",m_numASICs);
}

void DsscHDF5MeanRMSReader::readNumTrains()
{
  readValue("INSTRUMENT/DSSC/trainStats/numTrains",m_numTrains);
  trainID += m_numTrains;
}

void DsscHDF5MeanRMSReader::readNumValues()
{
  readValue("INSTRUMENT/DSSC/trainStats/numValues",m_numValues);
}

void DsscHDF5MeanRMSReader::readAvailableASICs()
{
  readVectorData("INSTRUMENT/DSSC/trainStats/acitveASICs",availableASICs,H5T_STD_U32LE);
  maxASIC = *std::max_element(availableASICs.begin(),availableASICs.end());
}


void DsscHDF5MeanRMSReader::readMeanValues()
{
  readVectorData("INSTRUMENT/DSSC/trainStats/meanValues",meanValues,H5T_IEEE_F32LE);
}


void DsscHDF5MeanRMSReader::readAcquisitionTime()
{
  struct stat t_stat;
  stat(fileName.c_str(), &t_stat);
  struct tm * timeinfo = localtime(&t_stat.st_ctime);
  string timeStr(asctime(timeinfo));
  m_acTime = timeStr;
}


const uint16_t * DsscHDF5MeanRMSReader::getData(const std::vector<int> & pixelsToExport)
{
  if(!loaded){
    cout << "file not loaded, can not rad data" << endl;
    return nullptr;
  }

  if(!pixelData){
    pixelData = new uint16_t[800*16*dimX*dimY];
  }

  sortData(pixelsToExport);

  return pixelData;
}


int DsscHDF5MeanRMSReader::getHDF5ASICID(uint32_t asic)
{
  auto vecElem = std::find(availableASICs.begin(), availableASICs.end(), asic);
  if(vecElem != availableASICs.end()){
    return vecElem - availableASICs.begin();
  }
  return 0;
}



void DsscHDF5MeanRMSReader::sortData(const std::vector<int> &pixelsToExport)
{
  int numPixels = dimX*dimY;

  int numPixelsToExport = pixelsToExport.size();

  std::vector<int> hdf5pixelAsic(numPixelsToExport);
  std::vector<int> rootPixelAsic(numPixelsToExport);
  std::vector<int> rootAsicPixel(numPixelsToExport);

  int idx=0;
  for(const auto & pixel : pixelsToExport){
    rootAsicPixel[idx] = utils::getAsicPixelNumber(pixel,false,maxASIC);
    rootPixelAsic[idx] = utils::getAsicOfPixel(pixel,false,maxASIC);
    hdf5pixelAsic[idx] = getHDF5ASICID(utils::getAsicOfPixel(pixel,false,maxASIC));
    idx++;
  }

//#pragma omp parallel for
  for(int idx=0; idx<numPixelsToExport; idx++)
  {
    const int pxDataIdx = rootPixelAsic[idx]*numPixels*800 + rootAsicPixel[idx]*800;
#ifdef DEBUG
    for(int i = 0; i<800; i++){
      pixelData[pxDataIdx+i] = (i<10)? frame : pixelsToExport[idx];
    }
#else

    const int imagePxIdx = hdf5pixelAsic[idx]*numPixels + rootAsicPixel[idx];
    float value = meanValues[imagePxIdx];
    int valueDwn = floor(value);
    int valueUp  = ceil(value);
    float value100 = (value * 100) - (valueDwn * 100);
    int numUpFrames = round(value100/100.0 * 800.0);

    for(int i = 0; i<numUpFrames; i++){
      pixelData[pxDataIdx+i] = valueUp;
    }
    for(int i=numUpFrames; i<800; i++){
      pixelData[pxDataIdx+i] = valueDwn;
    }
#endif
  }
}



