#ifndef RINGSTORAGE_HH
#define RINGSTORAGE_HH

#include "ThreadSafeQueue.hh"

template <class CONTAINER_TYPE>
class RingStorage
{
  public:
    RingStorage(): containerSize(0) {}
    RingStorage(size_t size, size_t containerSize)
     : containerSize(containerSize)
    {
      for(unsigned i=0;i<size; i++){
        storage.emplace_back(CONTAINER_TYPE(containerSize));
      }
      reset();
    }

    ~RingStorage(){
      exit();
    }

    size_t fillStand(){
      return validQueue.size();
    }


    void reset() {
      freeQueue.reset();
      validQueue.reset();
      for(unsigned i=0;i<storage.size(); i++){
        freeQueue.push_back(&(storage[i]));
      }
    }

    CONTAINER_TYPE * getNextFreeStorage(){
      if(freeQueue.empty()){
        if(validQueue.empty()){
          std::cout << "Wait for data application to return storage" << std::endl;
        }else{
          dismissValidTrain();
        }
      }
      return freeQueue.pop_front();
    }

    CONTAINER_TYPE * getNextFreeStorageWait(){
      return freeQueue.pop_front();
    }

    CONTAINER_TYPE * getNextValidStorage(){
      return validQueue.pop_front();
    }

    void addValidStorage(CONTAINER_TYPE* validTrain){
      validQueue.push_back(validTrain);
    }

    void addFreeStorage(CONTAINER_TYPE* freeTrain){
      freeQueue.push_back(freeTrain);
    }

    void dismissValidTrain(){
      addFreeStorage(getNextValidStorage());
      //std::cout << typeid(this).name() << " - Warning: Valid train dismissed" << std::endl;
    }

    void resizeContainer(size_t newSize){
      if(containerSize == newSize) return;

      containerSize = newSize;
      for(auto && item : storage){
        item.resize(containerSize);
      }
      reset();
    }

    void addStorage(int num){
      for(unsigned i=0;i<num; i++){
        storage.emplace_back(CONTAINER_TYPE(containerSize));
        addFreeStorage(&storage.back());
      }
    }

    void exit(){
      freeQueue.exit();
      validQueue.exit();
    }

  protected:

    std::vector<CONTAINER_TYPE> storage;
    ThreadSafeQueue<CONTAINER_TYPE*> validQueue;
    ThreadSafeQueue<CONTAINER_TYPE*> freeQueue;
    uint64_t containerSize;
};

#endif // RINGSTORAGE_HH
