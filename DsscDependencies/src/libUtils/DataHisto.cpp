#include "DataHisto.h"
#include "Minuit2/MnUserParameters.h"
#include "Minuit2/FunctionMinimum.h"
#include "FitUtils.h"

using namespace std;
namespace utils
{

bool DataHisto::enYNormalization = true;

DataHisto::DNLValuesMap DataHisto::DNLVALUESEMPTYMAP;

DataHisto::DataHisto()
  : count(0)
{
  std::fill(binValsBuffer,binValsBuffer+BUFS,0);

  if(DNLVALUESEMPTYMAP.empty()){
    for(int i=0; i<512; i++){
      DNLVALUESEMPTYMAP[i] = 0.0;
    }
  }
  histoValues.reserve(512);
}


DataHisto::DataHisto(const vector<uint16_t> & newValues)
   : count(0)
{
  std::fill(binValsBuffer,binValsBuffer+BUFS,0);

  histoValues.reserve(newValues.size());
  add(newValues.begin(),newValues.end());
}

DataHisto::DataHisto(const uint16_t *newValues, int numValues)
   : count(0)
{
  std::fill(binValsBuffer,binValsBuffer+BUFS,0);

  histoValues.reserve(numValues);
  add(newValues,newValues+numValues);
}


void DataHisto::operator+=(const DataHisto & histo)
{
  count += histo.count;
  for(const auto & histoBin : histo.histoValues){
    addN(histoBin.first,histoBin.second);
  }
}


void DataHisto::addToBuf(const uint16_t *value, uint32_t numValues, const std::vector<bool> & validSram)
{
  for(size_t i=0; i<numValues; i++){
    if(validSram[i]){
      binValsBuffer[value[i]]++;
    }
  }
}


void DataHisto::addN(uint16_t value, uint32_t num)
{
  histoValues[value] += num;
  count+=num;
}

void DataHisto::add(uint16_t value)
{
  histoValues[value]++;
  count++;
}


std::pair<int,int> DataHisto::getBinRange() const
{
  const auto bins = getBins();
  return {bins.front(),bins.back()};
}


vector<uint16_t> DataHisto::getBins() const
{
  vector<uint16_t> bins;
  for(auto && bin : histoValues){
    bins.push_back(bin.first);
  }
  sort(bins.begin(),bins.end());
  return bins;
}


std::vector<uint32_t> DataHisto::getBinValues() const
{
  const vector<uint16_t> bins = getBins();
  vector<uint32_t> values(bins.size());

  int idx = 0;
  for(auto && bin : bins){
    values[idx++] = getBinContent(bin);
  }
  return values;
}


vector<uint16_t> DataHisto::getBinsFilled() const
{
  const vector<uint16_t> bins = getBins();
  const auto mmElem = std::minmax_element(bins.begin(),bins.end());
  uint16_t min = *mmElem.first - 3;
  uint16_t max = *mmElem.second + 3;

  vector<uint16_t> filledBins(max-min+1,0);
  for(int idx=min; idx <= max; idx++){
    filledBins[idx-min] = idx;
  }
  return filledBins;
}


vector<uint32_t> DataHisto::getBinValuesFilled() const
{
  vector<uint16_t> bins = getBinsFilled();
  vector<uint32_t> values(bins.size());

  int idx = 0;
  for(const auto & bin : bins){
    values[idx++] = getBinContent(bin);
  }
  return values;
}


vector<double> DataHisto::getBinsDouble() const
{
  vector<double> bins;
  for(auto && bin : histoValues){
    bins.push_back(bin.first-0.5);
  }
  sort(bins.begin(),bins.end());
  return bins;
}


std::vector<double> DataHisto::getBinValuesDouble() const
{
  const vector<uint16_t> bins = getBins();
  vector<double> values(bins.size());

  int idx = 0;
  for(auto && bin : bins){
    values[idx++] = getBinContent(bin);
  }
  return values;
}


vector<double> DataHisto::getBinsDoubleFilled() const
{
  const auto bins = getBins();
  const auto mmElem = std::minmax_element(bins.begin(),bins.end());
  uint16_t min = *mmElem.first - 3;
  uint16_t max = *mmElem.second + 3;

  vector<double> filledBins(max-min+1);
  for(int idx=min; idx <= max; idx++){
    filledBins[idx-min] = idx-0.5;
  }
  return filledBins;
}


std::vector<double> DataHisto::getBinValuesDoubleFilled() const
{
  vector<uint16_t> bins = getBinsFilled();
  vector<double> values(bins.size());

  if(enYNormalization && !m_dnlValuesMap.empty())
  {
    int idx = 0;
    for(auto && bin : bins){
      values[idx++] = getBinContent(bin) / (1.0+getDNLValue(m_dnlValuesMap,bin));
    }
    return values;
  }
  else
  {
    int idx = 0;
    for(auto && bin : bins){
      values[idx++] = getBinContent(bin);
    }
    return values;
  }
}


double DataHisto::getDNLValue(const DNLValuesMap &dnlMap, int bin)
{
  const auto it = dnlMap.find(bin);
  if(it == dnlMap.end()){
    return 0.0;
  }
  return std::max(-0.99,it->second);
}


uint32_t DataHisto::getBinContent(uint16_t bin) const
{
  if(histoValues.find(bin) == histoValues.end()){
    return 0;
  }else{
    return histoValues.at(bin);
  }
}


void DataHisto::getDrawValues(std::vector<uint16_t> & xValues, std::vector<uint32_t> &yValues) const
{
  const auto xBins = getBins();
  const auto yBins = getBinValues();

  int numValues = xBins.size();
  if(numValues == 0){
    xValues.push_back(0);
    yValues.push_back(0);
    std::cout << "ERROR DataHisto : getDrawValues no bins found " << std::endl;
    return;
  }

  int numPoints = numValues * 4;
  xValues.resize(numPoints);
  yValues.resize(numPoints);

  xValues[0] = xBins[0];
  yValues[0] = 0;

  for(int i = 0; i<numValues; i++)
  {
    int idx = 4*i;
    xValues[idx  ] = xBins[i];
    xValues[idx+1] = xBins[i];
    xValues[idx+2] = xBins[i]+1;
    xValues[idx+3] = xBins[i]+1;

    yValues[idx  ] = 0.01; // 0.0 makes problems in logatirthmic scales
    yValues[idx+1] = yBins[i];
    yValues[idx+2] = yBins[i];
    yValues[idx+3] = 0.01;
  }

  //remove doubles
  for(int i=numValues-1; i>0; --i){
    if((xValues[i] == xValues[i-1]) && (yValues[i] == yValues[i-1]))
    {
      xValues.erase(xValues.begin() + i);
      yValues.erase(yValues.begin() + i);
    }
  }
}


utils::Stats DataHisto::getStats() const
{
  const double mean = calcMean();
  if(count < 2) return {mean,0};

  double rms = calcRMS(mean);

  return {mean,rms};
}


utils::Stats DataHisto::getStats(uint32_t minBin, uint32_t maxBin) const
{
  const double mean = calcMean(minBin,maxBin);
  if(count < 2) return {mean,0};

  double rms = calcRMS(mean,minBin,maxBin);

  return {mean,rms};
}

double DataHisto::calcMean(uint32_t minBin, uint32_t maxBin) const
{
  if(count == 0) {return 0;}
  if(count == 1) {return histoValues.begin()->first;}

  if(hasBinBoundaries())
  {
    double rangeCount = 0.0;
    const auto & binValues = getBinValues();
    double meanAcc = 0.0;
    for(size_t idx = minBin; idx<maxBin; idx++) {
      meanAcc += binValues[idx] * (m_binBoundaries[idx]+0.5);
      rangeCount += binValues[idx];
    }
    return meanAcc/rangeCount;
  }else{
    double rangeCount = 0.0;
    const auto & binValues = getBinValues();
    const auto & bins = getBins();
    double meanAcc = 0.0;

    for(size_t idx = minBin; idx<maxBin; idx++) {
      meanAcc += binValues[idx] * bins[idx];
      rangeCount += binValues[idx];
    }
    return meanAcc/rangeCount;
  }
}


double DataHisto::calcRMS(const double & mean, uint32_t minBin, uint32_t maxBin) const
{
  if(hasBinBoundaries()){
    double rangeCount = 0.0;
    double rmsAcc = 0.0;
    const auto binValues = getBinValues();
    for(size_t idx = minBin; idx<maxBin; idx++) {
      const double a = (m_binBoundaries[idx]+0.5-mean);
      rmsAcc += binValues[idx]*a*a;
      rangeCount += binValues[idx];
    }
    return sqrt(rmsAcc/(rangeCount-1));
  }else{
    double rangeCount = 0.0;
    double rmsAcc = 0.0;
    const auto binValues = getBinValues();
    const auto & bins = getBins();
    for(size_t idx = minBin; idx<maxBin; idx++) {
      const double a = bins[idx]-mean;
      rmsAcc += binValues[idx]*a*a;
      rangeCount += binValues[idx];
    }
    return sqrt(rmsAcc/(rangeCount-1));
  }
}

double DataHisto::calcMean() const
{
  if(count == 0) {return 0;}
  if(count == 1) {return histoValues.begin()->first;}

  if(hasBinBoundaries())
  {
    const auto & binValues = getBinValues();
    size_t numValues = binValues.size();
    double meanAcc = 0.0;
    for(size_t idx = 0; idx<numValues; idx++) {
      meanAcc += binValues[idx] * (m_binBoundaries[idx]+0.5);
    }
    return meanAcc/count;
  }else{
    uint64_t meanAcc = accumulate(histoValues.begin(),histoValues.end(),0ul,binWeight);
    return (double)meanAcc/(double)count;
  }
}


double DataHisto::calcRMS(const double & mean) const
{
  if(hasBinBoundaries()){
    double rmsAcc = 0.0;
    const auto binValues = getBinValues();
    size_t numValues = binValues.size();

    for(size_t idx = 0; idx<numValues; idx++) {
      const double a = (m_binBoundaries[idx]+0.5-mean);
      rmsAcc += binValues[idx]*a*a;
    }
    return sqrt(rmsAcc/(count-1));
  }else{
    double rmsAcc = 0.0;
    for(const auto & bin : histoValues){
      const double a = (bin.first-mean);
      rmsAcc += bin.second*a*a;
    }
    return sqrt(rmsAcc/(count-1));
  }
}


uint16_t DataHisto::getMaxBin()
{
  const auto binValues = getBinValues();
  const auto bins = getBins();
  const auto maxIdx = std::distance(binValues.begin(),std::max_element(binValues.begin(),binValues.end()));
  return bins[maxIdx];
}

DataHisto::DNLValuesVec DataHisto::calcDNLValues() const
{
  DNLValuesVec dnlValues;

  double meanCnt = 0;
  for(const auto & bin : histoValues){
    meanCnt += bin.second;
  }
  meanCnt /= histoValues.size();

  auto bins = getBins();


  // find min index which is first above meanCnt
  uint32_t minIdx=0;
  uint32_t maxIdx=bins.size()-1;
  for(uint32_t i=0; i<bins.size(); i++){
    if(getBinContent(bins[i]) > meanCnt){
      minIdx = i;
      break;
    }
  }

  // find max index which is last above meanCnt
  for(uint32_t i=bins.size()-1; i>=0; --i){
    if(bins[i] != 511){ // exclude error code bin from analysis
      if(getBinContent(bins[i]) > meanCnt){
        maxIdx = i;
        break;
      }
    }
  }

  uint16_t minBin = bins[minIdx];
  uint16_t maxBin = bins[maxIdx];

  //fit meanCnt to new range check if missing bins are found
  uint32_t numValidBins = maxIdx-minIdx+1;
  uint32_t numExpectedBins = maxBin-minBin+1;

  if(maxBin - minBin < 5){
    cout << "Utils: DNL Compute Warning: Very small Bin range. Will only fit few bins for DNL" << endl;
    print();
  }

  if(numValidBins != numExpectedBins){
    cout << "Utils: DNL Compute Warning: " << numExpectedBins - numValidBins << " missing bin found in DNL compute range" << endl;
    //print();
  }

  //update mean dnl for selected bin range
  meanCnt = 0;
  for(uint16_t bin=minBin; bin<=maxBin; bin++){
    meanCnt += getBinContent(bin);
  }
  meanCnt /= numExpectedBins;

  //generate dnl vector for selected bins
  for(uint16_t bin=minBin; bin<=maxBin; bin++){
    double dnl = getBinContent(bin)/meanCnt - 1.0;
    dnlValues.push_back({bin,dnl});
  }

  return dnlValues;
}


double DataHisto::calcDNLRMS() const
{
  const auto dnlValues = calcDNLValues();
  double rms = 0.0;
  for(auto && pair : dnlValues){
    rms += (pair.dnl*pair.dnl);
  }
  double N = std::max(dnlValues.size()-1,(size_t)1);
  return sqrt(rms/N);
}

DataHisto::DNLEval DataHisto::calcDNLEval() const
{
  DataHisto::DNLEval evalValues;
  const auto dnlValues = calcDNLValues();
  if(dnlValues.empty()){
    return evalValues;
  }
  evalValues.dnlMap = toMap(dnlValues);

  double maxINL = 0.0;
  double maxDNL = dnlValues.front().dnl;
  double maxMoveDNL = maxDNL;
  double currXValue = 0.0;
  double meanDNL = 0.0, meanDNLRMS = 0.0, meanINL = 0.0, meanINLRMS = 0.0;
  double movingMeanDNLRMS = 0.0;
  double movingMeanDNL;

  // remmeber start bin, is used to correct for undetected bins
  int startBin = dnlValues.front().bin - 1;

  int idx = 1;
  int maxIdx = dnlValues.size();
  for(auto && value : dnlValues)
  {
    //dnl computation
    double currDNLValue = value.dnl;
    double absDNL = abs(currDNLValue);
    if(maxDNL<absDNL){
      maxDNL = absDNL;
    }
    meanDNL += absDNL;
    meanDNLRMS += absDNL*absDNL;

    //inl computation
    currXValue += currDNLValue;
    double currBinCnt = value.bin - startBin;
    double currINLValue = ((currXValue - currBinCnt)/currBinCnt) - 1;
    double absINL = abs(currINLValue);
    if(maxINL<absINL){
      maxINL = absINL;
    }
    meanINL += absINL;
    meanINLRMS += absINL*absINL;

    // moving dnl computation over next 8 bins
    int cnt = 0;
    double movingMeanLSB = 0.0;
    for(int i=std::max(idx-4,0); i<std::min(idx+4,maxIdx); i++){
       movingMeanLSB += (dnlValues[i].dnl+1.0);
       cnt++;
    }
    if(cnt>0) movingMeanLSB /= cnt;

    double binWidth = currDNLValue+1;
    movingMeanDNL = binWidth/movingMeanLSB - 1.0;
    if(movingMeanDNL>maxMoveDNL) maxMoveDNL = movingMeanDNL;
    movingMeanDNLRMS += movingMeanDNL*movingMeanDNL;

    // add values to maps
    evalValues.dnlMoveMap[value.bin] = movingMeanDNL;
    evalValues.inlMap[value.bin] = currINLValue;
    idx++;
  }

  double cnt = (double) dnlValues.size();
  evalValues.meanDNL = meanDNL/cnt; // absoluteDNL meanDNL is 0 by definition
  evalValues.meanINL = meanINL/cnt;
  evalValues.maxDNL = maxDNL; // absoluteDNL meanDNL is 0 by definition
  evalValues.maxINL = maxINL;
  evalValues.meanDNLRMS = sqrt(meanDNLRMS/(cnt-1));
  evalValues.meanINLRMS = meanINLRMS - (meanINL*meanINL*cnt);
  evalValues.meanINLRMS = (meanINLRMS>0)? sqrt(meanINLRMS/(cnt-1)) : sqrt(-meanINLRMS/(cnt-1));
  evalValues.movingMeanDNLRMS = sqrt(movingMeanDNLRMS/(cnt-1));
  evalValues.numBinsFound = dnlValues.size();
  return evalValues;
}


DataHisto::DNLValuesMap DataHisto::toMap(const DataHisto::DNLValuesVec & dnlValues)
{
  DNLValuesMap dnlMap;
  for(auto && value : dnlValues){
    dnlMap[value.bin] = value.dnl;
  }
  return dnlMap;
}


std::vector<DataHisto::DNLValuesMap> DataHisto::toDnlValuesMapVec(const DNLEvalMap & dnlEvals)
{
  std::vector<DataHisto::DNLValuesMap> dnlValuesMapVec(65536,DNLVALUESEMPTYMAP);
  for(auto && item : dnlEvals){
    dnlValuesMapVec[item.first] = item.second.dnlMap;
  }
  return dnlValuesMapVec;
}


void DataHisto::clear()
{
  count = 0;
  histoValues.clear();
  std::fill(binValsBuffer,binValsBuffer+BUFS,0);
}


void DataHisto::toZero()
{
  count = 0;
  for(auto & item : histoValues){
    item.second = 0;
  }
  std::fill(binValsBuffer,binValsBuffer+BUFS,0);
}


void DataHisto::dumpContent(ostream &outStream, int width) const
{
  const auto bins = getBins();
  const auto binValues = getBinValues();
  int numBins = bins.size();
  for(int idx = 0; idx<numBins; idx++){
    outStream << bins[idx] << " ";
    outStream << binValues[idx];
    outStream << "\n";
  }
}


void DataHisto::dumpContent(std::ostream &outStream, const std::string & infoStr, int width) const
{
  const auto bins = getBins();
  const auto binValues = getBinValues();
  int numBins = bins.size();
  for(int idx = 0; idx<numBins; idx++){
    outStream << infoStr << " ";
    outStream << bins[idx] << " ";
    outStream << binValues[idx];
    outStream << "\n";
  }
}


void DataHisto::print(ostream &outStream) const
{
  const auto stats = getStats();
  outStream << endl;
  outStream << "|----------------------------------------------" << endl;
  outStream << "| Histogram stats: numValues = " << setw(6) << count      << endl;
  outStream << "|                  mean      = " << setw(6) << stats.mean << endl;
  outStream << "|                  rms       = " << setw(6) << stats.rms  << endl;
  outStream << "|----------------------------------------------" << endl;
  outStream << "| " << endl;
  outStream << "| " ;

  const auto bins = getBins();
  for(const auto & bin : bins){
    outStream << setw(6) <<getBinContent(bin);
  }
  outStream << endl;

  outStream << "| ";
  for(const auto & bin : bins){
    outStream << setw(6) << bin;
  }
  outStream << endl;

  outStream << "|______________________________________________" << endl;
}


//static function
vector<double> calcDNLRMSValuesFromHistograms(const DataHistoVec & pixelHistograms)
{
  const size_t numValues = pixelHistograms.size();

  vector<double> rmsValues(numValues);

#pragma omp parallel for
  for(size_t idx = 0; idx < numValues; idx++){
    rmsValues[idx] = pixelHistograms[idx].calcDNLRMS();
  }
  return rmsValues;
}

std::vector<double> calcMeanImageFromHistograms(const DataHistoVec & pixelHistograms, const std::vector<uint32_t> & pixels)
{
  static constexpr size_t MAXPIXEL = 16*4096;
  const size_t numValues = pixelHistograms.size();

  vector<double> imageValues(MAXPIXEL);

#pragma omp parallel for
  for(size_t idx = 0; idx < numValues; idx++){
    const auto imagePixel = pixels[idx];
    if(MAXPIXEL <= imagePixel){
      cout << "DataHisto ERROR: pixel larger than MAXPIXEL: pixel[" << idx << "] = " << imagePixel << endl;
    }else{
      imageValues[imagePixel] = pixelHistograms[imagePixel].calcMean();
    }
  }
  return imageValues;
}


DataHisto::DNLEvalVec calcDNLEvalsFromHistograms(const std::vector<uint32_t> & pixels, const DataHistoVec & pixelHistograms)
{
  const size_t numValues = pixelHistograms.size();
  DataHisto::DNLEvalVec dnlEvalVec(numValues);

#pragma omp parallel for
  for(size_t idx = 0; idx < numValues; idx++){
    uint32_t pixel = pixels[idx];
    DataHisto::DNLEval eval = pixelHistograms[idx].calcDNLEval();
    eval.pixel = pixel;
    dnlEvalVec[idx] = eval;
  }

  return dnlEvalVec;
}


void fillBufferToDataHistoVec(DataHistoVec & pixelHistograms)
{
  const size_t numHistos = pixelHistograms.size();
#pragma omp parallel for
  for(size_t idx = 0; idx < numHistos; idx++){
    pixelHistograms[idx].fillBufferToHistoMap();
  }
}


vector<DataHisto::DNLValuesVec> calcDNLValuesFromHistograms(const DataHistoVec & pixelHistograms)
{
  const size_t numValues = pixelHistograms.size();
  vector<DataHisto::DNLValuesVec> dnlValuesVector(numValues);

#pragma omp parallel for
  for(size_t idx = 0; idx < numValues; idx++){
    dnlValuesVector[idx] = pixelHistograms[idx].calcDNLValues();
  }

  return dnlValuesVector;
}


std::vector<double> DataHisto::calcStepBoarderFractions(const DataHistoVec & pixelHistograms, bool aboveThresh)
{
  if(pixelHistograms.empty()){return std::vector<double>();}

  int lowerBin;
  if(pixelHistograms.size() == 1){
    lowerBin = floor(pixelHistograms.front().calcMean());
  }else{
    double firstMean = pixelHistograms.front().calcMean();
    double lastMean  = pixelHistograms.back().calcMean();
    lowerBin = std::round((firstMean+lastMean)/2.0);
  }

  vector<double> stepBoarderValues(pixelHistograms.size());
  int idx = 0;
  for(auto && histo : pixelHistograms){
    stepBoarderValues[idx++]  = histo.calcStepBoarderFraction(lowerBin,aboveThresh);
  }
  return stepBoarderValues;
}


double DataHisto::calcStepBoarderFraction(int lowerBin, bool aboveThresh) const
{
  double higher = 0;
  for(auto && entry : histoValues){
    if(entry.first > lowerBin && aboveThresh){
      higher += entry.second;
    }
    if(entry.first < lowerBin && !aboveThresh){
      higher += entry.second;
    }
  }
  return higher/count;
}


void DataHisto::DNLEval::print(ofstream & out) const
{
  out << "######## Pixel " << pixel << " :\n";
  out << "Summary:\n";
  out << setw(17) << left << "- MeanDNLRMS: "    << meanDNLRMS        << "\n";
  out << setw(17) << left << "- MaxDNL: "        << maxDNL            << "\n";
  out << setw(17) << left << "- MeanDNL: "       << meanDNL           << "\n";
  out << setw(17) << left << "- MovingDNLRMS: " << movingMeanDNLRMS << "\n";
  out << setw(17) << left << "- MeanINLRMS: "    << meanINLRMS        << "\n";
  out << setw(17) << left << "- MaxINL: "        << maxINL            << "\n";
  out << setw(17) << left << "- MeanINL: "       << meanINL           << "\n";
  out << setw(17) << left << "- NumBinsFound: "  << numBinsFound      << "\n";
  out << "+ DNLValues: \n";
  for(auto && value : dnlMap){
    out << "(" << value.first << "/" << value.second << ");";
  }
  out << "\n";
}


void DataHisto::DNLValue::print(std::ofstream & out) const
{
  utils::printPair<int,double>({bin,dnl},out);
}


void DataHisto::DNLValue::read(const std::string & str)
{
  const auto values = utils::readPair<int,double>(str);
  bin = values.first;
  dnl = values.second;
}

void DataHisto::exportDNLEvaluationsVec(const DataHisto::DNLEvalVec & evalVec, const std::string & fileName, int deskewSetting)
{
  std::vector<uint32_t> pixels(evalVec.size());
  int idx=0;
  for(auto && eval : evalVec){
    pixels[idx++] = eval.pixel;
  }
  std::sort(pixels.begin(),pixels.end());

  ofstream out(fileName);
  out << "#DNL INL Evaluation\n";
  out << "#Contained pixels : " << utils::positionVectorToList(pixels)<< "\n";
  out << "#Clock Deskew Value : " << deskewSetting << "\n";
  out << "#---------------------------------\n";
  for(auto && eval : evalVec){
    eval.print(out);
  }

  std::cout << "DataHisto DNL Evaluation Vector exported to: " << fileName << std::endl;
}


void DataHisto::exportDNLEvaluationsMap(const DataHisto::DNLEvalMap & evalMap, const std::string & fileName, int deskewSetting)
{
  if(evalMap.empty())return;

  std::vector<uint32_t> pixels(evalMap.size());
  int idx=0;
  for(auto && eval : evalMap){
    pixels[idx++] = eval.first;
  }

  std::sort(pixels.begin(),pixels.end());
  ofstream out(fileName);
  out << "#DNL INL Evaluation\n";
  out << "#Contained pixels : " << utils::positionVectorToList(pixels)<< "\n";
  out << "#Clock Deskew Value : " << deskewSetting << "\n";
  out << "#---------------------------------\n";
  for(auto && eval : evalMap){
    eval.second.print(out);
  }

  std::cout << "DataHisto DNL Evaluation Map exported to: " << fileName << std::endl;
}


DataHisto::DNLEvalMap DataHisto::importDNLEvaluationsMap(const std::string & fileName)
{
  DataHisto::DNLEvalMap evalMap;

  if(!utils::checkFileExists(fileName)){
    cout << "Could not open DNL Evaluation file: " << fileName << "\n";
    return evalMap;
  }

  ifstream in(fileName);
  if(!in.is_open()){
    cout << "Could not open DNL Evaluation file: " << fileName << "\n";
    return evalMap;
  }


  vector<string> pixelEntries;
  utils::split(in,'#',pixelEntries);
  vector<uint32_t> pixels;

  for(auto && entry : pixelEntries){
    if(entry.substr(0,6) == " Pixel"){
      DNLEval eval = getDNLEvalFromImportString(entry);
      evalMap[eval.pixel] = eval;
      pixels.push_back(eval.pixel);
    }
  }

  cout << "DataHisto: Imported DNL Values of " << evalMap.size();
  cout << " Pixels: " << utils::positionVectorToList(pixels) << endl;

  return evalMap;
}


DataHisto::DNLEval DataHisto::getDNLEvalFromImportString(const string & str)
{
  vector<string> elements;
  utils::split(str,'\n',elements);

  int idx = 2;

  DNLEval eval;
  eval.pixel             = getPixelfromStr(elements[0]);
  eval.meanDNLRMS        = getValuefromStr(elements[idx++]);
  eval.maxDNL            = getValuefromStr(elements[idx++]);
  eval.meanDNL           = getValuefromStr(elements[idx++]);
  eval.movingMeanDNLRMS  = getValuefromStr(elements[idx++]);
  eval.maxINL            = getValuefromStr(elements[idx++]);
  eval.meanINL           = getValuefromStr(elements[idx++]);
  eval.meanINLRMS        = getValuefromStr(elements[idx++]);
  eval.numBinsFound      = getValuefromStr(elements[idx++]);

  vector<string> dnlStrVec;
  utils::split(elements[11],';',dnlStrVec);
  for(auto && dnlStr : dnlStrVec){
    auto dnlPair = utils::readPair<int,double>(dnlStr);
    if(dnlPair.second<-1.0){
      std::cout << "Pixel " << eval.pixel << " Empty Bin found " << dnlPair.first << " dnl = " << dnlPair.second << std::endl;
      dnlPair.second = -1.0;
    }
    eval.dnlMap[dnlPair.first] = dnlPair.second;

  }
  return eval;
}


uint32_t DataHisto::getPixelfromStr(const std::string & str)
{
  std::string dummy;
  uint32_t number;
  std::istringstream is(str);
  is >> dummy >> number;
  return number;
}


double DataHisto::getValuefromStr(const std::string & str)
{
  std::string dummy;
  double number;
  std::istringstream is(str);
  is >> dummy >> dummy;
  is >> number;
  return number;
}


void DataHisto::dumpHistogramsToASCII(const std::string & fileName, const std::vector<uint32_t> & pixels, const std::vector<DataHisto> & pixelHistos, int setting)
{
  ofstream out(fileName);
  int numPxs = pixels.size();
  out << "#Measurement File Dump \n";
  out << "#Number of Pixels: " << numPxs << "\n";
  out << "#Contained Pixels: " << utils::positionVectorToList(pixels) << "\n";
  out << "#DeviceID ";
  out << "Pixel ";
  out << "Setting ";
  out << "Bin ";
  out << "BinValue\n";

  for(int idx=0; idx<numPxs; idx++){
    string infoStr = "0 " +  to_string(pixels[idx]) + " " + to_string(setting);
    pixelHistos[idx].dumpContent(out,infoStr,9);
  }

  cout << "Histo Export to " << fileName << " successful" << endl;
}

DataHistoMap DataHisto::importHistogramsFromASCII(const std::string & fileName, const std::vector<int> &pixels, int numPixels)
{
  return importHistogramsFromASCII(fileName,utils::convertVectorType<int,uint32_t>(pixels),numPixels);
}


DataHistoMap DataHisto::importHistogramsFromASCII(const std::string & fileName, const std::vector<uint32_t> &pixels, uint32_t numPixels)
{
  std::vector<bool> pixelNeeded(numPixels,false);
  bool allPx = pixels.empty();
  for (auto p : pixels) if (p < numPixels) pixelNeeded[p] = true;

  DataHistoMap dataHistos;
  ifstream in(fileName);
  if (!in.good()) std::cout << "importHistogramsFromASCII(): file " << fileName << " not good." << std::endl;
  string line;
  int numCols = 5;
  while(line.substr(0,9) != "#DeviceID"){
    getline(in,line);
    if(line[0] != '#'){ // no header niformation rewind file and start directly
      std::vector<uint32_t> elems;
      split(line,' ',elems);
      numCols = elems.size();
      in.seekg(0, ios::beg);
      break;
    }
  }
  int device;
  int setting1,setting2;
  int bin;
  int binValue;
  int currPixel;

  if(numCols == 5){
    while(getline(in,line)){
      stringstream iss(line);
      iss >> device;
      iss >> currPixel;
      if (pixelNeeded[currPixel] || allPx) {
        iss >> setting1;
        iss >> bin;
        iss >> binValue;
        dataHistos[currPixel].addToBufN(bin,binValue);
      }
    }
  }else if(numCols == 6){
    while(getline(in,line)){
      stringstream iss(line);
      iss >> device;
      iss >> currPixel;
      if (pixelNeeded[currPixel] || allPx) {
        iss >> setting1;
        iss >> setting2;
        iss >> bin;
        iss >> binValue;
        int idx = currPixel*(16*64) + setting1 + setting2*64;
        dataHistos[idx].addToBufN(bin,binValue);
      }
    }
  }

  for (auto & entry : dataHistos) {
    entry.second.fillBufferToHistoMap();
  }

/*
//reorder Histograms if Pixels have been mixed up: ASIC Wise to Image Wise:
  for(int asic=0; asic<16; asic++){
    for(int asicPixel =0; asicPixel<4096; asicPixel++){
      size_t pixel = asic*4096+asicPixel;
      if(pixel>=numPixels) break;
      sortedHistos[utils::calcImagePixel(asic,asicPixel)] = dataHistos[pixel];
    }
  }
  dataHistos.clear();
  for(int pixel =0; pixel<numPixels; pixel++){
    dataHistos[pixel] = sortedHistos[pixel];
  }
*/
  return dataHistos;
}


struct SmallerBinPos{
  bool operator()(const DataHisto::DNLValue & a, DataHisto::DNLValue & b) const {
    return (a.bin < b.bin);
  }
};

//DataHisto::DNLBinVec DataHisto::calcDNLBINVec(DataHisto::DNLValuesVec dnlValues)
//{
//  //TODO:: check if function sorts for increasing bin values
//  sort(dnlValues.begin(),dnlValues.end(),SmallerBinPos);
//  DataHisto::DNLBinVec corrBinEntries;

//  double start = dnlValues.front().bin;
//  int lastBin  = start-1;
//  for(auto && dnl : dnlValues){
//    DataHisto::DNLBin nextBin{start,dnl.dnl,0};
//    binValues.push_back(nextBin);
//    if(lastBin != (dnl.bin+1)){
//      //if bin not included add one
//      start += 1.0;
//    }else{
//      start += dnl.dnl;
//    }
//  }

//  return corrBinEntries;
//}


StatsVec getMeandAndRMSVector(const DataHistoVec & pixelHistograms)
{
  const int numPxs = pixelHistograms.size();

  StatsVec statsVector(numPxs);

#pragma omp parallel for
  for(int idx=0; idx<numPxs; idx++){
    statsVector[idx] = pixelHistograms[idx].getStats();
  }
  return statsVector;
}


std::vector<double> getMeanVector(const DataHistoVec & pixelHistograms)
{
  const int numPxs = pixelHistograms.size();

  std::vector<double> values(numPxs);

#pragma omp parallel for
  for(int idx=0; idx<numPxs; idx++){
    values[idx] = pixelHistograms[idx].getStats().mean;
  }
  return values;
}


std::vector<double> getRMSVector(const DataHistoVec & pixelHistograms)
{
  const int numPxs = pixelHistograms.size();

  std::vector<double> values(numPxs);

#pragma omp parallel for
  for(int idx=0; idx<numPxs; idx++){
    values[idx] = pixelHistograms[idx].getStats().rms;
  }
  return values;
}


FitResultsVec fitSpectra(DataHistoVec & pixelHistograms, const DataHisto::DNLEvalMap & dnlEvalMap)
{
  const int numPxs = pixelHistograms.size();
  FitResultsVec fitResults(numPxs);

  std::atomic_int pixelCnt;
  pixelCnt = 0;

  const bool dnlsValid = !dnlEvalMap.empty();

#pragma omp parallel for
  for(int idx=0; idx<numPxs; idx++){
    const auto pixelDNLValues = dnlsValid? dnlEvalMap.at(idx).dnlMap : utils::DataHisto::DNLValuesMap();
    fitResults[idx] = pixelHistograms[idx].fitSpectrum(pixelDNLValues);
    utils::CoutColorKeeper keeper(utils::STDBLUE);
    cout << "++++ Fitted Spectrum "  << std::setw(5) << pixelCnt++ << "/" << numPxs << endl;
  }

  return fitResults;
}


FitResultsMap fitSpectra(DataHistoMap & pixelHistograms, const DataHisto::DNLEvalMap & dnlEvalMap)
{
  const size_t numPxs = pixelHistograms.size();
  FitResultsMap fitResults;

  //initialize map (probably slow)
  const auto keyVector = utils::getKeys(pixelHistograms);
  for(auto && pixel : keyVector){
    fitResults[pixel] = SpectrumFitResult();
  }

  std::atomic_int pixelCnt;
  pixelCnt = 0;

  const bool dnlsValid = !dnlEvalMap.empty();

#pragma omp parallel for
  for(size_t idx=0; idx<numPxs; idx++){
    const auto pixel = keyVector[idx];
    const auto pixelDNLValues = dnlsValid? dnlEvalMap.at(pixel).dnlMap : utils::DataHisto::DNLValuesMap();
    fitResults[pixel] = pixelHistograms.at(pixel).fitSpectrum(pixelDNLValues);
    utils::CoutColorKeeper keeper(utils::STDBLUE);
    cout << "++++ Fitted Spectrum " << std::setw(5) <<  pixelCnt++ << "/" << numPxs << endl;
  }

  return fitResults;
}



/* input: matrix of iramps and pxdelay histos, works also if vectors do not have the same size
   * output: mean pxdelay steps calculated on different iramp settings
   */
std::vector<double> calcPxDelays(const std::vector<DataHistoVec> & histos)
{
  std::vector<double> pxDelaySteps;
  if (histos.size()>0) {
    pxDelaySteps.resize(histos.at(0).size());
  }
  for (size_t irmp=0; irmp<histos.size(); ++irmp) {
    pxDelaySteps[0] = 0.0;
    for (size_t pxDelay=1; pxDelay<histos.at(irmp).size()-1; ++pxDelay) {
      pxDelaySteps[pxDelay] += histos[irmp][pxDelay-1].calcMean() - histos[irmp][pxDelay].calcMean();
    }
  }
  for (size_t pxDelay=0; pxDelay<pxDelaySteps.size(); ++pxDelay) {
    pxDelaySteps[pxDelay] /= histos.size();
  }
  std::vector<double> pxDelays;
  pxDelays.resize(16);
  for (size_t s=0; s<pxDelaySteps.size(); ++s) {
    pxDelays[s] = std::accumulate(pxDelaySteps.begin(),pxDelaySteps.begin()+s,0.0);
  }
  return pxDelays;
}

std::vector<double> dnlValuesToSingleVec(const DataHisto::DNLValuesVec & dnlValues)
{
  std::vector<double> values(dnlValues.size()*2);
  int idx = 0;
  for(auto && dnl: dnlValues){
    values[idx++] = dnl.bin;
    values[idx++] = dnl.dnl;
  }
  return values;
}

std::vector<double> dnlValuesToSingleVec(const DataHisto::DNLValuesMap & dnlValuesMap)
{
  std::vector<double> values(dnlValuesMap.size()*2);
  int idx = 0;
  for(auto && dnlElem: dnlValuesMap){
    values[idx++] = dnlElem.first;
    values[idx++] = dnlElem.second;
  }
  return values;
}

DataHisto::DNLValuesVec singleVecToDNLValues(const std::vector<double> & singleVec )
{
  DataHisto::DNLValuesVec dnlValues;
  for(size_t idx = 0; idx < singleVec.size(); idx +=2){
    DataHisto::DNLValue dnlValue {(int)singleVec[idx],singleVec[idx+1]};
    dnlValues.push_back(dnlValue);
  }
  return dnlValues;
}

