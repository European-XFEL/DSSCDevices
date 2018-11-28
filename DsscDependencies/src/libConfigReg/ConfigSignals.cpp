#include <algorithm>
#include "utils.h"

#include "ConfigSignals.h"


using namespace SuS;
using namespace std;

inline bool sortModules(const std::string & i, const std::string & j) { return ( stoi(i)<stoi(j) ); }

ConfigSignals::ConfigSignals()
: numModules(0),
  numSignals(0)
{
  reserve(0,0);
}

void ConfigSignals::reserve(const int _numSignals, const int _numModules)
{
    numModules = _numModules;
    numSignals = _numSignals;

    isVarious.reserve(numSignals);
    readOnly.reserve(numSignals);
    accessLevels.reserve(numSignals);
    activeLow.reserve(numSignals);

    outputs.reserve(numSignals);
    modSignalValues.reserve(numModules);

    bitPositions.reserve(numSignals);
    // bit names are initializes in initBitNames
}

void ConfigSignals::addOutputs(const string & newOutputsList)
{
  auto newOutputs = utils::positionListToStringVector(newOutputsList);
  outputs.insert(outputs.end(),newOutputs.begin(),newOutputs.end());

  if(outputs.size() != (unsigned)numModules){
    utils::CoutColorKeeper keeper(utils::STDBROWN);
    cout << "WARNING ConfigSignals: newOutputs have wrong size: modules/outputs " << numModules << " / " << outputs.size() << endl;
  }
}


void ConfigSignals::saveToStream(ofstream & out) const
{
  out << "#Signals:";
  for(const auto & sig : signalNames){
    out << sig <<":";
  }
  out << "\n";

  out << "#SignalsAliases:";
  for(const auto & sig : aliases){
    for (const auto & a : sig) {
      out << a << ";";
    }
    out << ":";
  }
  out << "\n";

  out << "#ReadOnly:";
  for(const auto & sig : readOnly){
    out << sig <<":";
  }
  out << "\n";

  out << "#ActiveLow:";
  for(const auto & sig : activeLow){
    out << sig <<":";
  }
  out << "\n";

  out << "#Positions:";
  for(const auto & sig : bitPositions){
    out << utils::positionVectorToList(sig)<<":";
  }
  out << "\n";

  out << "#AccessLevels:";
  for(const auto & sig : accessLevels){
    out << sig <<":";
  }
  out << "\n";

  out << "#Outputs:";
  for(const auto & output :outputs){
    out << output <<":";
  }
  out << "\n";

  int numExpModules = numModules;
  if(allModulesSame()){
    numExpModules = 1;
  }

  for(int mod=0; mod<numExpModules; mod++){
    out << "#Module:" << modules[mod] << ":";
    for(const auto & sigValue : modSignalValues[mod]){
      out << sigValue <<":";
    }
    out << endl;
  }
}

void ConfigSignals::initBitNames(int _numBitsPerModule)
{
  numBitsPerModule = _numBitsPerModule;
  bitNames.resize(numBitsPerModule,"nc");

  for(int sig=0; sig<numSignals; sig++)
  {
    const string & signalName = signalNames[sig];

    if (bitPositions[sig].size()>1)
    {
      int bit = 0;
      for(const auto & pos : bitPositions[sig]) {
        bitNames[pos] = signalName + "[" + to_string(bit) + "]";
        bit++;
      }
    }
    else
    {
      bitNames[bitPositions[sig][0]] = signalName;
    }
  }
}


bool ConfigSignals::signalIsVarious(const string & signalName, const vector<string> & modulesVec)
{
  const int sig  = sigToInt(signalName);

  if(!isVarious[sig]){
    return false;
  }

  const int mod  = modToInt(modulesVec.back());
  uint32_t value = getSignalValue(mod,sig);

  const vector<int> modV = toIdxVector(modulesVec);
  for(const auto & mod : modV){
    if(value != modSignalValues[mod][sig]){
      return true;
    }
  }

  if(modulesVec.size() == modules.size()){
    isVarious[sig] = false;
  }

  return false;
}

