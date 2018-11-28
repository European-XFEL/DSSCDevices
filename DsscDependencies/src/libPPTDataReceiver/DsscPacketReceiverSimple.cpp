#include <sys/socket.h>
#include <arpa/inet.h>
#include <omp.h>

#include <unistd.h>
#include <cstring>
#include <iostream>

#include "DsscTrainDataProcessor.h"
#include "DsscPacketReceiverSimple.h"

using namespace std;

bool DsscPacketReceiverSimple::s_ladderMode = true;
std::vector<uint> DsscPacketReceiverSimple::s_sendingAsics = utils::getUpCountingVector(15);
uint DsscPacketReceiverSimple::s_activeAsic = 10;
bool DsscPacketReceiverSimple::s_enableDummyDataChecking = false;

DsscPacketReceiverSimple::DsscPacketReceiverSimple()
 : m_unsortedTrainStorage(utils::DsscTrainData::s_trainSize,0),
   m_sortedTrainStorage(utils::DsscTrainData::s_trainSize,0),
   m_connected(false),
   m_UDPPort(8000),
   m_lastReceivedTrainId(0),
   m_validTrainId(0),
   m_inputFormat("unknown"),
   m_errorCnt(0),
   m_lostCnt(0),
   m_totalCnt(0)
{

  // select 1-8 threads
  unsigned int nthreads = std::max(1u,std::min(8u,std::thread::hardware_concurrency()/2-1));

  omp_set_dynamic(0);            // Explicitly disable dynamic teams
  omp_set_num_threads(nthreads); // Use not more than 8 threads for all consecutive parallel regions
  cout << "will operate with omp " << nthreads << " in parallel" << endl;

  m_unsortedTrainStorage.setInvalid();
  m_unsortedTrainStorage.setFormat(utils::DsscTrainData::DATAFORMAT::IMAGE);

  m_sortedTrainStorage.setInvalid();
  m_sortedTrainStorage.setFormat(utils::DsscTrainData::DATAFORMAT::PIXEL);

  m_unfilledTrainStoragePtr = &m_unsortedTrainStorage;
  m_unsortedTrainStoragePtr = &m_unsortedTrainStorage;
  m_sortedTrainStoragePtr   = &m_sortedTrainStorage;
}


DsscPacketReceiverSimple::~DsscPacketReceiverSimple()
{
  closeConnection();
}


uint32_t DsscPacketReceiverSimple::getFrameID(char * data, size_t length)
{
  return *((uint32_t*)&data[length - 8]);
}

uint16_t DsscPacketReceiverSimple::getPacketID(char * data, size_t length)
{
  return *((uint16_t*)&data[length - 4]);
}

uint64_t DsscPacketReceiverSimple::getTrainID(char * data)
{
  return *((uint64_t*)&data[16]);
}


void DsscPacketReceiverSimple::receiveAndSortData(uint64_t nextValidTrainId)
{
  m_validTrainId = nextValidTrainId;

  if(!m_connected){
    openConnection();
  }

  receivePackets();

  if(m_unsortedTrainStoragePtr->isValid()){
    sortData();
  }else{
    dataInvalid();
  }
}


void DsscPacketReceiverSimple::dataInvalid()
{
  m_sortedTrainStoragePtr->setInvalid();
  cout << "DATAINVALID!!!!!" << endl;
}


void DsscPacketReceiverSimple::sortData()
{
  m_sortedTrainStoragePtr->availableASICs = s_sendingAsics;
  m_sortedTrainStoragePtr->copyMeta(m_unsortedTrainStoragePtr);
  m_sortedTrainStoragePtr->fillMeta();

  utils::DsscTrainDataProcessor::sortDataArray(m_unsortedTrainStoragePtr,m_sortedTrainStoragePtr,utils::DsscTrainData::DATAFORMAT::PIXEL);

  updateAsicTrailerArray();

  // does nothing if testpattern is not 0x1
  if(s_enableDummyDataChecking){
    checkDummyData();
  }

  m_sortedTrainStoragePtr->setValid();
}


