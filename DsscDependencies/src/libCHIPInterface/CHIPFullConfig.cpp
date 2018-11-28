#include "CHIPFullConfig.h"

using namespace std;
using namespace SuS;

const std::string CHIPFullConfig::SEQTAG = "---Sequencer";
const std::string CHIPFullConfig::PXSTAG = "---Pixel Register";
const std::string CHIPFullConfig::JTGTAG = "---JTAG Register";
const std::string CHIPFullConfig::EPCTAG = "---EPC Register";
const std::string CHIPFullConfig::IOBTAG = "---IOB Register";


CHIPFullConfig::CHIPFullConfig()
 : CHIPFullConfig(1)
{
}


CHIPFullConfig::CHIPFullConfig(int numModules)
 :  sequencer(nullptr),pixelRegs(numModules,nullptr),jtagRegs(numModules,nullptr)
{
  clear();
}


CHIPFullConfig::CHIPFullConfig(const std::string & fullConfigFileName)
 : CHIPFullConfig()
{
  loadFullConfig(fullConfigFileName);
}


CHIPFullConfig::~CHIPFullConfig()
{
  if(!sequencer) return;

  delete sequencer;

  for(auto && reg : pixelRegs)
    delete reg;

  for(auto && reg : jtagRegs)
    delete reg;
}


// this function is replcaed in dervied class and not called anymore
void CHIPFullConfig::readFullConfigFile(const std::string & fileName)
{
  clear();
  configFileName = fileName;

  ifstream in(fileName);
  if(!in.is_open()){
    cout << "Error could not open full config file " << fileName << endl;
    return;
  }

  string line;
  RegType type = UNKNOWN;
  //reads the file and ignores lines with leading '#' character
  while (in.good())
  {
    getline(in,line);
    if (line.empty())
      continue;
    if (line[0]=='#')
      continue;

    if (line[0]==SEQTAG[0]){
      type = getType(line);
    }

    if(type == UNKNOWN){
      cout << "Reading in old config file." << endl;
      std::map<string,string> fileMap = readFullConfigFileOld(fileName);
      sequencerFileName = fileMap["seq"];
      pixelFileNames.push_back(fileMap["pixel"]);
      jtagFileNames.push_back(fileMap["jtag"]);
      return;
    }

    getline(in,line);
    addFileName(type,line);
  }

  sortFileNames();
}


void CHIPFullConfig::sortFileNames()
{
  std::sort(pixelFileNames.begin(),pixelFileNames.end());
  std::sort(jtagFileNames.begin(),jtagFileNames.end());
}


CHIPFullConfig::RegType CHIPFullConfig::getType(const std::string & line)
{
  if(line.find(JTGTAG) != string::npos){
    return JTG;
  }else if(line.find(PXSTAG) != string::npos){
    return PXS;
  }else if(line.find(SEQTAG) != string::npos){
    return SEQ;
  }else if(line.find(EPCTAG) != string::npos){
    return EPC;
  }else if(line.find(IOBTAG) != string::npos){
    return IOB;
  }
  return UNKNOWN;
}


void CHIPFullConfig::addFileName(CHIPFullConfig::RegType type, std::string line)
{
  // for saftey
  if (line.empty())
    return;
  if (line[0]=='#')
    return;

  //store path as absolute path
  if(line[0] != '/'){
    const string filePath = utils::getFilePath(configFileName);
    line = filePath + "/" + line;
  }

  switch(type){
    case SEQ:
      sequencerFileName = line;
      break;
    case PXS:
      pixelFileNames.push_back(line);
      break;
    case JTG:
      jtagFileNames.push_back(line);
      break;
    default:
      ;//noop;
  }
}


std::map<string,string> CHIPFullConfig::readFullConfigFileOld(const string &fileName)
{
  cout << "INFO CHIPFullConfig: Old config file found: " << fileName << endl;

  std::map<string,string> fileMap;

  ifstream stream(fileName);
  if(!stream.is_open()){
    cout << "Error could not open full config file " << fileName << endl;
    return fileMap;
  }

  const string filePath = utils::getFilePath(fileName);

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

  getline(stream,nextFileName);
  if(!nextFileName.empty()){
    if (nextFileName[0] != '/') { // check to be compatible to absolute paths also
      nextFileName = filePath + "/" + nextFileName;
    }
    fileMap["epc"] = nextFileName;
  }

  getline(stream,nextFileName);

  if(!nextFileName.empty()){
    if (nextFileName[0] != '/') { // check to be compatible to absolute paths also
      nextFileName = filePath + "/" + nextFileName;
    }
    fileMap["iob"] = nextFileName;
  }

  return fileMap;
}


//no extension required in PPT
void CHIPFullConfig::loadFullConfig(const string & fileName)
{
  {
    utils::CoutColorKeeper keeper(utils::STDGREEN);
    cout << "++++Init system from full config file: " << fileName << endl;
  }
  readFullConfigFile(fileName);

  bool ok = checkConfig();
  if(!ok) return;

  loadConfig();

  lastFileName = configFileName;
}


