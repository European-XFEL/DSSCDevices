#ifndef DSSCHDF5RECEIVETHREAD_HPP
#define DSSCHDF5RECEIVETHREAD_HPP
#include <mutex>
#include <vector>
#include <thread>
#include <string>

#include "DsscDataReceiveThread.h"
#include "DsscHDF5Writer.h"

#ifndef NO_SORT
class DsscHDF5ReceiveThread : public DsscDataReceiveThread
{
  public:

    static uint16_t m_numASICs;

    DsscHDF5ReceiveThread(int udpPort, uint32_t numFrames, uint32_t numTrains);
    virtual ~DsscHDF5ReceiveThread();

    void packData();

    // warning does not check if directory exists
    inline void setOutputDir(const std::string & outDir){m_outputDir = outDir;}

    utils::DsscTrainData getTrainData();

  private:
    std::mutex m_closeMutex;

    void closeSaveThreads();
    void setNextSortPointer();

    uint32_t m_numTrains;
    uint32_t m_currTrain;
    std::vector<uint32_t> availableASICs;
    std::vector<std::vector<uint16_t>> m_trainsToStore;
    std::vector<std::thread> m_write_hdf5_threads;
    std::string m_outputDir;
};

#endif

#endif // DSSCHDF5RECEIVETHREAD_HPP
