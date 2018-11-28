#ifndef THREADSAFEQUEUE_HH
#define THREADSAFEQUEUE_HH

#include <iostream>
#include <queue>
#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <unistd.h>
#include <chrono>

//#define DEBUG


template <class PTR_TYPE>
class ThreadSafeQueue
{
  public:

    ThreadSafeQueue()
     : m_userCnt(0),
       m_frameCnt(0),
       m_done(false)
    {}

    virtual ~ThreadSafeQueue(){
      exit();
    }

    inline bool ready() const {return !m_done && (m_frameCnt == 0);}
    inline uint32_t getFrameCnt() const { return m_frameCnt; }
    bool isDone() const {return m_done;}

    // no further data will be added, if worker sees empty it ends
    inline void finish(){ m_done = true; }

    // all workers will leave the queue
    void exit(){
      m_done = true;
      m_waitCondition.notify_all();
      clear();
    }

    inline void reset(){
      clear();
      m_frameCnt = 0;
      m_done = false;
    }

    size_t size() {
      std::unique_lock<std::mutex> lock(m_mt);
      return m_safeQueue.size();
    }

    bool empty() {
      std::unique_lock<std::mutex> lock(m_mt);
      return m_safeQueue.empty();
    }

    void push_back(PTR_TYPE typePtr)
    {
      {
        std::unique_lock<std::mutex> lock(m_mt);
        m_safeQueue.push(typePtr);
      }
      m_waitCondition.notify_one();
    }


    PTR_TYPE pop_front()
    {
      PTR_TYPE typePtr{nullptr};

      std::unique_lock<std::mutex> lock(m_mt);
      // wait for notify as long as the Queue is empty
      if(m_safeQueue.empty() && !m_done){
        m_waitCondition.wait(lock,[&]{return (!m_safeQueue.empty() || m_done);});
      }

      if(!m_safeQueue.empty()){
        m_frameCnt++;
        typePtr = m_safeQueue.front();
        m_safeQueue.pop();
      }
      return typePtr;
    }


    inline void open(int cnt = 1){
      m_userCnt += cnt;
    }

    // worker closes connection and releases queue
    bool close(){
      m_userCnt--;
      uint32_t newCnt = m_userCnt;
      // last worker is done queue can now be restarted
      if(newCnt == 0){
        m_done = false;
      }
      m_waitCondition.notify_all();
      return (newCnt == 0);
    }

  private:
    inline void clear(){
      std::unique_lock<std::mutex> lock(m_mt);
      while(!m_safeQueue.empty()) m_safeQueue.pop();
    }

  private:
    std::queue<PTR_TYPE> m_safeQueue;
    std::condition_variable m_waitCondition;
    std::atomic<uint32_t> m_userCnt;
    std::atomic<uint32_t> m_frameCnt;
    std::mutex m_mt;
    std::atomic<bool> m_done; // no further items, all should wake up and compute until empty

};

#endif // THREADSAFEQUEUE_HH
