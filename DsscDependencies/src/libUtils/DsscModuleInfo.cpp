#include "utils.h"

#include "DsscModuleInfo.h"

namespace utils{

const std::string DsscModuleInfo::s_moduleInfoFileName = "DsscModuleInfo.ini";

DsscModuleInfo globalModuleInfo;

DsscModuleInfo* DsscModuleInfo::g_DsscModuleInfo = &globalModuleInfo;

ModuleInfo::ModuleInfo()
 : moduleName("F2_2_01"),
   moduleNr(1),
   iobSerial(0x12345678),
   notSendingAsics(0x0000),
   asicIds({"A0","A1","A2","A3","A4","A5","A6","A7",
                "A8","A9","A10","A11","A12","A13","A14","A15"})
{}

ModuleInfo::ModuleInfo(int modNr)
 : ModuleInfo()
{
  moduleNr = modNr;
}

ModuleInfo::ModuleInfo(std::istream & in)
{
  std::string line;
  while (std::getline(in, line))
  {
    if(line.empty()) continue;
    if(line[0] == '#') continue;

    if(line.find("ModuleNr:") != std::string::npos)
    {
      std::vector<std::string> param;
      utils::split(line,':',param,1);
      if(param.size() >= 5){
        int idx = 0;
        moduleNr = std::stoul(utils::stringReplace(param[idx++]," ",""));
        moduleName = utils::stringReplace(param[idx++]," ","");
        iobSerial = std::stoul(utils::stringReplace(param[idx++]," ",""),0,16);
        notSendingAsics = std::stoul(utils::stringReplace(param[idx++]," ",""),0,16);
        std::string asicIdList = utils::stringReplace(param[idx++]," ","");
        std::vector<std::string> asicIdVec;
        utils::split(asicIdList,';',asicIdVec,0);
        int numAsics = std::min(16ul,asicIdVec.size());
        std::copy(asicIdVec.begin(),asicIdVec.begin()+numAsics,asicIds.begin());
      }
      break;
    }
  }
}


void ModuleInfo::print(std::ostream &out) const
{
  out << "ModuleNr: " << moduleNr << " : ";
  out << moduleName << " : ";
  out << getIOBSerialStr() << " : ";
  out << getNotSendingAsicsStr() << " : ";
  out << getAsicIdList() << "\n";
}

std::string ModuleInfo::getIOBSerialStr() const
{
  return utils::getIOBSerialStr(iobSerial);
}

std::string ModuleInfo::getNotSendingAsicsStr() const
{
  std::stringstream iss;
  iss << std::hex << "0x" << std::setw(4) << std::setfill('0') << notSendingAsics;
  return iss.str();
}

std::string ModuleInfo::getAsicIdList() const
{
  std::string list;
  for(auto && asicId : asicIds){
    list += asicId + ";";
  }
  list.pop_back();
  return list;
}


QuadrantInfo::QuadrantInfo()
{}

QuadrantInfo::QuadrantInfo(std::istream & in)
{
  std::string line;
  while (std::getline(in, line))
  {
    if(line.empty()) continue;
    if(line[0] == '#') continue;

    if(line.find("Quadrant:") != std::string::npos)
    {
      std::vector<std::string> param;
      utils::split(line,':',param,1);
      if(param.size() >= 6){
        int idx = 0;
        quadrantNr = std::stoul(utils::stringReplace(param[idx++]," ",""));
        quadrantId = utils::stringReplace(param[idx++]," ","");
        pptIpAddress = utils::stringReplace(param[idx++]," ","");
        idx++; //shortId = std::stoul(utils::stringReplace(param[idx++]," ",""),0,16);
        int numModules = std::stoul(utils::stringReplace(param[idx++]," ",""));
        quadrantInfo = param[idx++].substr(1);

        for(int mod=0; mod<numModules;mod++){
          ModuleInfo newModule(in);
          int moduleNr = newModule.moduleNr;
          moduleInfoMap[moduleNr] = newModule;
        }

        if(moduleInfoMap.empty()){
          std::cout << "DsscModuleInfo: CRITICAL ERROR: No modules for Quadrant defined, will exit now" << std::endl;
          std::cout << "In file "<< utils::getCwd() << "/DsscModuleInfo.ini" << std::endl;
          exit(0);
        }
      }
      break;
    }
  }
}

void QuadrantInfo::print(std::ostream &out) const
{
  out << "# Header  : QuadrantNr:QuadrantId:PPTIPAddress:ShortId:QuadrantInfo\n";
  out << "- Quadrant: " << quadrantNr << " : ";
  out << quadrantId << " : ";
  out << pptIpAddress << " : ";
  out << getShortIdStr() << " : ";
  out << numModules() << " : ";
  out << quadrantInfo << "\n";
  for(auto && module : moduleInfoMap){
    out << "  ";
    module.second.print(out);
  }
}


std::string QuadrantInfo::getShortIdStr() const
{
  std::stringstream iss;
  iss << std::hex << "0x" << std::setw(4) << std::setfill('0') << getShortId();
  return iss.str();
}


uint16_t QuadrantInfo::getShortId() const
  {
  if(quadrantId == "FENICE"){
    return 0x4645; // "FE"
  }else if(quadrantId[0] == 'Q'){
    return 0x5130 + quadrantNr; // "Q1"-"Q4"
  }
  return 0x5453; //"TS" --> TEST
}

std::vector<std::string> QuadrantInfo::getModuleNames() const
{
  std::vector<std::string> moduleNames;
  for(auto && item : moduleInfoMap){
    moduleNames.push_back(item.second.name());
  }
  return moduleNames;
}

std::string QuadrantInfo::getModuleName(int moduleNr) const
{
  const auto it = moduleInfoMap.find(moduleNr);
  if(it != moduleInfoMap.end()){
    return it->second.name();
  }
  return "unknown";
}

ModuleInfo & QuadrantInfo::getModule(int moduleNr)
{
  const auto it = moduleInfoMap.find(moduleNr);
  if(it != moduleInfoMap.end()){
    return it->second;
  }
  std::cout << "Error: QuadrantInfo module not found " << moduleNr << std::endl;

  return moduleInfoMap[1];
}

const ModuleInfo & QuadrantInfo::getModule(int moduleNr) const
{
  const auto it = moduleInfoMap.find(moduleNr);
  if(it != moduleInfoMap.end()){
    return it->second;
  }
  std::cout << "Error: QuadrantInfo module not found " << moduleNr << std::endl;

  return moduleInfoMap.begin()->second;
}



DsscModuleInfo::DsscModuleInfo()
{
  readModuleInfoFile();
}


DsscModuleInfo::~DsscModuleInfo()
{
  saveModuleInfoFile();
}


std::vector<std::string> DsscModuleInfo::getModuleNames(const std::string & quadrantId)
{
  const auto & locInfoMap = g_DsscModuleInfo->quadrantInfoMap;

  std::vector<std::string> moduleNames;
  const auto qit = locInfoMap.find(quadrantId);
  if(qit != locInfoMap.end()){
    const auto & quadMap = locInfoMap.at(quadrantId);
    return quadMap.getModuleNames();
  }else{
    std::cout << "DsscModuleInfo Error: quadrantID not found: " << quadrantId << std::endl;
  }
  return moduleNames;
}


std::string DsscModuleInfo::getModuleName(const std::string & quadrantId, int moduleNr)
{
  std::string moduleName = "unknown";
  const auto & locInfoMap = g_DsscModuleInfo->quadrantInfoMap;
  const auto qit = locInfoMap.find(quadrantId);
  if(qit != locInfoMap.end()){
    const auto & quadMap = locInfoMap.at(quadrantId);
    moduleName = quadMap.getModuleName(moduleNr);
  }else{
    std::cout << "DsscModuleInfo Error: quadrantID not found: " << quadrantId << std::endl;
  }
  return moduleName;
}


int DsscModuleInfo::getModuleNr(const std::string & quadrantId, const std::string & moduleName)
{
  const auto moduleNames = g_DsscModuleInfo->getModuleNames(quadrantId);
  const auto mit = find(moduleNames.begin(),moduleNames.end(),moduleName);
  if(mit != moduleNames.end()){
    return std::distance(moduleNames.begin(),mit);
  }else{
    std::cout << "DsscModuleInfo Error: module not found: " << quadrantId << " /M " << moduleName << std::endl;
  }
  return 0;
}

uint16_t DsscModuleInfo::toShortId(const std::string & quadrantId)
{
  const auto & locInfoMap = g_DsscModuleInfo->quadrantInfoMap;
  const auto qit = locInfoMap.find(quadrantId);
  if(qit != locInfoMap.end()){
    return qit->second.getShortId();
  }
  std::cout << "DsscModuleInfo Error: quadrant found: " << quadrantId << std::endl;

  return 0xFFFF;
}


void DsscModuleInfo::setIOBSerial(const std::string & quadrantId, int moduleNr, uint32_t iobSerial)
{
  auto & locInfoMap = g_DsscModuleInfo->quadrantInfoMap;
  auto qit = locInfoMap.find(quadrantId);
  if(qit != locInfoMap.end()){
    qit->second.getModule(moduleNr).iobSerial = iobSerial;
  }
}

std::string DsscModuleInfo::getIOBSerialStr(const std::string & quadrantId, int moduleNr)
{
  const auto & locInfoMap = g_DsscModuleInfo->quadrantInfoMap;
  const auto qit = locInfoMap.find(quadrantId);
  if(qit != locInfoMap.end()){
    return qit->second.getModule(moduleNr).getIOBSerialStr();
  }
  return "unknown";
}

std::string DsscModuleInfo::getModuleInfoStr(const std::string &quadrantId, int moduleNr,  uint32_t iobSerial)
{
  g_DsscModuleInfo->setIOBSerial(quadrantId,moduleNr,iobSerial);
  std::string moduleInfo;
  moduleInfo += "#QuadrantId:\t" + quadrantId + "\t\n";
  moduleInfo += "#ModuleNr:\t"   + std::to_string(moduleNr) + "\t\n";
  moduleInfo += "#ModuleName:\t" + g_DsscModuleInfo->getModuleName(quadrantId,moduleNr) + "\t\n";
  moduleInfo += "#IOBSerial:\t"  + g_DsscModuleInfo->getIOBSerialStr(quadrantId,moduleNr) +"\t\n";
  return moduleInfo;
}


int DsscModuleInfo::getQuadrantNumber(const std::string & quadrantId){
  const auto & locQuadrantIds = g_DsscModuleInfo->quadrantIds;
  return std::distance(locQuadrantIds.begin(),std::find(locQuadrantIds.begin(),locQuadrantIds.end(),quadrantId));
}

std::string DsscModuleInfo::getQuadrantIdList()
{
  return utils::stringVectorToList(getQuadrantIds(),",");
}


void DsscModuleInfo::readModuleInfoFile()
{
  //utils::printCwd();

  if(utils::checkFileExists(s_moduleInfoFileName))
  {
    std::ifstream in(s_moduleInfoFileName);
    std::string line;
    while (getline(in, line))
    {
      if(line.empty()) continue;
      if(line[0] == '#') continue;

      if(line.find("[Quadrant]") != std::string::npos)
      {
        QuadrantInfo nextQuadrantInfo(in);
        const auto quadId = nextQuadrantInfo.quadrantId;
        quadrantInfoMap[quadId] = nextQuadrantInfo;
        uint quadNr = nextQuadrantInfo.quadrantNr;
        if(quadrantIds.size() < quadNr+1){
          quadrantIds.resize(quadNr+1);
        }
        quadrantIds[quadNr] = quadId;
      }
    }
    in.close();

  }else{
    quadrantIds = {"FENICE","Q1","Q2","Q3","Q4"};

    QuadrantInfo q1;
    q1.quadrantInfo = "test";
    q1.moduleInfoMap = {{1,ModuleInfo(1)},{2,ModuleInfo(2)},{3,ModuleInfo(3)},{4,ModuleInfo(4)}};

    q1.pptIpAddress = "192.168.0.119";
    q1.quadrantId = "Q1";
    q1.quadrantNr = 1;
    quadrantInfoMap[q1.quadrantId] = q1;

    q1.pptIpAddress = "192.168.0.120";
    q1.quadrantId = "Q2";
    q1.quadrantNr = 2;
    quadrantInfoMap[q1.quadrantId] = q1;

    q1.pptIpAddress = "192.168.0.121";
    q1.quadrantId = "Q3";
    q1.quadrantNr = 3;
    quadrantInfoMap[q1.quadrantId] = q1;

    q1.pptIpAddress = "192.168.0.122";
    q1.quadrantId = "Q4";
    q1.quadrantNr = 4;
    quadrantInfoMap[q1.quadrantId] = q1;

    QuadrantInfo fenice;
    fenice.pptIpAddress = "192.168.0.125";
    fenice.quadrantId = "FENICE";
    fenice.quadrantNr = 0;
    fenice.quadrantInfo = "FENICE Ladder";
    fenice.moduleInfoMap = {{1,ModuleInfo(1)}};

    quadrantInfoMap[fenice.quadrantId] = fenice;
  }

  std::cout << "DsscModuleInfo initialization: Loaded " << quadrantInfoMap.size()  << " Quadrants with " << quadrantInfoMap[quadrantIds.front()].numModules() << " Modules" << std::endl;
}


void DsscModuleInfo::saveModuleInfoFile()
{
  std::ofstream out(s_moduleInfoFileName);

  out << "# DSSC Module Information File\n";
  out << "# " << utils::getLocalTimeStr() << "\n";
  out << "#\n";

  for(auto && quadId : quadrantIds){
    out << "[Quadrant]\n";
    quadrantInfoMap.at(quadId).print(out);
  }

  out.close();
  std::cout << "Module Info Saved to " << utils::getCwd() << "/" << s_moduleInfoFileName << std::endl;
}




} // namespace utils
