#include "utils.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

using namespace std;

namespace utils
{
Timer globalTimer("Global Timer duration");
Timer *Timer::g_Timer = &globalTimer;

Timer::Timer()
 :start(chrono::high_resolution_clock::now())
{}

Timer::Timer(const std::string & endText)
 : Timer()
{
  m_endText = endText;
}


Timer::~Timer()
{
  unsigned long time = chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now()-start).count();
  cout << m_endText << endl;
  cout << "Lapsed time was " << time << " nanoseconds (" << time/1e6 << " miliseconds)" << endl;
}

void Timer::restart(){
  start = chrono::high_resolution_clock::now();
}

string getLocalTimeStr()
{
    time_t t = time(nullptr);
    stringstream ss;
    char tmstr[100];
    strftime(tmstr,sizeof(tmstr),"%Y_%m_%d-%T",localtime(&t));
    ss << tmstr;
    return ss.str();
}

int getLocalTimeID()
{
    time_t t = time(nullptr);
    stringstream ss;
    char tmstr[100];
    strftime(tmstr,sizeof(tmstr),"%Y%m%d%T",localtime(&t));
    ss << tmstr;
    string timeIDStr = ss.str();
    return stoi(timeIDStr);
}

string getHighPrecisionTimeStr()
{
    chrono::high_resolution_clock::time_point tp = chrono::high_resolution_clock::now();
    chrono::high_resolution_clock::duration dtn  = tp.time_since_epoch();

    uint64_t microseconds = (double)1E6 * dtn.count() * chrono::high_resolution_clock::period::num / chrono::high_resolution_clock::period::den;

    return to_string(microseconds) + " microseconds";
}
//------------------------------------------------------------------------
// inialize static sorter maps
std::array<uint,s_totalNumPxs> genImagePixelMap()
{
  std::array<uint,s_totalNumPxs> pixelMap;

#pragma omp parallel for
  for(size_t asic=0; asic<s_numAsics; asic++){
    int offs = asic*s_numAsicPixels;
    for(size_t asicPixel=0; asicPixel<s_numAsicPixels; asicPixel++){
      pixelMap[offs++] = utils::calcImagePixel(asic,asicPixel);
    }
  }
  return pixelMap;
}

std::array<uint,s_totalNumPxs> genAsicPixelMap()
{
  std::array<uint,s_totalNumPxs> pixelMap;

#pragma omp parallel for
  for(size_t pixel = 0; pixel < s_totalNumPxs; pixel++){
    pixelMap[pixel] = utils::calcASICPixel(pixel);
  }

  return pixelMap;
}

std::array<uint,s_totalNumPxs> genAsicImageMap()
{
  std::array<uint,s_totalNumPxs> pixelMap;

#pragma omp parallel for
  for(int asic=0; asic<16; asic++){
    auto * asicPixelMap = pixelMap.data() + asic*s_numAsicPixels;
    for(size_t pixel = 0; pixel < s_numAsicPixels; pixel++){
      int col = pixel%64;
      int row = pixel/64;
      asicPixelMap[pixel] = row * 512 + col;
    }
  }

  return pixelMap;
}


std::array<uint,s_totalNumPxs> genPixelAsicMap()
{
  std::array<uint,s_totalNumPxs> pixelMap;

#pragma omp parallel for
  for(size_t pixel = 0; pixel < s_totalNumPxs; pixel++){
    pixelMap[pixel] = utils::getPixelASIC(pixel);
  }

  return pixelMap;
}


std::array<uint,s_totalNumPxs> genDataPixelMap()
{
  std::array<uint,s_totalNumPxs> pixelMap;

#pragma omp parallel for
  for(size_t pixel = 0; pixel < s_totalNumPxs; pixel++){
    pixelMap[s_imagePixelMap[pixel]] = pixel;
  }

  return pixelMap;
}

std::array<uint,s_totalNumPxs> getUpCountingMap()
{
  std::array<uint,s_totalNumPxs> pixelMap;

#pragma omp parallel for
  for(size_t pixel = 0; pixel < s_totalNumPxs; pixel++){
    pixelMap[pixel] = pixel;
  }

  return pixelMap;
}


//------------------------------------------------------------------------

std::string calcReadoutWindowFromStartPixel(int pixel, int windowSize)
{
  std::string resultString;
  int pixel_leftEdge = pixel;
  for (int i=0; i<windowSize; ++i)
  {
    if (i!=0)
    {
      resultString.append(";");
    }
    resultString.append(to_string(pixel_leftEdge));
    resultString.append("-");

    int col = pixel_leftEdge % 64;
    int pixel_rightEdge = pixel_leftEdge + (col < 55 ? windowSize : 63-col);
    resultString.append(to_string(pixel_rightEdge));
    pixel_leftEdge += 64;
    if (pixel_leftEdge > 4095) break;
  }
  //cout << "calcReadoutWindowFromStartPixel, pixel: " << pixel << ", windowSize " << windowSize << " - " << resultString << endl;
  return resultString;
}


void expandTo16BitData(const uint8_t * dataIn, uint16_t * dataOut, size_t numValues)
{
#pragma omp parallel for
  for(size_t idx=0; idx<numValues; idx++){
    dataOut[idx] = dataIn[idx];
  }
}

