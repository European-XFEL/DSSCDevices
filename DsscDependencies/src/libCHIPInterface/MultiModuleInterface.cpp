#include "LOGO.h"
#include "utils.h"

#include "MultiModuleInterface.h"

#define ERROR_OUT(message)  \
{       errorString = message;  \
        debugOut << errorString << "\n"; \
        errorMessages.push_back(errorString); }

#define DEBUG_OUT(message) \
{       debugOut << message << "\n"; \
        cout << utils::STDGRAY << message << utils::STDNORM << endl;  }

#define WARN_OUT(message) \
{       debugOut << message << "\n"; \
        cout << message << endl;  }

#define ERROR(message) \
{       debugOut << message << "\n"; \
        cout << utils::STDRED << message << utils::STDNORM << endl;  }

#define CHECK_ETH_CHANNEL(chan) \
  if(chan<1 || chan>4)          \
    cout << "Channel not valid 2<=chan<=4" << endl;\
    chan--;

using namespace SuS;
using namespace std;

std::ofstream MultiModuleInterface::debugOut("Debug.log", ofstream::out);

MultiModuleInterface::MultiModuleInterface(PPTFullConfig * fullConfig)
#ifdef F1IO
  :  CHIPInterface(fullConfig),
#elif F2IO
  : F2CHIPInterface(fullConfig),
#else
  : F2BCHIPInterface(fullConfig),
#endif
     pptFullConfig(fullConfig),
     epcRegisters(fullConfig->getEPCReg()),
     iobRegisters(fullConfig->getIOBReg()),
     currentModule(1)
{
  if(pptFullConfig->isGood()){
    if(sequencer->getHoldEnabled()){
      sequencer->setHoldEnabled(false);
      WARN_OUT("WARNING DSSC_PPT: Sequencer Hold disabled");
    }
  }else{
    ERROR_OUT("ERROR Full Config could not be loaded");
  }

  numOfPxs    = utils::s_numAsicPixels;
  totalNumPxs = numOfPxs;
  sramSize    = utils::s_numSram;

  //maybe this should be loaded always when used or after new config is loaded?
  c_controlBitsPerPx = pixelRegisters->getNumBitsPerModule("Control register");

  c_pxsPerCol = 64;
  c_pxsPerRow = 64;
  c_WordsInTrailer = 7;
  c_wordsInHeader  = 1;

  //setActiveAsics(1<<12); // Mannheim Test Setup ASIC 12 DO 11
  setActiveAsics(0xFFFF); // DSSC Setup

  updateMemberVectorSizes();

  setDefaultBurstParamValues();

  fillLastLoadedETHConfig();

  if(pptFullConfig->isGood())
      DEBUG_OUT("++++ MultiModuleInterface initialized! ++++");
}


void MultiModuleInterface::storeFullConfigFile(const string & fileName, bool keepName)
{
  pptFullConfig->storeFullConfigFile(fileName,keepName);
}


void MultiModuleInterface::loadFullConfig(const std::string & fileName, bool program)
{
  pptFullConfig->loadFullConfig(fileName);

  sequencer      = pptFullConfig->getSequencer();
  epcRegisters   = pptFullConfig->getEPCReg();
  iobRegisters   = pptFullConfig->getIOBReg();

  // required for old configs where outputs of jtag regs 2,3,4 are not updated for ladder mode.
  int modRem = currentModule;
  for(int mod = 1; mod<=4; mod++){
    setActiveModule(mod);
    updateNumberOfModulesInJtagRegisters();
    updateNumberOfModulesInPixelRegisters();
  }

  fillLastLoadedETHConfig();

  setActiveModule(modRem);

  if(program){
    initSystem();
  }
}


void MultiModuleInterface::setDefaultBurstParamValues()
{
  DEBUG_OUT("Set Burst Param Values to default values");
  burstParams["start_wait_time"] = 15000;  //?s after start_burst signal
  burstParams["start_wait_offs"] =   -50;  // constant offset for fine alignemnt of start time
  burstParams["gdps_on_time"]    =  3000;  //?s after start_burst signal 10ms before burst (GDPS ON)
  burstParams["fet_on_time"]     =     8;  //?s before iprog
  burstParams["clr_on_time"]     =     5;  //?s before burst
  burstParams["clr_cycle"]       =     3;  //cyc in cycle
  burstParams["iprog_clr_duty"]  =    MICROSECOND;  //cyc in cycle
  burstParams["iprog_clr_offset"]=   (burstParams["clr_on_time"]-1)*MICROSECOND;  //cyc in cycle
  burstParams["SW_PWR_ON"]       =    20;  //?s before iprog
  burstParams["iprog_clr_en"]    =     0;  //enable Clear during IPROG?
  burstParams["single_cap_en"]   =     0;  //enable Clear during IPROG?
}


