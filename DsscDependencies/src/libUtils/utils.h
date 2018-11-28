#ifndef UTILS_H
#define UTILS_H

#pragma once

#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <typeinfo>
#include <utility>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <iterator>
#include <limits>
#include <type_traits>
#include <array>
#include <unordered_map>
#include <sstream>
#include <istream>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <regex>
#include <bitset>
#include <iostream>
#include <iomanip>
#include <random>
#include <map>

#include "SimpleThreadSafeQueue.h"

namespace utils
{
  static const std::string STDRED       = "\x1b[31m";
  static const std::string STDGREEN     = "\x1b[32m";
  static const std::string STDBROWN     = "\x1b[33m";
  static const std::string STDBLUE      = "\x1b[34m";
  static const std::string STDMAGENTA   = "\x1b[35m";
  static const std::string STDCYAN      = "\x1b[36m";
  static const std::string STDGRAY      = "\x1b[37m";

  static const std::string STDNORM      = "\x1b[0m";

  static const std::string STDBOLD      = "\x1b[1m";
  static const std::string STDITALIC    = "\x1b[2m";
  static const std::string STDUNDERLINE = "\x1b[4m";
  static const std::string STDBLINK     = "\x1b[5m";
  static const std::string STDREVERSE   = "\x1b[7m";

  static constexpr size_t s_numAsicPixels = 4096;
  static constexpr size_t s_totalNumPxs   = 65536;
  static constexpr size_t s_numSram       = 800;
  static constexpr size_t s_numAsics      = 16;
  static constexpr size_t s_numAsicData   = 4096*800;
  static constexpr size_t s_totalNumWords = 65536*800;

  using PixelVector = std::vector<uint32_t>;

  std::array<uint,s_totalNumPxs> genImagePixelMap();
  std::array<uint,s_totalNumPxs> genPixelAsicMap();
  std::array<uint,s_totalNumPxs> genAsicPixelMap();
  std::array<uint,s_totalNumPxs> genAsicImageMap();
  std::array<uint,s_totalNumPxs> genDataPixelMap();
  std::array<uint,s_totalNumPxs> getUpCountingMap();

  static const std::array<uint,s_totalNumPxs> s_imagePixelMap = genImagePixelMap(); // find image pixel of asic*numAsicPixels+pixel
  static const std::array<uint,s_totalNumPxs> s_pixelAsicMap  = genPixelAsicMap();  // find asic of image pixel
  static const std::array<uint,s_totalNumPxs> s_asicPixelMap  = genAsicPixelMap();  // find asicpixel of image pixel
  static const std::array<uint,s_totalNumPxs> s_asicImageMap  = genAsicImageMap();  // find image pixel of asicpixel
  static const std::array<uint,s_totalNumPxs> s_dataPixelMap  = genDataPixelMap();  // for asic and pixel wise sorted data - opposite direction of imagePixelMap
  static const std::array<uint,s_totalNumPxs> s_linearMap     = getUpCountingMap();

  class CoutColorKeeper{
    public:
    CoutColorKeeper(const std::string & col){std::cout << col;}
    void change(const std::string & col){std::cout << col;}
    ~CoutColorKeeper(){std::cout << STDNORM;}
  };
  // templated function to produce well formatted float string with defined number of cyphers
  std::string setPrecision(double x, int num);

  //#####################################################################################################
  // Stats and statsAcc(umulators) can be used to calculate mean and rms values for large number of bursts
  // accumulators can hold accumulated values over all sram addresses of large amount of bursts
  // much more efficient than store all single sram values of multiple bursts just to compute
  // mean and rms
  struct Stats{
    void print() const { std::cout<<"Mean = " <<mean<<" +- "<<rms<<std::endl; }
    double mean;
    double rms;
  };

  struct SlopeMinMax{
      double slope;
      double yMin;
      double yMax;
  };

  struct StatsAcc
  {
      StatsAcc() : numValues(0),xAcc(0),xxAcc(0){}
      StatsAcc(uint64_t numVals,uint64_t xA,uint64_t xxA) : numValues(numVals),xAcc(xA),xxAcc(xxA){}

      void clear() {xAcc = xxAcc = numValues = 0;}

      template<typename T>
      inline void addValue(T value){ xAcc += value; xxAcc += pow(value,2); numValues++;}

      StatsAcc operator+(const StatsAcc & acc){
        StatsAcc newAcc (numValues + acc.numValues,
                         xAcc      + acc.xAcc,
                         xxAcc     + acc.xxAcc);
        return newAcc;
      }

      void operator+=(const StatsAcc & acc){ numValues += acc.numValues; xAcc += acc.xAcc; xxAcc += acc.xxAcc;}
      inline void operator+=(const std::vector<StatsAcc> & accVec){ for(auto && statsAcc : accVec) operator+=(statsAcc); }

      template<typename T>
      inline void operator+=(T value){ xAcc += value; xxAcc += pow(value,2); numValues++;}

      template<typename ITER>
      inline void addValues(ITER begin, ITER end){
        xAcc  += std::accumulate(begin,end,0ul);
        xxAcc += std::inner_product(begin, end, begin,0ul);
        numValues += std::distance(begin,end);
      }