string setPrecision(double x, int num)
{
  stringstream ss;
  ss << setprecision(log(x)+num) << x;

  string out = ss.str();
  auto pos = out.find('.');
  if(pos == string::npos)
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


string positionStringVectorToList(const vector<string> & positions, const std::string & delim)
{
  vector<string> rangeList;
  unsigned i=0;
  string newRange;

  if (positions.size()==1)
    return positions.front();

  while(i < positions.size())
  {
    newRange = positions.at(i);
    int lastVal = stoi(newRange);
    unsigned j=i+1;

    while(j<positions.size()){
      if(abs(lastVal-stoi(positions.at(j))) == 1){
        lastVal = stoi(positions.at(j));
        j++;
      }else{
        break;
      }
    }

    if(j-i>1){
      newRange += "-"+ positions.at(j-1);
    }

    rangeList.push_back(newRange);
    i=j;
  }
  return join(rangeList,delim);
}


vector<string> positionListToStringVector(const string &positions)
{
#ifdef DEBUG
  cout << "INFO positionListToStringVector: " << positions << endl;
#endif

  vector<string> pos;

  if (positions.length()==0){
    cout << "ERROR utils: Positions String empty" << endl;
    return pos;
  }
  vector<string> rangeList;
  split(positions,';',rangeList);
  if (rangeList.size() == 0){
    cout << "ERROR utils: RangeList Size zero" << endl;
    return pos;
  }

  for (const auto & activeRange :rangeList)
  {
    vector<string> startEnd;
    split(activeRange,'-',startEnd);

    if (startEnd.size() == 1)
    {
      pos.push_back(startEnd.front());
    }
    else if (startEnd.size() == 2)
    {
      int start = stoi(startEnd.front());
      int end = stoi(startEnd.at(1));

      if (start <= end)
      {
        for (int i=start; i <= end; i++)
          pos.push_back(to_string(i));
      }
      else
      {
        for (int i=start; i >= end; i--)
          pos.push_back(to_string(i));
      }
    }
    else
    {
      cout << "ERROR utils: String format invalid" << endl;
    }
  }

#ifdef DEBUG
  cout << "INFO positionListToStringVector: " << pos.size() << endl;
#endif
  return pos;
}

std::string stringVectorToList(const std::vector<std::string> & positions, const string & delim)
{
  std::string result;
  for (auto && s : positions) { result += s + delim; }
  result.pop_back();
  return result;
}


void replace(std::string & input, const std::string & before, const std::string & after){
  auto pos = input.find(before);
  if(pos == std::string::npos) return;

  int n = before.length();
  input.replace(pos,n,after);
}


vector<string> &split(istream &in, char delim, vector<string> &elems, int start)
{
    string item;
    int cnt =0;
    while (cnt < start){ getline(in, item, delim); cnt++;}

    while (getline(in, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


vector<int> &split(istream &in, char delim, vector<int> &elems, int start)
{
    string item;
    int cnt =0;
    while (cnt < start){ getline(in, item, delim); cnt++;}

    int value;
    while (getline(in, item, delim)) {
      istringstream iss(item);
      iss >> value;
      elems.push_back(value);
    }
    return elems;
}


vector<uint16_t> &split(istream &in, char delim, vector<uint16_t> &elems, int start)
{
    string item;
    int cnt =0;
    while (cnt < start){ getline(in, item, delim); cnt++;}

    uint16_t value;
    while (getline(in, item, delim)) {
      istringstream iss(item);
      iss >> value;
      elems.push_back(value);
    }
    return elems;
}


vector<uint32_t> &split(istream &in, char delim, vector<uint32_t> &elems, int start)
{
    string item;
    int cnt =0;
    while (cnt < start){ getline(in, item, delim); cnt++;}

    uint32_t value;
    while (getline(in, item, delim)) {
      istringstream iss(item);
      iss >> value;
      elems.push_back(value);
    }
    return elems;
}


vector<uint64_t> &split(istream &in, char delim, vector<uint64_t> &elems, int start)
{
    string item;
    int cnt =0;
    while (cnt < start){ getline(in, item, delim); cnt++;}

    uint64_t value;
    while (getline(in, item, delim)) {
      istringstream iss(item);
      iss >> value;
      elems.push_back(value);
    }
    return elems;
}


vector<double> &split(istream &in, char delim, vector<double> &elems, int start)
{
    string item;
    int cnt =0;
    while (cnt < start){ getline(in, item, delim); cnt++;}

    double value;
    while (getline(in, item, delim)) {
      istringstream iss(item);
      iss >> value;
      elems.push_back(value);
    }
    return elems;
}



vector<string> &split(const string &s, char delim, vector<string> &elems, int start)
{
    stringstream ss(s);
    return split(ss,delim,elems,start);
}


vector<int> &split(const string &s, char delim, vector<int> &elems, int start)
{
    stringstream ss(s);
    return split(ss,delim,elems,start);
}

vector<uint16_t> &split(const string &s, char delim, vector<uint16_t> &elems, int start)
{
    stringstream ss(s);
    return split(ss,delim,elems,start);
}

vector<uint32_t> &split(const string &s, char delim, vector<uint32_t> &elems, int start)
{
    stringstream ss(s);
    return split(ss,delim,elems,start);
}

vector<uint64_t> &split(const string &s, char delim, vector<uint64_t> &elems, int start)
{
    stringstream ss(s);
    return split(ss,delim,elems,start);
}


vector<double> &split(const string &s, char delim, vector<double> &elems, int start)
{
    stringstream ss(s);
    return split(ss,delim,elems,start);
}

vector<uint32_t> split(const std::string &s, char delim, int start)
{
  stringstream ss(s);
  vector<uint32_t> elems;
  return split(ss,delim,elems,start);
}


std::string & stringReplace(string &input, char find, char repl)
{
  size_t pos = 0;
  size_t len = input.length();
  while(pos<len){
    pos = input.find(find,pos);
    if(pos == string::npos){
      break;
    }
    string out = "";
    out += repl;
    input.replace(pos,1,out);
  }
  return input;
}


std::string stringReplace(const string &input, char find, char repl)
{
  std::string output = input;
  return stringReplace(output,find,repl);
}

std::string & stringReplace(std::string &input, const std::string & find, const std::string & repl)
{
  size_t pos = 0;
  size_t len = input.length();
  size_t findLen = find.length();
  while(pos<len){
    pos = input.find(find,pos);
    if(pos == string::npos){
      break;
    }
    input.replace(pos,findLen,repl);
  }
  return input;
}

std::string stringReplace(const std::string &input, const std::string & find, const std::string & repl)
{
  std::string output = input;
  return stringReplace(output,find,repl);
}



string join(const vector<string> & positions, string delim)
{
  if(positions.empty()) return "";

  string positionsString = positions.front();
  for(unsigned i=1; i<positions.size(); i++){
    positionsString += delim;
    positionsString += positions.at(i);
  }

  return positionsString;
}


double convertToUINT16(std::vector<uint16_t> &output, const std::vector<double> & input)
{
  const size_t numValues = input.size();
  if(output.size() != numValues){
    output.resize(numValues);
  }

  auto minmaxit = std::minmax_element(input.begin(),input.end());
  double min = *minmaxit.first;
  double max = *minmaxit.second;

  double diff = max-min;
  min -= diff*0.1;
  max += diff*0.1;
  diff = max-min;

  double range = std::log(diff)/std::log(10);
  double scaleFac = 1.0;
  if(range < 1){
    int exp = ceil(-range)+1;
    scaleFac = pow(10,exp);
  }else if(max>(1<<16)){
    int exp = ((int)(ceil(range)-4));
    if(exp > 0){
      scaleFac = 1.0/pow(10.0,exp);
    }
  }

#pragma omp parallel for
  for(size_t idx = 0; idx < numValues; idx++){
    output[idx] = scaleFac*input[idx];
  }

  return scaleFac;
}


bool checkFileExists(const std::string & fileName)
{
  ifstream inFile(fileName, ifstream::in|fstream::binary);
  bool exists = inFile.is_open();
  inFile.close();
  return exists;
}

//output with . : data.txt -> .txt
std::string getFileExtension(const std::string & fileName)
{
  size_t lastPointPos = fileName.rfind(".");
  if(lastPointPos == std::string::npos){
    return "";
  }

  return fileName.substr(lastPointPos);
}

std::string changeFileExtension(const std::string& fileName, const std::string & newExt)
{
  auto output = fileName;
  return changeFileExtension(output,newExt);
}


std::string& changeFileExtension(std::string & fileName, const std::string & newExt)
{
  if(fileName.find(".") == string::npos){
    fileName += newExt;
    return fileName;
  }

  auto oldExt = getFileExtension(fileName);
  auto newName = stringReplace(fileName,oldExt,newExt);
  if(newName.find(newExt) == string::npos){
    newName += newExt;
  }
  fileName = newName;
  return fileName;
}


std::string getBaseFileName(const std::string & fileName)
{
  size_t lastPointPos = fileName.rfind(".");
  size_t lastSlashPos = fileName.rfind("/");
  if(lastSlashPos == std::string::npos){
    lastSlashPos = 0;
  }

  size_t len = lastPointPos-lastSlashPos-1;
  return fileName.substr(lastSlashPos+1,len);
}

std::string getFileName(const std::string & fileName)
{
  size_t lastSlashPos = fileName.rfind("/");
  if(lastSlashPos == std::string::npos){
    lastSlashPos = 0;
  }

  size_t len = fileName.length()-lastSlashPos-1;
  return fileName.substr(lastSlashPos+1,len);
}


std::string getFilePath(const std::string & fileName)
{
  size_t lastSlashPos = fileName.rfind("/");
  if(lastSlashPos == std::string::npos){
    return ".";
  }
  return fileName.substr(0,lastSlashPos);
}


void fileCopy(const string & srcfileName, const string & dstFileName)
{
  ifstream  src(srcfileName, fstream::binary);
  ofstream  dst(dstFileName, fstream::trunc|fstream::binary);
  dst << src.rdbuf();
}

void fileMove(const string & srcfileName, const string & dstFileName)
{
  auto result= rename( srcfileName.c_str() , dstFileName.c_str() );
  if ( result != 0 )
    cerr << "Utils: Error renaming file"  << endl;
}

void fileDelete(const string & srcfileName)
{
  auto result = remove(srcfileName.c_str());
  if ( result != 0 )
    cerr << "Utils: Error deleting file"  << endl;
}

void fileBackup(const std::string & srcFileName)
{
  if(!checkFileExists(srcFileName)) return;

  string buFileName = getFilePath(srcFileName) + "/";
  buFileName += getBaseFileName(srcFileName) + "_" + getLocalTimeStr();
  buFileName += getFileExtension(srcFileName);

  fileCopy(srcFileName,buFileName);
}



StatsVec getMeandAndRMSVector(const StatsAccVec & statsAccumulators)
{
  StatsVec statsVector;
  statsVector.reserve(statsAccumulators.size());
  for(const auto & accs : statsAccumulators){
    statsVector.emplace_back(accs.calcStats());
  }
  return statsVector;
}

std::vector<double>& limitOutliersOfVector(std::vector<double> & inputValues, double rmsSigma)
{
  const auto statVals = getMeandAndRMS(inputValues);
  double minVal = statVals.mean- rmsSigma * statVals.rms;
  double maxVal = statVals.mean+ rmsSigma * statVals.rms;

  for(auto && value : inputValues){
    if(value < minVal) value = minVal;
    if(value > maxVal) value = maxVal;
  }

  return inputValues;
}

std::vector<double> removeOutliersFromVector(const std::vector<double> & inputValues, double rmsSigma)
{
  const auto statVals = getMeandAndRMS(inputValues);
  double minVal = statVals.mean- rmsSigma * statVals.rms;
  double maxVal = statVals.mean+ rmsSigma * statVals.rms;

  std::vector<double> goodValues;

  for(auto && value : inputValues){
    if(value < minVal) continue;
    if(value > maxVal) continue;

    goodValues.push_back(value);
  }

  return goodValues;
}

std::vector<double> removeOutliersFromVectorIt(const std::vector<double> & inputValues, double rmsSigma)
{
  size_t lastSize = inputValues.size();
  auto goodValues = removeOutliersFromVector(inputValues,rmsSigma);

  do{
    lastSize = goodValues.size();
    goodValues = removeOutliersFromVector(goodValues,rmsSigma);
  }while(lastSize != goodValues.size());

  return goodValues;
}


//################################################################
  string ipToString(uint32_t addr)
  {
    string ip;
    int sh = 24;
    ip = to_string((int)((addr>>sh)& 0xFF));
    for(int i=0; i<3; i++){
      sh -= 8;
      ip += "." + to_string((int)((addr>>sh)& 0xFF));
    }
    //cout << "INFO IpToString: " << addr << "/" << ip << endl;
    return ip;
  }


  uint32_t ipToInt(const string & addr)
  {
    uint32_t ip = 0;
    int part;
    char ignore;

    istringstream iss(addr.c_str(),istringstream::in);

    iss >> dec;

    for(int i=0; i<3; i++){
      iss >> part;
      iss >> ignore;
      ip <<= 8;
      ip +=  part & 0xFF;
    }
    iss >> part;

    ip <<= 8;
    ip +=  part & 0xFF;

    //cout << "INFO IpToInt: " << addr << "/" << ip << endl;
    return ip;
  }


  string macToString(uint64_t addr)
  {
    string mac;
    int sh = 40;
    int part = (addr>>sh) & 0xFF;
    {
    stringstream stream;
    stream << hex << part;
    mac = stream.str();
    }
    for(int i=0; i<5; i++){
      sh -= 8;
      part = (addr>>sh) & 0xFF;
      stringstream stream;
      stream << hex << part;

      mac += ":" + stream.str();
    }

    //cout << "INFO MacToString: " << addr << "/" << mac << endl;

    return mac;
  }


  uint64_t macToInt(const string & addr)
  {
    uint64_t mac = 0;
    int part;
    char ignore;

    istringstream iss(addr.c_str(),istringstream::in);

    iss >> hex;

    for(int i=0; i<5; i++){
      iss >> part;
      iss >> ignore;
      mac <<= 8;
      mac +=  part & 0xFF;
    }
    iss >> part;

    mac <<= 8;
    mac +=  part & 0xFF;

    //cout << "INFO MacToInt: " << addr << "/" << mac << endl;
    return mac;
  }


  float convertMAX6673values(int low, int high)
  {
    float ratio = (float)low / high;
    float temp_converted = -200*(0.85-ratio)*(0.85-ratio)*(0.85-ratio)+(425*ratio)-273;
    return temp_converted;
  }

  int calcASICPixel(int imagePixel)
  {
    int row = imagePixel/512;
    int col = imagePixel%512;

    int asicRow = row/64;
    int asicCol = col/64;

    row -= asicRow*64;
    col -= asicCol*64;

    int asicPixel = row*64+col;
    return asicPixel;
  }


  int calcImagePixel(int asic, int asicPixel)
  {
    int rowOffset = (asic>7)?  64 : 0;
    int colOffset = (asic%8) * 64;

    int row = asicPixel/64;
    int col = asicPixel%64;

    return (row + rowOffset)*512 + col + colOffset;
  }


  int getPixelASIC(int imagePixel)
  {
    int row = imagePixel/512;
    int col = imagePixel%512;
    int asicRow = row/64;
    int asicCol = col/64;

    return asicRow*8+asicCol;
  }


  int getAsicPixelNumber(int pixel, bool rotate, int maxASIC)
  {
    if(maxASIC==0) return pixel;

    int row = pixel/512;
    int col = pixel%512;


    int asicRow = row/64;
    int asicCol = col/64;

    row -= asicRow*64;
    col -= asicCol*64;


    int asicPixel = row*64+col;

    bool upperRow = (asicRow != 0);
    if(rotate && upperRow){
      asicPixel = 4095 - asicPixel;
    }

    return asicPixel;
  }


  int getAsicOfPixel(int pixel, bool invertUpperRow, int maxASIC)
  {
    if(maxASIC==0) return pixel/4096;

    int row = pixel/512;
    int col = pixel%512;

    int asicRow = row/64;
    int asicCol = col/64;

    if(invertUpperRow && asicRow==1){
      asicCol = 7 - asicCol;
    }

    row -= asicRow*64;
    col -= asicCol*64;

    int asic = asicRow * 8 + asicCol;

    return asic;
  }

  int getFirstOneInValue(uint16_t value)
  {
    for(int i=0; i<16;i++){
      if( (value & (1<<i)) != 0 ){
        return i;
      }
    }

    return -1;
  }

  // 00000001_00000110 --> 1;2;8
  std::vector<uint32_t> bitEnableValueToVector(uint16_t value)
  {
    vector<uint32_t> valueVector;
    for(int i=0; i<16; i++){
      if(value&(1<<i)){
        valueVector.push_back(i);
      }
    }
    return valueVector;
  }

  uint16_t vectorToBitEnableValue(const std::vector<uint32_t> & valuesVec)
  {
    uint16_t enable = 0;
    for(auto && value : valuesVec){
      enable |= (1<<value);
    }
    return enable;
  }


  uint16_t bitEnableStringToValue(const std::string & bitVectorStr)
  {
    if (bitVectorStr.length()==17u){

      uint16_t value = 0;
      uint16_t comp = 1<<15;
      for(unsigned i=0; i < bitVectorStr.length(); ++i){
        if(i==8) i++;

        if(bitVectorStr[i] == '1'){
          value |= comp;
        }
        comp >>=1;
      }
      return value;
    }
    cout << " Error: could not convert string " << bitVectorStr << " to bit enable value" << endl;
    return 0;
  }


  std::string vectorToBitEnableStringToValue(const std::vector<uint32_t> & valuesVec)
  {
    string asicsStr = "00000000_00000000";
    for(auto && value : valuesVec){
      int idx = 15 - value;
      if(idx >= 8){
        idx++;
      }
      asicsStr[idx] = '1';
    }
    return asicsStr;
  }


  std::string bitEnableValueToString(uint16_t value)
  {
    return vectorToBitEnableStringToValue(bitEnableValueToVector(value));
  }


  std::vector<uint32_t> bitEnableStringToVector(const std::string & bitVectorStr)
  {
    return bitEnableValueToVector(bitEnableStringToValue(bitVectorStr));
  }


  std::vector<std::string> readFilesFromDirectory(const std::string & dirName, const std::string & select)
  {
    std::vector<std::string> containedFiles;
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir (dirName.c_str())) != NULL)
    {
      /* print all the files and directories within directory */
      while ((ent = readdir (dir)) != NULL) {
        const string fileName = ent->d_name;

        if(fileName == ".") {continue;}

        if(select.empty()){
          containedFiles.push_back(fileName);
        }else{
          if(fileName.find(select) != string::npos){
            containedFiles.push_back(fileName);
          }
        }
      }
      closedir (dir);
    } else {
      /* could not open directory */
      perror ("");
    }
    return containedFiles;
  }


  bool dirExists(const std::string & dirPath)
  {
    struct stat buffer;
    if (stat (dirPath.c_str(), &buffer) == 0){
      return true;
    }else{
      fprintf(stderr, "Directory does not exist - file stats: %s\n", strerror(errno));
    }

    return false;
  }


  bool makePath(const std::string & dirPath)
  {
    if(dirExists(dirPath)){
      return true;
    }
    string execStr = "mkdir -p " + dirPath;
    int ret = system(execStr.c_str());
    return ret != -1;
  }


  std::string getMajorVersion(const std::string & versionStr){

    size_t cnt = std::count(versionStr.begin(),versionStr.end(),'.');
    if(cnt == 1){
      return versionStr;
    }

    size_t num = versionStr.rfind(".");
    return versionStr.substr(0,num);
  }

  std::string getMinorVersion(const std::string & versionStr){
    size_t cnt = std::count(versionStr.begin(),versionStr.end(),'.');
    if(cnt == 1){
      return "0";
    }

    size_t pos = versionStr.rfind(".")+1;
    return versionStr.substr(pos);
  }


  std::string getMaxVersion(std::vector<std::string> versions)
  {
    if(versions.empty()) return "v1.0.0";
    if(versions.size() == 1) return versions.front();

    sortVersionStrings(versions);

    return versions.back();
  }

  void sortVersionStrings(std::vector<std::string> &versions)
  {
    sort(versions.begin(),versions.end(),utils::compVersion);
  }


  std::vector<uint32_t> extractVersionNumbers(std::string verStr)
  {
    if(verStr[0] == 'v'){
      verStr.erase(0,1);
    }
    std::vector<uint32_t> verNumbers;
    utils::split(verStr,'.',verNumbers);
    return verNumbers;
  }

  bool compVersion (std::string left, std::string right)
  {
    if(left == right) return false;

    // count version points
    const auto leftVerNums = extractVersionNumbers(left);
    const auto rightVerNums = extractVersionNumbers(right);

    int numVer = std::min(leftVerNums.size(),rightVerNums.size());
    for(int i=0; i<numVer; i++){
      if(leftVerNums[i] < rightVerNums[i]){
        return true;
      }else if(leftVerNums[i] > rightVerNums[i]){
        return false;
      }
    }
    return false;
  }

  std::string getIOBSerialStr(uint32_t iobSerial)
  {
    std::stringstream iss;
    iss << std::hex << "0x" << std::setw(8) << std::setfill('0') << iobSerial;
    return iss.str();
  }

  int extractNumberFromString(std::string &input)
  {
    std::vector<size_t> posVec;
    for(int i=0; i<10; i++){
      auto pos = input.find_first_of('0'+i,0);
      if(pos!=std::string::npos){
        posVec.push_back(pos);
      }
    }
    size_t minPos = *std::min_element(posVec.begin(),posVec.end());
    int res = stoi(input.substr(minPos));
    input = input.substr(0,minPos);
    return res;
  }



  uint16_t convertGCC(uint16_t gray)
  {
    uint16_t result = gray & 256;
    result |= gray & 128;
    result |= (gray ^ (result >> 1)) & 64;
    result |= (gray ^ (result >> 1)) & 32;
    result |= (gray ^ (result >> 1)) & 16;
    result |= (gray ^ (result >> 1)) & 8;
    result |= (gray ^ (result >> 1)) & 4;
    result |= (gray ^ (result >> 1)) & 2;
    result |= (gray ^ (result >> 1)) & 1;
    return result;
  }

  std::string getCwd()
  {
    char dir[200];
    getcwd(dir,sizeof(dir));
    return dir;
  }

  void escapeSpaces(std::string & input)
  {
    auto pos = input.find(' ');
    while(pos != std::string::npos){
      input.insert(pos,"\\");
      pos = input.find(' ',pos+2);
    }
  }


  int getNumberOfLinesInFile(const std::string & fileName)
  {
    if(!checkFileExists(fileName)) return 0;
    ifstream aFile (fileName);
    std::size_t lines_count =0;
    std::string line;
    while (std::getline(aFile , line)){
      ++lines_count;
    }
    return lines_count;
  }

  double linearRegression(const std::vector<double> & xValues,
                          const std::vector<double> & yValues)
  {
    const uint32_t n = std::min(xValues.size(),yValues.size());

    const double s_x  = std::accumulate(xValues.begin(), xValues.begin()+n, 0.0);
    const double s_xx = std::inner_product(xValues.begin(), xValues.begin()+n, xValues.begin(), 0.0);

    return linearRegression(s_x,s_xx,xValues,yValues);
  }

  double sgn(double x)
  {
    return (x<0)? -1 : (x>0)? 1 : 0;
  }

  // inverseError Function goot between -0.0 and +0.9
  double inverseErf(double x)
  {
    static constexpr double PI = 3.141592653589793;
    static constexpr double A = 0.140;
    static constexpr double PI2A = 2.0/PI/A;

    double logX   = log(1.0-x*x);
    double logX2  = logX/2.0;
    double PI2LOG = PI2A + logX2;
    double sq1    = sqrt(pow(PI2LOG,2)-(logX/A));
    return sgn(x)*sqrt( sq1  - PI2LOG);
  }


  void exportDataFile(string filename, string header, vector<int> col0, vector<int> col1, vector<double> col2, bool append)
  {
    if (col0.empty()) {
      cout << "WARNING: col0 vector empty, nothing done." << std::endl;
    }
    if (col0.size()!=col1.size()) {
      cout << "WARNING: exportDataFile(): col vectors do not match in size ("
        << col0.size() << "/" << col1.size() << "/" << col2.size() << ") nothing done." << std::endl;
      return;
    }
    if (col0.size()==col1.size() && col1.size()==col2.size()) {
      ofstream ofs(filename, append ? ios_base::app : ios_base::out);
      if (!append && !header.empty())
        ofs << header << endl;
      for (size_t i=0; i<col0.size(); ++i) {
        ofs << col0[i] << "\t" << col1[i] << "\t" << col2[i] << std::endl;
      }
      ofs.close();
    }
  }

  void exportDataFile(string filename, string header, vector<int> col0, vector<double> col1, bool append)
  {
    if (col0.empty()) {
      cout << "WARNING: col0 vector empty, nothing done." << std::endl;
    }
    if (col0.size()!=col1.size()) {
      cout << "WARNING: exportDataFile(): col vectors do not match in size ("
        << col0.size() << "/" << col1.size() << ") nothing done." << std::endl;
      return;
    }
    if (col0.size()==col1.size()) {
      ofstream ofs(filename, append ? ios_base::app : ios_base::out);
      if (!append && !header.empty())
        ofs << header << endl;
      for (size_t i=0; i<col0.size(); ++i) {
        ofs << col0[i] << "\t" << col1[i] << std::endl;
      }
      ofs.close();
    }
  }

  void exportDataFile(string filename, string header, vector<double> col1, bool append)
  {
    if (col1.empty()) {
      cout << "WARNING: col1 vector empty, nothing done." << std::endl;
    }
    ofstream ofs(filename, append ? ios_base::app : ios_base::out);
    if (!append && !header.empty())
      ofs << header << endl;
    for (size_t i=0; i<col1.size(); ++i) {
      ofs << i << "\t" << col1[i] << std::endl;
    }
    ofs.close();
  }

  void exportDataFile(string filename, string header, vector<vector<double> > rowData, bool append)
  {
    if (rowData.empty()) {
      cout << "WARNING: rowData vector empty, nothing done." << std::endl;
    } else {
      cout << "INFO: Exporting " << rowData.size() << " rows and " << rowData[0].size() << " cols." << endl;
    }
    ofstream ofs(filename, append ? ios_base::app : ios_base::out);
    if (!append && !header.empty())
      ofs << header << endl;
    for (size_t i=0; i<rowData.size(); ++i) {
      ofs << i << "\t";
      for (auto c : rowData[i])
        ofs << c;
      ofs << std::endl;
    }
    ofs.close();
  }

  std::vector<std::string> getChipParts(const std::string &  chipPartsStr)
  {
    std::string chipParts = chipPartsStr;
    // select number of elem with #
    std::vector<uint32_t> selColParts;
    size_t hashPos = chipPartsStr.find('#');
    if(hashPos != std::string::npos){
      chipParts = chipPartsStr.substr(0,hashPos);
      std::string colParts = chipPartsStr.substr(hashPos+1);
      selColParts = utils::positionListToVector<uint32_t>(colParts);
    }

    std::vector<std::string> chipPartsVec;
    if (chipParts.find('|')!=std::string::npos) {
      utils::split(chipParts,'|',chipPartsVec);
    } else if (chipParts.find("colskipsplit")!=std::string::npos) {
      string str = chipParts.substr(12); // remove colskipsplit
      int startCol = stoi(str.substr(0,str.find('-')));
      int hypPos  = str.find('-');
      int endCol   = stoi(str.substr(hypPos+1,str.find(':')-hypPos));
      int skip     = stoi(str.substr(str.find(':')+1));
      for (auto c=startCol; c<startCol+skip; c=c+1) {
        stringstream ss;
        ss << "col";
        for (auto cc=c; cc<=endCol; cc+=skip) {
          ss << cc << ";";
        }
        chipPartsVec.push_back(ss.str() + ":32o0");
        chipPartsVec.push_back(ss.str() + ":32o32");
      }
    } else if (chipParts.find("colskip")!=std::string::npos) {
      string str = chipParts.substr(7); // remove colskip
      int startCol = stoi(str.substr(0,str.find('-')));
      int hypPos  = str.find('-');
      int endCol   = stoi(str.substr(hypPos+1,str.find(':')-hypPos));
      int skip     = stoi(str.substr(str.find(':')+1));
      for (auto c=startCol; c<startCol+skip; c=c+1) {
        stringstream ss;
        ss << "col";
        for (auto cc=c; cc<=endCol; cc+=skip) {
          ss << cc << ";";
        }
        chipPartsVec.push_back(ss.str());
      }
    } else if (chipParts.find(':')!=std::string::npos) {
      string str = chipParts.substr(3); // remove col
      int startCol = stoi(str.substr(0,str.find('-')));
      int hypPos  = str.find('-');
      int endCol   = stoi(str.substr(hypPos+1,str.find(':')-hypPos));
      int width    = stoi(str.substr(str.find(':')+1));
      for (auto c=startCol; c<=endCol; c=c+width) {
        if (width>1) {
          chipPartsVec.push_back("col"+std::to_string(c)+'-'+std::to_string(c+width-1));
        } else {
          chipPartsVec.push_back("col"+std::to_string(c));
        }
      }
    } else {
      chipPartsVec.push_back(chipParts);
    }

    if(!selColParts.empty()){
      std::vector<std::string> selChipParts;
      for(auto && elem : selColParts){
        if(elem<chipPartsVec.size()){
          selChipParts.push_back(chipPartsVec[elem]);
        }
      }
      return selChipParts;
    }

    return chipPartsVec;
  }

  void saveTrainIDBlacklistSummary(const std::string & fileName, const std::vector<uint32_t> & pixels, const std::vector<std::vector<uint32_t>> & outlierBurstIdxs, const std::vector<uint64_t> & trainIDs)
  {
    utils::makePath(utils::getFilePath(fileName));
    std::ofstream out(fileName);

    out << "#Outlier Trains per Pixel\n";
    out << "#Number of Trains:\t" << trainIDs.size() << "\t\n";
    out << "#AllTrains:\t" << utils::positionVectorToList(trainIDs) << "\t\n";
    out << "#Pixel\tOutlierTrainIdxs\n";
    int pxIdx = 0;
    for(auto && pixel : pixels){
      const auto & pxOutlierBurstIdxs = outlierBurstIdxs[pxIdx];
      out << pixel << "\t";
      for(auto && trainIdx : pxOutlierBurstIdxs){
        out << trainIdx << "\t";
      }
      out << "\n";
      pxIdx++;
    }
    out.close();
  }

  std::vector<std::vector<uint16_t>> importPxTrainIDBlacklistSummary(const std::string & fileName, std::vector<uint64_t> & trainIDs)
  {
    trainIDs.clear();
    std::vector<std::vector<uint16_t>> trainIdBlacklistSummary(utils::s_totalNumPxs);

    std::ifstream in(fileName);
    if(!in.is_open()){
      cout << "Could not open TrainIDBlacklist file: " << fileName << "\n";
      return trainIdBlacklistSummary;
    }

    bool header = true;
    std::string line;
    while (getline(in, line)) {
      if(line.empty()) continue;
      if(header){
        size_t pos = line.find("#AllTrains:");
        if(pos != string::npos){
          vector<string> fields;
          utils::split(line,'\t',fields,1);
          if(fields.empty()){
            cout << "ERROR importPxTrainIDBlacklistSummary: TrainIDs vector could not be extracted" << endl;
          }
          trainIDs = utils::positionListToVector<uint64_t>(fields.front());
          continue;
        }

        pos = line.find("#Pixel");
        if(pos != string::npos){
          header = false;
        }
      }
      else
      {
        std::vector<uint16_t> lineValues;
        utils::split(line,'\t',lineValues);
        if(lineValues.size()<=1) continue;
        uint16_t pixel = lineValues.front();
        trainIdBlacklistSummary[pixel].resize(lineValues.size()-1);
        std::copy(lineValues.begin()+1,lineValues.end(),trainIdBlacklistSummary[pixel].begin());
      }
    }
    return trainIdBlacklistSummary;
  }

  void saveTrainIDBlacklist(const std::string & fileName, const std::map<uint32_t,uint32_t> & outlierCounts, const std::vector<uint64_t> & trainIDs)
  {
    utils::makePath(utils::getFilePath(fileName));
    std::ofstream out(fileName);

    out << "#Outlier Trains per Pixel\n";
    out << "#Total Number of TrainsIDs:\t" << trainIDs.size() << "\t\n";
    out << "#NumOutlierTrains:\t" << outlierCounts.size() << "\t\n";
    out << "#TrainIdxs:\t";
    for(auto && elem : outlierCounts){
      out << elem.first << "\t";
    }
    out << "\n";
    out << "#TrainIds:\t";
    for(auto && elem : outlierCounts){
      out << trainIDs[elem.first] << "\t";
    }
    out << "\n";

    out << "#Counts  :\t";
    for(auto && elem : outlierCounts){
      out << elem.second << "\t";
    }
    out << "\n";
    out.close();

    cout << "TrainID Blacklist sucessfully saved to : " << fileName << endl;
    cout << "TrainID Blacklist contains " << outlierCounts.size() << " flagged bad TrainIDs" << endl;
  }

  std::vector<uint64_t> importTrainIDBlacklist(const std::string & fileName)
  {
    std::vector<uint64_t> trainIdBlacklist;
    ifstream in(fileName);
    if(!in.is_open()){
      cout << "Could not open TrainIDBlacklist file: " << fileName << "\n";
      return trainIdBlacklist;
    }

    cout << "Load TrainIDBlacklist from: " << fileName << endl;

    std::string line;
    while (getline(in, line)) {
      if(line.empty()) continue;
      size_t pos = line.find("#TrainIds:");
      if(pos != string::npos){
        utils::split(line,'\t',trainIdBlacklist,1);
        cout << "TrainIDBlacklist Import found " << trainIdBlacklist.size() << " trains to exclude:" << endl;
        cout << "Exclude TrainIDs: " << utils::positionVectorToList(trainIdBlacklist).substr(0,200) <<" ..." << endl;
        return trainIdBlacklist;
      }
    }
    return trainIdBlacklist;
  }


  void savePxSramBlacklist(const std::string & fileName, const std::vector<uint32_t> &pixels, const std::vector<std::vector<uint32_t> > &outlierSrams)
  {
    cout << "Save Pixel Sram Blacklist: " << fileName << endl;
    utils::makePath(utils::getFilePath(fileName));
    std::ofstream out(fileName);

    out << "#Outlier SramAddresses per Pixel\n";
    out << "#Number of Sram:\t" << utils::s_numSram << "\t\n";
    out << "#Pixel\tOutlierSramAddresses\n";
    int pxIdx = 0;
    for(auto && pixel : pixels){
      out << pixel << "\t";
      for(auto && sram : outlierSrams[pxIdx]){
        out << sram << "\t";
      }
      out << "\n";
      pxIdx++;
    }
    out.close();
  }

  std::vector<std::vector<uint32_t>> importPxSramBlacklist(const std::string & fileName)
  {
    std::vector<std::vector<uint32_t>> pxSramBlacklist(utils::s_totalNumPxs);
    ifstream in(fileName);
    if(!in.is_open()){
      cout << "Could not openPxSramBlacklist file: " << fileName << "\n";
      return pxSramBlacklist;
    }

    cout << "Load Sram Blacklist from: " << fileName << endl;

    std::string line;
    bool header = true;
    while (getline(in, line)) {
      if(line.empty()) continue;
      if(header){
        size_t pos = line.find("#Pixel");
        if(pos != string::npos){
          header = false;
        }
      }
      else
      {
        //Read SRAM Blacklist for each pixel, pixels may remain empty
        std::vector<uint32_t> nextSramList;
        utils::split(line,'\t',nextSramList);

        if(nextSramList.empty()) continue;

        uint32_t pixel = nextSramList.front();
        if(pixel>=pxSramBlacklist.size()) continue;

        auto & pxList = pxSramBlacklist[pixel];
        pxList.resize(nextSramList.size()-1);
        std::copy(nextSramList.begin()+1,nextSramList.end(),pxList.begin());
      }
    }
    return pxSramBlacklist;
  }

}
