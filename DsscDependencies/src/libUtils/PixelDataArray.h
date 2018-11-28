#ifndef PIXELDATAARRAY_H
#define PIXELDATAARRAY_H

#include "VariableIterator.h"

namespace utils {

class ConstPixelDataArray{
  public:

    enum DATAFORMAT{PIXEL=0,ASIC=1,IMAGE=2};


    ConstPixelDataArray(DATAFORMAT format, size_t imagePixel, size_t numFrames, bool ladderMode, const uint16_t * imageData);
    ConstPixelDataArray(DATAFORMAT format, size_t asicIdx, size_t asicPixelIdx, size_t numFrames, bool ladderMode, const uint16_t * imageData);


    const uint16_t* data() const { return m_pixelData; }

    const uint16_t& operator*() const { return *begin(); }

    const uint16_t& operator[](size_t offs) const {return begin()[offs]; }

    ConstVariableIterator operator+(size_t offs) const { return begin()+offs; }

    size_t getIncCnt() const;
    size_t size() const {return m_numEntries;}

    virtual ConstVariableIterator begin() const;
    virtual ConstVariableIterator end() const;

  protected:

    const uint16_t * calcPixelDataOffset(const uint16_t * imageData) const;

    const DATAFORMAT m_format;

    const size_t m_asicIdx;
    const size_t m_pixelIdx;
    const size_t m_imagePixel;
    const size_t m_numEntries;

    uint16_t * m_pixelData;
};


class PixelDataArray : public ConstPixelDataArray{
  public:

    PixelDataArray(DATAFORMAT format, size_t imagePixel, size_t numFrames, bool ladderMode, uint16_t * imageData);
    PixelDataArray(DATAFORMAT format, size_t asicIdx, size_t asicPixelIdx, size_t numFrames, bool ladderMode, uint16_t * imageData);

    DATAFORMAT getDataFormat() const { return m_format;}

    // dereference operator
    uint16_t& operator*() { return *begin(); }

    uint16_t* data(){ return m_pixelData; }

    uint16_t& operator[](size_t offs){ return begin()[offs]; }

    VariableIterator operator+(size_t offs) { return begin()+offs; }

    virtual VariableIterator begin();
    virtual VariableIterator end();

  protected:

    uint16_t * calcPixelDataOffset(uint16_t * imageData) const;
};


class ConstImageDataArray : public ConstPixelDataArray
{
  public:
    ConstImageDataArray(DATAFORMAT format, size_t frame, size_t numPixels, bool ladderMode, const uint16_t * imageData);

    ConstVariableIterator begin() const override;
    ConstVariableIterator end() const override;

  private:
    const uint16_t * calcPixelDataOffset(const uint16_t * imageData) const;
    const size_t m_frameIdx;
};


class ImageDataArray : public PixelDataArray
{
  public:
    ImageDataArray(DATAFORMAT format, size_t frame, size_t numPixels, bool ladderMode, uint16_t * imageData);

    ConstVariableIterator begin() const override;
    ConstVariableIterator end() const override;

    VariableIterator begin() override;
    VariableIterator end() override;

  private:
    uint16_t *calcPixelDataOffset(uint16_t *imageData) const;

    const size_t m_frameIdx;
};


class ConstAsicDataArray : public ConstPixelDataArray
{
  public:
    ConstAsicDataArray(DATAFORMAT format, size_t asicIdx, size_t frame, bool ladderMode, const uint16_t * imageData);

    ConstVariableIterator begin() const override;
    ConstVariableIterator end() const override;

  private:
    const uint16_t * calcPixelDataOffset(const uint16_t * imageData) const;
    const size_t m_frameIdx;
};


class AsicDataArray : public PixelDataArray
{
  public:
    AsicDataArray(DATAFORMAT format, size_t asicIdx, size_t frame, bool ladderMode, uint16_t * imageData);

    ConstVariableIterator begin() const override;
    ConstVariableIterator end() const override;

    VariableIterator begin() override;
    VariableIterator end() override;

  private:
    uint16_t * calcPixelDataOffset(uint16_t * imageData) const;
    const size_t m_frameIdx;
};
}

#endif // PIXELDATAARRAY_H
