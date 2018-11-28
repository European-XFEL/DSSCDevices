#ifndef VARIABLEITERATOR_H
#define VARIABLEITERATOR_H

#include "utils.h"

namespace utils {

class ConstVariableIterator
{
  public :
    typedef ConstVariableIterator self_type;
    typedef uint16_t value_type;
    typedef uint16_t& reference;
    typedef const uint16_t& const_reference;
    typedef uint16_t* pointer;
    typedef const uint16_t* const_pointer;
    typedef std::forward_iterator_tag iterator_category;
    typedef int difference_type;

    ConstVariableIterator(const_pointer ptr, size_t numVals, int inc, const std::array<uint,s_totalNumPxs> & sortMap, bool toEnd = false)
      : m_numEntries(numVals),m_pixelCnt(0),
        m_inc(inc),
        m_ptr((pointer)ptr),
        s_sortMap(sortMap),
        m_begin(ptr)
    {
      if(toEnd){
        setToEnd();
      }
    }

    virtual ~ConstVariableIterator(){
      m_ptr = nullptr;
    }

    bool operator==(const self_type & rhs) const { return m_pixelCnt == rhs.m_pixelCnt;}
    bool operator!=(const self_type & rhs) const { return m_pixelCnt != rhs.m_pixelCnt;}

    inline void setToEnd(){ m_pixelCnt=m_numEntries;}

    self_type operator+(int inc) const {
      self_type res = *this;
      res+=inc;
      return res;
    }

    self_type operator-(int dec) const{
      self_type res = *this;
      res-=dec;
      return res;
    }

    self_type& operator+=(int inc){
      increment(inc);
      return *this;
    }

    self_type& operator-=(int inc){
      increment(-inc);
      return *this;
    }

    self_type& operator++(){
      increment();
      return *this;
    }

    self_type operator++(int){
      self_type res = *this;
      increment();
      return res;
    }

    inline const_reference operator*() const { return *m_ptr; }
    inline const_pointer operator->() const {return m_ptr;}
    const_reference operator[](uint offs) const {
      const uint offs_i = std::min(offs,m_numEntries-1);
      return *(m_ptr+(s_sortMap[offs_i]*m_inc));
    }
  protected:

   // inline virtual void increment(int cnt) { m_ptr += (m_inc*cnt);}
   // inline virtual void increment(){ m_ptr += m_inc; }

    void increment() {
      m_pixelCnt++;
      m_ptr = (pointer)m_begin + (s_sortMap[m_pixelCnt]*m_inc);
    }

    void increment(int cnt) {
      m_pixelCnt+=cnt;
      if(m_pixelCnt < 0){
        m_pixelCnt = 0;
        m_ptr = (pointer)m_begin;
      }else if((uint)m_pixelCnt >= m_numEntries){
        m_pixelCnt = m_numEntries;
        m_ptr = nullptr;
      }else{
        m_ptr = (pointer)m_begin + (s_sortMap[m_pixelCnt]*m_inc);
      }
    }

    uint m_numEntries;
    int m_pixelCnt;
    int m_inc;
    pointer m_ptr;
    const std::array<uint,s_totalNumPxs> & s_sortMap;
    const_pointer m_begin;
};

class VariableIterator : public ConstVariableIterator
{
  public :
    typedef VariableIterator self_type;

    VariableIterator(pointer ptr, size_t numVals, int inc, const std::array<uint,s_totalNumPxs> & sortMap, bool toEnd = false)
      : ConstVariableIterator(ptr,numVals,inc,sortMap,toEnd)
    {}

    virtual ~VariableIterator(){
      m_ptr   = nullptr;
      m_begin = nullptr;
    }

    self_type operator+(size_t inc) const {
      self_type res = *this;
      res+=inc;
      return res;
    }

    pointer operator->() {return m_ptr;}
    reference operator*() {return *m_ptr;}

    reference operator[](uint offs) {
      const uint offs_i = std::min(offs,m_numEntries-1);
      return *(m_ptr+(s_sortMap[offs_i]*m_inc));
    }
};

} // utils

#endif // VARIABLEITERATOR_H
