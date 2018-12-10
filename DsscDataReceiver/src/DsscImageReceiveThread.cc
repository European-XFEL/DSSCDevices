/* 
 * File:   DsscImageReceiveThread.cpp
 * Author: kirchgessner
 * 
 * Created on May 17, 2016, 10:51 AM
 */
#include <iostream>

#include "DsscImageReceiveThread.h"
#include "DsscDataSorter.h"

const std::unordered_map<std::string,std::string> DsscImageReceiveThread::directoryStructure {
          {"imageData"        ,"INSTRUMENT.DSSC.imageData.data"},
          {"pCellId"          ,"INSTRUMENT.DSSC.pulseData.cellId"},
          {"pLength"          ,"INSTRUMENT.DSSC.pulseData.length"},
          {"pPulseId"         ,"INSTRUMENT.DSSC.pulseData.pulseId"},
          {"pStatus"          ,"INSTRUMENT.DSSC.pulseData.status"},
          {"ptrainId"         ,"INSTRUMENT.DSSC.pulseData.trainId"},
          {"asicTrailer"      ,"INSTRUMENT.DSSC.specificData.asicTrailer"},
          {"pptData"          ,"INSTRUMENT.DSSC.specificData.pptData"},
          {"sibData"          ,"INSTRUMENT.DSSC.specificData.sibData"},
          {"dataId"           ,"INSTRUMENT.DSSC.trainData.dataId"},
          {"imageCount"       ,"INSTRUMENT.DSSC.trainData.imageCount"},
          {"tbLinkId"         ,"INSTRUMENT.DSSC.trainData.tbLinkId"},
          {"femLinkId"        ,"INSTRUMENT.DSSC.trainData.femLinkId"},
          {"trainId"          ,"INSTRUMENT.DSSC.trainData.trainId"},     
          {"majorTrainFormatVersion","INSTRUMENT.DSSC.trainData.majorTrainFormatVersion"},
          {"minorTrainFormatVersion","INSTRUMENT.DSSC.trainData.minorTrainFormatVersion"},                  
          {"magicNumberStart"   ,"INSTRUMENT.DSSC.trainData.magicNumberStart"},                  
          {"checkSum0"          ,"INSTRUMENT.DSSC.trainData.checkSum0"},
          {"checkSum1"          ,"INSTRUMENT.DSSC.trainData.checkSum1"},
          {"status"             ,"INSTRUMENT.DSSC.trainData.status"},
          {"magicNumberEnd"     ,"INSTRUMENT.DSSC.trainData.magicNumberEnd"},
          {"detSpecificLength"  ,"INSTRUMENT.DSSC.trainData.detSpecificLength"},                  
          {"tbSpecificLength"   ,"INSTRUMENT.DSSC.trainData.tbSpecificLength"}};


DsscImageReceiveThread::DsscImageReceiveThread() 
 : DsscImageReceiveThread(8000,800)
{}

DsscImageReceiveThread::DsscImageReceiveThread(int udpPort, uint32_t numFrames) 
  :  DsscDataReceiveThread(udpPort,numFrames)
{
  setLadderReadout(false);
  resizeImageData();
}

DsscImageReceiveThread::~DsscImageReceiveThread() 
{
}


void DsscImageReceiveThread::resizeImageData()
{
  int numASICs = DsscDataSorter::isLadderReadout()? 16 : 1;
  karabo::util::Dims dims(DsscDataSorter::c_numPixels/64,64,numASICs,DsscDataSorter::mt_numFramesToReceive);
  // Init Karabo Image Data Class  
  karabo::util::NDArray imageArray(dataVector.data(),
                                   dataVector.size(),
                                   karabo::util::NDArray::NullDeleter(),
                                   dims);
  imageData = karabo::xms::ImageData(imageArray);
}


void DsscImageReceiveThread::updateNumFramesToReceive(uint32_t numFrames)
{  
  if(numFrames == DsscDataSorter::mt_numFramesToReceive) return;
  
  DsscDataReceiveThread::updateNumFramesToReceive(numFrames);
  
  resizeImageData();
}  


void DsscImageReceiveThread::packData()
{    
  const auto numFrames = getNumFramesToReceive();
  unsigned long long trainID = getTrainID();
  sendData.set<unsigned long long>(getDir("trainId"), trainID);

//  sendData.set(getDir("pCellId"), packetVector.getCellIds());
//  sendData.set(getDir("pLength"), std::vector<uint32_t>(numFrames,0x200));
  sendData.set(getDir("pPulseId"), packetVector.getPulseIds());
//  sendData.set(getDir("pStatus"), std::vector<uint32_t>(numFrames,0));
//  sendData.set(getDir("ptrainId"), std::vector<uint32_t>(numFrames,trainID));

//  sendData.set(getDir("asicTrailer"), packetVector.getASICTrailerData());
//  sendData.set(getDir("pptData"),     packetVector.getPPTData());
//  sendData.set(getDir("sibData"),     packetVector.getSIBData());

//  fillData(getHeaderData());
//  fillData(getTrailerData()); 
  sendData.set(getDir("imageData"),imageData);
}