void ConfigSignals::updateSignalIsVarious()
{
  checkModSignalValue();
  
  const vector<uint32_t> & defaultValues = modSignalValues[0];

  for(int sig=0; sig<numSignals; sig++){
    isVarious[sig] = false;
    for(int mod=1; mod<numModules; mod++){
      const auto & modValues = modSignalValues[mod];
      if(modValues[sig] != defaultValues[sig]){
        isVarious[sig] = true;
        continue;
      }
    }
  }
}


void ConfigSignals::initSignalValuesDefault()
{
  const vector<uint32_t> & defaultValues = modSignalValues[0];

  for(int mod=1; mod<numModules; mod++){
    modSignalValues[mod] = defaultValues;
  }
}


string ConfigSignals::getSignalPositionsList(const std::string & signalName)
{
  return utils::positionVectorToList<uint32_t>(bitPositions[sigToInt(signalName)]);
}

string ConfigSignals::getSignalAliases(const std::string & signalName)
{
  //return aliases[sigToInt(signalName)];
  std::stringstream strstrm;
  for (auto const & a: aliases[sigToInt(signalName)]) {
    strstrm << a << ";";
  }
  return strstrm.str();
  //return aliases[sigToInt(signalName)];
}


void ConfigSignals::checkModSignalValue()
{
  int mod=0;
  for(auto && moduleValues : modSignalValues){
    if(moduleValues.size() != (unsigned)numSignals){
      utils::CoutColorKeeper keeper(utils::STDBROWN);
      cout << "WARNING ConfigSignals: Signal count for module " << modules[mod] << " wrong " << moduleValues.size() << "/" << numSignals << " mod=" << mod << endl;
      cout << "Signal 0 " << signalNames[0] << endl;

      if(mod == 0){
        moduleValues.resize(numSignals); // remove doubles
      }else{
        moduleValues=modSignalValues[0]; // set to default values
      }
    }
    mod++;
  }
}

bool ConfigSignals::checkBitPositions()
{
  bool retVal = true;
  uint32_t numBitsacc = 0;
  uint32_t maxPos     = 0;
  for(const auto & pos:bitPositions){
    numBitsacc += pos.size();
    for (const auto & bit: pos){
      if(bit>maxPos) maxPos = bit;
    }
  }

  if(maxPos>=numBitsacc){
    utils::CoutColorKeeper keeper(utils::STDBROWN);
    cout << "WARNING: defined " << numBitsacc << " positions, but maxPos = " << maxPos << " maybe some bits are not defined yet" << endl;
  }

  numBitsacc = maxPos+1;

  vector<bool> checks(maxPos+1,false);
  //for(uint sig=0; sig<numSignals; sig++)
  for (const auto & sigVec: bitPositions)
  {
    for(const auto & bit: sigVec){
      if( !checks[bit]){
        checks[bit] = true;
      }else{
        utils::CoutColorKeeper keeper(utils::STDRED);
        cout << "ERROR: found double entry for bit " << bit << " (" << bitNames[bit] << ")" << endl;
        retVal = false;
      }
    }
  }

  if(retVal) numBitsPerModule = numBitsacc;

  int i=0;
  vector<uint32_t> missingBits;
  for (const auto & ck : checks) {
    if (!ck) {
      utils::CoutColorKeeper keeper(utils::STDBROWN);
      cout << "WARNING: Bit position " << i << " is missing. Will add it to nc" << endl;
      retVal = false;
      missingBits.push_back(i);
    }
    i++;
  }

  if(missingBits.size()>0)
    addToNc(missingBits);

  return retVal;
}


void ConfigSignals::renameSignal(unsigned signalIndex, const std::string & signalName){
    if(signalIndex>= signalNames.size()) throw "Row out of signal range";

    const string oldName = signalNames[signalIndex];
    const auto sigIt = signalsMap.find(oldName);
    signalsMap.erase(sigIt);

    signalNames[signalIndex] = signalName;

    signalsMap[signalName] = signalIndex;
}


