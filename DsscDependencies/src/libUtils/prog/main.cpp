#include <list>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <functional>
#include <numeric>
#include <iterator>
#include <unordered_map>
#include <math.h>
#include <algorithm>
#include <fstream>
#include <random>
#include <regex>
#include <string>
#include <chrono>
#include <omp.h>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/moment.hpp>

#include <sys/stat.h>

#include "utils.h"
#include "FitUtils.h"
#include "SimUtils.h"
#include "DataHisto.h"
#include "DsscCalibrationDB.h"
#include "DsscProgressBar.h"
#include "DsscTrainData.hh"
#include "DsscTrainDataProcessor.h"
#include "DsscModuleInfo.h"

using namespace std;
using namespace utils;
using namespace boost::accumulators;


/*
 * g++ -DF1IO -Wall -o test test.cc utils.cpp DataHisto.cpp DsscCalibrationDB.cpp -std=c++11 -lboost_system -fopenmp -O2
 */


void noiseComputationEvaluation()
{
  int numValues = 16;

  int RUNS = 20000;
  int RANGE = 120;
  double mu = 600;
  double sigma = 20.5;
  double xNoise = 0.5;
  double yNoise = 0.02;

  vector<double> sigmas;
  utils::DataHistoVec evalHistos;

  for(int sig = 0; sig < RANGE; sig++){
    sigma += 5;

    sigmas.push_back(sigma);
    utils::DataHisto runHisto;
    for(int run = 0; run < RUNS; run++){

      vector<double> xValues(numValues);
      vector<double> yValues(numValues);
      vector<double> yErf(numValues);
      vector<double> yInvErf(numValues);

      // fill x values with random
      for(int i=0; i<numValues; i++){
        xValues[i] = 55.0 * (i + (rand()%100)/100.0*xNoise);
        double xVal = 1/sqrt(2)*(xValues[i]-mu)/sigma;
        yErf[i] = erf(xVal) + ((rand()%100)/100.0*yNoise);
        yValues[i] =(yErf[i] + 1.0)/2.0;
        yInvErf[i] = sqrt(2.0) * utils::inverseErf(2.0*yValues[i] - 1.0);
      }

      if(run == 0){
        cout << "Sigma = " << sigma << endl;
        for(int i=0; i<numValues; i++){
          cout <<  setw(15) << xValues[i];
        }
        cout << endl;
        for(int i=0; i<numValues; i++){
          cout <<  setw(15) << yValues[i];
        }
        cout << endl;

      }

      //double sigmaCalc = utils::DataHisto::calcNoiseFromStepBorderVector(xValues,yValues);

      //runHisto.add(sigmaCalc*100);
    }
    evalHistos.push_back(runHisto);
  }

  cout << setprecision(5);
  for(int i=0; i<RANGE; i++){
    double mean = evalHistos[i].calcMean();
    cout << i << " Run Sigma = " << setw(8) << sigmas[i] << " / " << setw(8) << mean/100.0 << " +- " << setw(10) <<  evalHistos[i].calcRMS(mean)/100.0 << " Error: " << setw(8) << (sigmas[i] / (mean/100.0) - 1.0) * 100.0 << "%" << endl;
  }

  for(int i=0; i<RANGE; i++){
    cout << i << " Run Sigma = " << setw(8) << sigmas[i] << endl;
    evalHistos[i].print();
  }
}

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

template<typename OutIt>
void sierpinski(int n, OutIt result)
{
    if( n == 0 )
    {
        *result++ = "*";
    }
    else
    {
        list<string> prev;
        sierpinski(n-1, back_inserter(prev));

        string sp(1 << (n-1), ' ');
        result = transform(prev.begin(), prev.end(),
            result,
            [sp](const string& x) { return sp + x + sp; });
        transform(prev.begin(), prev.end(),
            result,
            [sp](const string& x) { return x + " " + x; });
    }
}

