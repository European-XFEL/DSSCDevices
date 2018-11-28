#ifndef TRAINDATARINGSTORAGE_H
#define TRAINDATARINGSTORAGE_H

#include <iostream>
#include <vector>

#include "RingStorage.hh"
#include "DsscTrainData.hh"

class TrainDataRingStorage : public RingStorage<utils::DsscTrainData>
{
  public:
    TrainDataRingStorage(int numTrains)
      : RingStorage(numTrains,utils::DsscTrainData::s_trainSize)
    {
    }

    void addFreeStorage(utils::DsscTrainData * freeData){
      freeData->setInvalid();
      RingStorage::addFreeStorage(freeData);
    }

    void addValidStorage(utils::DsscTrainData * validData){
      RingStorage::addValidStorage(validData);
    }
};

#endif // TRAINDATARINGSTORAGE_H