void ConfigSignals::addToNc(const vector<uint32_t> & missingBits)
{
  if(signalsMap.find("nc") == signalsMap.end()){
    addSignal("nc",missingBits,true);
  }else{
    int sig = sigToInt("nc");
    vector<uint32_t> & ncPos = bitPositions[sig];
    ncPos.insert(ncPos.end(),missingBits.begin(),missingBits.end());
    checkBitPositions();
    initBitNames(numBitsPerModule);
  }
}


vector<bool> ConfigSignals::printModuleContent(const string & module, bool reverse)
{
  int mod = modToInt(module);
  vector<bool> content(numBitsPerModule,false);

  int sig =0;
  for(const auto& val: modSignalValues[mod])
  {
    if(val != 0 || activeLow[sig]){
      int bit=0;
      for(const auto &bitPos : bitPositions[sig])
      {
        int comp = (1<<bit);
        if(activeLow[sig] ^ ((val & comp) != 0)){
          int bitNr  = (reverse)? numBitsPerModule - bitPos - 1 : bitPos;
          content[bitNr] = true;
        }
        bit++;
      }
    }
    sig++;
  }

  return content;
}

void ConfigSignals::removeModules(const string & modulesList)
{
    auto exModules = utils::positionListToStringVector(modulesList);
    sort(exModules.begin(),exModules.end(),sortModules);

    for(int i=exModules.size()-1; i>=0; i--){
      int mod = modulesMap[exModules[i]];
      modules.erase(modules.begin()+mod);
      modSignalValues.erase(modSignalValues.begin()+mod);
    }

    sort(modules.begin(),modules.end(),sortModules);

    numModules = modules.size();

    modulesMap.clear();

    int idx = 0;
    for(const auto & m : modules){
      modulesMap[m] = idx++;
    }
}


const vector<string> & ConfigSignals::getOutputs()
{
  if(outputs.size()==0){
    if(modules.size()==0){
      utils::CoutColorKeeper keeper(utils::STDRED);
      cout << "ConfigSignals ERROR: can not return outputs" << endl;
      throw "ConfigSuignals ERROR: can not return outputs";
    }
    return modules;
  }
  return outputs;
}


void ConfigSignals::addModules(const string & modulesList)
{
  vector<uint32_t> defaultValues;
  if(modSignalValues.size() != 0){
    defaultValues = modSignalValues[0];
  }

  auto newModules = utils::positionListToStringVector(modulesList);
  for(const auto & newM : newModules)
  {
    if(modulesMap.find(newM) == modulesMap.end())
    {
      modules.push_back(newM);
      modSignalValues.push_back(defaultValues);
    }
  }

  sort(modules.begin(),modules.end(),sortModules);

  modulesMap.clear();

  int idx = 0;
  for(const auto & m : modules){
    modulesMap[m] = idx++;
  }

  numModules = modules.size();
  if(modulesMap.size() != (unsigned)numModules){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "ConfigSignals modulesMap wrong size" << endl;
  }

  //cout << "INFO: ConfigSignals : " << newModules.size() << " new modules added! "<< numModules << " Modules have been resorted!" << endl;
}

void ConfigSignals::addSignal(const std::string & signalName, bool ro, bool actLow)
{
  int nextIdx = signalsMap.size();
  cout << "nextIdx " << nextIdx << std::endl;
  int max = -1;

  for(const auto & positions : bitPositions){
    for(const auto & bit : positions){
      if(max<(int)bit) max = bit;
    }
  }
  vector<uint32_t> newPos(1,max+1);
  addSignal(signalName,newPos,ro,actLow);
}


void ConfigSignals::addSignal(const string & signalName, const vector<uint32_t> & positions, bool ro, bool actLow)
{
  int nextIdx = numSignals++;
  // cannot use signalsMap anymore because it also holds the aliases
  signalsMap[signalName] = nextIdx;

  signalNames.push_back(signalName);

  bitPositions.push_back(positions);

  readOnly.push_back(ro);

  activeLow.push_back(actLow);

  aliases.push_back(vector<string>());
  //setSignalAliases(nextIdx,_aliases);

  accessLevels.push_back(0);

  isVarious.push_back(0);


  for(auto && moduleValues : modSignalValues){
     moduleValues.push_back(0);
  }

  checkBitPositions();

  numSignals = signalNames.size();

  bitNames.clear();
  initBitNames(numBitsPerModule);
}