  /* input: matrix of iramps and pxdelay histos, works also if vectors do not have the same size
   * output: mean pxdelays calculated on different iramp settings
   */
  std::vector<double> calcPxDelaySteps(int px, const DataHistoMap & histos)
  {
    std::vector<double> pxDelaySteps;
    pxDelaySteps.resize(16);
    pxDelaySteps[0] = 0.0;
    int numBrokenMeas = 0;
    std::vector<bool> irmpMeasGood(64,true);
    for (auto irmp=0; irmp<64; ++irmp) {
      if (!irmpMeasGood[irmp]) continue;
      for (auto pxDelay=0; pxDelay<16; ++pxDelay) {
        int i = 0*(16*64) + irmp + pxDelay*64; // as read in in DataHisto.cpp
        try{
          double step = histos.at(i+64).calcMean() - histos.at(i).calcMean();
        } catch (...) {
          numBrokenMeas++;
          std::cout << "WARNING: irmp = " << irmp << ", pxDelay = " << pxDelay <<
            " measurement is broken, not used to calcPxDelaySteps." << std::endl;
          irmpMeasGood[irmp] = false;
          if (numBrokenMeas > 10) {
            std::cout << "ERROR: pxDelay calculation aborted for pixel " << px  <<
              ", more than 10 broken measurements." << endl;
            return std::vector<double>();
          }
          break;
        }
      }
    }
    for (auto irmp=0; irmp<64; ++irmp) {
      if (irmpMeasGood[irmp]) {
        int i = 0;
        for (auto pxDelay=0; pxDelay<15; ++pxDelay) {
          i = px*(16*64) + irmp + pxDelay*64; // as read in in DataHisto.cpp
          //int idx = currPixel*(16*64) + setting1 + setting2*64;
          try {
            double step = histos.at(i+64).calcMean() - histos.at(i).calcMean();
            pxDelaySteps[pxDelay+1] += step;
          } catch (...) {
            std::cout << "ERROR utils : px=" << px << ", irmp=" << irmp << ", pxDelay=" << pxDelay
              << " not found in histos." << std::endl;
            return std::vector<double>();
          }
        }
      }
    }

    for (size_t pxDelay=0; pxDelay<pxDelaySteps.size(); ++pxDelay) {
      pxDelaySteps[pxDelay] /= (64-numBrokenMeas);
    }
    std::vector<double> pxDelays;
    pxDelays.resize(16);
    for (size_t s=1; s<pxDelaySteps.size(); ++s) {
      pxDelays[s] = std::accumulate(pxDelaySteps.begin(),pxDelaySteps.begin()+s+1,0.0);
    }
    return pxDelays;
  }