template<class T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
    almost_equal(T x, T y, int ulp)
{
    // the machine epsilon has to be scaled to the magnitude of the values used
    // and multiplied by the desired precision in ULPs (units in the last place)
    return std::abs(x-y) < std::numeric_limits<T>::epsilon() * std::abs(x+y) * ulp
    // unless the result is subnormal
           || std::abs(x-y) < std::numeric_limits<T>::min();
}

template <typename OUT_T>
std::vector<OUT_T> fillIdVector(uint8_t * inData, uint64_t numWords, uint64_t startByte)
{
  OUT_T * data = (OUT_T*)(inData + startByte);
  std::vector<OUT_T> idVec(data,data+numWords);
  return idVec;
}

string getDNLTrimPixels(int quarter)
{

    int start1 = quarter * 8192;
    int end1   = ((quarter+1)*8192)-1;
    int start2 = 32768 + (3-quarter)*8192;
    int end2   = 32768 + (4-quarter)*8192-1;
    return to_string(start1)+"-"+to_string(end1) + ";" +
           to_string(start2)+"-"+to_string(end2)    ;
}

template<typename T = uint32_t>
std::string setPrecision(T x, int num){
  std::stringstream ss;
  ss << std::setprecision(log(x)+num) << x;

  std::string out = ss.str();
  auto pos = out.find('.');
  if(pos == std::string::npos)
  {
    out += ".";
    for(int i=0; i<num; i++) out += "0";
  }
  else
  {
    int numChars = out.length() - pos -1;
    if(numChars>=num){
      out = out.substr(0,pos+num+1);
    }else{
      for(int i=numChars; i<num; i++) out += "0";
    }
  }
  return out;
}

std::map<string,string> readFullConfigFile(const string &fileName)
{
  std::map<string,string> fileMap;

  ifstream stream(fileName);
  if(!stream.is_open()){
    return fileMap;
  }

  int lastSlashPos = fileName.rfind("/");
  const string filePath = fileName.substr(0,lastSlashPos);

  string nextFileName;
  getline(stream,nextFileName);
  if (nextFileName[0] != '/') { // check to be compatible to absolute paths also
    nextFileName = filePath + "/" + nextFileName;
  }
  fileMap["seq"] = nextFileName;

  getline(stream,nextFileName);
  if (nextFileName[0] != '/') { // check to be compatible to absolute paths also
    nextFileName = filePath + "/" + nextFileName;
  }
  fileMap["pixel"] = nextFileName;


  getline(stream,nextFileName);
  if (nextFileName[0] != '/') { // check to be compatible to absolute paths also
    nextFileName = filePath + "/" + nextFileName;
  }
  fileMap["jtag"] = nextFileName;

  for(auto && file : fileMap){
    cout << "Loaded fileMap[" << file.first << "] = "<< file.second<< endl;
  }

  return fileMap;
}


double getRandomRealNumber(double min, double max)
{
  static std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());

    std::uniform_real_distribution<double> distribution(min,max);
    return distribution(generator);
}


void printRandomNumbers()
{
  cout << "Random Numbers" << endl;

  for(int i=0; i<10; i++){
    static std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());

    std::uniform_real_distribution<double> distribution(0.0,10.0);
    cout << distribution(generator) << endl;
  }
}

void generateRandomShifts()
{
  for(int i = 0; i<10; i++){
    double value     = getRandomRealNumber(-10.0,10.0);
    double absValue  = fabs(value);
    int floorValue   = floor(absValue);
    double restValue = absValue-floorValue;
    int sign = value/absValue;
    double random = getRandomRealNumber(0.0,1.0);
    int shift     = (random < restValue)? 1 : 0;

    cout << value << " ";
    cout << absValue << " ";
    cout << floorValue << " ";
    cout << restValue << " ";
    cout << sign << " ";
    cout << random << " ";
    cout << shift << " ";
    cout << "result: " << sign*(floorValue + shift) << endl;
  }
}

