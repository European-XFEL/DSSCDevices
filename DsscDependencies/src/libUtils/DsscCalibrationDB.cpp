#include <fstream>
#include <sstream>

#include "DsscCalibrationDB.h"

using namespace std;
using namespace SuS;

vector<string> DsscCalibrationSetConfig::s_parameterNames = {"active","numPixels"};

const std::vector<std::string> DsscCalibrationDB::s_dsscCameraComponents {"DSSC-F1-MSDD-Ladder-1-2",
                                                                          "DSSC-F1-MSDD-Ladder-1-3", // FENICE
                                                                          "DSSC-F1-MSDD-Ladder-1-4",
                                                                          "DSSC-F1-MSDD-Ladder-1-5",
                                                                          "DSSC-F1-DEPFET-Ladder-1-6",
                                                                          "DSSC-F2-MSDD-Ladder-2-1",
                                                                          "DSSC-F2-MSDD-Ladder-2-2",
                                                                          "DSSC-F2-MSDD-Ladder-2-3",
                                                                          "DSSC-F2-MSDD-Ladder-2-4",
                                                                          "DSSC-F2-MSDD-Ladder-2-5",
                                                                          "DSSC-F2-MSDD-Ladder-2-6",
                                                                          "DSSC-F2-MSDD-Ladder-2-7",
                                                                          "DSSC-F2-MSDD-Ladder-2-8",
                                                                          "DSSC-F2-MSDD-Ladder-2-9",
                                                                          "DSSC-F2-MSDD-Ladder-2-10",
                                                                          "DSSC-F2-ASIC-A1", // MANNHEIM FLO
                                                                          "DSSC-F2-ASIC-A2",
                                                                          "DSSC-F2-ASIC-A3",
                                                                          "DSSC-F2-ASIC-B1",
                                                                          "DSSC-F2-ASIC-B2",
                                                                          "DSSC-F2-ASIC-B3"
                                                                         };

DsscCalibrationSetConfig::DsscCalibrationSetConfig()
{
  m_dateTime = utils::getLocalTimeStr();
  m_version = generateMajorVersionStr();
  m_gainInfo = "unknown";
  m_componentID = DsscCalibrationDB::s_dsscCameraComponents.front();

  setParamValue("numPixels","65536");
  setParamValue("active","J");
  for(auto && paramName : s_parameterNames){
    setParamValue(paramName,"nA");
  }
}


DsscCalibrationSetConfig::DsscCalibrationSetConfig(const string & line)
{
  updateParameterNames(DsscGainParamMap::s_gainModeParamNames);

  if(line.empty()){
    updateParamsFromLine("");
  }else{
    std::vector<std::string> values;
    utils::split(line,'&',values);

    updateIdFromLine(values.front());
    if(values.size()==2){
      updateParamsFromLine(values.back());
    }
  }
}


std::string DsscCalibrationSetConfig::generateMajorVersionStr()
{
  return "v1.0.0";
}


void DsscCalibrationSetConfig::updateParameterNames(const vector<string> & newNames)
{
  s_parameterNames.clear();
  s_parameterNames = newNames;

  checkParam("active");
  checkParam("numPixels");
}


void DsscCalibrationSetConfig::updateIdFromLine(const string & line)
{
  vector<string> values;
  utils::split(line,';',values);

  while(values.size() < 4){
    values.push_back("nA");
  }

  m_componentID = values[0];
  m_gainInfo    = values[1];
  m_version     = values[2];
  m_dateTime    = values[3];

  if(m_componentID[0] == '<'){
    m_componentID.erase(0,1);
  }
}

void DsscCalibrationSetConfig::updateParamsFromLine(const string & line)
{
  m_parameterValues.clear();
  utils::split(line,';',m_parameterValues);
  while(m_parameterValues.size() < s_parameterNames.size()){
    m_parameterValues.push_back("nA");
  }
}


