#include <iostream>

#include "DsscTrainSorter.h"
#include "DsscHDF5Writer.h"

#include "utils.h"

#include "DsscHDF5Receiver.h"

using namespace SuS;
using namespace std;

DsscHDF5Receiver::DsscHDF5Receiver()
  : pixelWise(false),
    selfStart(false),
    run_thread(nullptr),
    m_trainSorter(nullptr)
{
}

DsscHDF5Receiver::DsscHDF5Receiver(const std::string &currentDir)
  : currentDir(currentDir),
    pixelWise(false),
    selfStart(false),
    run_thread(nullptr),
    m_trainSorter(nullptr)
{
}

DsscHDF5Receiver::~DsscHDF5Receiver()
{
  stop();
}

void DsscHDF5Receiver::tidyUp()
{
  if(selfStart){
    if(m_trainSorter){
      delete m_trainSorter;
    }
  }
  selfStart = false;
}


void DsscHDF5Receiver::start(DsscTrainSorter * trainSorter)
{
  if(run_thread) return;
  acquire = true;
  m_trainSorter = trainSorter;
  run_thread = new std::thread(&DsscHDF5Receiver::run,this);
}


void DsscHDF5Receiver::start()
{
  if(run_thread) return;

  selfStart    = true;
  acquire      = true;
  m_trainSorter = new DsscTrainSorter();
  m_trainSorter->startSortThread();

  run_thread   = new std::thread(&DsscHDF5Receiver::run,this);
}


void DsscHDF5Receiver::stop()
{
  acquire = false;
  if(run_thread){
    if(run_thread->joinable()){
      run_thread->join();
    }
    delete run_thread;
  }
  run_thread = nullptr;
}


void DsscHDF5Receiver::run()
{
  if(selfStart){
    m_trainSorter->startSortThread();
  }

  while(acquire)
  {
    auto trainData = m_trainSorter->getNextTrainData();

    const string timeStr    = utils::getLocalTimeStr();
    const string trainIDStr = to_string(trainData->trainId);
    const string fileName   = timeStr + "_trainId_" + trainIDStr + ".h5";
    const string filePath   = currentDir+"/"+fileName;

    DsscHDF5Writer::saveToFile(filePath,trainData,64,64);

    m_trainSorter->returnTrainData(trainData);
  }

  tidyUp();
}