      template <typename ITER>
      inline void addValues(ITER begin, ITER end, std::vector<bool>::const_iterator boolIt){
        for (ITER it = begin; it != end; it++) {
          if(*boolIt){
            const auto value = *it;
            xAcc  += value;
            xxAcc += pow(value,2);
            numValues++;
          }
          boolIt++;
        }
      }

      Stats calcStats() const {
        double mean  = (double)xAcc/numValues;
        double rms   = sqrt(((double)xxAcc-mean*mean*numValues)/(numValues-1));
        return {mean,rms};
      }

      uint64_t numValues;
      uint64_t xAcc;
      uint64_t xxAcc;
  };

  using StatsAccVec = std::vector<StatsAcc>;
  using StatsVec = std::vector<Stats>;

  StatsVec getMeandAndRMSVector(const StatsAccVec & statsAccumulators);

  template<typename T = uint32_t>
  Stats getMeandAndRMS(const std::vector<T> & values)
  {
    double cnt = values.size();

    if(cnt==0) return {0.0,0.0};
    if(cnt==1) return {(double)values.front(),0.0};

    // if start value type has small limit increase to uint32_t
    double mean = ( (std::numeric_limits<T>::max()<=std::numeric_limits<uint16_t>::max())?
                      std::accumulate(values.begin(), values.end(), (uint32_t)0)/cnt :
                      std::accumulate(values.begin(), values.end(), (T)0)/cnt );

    double rms  = ( (std::numeric_limits<T>::max()<=std::numeric_limits<uint16_t>::max())?
                    std::inner_product(values.begin(), values.end(), values.begin(), (uint32_t)0) :
                    std::inner_product(values.begin(), values.end(), values.begin(), (T)0) );
    rms = sqrt((rms - mean*mean*cnt)/(cnt-1));
    return {mean,rms};
  }

  template<typename T = uint32_t>
  Stats getMeandAndRMS(T *values, size_t cnt)
  {
    if(cnt==0) return {0.0,0.0};
    if(cnt==1) return {(double)values[0],0.0};

    // if start value type has small limit increase to uint32_t
    double mean = ( (std::numeric_limits<T>::max()<=std::numeric_limits<uint16_t>::max())?
                      std::accumulate(values, values+cnt, (uint32_t)0)/cnt :
                      std::accumulate(values, values+cnt, (T)0)/cnt );

    double rms  = ( (std::numeric_limits<T>::max()<=std::numeric_limits<uint16_t>::max())?
                    std::inner_product(values, values+cnt, values, (uint32_t)0) :
                    std::inner_product(values, values+cnt, values, (T)0) );

    rms = sqrt((rms - mean*mean*cnt)/(cnt-1));
    return {mean,rms};
  }

  template<typename T>
  double calcPerCent(T rel, T mean){
    return (100.0 * (double)rel/(double)mean);
  }

  template<typename TV, typename TM>
  double calcDiffPerCent(TV value, TM mean){
    return calcPerCent(value-mean,mean);
  }

  template<typename T>
  double calcSum(const std::vector<T> & input)
  {
    return std::accumulate(input.begin(),input.end(), (double)0.0);
  }

  std::vector<double> removeOutliersFromVector(const std::vector<double> & inputValues, double rmsSigma);
  std::vector<double> removeOutliersFromVectorIt(const std::vector<double> & inputValues, double rmsSigma);
  std::vector<double>& limitOutliersOfVector(std::vector<double> & inputValues, double rmsSigma);

  template<typename T = uint32_t>
  std::string getVectorStatsStr(const std::vector<T> & values)
  {
    if(values.empty()) return "error no values";

    auto minMaxIt = std::minmax_element(values.begin(), values.end());
    T min = *minMaxIt.first;
    T max = *minMaxIt.second;

    const auto stats = getMeandAndRMS(values);
    std::stringstream statuss;
    statuss << values.size() << " Values in Range = " << std::setw(4) << calcDiffPerCent(min,stats.mean) << "% - " << std::setw(4) << calcDiffPerCent(max,stats.mean) << "% ( Mean = ";
    statuss << stats.mean << " +- " <<  setPrecision(calcPerCent(stats.rms,stats.mean),2)<< "% )";
    return statuss.str();
  }

  template<typename T = uint32_t>
  std::string getMinMaxStatsStr(const std::vector<T> & values)
  {
    auto minMaxIt = std::minmax_element(values.begin(), values.end());
    auto minVHold = *minMaxIt.first;
    auto maxVHold = *minMaxIt.second;
    auto stats = getMeandAndRMS<T>(values);
    std::stringstream statuss;
    statuss << minVHold << " - " << maxVHold << "(";
    statuss << stats.mean << " +- " << stats.rms << ")";
    return statuss.str();
  }


  struct MeanRMSVectors{
      std::vector<float> meanValues;
      std::vector<float> rmsValues;
  };


  //#####################################################################################################


  template <typename T1, typename T2>
  void printPair(const std::pair<T1,T2> & value, std::ofstream & out)
  {
    out << "(" << value.first << "/" << std::fixed << value.second << ");";
  }


  template <typename T1, typename T2>
  std::pair<T1,T2> readPair(const std::string & str)
  {
    char a;
    double bin,dnl;
    std::istringstream is(str);
    is >> a; is >> bin;
    is >> a; is >> dnl;
    return {bin,dnl};
  }