void ConfigSignals::removeSignal(int signalIndex, bool check)
{
    isVarious.erase(isVarious.begin()+signalIndex);
    readOnly.erase(readOnly.begin()+signalIndex);
    activeLow.erase(activeLow.begin()+signalIndex);
    accessLevels.erase(accessLevels.begin()+signalIndex);

    removeSignalFromMap(signalIndex);

    signalNames.erase(signalNames.begin()+signalIndex);
    if (aliases.size()>0) aliases.erase(aliases.begin()+signalIndex);

    for(auto && sigValues : modSignalValues){
      sigValues.erase(sigValues.begin()+signalIndex);
    }

    bitPositions.erase(bitPositions.begin()+signalIndex);

    numSignals = signalNames.size();

    if(check)
      checkBitPositions();
}

void ConfigSignals::removeSignalFromMap(int signalIndex)
{
  //decrement all map entries with value larger than index
  for(unsigned idx = signalIndex+1; idx < signalNames.size(); idx++){
    signalsMap.find(signalNames[idx])->second--;
  }
  signalsMap.erase(signalsMap.find(signalNames[signalIndex]));
  for (auto const & a: aliases[signalIndex]) {
    signalsMap.erase(signalsMap.find(a));
  }
}



void ConfigSignals::setSignalValue(const vector<string> & modulesVec , const string & signalName, uint32_t value)
{
  int sig = sigToIntS(signalName);
  if(sig<0) return;

  if(modulesVec.size()==modules.size()){
    isVarious[sig] = false;
  }else{
    isVarious[sig] = true;
  }

  for(const auto & m: modulesVec){
    int mod = modToInt(m);
    setSignalValue(mod,sig,value);
  }
}


void ConfigSignals::setBitValue(uint32_t mod, uint32_t position, bool value)
{
  const int sig = getSignalNumberFromPosition(position);
  uint32_t oldValue  = getSignalValue(mod,sig);
  uint32_t bit = 0;
  for(const auto & pos : bitPositions[sig]){
    if(pos == position){
      if(value){
        oldValue |= ((uint64_t)1)<<bit;
      }else{
        oldValue &= ~(((uint64_t)1)<<bit);
      }
      setSignalValue(mod,sig,oldValue);
      return;
    }
    bit++;
  }
}

int ConfigSignals::getSignalNumberFromPosition(int position)
{
  string name = bitNames[position];

  while(signalsMap.find(name) == signalsMap.end()){
    name.pop_back();

    if(name.empty()){
      utils::CoutColorKeeper keeper(utils::STDRED);
      cout << "ERROR ConfigSignals: getSignalNumberFromPosition - Signal " << name << " Position " << position << " not found" << endl;
      return 0;
    }
  }

  return sigToInt(name);
}

vector<uint8_t> ConfigSignals::getReadOnlyMask(bool reverse)
{
  uint32_t totalBits  = numModules * numBitsPerModule;
  uint32_t  totalBytes = 1 + (totalBits-1) / 8;

  vector<uint8_t> mask(totalBytes,0);

  for(uint32_t byte=0; byte<totalBytes; byte++){
    for(uint32_t i=0; i<8u; i++){
      uint32_t absBit = byte*8+i;
      uint32_t bitPos = ((reverse)?totalBits- absBit - 1 : absBit) % numBitsPerModule;
      uint32_t sig = getSignalNumberFromPosition(bitPos);
      if(readOnly[sig]){
        mask[byte] |= 1 << i;
      }
    }
  }

  return mask;
}

int ConfigSignals::modToIntS(const std::string & module)
{
  const auto it = modulesMap.find(module);
  if(it == modulesMap.end()){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "ERROR ConfigSignals: Invalid module number: "<< module <<  endl;
    return 0;
  }
  return it->second;
}