void MultiModuleInterface::setBurstParam(const std::string &paramName, int value, bool program)
{
  if(burstParams.find(paramName) != burstParams.end()){
    burstParams[paramName] = value;

    if(program){
      if(paramName == "start_wait_offs"){
        updateStartWaitOffset(value);
      }else{
        updateAllCounters();
      }
    }
  }else{
    ERROR("DSSC_PPT BurstParamError: " << paramName << " unknown!");
  }
}


std::vector<std::string> MultiModuleInterface::getBurstParamNames() const
{
  std::vector<std::string> paramNames;

  for(const auto & param : burstParams){
    paramNames.emplace_back(param.first);
  }

  return paramNames;
}


void MultiModuleInterface::setSendingAsics(uint16_t asics)
{
  sendingAsics = asics;

  DEBUG_OUT("DSSC_PPT Info: Active Asics set to: " << getActiveAsicsStr());
  DEBUG_OUT("DSSC_PPT Info: Sending Asics set to: " << getSendingAsicsStr());
}


void MultiModuleInterface::setActiveAsics(uint16_t asics)
{
  activeAsics   = asics;
  setSendingAsics(asics);

  updateNumXors();

  int numAsics = getNumberOfActiveAsics();

  totalNumPxs = numOfPxs * numAsics;
  updateMemberVectorSizes();

  updateNumberOfModulesInRegisters();
}


void MultiModuleInterface::updateNumberOfModulesInRegisters()
{
  for(int module=1; module<=4; module++){
    setActiveModule(module);
    updateNumberOfModulesInJtagRegisters();
    updateNumberOfModulesInPixelRegisters();
  }
}


void MultiModuleInterface::updateNumberOfModulesInPixelRegisters()
{
  const string set = "Control register";
  const int pxNewNumMods = totalNumPxs;
  const int pxExtNumMods = pixelRegisters->getNumModules(set);
  const int pxExtNumOuts = pixelRegisters->getOutputs(set).size();

  if((pxNewNumMods == pxExtNumMods) && (pxNewNumMods == pxExtNumOuts)){
    return;
  }

  if(pxExtNumMods < pxNewNumMods){
    pixelRegisters->addModules(set,to_string(pxExtNumMods)+"-"+to_string(pxNewNumMods-1));
  }else if (pxExtNumMods > pxNewNumMods){
    pixelRegisters->removeModules(set,to_string(pxNewNumMods)+"-"+to_string(pxExtNumMods-1));
  }

  pixelRegisters->setOutputs(set,getPixelRegOutputList());
}


void MultiModuleInterface::updateNumberOfModulesInJtagRegisters()
{
  const int newNumMods = getNumberOfActiveAsics();
  const int extNumMods = jtagRegisters->getNumModules("Global Control Register");
  const int extNumOuts = jtagRegisters->getOutputs("Global Control Register").size();
  const auto moduleSets = jtagRegisters->getModuleSetNames();

  if((newNumMods == extNumMods) && (newNumMods == extNumOuts)){
    return;
  }

  if(extNumMods < newNumMods){
    for(const auto & set : moduleSets){
      jtagRegisters->addModules(set,to_string(extNumMods)+"-"+to_string(newNumMods-1));
    }
  }else if(extNumMods > newNumMods){
    for(const auto & set : moduleSets){
      jtagRegisters->removeModules(set,to_string(newNumMods)+"-"+to_string(extNumMods-1));
    }
  }

  for(const auto & set : moduleSets){
    jtagRegisters->setOutputs(set,getJtagRegOutputList());
  }
}


int MultiModuleInterface::getFirstActiveAsic() const
{
  int asic = utils::getFirstOneInValue(activeAsics)	;
  if(asic<0){
    WARN_OUT("Warning: no ASIC is active - may cause strange behaviour");
    return 0;
  }

  return asic;
}