  SpectrumFitResult DataHisto::fitGauss(const DNLValuesMap & dnlValuesMap)
  {
    //utils::Timer timer;
    // fill bin boundaries from 0 - maxBin (+1 needed since we have boundaries)
    const auto bins = getBinsFilled();
    int minBin = bins.front();
    int maxBin = bins.back();

    fillHistoDNLInformation(minBin,maxBin,dnlValuesMap);
    auto vals = getBinValuesDoubleFilled();

    //// search for first maximum
    uint32_t binIndex = 0;
    uint32_t noiseBinIndex = 0;
    uint32_t noiseBinContent = 10;

    auto minmax = std::minmax_element(vals.begin(),vals.end());
    noiseBinIndex = std::distance(vals.begin(), minmax.second);
    noiseBinContent = getBinContent(bins[noiseBinIndex]);
    std::cout << "DataHisto: noise bin at " << bins[noiseBinIndex] << std::endl;

    // search for local minimum right of noise peak;
    binIndex = noiseBinIndex;
    uint32_t locMinIndex = noiseBinIndex;
    uint32_t locMin = noiseBinContent;
    do {
      locMin = getBinContent(bins[binIndex++]);
      locMinIndex = binIndex;
    } while (getBinContent(bins[binIndex]) < locMin);
    std::cout << "DataHisto: loc min found at " << bins[locMinIndex] << std::endl;

    const auto stats = getStats();
    double noiseMeanInit = stats.mean;
    double noiseSigmaInit = stats.rms;

    //auto bbsDouble = getBinsDouble();
    auto bbsDouble = (hasBinBoundaries()) ? m_binBoundaries : getBinsDouble();

    // calculation of ROIs for the noise and signal fits
    // calculation of ROIs for the noise and signal fits
    uint32_t roiNoiseStart = std::max(0.0,noiseBinIndex - round(5*noiseSigmaInit) - 1);
    uint32_t roiNoiseEnd   = std::min(noiseBinIndex + round(5*noiseSigmaInit) + 1,(double)vals.size());

    size_t numValues = vals.size();
    size_t numNoise  = roiNoiseEnd-roiNoiseStart+1;
    std::vector<double> errors(numValues);
    for(size_t i=0; i<vals.size(); i++){
      errors[i] = sqrt(vals[i]);
    }

    SpectrumFitResult fitRes;
    fitRes.roiNoiseStart  = bins[roiNoiseStart];
    fitRes.roiNoiseEnd    = bins[roiNoiseEnd];

    ROOT::Minuit2::MnUserParameters uparNoise;
    uparNoise.Add("noise_amp", noiseBinContent, sqrt(noiseBinContent));
    // Name, InitValue, Error, Lower Bound, Upper Nound
    uparNoise.Add("noise_mean",  noiseMeanInit, noiseSigmaInit, fitRes.roiNoiseStart,fitRes.roiNoiseEnd);
    uparNoise.Add("noise_sigma", noiseSigmaInit, 0.5*noiseSigmaInit);
    uparNoise.Fix(0);

    // fit noise Peak
    ROOT::Minuit2::FunctionMinimum noiseFit = utils::minuitFit(&bbsDouble[roiNoiseStart],&vals[roiNoiseStart],
        &errors[roiNoiseStart],numNoise,uparNoise,&utils::gauss);

    fitRes.noiseAmp   = std::max(0.0,noiseFit.UserState().Value(0));
    fitRes.noiseMean  = std::max(0.0,noiseFit.UserState().Value(1));
    fitRes.noiseSigma = std::max(0.0,noiseFit.UserState().Value(2));

    return fitRes;
}


SpectrumFitResult DataHisto::fitSpectrum(const DNLValuesMap & dnlValuesMap)
{
  //utils::Timer timer;

  auto bins = getBinsFilled();
  const int minBin = bins.front();
  const int maxBin = bins.back();

  fillHistoDNLInformation(minBin,maxBin,dnlValuesMap);
  auto vals = getBinValuesDoubleFilled();

  auto bbsDouble = (hasBinBoundaries()) ? m_binBoundaries : getBinsDoubleFilled();
  const int numBBDoubles = bbsDouble.size();
  if(numBBDoubles<5){
    cout << "Error no spectrum fitting possible with only " << numBBDoubles << " binBoundaries" << endl;
    return SpectrumFitResult();
  }

  auto minmax = std::minmax_element(vals.begin(),vals.end());
  uint32_t noiseBinIndex = std::distance(vals.begin(), minmax.second);
  uint32_t noiseBinContent = getBinContent(bins[noiseBinIndex]);
  std::cout << "DataHisto: noise bin at " << bins[noiseBinIndex] << std::endl;

  const auto stats = getStats();
  const double noiseMeanInit = bbsDouble[noiseBinIndex];
  const double noiseSigmaInit = std::min(2.0,stats.rms);

  // calculation of ROIs for the noise and signal fits
  uint32_t roiNoiseStart = std::max(0.0,noiseMeanInit - round(5*noiseSigmaInit) - 1);
  uint32_t roiNoiseEnd   = std::min(noiseMeanInit + round(5*noiseSigmaInit) + 1,bbsDouble.back());

  if(noiseSigmaInit <= 0.1){
    roiNoiseStart -= 0.5;
    roiNoiseEnd   += 0.5;
  }

  // run through bins until value larger than roiNoiseStart is found
  int roiNoiseStartIdx = 0;
  while(bbsDouble[roiNoiseStartIdx] < floor(roiNoiseStart)){
    roiNoiseStartIdx++;
    if(roiNoiseStartIdx == numBBDoubles){
      break;
    }
  }
  roiNoiseStartIdx = std::min(std::max(roiNoiseStartIdx-1,0),(int)(bbsDouble.size()-1));

  // run through bins until value larger than roiNoiseEnd is found
  int roiNoiseEndIdx = roiNoiseStartIdx;
  while(bbsDouble[roiNoiseEndIdx] < ceil(roiNoiseEnd)){
    roiNoiseEndIdx++;
    if(roiNoiseEndIdx == numBBDoubles){
      break;
    }
  }
  roiNoiseEndIdx = std::min(roiNoiseEndIdx,(int)(bbsDouble.size()-1));


  SpectrumFitResult fitRes;
  fitRes.roiNoiseStart  = roiNoiseStart;
  fitRes.roiNoiseEnd    = roiNoiseEnd;

  std::cout << "DataHisto: noise init " << noiseMeanInit << " +-" << noiseSigmaInit << std::endl;
  std::cout << "DataHisto: noise roi " << fitRes.roiNoiseStart << "-" << fitRes.roiNoiseEnd << std::endl;
  std::cout << "DataHisto: noise roi idx " << roiNoiseStartIdx << "-" << roiNoiseEndIdx << std::endl;
  std::cout << "DataHisto: noise roi bins " << bbsDouble[roiNoiseStartIdx] << "-" << bbsDouble[roiNoiseEndIdx] << std::endl;


  ROOT::Minuit2::MnUserParameters uparNoise;
  uparNoise.Add("noise_amp", noiseBinContent, sqrt(noiseBinContent));
  // Name, InitValue, Error, Lower Bound, Upper Nound
  uparNoise.Add("noise_mean",  noiseMeanInit, noiseSigmaInit, fitRes.roiNoiseStart,fitRes.roiNoiseEnd);
  uparNoise.Add("noise_sigma", noiseSigmaInit, 0.5*noiseSigmaInit);
  uparNoise.Fix(0);

  size_t numValues = vals.size();
  size_t numNoise  = roiNoiseEndIdx-roiNoiseStartIdx+1;
  std::vector<double> errors(numValues);
  for(size_t i=0; i<vals.size(); i++){
    errors[i] = sqrt(vals[i]);
  }

  // fit noise Peak
  ROOT::Minuit2::FunctionMinimum noiseFit = utils::minuitFit(&bbsDouble[roiNoiseStartIdx],&vals[roiNoiseStartIdx],
      &errors[roiNoiseStartIdx],numNoise,uparNoise,&utils::gauss);

  fitRes.noiseAmp   = std::max(0.0,noiseFit.UserState().Value(0));
  fitRes.noiseMean  = std::max(0.0,noiseFit.UserState().Value(1));
  fitRes.noiseSigma = std::max(0.0,noiseFit.UserState().Value(2));

  fitRes.noiseAmpErr   = std::max(0.0,noiseFit.UserState().Error(0));
  fitRes.noiseMeanErr  = std::max(0.0,noiseFit.UserState().Error(1));
  fitRes.noiseSigmaErr = std::max(0.0,noiseFit.UserState().Error(2));

  if(abs(fitRes.noiseSigma - noiseSigmaInit)/noiseSigmaInit > 0.3 ){
    const auto stats = getStats(roiNoiseStartIdx,roiNoiseEndIdx);
    fitRes.noiseSigma  = stats.rms;
  }

  //###############################
  // Find Signal ROI:

  // search for local minimum right of noise peak;
  uint32_t binIndex = roiNoiseEndIdx;
  uint32_t locMinIndex = noiseBinIndex + 2*fitRes.noiseSigma;
  uint32_t locMin = noiseBinContent;
  do {
    locMin = vals[binIndex++];
    locMinIndex = binIndex;
    if(binIndex >= vals.size()) break;
  } while (vals[binIndex] <= locMin);

  if(locMinIndex >= vals.size()){
    cout << "Error no local minimum found, can not fit k alpha peak" << endl;
    return fitRes;
  }

  std::cout << "DataHisto: loc min found at " << bbsDouble[locMinIndex] << std::endl;

  // search for maximum from right of local minimum
  const auto maxElem = std::max_element(vals.begin()+locMinIndex,vals.end());

  uint32_t sigMaxBinIndex = std::distance(vals.begin(), maxElem);
  double sigMaxBin        = bbsDouble[sigMaxBinIndex];
  double sigMaxBinContent = std::max(vals[sigMaxBinIndex],1.0);
  double sigMaxBinErr     = sqrt(sigMaxBinContent);

  bool foundSigPeak =  sigMaxBin > bbsDouble[locMinIndex];
  if (foundSigPeak) {
    std::cout << "DataHisto: sigMaxBin found at " << sigMaxBin << std::endl;
  }
  else
  {
    std::cout << "No sigMaxBin found, aborting" << std::endl;
    return fitRes;
  }

  // 1/4th between right of noise peak and signal peak
  int roiSignalStartIndex = floor((3.0*locMinIndex + sigMaxBinIndex)/4.0);
  int roiSignalSize       = std::min((int)vals.size() - roiSignalStartIndex, (int)(fitRes.noiseSigma*8));

  double sigMeanInit   = round(bbsDouble[sigMaxBinIndex]+0.5);
  double troughRatInit = 1.0;

  ROOT::Minuit2::MnUserParameters uparSignal;
  uparSignal.Add("noise_mean", fitRes.noiseMean, fitRes.noiseMeanErr);
  uparSignal.Add("noise_sigma", fitRes.noiseSigma, fitRes.noiseSigmaErr);

  // Name, InitValue, Error, Lower Bound, Upper Nound
  uparSignal.Add("peak1_amp", sigMaxBinContent,sigMaxBinErr,0.0,2.0*sigMaxBinContent);
  uparSignal.Add("peak1_mean",sigMeanInit,noiseSigmaInit,sigMeanInit-5*noiseSigmaInit,sigMeanInit+5*noiseSigmaInit);
  uparSignal.Add("troughrat", troughRatInit, troughRatInit);

  uparSignal.Add("kabE_ratio", fitRes.s_currentTarget.kab_Eratio);
  uparSignal.Add("kabI_ratio", fitRes.s_currentTarget.kab_Iratio);

  uparSignal.Fix(0);
  uparSignal.Fix(1);
  uparSignal.Fix(5);
  uparSignal.Fix(6);

  fitRes.roiSignalStart = bbsDouble[roiSignalStartIndex];
  fitRes.roiSignalEnd   = bbsDouble[roiSignalSize-1];

  std::cout << "DataHisto: signal init " << sigMeanInit << " +-" << fitRes.noiseSigma << std::endl;
  std::cout << "DataHisto: signal roi " << fitRes.roiSignalStart << "-" << fitRes.roiSignalEnd << std::endl;


  // fit Signal Peak
  ROOT::Minuit2::FunctionMinimum signalFit = utils::minuitFit(&bbsDouble[roiSignalStartIndex],&vals[roiSignalStartIndex],
      &vals[roiSignalStartIndex],roiSignalSize,uparSignal,&utils::fitfunctionSimplified);

  fitRes.kaPeakAmp      = signalFit.UserState().Value(2);
  fitRes.kaPeakAmpErr   = signalFit.UserState().Error(2);
  fitRes.kaPeakMean     = signalFit.UserState().Value(3);
  fitRes.kaPeakMeanErr  = signalFit.UserState().Error(3);
  fitRes.troughrat      = signalFit.UserState().Value(4);
  fitRes.troughratErr   = signalFit.UserState().Error(4);

  return fitRes;
}


void DataHisto::fillHistoDNLInformation(int minBin, int maxBin, const DNLValuesMap & dnlValuesMap)
{
  m_dnlValuesMap.clear();
  m_binBoundaries.clear();
  if(dnlValuesMap.empty()) return;

  double newMean = 0.0;
  for(int i=minBin; i<=maxBin; i++){
    double dnl = 1.0+utils::DataHisto::getDNLValue(dnlValuesMap, i);
    newMean += dnl;
  }
  newMean /= (maxBin-minBin+1);

  for(int i=minBin; i<=maxBin; i++){
    m_dnlValuesMap[i] = (1.0+utils::DataHisto::getDNLValue(dnlValuesMap, i)/newMean) - 1.0;
  }
  calcBinBoundaries(m_dnlValuesMap);
}

void DataHisto::calcBinBoundaries(const DNLValuesMap & dnlValuesMap)
{
  m_binBoundaries.clear();
  if (dnlValuesMap.empty()) return;
  const auto bins = getBinsFilled();
  const auto maxBin = bins.back();
  std::vector<double> allBoundaries(maxBin+1);
  int bbIndex = 0;
  allBoundaries[bbIndex++] = -0.5;
  for(int i=0; i<maxBin; i++){
    allBoundaries[bbIndex] = allBoundaries[bbIndex-1]+1.0+utils::DataHisto::getDNLValue(dnlValuesMap, i);
    bbIndex++;
  }

  // take just the filled bins in order to keep usability of indices
  bbIndex = 0;
  m_binBoundaries.resize(bins.size(),0);
  for(auto && b : bins){
    m_binBoundaries[bbIndex++] = allBoundaries[b];
  }

  // maximum bin remains at the same position
  // maybe the shifting does bad things but it generates more stable dark frame maps
  const auto binValues = getBinValuesFilled();
  const auto maxElem = std::max_element(binValues.begin(),binValues.end());
  uint32_t maxIdx = std::distance(binValues.begin(),maxElem);

  // negative bins are not usable
  double diff = bins[maxIdx] - m_binBoundaries[maxIdx] - 0.5;
  if((diff + m_binBoundaries.front()) < -0.5){
    diff = 0.5 - m_binBoundaries.front();
  }
  //small shift to fix noise peak position
  for(auto & bb : m_binBoundaries){
    bb += diff;
  }
}


void DataHisto::fillBufferToHistoMap()
{
  for (size_t b=0; b<BUFS; ++b) {
    if (binValsBuffer[b]!=0) {
      addN(b,binValsBuffer[b]);
    }
  }
  std::fill(binValsBuffer,binValsBuffer+BUFS,0);
}


//int DataHisto::getMaximumBin()
//{
//}

std::string computeBinningRangeStr(const DataHisto::DNLEvalMap & evalMap)
{
  std::vector<int> minBins(evalMap.size());
  std::vector<int> maxBins(evalMap.size());
  int idx = 0;
  for(auto && evalItem : evalMap){
    auto dnlBins = getKeys(evalItem.second.dnlMap);
    auto minMaxId = std::minmax_element(dnlBins.begin(),dnlBins.end());
    minBins[idx] = *minMaxId.first;
    maxBins[idx] = *minMaxId.second;
    idx++;
  }

  int minBin = *std::min_element(minBins.begin(),minBins.end());
  int maxBin = *std::max_element(maxBins.begin(),maxBins.end());
  auto minStats = utils::getMeandAndRMS(minBins);
  auto maxStats = utils::getMeandAndRMS(maxBins);

  std::ostringstream out;

  out << "BinRange = " << minBin << " - " << maxBin;
  out << " MeanMin = " << minStats.mean << "+-" << minStats.rms;
  out << " MeanMax = " << maxStats.mean << "+-" << maxStats.rms;

  return out.str();
}

// value occuring at most in vector
uint16_t calcModeOfVector(const std::vector<uint16_t> & values)
{
  DataHisto histo;
  histo.addToBuf(values.begin(),values.end());
  histo.fillBufferToHistoMap();
  histo.print();
  return histo.getMaxBin();
}


void dumpInjectionCalibrationData(const std::string & fileName, uint32_t coarseGainSetting, const std::string & injectionMode, const InjectionCalibrationData & injectionCalibrationFactors)
{
  string fullFileName = fileName;
  if(fileName.rfind(".txt") == std::string::npos){
    fullFileName += ".txt";
  }


  ofstream out(fullFileName);

  out << "## Injection Calibration Data\n";
  out << "#Time:\t" << getLocalTimeStr() << "\n";
  out << "#Injection Mode:\t"<< injectionMode <<"\n";
  out << "#QInjEn10fF:\t"<< coarseGainSetting <<"\n";
  out << "#NumPixels:\t"<< injectionCalibrationFactors.size() <<"\n";
  out << "#Pixel\tFactor\tFactorErr\tSlope\tSlopeError\tGain(eV)\tGainErr(eV)\n";

  for(auto && data : injectionCalibrationFactors){
    for(auto && value : data){
      out << value << "\t";
    }
    out << "\n";
  }
  out.close();

  cout << "InjectionCalibrationData saved to: " << fullFileName << endl;
}


std::pair<std::string,std::vector<double>> importInjectionCalibrationFactors(const std::string & fileName)
{
  ifstream in(fileName);
  if(!in.is_open()){
    cout << "Could not open FitResults file: " << fileName << "\n";
    return {"",std::vector<double>()};
  }

  std::vector<double> factors(65536,0.0);

  string line;

  bool header = true;
  std::string coarseGain;
  while (getline(in, line)) {
    if(line.empty()) continue;

    if(header && line[0] == '#'){
      size_t pos = line.find("#CoarseGain:");
      if(pos != string::npos){
        coarseGain = std::stod(line.substr(pos));
        cout << "Found GainSetting Information = " << coarseGain << endl;
      }
      continue;
    }else{
      header = false;
    }

    if(!header){

      std::vector<double> values;
      utils::split(line,'\t',values);
      const auto & pixel = values[0];
      const auto & factor = values[1];
      factors[pixel] = factor;
    }
  }
  return {coarseGain,factors};
}

std::pair<std::string,InjectionCalibrationData> importInjectionCalibrationData(const std::string & fileName)
{
  InjectionCalibrationData calibData(65536);

  ifstream in(fileName);
  if(!in.is_open()){
    cout << "Could not open FitResults file: " << fileName << "\n";
    return {"",calibData};
  }

  string line;

  bool header = true;
  std::string coarseGain;
  while (getline(in, line)) {
    if(line.empty()) continue;

    if(header && line[0] == '#'){
      size_t pos = line.find("#CoarseGain:");
      if(pos != string::npos){
        coarseGain = std::stod(line.substr(pos));
        cout << "Found GainSetting Information = " << coarseGain << endl;
      }
      continue;
    }else{
      header = false;
    }

    if(!header){
      std::vector<double> values;
      utils::split(line,'\t',values);
      auto & pixel = values[0];
      calibData[pixel][0] = pixel;
      calibData[pixel][1] = values[1];
      calibData[pixel][2] = values[2];
      calibData[pixel][3] = values[3];
    }
  }
  return {coarseGain,calibData};
}


void dumpFitResultsToASCII(const std::string & fileName, const std::vector<uint32_t> & pixels, const ADCGainFitResultsVec & fitResults)
{
  string fullFileName = fileName;
  if(fileName.rfind(".txt") == std::string::npos){
    fullFileName += ".txt";
  }

  ofstream out(fullFileName);

  ADCGainFitResults::printHeader(out,positionVectorToList(pixels));

  for(auto && res : fitResults){
   for(auto && val : res){
     out << val << "\t";
   }
    out << "\n";
  }
  out.close();

  cout << "FitResults saved to: " << fullFileName << endl;
}


void dumpFitResultsToASCII(const std::string & fileName, const ADCGainFitResultsMap &fitResults)
{
  string fullFileName = fileName;
  if(fileName.rfind(".txt") == std::string::npos){
    fullFileName += ".txt";
  }

  ofstream out(fullFileName);

  ADCGainFitResults::printHeader(out,positionVectorToList(utils::getKeys(fitResults)));

  for(auto && item : fitResults){
    for(auto && val : item.second){
      out << val << "\t";
    }
    out << "\n";
  }
  out.close();

  cout << "FitResults saved to: " << fullFileName << endl;
}

ADCGainFitResultsMap importADCGainFitResultsFromASCII(const std::string & fileName)
{
  ADCGainFitResultsMap resultMap;
  ifstream in(fileName);
  if(!in.is_open()){
    cout << "Could not open FitResults file: " << fileName << "\n";
    return resultMap;
  }

  std::string line;
  bool header = true;

  while (getline(in, line))
  {
    if(line.empty()) continue;

    if(header && line[0] == '#'){
      continue;
    }else{
      header = false;
    }

    ADCGainFitResult result;

    int idx=0;
    istringstream iss(line);
    iss >> result[idx++];
    iss >> result[idx++];
    iss >> result[idx++];
    iss >> result[idx++];
    iss >> result[idx++];
    iss >> result[idx++];
    iss >> result[idx++];

    size_t pixel = result[0];
    resultMap[pixel] = result;
  }

  return resultMap;
}

} // namespace utils