std::vector<std::string> DsscCalibrationSetConfig::getCalibrationSetConfigValueNames()
{
  std::vector<std::string> configList {"ConfigId",
                                       "Gain",
                                       "Version",
                                       "DateTime"};
  const auto paramNames = getParameterNames();
  configList.insert(configList.end(),paramNames.begin(),paramNames.end());
  return configList;
}


std::vector<std::string> DsscCalibrationSetConfig::getCalibrationSetConfigVec()
{

  std::vector<std::string> configList {getId(),
                                       getGainInfo(),
                                       getVersion(),
                                       getDateTime()
                                      };
  const auto paramValues = getParameterValues();
  configList.insert(configList.end(),paramValues.begin(),paramValues.end());
  return configList;
}


//------------------------------------------------------------------------------------------

DsscCalibrationSet::DsscCalibrationSet(const string & configId)
  : DsscCalibrationSetConfig(configId)
{
}


DsscCalibrationSet::DsscCalibrationSet(istream & in, const string & configIdLine)
 : DsscCalibrationSetConfig(configIdLine)
{
  string line;
  while(getline(in,line)){
    if(line[0] == '#'){continue;}
    if(line[0] == '>'){break;}
    if(line[0] == '-'){
      auto paramName = line.erase(0,1);
      getline(in,line);
      vector<double> values;
      utils::split(line,';',values);
      addCalibrationParamSet(paramName,values);
    }
  }
}


std::string DsscCalibrationSet::updateMinorVersionNumber(const std::string & paramName)
{
  if(!isCalibrationParamSetIncluded(paramName)){
    return generateMajorVersionStr();
  }

  const auto paramVersions = getCalibrationParamVersions(paramName);
  std::string majorVersion = utils::getMajorVersion(paramVersions.back());
  return majorVersion + "." + std::to_string(paramVersions.size());
}



//------------------------------------------------------------------------------------------

DsscCalibrationDB::DsscCalibrationDB()
  : m_activeSet(nullptr),
    m_openThread(nullptr)
{
}


DsscCalibrationDB::DsscCalibrationDB(const std::string & fileName)
  : m_dbFileName(fileName),
    m_activeSet(nullptr),
    m_openThread(nullptr)
{  
  if(utils::checkFileExists(fileName)){
    m_openMutex.lock();
    m_openThread = new std::thread((void(DsscCalibrationDB::*)(void))&DsscCalibrationDB::open,this);
  }else{
    string component = "DSSC_F1_Test";
    string gainInfo = "0.7keV";
    string version  = "v1.0.0";

    // initialize Configuration for Calibration Set
    SuS::DsscCalibrationSetConfig config;
    config.setParamValue("numPixels","65536");
    config.setParamValue("active","J");
    config.setParamValue("FCF_EnCap","0.7");
    config.setParamValue("CSA_FbCap","0");
    config.setParamValue("CSA_Resistor","3");
    config.setParamValue("IntegrationTime","35");

    // open database
    // add new Calibration Set with date time version and componentID (ASIC or Sensor...)
    addNewCalibrationSetAndSetActive(SuS::DsscCalibrationDB::generateNewConfigID(component,gainInfo,version),
                                                                         config.getParameterValues());
  }
}


DsscCalibrationDB::~DsscCalibrationDB()
{
  if(m_dbFileName.rfind(".h5") == string::npos){
    save();
  }

  utils::fileBackup(m_dbFileName);

  if(m_openThread){
    if(m_openThread->joinable()){
      m_openThread->join();
    }

    delete m_openThread;
  }
}