string MultiModuleInterface::getAllPixelStringOfASIC(int asic)
{
  vector<int> imagePixels(numOfPxs);
  for(int px=0; px<numOfPxs; px++){
    imagePixels[px] = getImagePixelNumber(asic,px);
  }

  sort(imagePixels.begin(),imagePixels.end());

  return utils::positionVectorToList(imagePixels);
}


void MultiModuleInterface::setNumberOfActiveAsics(int numAsics)
{
  if(numAsics<1 || numAsics > 16){
    ERROR("Num Asics out of range: " << numAsics);
    return;
  }
  setActiveAsics((1<<numAsics)-1);
}


int MultiModuleInterface::getNumberOfSendingAsics() const
{
  if(sendingAsics ==      1) return  1;
  if(sendingAsics == 0xFFFF) return 16;

  return utils::countOnesInInt(sendingAsics);
}


int MultiModuleInterface::getNumberOfActiveAsics() const
{
  //shortcuts for certain cases
  if(activeAsics ==      1) return  1;
  if(activeAsics == 0xFFFF) return 16;

  return utils::countOnesInInt(activeAsics);
}

bool MultiModuleInterface::isSingleAsicMode() const
{
  return (getNumberOfActiveAsics() == 1);
}


void MultiModuleInterface::setActiveModule(int modNumber)
{
  if(modNumber<1 || modNumber>4){
    ERROR("ERROR:Modnumber out of range: " << modNumber);
    return;
  }

  DEBUG_OUT("Set Active Module = " << modNumber);

  currentModule = modNumber;
  pixelRegisters = pptFullConfig->getPixelReg(currentModule-1);
  jtagRegisters  = pptFullConfig->getJtagReg(currentModule-1);

  setD0Mode(checkD0Mode(),checkBypCompression());

  //setEPCParam("DataRecv_to_Eth0_Register","all","sel_eth0_datareceiver",currentModule-1);
}


void MultiModuleInterface::updateEPCRegister(const string & fileName)
{
  epcRegisters->initFromFile(fileName);
}

void MultiModuleInterface::updateIOBRegister(const string & fileName)
{
  iobRegisters->initFromFile(fileName);
}

void MultiModuleInterface::updateJTAGRegister(const string & fileName)
{
  jtagRegisters->initFromFile(fileName);
}

void MultiModuleInterface::updatePixelRegister(const string & fileName)
{
  pixelRegisters->initFromFile(fileName);
}

void MultiModuleInterface::updateSequencer(const string & fileName)
{
  sequencer->loadFile(fileName);
}


uint32_t MultiModuleInterface::getJTAGCurrentModule()
{
  return ASIC_JTAG_ENGINE_1_OFFSET + (currentModule-1)*4;
}



bool MultiModuleInterface::setLogoConfig(const std::string & signalName, uint32_t value)
{
  if(getNumberOfActiveAsics() != 16){
    ERROR_OUT("ERROR: LOGO only available in full ladder 16 asic configuration");
    return false;
  }

  LOGO::init();

  for(int px = 0; px < 128*512; px++){
    if(LOGO::image[px]){
      pixelRegisters->setSignalValue("Control register", to_string(px),signalName,value);
    }
  }

  return programPixelRegs(false);
}


bool MultiModuleInterface::measureBurstParamSweep( const std::string & burstParamName,
                             const std::vector<int> & paramValues,
                             const std::vector<uint32_t> & measurePixels,
                             std::vector<std::vector<double>> & binValues, const int STARTADDR, const int ENDADDR)
{
  runTrimming = true;

  int paramIdx = 0;
  for(const auto & value : paramValues)
  {
    cout << "MultiModuleInterface:: Set "<< burstParamName << " to " << value << "/" << paramValues.back();
    setBurstParam(burstParamName,value);

    const auto measuredData = measureBurstData(measurePixels,STARTADDR,ENDADDR,baselineValuesValid);

    int idx = 0;
    for(const auto & px : measurePixels){
      binValues[px][paramIdx] = measuredData[idx++];
    }

    paramIdx++;
    if(!runTrimming){break;}
  }

  return runTrimming;
}

