#ifndef DSSCTRAINDATA_HH
#define DSSCTRAINDATA_HH

#include <iostream>
#include <inttypes.h>
#include <vector>
#include <array>
#include <string>
#include <cassert>

#include <math.h>

#include "PixelDataArray.h"
#include "DsscTrainDataTypes.hh"

namespace utils{

  constexpr size_t s_numTrainHeaderBytes  = 64;
  constexpr size_t s_numTrainTrailerBytes = 32;
  constexpr size_t s_numASICTrailerBytes  = 256;
  constexpr size_t s_numSpecificBytes     = 160;  //16PPT + 144SIB
  constexpr size_t s_numTTPBytes          = 8;

  enum TrainSections{ THEAD = 0, TDATA = 1, CELLID = 2, PULSEID = 3, STATUS = 4, LEN = 5, SPECIFIC = 6, ASIC = 7, TTRAIL = 8 , TOTAL = 9 };
  size_t getNumPacketsToReceive(int numFramesToSend);
  size_t totalBytesToReceive(int numFramesToSend, bool pixelWise = false);
  size_t numBytesToReceive(int numFramesToSend, int num = TOTAL, bool pixelWise = false);
  size_t getCellIdsStartByte(int numFramesToSend, bool pixelWise = false);
  size_t getPulseIdsStartByte(int numFramesToSend, bool pixelWise = false);
  size_t getStatusStartByte(int numFramesToSend, bool pixelWise = false);
  size_t getSpecificStartByte(int numFramesToSend, bool pixelWise = false);
  size_t getASICTrailerStartByte(int numFramesToSend, bool pixelWise = false);
  size_t getTrainTrailerStartByte(int numFramesToSend, bool pixelWise = false);


class DsscTrainData : public std::vector<uint16_t>
{
  public:

    static const uint64_t RECV_ERROR_TIMEOUT = 0xFFFFFFFFFFFFFFFF;

    static const size_t s_packetSize    = 8184;
    static const size_t s_numPackets    = 12900;
    static const size_t s_trainSize     = s_numPackets * s_packetSize/2;

    static const uint16_t c_IOB_DUMMY_DATA_TESTPATTERN = 0x2;
    static const uint16_t c_DR_DUMMY_DATA_TESTPATTERN  = 0x1;

    static const size_t c_HEADER_OFFS  = 32;

    static bool s_en8BitMode;
    static uint16_t s_expectedTestpattern;

    using std::vector<uint16_t>::vector;
    using DATAFORMAT = utils::PixelDataArray::DATAFORMAT;

    enum RECVSTATUS{ OK = 0, TIMEOUT = 1, TESTPATTERN = 2, LOSTPACKETS = 3};

    bool isSingleAsicMode() const {return availableASICs.size() == 1;}
    bool isPixelWise() const {return (m_format == DATAFORMAT::PIXEL);}

    DATAFORMAT getFormat() const {return m_format;}
    void setFormat(DATAFORMAT newFormat) {m_format = newFormat;}
    std::string getFormatStr() const;
    static DATAFORMAT getFormat(const std::string & formatStr);
    RECVSTATUS getRecvStatus() const;
    std::string getRecvStatusStr() const;

    inline const uint16_t * imageData() const {return data() + c_HEADER_OFFS;}
    inline uint16_t * imageData() {return data() + c_HEADER_OFFS;}

    const uint16_t * imageData(uint32_t frameNum) const  //returns data asic wise
    {
      assert(frameNum < pulseCnt);

      if(isPixelWise())
      { // copy frame data into static ladder frame array
        static std::array<uint16_t,s_totalNumPxs> s_frameDataPixelWise;

        const size_t numAsics = availableASICs.size();
        auto frameDataOffset = imageData() + frameNum;

#pragma omp parallel for
        for(size_t asicIdx=0; asicIdx<numAsics; asicIdx++)
        {
          auto asicFrameData = frameDataOffset+(asicIdx*s_numAsicPixels*pulseCnt);
          auto pixelDataPixelWise = s_frameDataPixelWise.data()+(asicIdx*s_numAsicPixels);
          for(size_t pxIdx=0; pxIdx<s_numAsicPixels; pxIdx++){
            pixelDataPixelWise[pxIdx] = asicFrameData[pxIdx*pulseCnt];
          }
        }
        return s_frameDataPixelWise.data();
      }
      else
      {
        const size_t numPixels = availableASICs.size() * s_numAsicPixels;
        const size_t offset    = numPixels * frameNum;

        return imageData() + offset;
      }
    }