void CHIPFullConfig::loadConfig()
{
  if(!sequencer){
    newSequencer();
  }else{
    sequencer->loadFile(sequencerFileName);
  }
  good &= sequencer->isGood();

  for(size_t i=0; i<jtagRegs.size(); i++){
    int fileIdx = std::min(i,jtagFileNames.size()-1);
    if(!jtagRegs[i]){
      newJtagReg(i);
    }else{
      jtagRegs[i]->initFromFile(jtagFileNames[fileIdx]);
    }
    good &= jtagRegs[i]->isLoaded();
  }

  for(size_t i=0; i<pixelRegs.size(); i++){
    if(!pixelRegs[i]){
      newPixelReg(i);
    }else{
      int fileIdx = std::min(i,pixelFileNames.size()-1);
      pixelRegs[i]->initFromFile(pixelFileNames[fileIdx]);
    }
    good &= pixelRegs[i]->isLoaded();
  }
}


bool CHIPFullConfig::checkConfig()
{
  good = true;

  if(sequencerFileName.empty()){
    std::cout << "Sequencer file not set, full config not usable" << std::endl;
//    throw "Sequencer file not set";
    good = false;
  }

  if(!utils::checkFileExists(sequencerFileName)){
    std::cout << "Sequencer file does not exist " << sequencerFileName <<" full config not usable" << std::endl;
//    throw "Sequencer file does not exist";
    good = false;
  }

  for(const auto name : jtagFileNames){
    if(name.empty()){
      std::cout << "JTAG Register file empty, full config not usable" << std::endl;
//      throw "JTAG Register file empty";
      good = false;
    }

    if(!utils::checkFileExists(name)){
      std::cout << "JTAG Register file empty, full config not usable" << std::endl;
//      throw "JTAG Register file does not exist";
      good = false;
    }
  }

  for(const auto name : pixelFileNames){
    if(name.empty()){
      std::cout << "Pixel Register file does not exist " << name << " . Full config not usable" << std::endl;
//      throw "Pixel Register file empty";
      good = false;
    }

    if(!utils::checkFileExists(name)){
      std::cout << "JTAG Register file does not exist " << name << " . Full config not usable" << std::endl;
//      throw "Pixel Register file does not exist";
      good = false;
    }
  }

  return good;
}


void CHIPFullConfig::storeFullConfigFile(string fileName, bool keepName)
{
  if(fileName.rfind(".conf") == std::string::npos){
    fileName += ".conf";
  }

  configFileName = fileName;
  const string filePath = utils::getFilePath(configFileName);
  const string baseName = utils::getBaseFileName(configFileName);

  sequencerFileName = filePath + "/" + baseName +"_seq.xml";

  jtagFileNames.resize(jtagRegs.size());
  for(size_t mod = 0; mod < jtagRegs.size(); mod++){
    jtagFileNames[mod] = filePath + "/" + baseName + "_Module_" + to_string(mod+1) + "_jtagRegs.xml";
  }

  pixelFileNames.resize(pixelRegs.size());
  for(size_t mod = 0; mod < pixelRegs.size(); mod++){
    pixelFileNames[mod] = filePath + "/" + baseName + "_Module_" + to_string(mod+1) + "_pxRegs.xml";
  }

  storeFullConfigFile();

  if(keepName){
    configFileName = lastFileName;
  }
}


void CHIPFullConfig::storeFullConfigFile()
{
  sequencer->saveToFile(sequencerFileName);

  int mod = 0;
  for(auto && reg : jtagRegs){
    reg->saveToFile(jtagFileNames[mod]);
    mod++;
  }

  mod = 0;
  for(auto && reg : pixelRegs){
    reg->saveToFile(pixelFileNames[mod]);
    mod++;
  }

  generateFullConfigFile();
}


void CHIPFullConfig::generateFullConfigFile()
{
  ofstream stream(configFileName);
  if (!stream.is_open()){
    cout << "Error could not open full config file to save " << configFileName << endl;
    return;
  }

  stream << SEQTAG << ":\n";
  stream << utils::getFileName(sequencerFileName) <<"\n";

  int mod = 1;
  for(auto && name : jtagFileNames){
    stream << JTGTAG << " Module " << mod++ << ":\n";
    stream << utils::getFileName(name)   << "\n";
  }

  mod = 1;
  for(auto && name : pixelFileNames){
    stream << PXSTAG << " Module " << mod++ << ":\n";
    stream << utils::getFileName(name)   << "\n";
  }
}


void CHIPFullConfig::clear()
{
  good = false;
  sequencerFileName = configFileName = "";
  jtagFileNames.clear();
  pixelFileNames.clear();
}


void CHIPFullConfig::newSequencer()
{
  sequencer = new Sequencer(sequencerFileName);
}


void CHIPFullConfig::newPixelReg(size_t idx)
{
  int fileIdx = std::min(idx,pixelFileNames.size()-1);
  pixelRegs[idx] = new ConfigReg(pixelFileNames[fileIdx]);
}


void CHIPFullConfig::newJtagReg(size_t idx)
{
  int fileIdx = std::min(idx,jtagFileNames.size()-1);
  jtagRegs[idx] = new ConfigReg(jtagFileNames[fileIdx]);
}

