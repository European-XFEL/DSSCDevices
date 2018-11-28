#include <sys/socket.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <cstring>

#include "utils.h"

#include "DsscPacketReceiverNoSort.h"

#define PSIZE 8192
using namespace std;


DsscPacketReceiverNoSort::DsscPacketReceiverNoSort()
  : DsscPacketReceiverSimple(),
    TrainDataRingStorage(10),
    m_run(false),
    m_runThread(nullptr)
{
}

DsscPacketReceiverNoSort::~DsscPacketReceiverNoSort()
{
  stop();
}

void DsscPacketReceiverNoSort::start()
{
  if(m_run){
    cout << "++++ Already running will not start new thread" << endl;
    return;
  }

  if(!m_connected){
    openConnection();
  }

  m_run = true;

  reset();
  m_unfilledTrainStoragePtr = &m_unsortedTrainStorage;
  m_unfilledTrainStoragePtr->setInvalid();
  cout << "Simple receiver receive thread started" << endl;

  m_runThread = new thread(&DsscPacketReceiverNoSort::receiveContinuously,this);
}


void DsscPacketReceiverNoSort::receiveContinuously()
{
  while(m_run)
  {
    receivePackets();
    if(m_run && m_unfilledTrainStoragePtr->isValid()){
      addValidStorage(m_unfilledTrainStoragePtr);
      m_unfilledTrainStoragePtr = getNextFreeStorageWait();
      if(m_unfilledTrainStoragePtr){
        m_unfilledTrainStoragePtr->setInvalid();
      }else{
        m_run = false;
      }
    }
  }
  exit();
}


void DsscPacketReceiverNoSort::stop()
{
  m_run=false;

  exit();

  if(m_runThread){
    if(m_runThread->joinable()){
      m_runThread->join();
    }
    delete m_runThread;
    m_runThread = nullptr;
  }

  reset();

  cout << "++++ RunThread stopped " << endl;
}


utils::DsscTrainData * DsscPacketReceiverNoSort::getNextTrainData()
{
  const auto timeout = std::chrono::system_clock::now() + std::chrono::milliseconds(1000);

  bool wait = true;
  while(wait)
  {
    auto * nextTrainData = getNextValidStorage();
    // when queue exit was triggered null pointer is returned
    if(nextTrainData == nullptr)
    {
      wait = false;
      auto * nextInvalidData = getNextFreeStorage();
      nextInvalidData->setInvalid();
      return nextInvalidData;
    }
    else
    {
      // data is valid data can be checked if train Id is in expected range
      if(nextTrainData->trainId < m_validTrainId){
        nextTrainData->setInvalid();
        addFreeStorage(nextTrainData);
      }else{
        if(!nextTrainData->isValid()){
          cout << "STOP GOT Invalid from valid array" << endl;
        }
        return nextTrainData;
      }
    }
    if(timeout < std::chrono::system_clock::now()){
      wait = false;
    }
  }

  auto * nextInvalidData = getNextFreeStorage();
  nextInvalidData->setInvalid();
  return nextInvalidData;
}


utils::DsscTrainData * DsscPacketReceiverNoSort::receiveAndSortData(uint64_t nextValidTrainId)
{
  setNextValidTrainId(nextValidTrainId);

  if(!m_connected){
    openConnection();
  }

  m_unsortedTrainStoragePtr = getNextTrainData();

  if(m_unsortedTrainStoragePtr->isValid()){
    sortData();
  }else{
    dataInvalid();
  }

  m_unsortedTrainStoragePtr->setInvalid();
  addFreeStorage(m_unsortedTrainStoragePtr);

  return m_sortedTrainStoragePtr;
}