  uint16_t convertGCC(uint16_t gray);

  template <typename T_IN, typename T_OUT>
  std::vector<T_OUT> convertVectorType(const std::vector<T_IN> & input)
  {
    const size_t numValues = input.size();

    std::vector<T_OUT> output(numValues);

#pragma omp parallel for
    for(size_t idx = 0; idx < numValues; idx++){
      output[idx] = input[idx];
    }

    return output;
  }

  double convertToUINT16(std::vector<uint16_t> & output, const std::vector<double> & input);

  int getLocalTimeID();
  std::string getLocalTimeStr();
  std::string getHighPrecisionTimeStr();

  bool checkFileExists(const std::string & fileName);
  std::string getBaseFileName(const std::string & fileName);
  std::string getFileName(const std::string & fileName);
  std::string getFilePath(const std::string & fileName);
  std::string getFileExtension(const std::string & fileName);
  std::string &changeFileExtension(std::string & fileName, const std::string & newExt);
  std::string changeFileExtension(const std::string & fileName, const std::string & newExt);

  void fileCopy(const std::string & srcfileName, const std::string & dstFileName);
  void fileMove(const std::string & srcfileName, const std::string & dstFileName);
  void fileDelete(const std::string & srcfileName);
  void fileBackup(const std::string & srcfileName);

  std::string & stringReplace(std::string & input, char find, char repl);
  std::string stringReplace(const std::string &input, char find, char repl);
  std::string & stringReplace(std::string &input, const std::string & find, const std::string & repl);
  std::string stringReplace(const std::string &input, const std::string & find, const std::string & repl);

  std::string join(const std::vector<std::string> & positions, std::string delim);

  template <typename T>
  void join(std::ostream & out, const std::vector<T> & values, std::string delim)
  {
    for(auto && value : values){
      out << value << delim;
    }
  }

  template <typename T>
  std::string joinToString(const std::vector<T> & values, std::string delim)
  {
    std::stringstream ss;
    join(ss,values,delim);
    return ss.str();
  }


  void replace(std::string & input, const std::string & before, const std::string & after);

  //split from stream
  std::vector<int>         &split(std::istream &in, char delim, std::vector<int> &elems, int start = 0);
  std::vector<uint16_t>    &split(std::istream &in, char delim, std::vector<uint16_t> &elems, int start = 0);
  std::vector<uint32_t>    &split(std::istream &in, char delim, std::vector<uint32_t> &elems, int start = 0);
  std::vector<uint64_t>    &split(std::istream &in, char delim, std::vector<uint64_t> &elems, int start = 0);
  std::vector<double>      &split(std::istream &in, char delim, std::vector<double> &elems, int start = 0);
  std::vector<std::string> &split(std::istream &in, char delim, std::vector<std::string> &elems, int start = 0);

  //split from string calls split from stream internally
  std::vector<int>      &split(const std::string &s, char delim, std::vector<int> &elems, int start = 0);
  std::vector<uint16_t> &split(const std::string &s, char delim, std::vector<uint16_t> &elems, int start = 0);
  std::vector<uint32_t> &split(const std::string &s, char delim, std::vector<uint32_t> &elems, int start = 0);
  std::vector<uint64_t> &split(const std::string &s, char delim, std::vector<uint64_t> &elems, int start = 0);
  std::vector<double>   &split(const std::string &s, char delim, std::vector<double> &elems, int start = 0);
  std::vector<uint32_t> split(const std::string &s, char delim, int start = 0);

