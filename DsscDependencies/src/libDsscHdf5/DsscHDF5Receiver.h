#ifndef DSSCHDF5RECEIVER_H
#define DSSCHDF5RECEIVER_H

#include <string>
#include <thread>

#include "DsscTrainSorter.h"

namespace SuS {



class DsscHDF5Receiver
{
  public:
    DsscHDF5Receiver();
    DsscHDF5Receiver(const std::string &currentDir);
    ~DsscHDF5Receiver();

    void setOutputDirectory(const std::string & dirName){
      currentDir = dirName;
    }

    void start(DsscTrainSorter * sorter);
    void start();
    void stop();

  private:

    void run();
    void tidyUp();

    std::string currentDir;
    bool pixelWise;

    bool acquire;
    bool selfStart;
    std::thread *run_thread;

    DsscTrainSorter * m_trainSorter;
};

}

#endif // DSSCHDF5RECEIVER_H