void DsscPacketReceiverSimple::receivePackets()
{
  static const std::vector<uint32_t> availableAsics = utils::getUpCountingVector(16);
  static const size_t msg_len = c_pktSize;
  static const size_t ttp_len = 8;

  uint32_t currentFrameID = 0;

  init();

  m_lostCnt = 0;

  bool run = true;

  auto endTime = std::chrono::system_clock::now() + std::chrono::milliseconds(1000);

  while(run)
  {
    char * currentData = ((char *)m_unfilledTrainStoragePtr->data()) + m_currentOffset;
    int length = recv(m_udpSocket,currentData,msg_len,MSG_WAITALL);
    if(length<0){
      //perror("SocketReceive error");
      cout << "+++ Receiver timeout" << endl;
      init();
      if(std::chrono::system_clock::now()>endTime){
        run = false;
      }
    }else{
      if(!m_sof_received){
        // CHECK MAGIC HEADER
        if(m_unfilledTrainStoragePtr->getMagicHeader() == c_magic_header){
          // CHECK TRAINID
          m_lastReceivedTrainId = getTrainID(currentData);
          m_unfilledTrainStoragePtr->trainId = m_lastReceivedTrainId;
          if(m_unfilledTrainStoragePtr->trainId >= m_validTrainId){
            init();
            m_sof_received = true; // HEADER AND TRAIN ID CORRECT
            currentFrameID = getFrameID(currentData,length);
          }else{
            cout << "train discarded: " << m_unfilledTrainStoragePtr->trainId << "/" << m_validTrainId << endl;
          }
        }
      }

      if(m_sof_received)
      {
        m_currentOffset += length - ttp_len;
        const auto & pktID = getPacketID(currentData,length);

        if(m_numTrainPackets != pktID){
          cout << "Missed a packet " << m_numTrainPackets << " - packetID " << pktID << " - missing packets = " << pktID - m_numTrainPackets<< endl;
          m_lostCnt++;
          init();
        }else if((uint64_t)length != c_pktSize){
          //cout << "\rGot " << m_numTrainPackets <<  " packets of train " << currentFrameID << endl;
          run = false;

          m_unfilledTrainStoragePtr->setValid();
          m_unfilledTrainStoragePtr->readFormat();
          m_unfilledTrainStoragePtr->availableASICs = availableAsics;

          m_inputFormat = m_unfilledTrainStoragePtr->getFormatStr();
          m_currentOffset = 0;
        }else{
          const auto & pktFrameID = getFrameID(currentData,length);
          if(pktFrameID != currentFrameID){
            cout << "Packet out of order " << m_numTrainPackets << " - frameID " << pktFrameID << endl;
            init();
          }
        }
        m_numTrainPackets++;
      }
    }
  }
}


