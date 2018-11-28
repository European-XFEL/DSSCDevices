#ifndef SIMPLETHREADSAFEQUEUE_H
#define SIMPLETHREADSAFEQUEUE_H

#include <iostream>
#include <queue>
#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace utils {


template <class PTR_TYPE>
class SimpleThreadSafeQueue : public std::queue<PTR_TYPE>
{
  public:
    SimpleThreadSafeQueue(){}
    ~SimpleThreadSafeQueue(){}

    void push_back(PTR_TYPE ptr){
      std::unique_lock<std::mutex> lock(m_mt);
      this->push(ptr);
    }

    PTR_TYPE pop_front(){
      if(this->empty()) return nullptr;

      std::unique_lock<std::mutex> lock(m_mt);
      PTR_TYPE retVal = this->front();
      this->pop();
      return retVal;
    }

    void clear(){
      std::unique_lock<std::mutex> lock(m_mt);
      while(!this->empty()) this->pop();
    }

  private:
    std::mutex m_mt;
};

}
#endif // SIMPLETHREADSAFEQUEUE_H