  std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems, int start = 0);

  template <typename T>
  T getStringElement(const std::string & in, char delim, int numElement){
    std::string item;
    int cnt =0;

    std::stringstream inStream(in);
    while (cnt < numElement){ std::getline(inStream, item, delim); cnt++;}

    T value;
    std::getline(inStream, item, delim);
    std::istringstream iss(item);
    iss >> value;
    return value;
  }

  template<typename T>
  struct has_const_iterator
  {
  private:
      typedef char                      yes;
      typedef struct { char array[2]; } no;

      template<typename C> static yes test(typename C::const_iterator*);
      template<typename C> static no  test(...);
  public:
      static const bool value = sizeof(test<T>(0)) == sizeof(yes);
      typedef T type;
  };

  template <typename T>
  struct has_begin_end
  {
      template<typename C> static char (&f(typename std::enable_if<
        std::is_same<decltype(static_cast<typename C::const_iterator (C::*)() const>(&C::begin)),
        typename C::const_iterator(C::*)() const>::value, void>::type*))[1];

      template<typename C> static char (&f(...))[2];

      template<typename C> static char (&g(typename std::enable_if<
        std::is_same<decltype(static_cast<typename C::const_iterator (C::*)() const>(&C::end)),
        typename C::const_iterator(C::*)() const>::value, void>::type*))[1];

      template<typename C> static char (&g(...))[2];

      static bool const beg_value = sizeof(f<T>(0)) == 1;
      static bool const end_value = sizeof(g<T>(0)) == 1;
  };

  template<typename T>
  struct is_container : std::integral_constant<bool, has_const_iterator<T>::value && has_begin_end<T>::beg_value && has_begin_end<T>::end_value>
  { };

  template <typename T>
  void appendBitVectorToByteVector(std::vector<T>& byteVector, const std::vector<bool>& bits)
  {
    const uint32_t numBits  = bits.size();
    const uint32_t numBytes = ceil((double)numBits/8.0);
    byteVector.reserve(numBytes+byteVector.size());

    uint32_t bit = 0;
    for(uint32_t i=0; i<numBytes; ++i)
    {
      uint32_t byte = 0;
      for (uint32_t j=0; j<8u; ++j)
      {
        if (bits[bit++]) byte |= 1 << j;

        if(bit==numBits){
          j=8;
          i=numBytes;
        }
      }
      byteVector.push_back(byte);
    }
  }

  template<typename T>
  void printVector(const std::vector<T> vec, const std::string &delim = "\t")
  {
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<T>(std::cout, delim));
  }


  template<typename T>
  void removeDuplicates(std::vector<T>& vec)
  {
    std::sort(vec.begin(), vec.end());
    vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
  }

  template<class Cont, typename Type >
  void removeZeros(Cont & vec) {
    vec.erase(std::remove(vec.begin(), vec.end(), (Type)0), vec.end());
  }

  template<class Cont >
  void removeZerosMap(Cont & map) {
    for(auto it = map.begin(); it!=map.end(); ) {
      if((*it).second == 0)
        it = map.erase(it);
      else
        it++;
    }
  }


  template<class Cont, typename Type >
  void removeValue(Cont & vec, Type value) {
    auto it = std::find(vec.begin(), vec.end(), value);
    if (it != vec.end()) {
      // swap the one to be removed with the last element
      // and remove the item at the end of the container
      // to prevent moving all items after '5' by one
      swap(*it, vec.back());
      vec.pop_back();
    }
  }

  template<typename Type >
  void removeValues(std::vector<Type> & vec, const std::vector<Type> & rem) {
    vec.erase( std::remove_if( vec.begin(),vec.end(),
                               [&](Type x){return std::find(rem.begin(),rem.end(),x)!=rem.end();}), vec.end() );
  }


  template<typename T>
  std::vector<double> calcRelativeDistribution(const std::vector<T> & values)
  {
    double meanValue = getMeandAndRMS(values).mean;
    std::vector<double> relValues(values.size());
    int idx=0;
    for(auto && value : values){
      relValues[idx++] = (value - meanValue)/meanValue*100.0;
    }
    return relValues;
  }

  inline std::string boolVecToStdStr(const std::vector<bool> & bits)
  {
    std::stringstream s;
    for(auto bit : bits) s << bit;

    return s.str();
  }

  template <typename T=int, size_t S=sizeof(int)>
  inline std::string intToBitString(T word, int numBits)
  {
    //bitstring: MSB..LSB
    std::bitset<S*8> bitset(word);
    return "'b" + bitset.to_string().substr(S*8-numBits, S*8-1);
  }


  inline void doubleRead(std::istream &in, std::string &str, char delim)
  {
    std::getline(in,str,delim); // ergebnis aus erstem read wird nicht ben??tigt
    std::getline(in,str);
  }

  template<typename T>
  inline void doubleRead(std::istream &in, T &val, char delim)
  {
    std::string line;
    std::getline(in,line,delim); // ergebnis aus erstem read wird nicht ben??tigt
    std::getline(in,line); // second getline is required to remove \n character
    std::istringstream iss(line);
    iss >> val;
  }

  inline std::string getSection(std::istream &in, int numPos, char delim)
  {
    std::string part;
    for(int i=0; i<=numPos; i++){
      std::getline(in,part,delim);
    }
    return part;
  }


  inline std::string getSection(const std::string &line, int numPos, char delim)
  {
    std::istringstream iss(line);
    return getSection(iss,numPos,delim);
  }

  struct Timer{
      Timer();
      Timer(const std::string & endText);
    ~Timer();
    void restart();

    static Timer * g_Timer;

    template<typename TimeUnit>
    uint64_t lapsed_time(){
      return std::chrono::duration_cast<TimeUnit>(std::chrono::high_resolution_clock::now()-start).count();
    }

    std::chrono::high_resolution_clock::time_point start;
    std::string m_endText;
  };



  //do not check doubles for equality, use this function instead
  template<class T>
  typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
      almost_equal(T x, T y)
  {
      // the machine epsilon has to be scaled to the magnitude of the values used
      // and multiplied by the desired precision in ULPs (units in the last place)
      return std::abs(x-y) < std::numeric_limits<T>::epsilon() * std::abs(x+y) * (T)2
      // unless the result is subnormal
             || std::abs(x-y) < std::numeric_limits<T>::min();
  }

  template<class T>
  bool vectors_are_identical(const std::vector<T> & x, const std::vector<T> & y)
  {
    const auto numValues = x.size();
    if(y.size() != numValues) return false;

    for(size_t idx=0; idx<numValues; idx++){
      if(x[idx]!=y[idx]) return false;
    }

    return true;
  }

  std::string calcReadoutWindowFromStartPixel(int pixel, int windowSize = 10);

  template<typename TYPE>
  void reduceTo8BitData(const TYPE * dataIn, uint8_t * dataOut, size_t numValues)
  {
    int numChunks      = numValues/8;
    int numChunkValues = numValues/numChunks;

#pragma omp parallel for
    for(int chunk=0; chunk<numChunks; chunk++)
    {
      auto * chunkDataIn = dataIn + chunk*numChunkValues;
      auto * chunkDataOut = dataOut + chunk*numChunkValues;
      for(int idx=0; idx<numChunkValues; idx++){
        chunkDataOut[idx] = (chunkDataIn[idx] & 0xFF);
      }
    }

    if(numValues%8 != 0){
      auto * chunkDataIn = dataIn + numChunks*numChunkValues;
      auto * chunkDataOut = dataOut + numChunks*numChunkValues;
      for(size_t idx=0; idx<numValues%8 ; idx++){
        chunkDataOut[idx] = (chunkDataIn[idx] & 0xFF);
      }
    }

  }

  void expandTo16BitData(const uint8_t * dataIn, uint16_t * dataOut, size_t numValues);

  std::string positionStringVectorToList(const std::vector<std::string> & positions, const std::string & delim = ";");
  std::vector<std::string> positionListToStringVector(const std::string &positions);

  std::string stringVectorToList(const std::vector<std::string> & positions, const std::string & delim = ";");

  template<typename T = uint32_t>
  static std::vector<T> positionListToVector(std::string positions)
  {
  #ifdef DEBUG
    std::cout << "INFO positionListToVector: " << positions << std::endl;
  #endif

    if(positions.compare("all") == 0){ throw "PositionList error can not convert all to vector!";}

    std::vector<T> pos;

    if (positions.length()==0){
      std::cout << "ERROR utils: Positions String empty" << std::endl;
      return pos;
    }

    if(positions.compare(positions.size()-1,1,";") == 0 || positions.compare(positions.size()-1,1,"-") == 0 ){
      positions.erase(positions.end()-1);
    }

    std::vector<std::string> rangeList;
    split(positions,';',rangeList);
    if (rangeList.size() == 0){
      std::cout << "ERROR utils: RangeList Size zero" << std::endl;
      return pos;
    }

    for (const auto & activeRange :rangeList)
    {
      std::vector<std::string> startEnd;
      split(activeRange,'-',startEnd);

      if (startEnd.size() == 1)
      {
        T newpos = stoul(startEnd.front());
        pos.push_back(newpos);
      }
      else if (startEnd.size() == 2)
      {
        int start = stoi(startEnd.front());
        int end = stoi(startEnd[1]);

        if (start <= end)
        {
          for (int i=start; i <= end; i++)
            pos.push_back(static_cast<T>(i));
        }
        else
        {
          for (int i=start; i >= end; i--)
            pos.push_back(static_cast<T>(i));
        }
      }
      else
      {
        std::cout << "ERROR utils: String format invalid: '" << positions << "'"  << std::endl;
      }
    }

  #ifdef DEBUG
    std::cout << "INFO positionListToVector: " << pos.size() << std::endl;
  #endif
    return pos;
  }

  template<typename T = uint32_t>
  static std::string positionVectorToList(const std::vector<T> & positions)
  {
    std::vector<std::string> rangeList;
    unsigned i=0;
    std::string newRange;

    if (positions.size()==0)
      return "";

    if (positions.size()==1)
      return std::to_string(positions.front());

    if (positions.size()==2)
      return std::to_string(positions.front()) + ";" + std::to_string(positions.back());

    while(i<positions.size())
    {
      T lastVal = positions[i];
      newRange = std::to_string(lastVal);

      unsigned j=i+1;

      while(j<positions.size()){
        if(abs((int)positions[j]-(int)lastVal) == 1){
          lastVal = positions[j];
          j++;
        }else{
          break;
        }
      }

      if(j-i>1){
        newRange += "-"+ std::to_string(positions[j-1]);
      }

      rangeList.push_back(newRange);
      i=j;
    }
    return join(rangeList,";");
  }

  std::vector<std::string> getChipParts(const std::string & chipParts);

  template <typename TYPE_OUT=uint32_t, typename TYPE_IN=int>
  std::vector<TYPE_OUT> getSendingColumnPixels(const std::vector<TYPE_IN> & sendingAsics, int col, bool ladderMode = true)
  {
    size_t numAsics = 1;
    size_t numColPixels = 64;
    size_t numRowPixels = 64;

    if(ladderMode){
      numAsics = 16;
      numRowPixels = 512;
    }

    std::vector<TYPE_OUT> pixels;
    for(auto && sendingAsic : sendingAsics){
      size_t asic = (numAsics==1)? 0 : sendingAsic;
      size_t asicColOffs = asic%8 * 64;
      size_t asicRowOffs = asic/8 * 64;
      size_t asicOffs   = asicRowOffs * numRowPixels + asicColOffs;
      for (size_t row=0; row<numColPixels; ++row) {
        TYPE_OUT imagePixel = asicOffs + col + row*numRowPixels;
        pixels.push_back(imagePixel);
      }
    }
    std::sort(pixels.begin(),pixels.end());
    return pixels;
  }


  template <typename TYPE_OUT=uint32_t, typename TYPE_IN=int>
  std::vector<TYPE_OUT> getSendingColumnPixels(const std::vector<TYPE_IN> & sendingAsics, const std::string & colsStr, bool ladderMode = true)
  {
    std::string remColsStr;
    auto colonPos = colsStr.find(":");
    int offset = 0;
    int count  = 0;
    if (colonPos!=std::string::npos) {
      remColsStr = colsStr.substr(0,colonPos);
      int offsetPos = colsStr.find("o");
      count  = stoi(colsStr.substr(colonPos+1,colonPos-offsetPos));
      offset = stoi(colsStr.substr(offsetPos+1));
    } else {
      remColsStr = colsStr;
      if(colsStr.find("col") != std::string::npos){
        remColsStr = colsStr.substr(3);
      }
    }

    std::vector<int> cols = utils::positionListToVector<int>(remColsStr);
    std::vector<TYPE_OUT> pixels;
    for (auto c: cols) {
      std::vector<TYPE_OUT> colPxs = getSendingColumnPixels<TYPE_OUT,TYPE_IN>(sendingAsics,c,ladderMode);
      pixels.insert(pixels.end(),colPxs.begin()+offset,
                    //(count==0 || count+offset>colPxs.size() ? colPxs.end()
                    (count==0 ? colPxs.end() : colPxs.begin()+offset+count)
                   );
    }
    return pixels;
  }

  template <typename TYPE_OUT=uint32_t, typename TYPE_IN=int>
  std::vector<TYPE_OUT> getSendingChipPartPixels(const std::vector<TYPE_IN> & sendingAsics, const std::string & chipParts, bool ladderMode = true)
  {
    std::vector<TYPE_OUT> pixels;
    const auto chipPartsVec = getChipParts(chipParts);
    for(auto && part : chipPartsVec){
      auto runPixels = getSendingColumnPixels(sendingAsics,part,ladderMode);
      pixels.insert(pixels.end(),runPixels.begin(),runPixels.end());
    }
    std::sort(pixels.begin(),pixels.end());
    return pixels;
  }


  template<typename T>
  int countZeroes(const std::vector<T> & values)
  {
    int cnt = 0;
    for(const auto & value : values){
      if(value == (T)0){
        cnt++;
      }
    }
    return cnt;
  }


  struct LargerZeroSection{
    bool operator()(const std::pair<int,int> & a, const std::pair<int,int> & b) const {
      return (a.second-a.first) > (b.second-b.first);
    }
  };


  template<typename T>
  std::pair<int,int> getLargestZeroSection(const std::vector<T> & values)
  {
    int numValues = values.size();
    std::vector<std::pair<int,int>> zeroSections;
    bool found = false;
    int start = 0;
    int end = numValues-1;

    int idx = 0;
    for(const auto & value : values){
      if(!found){
        if(value == (T)0){
          start = idx;
          end   = idx;
          found = true;
        }
      }else{
        if(value==(T)0){
          end = idx;
        }else{
          zeroSections.push_back({start,end});
          found = false;
        }
      }
      idx++;
    }

    if(found){
      zeroSections.push_back({start,end});
    }

    if(zeroSections.empty()) return {0,numValues-1};

    std::sort(zeroSections.begin(),zeroSections.end(),LargerZeroSection());
    return zeroSections.front();
  }


  template<typename T>
  int getMiddleZero(const std::vector<T> & values)
  {
    const auto largestSection = utils::getLargestZeroSection<T>(values);
    return (largestSection.second+largestSection.first)/2;
  }

  template <typename T>
  T getRandomNumber(T min, T max)
  {
    static std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
  }

  template <typename T>
  T getRandomRealNumber(T min, T max)
  {
    static std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<double> distribution(min,max);
    return distribution(generator);
  }



  std::string ipToString(uint32_t addr);
  uint32_t ipToInt(const std::string & addr);
  std::string macToString(uint64_t addr);
  uint64_t macToInt(const std::string & addr);

  //#########################################
  template<typename TYPE = uint32_t>
  std::pair<TYPE,TYPE> getRange(const std::string & rangeStr)
  {
    std::vector<TYPE> elements;
    split(rangeStr,'-',elements);

    if(elements.size() != 2) return {0,0};

    return {elements.front(),elements.back()};
  }

  template<typename T>
  bool getSimpleSweepVector(const std::string & str, std::vector<T>& settings)
  {
    settings.clear();

    std::string rangeStr = str;
    int stepSize = 1;

    // if 0-63;8 =-> 0,8,16
    if(str.find(';') != std::string::npos)
    {
      if(str.find('-') == std::string::npos){
        std::cout << "Generation of SimpleSweepVectorFailed" << std::endl;
        return false;
      }

      std::vector<std::string> elements;
      split(str,';',elements);

      if(elements.size() < 2){
        return false;
      }

      stepSize = stoi(elements.back());
      rangeStr = elements.front();
    }

    auto range = getRange<uint32_t>(rangeStr);

    for(uint value = range.first; value<=range.second; value += stepSize){
      settings.push_back(value);
    }

    return true;
  }

  template <typename T = uint32_t >
  static std::vector<T> getSettingsVector(int start, int end, int numValues)
  {
    const int step = (end-start)/(numValues-1);

    std::vector<T> values(numValues);
    for(int i=0,val = start; i<numValues; i++){
      values[i] = val;
      val += step;
    }

    return values;
  }


  template<typename T>  // be careful not safe for gcc-4.8 because of regex_iterator
  bool getSweepVector(const std::string & str, std::vector<T>& settings, bool randomize = false)
  {
    static const std::string doubleEx   = "\\d+(\\.\\d+)*";
    static const std::string range1Ex    = "(^"+doubleEx+"\\-"+doubleEx+"(;|:)("+doubleEx+")$)";
    static const std::string range2Ex    = "(^"+doubleEx+"\\-"+doubleEx+"$)";
    static const std::string rangeEx    = range1Ex + "|" + range2Ex;
    static const std::string stepSizeEx = "(^.*;.*$)";
    static const std::string numStepsEx = "(^.*:\\d+$)";
    static const std::string list1 = "((.*)("+doubleEx+";){2,}(.*))";
    static const std::string list2 = "(^"+doubleEx+";"+doubleEx+"$)";
    static const std::string list3 = "(^"+doubleEx+"$)";
    static const std::string listEx = list1 + "|" + list2 + "|" + list3;

    settings.clear();

    {
      static const std::regex ex(doubleEx);
      std::regex_iterator<std::string::iterator> rit (str.begin(),str.end(),ex);
      std::regex_iterator<std::string::iterator> rend;

      while (rit!=rend) {
        settings.push_back(stod(rit->str()));
        rit++;
      }
    }

    if (std::regex_match(str.begin(),str.end(),std::regex(rangeEx)))
    {
      int numValues = settings.size();

      T settingStep  = 1;
      T startSetting = settings[0];
      T stopSetting  = settings[1];

      if(numValues == 3)
      {
        if (std::regex_match(str.begin(),str.end(),std::regex(numStepsEx))) {
          int totalSteps = settings[2];
          if (totalSteps<=1) {
            stopSetting = startSetting;
          } else {
            settingStep = (stopSetting-startSetting) / (totalSteps-1);
          }
        } else if (std::regex_match(str.begin(),str.end(),std::regex(stepSizeEx))) {
          settingStep = settings[2];
        }

        if (!(settingStep>0 || settingStep<0)) { // double
          std::cout <<  "warning setting step must be different from 0. Otherwise measurement will take forever..." << std::endl;
          return false;
        }
      }
  //     std::cout << "startSetting is " << startSetting << std::endl;
  //     std::cout << "stopSetting  is " << stopSetting  << std::endl;
  //     std::cout << "settingStep  is " << settingStep  << std::endl;

      settings.clear();
      T currSetting = startSetting;
      do { // add at least one setting if startSetting == stopSetting
        settings.push_back(currSetting);
        currSetting += settingStep;
      } while (currSetting <= stopSetting);

    } else if(!std::regex_match(str.begin(),str.end(),std::regex(listEx))){
      std::cout << "Error getting sweep settings from " << str << "." << std::endl;
      return false;
    }

    if (randomize) std::random_shuffle(settings.begin(), settings.end());

    std::cout << "Settings Vector = ";
    for (const auto & s : settings) {
      std::cout <<  s << " ";
    }
    std::cout << std::endl;

    return true;
  }

  int calcASICPixel(int imagePixel);
  int calcImagePixel(int asic, int asicPixel);
  int getPixelASIC(int imagePixel);

  int getAsicOfPixel(int pixel, bool invertUpperRow, int maxASIC);
  int getAsicPixelNumber(int pixel, bool rotate, int maxASIC);

  template <typename TYPE>
  std::vector<TYPE> calcASICPixels(int asic)
  {
    std::vector<TYPE> pixels(4096);
    for(int idx=0; idx<4096; idx++){
      pixels[idx] = calcImagePixel(asic,idx);
    }
    return pixels;
  }

  template <typename TYPE>
  std::vector<TYPE> generateASICWiseSortedPixelVector(){
    std::vector<TYPE> pixelVec(16*4096);
    int idx = 0;
    for(int asic = 0; asic<16; asic++){
      for(int px = 0; px < 4096; px++){
        pixelVec[idx++] = utils::calcImagePixel(asic,px);
      }
    }
    return pixelVec;
  }

  float convertMAX6673values(int low, int high);

  int getFirstOneInValue(uint16_t value);
  std::vector<uint32_t> bitEnableValueToVector(uint16_t value);
  uint16_t vectorToBitEnableValue(const std::vector<uint32_t> & valuesVec);
  uint16_t bitEnableStringToValue(const std::string & bitVectorStr);
  std::string vectorToBitEnableStringToValue(const std::vector<uint32_t> & valuesVec);
  std::string bitEnableValueToString(uint16_t value);
  std::vector<uint32_t> bitEnableStringToVector(const std::string & bitVectorStr);

  template<typename T = uint32_t>
  T invertBitEnableValue(T in, int numBits)
  {
    T mirror = 0;
    for(int i=0; i<numBits; i++){
      if((1ul<<i) & in){
        mirror += (1<<(numBits-i-1));
      }
    }
    return mirror;
  }

  template <typename KEY_T, typename VAL_T>
  std::vector<KEY_T> getKeys(const std::map<KEY_T,VAL_T> & valuesMap)
  {
    std::vector<KEY_T> keys;
    keys.reserve(valuesMap.size());
    for(auto && item : valuesMap){
      keys.push_back(item.first);
    }
    return keys;
  }

  std::vector<std::string> readFilesFromDirectory(const std::string & fileName, const std::string & select = "");

  bool dirExists(const std::string & dirPath);
  bool makePath(const std::string & dirPath);

  std::string getCwd();
  void escapeSpaces(std::string & input);
  int getNumberOfLinesInFile(const std::string & fileName);

  template <typename T>
  std::vector<T> getCountingVector(T start, T end){
    std::vector<T> values;
    for(T i = start; i<=end; i++){
      values.push_back(i);
    }
    return values;
  }

  template <typename T>
  void reduceRange(std::vector<T> & values, double nSigma)
  {
    const auto stats = getMeandAndRMS(values);
    T minVal = stats.mean - (nSigma * stats.rms);
    T maxVal = stats.mean + (nSigma * stats.rms);
    const size_t numValues = values.size();
#pragma omp parallel for
    for(size_t idx = 0; idx<numValues; idx++){
      values[idx] = std::min(maxVal,values[idx]);
      values[idx] = std::max(minVal,values[idx]);
    }
  }


  template <typename T>
  uint32_t countOnesInInt(T value){
    uint32_t v = value;
    v = v - ((v >> 1) & 0x55555555);                  // Zähle Bits in Zweiergruppen
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);   // Addiere Werte in Zweiergruppen -> 4er
    v = (v + (v >> 4));                               // Addiere Werte in Vierergruppen -> 8er
    v &= 0x0F0F0F0F;                                  // Lösche unnütze Bits
    return ((v * 0x01010101) >> 24);                  // Addiere die 4 Achtergruppen
  }

  template <typename DATA_T, typename OUT_T = uint32_t>
  static bool fillIdVector(const uint8_t* inData, std::vector<OUT_T> & idVec)
  {
    const uint32_t INC = sizeof(DATA_T);
    uint32_t nextByte  = 0;
    for(uint32_t i=0; i<idVec.size();i++)
    {
      idVec[i] = *((DATA_T*)&inData[nextByte]);
      nextByte += INC;
    }
    return true;
  }

  template <typename TYPE>
  TYPE wordConv(const uint8_t * data, uint32_t &num){
    TYPE word = *((TYPE *) &data[num]);
    num += sizeof(TYPE);
    return word;
  }

  double sgn(double x);
  double inverseErf(double x);   // inverseError Function goot between -0.9 and +0.9 : delta < 0.1%
  double linearRegression(const std::vector<double> & xValues,
                          const std::vector<double> & yValues);

  template <typename T>
  double linearRegression(const double s_x, const double s_xx,
                          const std::vector<T> & xValues,
                          const std::vector<double> & yValues)
  {
    const uint32_t n = std::min(xValues.size(),yValues.size());

    //uint32_t s_x = 0.0;
    //uint32_t s_xx = 0.0;
    double s_y  = 0.0;
    double s_xy = 0.0;

    for(unsigned i=0; i<n; i++){
      //s_x  += xValues[i];
      //s_xx += xValues[i]*xValues[i];
      s_xy += xValues[i]*yValues[i];
      s_y  += yValues[i];
    }

    const double covXY = (n * s_xy - s_x * s_y);
    const double varX  = (n * s_xx - s_x * s_x);
    const double slope  =  covXY / varX;

    return slope;
  }

  void exportDataFile(std::string filename, std::string header, std::vector<int> col0, std::vector<int> col1, std::vector<double> col2, bool append=false);
  void exportDataFile(std::string filename, std::string header, std::vector<int> col0, std::vector<double> col1, bool append=false);
  void exportDataFile(std::string filename, std::string header, std::vector<double> col1, bool append=false);
  void exportDataFile(std::string filename, std::string header, std::vector<std::vector<double> > rowData, bool append=false);


  struct RetrieveKey
  {
    template <typename T>
    typename T::first_type operator()(T keyValuePair) const
    {
      return keyValuePair.first;
    }
  };

  std::string getIOBSerialStr(uint32_t iobSerial);

  int extractNumberFromString(std::string &input);

  template<typename Type = uint32_t>
  std::vector<Type> getUpCountingVector(size_t num){
    std::vector<Type> values(num);
    int idx = 0;
    for(auto & val : values){
      val = idx++;
    }
    return values;
  }

  std::vector<uint32_t> extractVersionNumbers(std::string verStr);
  bool compVersion (std::string left, std::string right);

  std::string getMajorVersion(const std::string & versionStr);
  std::string getMinorVersion(const std::string & versionStr);
  std::string getMaxVersion(std::vector<std::string> versions);
  void sortVersionStrings(std::vector<std::string> &versions);

  void saveTrainIDBlacklist(const std::string & fileName, const std::map<uint32_t, uint32_t> &outlierCounts, const std::vector<uint64_t> & trainIds);
  std::vector<uint64_t> importTrainIDBlacklist(const std::string & fileName);


  void savePxSramBlacklist(const std::string & fileName, const std::vector<uint32_t> &pixels, const std::vector<std::vector<uint32_t>> & outlierSrams);
  std::vector<std::vector<uint32_t> > importPxSramBlacklist(const std::string & fileName);

  void saveTrainIDBlacklistSummary(const std::string & fileName, const std::vector<uint32_t> & pixels, const std::vector<std::vector<uint32_t>> & outlierBurstIdxs, const std::vector<uint64_t> & trainIDs);
  std::vector<std::vector<uint16_t>> importPxTrainIDBlacklistSummary(const std::string & fileName, std::vector<uint64_t> &trainIDs);
}



#endif