THeader DsscImageReceiveThread::getTHeader(const karabo::util::Hash & data) const
{      
  THeader header; 
  header.majorTrainFormatVersion = data.get<unsigned int>(getDir("majorTrainFormatVersion"));
  header.minorTrainFormatVersion = data.get<unsigned int>(getDir("minorTrainFormatVersion"));
  header.tbLinkID          = data.get<unsigned int>(getDir("tbLinkId"));
  header.detLinkID         = data.get<unsigned int>(getDir("femLinkId"));
  header.tbSpecificLength  = data.get<unsigned int>(getDir("tbSpecificLength"));
  header.detSpecificLength = data.get<unsigned int>(getDir("detSpecificLength"));
  header.pulseCnt          = data.get<unsigned long long>(getDir("imageCount"));  
  header.magicNumber       = data.get<unsigned long long>(getDir("magicNumberStart"));
  header.trainID           = data.get<unsigned long long>(getDir("trainId"));
  header.dataID            = data.get<unsigned long long>(getDir("dataId"));   
  return header;
}


void DsscImageReceiveThread::fillData(const THeader & header) 
{
  sendData.set<unsigned long long>(getDir("magicNumberStart"), header.magicNumber); 
  sendData.set<unsigned long long>(getDir("trainId"),          header.trainID);  
  sendData.set<unsigned long long>(getDir("dataId"),           header.dataID);
  sendData.set<unsigned long long>(getDir("imageCount"),       header.pulseCnt);
  sendData.set<unsigned int>(getDir("tbLinkId"),                header.tbLinkID);
  sendData.set<unsigned int>(getDir("femLinkId"),              header.detLinkID);
  sendData.set<unsigned int>(getDir("detSpecificLength"),      header.detSpecificLength);      
  sendData.set<unsigned int>(getDir("tbSpecificLength"),       header.tbSpecificLength);
  sendData.set<unsigned int>(getDir("majorTrainFormatVersion"),header.majorTrainFormatVersion);       
  sendData.set<unsigned int>(getDir("minorTrainFormatVersion"),header.minorTrainFormatVersion);  
}  
    
TTrailer DsscImageReceiveThread::getTTrailer(const karabo::util::Hash & data) const
{
  TTrailer trailer;  
  trailer.checkSum0   = data.get<unsigned long long>(getDir("checkSum0"));
  trailer.checkSum1   = data.get<unsigned long long>(getDir("checkSum1"));
  trailer.status      = data.get<unsigned long long>(getDir("status"));
  trailer.magicNumber = data.get<unsigned long long>(getDir("magicNumberEnd"));  
  return trailer;
} 
    
void DsscImageReceiveThread::fillData(const TTrailer & trailer)
{
  sendData.set<unsigned long long>(getDir("checkSum0"),      trailer.checkSum0);
  sendData.set<unsigned long long>(getDir("checkSum1"),      trailer.checkSum1);
  sendData.set<unsigned long long>(getDir("status"),         trailer.status);
  sendData.set<unsigned long long>(getDir("magicNumberEnd"), trailer.magicNumber);
}

std::string DsscImageReceiveThread::getDir(const std::string & name)
{
  const auto it = directoryStructure.find(name);
  if(it != directoryStructure.cend()){
    return it->second;
  }
  
  std::cout << "unknown parameter name " << name << std::endl;
  exit(0);
  return "";
}


DsscTrainData DsscImageReceiveThread::getTrainData()
{
  size_t pulseCnt = packetVector.getHeaderData().pulseCnt;
  size_t numASICS = isLadderReadout()? 16 : 1;
  size_t totalNumData = pulseCnt*64*64*numASICS+64;
  
  DsscTrainData trainData(DsscDataSorter::mt_dataPointer+64,DsscDataSorter::mt_dataPointer+totalNumData);

  trainData.pulseCnt = packetVector.getHeaderData().pulseCnt;
  trainData.trainId  = packetVector.getHeaderData().trainID;
  trainData.detLinkId= packetVector.getHeaderData().detLinkID;
  trainData.tbLinkId = packetVector.getHeaderData().tbLinkID;
  trainData.dataId   = packetVector.getHeaderData().dataID;
  
  trainData.detSpecificLength = packetVector.getHeaderData().detSpecificLength;
  trainData.tbSpecificLength  = packetVector.getHeaderData().tbSpecificLength;
  trainData.status   = 0;

  trainData.cellIds         = packetVector.getCellIds();
  trainData.pulseIds        = packetVector.getPulseIds();
  trainData.pptData         = packetVector.getPPTData();
  trainData.sibData         = packetVector.getSIBData();
 // trainData.asicTrailerData = packetVector.getASICTrailerData();

  trainData.availableASICs.assign(numASICS,0);
  for(size_t asic = 0; asic < numASICS; asic++){
    trainData.availableASICs[asic] = asic;
  }
  
  return trainData;
}
