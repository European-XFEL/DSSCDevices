#include "DsscSramBlacklist.h"
#include "DataHisto.h"

using namespace std;

namespace utils{

DsscSramBlacklist::DsscSramBlacklist()
 : m_sramBlacklistValid(false),
   m_sramAddrBlacklist(s_totalNumPxs),
   m_validPxSramAddresses(s_totalNumPxs,std::vector<bool>(s_numSram,true))
{
}


DsscSramBlacklist::DsscSramBlacklist(const std::string & fileName)
{
  initFromFile(fileName);
}

DsscSramBlacklist::DsscSramBlacklist(const std::vector<double> &meanSramContent)
 : DsscSramBlacklist()
{
  generateSramBlacklist(meanSramContent);
}


void DsscSramBlacklist::saveToFile(const std::string & fileName) const
{
  utils::savePxSramBlacklist(fileName,utils::getUpCountingVector(m_sramAddrBlacklist.size()),m_sramAddrBlacklist);
}


bool DsscSramBlacklist::isBadSramCellsFile(const std::string & fileName) const
{
  ifstream in(fileName);
  if(!in.is_open()){
    cout << "Could not open BadSramCells file: " << fileName << "\n";
    return false;
  }

  std::string line;
  while (getline(in, line)) {
    if(line.empty()) continue;
    if(line.find("BadSramCells") != std::string::npos){
      return true;
    }
    return false;
  }

  return false;
}


void DsscSramBlacklist::initFromFile(const std::string & fileName, bool add)
{
  m_sramBlacklistValid = false;
  if(utils::checkFileExists(fileName))
  {
    if(isBadSramCellsFile(fileName)){
      importBadSramCellsMap(fileName);
    }else{
      importFromBlacklistFile(fileName,add);
    }
  }
  else
  {
    cout << "SramBlacklist file not found: " << fileName << endl;
  }

  if(!m_sramBlacklistValid){
    initVectors();
  }
}

void DsscSramBlacklist::importFromBlacklistFile(const std::string & fileName, bool add)
{
  m_sramAddrBlacklist = utils::importPxSramBlacklist(fileName);
  if(!m_sramAddrBlacklist.empty())
  {
    //initialize and overwrite old values
    if(!add){
      m_validPxSramAddresses.assign(m_sramAddrBlacklist.size(),std::vector<bool>(s_numSram,true));
    }

    fillValidSramAddressesFromSramBlacklist();

    cout << "PxSram Blacklist Loaded from " << fileName << endl;

    m_sramBlacklistValid = true;
  }else{
    cout << "Sram Blacklist is empty: " << fileName << endl;
  }
}

void DsscSramBlacklist::fillValidSramAddressesFromSramBlacklist()
{
  int px = 0;
  for(auto && blacklist : m_sramAddrBlacklist){
    auto & pxAddresses = m_validPxSramAddresses[px++];
    for(auto && badSram : blacklist){
      pxAddresses[badSram] = false;
    }
  }
}


void DsscSramBlacklist::clear()
{
  m_validPxSramAddresses.clear();
  m_sramAddrBlacklist.clear();
  m_sramBlacklistValid = false;
}


void DsscSramBlacklist::initVectors()
{
  m_sramAddrBlacklist = std::vector<std::vector<uint32_t>>(s_totalNumPxs,std::vector<uint32_t>());
  m_validPxSramAddresses = std::vector<std::vector<bool>>(s_totalNumPxs,std::vector<bool>(s_numSram,true));
}


void DsscSramBlacklist::initForPixelInjection(uint32_t offset, uint32_t addrSkipCnt, bool signalNotBaseline)
{
  m_validPxSramAddresses = std::vector<std::vector<bool>>(s_totalNumPxs,std::vector<bool>(s_numSram,!signalNotBaseline));

#pragma omp parallel for
  for(size_t px=0; px < s_totalNumPxs; px++)
  {
    auto & pxSramAddresses = m_validPxSramAddresses[px];
    for(size_t sram=offset; sram<s_numSram; sram += addrSkipCnt){
      pxSramAddresses[sram] = signalNotBaseline;
    }
  }

  // for baseline exclude also the next after signal address
  if(!signalNotBaseline){
#pragma omp parallel for
    for(size_t px=0; px < s_totalNumPxs; px++)
    {
      auto & pxSramAddresses = m_validPxSramAddresses[px];
      for(size_t sram=offset+1; sram<s_numSram; sram += addrSkipCnt){
        pxSramAddresses[sram] = false;
      }
    }
  }

  m_sramAddrBlacklist = std::vector<std::vector<uint32_t>>(s_totalNumPxs);
#pragma omp parallel for
  for(size_t px=0; px<s_totalNumPxs; px++)
  {
    auto & pxSramAddresses = m_validPxSramAddresses[px];
    auto & pxBadSramAddresses = m_sramAddrBlacklist[px];
    int sram=0;
    for(auto && goodAddr : pxSramAddresses){
      if(!goodAddr){
        pxBadSramAddresses.push_back(sram);
      }
      sram++;
    }
  }

  m_sramBlacklistValid = true;
}


void DsscSramBlacklist::generateSramBlacklist(const std::vector<double> &meanSramContent)
{
  static constexpr double SIGMA = 3.0;

  cout << "Compute Sram Outliers" << endl;
  const uint numPixels = meanSramContent.size()/s_numSram;

  if(numPixels != s_totalNumPxs){
    cout << "WARNING meanSramContent has bad size, total number of pixel must be 65536" << endl;
    return;
  }

  static const std::vector<double> XVALUES = getUpCountingVector<double>(s_numSram);
  static const double S_X  = std::accumulate(XVALUES.begin(), XVALUES.end(), 0.0);
  static const double S_XX = std::inner_product(XVALUES.begin(), XVALUES.end(), XVALUES.begin(), 0.0);

  m_sramAddrBlacklist.resize(s_totalNumPxs);

#pragma omp parallel for
  for(size_t px=0; px<s_totalNumPxs;px++)
  {
    auto * pixelMeanSramsIn = meanSramContent.data() + px * s_numSram;

    //### always run advanced outlier removal
    //### experimental Sram outliers improvement,
    // if large fraction of srams, bad extreme values are eliminated
    // if rms > 10 sram errors sram errors occured, can be found since they have normally high values
    // find most occuring value and remove all values with more than 5 distance, is valid for reasonable sram slopes
    std::vector<double> pixelMeanSramVec(pixelMeanSramsIn,pixelMeanSramsIn+s_numSram);
    const auto stats = utils::getMeandAndRMS(pixelMeanSramVec);

    double slope;
    double maxLimit;
    double minLimit;

    if(stats.rms < 3.0)
    {
      slope = utils::linearRegression(S_X,S_XX,XVALUES,pixelMeanSramVec);
      // 1. correct for slope and
      // 2. then use the mean value
      for(size_t sram = 0; sram<s_numSram; sram++){
        pixelMeanSramVec[sram] -= sram*slope;
      }
      const auto checkStats = utils::getMeandAndRMS(pixelMeanSramVec);

      maxLimit = checkStats.mean + checkStats.rms*SIGMA;
      minLimit = checkStats.mean - checkStats.rms*SIGMA;
    }
    else
    {
      // 1. find worst outliers,
      // 2. then correct for slope and
      // 3. then use the mean value
      uint16_t checkMeanValue = 0;
      {
        DataHisto histo;
        for(size_t sram = 0; sram<s_numSram; sram++){
          histo.addToBuf(round(std::min(511.0,std::max(0.0,pixelMeanSramVec[sram]))));
        }
        histo.fillBufferToHistoMap();
        checkMeanValue = histo.getMaxBin();
      }

      double checkMaxLimit = checkMeanValue + 5.0;
      double checkMinLimit = checkMeanValue - 5.0;
      std::vector<double> goodValues;
      std::vector<double> goodAddresses;
      for(uint sram= 0; sram<s_numSram; sram++){
        const uint16_t value = pixelMeanSramVec[sram];
        if(value > checkMinLimit && value < checkMaxLimit){
          goodValues.push_back(value);
          goodAddresses.push_back(sram);
        }
      }

      // correct for sram slope for improved outlier detection
      slope = utils::linearRegression(goodAddresses,goodValues);

      for(size_t idx = 0; idx<goodValues.size(); idx++){
        goodValues[idx] -= goodAddresses[idx]*slope;
      }

      const auto checkStats = utils::getMeandAndRMS(goodValues.data(),goodValues.size());
      maxLimit = checkStats.mean + checkStats.rms*SIGMA;
      minLimit = checkStats.mean - checkStats.rms*SIGMA;

      for(size_t sram = 0; sram<s_numSram; sram++){
        pixelMeanSramVec[sram] -= sram*slope;
      }
    }

    { // fill sram outliers, added to existing
      auto & pxOutlierSrams = m_sramAddrBlacklist[px];

      for(size_t sram = 0; sram<s_numSram; sram++){
        double meanSram = pixelMeanSramVec[sram];
        if(meanSram > maxLimit || meanSram < minLimit){
          pxOutlierSrams.push_back(sram);
        }
      }

      // sram blacklist needs not to be empty
      utils::removeDuplicates(pxOutlierSrams);
    }
  }

  m_validPxSramAddresses.assign(m_sramAddrBlacklist.size(),std::vector<bool>(s_numSram,true));

  fillValidSramAddressesFromSramBlacklist();

  m_sramBlacklistValid = true;
}

// load sram error list from matrix sram test
void DsscSramBlacklist::importBadSramCellsMap(const std::string & fileName)
{
  ifstream in(fileName);
  if(!in.is_open()){
    cout << "Could not open BadSramCells file: " << fileName << "\n";
    return;
  }

  m_sramAddrBlacklist.resize(s_totalNumPxs);

  cout << "Load Sram Blacklist from: " << fileName << endl;

  std::string line;
  while (getline(in, line)) {
    if(line.empty()) continue;
    if(line[0] == '#') continue;

    //Read SRAM Blacklist for each pixel, pixels may remain empty
    std::vector<std::string> itemsList;
    utils::split(line,' ',itemsList);

    if(itemsList.size() < 7) continue;

    std::string &asicStr   = itemsList[3];
    std::string & pixelStr = itemsList[4];
    std::string & addrStr  = itemsList[6];

    auto pos = asicStr.find("pixel");
    int asic = stoi(asicStr.substr(0,pos));

    pos = pixelStr.find(",");
    int pixel = stoi(pixelStr.substr(0,pos));

    pos = addrStr.find(".");
    int addr = stoi(addrStr.substr(0,pos));

    uint32_t imagePixel = calcImagePixel(asic,pixel);
    m_sramAddrBlacklist[imagePixel].push_back(addr);
  }

  m_validPxSramAddresses.assign(m_sramAddrBlacklist.size(),std::vector<bool>(s_numSram,true));

  fillValidSramAddressesFromSramBlacklist();

  m_sramBlacklistValid = true;
}

} //utils
