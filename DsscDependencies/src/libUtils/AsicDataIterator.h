#ifndef ASICDATAITERATOR_H
#define ASICDATAITERATOR_H

#include "VariableIterator.h"

namespace utils{

class AsicDataIterator : public VariableIterator
{
  public:

    AsicDataIterator(pointer ptr, size_t numVals, int inc)
      : VariableIterator(ptr,numVals,inc)
    {}

  private:
    void increment(int cnt = 1) override {
      VariableIterator::increment(cnt);
      size_t dist = std::distance(m_begin,m_ptr);
      if((dist%512) > 63){
        size_t addRows = dist/64;
        size_t addCols = dist%64;
        m_ptr += addRows*512;
        m_ptr  += addCols;
        if(m_ptr>m_end){
          m_ptr = m_end;
        }
      }
    }
};

}

#endif // ASICDATAITERATOR_H
