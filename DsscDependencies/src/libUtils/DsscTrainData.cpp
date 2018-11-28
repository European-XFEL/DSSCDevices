#include "DsscTrainData.hh"

using namespace std;

namespace utils{

bool DsscTrainData::s_en8BitMode = false;
uint16_t DsscTrainData::s_expectedTestpattern = 5;

vector<uint32_t> & DsscTrainData::accumulateFrameData(uint32_t minSram, uint32_t maxSram)
{
  static vector<uint32_t> accumulateVector;
  const size_t numAsics = availableASICs.size();
  const size_t numPixels = numAsics * s_numAsicPixels;

  minSram = std::min(minSram,pulseCnt-1);
  maxSram = std::min(maxSram,pulseCnt-1);

  accumulateVector.resize(numPixels);
  std::fill(accumulateVector.begin(),accumulateVector.end(),0);

  if(isPixelWise())
  {
    const size_t numSram = maxSram - minSram + 1;

#pragma omp parallel for
    for(size_t px=0; px<numPixels; px++){
      auto pixelOffset = this->data() + px*pulseCnt + minSram;
      accumulateVector[px] = std::accumulate(pixelOffset,pixelOffset+numSram,(uint32_t)0);
    }
  }
  else
  {
#pragma omp parallel for
    for(size_t asic=0; asic < numAsics; asic++){
      auto asicAccVector = accumulateVector.data() + asic*s_numAsicPixels;
      auto asicDataPtr   = this->data() + asic*s_numAsicPixels;
      for(size_t px=0; px<s_numAsicPixels; px++){
        auto pixelDataPtr = asicDataPtr + px;
        for(size_t frame = minSram; frame < maxSram; frame++){
          asicAccVector[px] += pixelDataPtr[frame*numPixels];
        }
      }
    }
  }

  return accumulateVector;
}


uint DsscTrainData::checkTestPatternData(uint16_t expectedTestPattern) const
{
  uint errorCnt = 0;
#pragma omp parallel for
  for(uint frame=0; frame<pulseCnt; frame++){
    auto imageDataArray = getImageDataArray(frame);
    for(auto && value : imageDataArray){
      if(value != expectedTestPattern){
#pragma omp critical
        errorCnt++;
      }
    }
  }
  return errorCnt;
}

void DsscTrainData::setInvalid()
{
  pptData.clear();
  asicTrailerData.clear();
  pulseCnt = 0;
  trainId = RECV_ERROR_TIMEOUT;
  valid = false;
}

void DsscTrainData::setValid()
{
  setPulseCnt();
  setTrainId();
  fillAsicTrailerVec();
  valid = true;
}

void DsscTrainData::readFormat()
{
  const auto format = (getSpecificData().sort_asic_wise)? DATAFORMAT::ASIC : DATAFORMAT::IMAGE;
  setFormat(format);
  //cout << "DsscTrainData: self detected Format = " << getFormatStr() << endl;
}

void DsscTrainData::init(DsscTrainData * other)
{
  availableASICs = other->availableASICs;
  pulseCnt = other->pulseCnt;
  trainId = other->trainId;
  valid = false;
}

void DsscTrainData::fillAsicTrailerVec()
{
  if(asicTrailerData.empty()){
    asicTrailerData.resize(s_numASICTrailerBytes,0);
    uint8_t * data   = (uint8_t * )this->data();
    uint8_t * inData = data + utils::getASICTrailerStartByte(pulseCnt);
    utils::fillIdVector<uint8_t,uint8_t>(inData,asicTrailerData);
  }
  getDsscAsicTrailerArray();
}


void DsscTrainData::copyMeta(DsscTrainData * other)
{
  auto * thisData = this->data();
  const auto * otherData = other->data();

  // copy header data
  std::copy(otherData,otherData+c_HEADER_OFFS,thisData);

  const bool otherPixelwise = other->getFormat() == DATAFORMAT::PIXEL;
  const bool thisPixelWise  =  this->getFormat() == DATAFORMAT::PIXEL;

  // copy descriptor and trailer data array
  const size_t trailerOffset = utils::getCellIdsStartByte(other->pulseCnt,otherPixelwise)/2;
  const size_t trailerLength = utils::totalBytesToReceive(other->pulseCnt,otherPixelwise)/2 - trailerOffset;

  const auto * otherTrailerData = other->data() + trailerOffset;

  const size_t thisTrailerOffset = utils::getCellIdsStartByte(other->pulseCnt,thisPixelWise)/2;
  auto * thisTrailerData = this->data() + thisTrailerOffset;

  std::copy(otherTrailerData,otherTrailerData+trailerLength,thisTrailerData);
}


void DsscTrainData::fillMeta()
{
  fillHeader();
  fillDescriptors();
  getDsscAsicTrailerArray();
}


void DsscTrainData::fillHeader()
{
  const uint8_t * headerData = (const uint8_t*)this->data();
//  uint64_t h_magicNumber             = *((uint64_t *) &headerData[ 0]);
//  uint32_t h_majorTrainFormatVersion = *((uint32_t *) &headerData[ 8]);
//  uint32_t h_minorTrainFormatVersion = *((uint32_t *) &headerData[12]);
  trainId           = *((uint64_t *) &headerData[16]);
  dataId            = *((uint64_t *) &headerData[24]);
  detLinkId         = *((uint32_t *) &headerData[32]);
  tbLinkId          = *((uint32_t *) &headerData[36]);
  pulseCnt          = *((uint64_t *) &headerData[40]);
  detSpecificLength = *((uint32_t *) &headerData[48]);
  tbSpecificLength  = *((uint32_t *) &headerData[52]);
}


void DsscTrainData::fillDescriptors()
{
  cellIds.resize(pulseCnt,0);
  pulseIds.resize(pulseCnt,0);
  pptData.resize(8,0);
  sibData.resize(72,0);
  asicTrailerData.resize(s_numASICTrailerBytes,0);

  bool isPixelWise = getFormat() == DATAFORMAT::PIXEL;

  uint8_t * data = (uint8_t * )this->data();
  uint8_t * inData;
  inData = data + utils::getCellIdsStartByte(pulseCnt,isPixelWise);
  utils::fillIdVector<uint16_t,uint16_t>(inData,cellIds);

  inData = data + utils::getPulseIdsStartByte(pulseCnt,isPixelWise);
  utils::fillIdVector<uint64_t,uint64_t>(inData,pulseIds);

  inData = data + utils::getSpecificStartByte(pulseCnt,isPixelWise);
  utils::fillIdVector<uint16_t,uint16_t>(inData,pptData);

  inData = data + utils::getSpecificStartByte(pulseCnt+(pptData.size()*2),isPixelWise); // 16 Bytes PPT Data
  utils::fillIdVector<uint16_t,uint16_t>(inData,sibData);

  inData = data + utils::getASICTrailerStartByte(pulseCnt,isPixelWise);
  utils::fillIdVector<uint8_t,uint8_t>(inData,asicTrailerData);
}


const DsscAsicTrailerDataArr& DsscTrainData::getDsscAsicTrailerArray()
{
  for(int i=0; i<16; i++){
    m_asicTrailerDataArray[i] = DsscAsicTrailerData(asicTrailerData,i);
  }
  return m_asicTrailerDataArray;
}


const DsscAsicTrailerData & DsscTrainData::getAsicTrailerData() const
{
  return m_asicTrailerDataArray[availableASICs.front()];
}


const DsscAsicTrailerData& DsscTrainData::getAsicTrailerData(int asic) const
{
  return m_asicTrailerDataArray[asic];
}

DsscSpecificData DsscTrainData::getSpecificData()
{
  if(isValid()){
    if(pptData.empty()){
      pptData.resize(8,0);
      const uint8_t * data8 = (const uint8_t * )this->data();
      const auto * inData = data8 + utils::getSpecificStartByte(pulseCnt);
      utils::fillIdVector<uint16_t,uint16_t>(inData,pptData);
    }
  }

  return (((const DsscTrainData*)this)->getSpecificData());
}

DsscSpecificData DsscTrainData::getSpecificData() const
{
  DsscSpecificData specific;
  if(isValid() && (pptData.size() == 8)){
    specific.pptVetoCnt      =  pptData[0];
    specific.numPreBursVetos =  pptData[1];
    specific.userSpecific1   =  pptData[2];
    specific.userSpecific2   =  pptData[3];
    specific.userSpecific3   =  pptData[4];
    specific.moduleNr        =  pptData[5] & 0xF;
    specific.iobSerial       = (pptData[7] << 16) + pptData[6];

    uint8_t data_flags = (pptData[5]>>8) & 0xF;
    int idx = 0;
    specific.sort_asic_wise     = (data_flags & (1<<idx)) != 0; idx++;
    specific.rotate_ladder      = (data_flags & (1<<idx)) != 0; idx++;
    specific.send_dummy_dr_data = (data_flags & (1<<idx)) != 0; idx++;
    specific.send_raw_data      = (data_flags & (1<<idx)) != 0; idx++;
    specific.send_conv_data     = (data_flags & (1<<idx)) != 0; idx++;
    specific.send_reord_data    = (data_flags & (1<<idx)) != 0; idx++;
    specific.single_ddr3_block  = (data_flags & (1<<idx)) != 0; idx++;
    specific.clone_eth0_to_eth1 = (data_flags & (1<<idx)) != 0; idx++;

  }else{
    specific.pptVetoCnt      = 0xFFFF;
  }

  //specific.print();

  return specific;
}


THeader DsscTrainData::getTHeader() const
{
  THeader header;
  if(isValid()){
    const uint8_t * inData = ((uint8_t*)data());
    header.magicNumber             = *((uint64_t*)inData);
    header.majorTrainFormatVersion = *((uint32_t*)(inData+8));
    header.minorTrainFormatVersion = *((uint32_t*)(inData+12));
    header.trainID                 = trainId;
    header.dataID                  = dataId;
    header.tbLinkID                = tbLinkId;
    header.detLinkID               = detLinkId;
    header.pulseCnt                = pulseCnt;
    header.detSpecificLength       = detSpecificLength;
    header.tbSpecificLength        = tbSpecificLength;
  }else{
    header.magicNumber = 0xFFFFFFFFFFFFFFFF;
    header.trainID     = 0xFFFFFFFFFFFFFFFF;
  }
  return header;
}


TTrailer DsscTrainData::getTTrailer() const
{
  TTrailer trailer;
  if(isValid()){
    bool isPixelWise = (getFormat() == DATAFORMAT::PIXEL);
    const size_t startByte = utils::getTrainTrailerStartByte(pulseCnt,isPixelWise);
    cout << "PulseCnt = " << pulseCnt << " : " << startByte << " + Last Packet Size = "  << (startByte%8184+40+42) << endl;

    std::vector<uint8_t> test(8184);
    size_t lastPacket = startByte / 8184;
    lastPacket *= 8184;
    const uint8_t *testData =  ((uint8_t*)data()) + lastPacket;
    std::copy(testData,testData+8184,test.begin());

    const uint8_t * inData = ((uint8_t*)data()) + startByte;

    trailer.checkSum0   = *((uint64_t*)inData);
    trailer.checkSum1   = *((uint64_t*)(inData+8));
    trailer.status      = *((uint64_t*)(inData+16));
    trailer.magicNumber = *((uint64_t*)(inData+24));
    trailer.ttp         = *((uint64_t*)(inData+32));

  }else{
    trailer.magicNumber = 0xFFFFFFFFFFFFFFFF;
  }
  return trailer;
}


uint16_t DsscTrainData::getTempADCValue(int asic) const
{
  return m_asicTrailerDataArray[asic].m_temp0;
}


uint16_t DsscTrainData::getTestPattern(int asic) const
{
  return m_asicTrailerDataArray[asic].m_testPattern;
}


void DsscTrainData::setPulseCnt()
{
  const uint8_t * headerData = (const uint8_t*)this->data();
  pulseCnt = *((uint64_t *) &headerData[40]);
}

void DsscTrainData::setTrainId()
{
  const uint8_t * headerData = (const uint8_t*)this->data();
  trainId = *((uint64_t *) &headerData[16]);
}

string DsscTrainData::getFormatStr() const
{
  switch(m_format){
    case DATAFORMAT::PIXEL : return "pixelwise";
    case DATAFORMAT::ASIC  : return "asicwise";
    case DATAFORMAT::IMAGE : return "imagewise";
  }
  return "pixelwise";
}

DsscTrainData::DATAFORMAT DsscTrainData::getFormat(const string & formatStr)
{
  if(formatStr == "pixelwise"){
    return DATAFORMAT::PIXEL;
  }else if(formatStr == "asicwise"){
    return DATAFORMAT::ASIC;
  }else if(formatStr == "imagewise"){
    return DATAFORMAT::IMAGE;
  }else{
    return DATAFORMAT::PIXEL;
  }
}


DsscTrainData::RECVSTATUS DsscTrainData::getRecvStatus() const
{
  if(!isValid()){
    return RECVSTATUS::TIMEOUT;
  }
  if(getTestPattern(0) != s_expectedTestpattern){
    return RECVSTATUS::TESTPATTERN;
  }
  return RECVSTATUS::OK;
}


std::string DsscTrainData::getRecvStatusStr() const
{
  if(!isValid()){
    return "SINGECYCLE ERROR: Data Receiver timeout";
  }
  if(getTestPattern(0) != s_expectedTestpattern){
    return "SINGECYCLE ERROR: TestPattern was wrong in train" + to_string(trainId);
  }
  return "SINGECYCLE INFO: Received train with train id " + to_string(trainId) + " successfully";
}

size_t getNumPacketsToReceive(int numFramesToSend)
{
  static constexpr int PAYLOAD = 8184;
  //  16 Packets per image.
  // 256 16bit words per 8K UDP Packet
  // 8KB Per Image per ASIC
  uint32_t numBytes  = totalBytesToReceive(numFramesToSend);
  uint32_t numPackets = numBytes/PAYLOAD;
  if( (numBytes%PAYLOAD) != 0){
    numPackets++;
  }

  return numPackets;
}

size_t totalBytesToReceive(int numFramesToSend, bool pixelWise)
{
  return numBytesToReceive(numFramesToSend,TOTAL,pixelWise);
}


size_t numBytesToReceive(int numFramesToSend, int num, bool pixelWise)
{
  const size_t numTrainDataBytes = (pixelWise)? 800*2*16*4096 : numFramesToSend*2*16*4096;

  const size_t numDescr0Bytes = ((numFramesToSend-1)*2/32+1)*32;
  const size_t numDescr1Bytes = ((numFramesToSend-1)*8/32+1)*32;
  const size_t numDescr2Bytes = ((numFramesToSend-1)*2/32+1)*32;
  const size_t numDescr3Bytes = ((numFramesToSend-1)*4/32+1)*32;

  vector<size_t> offsetVec(10);

  int i=0;
  offsetVec[i++] = s_numTrainHeaderBytes; // num = 0
  offsetVec[i++] = numTrainDataBytes;     // num = 1
  offsetVec[i++] = numDescr0Bytes;        // num = 2
  offsetVec[i++] = numDescr1Bytes;        // num = 3
  offsetVec[i++] = numDescr2Bytes;        // num = 4
  offsetVec[i++] = numDescr3Bytes;        // num = 5
  offsetVec[i++] = s_numSpecificBytes;    // num = 6
  offsetVec[i++] = s_numASICTrailerBytes; // num = 7
  offsetVec[i++] = s_numTrainTrailerBytes;// num = 8
  offsetVec[i++] = s_numTTPBytes;         // num = 9

  size_t sum = 0;
  for(int i=0; i<=num; i++){
   sum += offsetVec[i];
  }
  return sum;
}

size_t getCellIdsStartByte(int numFramesToSend, bool pixelWise){
  return numBytesToReceive(numFramesToSend,TDATA,pixelWise); // all data incl TDATA
}

size_t getPulseIdsStartByte(int numFramesToSend, bool pixelWise){
  return numBytesToReceive(numFramesToSend,CELLID,pixelWise); // all data incl CELLID
}

size_t getStatusStartByte(int numFramesToSend, bool pixelWise){
  return numBytesToReceive(numFramesToSend,PULSEID,pixelWise);
}

size_t getSpecificStartByte(int numFramesToSend, bool pixelWise){
  return numBytesToReceive(numFramesToSend,LEN,pixelWise);
}

size_t getASICTrailerStartByte(int numFramesToSend, bool pixelWise){
  return numBytesToReceive(numFramesToSend,SPECIFIC,pixelWise);
}

size_t getTrainTrailerStartByte(int numFramesToSend, bool pixelWise){
  return numBytesToReceive(numFramesToSend,ASIC,pixelWise); // all data incl ASIC
}

}

