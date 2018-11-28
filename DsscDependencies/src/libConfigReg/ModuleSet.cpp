

#include <bitset>
#include <cassert>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "ModuleSet.h"
#include <string>

#include "utils.h"
//#define DEBUG

using namespace std;

namespace SuS {



ModuleSet::ModuleSet()
  : name("Unknown"),
    modulesString(""),
    numBitsPerModule(0),
    numBitReminder(0),
    numSignals(0),
    numModules(0),
    setIsReverse(false),
    address(0),
    changed(false)
{}

ModuleSet::ModuleSet(const string & setName)
 : name(setName),
   modulesString(""),
   numBitsPerModule(0),
   numBitReminder(0),
   numSignals(0),
   numModules(0),
   setIsReverse(false),
   address(0),
   changed(false)
{
}

ModuleSet::ModuleSet(ifstream & in)
:  name("newSet"),
   modulesString(""),
   numBitsPerModule(0),
   numBitReminder(0),
   numSignals(0),
   numModules(0),
   setIsReverse(false),
   address(0),
   changed(false)
{
  utils::doubleRead(in,name,':');
  utils::doubleRead(in,modulesString,':');
  utils::doubleRead(in,numBitsPerModule,':');
  utils::doubleRead(in,numSignals,':');
  utils::doubleRead(in,setIsReverse,':');
  utils::doubleRead(in,address,':');

  vector<uint32_t> modVec  = utils::positionListToVector<>(modulesString);
  numModules = modVec.size();

  numBitReminder = numBitsPerModule;
  cSignals.reserve(numSignals,numModules);
  cSignals.addModules(modulesString);

  readSignalsAndOutputs(in);
  readModuleValues(in);

  if(!cSignals.checkSignals()){
    cout << "Initialization of "<< name << " went wrong" << endl;
  }
}


void ModuleSet::readSignalsAndOutputs(ifstream & in)
{
  vector<string> posVec;

  {
    string line;
    getline(in,line); utils::split(line,':',cSignals.signalNames,1);
    getline(in,line);
    if (line.compare(0,15,"#SignalsAliases")==0) {
      vector<string> aliases;
      utils::split(line,':',aliases,1);
      cSignals.aliases.resize(numSignals);
      int sig = 0;
      for (const auto & a : aliases) {
        utils::split(a,';',cSignals.aliases[sig++],1);
      }
      getline(in,line);
    } else {
      cSignals.aliases.clear();
      cSignals.aliases.resize(numSignals);
    }
    utils::split(line,':',cSignals.readOnly,1);
    getline(in,line);
    if (line.compare(0,10,"#ActiveLow")==0) {
      utils::split(line,':',cSignals.activeLow,1);
      getline(in,line);
      //cout << "Found #ActiveLow signals line." << std::endl;
    } else {
      cSignals.activeLow.clear();
      cSignals.activeLow.resize(numSignals,false);
    }
                      utils::split(line,':',posVec,1);
    getline(in,line); utils::split(line,':',cSignals.accessLevels,1);
    getline(in,line); utils::split(line,':',cSignals.outputs,1);
  }

  if(cSignals.signalNames.size() != (unsigned)numSignals){
    cout << utils::STDRED << "ERROR: ModuleSet " << name << " signal names not correct found " << cSignals.signalNames.size() << "/" << numSignals << utils::STDNORM << endl;
    exit(-1);
  }
  bool various = cSignals.modules.size() != 1;
  cSignals.isVarious.resize(numSignals,various);

  //cout << "Found #SignalsAliases line, aliases.size() = " << cSignals.aliases.size() << std::endl;
  if (cSignals.aliases.size() != cSignals.signalNames.size()) {
    cout << utils::STDRED << "Warning: ModuleSet " << name <<": #SignalsAliases must have same size as #Signals, discarding all aliases." << cSignals.aliases.size() << utils::STDNORM << std::endl;
    cSignals.aliases.clear();
    cSignals.aliases.assign(cSignals.signalNames.size(),std::vector<std::string>(1,""));
  }

  int sig = 0;
  for(const auto& signal : cSignals.signalNames)
  {
    cSignals.bitPositions.emplace_back(utils::positionListToVector<uint32_t>(posVec[sig]));
    cSignals.signalsMap[signal] = sig;
    for (auto a: cSignals.aliases[sig]){
      if (!a.empty()) cSignals.signalsMap[a] = sig;
    }
    sig++;
  }

  cSignals.initBitNames(numBitsPerModule);

  if(!cSignals.checkBitPositions()){
    std::cout << "Error in bit positions: ModuleSet = " << name << std::endl;
  }
}


void ModuleSet::readModuleValues(ifstream & in)
{
  string line;
  int modulesRead = 0;
  while(!in.eof()){
    string info = utils::getSection(in,0,':');
    info += ":"; //add ':' to be able to distinguish #ModuleSet
    if(info.compare(0,8,"#Module:")==0){
      string module = utils::getSection(in,0,':');
      int mod = cSignals.modToIntS(module);
      getline(in,line);

      if(line=="") break;
      utils::split(line,':',cSignals.modSignalValues[mod],0);
      modulesRead++;
    }else{
      for(unsigned i=0;i<info.size();i++) in.unget();
      break;
    }
  }

  if(modulesRead!=numModules){
    cSignals.initSignalValuesDefault();
  }else{
    cSignals.updateSignalIsVarious();
  }
}


/**
        \brief Saves the memory structure and values to given file
*/
void ModuleSet::saveToStream(ofstream & out)
{
    out << "#ModuleSet:" << name             << "\n";
    out << "modules   :" << modulesString    << "\n";
    out << "numBits   :" << numBitsPerModule << "\n";
    out << "numSignals:" << numSignals       << "\n";
    out << "reverse   :" << setIsReverse     << "\n";
    out << "address   :" << address          << "\n";

    cSignals.saveToStream(out);
}


void ModuleSet::setSignalValue(string modulesStr, const string & signalName, uint32_t value)
{
  if(cSignals.sigToIntS(signalName)<0) return;

  if(modulesStr.compare(0,3,"all") == 0){
    modulesStr = modulesString;
  }

  if(value > getMaxSignalValue(signalName)){
     cout << "ERROR ConfigSignals: " << signalName << " Value too big, no value set!" << endl;
     return;
  }

  const vector<string> modulesToChange = utils::positionListToStringVector(modulesStr);

  cSignals.setSignalValue(modulesToChange,signalName,value);
}


uint32_t ModuleSet::getSignalValue(string modulesStr, const string & signalName)
{
  if(modulesStr.compare(0,3,"all") == 0){
    modulesStr = cSignals.modules.front();
  }else if(modulesStr.find("-",1) != string::npos){
    modulesStr = utils::positionListToStringVector(modulesStr).front();
  }else if(modulesStr.find(";",1) != string::npos){
    modulesStr = utils::positionListToStringVector(modulesStr).front();
  }

  return cSignals.getSignalValue(modulesStr,signalName);
}


vector<uint32_t> ModuleSet::getSignalValues(const string & moduleStr, const string & signalName)
{
  vector<std::string> modulesVec;
  if(moduleStr.compare(0,1,"a") == 0){
    modulesVec = cSignals.modules;
  }else{
    modulesVec = utils::positionListToStringVector(moduleStr);
  }

  vector<uint32_t> signalValues(modulesVec.size());

  int idx = 0;
  for(const auto & mod : modulesVec){
    signalValues[idx++] = getSignalValue(mod,signalName);
  }
  return signalValues;
}


uint32_t ModuleSet::getIntegerValue(const std::string & module)
{
  const auto modContent = cSignals.printModuleContent(module,false);
  if(modContent.size() != 32){
    cout << "Error: moduleSet with wrong bitnumber found: " + name  << ":" << modContent.size() << endl;
    return 0;
  }

  uint32_t value = 0;
  for(int i=0; i<32; i++){
    if(modContent[i]){
      value |= (uint32_t)1<<i;
    }
  }

  return value;
}

bool ModuleSet::isBitReadOnly(int bit)
{
  int pos = (setIsReverse)? numBitsPerModule - (bit%numBitsPerModule) - 1 : bit%numBitsPerModule;
  int sig = cSignals.getSignalNumberFromPosition(pos);
  return cSignals.isSignalReadOnly(sig);
}


bool ModuleSet::isModuleSetReadOnly()
{
  for(const auto & readOnly: cSignals.readOnly){
    if(!readOnly) return false;
  }
  return true;
}

bool ModuleSet::compareContent(const vector<bool> &data_vec, bool overwrite)
{
  vector<bool> bits = printContent();
  if(data_vec.size() < bits.size()) {
    compareErrors.push_back("ModuleSet: Data Vector too short: " + to_string(data_vec.size()) + "/" +
                                                                    to_string(bits.size()));
    cout << compareErrors.back() << endl;
    return false;
  }

  bool ok = true;
  for(uint i=0; i< bits.size(); i++){
    int bitNr = i%numBitsPerModule;
    if(!isBitReadOnly(bitNr)){
      if(data_vec[i] != bits[i]){
        ok = false;
        if(compareErrors.size() < 2000u){
          ostringstream oss;
          oss << ";: \"" << name
              << "\" Module " << setw(4) << i/numBitsPerModule
              << "  Bit "     << setw(2) << getBitNumber(i)
              << " ( " << getBitName(i) << " ) "
              << "was: "  << data_vec[i];
          compareErrors.push_back(oss.str());
        }
      }
    }else if(overwrite){
      if(!setIsReverse){
        int module = i/numBitsPerModule;
        //TODO: check how this works at reversed register
        setBitValue(module,bitNr,data_vec[i]);
      }
    }
  }

  return ok;
}


bool ModuleSet::compareContent(const vector<bool> &data_vec, int asic, bool overwrite)
{
  int asicIdx     = (asic>7)? asic : 7-asic;

  int startModule =   asicIdx    * 4096;
  int endModule   =  (asicIdx+1) * 4096;

  int startBit = startModule*numBitsPerModule;
  int endBit   = endModule*numBitsPerModule;

  vector<bool> bits = printContent();
  vector<bool> subModuleBits;
  subModuleBits.assign(bits.begin() + startBit, bits.begin() + endBit);
  //for each asic, only above 8, one bit has to be removed
  if(asic>7){
    for(int i=0; i<asic; i++){
      subModuleBits.pop_back();
    }
  }

  if(data_vec.size() < subModuleBits.size()) {
    compareErrors.push_back("Single ASIC ModuleSet: Data Vector too short: " + to_string(data_vec.size()) + "/" +
                                                                    to_string(subModuleBits.size()));
    cout << compareErrors.back() << endl;
    return false;
  }

  bool ok = true;
  for(uint i=0; i< subModuleBits.size(); i++){
    int bitNr = i%numBitsPerModule;
    if(!isBitReadOnly(bitNr)){
      if(data_vec[i] != subModuleBits[i]){
        ok = false;
        if(compareErrors.size() < 2000u){
          int asicPixel = i/numBitsPerModule;
          int outputNr = startModule+asicPixel;
          if(setIsReverse){
            outputNr = cSignals.outputs.size()-outputNr-1;
          }
          ostringstream oss;
          oss << "RBError: \"" << name
              << "\" ModuleIdx " << setw(4) << (double)i/numBitsPerModule
              << "  Output "  << setw(4) << cSignals.outputs[outputNr]
              << "  Bit "     << setw(2) << getBitNumber(i)
              << " ( " << getBitName(i) << " ) "
              << "was: "  << data_vec[i];
          compareErrors.push_back(oss.str());
        }
      }
    }else if(overwrite){
      if(!setIsReverse){
        int module = startModule+i/numBitsPerModule;
        //TODO: check how this works at reversed register
        setBitValue(module,bitNr,data_vec[i]);
      }
    }
  }

  return ok;
}

bool ModuleSet::compareContent(uint32_t data, bool overwrite)
{
  if(numBitsPerModule>32){
    cout << "This function can only be used for module sets that store 32 bits or less. Use the bool vector function" << endl;
    return false;
  }

  vector<bool>data_vec(32);
  for(uint i=0; i<32; i++){
    bool value = (data & (1<<i)) != 0;
    data_vec[i] = value;
  }

  return compareContent(data_vec,overwrite);
}


bool ModuleSet::compareContent(uint32_t data, const string & module, bool overwrite)
{
  if(numBitsPerModule>32){
    cout << "This function can only be used for module sets that store 32 bits or less. Use the bool vector function" << endl;
    return false;
  }

  vector<bool>data_vec(32);
  for(uint i=0; i<32; i++){
    bool value = (data & (1<<i)) != 0;
    data_vec[i] = value;
  }

  return compareContent(data_vec,module,overwrite);
}


bool ModuleSet::compareContent(const vector<bool> & data_vec, const string & module, bool overwrite)
{
  const vector<bool> bits = cSignals.printModuleContent(module,setIsReverse);

  if(data_vec.size() < bits.size()) {
    compareErrors.push_back("Module Data Vector too short: " + to_string(data_vec.size()) + "/" + to_string(bits.size()));
    cout <<  "Module Data Vector too short: " << data_vec.size() << "/" << bits.size() << endl;
    return false;
  }

  bool ok = true;
  for(uint32_t i=0; i< bits.size(); i++){
    if(!isBitReadOnly(i)){
      if(data_vec[i] != bits[i]){
        ok = false;
        ostringstream oss;
        oss << "RBError: \"" << name
            << "\" Module " << setw(4) << module
            << "  Bit "     << setw(2) << getBitNumber(i)
            << " ( " << getBitName(i) << " ) "
            << "was: " << data_vec[i];
        compareErrors.push_back(oss.str());
      }
    }else if(overwrite){ // in JTAG XORS the overwire generates bad behaviour must be possible to
      if(!setIsReverse){
        const uint32_t bitNr = i%numBitsPerModule;
        //TODO: check how this works at reversed register
        setBitValue(module,bitNr,data_vec[i]);
      }
    }
  }

  return ok;
}



vector<bool> ModuleSet::printContent()
{
  vector<bool> content;
  unsigned totalBits  = numModules * numBitsPerModule;
  content.reserve(totalBits);

  if(setIsReverse){
    for(auto mod_it = cSignals.outputs.rbegin(); mod_it != cSignals.outputs.rend(); ++mod_it){
      const auto modContent = cSignals.printModuleContent(*mod_it,true);
      content.insert(content.end(),modContent.begin(),modContent.end());
    }
  }else{
    for(const auto & module : cSignals.outputs){
      const auto modContent = cSignals.printModuleContent(module,false);
      content.insert(content.end(),modContent.begin(),modContent.end());
    }
  }

  if(content.size() != totalBits){
    cout << "ERROR ConfigSignals: " << "Print Content: something goes wrong!" << endl;
  }

  return content;
}


string ModuleSet::printModuleContentToString(const string & moduleList, bool verbose)
{
  const vector<string> modulesToPrint = utils::positionListToStringVector(moduleList);

  ostringstream out;

  for(const auto & module : modulesToPrint){
    out << "Module " <<  module << " Bits 0 - " << numBitsPerModule << " : ";
    for (const auto& bit : cSignals.printModuleContent(module,setIsReverse)){
      out << bit;
    }
    out << endl;
  }

  if(verbose)
    cout << out.str() << endl;

  return out.str();
}

string ModuleSet::printContentToString(bool verbose)
{
  ostringstream out;
  out << "All Bits: ";
  for (const auto& bit : printContent()){
    out << bit;
  }
  out << endl;

  if(verbose)
    cout << out.str() << endl;

  return out.str();
}

const string & ModuleSet::getBitName(int pos) const
{
  uint32_t bitNr = (setIsReverse)? numBitsPerModule - (pos%numBitsPerModule) - 1 : pos%numBitsPerModule;
  return cSignals.bitNames[bitNr];
}

int ModuleSet::getBitNumber(int pos) const
{
  uint32_t bitNr = (setIsReverse)? numBitsPerModule - (pos%numBitsPerModule) - 1 : pos%numBitsPerModule;
  return bitNr;
}


bool ModuleSet::moduleSetIsVarious(std::string modulesStr)
{
  if(modulesStr.compare(0,3,"all") == 0){
    modulesStr = modulesString;
  }

  const vector<string> modulesVec = utils::positionListToStringVector(modulesStr);

  for(auto && signalName : cSignals.signalNames)
  {
    if(cSignals.signalIsVarious(signalName,modulesVec)){
      return true;
    }
  }
  return false;
}


bool ModuleSet::signalIsVarious(const string & signalName, string modulesStr)
{
  if(modulesStr.compare(0,3,"all") == 0){
    modulesStr = modulesString;
  }

  const vector<string> modulesVec = utils::positionListToStringVector(modulesStr);

  return cSignals.signalIsVarious(signalName,modulesVec);
}

std::string ModuleSet::getModuleAtOutputNumber(uint32_t number)
{
  if(number >= cSignals.outputs.size())
  {
    cout << "ERROR ModuleSet: output number "<< number << " out of range" << endl;
    return "0";
  }

  return cSignals.outputs[number];
}


void ModuleSet::setOutputs(const string & outputList)
{
  auto newOutputs = utils::positionListToStringVector(outputList);

  if(newOutputs.size() != (unsigned)numModules){
    cout << "WARNING ModuleSet: newOutputs have wrong size: modules/outputs " << numModules << " / " << newOutputs.size() << endl;
    return;
  }

  cSignals.outputs = move(newOutputs);
}

void ModuleSet::setOutputs(const vector<string> & outputs)
{
  if(outputs.size() != (unsigned)numModules){
    cout << "WARNING ModuleSet: newOutputs have wrong size: modules/outputs " << numModules << " / " << outputs.size() << endl;
    return;
  }

  cSignals.outputs = move(outputs);
}


string ModuleSet::addModules(const string & modulesList)
{
  cSignals.addModules(modulesList);
  numModules = cSignals.numModules;
  modulesString = utils::positionStringVectorToList(cSignals.modules);

  return modulesString;
}


string ModuleSet::removeModules(const string & modulesList)
{
  cSignals.removeModules(modulesList);
  numModules = cSignals.numModules;
  modulesString = utils::positionStringVectorToList(cSignals.modules);

  return modulesString;
}


void ModuleSet::removeNCSignal()
{
  int signalIndex = 0;
  int i=0;
  bool found = false;
  for(const auto & sigName : cSignals.signalNames)
  {
    if(sigName.substr(sigName.length()-2,2).compare("nc") == 0){
      signalIndex = i;
      found = true;
      break;
    }
    i++;
  }
  if(found){
    numBitReminder = cSignals.numBitsPerModule;
    cSignals.removeSignal(signalIndex,false);
    numBitsPerModule = cSignals.numBitsPerModule;
    numSignals = cSignals.numSignals;
  }
}

}