vector<double> MultiModuleInterface::sweepBurstWaitOffset(uint32_t pixel, const std::vector<int> & paramValues, const int STARTADDR, const int ENDADDR)
{
  const string paramName = "start_wait_offs";

  vector<uint32_t> measurePixels(1,pixel);
  vector<vector<double>> binValues(totalNumPxs,vector<double>(paramValues.size(),0));

  measureBurstParamSweep(paramName,paramValues,measurePixels,binValues,STARTADDR,ENDADDR);

  auto pixelBinValues = binValues[pixel];

  cout << "Pixel " << pixel << " " << paramName << " sweep:" << endl;
  for(const auto & binValue : pixelBinValues){
    cout << setw(8) << setprecision(2) << binValue;
  }
  cout << endl;
  for(const auto & value : paramValues){
    cout << setw(5) << value;
  }
  cout << endl << endl;
  auto maxElem = std::max_element(pixelBinValues.begin(),pixelBinValues.end());
  int maxIdx = maxElem-pixelBinValues.begin();
  cout << "MaxValue = " << *maxElem << " at setting " << paramValues[maxIdx]<< endl << endl;

  // TODO: maybe automtated find maximum entry
  return pixelBinValues;
}



string MultiModuleInterface::getETHIP(int channel, bool recvNotSend)
{
  CHECK_ETH_CHANNEL(channel)

  string signalName;
  string moduleSet = "10GE_Engine" + to_string(channel) + "_Control";
  string ethNumStr = "eth" + to_string(channel);
  if(recvNotSend){
    signalName = ethNumStr + "_destination_address";
  }else{
    signalName = ethNumStr + "_source_address";
  }
  uint32_t ip;
  ip  = epcRegisters->getSignalValue(moduleSet,"0",signalName);
  return utils::ipToString(ip);
}


void MultiModuleInterface::enableLastLoadedETHConfig()
{
  for(int i=1; i<5; i++){
    const auto & config = m_lastPPTETHConfig[i-1];
    const auto & sender = config.first;
    const auto & receiver = config.second;

    setETHMAC (i,false,sender.mac,false);
    setETHIP  (i,false,sender.ip,false);
    setETHPort(i,false,sender.port,false);

    setETHMAC (i,true,receiver.mac,false);
    setETHIP  (i,true,receiver.ip,false);
    setETHPort(i,true,receiver.port,false);

    cout << "Receiver " << i << " IP: " << receiver.ip << endl;
  }
}


void MultiModuleInterface::fillLastLoadedETHConfig()
{
  for(int i=1; i<5; i++){
    auto & config = m_lastPPTETHConfig[i-1];
    auto & sender = config.first;
    auto & receiver = config.second;

    sender.ip   = getETHIP(i,false);
    sender.mac  = getETHMAC(i,false);
    sender.port = getETHPort(i,false);

    receiver.ip   = getETHIP(i,true);
    receiver.mac  = getETHMAC(i,true);
    receiver.port = getETHPort(i,true);
  }
}


string MultiModuleInterface::getETHMAC(int channel, bool recvNotSend)
{
  CHECK_ETH_CHANNEL(channel)

  string signalName0;
  string signalName1;
  string moduleSet = "10GE_Engine" + to_string(channel) + "_Control";
  string ethNumStr = "eth" + to_string(channel);

  uint64_t mac;
  if(recvNotSend){
    signalName0 = ethNumStr + "_recv_mac_addr0";
    signalName1 = ethNumStr + "_recv_mac_addr1";
    mac  = (uint32_t)(epcRegisters->getSignalValue(moduleSet,"0",signalName1) & 0xFFFFFFFF);
    mac <<= 16;
    mac += (uint32_t)(epcRegisters->getSignalValue(moduleSet,"0",signalName0) & 0xFFFF);
  }else{
    signalName0 = ethNumStr + "_this_mac_addr0";
    signalName1 = ethNumStr + "_this_mac_addr1";
    mac  = (uint32_t)(epcRegisters->getSignalValue(moduleSet,"0",signalName1) & 0xFFFF);
    mac <<= 32;
    mac += (uint32_t)(epcRegisters->getSignalValue(moduleSet,"0",signalName0) & 0xFFFFFFFF);
  }

  return utils::macToString(mac);
}

//starts counting at 1
uint32_t MultiModuleInterface::getETHPort(int channel, bool recvNotSend)
{
  CHECK_ETH_CHANNEL(channel)

  string signalName;
  string moduleSet = "10GE_Engine" + to_string(channel) + "_Control";
  string ethNumStr = "eth" + to_string(channel);
  if(recvNotSend){
    signalName = ethNumStr + "_destination_port";
  }else{
    signalName = ethNumStr + "_source_port";
  }
  return epcRegisters->getSignalValue(moduleSet,"0",signalName);
}