    const uint16_t * imageData(uint32_t frameNum, uint32_t asicIdx) const
    {
      assert(frameNum < pulseCnt);
      assert(asicIdx < availableASICs.size());

      if(isPixelWise())
      { // copy frame data into static ladder frame array
        static std::array<uint16_t,s_numAsicPixels> s_asicDataPixelWise;

        auto asicFrameData = imageData()+(asicIdx*s_numAsicPixels*pulseCnt) + frameNum;

#pragma omp parallel for
        for(size_t pxIdx=0; pxIdx<s_numAsicPixels; pxIdx++){
          s_asicDataPixelWise[pxIdx] = asicFrameData[pxIdx*pulseCnt];
        }

        return s_asicDataPixelWise.data();
      }
      else
      {
        static std::array<uint16_t,s_numAsicPixels> s_asicDataFrameWise;

        const size_t numPixels = availableASICs.size() * s_numAsicPixels;
        const size_t offset    = numPixels * frameNum; // + 32 comes from no sort
        const auto frameData = imageData() + offset;

#pragma omp parallel for
        for(size_t pxIdx=0; pxIdx<s_numAsicPixels; pxIdx++){
          s_asicDataFrameWise[pxIdx] = frameData[s_imagePixelMap[asicIdx*s_numAsicPixels+pxIdx]];
        }

        return s_asicDataFrameWise.data();
      }
    }

    const uint16_t * getAsicDataPixelWise(uint32_t asicIdx) const
    {
      if(isPixelWise())
      {
        const auto * asicData = imageData() + (asicIdx*s_numAsicPixels*s_numSram);
        return asicData;
      }
      else
      {
        const size_t numAsics = availableASICs.size();
        const size_t totalNumWords = numAsics * s_numAsicPixels;

        static std::array<uint16_t,s_numSram*s_numAsicPixels> asicDataArr;

#pragma omp parallel for
        for(uint32_t pixel = 0; pixel < s_numAsicPixels; pixel++){
          size_t imagePixel = s_imagePixelMap[asicIdx*s_numAsicPixels+pixel];
          auto * pixelDataArr = asicDataArr.data() + s_numSram*pixel;
          const auto * pixelData = imageData() + imagePixel;
          for(uint32_t frame = 0; frame < pulseCnt; frame++){
            pixelDataArr[frame] = pixelData[totalNumWords*frame];
          }
        }
        return asicDataArr.data();
      }
    }

    const uint16_t * getPixelData(uint32_t asic, uint32_t pixel) const
    {
      const size_t numAsics = availableASICs.size();
      size_t asicIdx = 0;
      for(size_t cnt = 0; cnt<numAsics; cnt++){
        if(availableASICs[cnt]==asic){
          asicIdx = cnt;
          break;
        }
      }

      if(isPixelWise())
      {
        const auto pixelData = imageData() + (asicIdx*s_numAsicPixels+pixel) * s_numSram;
        return pixelData;
      }
      else
      {
        const size_t totalNumWords = numAsics * s_numAsicPixels;

        static std::array<uint16_t,s_numSram> pixelDataArr;

        auto pixelData = imageData() + s_imagePixelMap[asicIdx*s_numAsicPixels+pixel];
        for(uint32_t frame = 0; frame < pulseCnt; frame++){
          pixelDataArr[frame] = pixelData[totalNumWords*frame];
        }
        return pixelDataArr.data();
      }
    }

    utils::PixelDataArray getPixelDataPixelWise(uint32_t imagePixel)
    {
      bool ladderMode = availableASICs.size() == 1? false : true;
      return utils::PixelDataArray(getFormat(),imagePixel,pulseCnt,ladderMode,imageData());
    }

    utils::ConstPixelDataArray getPixelDataPixelWise(uint32_t imagePixel) const
    {
      bool ladderMode = availableASICs.size() == 1? false : true;
      return utils::ConstPixelDataArray(getFormat(),imagePixel,pulseCnt,ladderMode,imageData());
    }