int DsscPacketReceiverSimple::openConnection()
{
  int length;
  struct sockaddr_in serverinfo;

  m_udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (m_udpSocket == -1)
  {
    perror("Socket creation error");
    exit(0);
    return 0;
  }

  serverinfo.sin_family = AF_INET;
  serverinfo.sin_addr.s_addr = htonl(INADDR_ANY);
  serverinfo.sin_port = htons(m_UDPPort);
  length = sizeof(serverinfo);

  int yes=1;
  if (setsockopt(m_udpSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
      perror("SetSocktOpt error");
      exit(0);
      return 0;
  }

  while(bind(m_udpSocket, (struct sockaddr *)&serverinfo, length) == -1)
  {
    m_UDPPort++;
    serverinfo.sin_port = htons(m_UDPPort);
    length = sizeof(serverinfo);
    yes=1;
    if (setsockopt(m_udpSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
      perror("SetSocktOpt error");
      exit(0);
      return 0;
    }
  }

  setSocketTimeout(s_SOCKET_TIMEOUT);

  setSocketRecvBufSize(11e7); //104857600Bytes per train

  if(m_UDPPort == 100000){
    cout << "++++ Packet Receive-Socket could not be bound, can not receive data packets" <<  endl;
    m_UDPPort = 8000;
    return 0;
  }

  m_connected = true;
  cout << "++++ Packet Receive Socket Bound at port " << m_UDPPort << endl;
  return 0;
}


void DsscPacketReceiverSimple::setSocketTimeout(uint32_t miliseconds)
{
  struct timeval tv;
  tv.tv_sec = miliseconds/1000;
  tv.tv_usec = (miliseconds%1000)*1000;

  if (setsockopt(m_udpSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv) == -1) {
    perror("SetSocktOpt RCV TIMEOUT error");
    exit(0);
  }
}


void DsscPacketReceiverSimple::setSocketRecvBufSize(uint32_t size)
{
  socklen_t len = 4;
  uint32_t buf;
  getsockopt(m_udpSocket, SOL_SOCKET, SO_RCVBUF, &buf, &len);
  if(buf < size){
    buf = size;
    if (setsockopt(m_udpSocket, SOL_SOCKET, SO_RCVBUF, &buf, sizeof(int)) == -1) {
      fprintf(stderr, "Error setting socket opts: %s\n", strerror(errno));
    }
  }
}


void DsscPacketReceiverSimple::closeConnection()
{
  cout << "++++ Close UDP Socket port "<< m_UDPPort << " ++++" << endl;

  close(m_udpSocket);

  m_connected = false;
}


utils::DsscSpecificData DsscPacketReceiverSimple::getSpecificData()
{
  return m_sortedTrainStoragePtr->getSpecificData();
}


utils::DsscAsicTrailerData& DsscPacketReceiverSimple::getAsicTrailer()
{
  int asic = s_sendingAsics.front();
  return m_trailerData[asic];
}


const utils::DsscAsicTrailerData& DsscPacketReceiverSimple::getAsicTrailer() const
{
  int asic = s_sendingAsics.front();
  return m_trailerData[asic];
}


utils::DsscAsicTrailerData& DsscPacketReceiverSimple::getAsicTrailer(int asic)
{
  return m_trailerData[asic];
}


const utils::DsscAsicTrailerData& DsscPacketReceiverSimple::getAsicTrailer(int asic) const
{
  return m_trailerData[asic];
}


utils::THeader DsscPacketReceiverSimple::getHeaderData()
{
  return m_sortedTrainStoragePtr->getTHeader();
}


void DsscPacketReceiverSimple::updateAsicTrailerArray()
{
  m_trailerData = m_sortedTrainStoragePtr->getDsscAsicTrailerArray();
}


void DsscPacketReceiverSimple::checkDummyData()
{
  const auto asicTrailer = getAsicTrailer();
  if(asicTrailer.m_testPattern == utils::DsscTrainData::c_DR_DUMMY_DATA_TESTPATTERN){
    checkDRDummyData();
  }else if(asicTrailer.m_testPattern == utils::DsscTrainData::c_IOB_DUMMY_DATA_TESTPATTERN){
    checkIOBDummyData();
  }
}


uint16_t DsscPacketReceiverSimple::calcDummyDrExpectedValue(uint32_t asic, uint32_t pixel)
{
  uint16_t pxIdx = (asic>7)? utils::s_numAsicPixels-pixel-1 : pixel;
  uint16_t col = pxIdx%64;
  uint16_t row = pxIdx/64;
  uint16_t expectedValue = col < 32? 33-col: col+2;
  expectedValue = (expectedValue + row*64) % 512;
  return expectedValue;
}


void DsscPacketReceiverSimple::checkDRDummyData()
{
  const auto availableAsics = s_sendingAsics;
  const size_t numAsics = availableAsics.size();
  const size_t numFrames = m_sortedTrainStoragePtr->pulseCnt;

#pragma omp parallel for
  for(size_t asicIdx = 0; asicIdx < numAsics; asicIdx++)
  {
    const auto * asicData = m_sortedTrainStoragePtr->getAsicDataPixelWise(asicIdx);
    for(size_t pxIdx = 0; pxIdx < utils::s_numAsicPixels; pxIdx++)
    {
      const auto expectedValue = calcDummyDrExpectedValue(availableAsics[asicIdx],pxIdx);

      const auto * pixelData = asicData + utils::s_numSram * pxIdx;

      for(size_t frame = 0; frame < numFrames; frame++)
      {
        const uint16_t & data = pixelData[frame];
        if(data != expectedValue)
        {
#pragma omp critical
          {
            if(m_errorCnt<100){
              uint16_t pixel = availableAsics[asicIdx]>7? utils::s_numAsicPixels-pxIdx-1 : pxIdx;
              cout << "Asic " <<availableAsics[asicIdx] << " Pixel "<< setw(6) << pixel << " Frame " << frame << " : Expected = " << setw(3) << expectedValue << " Word = " << data << endl;
            }
            m_errorCnt++;
          }
        }
      }
    }
  }

  size_t numWordsPerTrain = numFrames*numAsics*utils::s_numAsicPixels;
  m_totalCnt += numWordsPerTrain;

  if(m_errorCnt == 0){
    cout << "No Errors after " << m_totalCnt << " checked words (" << m_totalCnt/numWordsPerTrain <<" checked trains)" << endl;
  }else{
    cout << m_errorCnt << " Errors after " << m_totalCnt << " checked words (" << m_totalCnt/numWordsPerTrain <<" checked trains)"  << endl;
  }
}


void DsscPacketReceiverSimple::checkIOBDummyData()
{

}


uint64_t DsscPacketReceiverSimple::getLostFraction() const
{
  if(m_totalCnt == 0) return 0;

  return 100000.0 * m_lostCnt/m_totalCnt;
}


void DsscPacketReceiverSimple::checkDataArray()
{
  const size_t numFrames = m_unfilledTrainStoragePtr->pulseCnt;

  const auto * imageData = m_unfilledTrainStoragePtr->imageData();
#pragma omp parallel for
  for(size_t frame =0; frame < numFrames; frame++){
    const auto * frameData = imageData + frame * utils::s_totalNumPxs;
    for(size_t pixel =0; pixel < utils::s_totalNumPxs; pixel++)
    {
      if(frameData[pixel] >= 512){
#pragma omp critical
        cout << "Frame " << frame << " Pixel " << pixel << " data = " << frameData[pixel] << endl;
      }
    }
  }
}