void DsscCalibrationDB::open()
{
  ifstream in(m_dbFileName);
  string line;
  while(getline(in,line)){
    if(line[0] == '#'){continue;}
    if(line[0] == '<'){
      getline(in,line);
      auto newSet = DsscCalibrationSet(in,line);
      m_valuesMap[newSet.getId()] = newSet;
    }
    if(line == "!SetConfigParameterNames:"){
      getline(in,line);
      vector<string> paramNames;
      utils::split(line,';',paramNames);
      DsscCalibrationSetConfig::updateParameterNames(paramNames);
    }
  }

  for(auto && entry : m_valuesMap){
    if(entry.second.isActive()){
      m_activeSet = &entry.second;
      break;
    }
  }

  if(m_activeSet == nullptr){
    if(m_valuesMap.empty()){
      utils::CoutColorKeeper keeper(utils::STDRED);
      cout << "Error: no calibration set found" << endl;
    }else{
      m_activeSet = &(m_valuesMap.begin()->second);
    }
  }

  cout << "Loaded configs:" << endl;
  for(auto && calibSet : m_valuesMap){
    cout << calibSet.first << endl;
  }

  in.close();
  // unlock mutex if openHDF5 is called from constructor where mutex is locked
  if(!m_openMutex.try_lock()){
    m_openMutex.unlock();
  }
}


void DsscCalibrationDB::saveCopyAs(const std::string &fileName)
{
  ofstream out(fileName);

  out << "!SetConfigParameterNames:\n";
  out << DsscCalibrationSetConfig::getParameterNamesList() << "\n";
  for(auto && elem : m_valuesMap){
    out << "<" << elem.first << "\n";
    elem.second.saveCalibrationSet(out);
  }

  utils::CoutColorKeeper keeper(utils::STDGREEN);
  cout << "Calibration DB saved to "  << m_dbFileName << endl;
}

// includes Gain information
std::string DsscCalibrationDB::extractMajorVersion(const std::string & idStr)
{
  std::string versionStr = "v1.0.0";

  std::vector<std::string> fields;
  utils::split(idStr,';',fields);
  if(fields.size() == 1){
    versionStr= fields.front();
  }else if(fields.size() == 4){
    versionStr = fields.at(2);
  }
  return utils::getMajorVersion(versionStr);
}


// includes Gain information
std::string DsscCalibrationDB::extractGainInfo(const std::string & idStr)
{
  std::vector<std::string> fields;
  utils::split(idStr,';',fields);
  if(fields.size() < 4) return "Gain_nA";
  return fields.at(1);
}

std::string DsscCalibrationDB::extractComponentId(const std::string & idStr)
{
  std::vector<std::string> fields;
  utils::split(idStr,';',fields);
  if(fields.size() < 4) return "";
  return fields.front();
}


std::string DsscCalibrationDB::extractDateTime(const std::string & idStr)
{
  std::vector<std::string> fields;
  utils::split(idStr,';',fields);
  if(fields.size() < 4) return "";
  return fields.back();
}



std::string DsscCalibrationDB::generateNewConfigID(const std::string & oldConfigId, int minorVersion)
{
  std::string majorVersion = extractMajorVersion(oldConfigId);

  std::vector<std::string> fields;
  utils::split(oldConfigId,';',fields);

  if(fields.size() != 4){
    throw "Bad format of config Id";
  }
  const std::string componentId = fields.at(0);
  const std::string gainInfo = fields.at(1);
  const std::string newVersionStr = majorVersion + "." + std::to_string(minorVersion);

  return componentId + ";" + gainInfo + ";" + newVersionStr + ";" + fields.back();
}


std::string DsscCalibrationDB::updateMinorVersionNumber(const std::string & configId)
{
  // find component and major version count:
  string majorGainVersion = extractMajorVersion(configId); // incl Gain Information
  string componentId = extractComponentId(configId);

  int minorVersion = 0;
  for(auto && set : m_valuesMap){
    auto currentId = set.first;
    string ver = extractMajorVersion(currentId);
    string comp = extractComponentId(currentId);
    if(ver == majorGainVersion && componentId == comp){
      minorVersion++;
    }
  }

  auto newId = configId;

  if(minorVersion>0){
    newId = generateNewConfigID(configId,minorVersion);
    utils::CoutColorKeeper keeper(utils::STDGREEN);
    std::cout << "DsscCalibrationDB - Info: Updated Minor Version " << newId << endl;
  }
  return newId;
}


