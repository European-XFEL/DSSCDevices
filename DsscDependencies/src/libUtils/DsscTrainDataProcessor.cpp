#include <omp.h>
#include "DsscTrainDataProcessor.h"

using namespace std;

namespace utils{

DsscTrainDataProcessor::DsscTrainDataProcessor()
 : m_sortedTrainData(DsscTrainData::s_trainSize),
   m_statsAccVec(s_totalNumPxs),
   m_pixelMeanData(s_totalNumPxs,0.0),
   m_pixelRMSData(s_totalNumPxs,0.0),
   m_availableAsics(utils::getUpCountingVector(16)),
   m_alsoRMS(true),
   m_minSram(0),
   m_maxSram(s_numSram-1),
   m_numFrames(800),
   m_iterationCnt(0),
   m_numIterations(1),
   m_inputFormat(DATAFORMAT::PIXEL)
{
  m_pixelsToProcess = getPixelsToProcess();
}

void DsscTrainDataProcessor::setParameters(const std::vector<uint32_t> & availableAsics, int numFrames, int minSram, int maxSram, DATAFORMAT format)
{
  m_availableAsics  = availableAsics;
  m_minSram         = std::min(numFrames,minSram);
  m_maxSram         = std::min(numFrames,maxSram);
  m_numFrames       = numFrames;
  m_inputFormat     = format;
  m_pixelsToProcess = getPixelsToProcess();
}


void DsscTrainDataProcessor::clearAccumulators()
{
  m_iterationCnt = 0;
#pragma omp parallel for
  for(uint idx=0; idx<s_totalNumPxs; idx++){
    m_statsAccVec[idx].clear();
  }

  std::fill(m_pixelMeanData.begin(), m_pixelMeanData.end(), 0.0);
  std::fill(m_pixelRMSData.begin(), m_pixelRMSData.end(), 0.0);
}

//------------------------
// if the DsscTrainDataProcessor is supplied with required data, several correction modes are available and applied
// during accumulator filling:
// - sram correction
// - sram blacklist
// - gccwrap
// - and baseline subtraction (in calcMeanValues() )
void DsscTrainDataProcessor::fillMeanAccumulators(const DsscTrainData * trainData, const PixelVector & pixels)
{
  // function has four cases whcih are implemented differently
  // one could also change it to one implementation with zero correction and sram blacklist = all true
  // but this would increase the computational effort for all cases to the max case

  const auto numPixels = pixels.size();
  if(isSramCorrectionDataValid())
  {
    if(isSramBlacklistValid())
    {
#pragma omp parallel for
      for(size_t pixelIdx=0; pixelIdx<numPixels; pixelIdx++)
      { // sram correction && sramBlacklist
        const auto imagePixel = pixels[pixelIdx];
        const auto pixelDataArr = trainData->getPixelDataPixelWise(imagePixel);
        const auto pxValidSrams = m_sramBlacklist.getValidSramAddresses(imagePixel);
        auto & pixelAcc  = m_statsAccVec[imagePixel];
        auto & pixelCorr = m_sramCorrectionData[imagePixel];
        uint16_t corrValue;
        for(uint sram = m_minSram; sram <= m_maxSram; sram++){
          if(pxValidSrams[sram]){
            applySramCorrection(corrValue,pixelDataArr[sram],pixelCorr[sram]);
            pixelAcc.addValue(corrValue);
          }
        }
      }
    }
    else
    {
#pragma omp parallel for
      for(size_t pixelIdx=0; pixelIdx<numPixels; pixelIdx++)
      { // sram correction && !sramBlacklist
        const auto imagePixel = pixels[pixelIdx];
        const auto pixelDataArr = trainData->getPixelDataPixelWise(imagePixel);
        auto & pixelAcc  = m_statsAccVec[imagePixel];
        auto & pixelCorr = m_sramCorrectionData[imagePixel];
        uint16_t corrValue;
        for(uint sram = m_minSram; sram <= m_maxSram; sram++){
          applySramCorrection(corrValue,pixelDataArr[sram],pixelCorr[sram]);
          pixelAcc.addValue(corrValue);
        }
      }
    }
  }
  else
  {
    if(isSramBlacklistValid())
    {
#pragma omp parallel for
      for(size_t pixelIdx=0; pixelIdx<numPixels; pixelIdx++)
      { // !sram correction && sramBlacklist
        const auto imagePixel = pixels[pixelIdx];
        const auto pixelDataArr = trainData->getPixelDataPixelWise(imagePixel);
        const auto pxValidSrams = m_sramBlacklist.getValidSramAddresses(imagePixel);
        m_statsAccVec[imagePixel].addValues(pixelDataArr.begin()+m_minSram,pixelDataArr.begin()+m_maxSram,pxValidSrams.begin()+m_minSram);
      }
    }
    else
    {
#pragma omp parallel for
      for(size_t pixelIdx=0; pixelIdx<numPixels; pixelIdx++)
      { // !sram correction && !sramBlacklist
        const auto imagePixel = pixels[pixelIdx];
        const auto pixelDataArr = trainData->getPixelDataPixelWise(imagePixel);
        m_statsAccVec[imagePixel].addValues(pixelDataArr.begin()+m_minSram,pixelDataArr.begin()+m_maxSram);
      }
    }
  }
}

//------------------------
// if the DsscTrainDataProcessor is supplied with required data, several correction modes are available and applied
// during accumulator filling:
// - sram correction
// - sram blacklist
// - gccwrap
// - and baseline subtraction (in calcMeanValues() )
void DsscTrainDataProcessor::fillMeanAccumulators(const uint16_t * trainDataPtr, DATAFORMAT format, const PixelVector & pixels)
{
  const auto numPixels = pixels.size();

  if(isSramCorrectionDataValid())
  {
    if(isSramBlacklistValid())
    {
#pragma omp parallel for
      for(size_t pixelIdx=0; pixelIdx<numPixels; pixelIdx++)
      { // sram correction && sramBlacklist
        const auto imagePixel = pixels[pixelIdx];
        const auto pixelDataArr = ConstPixelDataArray(format,imagePixel,m_numFrames,true,trainDataPtr);
        const auto pxValidSrams = m_sramBlacklist.getValidSramAddresses(imagePixel);
        auto & pixelAcc  = m_statsAccVec[imagePixel];
        auto & pixelCorr = m_sramCorrectionData[imagePixel];
        uint16_t corrValue;
        for(uint sram = m_minSram; sram <= m_maxSram; sram++){
          if(pxValidSrams[sram]){
            applySramCorrection(corrValue,pixelDataArr[sram],pixelCorr[sram]);
            pixelAcc.addValue(corrValue);
          }
        }
      }
    }
    else
    {
#pragma omp parallel for
      for(size_t pixelIdx=0; pixelIdx<numPixels; pixelIdx++)
      { // sram correction && !sramBlacklist
        const auto imagePixel = pixels[pixelIdx];
        const auto pixelDataArr = ConstPixelDataArray(format,imagePixel,m_numFrames,true,trainDataPtr);
        auto & pixelAcc  = m_statsAccVec[imagePixel];
        auto & pixelCorr = m_sramCorrectionData[imagePixel];
        uint16_t corrValue;
        for(uint sram = m_minSram; sram <= m_maxSram; sram++){
          applySramCorrection(corrValue,pixelDataArr[sram],pixelCorr[sram]);
          pixelAcc.addValue(corrValue);
        }
      }
    }
  }
  else
  {
    if(isSramBlacklistValid())
    {
#pragma omp parallel for
      for(size_t pixelIdx=0; pixelIdx<numPixels; pixelIdx++)
      {// !sram correction && sramBlacklist
        const auto imagePixel = pixels[pixelIdx];
        const auto pixelDataArr = ConstPixelDataArray(format,imagePixel,m_numFrames,true,trainDataPtr);
        const auto pxValidSrams = m_sramBlacklist.getValidSramAddresses(imagePixel);
        m_statsAccVec[imagePixel].addValues(pixelDataArr.begin()+m_minSram,pixelDataArr.begin()+m_maxSram,pxValidSrams.begin()+m_minSram);
      }
    }
    else
    {
#pragma omp parallel for
      for(size_t pixelIdx=0; pixelIdx<numPixels; pixelIdx++)
      {
        // !sram correction && !sramBlacklist
        const auto imagePixel = pixels[pixelIdx];
        const auto pixelDataArr = ConstPixelDataArray(format,imagePixel,m_numFrames,true,trainDataPtr);
        m_statsAccVec[imagePixel].addValues(pixelDataArr.begin()+m_minSram,pixelDataArr.begin()+m_maxSram);
      }
    }
  }
}


void DsscTrainDataProcessor::processMeanValues(const DsscTrainData * trainData)
{
  m_numFrames = trainData->pulseCnt;

  fillMeanAccumulators(trainData,m_pixelsToProcess);

  m_iterationCnt++;

  if(m_iterationCnt == m_numIterations){
    // send to output
    calcMeanValues();
  }else{
    cout << "DataProcessor: sorted train " << m_iterationCnt << "/" << m_numIterations << endl;
  }
}


void DsscTrainDataProcessor::processMeanValues(const uint16_t * trainData, DATAFORMAT format)
{
  fillMeanAccumulators(trainData,format,m_pixelsToProcess);

  m_iterationCnt++;

  if(m_iterationCnt == m_numIterations){
    // send to output
    calcMeanValues();
  }else{
    cout << "DataProcessor: sorted train " << m_iterationCnt << "/" << m_numIterations << endl;
  }
}


const DsscTrainData &DsscTrainDataProcessor::processPixelData(DsscTrainData * unsortedTrainData)
{
  size_t numFrames = unsortedTrainData->pulseCnt;
  size_t numAsics = unsortedTrainData->availableASICs.size();
  auto format = unsortedTrainData->getFormat();

  if(format == DATAFORMAT::PIXEL && numFrames == s_numSram && numAsics == s_numAsics)
  {
    // nothing to sort
    return *unsortedTrainData;
  }

  sortDataArray(unsortedTrainData,&m_sortedTrainData,DATAFORMAT::PIXEL);
  return m_sortedTrainData;
}


const uint16_t *DsscTrainDataProcessor::processPixelData(const uint16_t * unsortedTrainData, DATAFORMAT format)
{
  size_t numAsics = m_availableAsics.size();

  if(sramCorrectionValid()){
    sortDataArray(unsortedTrainData,m_inputFormat,m_numFrames,&m_sortedTrainData,DATAFORMAT::PIXEL,m_sramCorrectionData);
  }
  else
  {
    if(m_inputFormat == DATAFORMAT::PIXEL && m_numFrames == s_numSram && numAsics == s_numAsics)
    {
      //no need to process data
      return unsortedTrainData;
    }
    sortDataArray(unsortedTrainData,m_inputFormat,m_numFrames,&m_sortedTrainData,DATAFORMAT::PIXEL);
  }
  return m_sortedTrainData.imageData();
}


void DsscTrainDataProcessor::calcMeanValues()
{
#pragma omp parallel for
  for(size_t pixelIdx=0; pixelIdx<s_totalNumPxs; pixelIdx++)
  {
    auto & pixelMean = m_pixelMeanData[pixelIdx];
    auto & pixelRms  = m_pixelRMSData[pixelIdx];
    const auto stats = m_statsAccVec[pixelIdx].calcStats();
    pixelMean = stats.mean;
    pixelRms = stats.rms;
  }

  // baseline subtraction from mean values is implemented in LadderTrimmingDevice
  // since some functions enable and disable baseline subtraction during same run
  // it can not be done by the processor
}

void DsscTrainDataProcessor::sortDataArray(const DsscTrainData *inData,  DsscTrainData *outData, DsscTrainData::DATAFORMAT outFormat, const std::vector<std::vector<float>> & sramCorrectionMap)
{
  static const unsigned int nthreads = std::min(4u,std::thread::hardware_concurrency());

  outData->setFormat(outFormat);
  outData->pulseCnt = s_numSram;

  const auto & outAsics = outData->availableASICs;
  const size_t numAsics = outAsics.size();
  const size_t numFrames = inData->pulseCnt;

  cout << "Pro Ladder Mode   : " << (numAsics == 1) << endl;
  cout << "Pro Sending ASICs : " << utils::positionVectorToList(outAsics) << endl;
  cout << "Pro Active ASIC   : " << outData->availableASICs.front() << endl;

  //Timer timer("time to sort data array for " + to_string(numAsics) + " asics and " + to_string(numFrames) + " frames");

  if(numAsics==1)
  {// parallelize asic pixels
    const uint32_t asic = outAsics.front();

#pragma omp parallel for num_threads(nthreads)
    for(size_t frameIdx=0; frameIdx<numFrames; frameIdx++){
      auto inAsicData = inData->getAsicDataArray(asic,frameIdx);
      auto outAsicData = outData->getAsicDataArray(0,frameIdx);
      const auto numData = inAsicData.size();
      for(uint asicPx=0; asicPx<numData; asicPx++){
        applySramCorrection(outAsicData[asicPx],inAsicData[asicPx],sramCorrectionMap[s_imagePixelMap[asicPx]][frameIdx]);
      }
    }
  }
  else
  {// parallelize frames
#pragma omp parallel for num_threads(nthreads)
    for(size_t frame=0; frame<numFrames; frame++){
      auto inImageData = inData->getImageDataArray(frame);
      auto outImageData = outData->getImageDataArray(frame);
      // std::copy is faster than standard loops
      const auto numData = inImageData.size();
      for(uint imgPx=0; imgPx<numData; imgPx++){
        applySramCorrection(outImageData[imgPx],inImageData[imgPx],sramCorrectionMap[imgPx][frame]);
      }
    }
  }
}


void DsscTrainDataProcessor::sortDataArray(const DsscTrainData * inData,  DsscTrainData * outData, DATAFORMAT outFormat)
{
  static const unsigned int nthreads = std::min(4u,std::thread::hardware_concurrency());

  outData->setFormat(outFormat);
  outData->pulseCnt = s_numSram;

  const auto & outAsics = outData->availableASICs;
  const size_t numAsics = outAsics.size();
  const size_t numFrames = inData->pulseCnt;

  //Timer timer("time to sort data array for " + to_string(numAsics) + " asics and " + to_string(numFrames) + " frames");

  if(numAsics==1)
  {// parallelize asic pixels
    const uint32_t asic = outAsics.front();
#pragma omp parallel for num_threads(nthreads)
    for(size_t frameIdx=0; frameIdx<numFrames; frameIdx++){
      auto inAsicData = inData->getAsicDataArray(asic,frameIdx);
      auto outAsicData = outData->getAsicDataArray(0,frameIdx);
      // std::copy is faster than standard loops
      std::copy(inAsicData.begin(),inAsicData.end(),outAsicData.begin());
    }
  }
  else
  {// parallelize frames
#pragma omp parallel for num_threads(nthreads)
    for(size_t frame=0; frame<numFrames; frame++){
      auto inImageData = inData->getImageDataArray(frame);
      auto outImageData = outData->getImageDataArray(frame);
      // std::copy is faster than standard loops
      std::copy(inImageData.begin(),inImageData.end(),outImageData.begin());
    }
  }
}


//special case if num asics and/or num frames vary from the default format 16-4096-800
void DsscTrainDataProcessor::sortDataArray(const unsigned short * inData, DATAFORMAT inFormat, uint32_t numFrames,  DsscTrainData * outData, DATAFORMAT outFormat)
{
  static const unsigned int nthreads = std::min(4u,std::thread::hardware_concurrency());

  //Timer timer;
  outData->setFormat(outFormat);

  const auto & outAsics = outData->availableASICs;
  const size_t numAsics = outAsics.size();

  if(outFormat == DATAFORMAT::PIXEL){
    outData->pulseCnt = s_numSram;
    if(numAsics==1)
    {// parallelize asic pixels
      const uint32_t asic = outAsics.front();

#pragma omp parallel for num_threads(nthreads)
      for(size_t pixelIdx=0; pixelIdx<s_numAsicPixels; pixelIdx++){
        uint32_t imagePixel = s_imagePixelMap[asic*s_numAsicPixels+pixelIdx];
        const auto inPixelData = getDataSortIterator(inData,inFormat,outFormat,imagePixel,numFrames);
        auto outPixelData = outData->getPixelDataPixelWise(pixelIdx);
        std::copy(inPixelData.begin(),inPixelData.end(),outPixelData.begin());
      }
    }
    else
    {// parallelize asics
#pragma omp parallel for num_threads(nthreads)
      for(size_t asicIdx = 0; asicIdx < numAsics; asicIdx++){
        for(size_t pixelIdx=0; pixelIdx<s_numAsicPixels; pixelIdx++){
          uint32_t imagePixel = s_imagePixelMap[outAsics[asicIdx]*s_numAsicPixels+pixelIdx];
          const auto inPixelData = getDataSortIterator(inData,inFormat,outFormat,imagePixel,numFrames);
          auto outPixelData = outData->getPixelDataPixelWise(imagePixel);
          std::copy(inPixelData.begin(),inPixelData.end(),outPixelData.begin());
        }
      }
    }
  }
  else if(outFormat == DATAFORMAT::ASIC)
  {
    outData->pulseCnt = numFrames;
#pragma omp parallel for
    for(size_t frameIdx=0; frameIdx<numFrames; frameIdx++){
      for(size_t asicIdx=0; asicIdx<numAsics; asicIdx++){
        const auto inAsicData = getDataSortIterator(inData,inFormat,outFormat,outAsics[asicIdx],frameIdx);
        auto outAsicData = outData->getAsicDataArray(frameIdx,outAsics[asicIdx]);
        std::copy(inAsicData.begin(),inAsicData.end(),outAsicData.begin());
      }
    }
  }
  else // outFormat == DATAFORMAT::IMAGE
  {
    outData->pulseCnt = numFrames;

#pragma omp parallel for
    for(size_t frameIdx=0; frameIdx<numFrames; frameIdx++){
      const auto inImageData = getDataSortIterator(inData,inFormat,outFormat,frameIdx,0);
      auto outImageData = outData->getImageDataArray(frameIdx);
      std::copy(inImageData.begin(),inImageData.end(),outImageData.begin());
    }
  }
}


//special case if num asics and/or num frames vary from the default format 16-4096-800
void DsscTrainDataProcessor::sortDataArray(const unsigned short * inData, DATAFORMAT inFormat, uint32_t numFrames,  DsscTrainData * outData, DATAFORMAT outFormat, const std::vector<std::vector<float>> & sramCorrectionMap)
{
  if(sramCorrectionMap.empty()){
    sortDataArray(inData,inFormat,numFrames,outData,outFormat);
  }else{

    static const unsigned int nthreads = std::min(4u,std::thread::hardware_concurrency());
    //Timer timer;
    outData->setFormat(outFormat);

    const auto & outAsics = outData->availableASICs;
    const size_t numAsics = outAsics.size();

    if(outFormat == DATAFORMAT::PIXEL){
      outData->pulseCnt = s_numSram;
      if(numAsics==1)
      {// parallelize asic pixels
        const uint32_t asic = outAsics.front();

  #pragma omp parallel for num_threads(nthreads)
        for(size_t pixelIdx=0; pixelIdx<s_numAsicPixels; pixelIdx++){
          uint32_t imagePixel = s_imagePixelMap[asic*s_numAsicPixels+pixelIdx];
          const auto inPixelData = getDataSortIterator(inData,inFormat,outFormat,imagePixel,numFrames);
          auto outPixelData = outData->getPixelDataPixelWise(pixelIdx);
          const auto numData = inPixelData.size();
          for(uint sram=0; sram<numData; sram++){
            applySramCorrection(outPixelData[sram],inPixelData[sram],sramCorrectionMap[imagePixel][sram]);
          }
        }
      }
      else
      {// parallelize asics
  #pragma omp parallel for num_threads(nthreads)
        for(size_t asicIdx = 0; asicIdx < numAsics; asicIdx++){
          for(size_t pixelIdx=0; pixelIdx<s_numAsicPixels; pixelIdx++){
            uint32_t imagePixel = s_imagePixelMap[outAsics[asicIdx]*s_numAsicPixels+pixelIdx];
            const auto inPixelData = getDataSortIterator(inData,inFormat,outFormat,imagePixel,numFrames);
            auto outPixelData = outData->getPixelDataPixelWise(imagePixel);
            const auto numData = inPixelData.size();
            for(uint sram=0; sram<numData; sram++){
              applySramCorrection(outPixelData[sram],inPixelData[sram],sramCorrectionMap[imagePixel][sram]);
            }
          }
        }
      }
    }
    else if(outFormat == DATAFORMAT::ASIC)
    {
      outData->pulseCnt = numFrames;
  #pragma omp parallel for
      for(size_t frameIdx=0; frameIdx<numFrames; frameIdx++){
        for(size_t asicIdx=0; asicIdx<numAsics; asicIdx++){
          const auto * asicOffs = s_imagePixelMap.data() + outAsics[asicIdx] * s_numAsicPixels;
          const auto inAsicData = getDataSortIterator(inData,inFormat,outFormat,outAsics[asicIdx],frameIdx);
          auto outAsicData = outData->getAsicDataArray(frameIdx,outAsics[asicIdx]);
          const auto numData = inAsicData.size();
          for(uint asicPx=0; asicPx<numData; asicPx++){
            applySramCorrection(outAsicData[asicPx],inAsicData[asicPx],sramCorrectionMap[asicOffs[asicPx]][frameIdx]);
          }
        }
      }
    }
    else // outFormat == DATAFORMAT::IMAGE
    {
      outData->pulseCnt = numFrames;
  #pragma omp parallel for
      for(size_t frameIdx=0; frameIdx<numFrames; frameIdx++){
        const auto inImageData = getDataSortIterator(inData,inFormat,outFormat,frameIdx,0);
        auto outImageData = outData->getImageDataArray(frameIdx);
        const auto numData = inImageData.size();
        for(uint imagePixel=0; imagePixel<numData; imagePixel++){
          applySramCorrection(outImageData[imagePixel],inImageData[imagePixel],sramCorrectionMap[imagePixel][frameIdx]);
        }
      }
    }
  }
}


ConstPixelDataArray DsscTrainDataProcessor::getDataSortIterator(const uint16_t * inData, DATAFORMAT inFormat, DATAFORMAT outFormat, uint32_t index1, uint32_t index2)
{
  switch(outFormat){
    case DATAFORMAT::PIXEL : return ConstPixelDataArray(inFormat,index1,index2,true,inData); // index1 = imagePixel, index2 = numFrames
    case DATAFORMAT::ASIC  : return ConstAsicDataArray(inFormat,index1,index2,true,inData);  // index1 = asicIdx, index2 = frameIdx
    case DATAFORMAT::IMAGE : return ConstImageDataArray(inFormat,index1,s_totalNumPxs,true,inData); // index1 = frameIdx, index2 = not used
  }
  return ConstPixelDataArray(inFormat,index1,index2,true,inData);
}


void DsscTrainDataProcessor::fillDataHistoVec(const DsscTrainData * trainData, DataHistoVec & pixelHistos, bool fill)
{
  m_inputFormat = trainData->getFormat();

  if(m_sramBlacklist.isValid() && sramCorrectionValid())
  {// sram blacklist and sram correction
    cout << "fill histo with blacklist and correction" << endl;
    fillDataHistoVec(trainData,pixelHistos,m_sramBlacklist,m_sramCorrectionData,m_pixelsToProcess,fill);
    return;
  }

  if(m_sramBlacklist.isValid())
  {// sram blacklist
    cout << "fill histo with blacklist" << endl;
    fillDataHistoVec(trainData,pixelHistos,m_sramBlacklist,m_pixelsToProcess,fill);
    return;
  }

  if(sramCorrectionValid())
  {// sram correction
    cout << "fill histo with correction" << endl;
    fillDataHistoVec(trainData,pixelHistos,m_sramCorrectionData,m_pixelsToProcess,fill);
    return;
  }

  // fill raw data
  cout << "fill histo" << endl;
  fillDataHistoVec(trainData,pixelHistos,m_pixelsToProcess,fill);

}


void DsscTrainDataProcessor::fillDataHistoVec(const DsscTrainData *trainData, DataHistoVec & pixelHistos, const PixelVector & pixels, bool fill)
{
  //Timer timer("Fill Data Histos");

  const auto numPixels = pixels.size();

#pragma omp parallel for
  for(size_t pixelIdx=0; pixelIdx<numPixels; pixelIdx++){
    const auto imagePixel = pixels[pixelIdx];
    const auto pixelDataArr = trainData->getPixelDataPixelWise(imagePixel);
    pixelHistos[imagePixel].addToBuf(pixelDataArr.begin(),pixelDataArr.end());
  }

  if(fill){
    fillBufferToDataHistoVec(pixelHistos);
  }
}


void DsscTrainDataProcessor::fillDataHistoVec(const DsscTrainData *trainData, DataHistoVec & pixelHistos, const DsscSramBlacklist & sramBlacklist, const PixelVector & pixels, bool fill)
{
  //Timer timer("Fill Data Histos");

  const auto numPixels = pixels.size();

#pragma omp parallel for
  for(size_t pixelIdx=0; pixelIdx<numPixels; pixelIdx++){
    const auto imagePixel = pixels[pixelIdx];
    const auto & validPixelSrams = sramBlacklist.getValidSramAddresses(imagePixel);
    const auto pixelDataArr = trainData->getPixelDataPixelWise(imagePixel);
    pixelHistos[imagePixel].addToBuf(pixelDataArr.begin(),pixelDataArr.end(),validPixelSrams);
  }

  if(fill){
    fillBufferToDataHistoVec(pixelHistos);
  }
}


void DsscTrainDataProcessor::fillDataHistoVec(const DsscTrainData *trainData, DataHistoVec & pixelHistos, const std::vector<std::vector<float>>& sramCorrection, const PixelVector & pixels, bool fill)
{
  //Timer timer("Fill Data Histos");

  const auto numPixels = pixels.size();
  const auto numFrames = trainData->pulseCnt;

#pragma omp parallel for
  for(size_t pixelIdx=0; pixelIdx<numPixels; pixelIdx++){
    const auto imagePixel = pixels[pixelIdx];
    const auto & pxSramCorrection = sramCorrection[imagePixel];
    const auto pixelDataArr = trainData->getPixelDataPixelWise(imagePixel);
    for(uint16_t sram=0,corrValue=0; sram<numFrames; sram++){
      applySramCorrection(corrValue,pixelDataArr[sram],pxSramCorrection[sram]);
      pixelHistos[imagePixel].addToBuf(corrValue);
    }
  }

  if(fill){
    fillBufferToDataHistoVec(pixelHistos);
  }
}


void DsscTrainDataProcessor::fillDataHistoVec(const DsscTrainData *trainData, DataHistoVec & pixelHistos, const DsscSramBlacklist & sramBlacklist, const std::vector<std::vector<float>>& sramCorrection, const PixelVector & pixels, bool fill)
{
  //Timer timer("Fill Data Histos");

  const auto numPixels = pixels.size();
  const auto numFrames = trainData->pulseCnt;

#pragma omp parallel for
  for(size_t pixelIdx=0; pixelIdx<numPixels; pixelIdx++){
    const auto imagePixel = pixels[pixelIdx];
    const auto & validPixelSrams = sramBlacklist.getValidSramAddresses(imagePixel);
    const auto & pxSramCorrection = sramCorrection[imagePixel];
    const auto pixelDataArr = trainData->getPixelDataPixelWise(imagePixel);
    for(uint16_t sram=0,corrValue=0; sram<numFrames; sram++){
      if(validPixelSrams[sram]){
        applySramCorrection(corrValue,pixelDataArr[sram],pxSramCorrection[sram]);
        pixelHistos[imagePixel].addToBuf(corrValue);
      }
    }
  }

  if(fill){
    fillBufferToDataHistoVec(pixelHistos);
  }
}


// pixelwise is always 16 - 4096 - 800
void DsscTrainDataProcessor::sortPixelToPixelWise(const unsigned short* inDataPtr, unsigned short* outDataPtr, const std::vector<uint32_t> &availableAsics, int numFrames)
{
  const size_t numAsics = availableAsics.size();

  if(numAsics == s_numAsics && numFrames == s_numSram)
  {
    cout << "Warning: PixelData is copied from in to out data pointer, step can maybe avoided" << endl;
    std::copy(inDataPtr,inDataPtr+s_totalNumWords,outDataPtr);
  }
  else
  {
#pragma omp parallel for
    for(size_t asicIdx=0; asicIdx<numAsics; asicIdx++)
    {
      auto asicInDataPtr  = inDataPtr  + (               asicIdx  * s_numAsicPixels * s_numSram);
      auto asicOutDataPtr = outDataPtr + (availableAsics[asicIdx] * s_numAsicPixels * s_numSram);

      //data already sorted, just fill it
      if(numFrames == s_numSram){
        std::copy(asicInDataPtr,asicInDataPtr+s_numAsicData,asicOutDataPtr);
      }
      else
      {
        // output has always 800 frames, data must be shifted
        for(size_t pixelIdx=0; pixelIdx<s_numAsicPixels; pixelIdx++)
        {
          auto pixelInDataPtr  = asicInDataPtr  + (pixelIdx*s_numSram);
          auto pixelOutDataPtr = asicOutDataPtr + (pixelIdx*s_numSram);
          std::copy(pixelInDataPtr,pixelInDataPtr+numFrames,pixelOutDataPtr);
        }
      }
    }
  }
}


// pixelwise is always 16 - 4096 - 800
void DsscTrainDataProcessor::sortAsicToPixelWise(const unsigned short* inDataPtr, unsigned short* outDataPtr, const std::vector<uint32_t> &availableAsics, int numFrames)
{
  const size_t numAsics  = availableAsics.size();
  const size_t numPixels = numAsics * s_numAsicPixels;

#pragma omp parallel for
  for(size_t asicIdx=0; asicIdx<numAsics; asicIdx++)
  {
    auto asicInDataPtr  = inDataPtr  + (               asicIdx  * s_numAsicPixels);
    auto asicOutDataPtr = outDataPtr + (availableAsics[asicIdx] * s_numAsicPixels * s_numSram);

    for(size_t pixelIdx=0; pixelIdx<s_numAsicPixels; pixelIdx++){

      auto pixelInDataPtr  = asicInDataPtr + pixelIdx;
      auto pixelOutDataPtr = asicOutDataPtr  + pixelIdx*s_numSram;
      for(int frame=0; frame<numFrames; frame++){
        pixelOutDataPtr[frame] = pixelInDataPtr[frame*numPixels];
      }
    }
  }
}

// pixelwise is always 16 - 4096 - 800
void DsscTrainDataProcessor::sortImageToPixelWise(const unsigned short* inDataPtr, unsigned short* outDataPtr, const std::vector<uint32_t> & availableAsics, size_t numFrames, bool removeNotSending)
{
  const size_t numAsics = availableAsics.size();

  std::vector<uint32_t> outputAsics;
  const std::vector<uint32_t> sortMapAsics = availableAsics;
  if(removeNotSending){
    for(size_t idx=0; idx<numAsics; idx++){
      outputAsics.push_back(idx);
    }
  }else{
    outputAsics = availableAsics;
  }

  //#pragma omp parallel for
  for(size_t asicIdx=0; asicIdx<numAsics; asicIdx++)
  {
    const auto * asicSortMap  = s_imagePixelMap.data() + (sortMapAsics[asicIdx] * s_numAsicPixels);
    uint16_t * asicOutDataPtr = outDataPtr   + (outputAsics[asicIdx]  * s_numAsicPixels * s_numSram);

    for(size_t pixelIdx=0; pixelIdx<s_numAsicPixels; pixelIdx++)
    {
      const uint16_t * pixelInDataPtr = inDataPtr + asicSortMap[pixelIdx];
      uint16_t * pixelOutDataPtr      = asicOutDataPtr + pixelIdx*s_numSram;

      for(size_t frame=0; frame<numFrames; frame++){
        const size_t offs = frame*s_totalNumPxs;
        const uint16_t & value = pixelInDataPtr[offs];
        pixelOutDataPtr[frame] = value;
        if(pixelOutDataPtr[frame]>512){
          const uint16_t *inErrorPtr = &pixelInDataPtr[offs];
          uint16_t *outErrorPtr = pixelOutDataPtr + frame;
          const uint16_t inValue = inErrorPtr[0];
          const uint16_t outValue =outErrorPtr[0];
          std::vector<uint16_t> testVector(pixelInDataPtr+(offs)-10,pixelInDataPtr+(offs)+10);

          cout << inValue << "  AAAARGGGHHH  " << outValue << " " << testVector.size() << endl;
        }
      }
    }
  }
}


PixelVector DsscTrainDataProcessor::getPixelsToProcess() const
{
  // sort asic wise, on pixel data, pixels are also sorted asic wise
  PixelVector pixels;
  for(auto && asic : m_availableAsics){
    const auto asicPixelsMapBegin = s_imagePixelMap.data() + (asic * s_numAsicPixels);
    const auto asicPixelsMapEnd   = asicPixelsMapBegin + s_numAsicPixels;
    pixels.insert(pixels.end(),asicPixelsMapBegin,asicPixelsMapEnd);
  }
  return pixels;
}


std::vector<double> DsscTrainDataProcessor::computeMeanSramRMSMatrix(const std::vector<std::vector<float>> & meanSramMatrixFloat)
{
  const auto numPixels = meanSramMatrixFloat.size();
  std::vector<std::vector<double>>meanSramMatrix(s_totalNumPxs,std::vector<double>(s_numSram));

#pragma omp parallel for
  for(size_t pxID = 0; pxID < numPixels; pxID++){
    std::copy(meanSramMatrixFloat[pxID].begin(),meanSramMatrixFloat[pxID].end(),meanSramMatrix[pxID].begin());
  }
  return DsscTrainDataProcessor::computeMeanSramRMSMatrix(meanSramMatrix);
}


std::vector<double> DsscTrainDataProcessor::computeMeanSramRMSMatrix(const std::vector<std::vector<double>> & meanSramMatrix)
{
  const size_t numPixels = meanSramMatrix.size();
  if(numPixels==0) return std::vector<double>();

  std::vector<double> sramRmsValues(numPixels);

#pragma omp parallel for
  for(size_t pxID = 0; pxID < numPixels; pxID++){
    const auto & dataVec = meanSramMatrix[pxID];
    sramRmsValues[pxID] = utils::getMeandAndRMS(dataVec).rms;
  }
  return sramRmsValues;
}


std::vector<double> DsscTrainDataProcessor::computeSramSlopeVectorFromMeanSramMatrix(const std::vector<std::vector<float>> & meanSramMatrixFloat)
{
  const auto numPixels = meanSramMatrixFloat.size();
  std::vector<std::vector<double>>meanSramMatrix(s_totalNumPxs,std::vector<double>(s_numSram));

#pragma omp parallel for
  for(size_t pxID = 0; pxID < numPixels; pxID++){
    std::copy(meanSramMatrixFloat[pxID].begin(),meanSramMatrixFloat[pxID].end(),meanSramMatrix[pxID].begin());
  }
  return computeSramSlopeVectorFromMeanSramMatrix(meanSramMatrix);
}


std::vector<double> DsscTrainDataProcessor::computeSramSlopeVectorFromMeanSramMatrix(const std::vector<std::vector<double>> & meanSramMatrix)
{
  const size_t numPixels = meanSramMatrix.size();
  const size_t numSram = meanSramMatrix.front().size();

  if(numPixels==0) return std::vector<double>();

  std::vector<double> sramSlopes(numPixels);
  std::vector<double> XVALUES(numSram);
  for(size_t i=0; i<numSram; i++){
    XVALUES[i] = i;
  }
  const double S_X  = std::accumulate(XVALUES.begin(), XVALUES.end(), 0.0);
  const double S_XX = std::inner_product(XVALUES.begin(), XVALUES.end(), XVALUES.begin(), 0.0);

#pragma omp parallel for
  for(size_t pxID = 0; pxID < numPixels; pxID++)
  {
    auto & dataVec = meanSramMatrix[pxID];
    const double slope  = linearRegression(S_X,S_XX,XVALUES,dataVec);
    sramSlopes[pxID] = slope;
  }
  return sramSlopes;
}

}
