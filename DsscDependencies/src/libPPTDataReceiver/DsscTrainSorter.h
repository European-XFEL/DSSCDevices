#ifndef DSSCTRAINSORTER_H
#define DSSCTRAINSORTER_H

#include <vector>
#include <thread>

#include "DsscPacketReceiverNoSort.h"

class DsscTrainSorter : public DsscPacketReceiverNoSort
{
  public:
    DsscTrainSorter();
    ~DsscTrainSorter();

    using DATAFORMAT = utils::DsscTrainData::DATAFORMAT;

    enum Optype{Recv = 1, Sort = 2, Lost = 3};

    bool isRunning() const {return m_run;}
    void reStartSort();

    void startSortThread();
    void stopSortThread();

    int getFillStand(Optype type);

    void startReadHDF5FilesDirectory(const std::string & inputDirectory);

    inline void setMaxFilesToRead(uint64_t value){m_maxFilesToRead = value;}
    inline void setNextValidTrainId(uint64_t value){m_nextValidTrainId = value;}

    utils::DsscTrainData* getNextTrainData(){
      return m_sortedTrainData.getNextValidStorage();
    }

    void dismissInvalidTrains(uint64_t nextValidTrainId);

    void returnTrainData(utils::DsscTrainData* freeTrainData){
      m_sortedTrainData.addFreeStorage(freeTrainData);
    }


    TrainDataRingStorage m_sortedTrainData;
  protected:

    void sortReceivedData();

    void readHDF5FilesFromDirectory(const std::string & inputDirectory);

  private:

    DATAFORMAT m_outputFormat;

    bool m_run;
    std::thread * m_runThread;

    uint64_t m_maxFilesToRead;
    uint64_t m_nextValidTrainId;

};

#endif // DSSCTRAINSORTER_H
