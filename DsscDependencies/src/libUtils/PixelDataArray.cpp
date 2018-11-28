#include "PixelDataArray.h"

using namespace std;

namespace utils{

ConstPixelDataArray::ConstPixelDataArray(DATAFORMAT format, size_t imagePixel, size_t numFrames, bool ladderMode, const uint16_t * imageData)
 : m_format(format),
   m_asicIdx( ladderMode? utils::s_pixelAsicMap[imagePixel] : 0),
   m_pixelIdx(ladderMode? utils::s_asicPixelMap[imagePixel] : imagePixel),
   m_imagePixel(imagePixel),
   m_numEntries(numFrames),
   m_pixelData((uint16_t*)calcPixelDataOffset(imageData))
{
}

ConstPixelDataArray::ConstPixelDataArray(DATAFORMAT format, size_t asicIdx, size_t asicPixelIdx, size_t numFrames, bool ladderMode, const uint16_t * imageData)
 : m_format(format),
   m_asicIdx(ladderMode? asicIdx : 0),
   m_pixelIdx(asicPixelIdx),
   m_imagePixel(utils::s_imagePixelMap[m_asicIdx*s_numAsicPixels+asicPixelIdx]),
   m_numEntries(numFrames),
   m_pixelData((uint16_t*)calcPixelDataOffset(imageData))
{
}

const uint16_t * ConstPixelDataArray::calcPixelDataOffset(const uint16_t * imageData) const
{
  switch(m_format){
    case DATAFORMAT::PIXEL  : return imageData + (utils::s_numAsicPixels*m_asicIdx + m_pixelIdx) * utils::s_numSram;
    case DATAFORMAT::ASIC   : return imageData + (utils::s_numAsicPixels*m_asicIdx + m_pixelIdx);
    case DATAFORMAT::IMAGE  : return imageData + m_imagePixel;
  }
  return imageData;
}

ConstVariableIterator ConstPixelDataArray::begin() const
{
  return ConstVariableIterator(m_pixelData,m_numEntries,getIncCnt(),s_linearMap);
}

ConstVariableIterator ConstPixelDataArray::end() const
{
  return ConstVariableIterator(m_pixelData,m_numEntries,getIncCnt(),s_linearMap,true);
}


size_t ConstPixelDataArray::getIncCnt() const
{
  switch(m_format){
    case DATAFORMAT::PIXEL: return 1u;
    case DATAFORMAT::ASIC : return utils::s_totalNumPxs;
    case DATAFORMAT::IMAGE: return utils::s_totalNumPxs;
  }
  return 1u;
}

// ####################################################################################

PixelDataArray::PixelDataArray(DATAFORMAT format, size_t imagePixel, size_t numFrames, bool ladderMode, uint16_t * imageData)
 : ConstPixelDataArray(format,imagePixel,numFrames,ladderMode,imageData)
{
}

PixelDataArray::PixelDataArray(DATAFORMAT format, size_t asicIdx, size_t asicPixelIdx, size_t numFrames, bool ladderMode, uint16_t * imageData)
 : ConstPixelDataArray(format,asicIdx,asicPixelIdx,numFrames,ladderMode,imageData)
{
}

VariableIterator PixelDataArray::begin()
{
  return VariableIterator(m_pixelData,m_numEntries,getIncCnt(),s_linearMap);
}

VariableIterator PixelDataArray::end()
{
  return VariableIterator(m_pixelData,m_numEntries,getIncCnt(),s_linearMap,true);
}

// ####################################################################################
// ####################################################################################

ConstImageDataArray::ConstImageDataArray(DATAFORMAT format, size_t frame, size_t numPixels, bool ladderMode, const uint16_t * imageData)
  : ConstPixelDataArray(format,0,numPixels,ladderMode,imageData),
    m_frameIdx(frame)
{
  m_pixelData = (uint16_t*)calcPixelDataOffset(imageData);
}

const uint16_t * ConstImageDataArray::calcPixelDataOffset(const uint16_t * imageData) const
{
  if(m_format == DATAFORMAT::PIXEL){
    return imageData + m_frameIdx;
  }else{
    return imageData + m_numEntries*m_frameIdx;
  }
}

ConstVariableIterator ConstImageDataArray::begin() const
{
  if(m_format == DATAFORMAT::IMAGE){
    return ConstVariableIterator(m_pixelData,m_numEntries,1u,s_linearMap);
  }else if(m_format == DATAFORMAT::ASIC){
    return ConstVariableIterator(m_pixelData,m_numEntries,1u,s_dataPixelMap);
  }else{ // PIXEL
    return ConstVariableIterator(m_pixelData,m_numEntries,s_numSram,s_dataPixelMap);
  }
}

ConstVariableIterator ConstImageDataArray::end() const
{
  if(m_format == DATAFORMAT::IMAGE){
    return ConstVariableIterator(m_pixelData,m_numEntries,1u,s_linearMap,true);
  }else if(m_format == DATAFORMAT::ASIC){
    return ConstVariableIterator(m_pixelData,m_numEntries,1u,s_dataPixelMap,true);
  }else{ // PIXEL
    return ConstVariableIterator(m_pixelData,m_numEntries,s_numSram,s_dataPixelMap,true);
  }
}

// ####################################################################################

ImageDataArray::ImageDataArray(DATAFORMAT format, size_t frame, size_t numPixels, bool ladderMode, uint16_t * imageData)
  : PixelDataArray(format,0,numPixels,ladderMode,imageData),
    m_frameIdx(frame)
{
  m_pixelData = (uint16_t*)calcPixelDataOffset(imageData);
}

uint16_t * ImageDataArray::calcPixelDataOffset(uint16_t * imageData) const
{
  if(m_format == DATAFORMAT::PIXEL){
    return imageData + m_frameIdx;
  }else{
    return imageData + m_numEntries*m_frameIdx;
  }
}

ConstVariableIterator ImageDataArray::begin() const
{
  if(m_format == DATAFORMAT::IMAGE){
    return ConstVariableIterator(m_pixelData,m_numEntries,1u,s_linearMap);
  }else if(m_format == DATAFORMAT::ASIC){
    return ConstVariableIterator(m_pixelData,m_numEntries,1u,s_dataPixelMap);
  }else{ // PIXEL
    return ConstVariableIterator(m_pixelData,m_numEntries,s_numSram,s_dataPixelMap);
  }
}

ConstVariableIterator ImageDataArray::end() const
{
  if(m_format == DATAFORMAT::IMAGE){
    return ConstVariableIterator(m_pixelData,m_numEntries,1u,s_linearMap,true);
  }else if(m_format == DATAFORMAT::ASIC){
    return ConstVariableIterator(m_pixelData,m_numEntries,1u,s_dataPixelMap,true);
  }else{ // PIXEL
    return ConstVariableIterator(m_pixelData,m_numEntries,s_numSram,s_dataPixelMap,true);
  }
}

VariableIterator ImageDataArray::begin()
{
  if(m_format == DATAFORMAT::IMAGE){
    return VariableIterator(m_pixelData,m_numEntries,1u,s_linearMap);
  }else if(m_format == DATAFORMAT::ASIC){
    return VariableIterator(m_pixelData,m_numEntries,1u,s_dataPixelMap);
  }else{ // PIXEL
    return VariableIterator(m_pixelData,m_numEntries,s_numSram,s_dataPixelMap);
  }
}

VariableIterator ImageDataArray::end()
{
  if(m_format == DATAFORMAT::IMAGE){
    return VariableIterator(m_pixelData,m_numEntries,1u,s_linearMap,true);
  }else if(m_format == DATAFORMAT::ASIC){
    return VariableIterator(m_pixelData,m_numEntries,1u,s_dataPixelMap,true);
  }else{ // PIXEL
    return VariableIterator(m_pixelData,m_numEntries,s_numSram,s_dataPixelMap,true);
  }
}

// ####################################################################################
// ####################################################################################

ConstAsicDataArray::ConstAsicDataArray(DATAFORMAT format, size_t asicIdx, size_t frame, bool ladderMode, const uint16_t * imageData)
  : ConstPixelDataArray(format,asicIdx,0,s_numAsicPixels,ladderMode,imageData),
    m_frameIdx(frame)
{
  m_pixelData = (uint16_t*)calcPixelDataOffset(imageData);
}

const uint16_t * ConstAsicDataArray::calcPixelDataOffset(const uint16_t * imageData) const
{
  if(m_format == DATAFORMAT::PIXEL){
    return imageData + m_asicIdx * s_numAsicPixels * s_numSram + m_frameIdx;
  }else if(m_format == DATAFORMAT::IMAGE){
    if(m_asicIdx>7){
      return imageData + m_frameIdx * s_totalNumPxs + 8 * s_numAsicPixels + (m_asicIdx-8) * 64;
    }else{
      return imageData + m_frameIdx * s_totalNumPxs + m_asicIdx * 64;
    }
  }else{ //ASIC
    return imageData + m_frameIdx * s_totalNumPxs + m_asicIdx * s_numAsicPixels;
  }
}

ConstVariableIterator ConstAsicDataArray::begin() const
{
  if(m_format == DATAFORMAT::IMAGE){
    return ConstVariableIterator(m_pixelData,s_numAsicPixels,1u,s_asicImageMap);
  }else if(m_format == DATAFORMAT::ASIC){
    return ConstVariableIterator(m_pixelData,s_numAsicPixels,1u,s_linearMap);
  }else{ // PIXEL
    return ConstVariableIterator(m_pixelData,s_numAsicPixels,800u,s_linearMap);
  }
}

ConstVariableIterator ConstAsicDataArray::end() const
{
  if(m_format == DATAFORMAT::IMAGE){
    return ConstVariableIterator(m_pixelData,s_numAsicPixels,1u,s_asicImageMap,true);
  }else if(m_format == DATAFORMAT::ASIC){
    return ConstVariableIterator(m_pixelData,s_numAsicPixels,1u,s_linearMap,true);
  }else{ // PIXEL
    return ConstVariableIterator(m_pixelData,s_numAsicPixels,800u,s_linearMap,true);
  }
}

// ####################################################################################

AsicDataArray::AsicDataArray(DATAFORMAT format, size_t asicIdx, size_t frame, bool ladderMode, uint16_t * imageData)
  : PixelDataArray(format,asicIdx,0,s_numAsicPixels,ladderMode,imageData),
    m_frameIdx(frame)
{
  m_pixelData = calcPixelDataOffset(imageData);
}

uint16_t * AsicDataArray::calcPixelDataOffset(uint16_t * imageData) const
{
  if(m_format == DATAFORMAT::PIXEL){
    return imageData + m_asicIdx * s_numAsicPixels * s_numSram + m_frameIdx;
  }else if(m_format == DATAFORMAT::IMAGE){
    if(m_asicIdx>7){
      return imageData + m_frameIdx * s_totalNumPxs + 8 * s_numAsicPixels + (m_asicIdx-8) * 64;
    }else{
      return imageData + m_frameIdx * s_totalNumPxs + m_asicIdx * 64;
    }
  }else{ //ASIC
    return imageData + m_frameIdx * s_totalNumPxs + m_asicIdx * s_numAsicPixels;
  }
}

ConstVariableIterator AsicDataArray::begin() const
{
  if(m_format == DATAFORMAT::IMAGE){
    return ConstVariableIterator(m_pixelData,s_numAsicPixels,1u,s_asicImageMap);
  }else if(m_format == DATAFORMAT::ASIC){
    return ConstVariableIterator(m_pixelData,s_numAsicPixels,1u,s_linearMap);
  }else{ // PIXEL
    return ConstVariableIterator(m_pixelData,s_numAsicPixels,800u,s_linearMap);
  }
}

ConstVariableIterator AsicDataArray::end() const
{
  if(m_format == DATAFORMAT::IMAGE){
    return ConstVariableIterator(m_pixelData,s_numAsicPixels,1u,s_asicImageMap,true);
  }else if(m_format == DATAFORMAT::ASIC){
    return ConstVariableIterator(m_pixelData,s_numAsicPixels,1u,s_linearMap,true);
  }else{ // PIXEL
    return ConstVariableIterator(m_pixelData,s_numAsicPixels,800u,s_linearMap,true);
  }
}

VariableIterator AsicDataArray::begin()
{
  if(m_format == DATAFORMAT::IMAGE){
    return VariableIterator(m_pixelData,s_numAsicPixels,1u,s_asicImageMap);
  }else if(m_format == DATAFORMAT::ASIC){
    return VariableIterator(m_pixelData,s_numAsicPixels,1u,s_linearMap);
  }else{ // PIXEL
    return VariableIterator(m_pixelData,s_numAsicPixels,800u,s_linearMap);
  }
}

VariableIterator AsicDataArray::end()
{
  if(m_format == DATAFORMAT::IMAGE){
    return VariableIterator(m_pixelData,s_numAsicPixels,1u,s_asicImageMap,true);
  }else if(m_format == DATAFORMAT::ASIC){
    return VariableIterator(m_pixelData,s_numAsicPixels,1u,s_linearMap,true);
  }else{ // PIXEL
    return VariableIterator(m_pixelData,s_numAsicPixels,800u,s_linearMap,true);
  }
}



}