// add set without data only from string
std::string DsscCalibrationDB::addNewCalibrationSet(const std::string & configId, const std::vector<std::string> & configParameters)
{
  auto finalId = updateMinorVersionNumber(configId);
  return addNewCalibrationSet(DsscCalibrationSet(finalId+"&"+utils::join(configParameters,";")));
}


// add set with data from DsscCalibrationSet class
std::string DsscCalibrationDB::addNewCalibrationSet(const DsscCalibrationSet & newSetToAdd)
{
  auto finalId = updateMinorVersionNumber(newSetToAdd.getId());
  m_valuesMap[finalId] = newSetToAdd;

  utils::CoutColorKeeper keeper(utils::STDGREEN);
  std::cout << "DsscCalibrationDB - Info: New Calibration Set added: " << finalId << std::endl;
  return finalId;
}


std::string DsscCalibrationDB::addNewCalibrationSetAndSetActive(const std::string & configId, const std::vector<std::string> & configParameters)
{
  std::unique_lock<std::mutex> lock(m_openMutex);
  auto finalId = addNewCalibrationSet(configId,configParameters);
  setConfigActive(finalId);
  return finalId;
}


DsscCalibrationSet* DsscCalibrationDB::setConfigActive(const std::string & configId){

  auto elem = m_valuesMap.find(configId);
  if(elem == m_valuesMap.end()){
    return nullptr;
  }

  for(auto && elem : m_valuesMap){
    elem.second.deactivate();
  }

  m_activeSet = &(elem->second);
  m_activeSet->activate();

  return m_activeSet;
}


std::vector<std::string> DsscCalibrationDB::getConfigurationIds() const
{
  std::vector<std::string> configIds(m_valuesMap.size());

  int idx = 0;
  for(auto && elem : m_valuesMap){
    configIds[idx++] = elem.first;
  }
  return configIds;
}


std::vector<std::string> DsscCalibrationDB::getConfigurationIdsOfComponent(const std::string & componentId) const
{
  std::vector<std::string> configIds;

  for(auto && elem : m_valuesMap){
    auto id = elem.first;
    if(componentId == DsscCalibrationDB::extractComponentId(id))
    configIds.push_back(id);
  }
  sort(configIds.begin(),configIds.end());
  return configIds;
}

std::vector<std::string> DsscCalibrationDB::getGainInfos(const std::string & componentId) const
{
  std::vector<std::string> gainInfos;

  const auto configIds = getConfigurationIdsOfComponent(componentId);
  for(auto && configId : configIds){
    auto gainInfoStr = extractGainInfo(configId);
    gainInfos.push_back(gainInfoStr);
  }

  utils::removeDuplicates(gainInfos);
  return gainInfos;
}



const DsscCalibrationSet & DsscCalibrationDB::getCalibrationSet(const std::string & configId) const
{
  auto elem = m_valuesMap.find(configId);
  if(elem == m_valuesMap.end()){
    throw;
  }
  return elem->second;
}


std::vector<std::string> DsscCalibrationDB::getComponentIds() const
{
  std::vector<std::string> componentIds(m_valuesMap.size());

  int idx = 0;
  for(auto && elem : m_valuesMap){
    componentIds[idx++] = extractComponentId(elem.first);
  }
  utils::removeDuplicates(componentIds);
  return componentIds;
}

void DsscCalibrationDB::initializeActiveCalibrationConfig(int numPixels, const std::string & componentId, const std::string & gainStr, const DsscGainParamMap & paramMap)
{
  std::vector<std::string> calibSetParameterNames {"active","numPixels"};
  std::string gainParamValues = "J;" + to_string(numPixels) + ";";

  for(auto && calibParam : DsscGainParamMap::s_gainModeParamNames){
    calibSetParameterNames.push_back(calibParam);
    gainParamValues += paramMap.at(calibParam) + ";";
  }
  DsscCalibrationSetConfig::updateParameterNames(calibSetParameterNames);

  std::string initLine = generateNewConfigID(componentId,gainStr,DsscCalibrationSetConfig::generateMajorVersionStr());
  initLine += "&" + gainParamValues;
  m_activeConfig = DsscCalibrationSetConfig(initLine);
}