template<typename T>
bool getSweepVector(std::string str, std::vector<T>& settings, bool randomize = false)
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
    std::smatch m;
    std::regex e (doubleEx);   // matches words beginning by "sub"

    std::cout << "Target sequence: " << str << std::endl;
    std::cout << "Regular expression: /\\b(sub)([^ ]*)/" << std::endl;
    std::cout << "The following matches and submatches were found:" << std::endl;

    while (std::regex_search (str,m,e)) {
      for (auto x:m){
        std::cout << x << " ";
      }
      std::cout << std::endl;
      //s = m.suffix().str();
    }
    static const std::regex ex(doubleEx);
    std::regex_iterator<std::string::iterator> rit (str.begin(),str.end(),ex);
    std::regex_iterator<std::string::iterator> rend;

    while (rit!=rend) {
      settings.push_back(stod(rit->str()));
      rit++;
    }
  }

  if (std::regex_match(str.begin(),str.end(),regex(rangeEx)))
  {
    int numValues = settings.size();

    T settingStep  = 1;
    T startSetting = settings[0];
    T stopSetting  = settings[1];

    if(numValues == 3)
    {
      if (std::regex_match(str.begin(),str.end(),regex(numStepsEx))) {
        int totalSteps = settings[2];
        if (totalSteps<=1) {
          stopSetting = startSetting;
        } else {
          settingStep = (stopSetting-startSetting) / (totalSteps-1);
        }
      } else if (std::regex_match(str.begin(),str.end(),regex(stepSizeEx))) {
        settingStep = settings[2];
      }

      if (!(settingStep>0 || settingStep<0)) { // double
        std::cout <<  "warning Setting step must be different from 0. Otherwise measurement will take forever..." << std::endl;
        return false;
      }

    }

//     std::cout <<  "startSetting is " << startSetting << std::endl;
//     std::cout <<  "stopSetting is "  << stopSetting << std::endl;
//     std::cout << "settingStep is "   << settingStep << std::endl;
    settings.clear();
    T currSetting = startSetting;
    do { // add at least one setting if startSetting == stopSetting
      settings.push_back(currSetting);
      currSetting += settingStep;
    } while (currSetting <= stopSetting);

  } else if(!std::regex_match(str.begin(),str.end(),regex(listEx))){
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

using namespace std::placeholders;


class Test
{
  public :

  float add(int a, int & b) const
  {
    std::cout << "Add called" << a << b << std::endl;
    return a+b;
  }

  std::function<float(int,int&)> funcReturn() const
  {

    return [&] (int a, int b)-> float {
              return this->add(a, b);};
  }

  void test123() const
  {
    auto testFunc = funcReturn();

    int a = 5,b=120;

    testFunc(a,b);
  }
};

void escapeSpaces(std::string & input)
{
  auto pos = input.find(' ');
  while(pos != std::string::npos){
    input.insert(pos,"\\");
    pos = input.find(' ',pos+2);
  }
}


int main()
{
/*
  vector<vector<double> > binValues(2,vector<double>(200,0));
  for(const auto & vec : binValues){
    for(const auto & val : vec){
      cout << val << "/";
    }
    cout << endl;
  }

  double x = 1001;

  cout << setPrecision(x,3) << endl;

  string input = "1;2;3;4;5;6;7;8;9";
  regex integer("(\\+|-)?[[:digit:]]+");

  //printRandomNumbers();

  //printRandomNumbers();

  readFullConfigFile("/home/kirchgessner/karabo/karaboRun/packages/controlDevices/dsscPpt/ConfigFiles/session.conf");

  std::string fileName = "hallo123.txt";
  size_t lastSlashPos = fileName.rfind("/");
  if(lastSlashPos == std::string::npos){
    lastSlashPos = 0;
  }
  const string filePath = fileName.substr(0,lastSlashPos);

  cout << "File Path of " << fileName << " is " <<  filePath << endl;

  string sweepSettings = "1;2;3;4;5;6;7;8;9";
  vector<double> settings;
  sweepSettings = "1;2;3;4;5;6;7;8;9";
  getSweepVector(sweepSettings,settings);

  sweepSettings = "1.4-8.3;1.3";
  getSweepVector(sweepSettings,settings);

  sweepSettings = "1-8:2";
  getSweepVector(sweepSettings,settings);

  sweepSettings = "1-20";
  getSweepVector(sweepSettings,settings);

  sweepSettings = "100";
  getSweepVector(sweepSettings,settings);

  sweepSettings = "1;20";
  getSweepVector(sweepSettings,settings);


  string str = "5;1-20;30-40;45;50-60;70-80;9";
  string test1 = "(\\d+\\-\\d+)";
  string test2 = "((;)(\\d+)(;))";
  string test3 = "(^\\d+;)";
  string test4 = "(;\\d+$)";

  regex ex(test4);


  std::cout << std::endl;

  std::regex_iterator<std::string::iterator> rit (str.begin(),str.end(),ex);
  std::regex_iterator<std::string::iterator> rend;

  while (rit!=rend) {
    cout << rit->str() << endl;
    rit++;
  }

  const Test test;
  test.test123();

  struct stat buffer;
  string name = "Test";
  if (stat (name.c_str(), &buffer) == 0){
    cout << "Directory " << name << "found "  << endl;
  } else{
    cout << "Directory " << name << " not found "  << endl;
  }

  string hallo = " Ich bims 1 lauch";
  cout << hallo << endl;
  escapeSpaces((hallo));
  cout << hallo << endl;

  std::vector<uint8_t> testVec {0,1};
  uint16_t value1 = testVec[0] + (testVec[1]<<8);
  const auto data = testVec.data();
  uint16_t value2 = (*((uint16_t *)&data[0]));

  std::cout << "Value1 = " << value1 << " | " << value2 << std::endl;

  std::string res,testStr = "Hallo Welt\n";
  std::istringstream in(testStr);
  in >> res;

  cout << res << endl;


  std::vector<uint16_t> dataVec1{100,222,144,122,111,002,1,45};
  std::vector<uint16_t> dataVec2(8,0);

  uint8_t * data2 = (uint8_t*)dataVec2.data();
  for(int i = 0; i<8; i++){
    data2[i] = (uint8_t)dataVec1[i];
  }

  for(auto value : dataVec2){
    cout << hex << value << endl;
  }


  vector<uint8_t> values;
  for(int i=0; i<100; i++){
    values.push_back(i);
  }

  auto output = fillIdVector<uint16_t>(values.data(),20,10);
  for(auto && value : output){
    cout  << value << " ";
  }
  cout << endl;

  std::vector<uint8_t> testVecxy(values.data(),values.data()+10);
  cout << dec << testVecxy.size() << endl;

  {
    int SIZE = 20;

    vector<uint16_t> data(SIZE*SIZE);
  int x, y;
  int idx = 0;
    for (y = SIZE - 1; y >= 0; y--) {
      for (x = 0; x < SIZE; x++)
        data[idx++] = ((x & y) ? 0 : 1);
    }


  idx = 0;
  for(int y=0; y <SIZE; y++){
    for(int x=0; x<SIZE; x++){
      cout << " " << (data[idx++] == 0? " " : "x");
    }
    cout << endl;
  }
  }

  const int SAMPLES = 100000;
  const int NUMRUNS = 1;

  for(int run=0; run < NUMRUNS; run++)
  {
    utils::Timer timer;
    utils::DataHisto histo1;

    for(int i=0; i<SAMPLES; i++){
      histo1.add((i/13)%511);
    }
    auto mean = histo1.calcMean();
    cout << "Mean " << mean << endl;
  }

  for(int run=0; run < NUMRUNS; run++)
  {
    utils::Timer timer;
    accumulator_set<uint16_t, stats<tag::mean> > acc;

    for(int i=0; i<SAMPLES; i++){
      acc((i/13)%511);
    }
    cout << "Mean " << mean(acc) << endl;
  }


  cout << "End"<< endl;

  uint32_t x1 = 0xFF01;
  cout << hex << "0x" << x1 << " = 0x" << invertBitEnableValue(x1,32) << dec << endl;

  cout << "Start allocating Data"<< endl;

  vector<utils::DataHisto> pixelHistos(65536);
  vector<vector<uint16_t>> pixelData(65536,vector<uint16_t>(800,5));
  cout << "Start Filling Histograms:"<< endl;

  {
    utils::Timer timer;

#pragma omp parallel for
    for(int i=0; i<65536; i++){
      auto pixelHisto = pixelHistos[i];
      auto pxData = pixelData[i];
      for(int sram=0; sram<800; sram++){
        pixelHisto.add(pxData[sram]);
      }
    }
  }
*/

  //noiseComputationEvaluation();

  //initialize confiugration parameter list before first opening of the DB,
  //will be laoded from data base afterwards
//  SuS::DsscCalibrationSetConfig::updateParameterNames({"numPixels",
//                                                       "active",
//                                                       "gain_eV",
//                                                       "CSA_CapIn",
//                                                       "FC_CapEn"});

//  string component = "DSSC_F1_Test";
//  string version = "Gain_0.5keV_v1.0.0.34";

//  cout << "File Extension= " << utils::getFileExtension("ha.llo/her.hrh.txt123") << endl;


//  vector<string> versions {"v1.0.0",
//                           "v1.0.1",
//                           "v1.0.1",
//                          "v1.0.11",
//                          "v1.10.1",
//                          "v1.2.1",
//                          "v1.22.1",
//                          "v12.9.1",
//                          "v5.0.1",
//                          "v9.0.1"};
//  cout << "Max Version = " << utils::getMaxVersion(versions) << endl;
/*
  // initialize Configuration for Calibration Set
  SuS::DsscCalibrationSetConfig config;
  config.setParamValue("numPixels","4096");
  config.setParamValue("active","J");
  config.setParamValue("gain_eV","0.7");
  config.setParamValue("CSA_CapIn","1");
  config.setParamValue("FC_CapEn","5");

  // open database
  SuS::DsscCalibrationDB calDB("testDB");
  // add new Calibration Set with date time version and componentID (ASIC or Sensor...)
  calDB.addNewCalibrationSetAndSetActive(SuS::DsscCalibrationDB::generateNewConfigID(version,component),
                                         config.getParameterValues());

  auto activeConfig = calDB.getActiveCalibrationSet();

  // Add Calibration Parameters as 1 dim vectors
  std::vector<double> pixelDelaySteps(4096*16,2.0);
  std::vector<double> dnlValues(4096*256,0.3);
  std::vector<double> gainCorrectionValues(4096,0.93);
  activeConfig->addCalibrationParamSet("PixelDelaySteps",pixelDelaySteps);
  activeConfig->addCalibrationParamSet("DNLValues",dnlValues);
  activeConfig->addCalibrationParamSet("GainCorrectionValues",gainCorrectionValues);


  constexpr size_t numValues = 800*65536;
  uint16_t * pixelData = new uint16_t[numValues];
  std::fill(pixelData,pixelData+numValues,55);

  utils::DataHistoVec histoVec1(65536);
  utils::DataHistoVec histoVec2(65536);

  {
    utils::Timer timer1;
    for(size_t frame=0; frame < 800; frame++){
      uint16_t * frameData = pixelData+(frame*65536);
#pragma omp parallel for
      for(size_t px = 0; px <65536; px++){
        histoVec1[px].addToBuf(frameData[px]);
      }
    }
  }
  cout << "Sorting 1 done" << endl;

  {
    utils::Timer timer1;
#pragma omp parallel for
    for(size_t px = 0; px <65536; px++){
      for(size_t frame=0; frame < 800; frame++){
        uint16_t * frameData = pixelData+(frame*65536);
        histoVec1[px].addToBuf(frameData[px]);
      }
    }
  }
  cout << "Sorting 2 done" << endl;


  {
    utils::Timer timer1;
#pragma omp parallel for
    for(size_t px = 0; px <65536; px++){
      histoVec2[px].addToBuf(pixelData+(px*800),800);
    }
  }
  cout << "Sorting 3 done" << endl;

  */

  // select 1-6 threads

  omp_set_dynamic(0);     // Explicitly disable dynamic teams
  omp_set_num_threads(8); // Use 4 threads for all consecutive parallel regions

  DsscTrainData trainData(DsscTrainData::s_trainSize);
  DsscTrainData outTrainData(DsscTrainData::s_trainSize);
  DsscTrainData outTrainData2(DsscTrainData::s_trainSize);
  DsscTrainData outTrainData3(DsscTrainData::s_trainSize);
  trainData.setFormat(utils::DsscTrainData::DATAFORMAT::IMAGE);
  trainData.availableASICs = utils::getUpCountingVector(16);
  trainData.pulseCnt = s_numSram;
  outTrainData.availableASICs = utils::getUpCountingVector(16);
  outTrainData.pulseCnt = s_numSram;
  outTrainData2.availableASICs = utils::getUpCountingVector(16);
  outTrainData2.pulseCnt = s_numSram;
  outTrainData3.availableASICs = utils::getUpCountingVector(16);
  outTrainData3.pulseCnt = s_numSram;

  //uint16_t * outData = outTrainData.imageData();
  uint16_t * inData = trainData.imageData();
  //uint16_t * inDataEnd = inData + 16*800*4096;
  //size_t frame = 5;
  inData[0] = 10;
  //uint32_t imagePixel = 10;

  if(trainData.getFormat() == utils::DsscTrainData::DATAFORMAT::PIXEL){
    // fill frame with frame number % 512
#pragma omp parallel for
    for(size_t asic=0; asic<16; asic++){
      for(size_t px=0; px<s_numAsicPixels; px++){
        for(uint frame = 0; frame<s_numSram; frame++){
          inData[asic * s_numAsicPixels * s_numSram + px * s_numSram + frame] = asic;
        }
      }
    }
  }else{
    // fill frame with pixel number % 512
#pragma omp parallel for
    for(uint frame = 0; frame<s_numSram; frame++){
      for(size_t i=0; i<s_totalNumPxs; i++){
        inData[frame * s_totalNumPxs + i] = getPixelASIC(i);
      }
    }
  }

  utils::DsscTrainDataProcessor::sortDataArray(&trainData,&outTrainData,DsscTrainData::DATAFORMAT::PIXEL);


  auto dataArray4_0 = outTrainData.getAsicDataArray(4,0);
  auto dataArray4_4 = outTrainData.getAsicDataArray(4,4);
  auto dataArray6_4 = outTrainData.getAsicDataArray(6,4);


  cout << " Asic 4 frame 0 " << dataArray4_0[0]  << " _ " << dataArray4_0[10]<< endl;
  cout << " Asic 4 frame 4 " << dataArray4_4[0]  << " _ " << dataArray4_0[10]<< endl;
  cout << " Asic 6 frame 4 " << dataArray6_4[0]  << " _ " << dataArray6_4[10]<< endl;
  /*
  std::vector<uint16_t> values(s_numSram,0);
  std::vector<uint16_t> imageValues(s_totalNumPxs,0);


  auto pixelDataArr = trainData.getPixelDataPixelWise(imagePixel);
  std::copy(pixelDataArr.begin(),pixelDataArr.end(),values.begin());


  auto imageDataArr = trainData.getImageDataArray(7);
  std::copy(imageDataArr.begin(),imageDataArr.end(),imageValues.begin());

  for(int numThreads=1; numThreads<10; numThreads++)
  {
    omp_set_num_threads(numThreads);

    utils::Timer timer;
    for(int x=0; x<10; x++){
      utils::DsscTrainDataProcessor::sortDataArray(&trainData,&outTrainData,DsscTrainData::DATAFORMAT::PIXEL);
    }
    cout << "Number of threads 16 " << numThreads << endl;
  }
  */

  omp_set_num_threads(4);
/*
  {
    cout << "copy" << endl;
    utils::Timer timer;
    for(int x=0; x<100; x++){
      std::copy(inData,inDataEnd,outData);
    }
  }

  {
    cout << "copy openMp 65536" << endl;
    utils::Timer timer;
    for(int x=0; x<100; x++){
#pragma omp parallel for
      for(size_t pixelIdx=0; pixelIdx<s_totalNumPxs; pixelIdx++){
        auto inPixelData = inData + s_numSram * pixelIdx;
        auto inPixelDataEnd = inPixelData + s_numSram;
        auto outPixelData = outData + s_numSram * frame;
        std::copy(inPixelData,inPixelDataEnd,outPixelData);
      }
    }
  }

  {
    cout << "copy openMp 800" << endl;
    utils::Timer timer;
    for(int x=0; x<100; x++){
#pragma omp parallel for
      for(size_t frame=0; frame<s_numSram; frame++){
        auto inPixelData = inData + s_totalNumPxs * frame;
        auto inPixelDataEnd = inPixelData + s_totalNumPxs;
        auto outPixelData = outData + s_totalNumPxs * frame;
        std::copy(inPixelData,inPixelDataEnd,outPixelData);
      }
    }
  }

  {
    cout << "sort pixel wise image wise to pixel wise" << endl;
    utils::Timer timer;
    for(int x=0; x<100; x++){
#pragma omp parallel for
      for(size_t pixelIdx=0; pixelIdx<s_totalNumPxs; pixelIdx++){
        auto inPixelData = trainData.getPixelDataPixelWise(pixelIdx);
        auto outPixelData = outTrainData.getPixelDataPixelWise(pixelIdx);
        std::copy(inPixelData.begin(),inPixelData.end(),outPixelData.begin());
      }
    }
  }

  {
    cout << "sort image wise image wise to pixel wise" << endl;
    utils::Timer timer;
    for(int x=0; x<100; x++){

#pragma omp parallel for
      for(size_t sram=0; sram<s_numSram; sram++){
        auto inImageData = trainData.getImageDataArray(sram);
        auto outImageData = outTrainData.getImageDataArray(sram);
        std::copy(inImageData.begin(),inImageData.end(),outImageData.begin());
      }
    }
  }


  {
    cout << "sort pixel wise pixel wise to pixel wise" << endl;
    utils::Timer timer;
    for(int x=0; x<100; x++){
#pragma omp parallel for
      for(size_t pixelIdx=0; pixelIdx<s_totalNumPxs; pixelIdx++){
        auto inPixelData = outTrainData.getPixelDataPixelWise(pixelIdx);
        auto outPixelData = outTrainData2.getPixelDataPixelWise(pixelIdx);
        std::copy(inPixelData.begin(),inPixelData.end(),outPixelData.begin());
      }
    }
  }

  {
    cout << "sort image wise pixel wise to pixel wise" << endl;
    utils::Timer timer;
    for(int x=0; x<100; x++){

#pragma omp parallel for
      for(size_t sram=0; sram<s_numSram; sram++){
        auto inImageData = outTrainData.getImageDataArray(sram);
        auto outImageData = outTrainData2.getImageDataArray(sram);
        std::copy(inImageData.begin(),inImageData.end(),outImageData.begin());
      }
    }
  }


  {
    cout << "sort pixel wise pixel wise to image wise" << endl;
    utils::Timer timer;
    for(int x=0; x<100; x++){
#pragma omp parallel for
      for(size_t pixelIdx=0; pixelIdx<s_totalNumPxs; pixelIdx++){
        auto inPixelData = outTrainData.getPixelDataPixelWise(pixelIdx);
        auto outPixelData = trainData.getPixelDataPixelWise(pixelIdx);
        std::copy(inPixelData.begin(),inPixelData.end(),outPixelData.begin());
      }
    }
  }

  {
    cout << "sort image wise pixel wise to image wise" << endl;
    utils::Timer timer;
    for(int x=0; x<100; x++){

#pragma omp parallel for
      for(size_t sram=0; sram<s_numSram; sram++){
        auto inImageData = outTrainData.getImageDataArray(sram);
        auto outImageData = trainData.getImageDataArray(sram);
        std::copy(inImageData.begin(),inImageData.end(),outImageData.begin());
      }
    }
  }

 cout << "done" << endl;
  utils::DataHistoVec histoVec(s_totalNumPxs);
  for(size_t i=0; i<utils::s_totalNumPxs; i++){
    const auto pxData = inData + s_numSram*i;
    histoVec[i].addToBuf(pxData,pxData+s_numSram);
  }

  for(size_t i=0; i<utils::s_totalNumPxs; i++){
    const auto pxData = inData + s_numSram*i;
    histoVec[i].addToBuf(pxData,pxData+s_numSram);
  }
#pragma omp parallel for
  for(size_t i=0; i<s_totalNumPxs; i++){
      histoVec[i].toZero();
  }
  {
    cout << "fill histo" << endl;
    utils::Timer timer;

    for(int x=0; x<100; x++){
      for(size_t i=0; i<utils::s_totalNumPxs; i++){
        const auto pxData = inData + s_numSram*i;
        histoVec[i].addToBuf(pxData,pxData+s_numSram);
      }
    }
    for(size_t i=0; i<s_totalNumPxs; i++){
      histoVec[i].fillBufferToHistoMap();
    }
    cout << "histoVec[0].count() = " << histoVec[0].getCount() << endl;
  }
#pragma omp parallel for
  for(size_t i=0; i<s_totalNumPxs; i++){
      histoVec[i].toZero();
  }
  {
    cout << "fill histo 65536" << endl;
    utils::Timer timer;

    for(int x=0; x<100; x++){

#pragma omp parallel for
    for(size_t i=0; i<utils::s_totalNumPxs; i++){
      const auto pxData = inData + s_numSram*i;
      histoVec[i].addToBuf(pxData,pxData+s_numSram);
    }

    }
#pragma omp parallel for
    for(size_t i=0; i<s_totalNumPxs; i++){
      histoVec[i].fillBufferToHistoMap();
    }

    cout << "histoVec[0].count() = " << histoVec[0].getCount() << endl;

  }


#pragma omp parallel for
  for(size_t i=0; i<s_totalNumPxs; i++){
      histoVec[i].toZero();
  }


  utils::DataHistoVec histoVec(s_totalNumPxs);
  for(size_t i=0; i<s_totalNumPxs; i++){
    auto pixelDataArr = trainData.getPixelDataPixelWise(i);
    histoVec[i].addToBuf(pixelDataArr.begin(),pixelDataArr.end());
    histoVec[i].fillBufferToHistoMap();
  }
  for(size_t i=0; i<s_totalNumPxs; i++){
    histoVec[i].toZero();
  }

  {
    cout << "fill histo image wise with fill" << endl;
    utils::Timer timer;

    for(int x=0; x<100; x++){
#pragma omp parallel for
      for(size_t i=0; i<s_totalNumPxs; i++){
        auto pixelDataArr = trainData.getPixelDataPixelWise(i);
        histoVec[i].addToBuf(pixelDataArr.begin(),pixelDataArr.end());
        histoVec[i].fillBufferToHistoMap();
      }
    }
  }



#pragma omp parallel for
  for(size_t i=0; i<s_totalNumPxs; i++){
      histoVec[i].toZero();
  }

  {
    cout << "fill histo pixel wise with fill" << endl;
    utils::Timer timer;
    for(int x=0; x<100; x++){
#pragma omp parallel for
      for(size_t i=0; i<s_totalNumPxs; i++){
        auto pixelDataArr = outTrainData.getPixelDataPixelWise(i);
        histoVec[i].addToBuf(pixelDataArr.begin(),pixelDataArr.end());
        histoVec[i].fillBufferToHistoMap();
      }
    }
  }

  */

}


/*
 * g++ -Wall -o test test.cc -std=c++11 -lboost_system
 */