//starts counting at 1
void MultiModuleInterface::setETHIP(int channel, bool recvNotSend, const string & addr, bool program)
{
  //if(recvNotSend)
  //cout << "setETH. channel: " << channel << " addr: " << &addr << endl;

  CHECK_ETH_CHANNEL(channel)

  string signalName;
  string moduleSet = "10GE_Engine" + to_string(channel) + "_Control";
  string ethNumStr = "eth" + to_string(channel);
  if(recvNotSend){
    signalName = ethNumStr + "_destination_address";
  }else{
    signalName = ethNumStr + "_source_address";
  }
  uint32_t ip = utils::ipToInt(addr);
  epcRegisters->setSignalValue(moduleSet,"0",signalName,ip);

  if(program){
    programEPCRegister(moduleSet);
  }
}


void MultiModuleInterface::setETHMAC(int channel, bool recvNotSend, const string & addr, bool program)
{

  CHECK_ETH_CHANNEL(channel)

  string signalName0;
  string signalName1;
  string moduleSet = "10GE_Engine" + to_string(channel) + "_Control";
  string ethNumStr = "eth" + to_string(channel);

  uint64_t mac = utils::macToInt(addr);
  uint32_t value0;
  uint32_t value1;

  if(recvNotSend){
    signalName0 = ethNumStr + "_recv_mac_addr0";
    signalName1 = ethNumStr + "_recv_mac_addr1";

    value0 = (uint32_t)( mac      &     0xFFFF);
    value1 = (uint32_t)((mac>>16) & 0xFFFFFFFF);
  }else{
    signalName0 = ethNumStr + "_this_mac_addr0";
    signalName1 = ethNumStr + "_this_mac_addr1";

    value0 = (uint32_t)( mac &  0xFFFFFFFF);
    value1 = (uint32_t)((mac>>32) & 0xFFFF);
  }

  epcRegisters->setSignalValue(moduleSet,"0",signalName0,value0);
  epcRegisters->setSignalValue(moduleSet,"0",signalName1,value1);

  if(program){
    programEPCRegister(moduleSet);
  }
}


void MultiModuleInterface::setETHPort(int channel, bool recvNotSend, uint32_t port, bool program)
{
  CHECK_ETH_CHANNEL(channel)

  string signalName;
  string moduleSet = "10GE_Engine" + to_string(channel) + "_Control";
  string ethNumStr = "eth" + to_string(channel);
  if(recvNotSend){
    signalName = ethNumStr + "_destination_port";
  }else{
    signalName = ethNumStr + "_source_port";
  }
  epcRegisters->setSignalValue(moduleSet,"0",signalName,port);

  if(program){
    programEPCRegister(moduleSet);
  }
}

#ifdef HAVE_HDF5

void MultiModuleInterface::updateConfigRegister(const DsscHDF5RegisterConfig & newRegisterConfig)
{
  if(newRegisterConfig.registerName == "PixelRegister"){
    updateConfigRegister(pixelRegisters,newRegisterConfig);
  }else if(newRegisterConfig.registerName == "JtagRegister"){
    updateConfigRegister(jtagRegisters,newRegisterConfig);
  }else if(newRegisterConfig.registerName == "IOBRegister"){
    updateConfigRegister(iobRegisters,newRegisterConfig);
  }else if(newRegisterConfig.registerName == "EPCRegister"){
    updateConfigRegister(epcRegisters,newRegisterConfig);
  }
}


void MultiModuleInterface::updateConfigRegister(ConfigReg * reg, const DsscHDF5RegisterConfig & newRegisterConfig)
{
// not yet implemented needed for readback in Karabo
}

DsscHDF5RegisterConfig MultiModuleInterface::getRegisterConfig(const std::string & registerName, const std::string & configFileName)
{
  SuS::ConfigReg reg(configFileName);
  return getRegisterConfig(registerName,&reg);
}