std::string DsscCalibrationDB::addCalibrationSet(const std::string & calibParamName, const std::vector<double> & calibrationParameterValues)
{
  if(newCalibrationSetRequired(calibParamName)){
    addNewCalibrationSetAndSetActive(m_activeConfig.getId(),m_activeConfig.getParameterValues());
  }
  m_activeSet->addCalibrationParamSet(calibParamName,calibrationParameterValues);
  return m_activeSet->getId();
}

bool DsscCalibrationDB::newCalibrationSetRequired(const std::string & calibParamName)
{
  auto oldId = m_activeConfig.getId();
  auto newId = updateMinorVersionNumber(oldId);

  // if equal no set with component and major version number (incl Gain Info) is existing
  if(newId == oldId){
    return true;
  }else if(isCalibParamExisting(calibParamName)){
    return true;
  }
  return false;
}


bool DsscCalibrationDB::isCalibParamExisting(const std::string & calibParamName)
{
  setConfigActive(m_activeConfig.getId());
  return m_activeSet->isCalibrationParamSetIncluded(calibParamName);
}


bool DsscCalibrationDB::isCalibrationSetExisting(const std::string & componentId, const std::string & gainInfo) const
{
  const auto exCompIds = getComponentIds();
  if(find(exCompIds.begin(),exCompIds.end(),componentId) == exCompIds.end()) {
    return false;
  }

  const auto exGainInfos = getGainInfos(componentId);
  if(find(exGainInfos.begin(),exGainInfos.end(),gainInfo) == exGainInfos.end()){
    return false;
  }
  return true;
}


std::vector<std::string> DsscCalibrationDB::getConfigIdsForComponentGain(const std::string & componentId, const std::string & gainInfo) const
{
  std::vector<std::string> compGainIds;
  auto componentIds = getConfigurationIdsOfComponent(componentId);
  for(auto && compId : componentIds){
    if(extractGainInfo(compId) == gainInfo){
      compGainIds.push_back(compId);
    }
  }

  return compGainIds;
}


std::string DsscCalibrationDB::getMaxVerConfigId(const std::vector<std::string> & configIds)
{
  std::vector<std::string> versions;
  for(auto && configId : configIds){
    versions.push_back(extractGainInfo(configId));
  }
  auto maxVersion = utils::getMaxVersion(versions);
  for(auto && configId : configIds){
    if(extractGainInfo(configId) == maxVersion){
      return configId;
    }
  }

  // if something went wrong or configIds is empty
  return "";
}


std::string DsscCalibrationDB::getMaxVerConfigIdForComponentGain(const std::string & componentId, const std::string & gainInfo) const
{
  const auto configGainIDs = getConfigIdsForComponentGain(componentId,gainInfo);
  return getMaxVerConfigId(configGainIDs);
}


bool DsscCalibrationDB::activateLatestConfig(const std::string & componentId, const std::string & gainInfo){
  bool ok = isCalibrationSetExisting(componentId,gainInfo);
  if(ok){
    const auto latestConfigId = getMaxVerConfigIdForComponentGain(componentId,gainInfo);
    setConfigActive(latestConfigId);
  }else{
    utils::CoutColorKeeper keeper(utils::STDBROWN);
    cout << "Warning: no calibration config found for "<< componentId << " and " << gainInfo << endl;
  }
  return ok;
}

utils::DataHisto::DNLValuesVec DsscCalibrationDB::getPixelBinningInformation(uint32_t pixel, int irampSetting)
{
  auto binningSetName = getBinningParamName(pixel,irampSetting);
  if(!m_activeSet->isCalibrationParamSetIncluded(binningSetName)){
    return utils::DataHisto::DNLValuesVec();
  }

  return utils::singleVecToDNLValues(m_activeSet->getCalibrationParamSet(binningSetName));
}