    utils::PixelDataArray getPixelDataPixelWise(uint32_t asicIdx, uint32_t asicPixelIdx)
    {
      bool ladderMode = availableASICs.size() == 1? false : true;
      size_t imagePixel = s_imagePixelMap[asicIdx*s_numAsicPixels+asicPixelIdx];
      return utils::PixelDataArray(getFormat(),imagePixel,pulseCnt,ladderMode,imageData());
    }

    utils::ConstPixelDataArray getPixelDataPixelWise(uint32_t asicIdx, uint32_t asicPixelIdx) const
    {
      bool ladderMode = availableASICs.size() == 1? false : true;
      size_t imagePixel = s_imagePixelMap[asicIdx*s_numAsicPixels+asicPixelIdx];
      return utils::ConstPixelDataArray(getFormat(),imagePixel,pulseCnt,ladderMode,imageData());
    }

    utils::ImageDataArray getImageDataArray(uint32_t frameIdx)
    {
      bool ladderMode = availableASICs.size() == 1? false : true;
      return utils::ImageDataArray(getFormat(),frameIdx,s_totalNumPxs,ladderMode,imageData());
    }

    utils::ConstImageDataArray getImageDataArray(uint32_t frameIdx) const
    {
      bool ladderMode = availableASICs.size() == 1? false : true;
      return utils::ConstImageDataArray(getFormat(),frameIdx,s_totalNumPxs,ladderMode,imageData());
    }

    utils::AsicDataArray getAsicDataArray(uint32_t asicIdx, uint32_t frameIdx)
    {
      bool ladderMode = availableASICs.size() == 1? false : true;
      return utils::AsicDataArray(getFormat(),asicIdx,frameIdx,ladderMode,imageData());
    }

    utils::ConstAsicDataArray getAsicDataArray(uint32_t asicIdx, uint32_t frameIdx) const
    {
      bool ladderMode = availableASICs.size() == 1? false : true;
      return utils::ConstAsicDataArray(getFormat(),asicIdx,frameIdx,ladderMode,imageData());
    }

    uint64_t getMagicHeader(){
      return *((uint64_t *) &operator[](0));
    }

    uint16_t getTempADCValue(int asic) const;
    uint16_t getTestPattern(int asic) const;

    uint checkTestPatternData(uint16_t expectedTestPattern) const;

    const DsscAsicTrailerDataArr &getDsscAsicTrailerArray();
    const DsscAsicTrailerData &getAsicTrailerData() const;
    const DsscAsicTrailerData &getAsicTrailerData(int asic) const;

    DsscSpecificData getSpecificData() const;
    DsscSpecificData getSpecificData();
    THeader getTHeader() const;
    TTrailer getTTrailer() const;

    void setPulseCnt();
    void setTrainId();

    size_t getNumData() const {
        return s_numAsicPixels * pulseCnt * availableASICs.size();
    }

    void setInvalid();
    void setValid();
    void readFormat();

    bool isValid() const {return valid;}

    void fillAsicTrailerVec();
    void copyMeta(DsscTrainData * other);
    void fillMeta();
    void init(DsscTrainData * other);


    std::vector<uint32_t> & accumulateFrameData(uint32_t minSram, uint32_t maxSram);

    uint32_t pulseCnt;
    uint64_t trainId;
    uint32_t detLinkId;
    uint32_t tbLinkId;
    uint32_t status;
    uint32_t dataId;
    uint32_t tbSpecificLength;
    uint32_t detSpecificLength;

    std::vector<uint32_t> availableASICs;
    std::vector<uint16_t> cellIds;
    std::vector<uint64_t> pulseIds;
    std::vector<uint16_t> sibData;
    std::vector<uint16_t> pptData;
    std::vector<uint8_t> asicTrailerData;

    DsscAsicTrailerDataArr m_asicTrailerDataArray;

    bool valid;
    DATAFORMAT m_format;

  private:

    void fillHeader();
    void fillDescriptors();
};

#ifdef DSSCTRAINDATA_INIT
bool DsscTrainData::en8BitMode = false;
#endif

}

#endif //DSSCTRAINDATA_HH