DsscHDF5RegisterConfig MultiModuleInterface::getRegisterConfig(const std::string & registerName,  ConfigReg *reg)
{
  DsscHDF5RegisterConfig h5RegConfig;
  h5RegConfig.registerName = registerName;
  h5RegConfig.numModuleSets = reg->getNumModuleSets();

  h5RegConfig.moduleSets = reg->getModuleSetNames();

  for(auto & modSetName :  h5RegConfig.moduleSets){
    h5RegConfig.addresses.push_back(reg->getRegAddress(modSetName));
    h5RegConfig.numBitsPerModule.push_back(reg->getNumBitsPerModule(modSetName));
    h5RegConfig.numberOfModules.push_back(reg->getNumModules(modSetName));
    h5RegConfig.modules.push_back(utils::positionListToVector(reg->getModuleNumberList(modSetName)));
    h5RegConfig.outputs.push_back(utils::positionListToVector(reg->getOutputList(modSetName)));
    h5RegConfig.setIsReverse.push_back(reg->isSetReverse(modSetName));
    h5RegConfig.numSignals.push_back(reg->getNumSignals(modSetName));
    h5RegConfig.signalNames.push_back(reg->getSignalNames(modSetName));

    std::vector<std::string> bitPosMask;

    std::vector<uint32_t> readOnlyMask;
    std::vector<uint32_t> activeLowMask;
    std::vector<uint32_t> accessLevelsMask;
    for(auto & signalName : h5RegConfig.signalNames.back()){
      bitPosMask.push_back(reg->getSignalPositionsList(modSetName,signalName));
      readOnlyMask.push_back(reg->isSignalReadOnly(modSetName,signalName));
      activeLowMask.push_back(reg->isSignalActiveLow(modSetName,signalName));
      accessLevelsMask.push_back(reg->getAccessLevel(modSetName,signalName));
    }
    h5RegConfig.bitPositions.push_back(bitPosMask);
    h5RegConfig.readOnly.push_back(readOnlyMask);
    h5RegConfig.activeLow.push_back(activeLowMask);
    h5RegConfig.accessLevels.push_back(accessLevelsMask);
  }

  // image like data arrays of config data numModuleSets x numSignals x numModules
  int modSet = 0;
  for(const auto & modSetName : h5RegConfig.moduleSets){
    std::vector<std::vector<uint32_t>> moduleSetValues;
    for(const auto & signalName : h5RegConfig.signalNames[modSet]){
      moduleSetValues.emplace_back(reg->getSignalValues(modSetName,"all",signalName));
    }
    h5RegConfig.registerData.emplace_back(moduleSetValues);
    modSet++;
  }

  return h5RegConfig;
}


DsscHDF5ConfigData MultiModuleInterface::getHDF5ConfigData(const std::string & fullConfigFileName)
{
  SuS::PPTFullConfig pptConfig(fullConfigFileName);

  DsscHDF5ConfigData configData;

  configData.timestamp = utils::getLocalTimeStr();

  for(int idx=0; idx<pptConfig.numPixelRegs(); idx++){
    configData.pixelRegisterDataVec.push_back(getRegisterConfig("PixelRegister Module " + to_string(idx+1),pptConfig.getPixelReg(idx)));
  }
  for(int idx=0; idx<pptConfig.numJtagRegs(); idx++){
    configData.jtagRegisterDataVec.push_back(getRegisterConfig("JtagRegister Module " + to_string(idx+1),pptConfig.getJtagReg(idx)));
  }
  if(!pptConfig.getIOBRegsFileName().empty()){
    configData.iobRegisterData = getRegisterConfig("IOBRegister",pptConfig.getIOBReg());
  }
  if(!pptConfig.getEPCRegsFileName().empty()){
    configData.epcRegisterData = getRegisterConfig("EPCRegister",pptConfig.getEPCReg());
  }

  if(!pptConfig.getSequencerFileName().empty()){
    Sequencer * sequencer = pptConfig.getSequencer();
    if(sequencer->configGood){
      configData.sequencerData = sequencer->getSequencerParameterMap();
    }
  }

  return configData;
}


DsscHDF5ConfigData MultiModuleInterface::getHDF5ConfigData(SuS::MultiModuleInterface * mmi)
{
  DsscHDF5ConfigData configData;
  configData.timestamp = utils::getLocalTimeStr();

  configData.pixelRegisterDataVec.push_back(getRegisterConfig("PixelRegister",mmi->getRegisters("pixel")));

  configData.jtagRegisterDataVec.push_back(getRegisterConfig("JtagRegister",mmi->getRegisters("jtag")));

  configData.iobRegisterData = getRegisterConfig("IOBRegister",mmi->getRegisters("iob"));

  configData.epcRegisterData = getRegisterConfig("EPCRegister",mmi->getRegisters("epc"));

  if(mmi->getSequencer()->configGood){
    configData.sequencerData = mmi->getSequencer()->getSequencerParameterMap();
  }

  return configData;
}
#endif