int ConfigSignals::sigToIntS(const std::string & signalName)
{
  const auto it = signalsMap.find(signalName);
  if(it == signalsMap.end()){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "ERROR ConfigSignals: SignalName not found: " << signalName << endl;
    return -1;
  }
  return it->second;
}

bool ConfigSignals::checkSignals()
{
  bool allOk = true;

  bool ok = true;
  ok = modulesMap.size() == (unsigned) numModules;
  if(!ok){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "ERROR: ConfigSignals modulesMap wrong size" << endl;
    exit(-1);
  }
  allOk &= ok;

  ok = outputs.size() == (unsigned) numModules;
  if(!ok){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "ERROR: ConfigSignals outputs wrong size" << endl;
    exit(-1);
  }
  allOk &= ok;

  ok = modules.size() == (unsigned) numModules;
  if(!ok){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "ERROR: ConfigSignals modules wrong size" << endl;
    exit(-1);
  }
  allOk &= ok;

  ok = modSignalValues.size() == (unsigned) numModules;
  if(!ok){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "ERROR: ConfigSignals modSignalValues wrong size" << endl;
    exit(-1);
  }
  allOk &= ok;

  for(const auto & signalValues : modSignalValues){
    ok = signalValues.size() == (unsigned) numSignals;
    if(!ok){
      utils::CoutColorKeeper keeper(utils::STDRED);
      cout << "ERROR: ConfigSignals number of signal values wrong size" << endl;
      exit(-1);
    }
  }
  allOk &= ok;

  int numAliases = 0;
  for (const auto & a: aliases) {
    if (!a.empty()) numAliases += a.size();
  }
  int expectedSize = numSignals + numAliases;
  ok = signalsMap.size() == (unsigned) expectedSize;
  if(!ok){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "ERROR: ConfigSignals signalsMap wrong size, expected (numSignals + numAliases) " << expectedSize << " but was " << signalsMap.size() << endl;
    exit(-1);
  }
  allOk &= ok;

  ok = isVarious.size() == (unsigned) numSignals;
  if(!ok){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "ERROR: ConfigSignals isVarious wrong size" << endl;
    exit(-1);
  }
  allOk &= ok;

  ok = readOnly.size() == (unsigned) numSignals;
  if(!ok){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "ERROR: ConfigSignals readOnly wrong size" << endl;
  }
  allOk &= ok;

  ok = activeLow.size() == (unsigned) numSignals;
  if(!ok){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "ERROR: ConfigSignals activeLow wrong size" << endl;
  }
  allOk &= ok;

  ok = accessLevels.size() == (unsigned) numSignals;
  if(!ok){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "WARNING: ConfigSignals accessLevels wrong size" << endl;
    accessLevels.resize(numSignals);
  }
  allOk &= ok;

  ok = signalNames.size()  == (unsigned) numSignals;
  if(!ok){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "ERROR: ConfigSignals signalNames wrong size" << endl;
    exit(-1);
  }
  allOk &= ok;

  ok = bitPositions.size() == (unsigned) numSignals;
  if(!ok){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "ERROR: ConfigSignals bitPositions wrong size" << endl;
    exit(-1);
  }
  allOk &= ok;

  ok = bitNames.size()     == (unsigned) numBitsPerModule;
  if(!ok){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "WARNING: ConfigSignals bitNames wrong size" << endl;
  }
  allOk &= ok;

  return allOk;
}

void ConfigSignals::setSignalAliases(int signalIndex, const std::string & _aliases)
{
  vector<string> aliasesVector;
  utils::split(_aliases,';',aliasesVector,0);
  setSignalAliases(signalIndex, aliasesVector);
}

void ConfigSignals::setSignalAliases(int signalIndex, const std::vector<string> & _aliases)
{
  // remove all and then set them
  for (auto const & a: aliases[signalIndex]) {
    signalsMap.erase(signalsMap.find(a));
  }
  checkRow(signalIndex);
  aliases[signalIndex].clear();
  for (const auto & a : _aliases) {
    aliases[signalIndex].push_back(a);
  }
  for (auto const& a: aliases[signalIndex]){
    if (!a.empty()) signalsMap[a] = signalIndex;
  }
}
