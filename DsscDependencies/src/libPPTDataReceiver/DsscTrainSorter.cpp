
#include "DsscTrainSorter.h"
#include "CHIPInterface.h"

#define DSSCTRAINDATA_INIT
#ifdef HAVE_HDF5
#include "DsscHDF5TrainDataReader.h"
#endif
#include "DsscTrainDataProcessor.h"

using namespace std;

DsscTrainSorter::DsscTrainSorter()
  : DsscPacketReceiverNoSort(),
    m_sortedTrainData(3),
    m_outputFormat(DATAFORMAT::PIXEL),
    m_run(false),
    m_runThread(nullptr),
    m_nextValidTrainId(0)
{
  setActiveAsic(12);
  setSendingAsics(0xFFFF);
}

DsscTrainSorter::~DsscTrainSorter()
{
  stopSortThread();
}


void DsscTrainSorter::stopSortThread()
{
  m_run = false;

  m_sortedTrainData.exit();

  if(m_runThread){
    if(m_runThread->joinable()){
      m_runThread->join();
    }
    delete m_runThread;
    m_runThread = nullptr;
  }

  m_sortedTrainData.reset();
  cout << "DsscTrainSorter stopped" << endl;
}


void DsscTrainSorter::reStartSort()
{
  stopSortThread();

  stop();
  start();

  startSortThread();
}


void DsscTrainSorter::startSortThread()
{
  if(!DsscPacketReceiverNoSort::isRunning()){
    DsscPacketReceiverNoSort::start();
  }

  if(m_run) return;

  m_run = true;

  m_runThread = new thread(&DsscTrainSorter::sortReceivedData,this);
}


void DsscTrainSorter::dismissInvalidTrains(uint64_t nextValidTrainId)
{
  uint64_t lastInvalidTrainId = nextValidTrainId-1;
  uint64_t trainId = 0;
  utils::DsscTrainData * trainData;
  do{
    trainData = getNextValidStorage();
    trainId = trainData->trainId;
    addFreeStorage(trainData);
  }while(trainId < lastInvalidTrainId);
}


void DsscTrainSorter::sortReceivedData()
{
  while(m_run){
    receiveAndSortData(m_nextValidTrainId);

    if(m_run){
      if(!m_sortedTrainStoragePtr->isValid()){
        cout << "STOP , got invalid data to sort" << endl;
      }else{
        m_sortedTrainData.addValidStorage(m_sortedTrainStoragePtr);
        m_sortedTrainStoragePtr = m_sortedTrainData.getNextFreeStorage();
      }
    }
  }
  m_sortedTrainData.exit();
  std::cout << "Train Sorter Done" << std::endl;
}


void DsscTrainSorter::startReadHDF5FilesDirectory(const std::string & inputDirectory)
{
  if(m_run) return;

  m_run = true;

  cout << "Data Receiver started reading files from directory" << endl;

  m_runThread = new thread(&DsscTrainSorter::readHDF5FilesFromDirectory,this,inputDirectory);
}



void DsscTrainSorter::readHDF5FilesFromDirectory(const std::string & inputDirectory)
{
#ifdef HAVE_HDF5

  if(!utils::dirExists(inputDirectory)){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "ERROR: Directory does not exist, will stop" << endl;
    return;
  }

  string inDir = inputDirectory;
  if((inDir.end()-1)[0] != '/'){
    inDir +="/";
  }

  string donePath = inDir + "done";
  utils::makePath(donePath);
  donePath += "/";

  unsigned long long fileCnt = 0;
  bool newFileFound = true;

  while(newFileFound && m_run)
  {

    newFileFound = false;

    auto fileNames = utils::readFilesFromDirectory(inputDirectory,"TrainData");

    for(auto && fileName : fileNames){
      newFileFound = true;

      DsscHDF5TrainDataReader reader(inDir+fileName);
      if(reader.isValid()){
        auto freeTrainData = getNextFreeStorageWait();

        if(!m_run) break;

        reader.fillTrainData(freeTrainData);

        addValidStorage(freeTrainData);
      }

      utils::fileMove(inDir + fileName, donePath + fileName);
    }

    fileCnt++;

    if(m_maxFilesToRead > 0){
      if(fileCnt >= m_maxFilesToRead){
        m_run = false;
      }
    }
  }

  m_run = false;

  utils::CoutColorKeeper keeper(utils::STDGREEN);
  cout << "DsscTrainSorter:: All " << fileCnt << " Trains Loaded" << endl;
#endif
}


int DsscTrainSorter::getFillStand(Optype type)
{
  switch(type){
    case Optype::Recv : return fillStand();
    case Optype::Sort : return m_sortedTrainData.fillStand();
    case Optype::Lost : return getLostFraction();
  }
  return 0;
}



