

#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <bitset>

#include "DSSC_PPT.h"
#include "DSSC_PPT_FTP.h"
#include "SC.h"
#include "utils.h"
#include "DsscModuleInfo.h"
#include <cmath>

#define TESTSYSTEM

#define ERROR_OUT(message)  \
{       utils::CoutColorKeeper keeper(utils::STDRED); \
        errorString = message;  \
        debugOut << errorString << "\n"; \
        errorMessages.push_back(errorString); }

#define DEBUG_OUT(message) \
{       utils::CoutColorKeeper keeper(utils::STDGRAY); \
        debugOut << message << "\n"; \
        cout << "DEBUG: " << message << endl;  }

#define STATUS_OUT(message) \
{       utils::CoutColorKeeper keeper(utils::STDGREEN); \
        debugOut << message << "\n"; \
        cout << "Info: " << message << endl;  }

#define WARN_OUT(message) \
{       utils::CoutColorKeeper keeper(utils::STDBROWN); \
        debugOut << message << "\n"; \
        cout <<"WARNING: " << message << endl;  }

#define ERROR(message) \
{       utils::CoutColorKeeper keeper(utils::STDRED); \
        debugOut << message << "\n"; \
        cout << "ERROR: " << message << endl;  }

#define SENDDIRECT

//#define DEBUGPPT

using namespace SuS;
using namespace std;

#define PPTv2FIRST 10939
#define PPTv1FIRST 9771


//Compatible Configuration File revisions
int DSSC_PPT::firstFirmware = PPTv1FIRST;
int DSSC_PPT::firstLinux    = 10780;
int DSSC_PPT::firstIOB      = 10502;
int DSSC_PPT::pptRev        = 0;


//const uint32_t DSSC_PPT::LMK_RES_0 = 0x80000000;   // in REG0 bit 31 is the reset bit is programmed automatically by lmk module
const uint32_t DSSC_PPT::LMK_REG9  = 0x00022A09;   // 0x00032A09 would be Vboost enabled
const uint32_t DSSC_PPT::LMK_REG14N= 0x6800000E;   // Reg14 in normal Mode Global Clock enable = 1  PowerDown = 0
const uint32_t DSSC_PPT::LMK_REG14R= 0x6400000E;   // Reg14 in reset  Mode Global Clock enable = 0  PowerDown = 1
//const uint32_t DSSC_PPT::LMK_REG14D= 0x6000000E;   // Reg14 in diable Mode Global Clock enable = 0  PowerDown = 0

const std::vector<uint32_t> LMKSettings::clk_num  {7,6,5,4,3,2,1,0,0,1,2,3,4,5,6,7};
const std::vector<uint32_t> LMKSettings::lmk_fast {1,1,1,1,8,8,8,8,1,1,1,1,8,8,8,8};
const std::vector<uint32_t> LMKSettings::lmk_num  {2,2,2,2,4,4,4,4,2,2,2,2,4,4,4,4};

DSSC_PPT::SETUPCONFIG DSSC_PPT::actSetup = MANNHEIM;

DSSC_PPT::DSSC_PPT(PPTFullConfig * fullConfig) :
  SuS::MultiModuleInterface(fullConfig),
  m_opened(false),
  pptTcpAddress("192.168.0.116"),tcpPort(2384),ftpPort(21),
  iobsNotChecked(true),
  simulationMode(false),
  simFileName("simulationFile.txt"),
  m_standAlone(true),
  lmkFastSettings(true),
  lmkSlowSettings(false)
{
  if(pptFullConfig->isGood()){
    if(sequencer->getHoldEnabled()){
      sequencer->setHoldEnabled(false);
      WARN_OUT("WARNING DSSC_PPT: Sequencer Hold disabled");
    }
  }else{
    ERROR_OUT("ERROR Full Config could not be loaded");
  }

  fastInitconfigSpeed = 14;
  initDist = 120;  // if all supplys are switched on init distance must be larger!?
                  // 7.6.2016

  errorString = "";
  m_opened = false;

  if(pptFullConfig->isGood())
    DEBUG_OUT("++++ DSSC_PPT initialized! ++++");
}


DSSC_PPT::~DSSC_PPT()
{
  closeConnection();

  DEBUG_OUT("Close DSSC_PPT");
  debugOut.close();
}


void DSSC_PPT::setPPTAddress(const string& host, unsigned short int port)
{
  pptTcpAddress = host;
  tcpPort = port;
}


int DSSC_PPT::openConnection()
{
  int laenge, ergebnis = -1;
  struct sockaddr_in adresse;

  const char *server_ip = pptTcpAddress.c_str();
  socket_nummer = socket(AF_INET, SOCK_STREAM, 0);

  // set timeout
  struct timeval tv;
  tv.tv_sec  = 5; // 5 seconds timeout to connect
  tv.tv_usec = 0;

  if (setsockopt(socket_nummer, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,  sizeof tv) == -1) {
    perror("SetSocktOpt SND TIMEOUT error");
    exit(0);
  }

  adresse.sin_family = AF_INET;
  inet_pton(AF_INET,server_ip,&adresse.sin_addr.s_addr);
  adresse.sin_port = htons(tcpPort);
  laenge = sizeof(adresse);

  DEBUG_OUT("Verbindung zu IP " << server_ip  << " port " << tcpPort);

  ergebnis = connect(socket_nummer,(struct sockaddr *)&adresse, laenge);
  if(ergebnis == -1){
    ERROR_OUT("Could not connect to ppt");
    pptTcpAddress = "";
    return DSSC_PPT::ERROR_PPT_CONNECT_FAILED;
  }

  m_opened = true;
  return DSSC_PPT::ERROR_OK;
}


int DSSC_PPT::closeConnection()
{
  DEBUG_OUT("INFO DSSC_PPT: Close PPT connection");

  close(socket_nummer);
  m_opened = false;

  DEBUG_OUT("INFO DSSC_PPT: Connection to PPT closed");

  return DSSC_PPT::ERROR_OK;
}


int DSSC_PPT::resetConnection()
{
  closeConnection();
  sleep(1);
  return openConnection();
}


std::string DSSC_PPT::getConnectedETHChannels()
{
  readBackEPCRegister("Ethernet_ReadbackRegister1");
  readBackEPCRegister("Ethernet_ReadbackRegister2");

  std::vector<int> connectedChannels;
  if(getEPCParam("Ethernet_ReadbackRegister1","0","Chan0 PCS_Block_Lock") != 0){
    connectedChannels.push_back(1);
  }
  if(getEPCParam("Ethernet_ReadbackRegister1","0","Chan1 PCS_Block_Lock") != 0){
    connectedChannels.push_back(2);
  }
  if(getEPCParam("Ethernet_ReadbackRegister1","0","Chan2 PCS_Block_Lock") != 0){
    connectedChannels.push_back(3);
  }
  if(getEPCParam("Ethernet_ReadbackRegister2","0","Chan3 PCS_Block_Lock") != 0){
    connectedChannels.push_back(4);
  }
  if(connectedChannels.size() == 0){
    ERROR_OUT("No Ethernet channels connected");
  }
  return utils::positionVectorToList(connectedChannels);
}


void DSSC_PPT::initAsicReadout()
{
  programLMK();
  programJtag();
  enableDataReadout();
}


int DSSC_PPT::initSingleModule(int currentModule)
{
  setActiveModule(currentModule);

  resetCurrentModule(true);
  usleep(80000);
  resetCurrentModule(false);

  int rc = initCurrentIOB();
  if(rc != ERROR_OK){
    return rc;
  }

  if(!initModuleASICs()){
    return DSSC_PPT::ERROR_ASIC_INIT_ERROR;
  }

  return DSSC_PPT::ERROR_OK;
}


int DSSC_PPT::initSystem()
{
  resetAll(true);
  usleep(80000);
  resetAll(false);

  if(!checkPPTPllLocked()){
    return ERROR_PLL_NOT_LOCKED;
  }

  programEPCRegisters();

  int ret = DSSC_PPT::ERROR_OK;
  for(auto && iobNumber : activeIOBs){
    ret += initSingleModule(iobNumber);
  }

  resetDataFailRegister();

  return ret;
}


bool DSSC_PPT::checkCorrectASICResetCurrentModule()
{
  int cnt = 0;
  int good = 0;
  int TO = 5;
  bool allOk;
  uint16_t asicChannelFailed;

  do{
    usleep(150000);

    asicChannelFailed = getActiveChannelFailure();

    allOk = true;
    if(asicChannelFailed == 0){
      good++;
      continue;
    }
    good = 0;

    for(int channel = 0; channel<16; channel++){
      if(asicChannelFailed & (1<<channel))
      {
        allOk = false;
        int asic = asicDONumber[channel];
        programLMKASIC(asic,false,false);
        programLMKASIC(asic,true,false);
      }
    }

    resetDataFailRegister();

    cnt++;

  }while(!allOk && cnt<TO && good < 10);

  if(!allOk){
    ERROR("+++Error: Module " << currentModule << " : not all ASICs are sending data:");
    for(int channel = 0; channel<16; channel++){
      if(asicChannelFailed & (1<<channel)){
        int asic = asicDONumber[channel];
        WARN_OUT("+++Warning: Module " << currentModule << " - ASIC " << asic << " could not correctly be initialized");
      }
    }
  }
  return allOk;
}


bool DSSC_PPT::checkCorrectASICReset()
{
  updateExpectedTestPattern(getExpectedTestPattern());

  int TO = 5;

  uint16_t asicChannelFailed;
  bool allOk = true;


  int currentModRem = currentModule;

  CheckSendingKeeper keeper(this);

  for(const auto & module : activeIOBs)
  {
    int cnt = 0;
    int good = 0;

    currentModule = module;

    do{
      usleep(150000);

      asicChannelFailed = getActiveChannelFailure();

      allOk = true;
      if(asicChannelFailed == 0){
        good++;
        continue;
      }
      good = 0;

      for(int channel = 0; channel<16; channel++){
        if(asicChannelFailed & (1<<channel)){
          allOk = false;
          int asic = asicDONumber[channel];
          programLMKASIC(asic,false,false);
          programLMKASIC(asic,true,false);
        }
      }

      resetDataFailRegister();

      cnt++;

    }while(!allOk && cnt<TO && good < 10);

    if(!allOk){
      ERROR("+++Error: Module " << currentModule << " : not all ASICs are sending data:");
      for(int channel = 0; channel<16; channel++){
        if(asicChannelFailed & (1<<channel)){
          int asic = asicDONumber[channel];
          WARN_OUT("+++Warning: Module " << currentModule << " - ASIC " << asic << " could not correctly be initialized");
        }
      }
    }
  }

  currentModule = currentModRem;
  return allOk;
}


uint16_t DSSC_PPT::getActiveChannelFailure()
{
  std::string modSet = "Data_Receive_Status_";
  if(currentModule>2){
    modSet += "1";
  }else{
    modSet += "0";
  }
  readBackEPCRegister(modSet);

  std::string signal = "Failed_Asics_Module_"+to_string(currentModule);
  uint16_t sendingASICDOs = getAsicDOs(sendingAsics);
  uint16_t failed = (getEPCParam(modSet,"0",signal)) & sendingASICDOs;
  if(failed){
    DEBUG_OUT("active asics = " << bitset<16>(sendingASICDOs));
    DEBUG_OUT("failed asics = " << bitset<16>(failed));
  }

  return failed;
}


void DSSC_PPT::resetDataFailRegister()
{
  setEPCParam("DataRecv_to_Eth0_Register","all","reset_fail_data",1);
  programEPCRegister("DataRecv_to_Eth0_Register");
  usleep(200000);
  setEPCParam("DataRecv_to_Eth0_Register","all","reset_fail_data",0);
  programEPCRegister("DataRecv_to_Eth0_Register");
}


int DSSC_PPT::initCurrentIOB()
{
  if(isIOBAvailable(currentModule)!=ERROR_OK){
    return ERROR_IOB_NOT_FOUND;
  }

  if(!checkSingleAuroraReady()){
    return ERROR_AURORA_NOT_LOCKED;
  }

  iobRegisters->setSignalValue("ASIC_readout_enable",to_string(currentModule),"ASIC_readout_enable",1);
  iobRegisters->setSignalValue("ASIC_reset",to_string(currentModule),"ASIC_reset",0);
  iobRegisters->setSignalValue("ASIC_send_dummy_data",to_string(currentModule),"ASIC_send_dummy_data",0);

  bool ok = programIOBRegisters();
  return ok? ERROR_OK : ERROR_PARAM_READBACK;
}


int DSSC_PPT::initIOBs()
{
  int numIOBsFound = numAvailableIOBs();
  if(numIOBsFound==0){
    return ERROR_IOB_NOT_FOUND;
  }

  if(!checkAuroraReady()){
    return ERROR_AURORA_NOT_LOCKED;
  }

  iobRegisters->setSignalValue("ASIC_readout_enable","all","ASIC_readout_enable",1);
  iobRegisters->setSignalValue("ASIC_reset","all","ASIC_reset",0);
  iobRegisters->setSignalValue("ASIC_send_dummy_data","all","ASIC_send_dummy_data",0);

  return programQuadIOBRegisters();
}


void DSSC_PPT::initChip()
{
  sequencer->compileAndCheckAllTracks();

  updateNumberOfModulesInRegisters();

  programmFullASICConfig(false);
}


int DSSC_PPT::initModuleASICs()
{
  int ret = ERROR_OK;

  //disableLMKs();
  programLMKGlobal(true);

  ret |= fastASICInitTestSystem();
  setASICReset(true);

  enableJTAGEngines(true);

  sequencer->compileAllTracks();

  ret |= programmASICConfig(false);

  setASICReset(false);

  programLMKGlobal(false);

  enableLMK();
  programLMK();

  ret |= checkASICPllLocked();

  return ret;
}


int DSSC_PPT::initASICs()
{
  int ret = ERROR_OK;

  //disableLMKs();
  programLMKGlobal(true);

  for(const auto & module :activeIOBs){
    setActiveModule(module);
    ret |= fastASICInitTestSystem();
    setASICReset(true);
  }

  enableJTAGEngines(true);

  ret |= programmFullASICConfig(false);

  for(const auto & module :activeIOBs){
    setActiveModule(module);
    setASICReset(false);
  }

  programLMKGlobal(false);
  enableLMKs();

  ret |= checkAllASICPllLocked();

  return ret;
}


int DSSC_PPT::fastASICInitTestSystem()
{
  enableJTAGEngines(false);

  return programFastInitConfig();
}


int DSSC_PPT::programFastInitConfig()
{
  DEBUG_OUT("Fast Init Module " << currentModule);
  DEBUG_OUT("Programming fast Init with clock divider " << dec << fastInitconfigSpeed);

  JTAGClockDivKeeper keeper(this,fastInitconfigSpeed);

  initConfigVector();

  appendJtagReset();

  setPixelRegsInChain(false);

  addSingleJTAGRegistersToConfigVector("JTAG SRAM Control");
  addSingleJTAGRegistersToConfigVector("Global Control Register");
  addDefaultPixelRegisterToConfigVector("Control register");
  addSingleJTAGRegistersToConfigVector("Global FCSR 0");
  addSingleJTAGRegistersToConfigVector("Global FCSR 1");

  bool ok = sendConfigurationVector(configVector,NoRegs);
  if(!ok){
    ERROR_OUT("Fast Init Error: could not send data vector");
    return ERROR_TCP_SEND_CMD_FAILED;
  }

  ok = doFastInit();

  if(!ok){
    ERROR_OUT("Fast Init Error: Fast Init Sequence went wrong, shut down power!");
    iobRegisters->setSignalValue("PRB_control",to_string(currentModule),"PRB_power_off",1);
    programIOBRegister("PRB_control");
    return ERROR_FAST_INIT_FAILED;
  }

  iobRegisters->setSignalValue("PRB_control",to_string(currentModule),"PRB_power_off",0);
  readBackIOBSingleReg("PRB_control");

  return ERROR_OK;
}


bool DSSC_PPT::isXFELMode()
{
  int value = m_standAlone? 0 : 1;
  epcRegisters->setSignalValue("Multi_purpose_Register","0","CmdProtocolEngine_enable",value);
  return !m_standAlone;
}


int DSSC_PPT::start()
{
    m_standAlone = false;
    runContinuousMode(true);
    disableSending(false);

    int rc = DSSC_PPT::ERROR_OK;
    return rc;
}


int DSSC_PPT::stop()
{
    runContinuousMode(false);
    disableSending(true);

    int rc = DSSC_PPT::ERROR_OK;
    return rc;
}


void DSSC_PPT::appendJtagReset()
{
  vector<uint32_t> resData(16,0xff);
  configVector.insert(configVector.end(),resData.begin(),resData.end());
}


void DSSC_PPT::appendAndSendJtagReg(vector<uint32_t> &data, uint8_t jtagInstr,const vector<bool> &bits, bool readBack)
{
  const int numChipsJtagChain = getNumberOfActiveAsics();

  { // fill jtag instr register
    vector<bool> intrBits;
    for (int i=0; i<numChipsJtagChain; ++i) {
      for(int bit=0; bit<c_jtagInstrLength; bit++){
        intrBits.push_back((jtagInstr&(1<<bit))!=0);
      }
    }
    // jtag header:
    // jtagEngInstr, 1byte -> lock /unlock, enable readBack
    // 2 bytes instruction (bit) length
    // instr bit length % 8 + 1 bytes instruction bytes (concatenate instructions to a bit string)
    // 2 bytes data (bit) length
    // data bit length % 8 + 1 bytes data bytes
    data.push_back(readBack ? c_jtagInstrEnReadback : 0x00);
    data.push_back(numChipsJtagChain*c_jtagInstrLength-1);
    data.push_back(0);
    utils::appendBitVectorToByteVector(data,intrBits);
  }

  { // fill data register
    int bitStreamLengthToSend = bits.size() - 1;
    data.push_back((uint8_t)bitStreamLengthToSend);
    data.push_back((uint8_t)(bitStreamLengthToSend >> 8));

    utils::appendBitVectorToByteVector(data,bits);
  }
#ifdef SENDDIRECT
  sendConfigurationDirectly(data);
#endif

}


void DSSC_PPT::appendAndSendJtagReg(vector<uint32_t> &data, int asic, uint8_t jtagInstr,const vector<bool> &bits, bool readBack)
{
  const int numChipsJtagChain = getNumberOfActiveAsics();

  int asicIdx = (asic>7)? asic : 7-asic;
  { // fill jtag instr register
    vector<bool> intrBits;
    for (int i=0; i<numChipsJtagChain; ++i) {
      if(asicIdx == i){
        for(int bit=0; bit<c_jtagInstrLength; bit++){
          intrBits.push_back((jtagInstr&(1<<bit))!=0);
        }
      }else{
        for(int bit=0; bit<c_jtagInstrLength; bit++){
          intrBits.push_back((c_jtagInstrBypass&(1<<bit))!=0);
        }
      }
    }

    // jtag header:
    // jtagEngInstr, 1byte -> lock /unlock, enable readBack
    // 2 bytes instruction (bit) length
    // instr bit length % 8 + 1 bytes instruction bytes (concatenate instructions to a bit string)
    // 2 bytes data (bit) length
    // data bit length % 8 + 1 bytes data bytes
    data.push_back(readBack ? c_jtagInstrEnReadback : 0x00);
    data.push_back(numChipsJtagChain*c_jtagInstrLength-1);
    data.push_back(0);
    utils::appendBitVectorToByteVector(data,intrBits);
  }

  { // fill data register
    int bitStreamLengthToSend = bits.size() - 1;
    data.push_back((uint8_t)bitStreamLengthToSend);
    data.push_back((uint8_t)(bitStreamLengthToSend >> 8));

    utils::appendBitVectorToByteVector(data,bits);
  }

//  can be enabled to save input jtag data
//  static int fileCnt = 0;
//  auto dataToPrint = data;
//  printVectorToConfigFile(dataToPrint,"JtagCommandBits"+to_string(fileCnt++)+".txt");

#ifdef SENDDIRECT
  sendConfigurationDirectly(data);
#endif
}


bool DSSC_PPT::setEthernetOutputDatarate(double megabit)
{
  static constexpr double minDuration = 256/156.25E6;
  double aimDuration = 4096*8/megabit/1E6;

  double numFrames = getNumFramesToSend();
  double requiredRate = 1.1*numFrames*64*64*16*10/1E6;
  if(megabit < requiredRate){
    ERROR_OUT("ERROR: Output datarate can not set to "+to_string(megabit)+" Megabit. Min "+to_string(requiredRate)+" MBit required for sending " + to_string(numFrames) + " frames");
    return false;
  }

  int ddr3_throttle_divider = ceil((aimDuration-minDuration)*200E6);
  setEPCParam("Multi_purpose_Register","all","ddr throttle divider",ddr3_throttle_divider);
  programEPCRegister("Multi_purpose_Register");
  DEBUG_OUT("PPT Output Datarate set to " << megabit << " Megabit/s");
  return true;
}


void DSSC_PPT::setEthernetOutputThrottleDivider(uint _ddr3_throttle_divider)
{
  setEPCParam("Multi_purpose_Register","all","ddr throttle divider",_ddr3_throttle_divider);
  programEPCRegister("Multi_purpose_Register");
  return;
}


uint DSSC_PPT::getEthernetOutputThrottleDivider()
{
  uint32_t ddr3_throttle_divider;
  readBackEPCRegister("Multi_purpose_Register");
  ddr3_throttle_divider = getEPCParam("Multi_purpose_Register","all","ddr throttle divider");
  return ddr3_throttle_divider;
}


void DSSC_PPT::setNumberOfActiveAsics(int numAsics)
{
  SuS::MultiModuleInterface::setNumberOfActiveAsics(numAsics);

  vector<int> activePowers;
  activePowers.insert(activePowers.begin(),prbPowerInJTAGChain.begin(),prbPowerInJTAGChain.begin()+numAsics);
  for(const auto & module : activeIOBs){
    setActiveModule(module);
    setPRBPowerSelect(utils::positionVectorToList(activePowers),true);
  }
}


uint DSSC_PPT::numAvailableIOBs()
{
  return getAvailableIOBoards().size();
}


vector<int> DSSC_PPT::getAvailableIOBoards()
{
  if(iobsNotChecked){

    activeIOBs.clear();

    {
      vector<int> foundIOBs;
      for(int i=1; i<5; i++){
        if(isIOBAvailable(i)){
          foundIOBs.push_back(i);
        }
      }
      activeIOBs = foundIOBs;
    }

    for(const auto & module : activeIOBs){
      currentModule = module;
      DEBUG_OUT("Programming PRB_en_static select of module " << currentModule << " to " << getIOBParam("PRB_en_static_sel",to_string(currentModule),"PRB_en_static_sel"));
      //init regulator switched powers correctly
      programIOBRegister("PRB_en_static_sel");
      programIOBRegister("PRB_en_switched_sel");
    }

    if(activeIOBs.size() > 0){
      iobsNotChecked = false;
      currentModule = activeIOBs.front();
    }else{
      iobsNotChecked = true;
      currentModule = 1;
    }

    STATUS_OUT("Found " << activeIOBs.size() << " IOBs set " << currentModule << " active");
  }

  return activeIOBs;  // return 1-4 not 0-3
}


bool DSSC_PPT::isIOBAvailable(int iobNumber)
{
  if(simulationMode) return true;

  currentModule = iobNumber;
  if(iobsNotChecked){
    return (checkIOBAvailable() != 0);
  }

  for(uint i=0; i<activeIOBs.size(); i++){
    if(iobNumber == activeIOBs.at(i)){
        return true;
    }
  }
  return false;
}


int DSSC_PPT::checkIOBAvailable()
{
  uint32_t value = iobControl_read(c_IOBoardConfRegAddr_board_type);

  bool iob = ((value & 0xFFFFFF00) == 0x494F4200); //IOB in ASCII encoding
  int iob_rev = (value & 0x000000FF);

  if (!iob){
    ERROR("++++ DSSC_PPT: IOB " << currentModule << " not found!");
    return 0;
  }else{
    STATUS_OUT("++++ DSSC_PPT: IOB " << currentModule << " found!");
    return iob_rev;
  }
}


int DSSC_PPT::readFPGATemperature()
{
  dataString = "TEMP";
  return sendReadPacket();
}


//In the Latter: IOB Temperature will be converted by Safety Interlock board and then read by PPT... not implemented yet
double DSSC_PPT::readIOBTemperature_TestSystem()
{
  if(actSetup!=MANNHEIM){
    return 0.0;
  }

  readBackEPCRegister("IOBTempReaderOut");
  uint32_t iob_temp_out = (uint32_t)epcRegisters->getSignalValue("IOBTempReaderOut","0","IOB Temperature");
  if(iob_temp_out == 0){
    return 0.0;
  }

  uint16_t t_high = ((iob_temp_out & 0xFFFF0000) >> 16);
  double t_high_f = (double)t_high;
  uint16_t t_low = iob_temp_out & 0x0000FFFF;
  double t_low_f = (double)t_low;
  double iob_temp_converted = -200*(0.85-t_low_f/t_high_f)*(0.85-t_low_f/t_high_f)*(0.85-t_low_f/t_high_f)+(425*t_low_f/t_high_f)-273;
  DEBUG_OUT("IOB 1 Temperature " << iob_temp_converted << "  " << iob_temp_out);
  return iob_temp_converted;
}



uint32_t DSSC_PPT::readBuildDate()
{
  readBackEPCRegister("Build_Date");
  return epcRegisters->getSignalValue("Build_Date","0","Build_Date");
}


uint32_t DSSC_PPT::readBuildTimeRev()
{
  readBackEPCRegister("Build_Info");
  uint32_t time_rev = epcRegisters->getSignalValue("Build_Info","0","Build_Rev");
  time_rev |= epcRegisters->getSignalValue("Build_Info","0","Build_Time") << 16;
  return time_rev;
}


uint64_t DSSC_PPT::readSerialNumber()
{
  dataString = "SERN";
  uint64_t serial =  sendReadULPacket();

  if(serial > 0x18000000){
    firstFirmware = PPTv2FIRST;
    STATUS_OUT("++++ PPTv2 found!");
    pptRev = 2;
  }else{
    firstFirmware = PPTv1FIRST;
    STATUS_OUT("++++ PPTv1 found!");
    pptRev = 1;
  }
  return serial;
}


bool DSSC_PPT::checkPPTRevision(const std::string &fileName)
{
  string infoFileName = fileName.substr(0,fileName.length()-3) + "txt";

  int build;
  int givenRev = readRevFromFile(infoFileName,build);

  bool ok = (givenRev == pptRev);
  if(!ok){
    if(givenRev>0){
      ERROR_OUT("ERROR: Firmare for PPTRev"+to_string(givenRev)+" selected. PPTRev"+to_string(pptRev)+" is required");
    }else{
      // there is already an error message: file not found or PPTRev information not set
    }
  }else{
    if(build<firstFirmware){
      ERROR_OUT("ERROR: Firmare for PPT with svn build_rev "+to_string(build)+" is outdated."
                " Use build_rev " + to_string(firstFirmware));
      ok = false;
    }
  }
  return ok;
}


bool DSSC_PPT::checkLinuxRevision(const std::string &fileName)
{
  string infoFileName = fileName.substr(0,fileName.length()-3) + "txt";

  int build;
  readRevFromFile(infoFileName,build);

  bool ok = (build>=firstLinux);

  if(!ok && build>0){
    ERROR_OUT("ERROR: Linux for PPT with svn build_rev "+to_string(build)+" is outdated."
              " Use build_rev " + to_string(firstLinux));
    ok = false;
  }

  return ok;
}


bool DSSC_PPT::checkIOBRevision(const std::string &fileName)
{
  string infoFileName = fileName.substr(0,fileName.length()-4) + "txt";

  int build;
  readRevFromFile(infoFileName,build);

  bool ok = (build>=firstIOB);

  if(!ok && build>0){
    ERROR_OUT("ERROR: IOB Firmware with svn build_rev "+to_string(build)+" is outdated."
              " Use build_rev " + to_string(firstIOB));
  }

  return ok;
}


void DSSC_PPT::setUARTBaudRate(const std::string &baudrate)
{
  //DEBUG_OUT("PPT UART SetBaudrate to " << baudrate);

  int baud = stoi(baudrate);

  int clk_divide = 100e6/4/baud;
  epcRegisters->setSignalValue("UART_Control_Register","0","uart_clock_divide",clk_divide);
  programEPCRegister("UART_Control_Register");
}


void DSSC_PPT::enUARTTestPins(bool enable)
{
  int value = (enable)? 1 : 0;
  epcRegisters->setSignalValue("UART_Control_Register","0","UART_test_en",value);
  programEPCRegister("UART_Control_Register");
}


void DSSC_PPT::setSIBLoopBackMode(bool enable)
{
  int value = (enable)? 1 : 0;
  epcRegisters->setSignalValue("UART_Control_Register","0","loopback",value);
  epcRegisters->setSignalValue("UART_Control_Register","0","manual_mode",0);
  programEPCRegister("UART_Control_Register");
}


int DSSC_PPT::SIBRead()
{
  dataString = "SIBC R";
  return  sendReadPacket();
}


void DSSC_PPT::SIBWrite(int data)
{
  dataString = "SIBC W " + to_string(data);
  sendReadPacket(true);
}


int DSSC_PPT::SIBFlush()
{
  dataString = "SIBC F";
  return  sendReadPacket(true);
}


void DSSC_PPT::flushSIBFifo()
{
  dataString = "SIBC F";
  uint32_t flushed =  sendReadPacket();

  DEBUG_OUT("Flushed " << flushed << " words from SIB FIFO");
}


void DSSC_PPT::SIBSendManualTemperatures()
{
  setEPCParam("UART_Control_Register","0","manual_mode",1);
  programEPCRegister("UART_Control_Register");

  SIBWrite(21);
  DEBUG_OUT("SIB Ack = 0x" << hex << SIBRead() << dec);
  int wordCnt = 0;
  for(int i=0; i<128; i++){
    SIBWrite(i);
    wordCnt++;
    if((wordCnt%14) == 0){
      DEBUG_OUT("SIB Ack = 0x" << hex << SIBRead() << dec);
    }
  }

  SIBWrite(c_SIB_SEND_DONE_CMD);

  vector<uint8_t> status;
  getSIBStatus(status);

  cout << "GOT Status: ";
  for(const auto & word : status){
    cout << word << " ";
  }
  cout << endl;

  setEPCParam("UART_Control_Register","0","manual_mode",0);
  programEPCRegister("UART_Control_Register");
}


std::vector<uint32_t> DSSC_PPT::readSIBReceiveRegister()
{
  dataString = "SIBR";
  std::vector<uint32_t> sibRegister = sendReadVectorPacket();

  if((sibRegister.back() & 0x4700ff00) != 0x4700ff00){
    ERROR_OUT("SIB Register readour error");
  }else if((sibRegister.back() & 0x0f) == 0x0f){
    ERROR_OUT("SIB timed out during last burst");
  }

  return sibRegister;
}

void DSSC_PPT::getSIBStatus(std::vector<uint8_t> &status)
{
  SIBWrite(c_SIB_READ_STAT_CMD);

  usleep(250000);
  status.clear();
  for(int i=0; i<65; i++){
    status.push_back(SIBRead());
  }
  //TODO: remove pause and flushSIBFifo, when number of send byte is correct as e
  // usleep(30000);
}


int DSSC_PPT::readRevFromFile(const std::string &fileName, int &build)
{
  int ppt_rev = -1;
  build       = -1;
  ifstream infoFile(fileName.c_str(), ifstream::in);
  if(!infoFile.is_open()){
    ERROR_OUT("ERROR Update Firmware Flash: Can't update flash without revision information.\n Must provide info file for update. "+ fileName + " not found!");
    return ppt_rev;
  }

  string line;
  while (getline(infoFile, line) && (ppt_rev == -1))
  {
    if(line.length()>0){
      stringstream iss(line);
      string info;
      iss >> info; // error
      if(info.compare("ppt_rev") == 0){
        iss >> info;
        iss >> info;

        if(info.compare("x")==0){
          ERROR_OUT("ERROR Update Firmware Flash: must provide valid revision information!");
          infoFile.close();
          return ppt_rev;
        }
        ppt_rev = stoi(info);
        DEBUG_OUT("PPT Firmware file is for PPTRev"<<ppt_rev);
      }else if(info.compare("build_rev")==0){
        iss >> info;
        iss >> build;
      }
    }
  }

  infoFile.close();
  return ppt_rev;
}


bool DSSC_PPT::isPRBSelfDetected()
{
  return iobRegisters->getSignalValue("PRB_control",to_string(currentModule),"PRB_self_detect") == 1;
}


string DSSC_PPT::readIOBBuildStamp()
{
  readBackIOBSingleReg("Build_Date");
  readBackIOBSingleReg("Build_Info");

  int buildDate = iobRegisters -> getSignalValue("Build_Date",to_string(currentModule),"Build_Date");
  string builddate = to_string(buildDate);
  builddate.insert(6,"-");
  builddate.insert(4,"-");

  int buildTime = iobRegisters -> getSignalValue("Build_Info",to_string(currentModule),"Build_Time");
  int buildRev  = iobRegisters -> getSignalValue("Build_Info",to_string(currentModule),"Build_Rev");
  string buildtime = to_string(buildTime);
  if (buildtime.size()==3){
    buildtime.insert(1,":");
  }else{
    buildtime.insert(2,":");
  }

  string buildStr = "Build: " + builddate +
  " - "     + buildtime +
  " - "     + to_string(buildRev);

  if(buildRev < firstIOB){
    buildStr += " - not compatible update to rev " + to_string(firstIOB);
    ERROR_OUT("PPT Firmware ERROR: Build revision not compatible! Update to revision " + to_string(firstIOB));
  }

  return buildStr;
}


string DSSC_PPT::readBuildStamp()
{
  string builddate = to_string(readBuildDate());
  builddate.insert(6,"-");
  builddate.insert(4,"-");

  int buildtime_rev = readBuildTimeRev();
  string buildtime = to_string((buildtime_rev & 0xFFFF0000) >> 16);
  if (buildtime.size()==3){
    buildtime.insert(1,":");
  }else{
    buildtime.insert(2,":");
  }

  int build_rev = buildtime_rev & 0x0000FFFF;
  string buildStr = builddate +
        " - "     + buildtime +
        " - "     + to_string(build_rev);


  if(build_rev < firstFirmware){
    buildStr = "Error "+buildStr+" - not compatible! Update to build " + to_string(firstFirmware);
    ERROR_OUT("PPT Firmware ERROR: Build revision not compatible! Update to revision " + to_string(firstFirmware));
  }

  return buildStr;
}


string DSSC_PPT::readLinuxBuildStamp()
{
  dataString = "BILT 1";
  int buildDate = sendReadPacket();
  string builddate = to_string(buildDate);
  builddate.insert(6,"-");
  builddate.insert(4,"-");

  dataString = "BILT 2";
  int buildTime = sendReadPacket();
  string buildtime = to_string(buildTime);
  if (buildtime.size()==3){
    buildtime.insert(1,":");
  }else{
    buildtime.insert(2,":");
  }

  dataString = "BILT 3";
  int buildRev = sendReadPacket();

  string linuxRev = builddate +
        " - "     + buildtime +
        " - "     + to_string(buildRev);

  if(buildRev < firstLinux){
    linuxRev += " - not compatible update to rev " + to_string(firstLinux);
    ERROR_OUT("PPT Linux ERROR: Build revision not compatible! Update to revision " + to_string(firstLinux));
  }

  return linuxRev;
}


void DSSC_PPT::initEPCRegsAfterReset()
{
  epcRegisters->setSignalValue("JTAG_Control_Register","all","EnJTAG"+ to_string(currentModule),0);
  epcRegisters->setSignalValue("JTAG_Control_Register","all","RESET_ASIC1_N",0);

  if(epcRegisters->getSignalValue("CMD_PROT_ENGINE","0","num_pre_burst_wait_vetos") == 0){
    epcRegisters->setSignalValue("CMD_PROT_ENGINE","all","num_pre_burst_wait_vetos",1);
  }
}

void DSSC_PPT::initIOBRegsAfterReset()
{
  iobRegisters->setSignalValue("PRB_control","all","PRB_en",1);
  iobRegisters->setSignalValue("PRB_control","all","PRB_power_off",1);
  iobRegisters->setSignalValue("PRB_control","all","PRB_num_prbs_found",0);
  iobRegisters->setSignalValue("PRB_control","all","PRB_manual_ctrl_en",0);

  iobRegisters->setSignalValue("LMK_control","all","LMK_en",0);
  iobRegisters->setSignalValue("LMK_control","all","LMK_valid",0);
  iobRegisters->setSignalValue("LMK_control","all","LMK_dev_sel",0);
  iobRegisters->setSignalValue("LMK_data"   ,"all","LMK_data",0);

  iobRegisters->setSignalValue("ASIC_reset","all","ASIC_reset",1);

  iobRegisters->setSignalValue("PRB_control_sw_supplies_always_on","all","PRB_control_sw_supplies_always_on",0);
}

void DSSC_PPT::initModuleIOBRegsAfterReset()
{
  const std::string currentModStr = to_string(currentModule);

  iobRegisters->setSignalValue("PRB_control",currentModStr,"PRB_en",1);
  iobRegisters->setSignalValue("PRB_control",currentModStr,"PRB_power_off",1);
  iobRegisters->setSignalValue("PRB_control",currentModStr,"PRB_num_prbs_found",0);
  iobRegisters->setSignalValue("PRB_control",currentModStr,"PRB_manual_ctrl_en",0);

  iobRegisters->setSignalValue("LMK_control",currentModStr,"LMK_en",0);
  iobRegisters->setSignalValue("LMK_control",currentModStr,"LMK_valid",0);
  iobRegisters->setSignalValue("LMK_control",currentModStr,"LMK_dev_sel",0);
  iobRegisters->setSignalValue("LMK_data"   ,currentModStr,"LMK_data",0);

  iobRegisters->setSignalValue("ASIC_reset",currentModStr,"ASIC_reset",1);

  iobRegisters->setSignalValue("PRB_control_sw_supplies_always_on",currentModStr,"PRB_control_sw_supplies_always_on",0);
}


void DSSC_PPT::setDebugOutputSelect(int SMA_1_val, int SMA_2_val, bool enableSMA_1, bool enableSMA_2)
{
  epcRegisters->setSignalValue("CMD_PROT_ENGINE","all","sel_dbg_out(bin)",SMA_1_val);
  if(enableSMA_1)
    programEPCRegister("CMD_PROT_ENGINE");
}


void DSSC_PPT::setLinkID(uint32_t id)
{
  string sigName = "linkID"+to_string(currentModule);
  epcRegisters->setSignalValue("LinkID_Config_Register","0",sigName,id);
}


uint64_t DSSC_PPT::getCurrentTrainID()
{
  readBackEPCRegister("Current_Train_Id");
  return getEPCParam("Current_Train_Id","0","Current_Train_Id");
}


bool DSSC_PPT::checkIOBDataFailed()
{
  readBackIOBSingleReg("ASIC_ro_status");
  uint16_t sendingASICDOs = getAsicDOs(sendingAsics);
  uint16_t activeFailed = iobRegisters->getSignalValue("ASIC_ro_status",to_string(currentModule),"ASIC_readout_failure") & sendingASICDOs;
  bool allFailed =(activeFailed == sendingASICDOs);

  if(allFailed){
    ERROR_OUT("++++ DSSC_PPT: All channels failed!");
    if((iobRegisters->getSignalValue("ASIC_ro_status",to_string(currentModule),"ASIC_early_data_error") & sendingASICDOs ) != 0){
      ERROR_OUT("++++ DSSC_PPT: ASIC early data detected. ASIC in Testpattern mode or line constantly high!");
    }
  }else if(activeFailed){
    const auto failedAsics = getFailedAsicNumbers(activeFailed);
    WARN_OUT("Warning : some active asics did not send data: " << utils::positionVectorToList(failedAsics));
  }

  return allFailed;
}

vector<uint32_t> DSSC_PPT::getFailedAsicNumbers(uint16_t value)
{
  vector<uint32_t> numbers;
  for(int i=0; i<16; i++){
    bool set = (value & (1<<i)) != 0;
    if(set){
      numbers.push_back(asicDONumber[i]);
    }
  }
  return numbers;
}


uint32_t DSSC_PPT::getExpectedTestPattern()
{
  return getJTAGParam("Serializer Config Register","0","Serializer Output Test Pattern");
}


void DSSC_PPT::updateExpectedTestPattern(uint16_t testPattern)
{
  setEPCParam("DataRecv_to_Eth0_Register","all","expected_test_pattern",testPattern);
  programEPCRegister("DataRecv_to_Eth0_Register");
}


void DSSC_PPT::startTestPattern()
{
  dataString = "TEST";
  sendReadPacket();
}


void DSSC_PPT::waitSingleCycle()
{
  if(!isSendingDisabled()) return;

  static const uint32_t TO = 1000;
  uint32_t cnt = 0;
  bool done = false;

  do
  {
    usleep(50000);
    readBackEPCRegister("Single_Cycle_Register");
    done = ( getEPCParam("Single_Cycle_Register","0","single_cycle_done") != 0 );

  }while(!done && (cnt++ < TO));

  STATUS_OUT("Single Cycle Done - iterations = " << getEPCParam("Single_Cycle_Register","0","iterations"));
}


bool DSSC_PPT::isSendingDisabled()
{
  return (epcRegisters->getSignalValue("Single_Cycle_Register","0","disable_sending") == 1);
}


void DSSC_PPT::startSingleCycle()
{
  dataString = "SIGL";
  sendReadPacket();
}


void DSSC_PPT::startBurst()
{
  dataString = "BURS";
  sendReadPacket();
}

void DSSC_PPT::startReadout()
{
  dataString = "READ";
  sendReadPacket();
}

void DSSC_PPT::startSingleReadout()
{
  epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","start_readout_from_reg",1);
  programEPCRegister("CMD_PROT_ENGINE");
  epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","start_readout_from_reg",0);
  programEPCRegister("CMD_PROT_ENGINE");
}


uint8_t DSSC_PPT::readSystemStatus()
{
  dataString = "STAT";
  return sendReadPacket() & 0xFF;
}

void DSSC_PPT::enableXFELControl(bool enable)
{
  clockSourceSelect(enable);

  checkPPTPllLocked();

  checkAllASICPllLocked();

  if(enable){
    DEBUG_OUT("DSSC_PPT: Enable XFEL Control");
    m_standAlone = false;
  }else{
    DEBUG_OUT("DSSC_PPT: Enable Manual Control");
    m_standAlone = true;
  }
}


void DSSC_PPT::enableDummyPackets(bool enable)
{
  int value = (enable)? 1 : 0;
  epcRegisters->setSignalValue("DataRecv_to_Eth0_Register","0","send_dummy_packets",value);
  programEPCRegister("DataRecv_to_Eth0_Register");
}


void DSSC_PPT::setSendRawData(bool enable, bool reordered, bool converted)
{
  int raw  = (enable)?    1 : 0;
  int conv = 0;
  int reor = 0;
  if(!enable){
    conv = (converted)? 1 : 0;
    if(!converted){
      reor = (reordered)? 1 : 0;
    }
  }

  epcRegisters->setSignalValue("AuroraRX_Control","0","send_converted_data",conv);
  epcRegisters->setSignalValue("AuroraRX_Control","0","send_reordered_data",reor);
  epcRegisters->setSignalValue("AuroraRX_Control","0","send_raw_data",raw);
  if(isOpen()){
    programEPCRegister("AuroraRX_Control");
  }
}


void DSSC_PPT::enableDummyDRData(bool enable)
{
  int value = (enable)? 1 : 0;
  epcRegisters->setSignalValue("AuroraRX_Control","0","send_dummy_dr_data",value);
  if(enable){
    updateExpectedTestPattern(c_DR_DUMMY_DATA_TESTPATTERN);
  }
  setSendRawData(enable,false,true);
  if(isOpen()){
    programEPCRegister("AuroraRX_Control");
  }
}


void DSSC_PPT::enableDummyAsicData(bool enable)
{
  int value = (enable)? 1 : 0;

  if(enable){
    updateExpectedTestPattern(c_IOB_DUMMY_DATA_TESTPATTERN);
  }

  iobRegisters->setSignalValue("ASIC_send_dummy_data","all","ASIC_send_dummy_data",value);
  iobRegisters->setSignalValue("Aurora_Reset","all","Aurora_Reset",0);
  iobRegisters->setSignalValue("ASIC_reset","all","ASIC_reset",0);

  for(const auto & module : activeIOBs)
  {
    currentModule = module;

    setSendRawData(enable,false,true);
    // set dummy data config bit

    programIOBRegister("ASIC_send_dummy_data");

    // disable IOB resets to make shure that data comes out
    if(value == 1){
      programIOBRegister("Aurora_Reset");
      programIOBRegister("ASIC_reset");

      //checkAuroraReady(); // probably not needed
    }
  }
}


void DSSC_PPT::disableAllDummyData()
{
  ContModeKeeper keeper(this);

  enableDummyDRData(false);
  enableDummyAsicData(false);

  setSendRawData(false,false,true);

  updateExpectedTestPattern(getExpectedTestPattern());

  STATUS_OUT("All dummy data generators disabled");
}


int DSSC_PPT::getDummyDataMode()
{
  if(epcRegisters->getSignalValue("DataRecv_to_Eth0_Register","0","send_dummy_packets")==1){
    return 1;
  }

  if(epcRegisters->getSignalValue("AuroraRX_Control","0","send_dummy_dr_data")==1){
    return 2;
  }

  if(iobRegisters->getSignalValue("ASIC_send_dummy_data",to_string(currentModule),"ASIC_send_dummy_data")==1){
    return 3;
  }

  return 0;
}


bool DSSC_PPT::doFastInit()
{
  dataString = "FINT " + to_string(currentModule) + " " + to_string(initDist);
  return sendReadPacket(true) == 0;
}


bool DSSC_PPT::startJTAGProgramming()
{
  dataString = "JTG1";
  return (sendReadPacket() == 0);
}


bool DSSC_PPT::programEPCRegisters()
{
  initEPCRegsFromJTAGConfigFile();
  bool ok = true;
  const auto moduleSets = epcRegisters->getModuleSetNames();
  for(const auto & set : moduleSets){
    programEPCRegister(set);

    ok &= readBackEPCRegister(set);
  }
  return ok;
}


void DSSC_PPT::programEPCRegister(const string & moduleSet)
{

  if(epcRegisters->isModuleSetReadOnly(moduleSet) || (!m_opened && !simulationMode )){
    return;
  }

  int comp = 1;
  int reg  = 0;
  int address = epcRegisters->getRegAddress(moduleSet);

  vector<bool> moduleSetContent = epcRegisters->printContent(moduleSet);

  for(uint i=0; i<moduleSetContent.size(); i++)
  {
    if(moduleSetContent.at(i)){
      reg |= comp;
    }
    comp <<= 1;
    if(i%32 == 31){
      EPCWrite(address,reg);
      reg  = 0;
      comp = 1;
    }
  }
}


int DSSC_PPT::programQuadIOBRegisters()
{
  int numIOBsFound = numAvailableIOBs();

  int rc = 0;
  for(int i=0; i<numIOBsFound; i++){
    currentModule = activeIOBs[i];
    rc += programIOBRegisters();
  }

  return (rc==0)? ERROR_OK : ERROR_PARAM_READBACK;
}



int DSSC_PPT::programQuadIOBRegisters(const string & moduleSet)
{
  int rc = 0;
  for(uint i=0; i<numAvailableIOBs(); i++){
    currentModule = activeIOBs.at(i);
      rc |= programIOBRegister(moduleSet);
  }

  return (rc==0)? ERROR_OK : ERROR_PARAM_READBACK;
}


int DSSC_PPT::programIOBRegisters()
{
  initIOBRegsFromJTAGConfigFile();

  int rc = 0;
  const auto moduleSets = iobRegisters->getModuleSetNames();
  for(const auto & set : moduleSets){
    rc += programIOBRegister(set); // includes a readBack
  }

  return (rc==0)? ERROR_OK : ERROR_PARAM_READBACK;
}


int DSSC_PPT::programIOBRegister(const string & moduleSet, bool overwrite)
{
  if(!isIOBAvailable(currentModule)) return ERROR_IOB_NOT_FOUND;

  if(!iobRegisters->isModuleSetReadOnly(moduleSet)){
    int address = iobRegisters->getRegAddress(moduleSet);
    int value   = iobRegisters->getModuleSetValue(moduleSet,to_string(currentModule));

    iobControl_write(address,value);
  }

  //do always readBack

  if(readBackIOBSingleReg(moduleSet,overwrite)){
    return ERROR_OK;
  }

  return ERROR_PARAM_READBACK;
}

int DSSC_PPT::readBackIOBSingleRegs(const string & moduleSet, bool overwrite)
{
  if(simulationMode) return 0;

  int rc = 0;
  for(uint i=0; i<numAvailableIOBs(); i++){
    currentModule = activeIOBs.at(i);
    if(!readBackIOBSingleReg(moduleSet,overwrite)){
      rc++;
    }
  }
  return rc;
}


bool DSSC_PPT::readBackIOBSingleReg(const string & moduleSet, bool overwrite)
{
  if(simulationMode) return true;

  uint32_t address = iobRegisters->getRegAddress(moduleSet);
  uint32_t retVal  = iobControl_read(address);
#ifdef DEBUGPPT
  cout << "IOB Readback Set " << moduleSet << " " << retVal << " 0x" << hex << retVal << dec << endl;
#endif

  bool ok = iobRegisters->compareContent(retVal,moduleSet,to_string(currentModule),overwrite);
  if(!ok){
    showCompareErrors(iobRegisters,"IOBRegister_"+moduleSet+"_RB.txt");
  }
  return ok;
}


int DSSC_PPT::readBackIOBRegisters()
{
  if(simulationMode) return ERROR_OK;

  bool ok = true;

  for(uint i=0; i < numAvailableIOBs(); i++){
    currentModule = activeIOBs.at(i);
    ok  &= (readBackIOBRegister() == ERROR_OK);
  }

  return (ok)? ERROR_OK : ERROR_PARAM_READBACK;
}

int DSSC_PPT::readBackIOBRegister()
{
  if(simulationMode) return ERROR_OK;

  bool ok = true;
  if(!isIOBAvailable(currentModule)){
    // if IOB is not available do not send an error to not signal
    // a false readback result
    return ERROR_OK;
  }

  const auto moduleSets = iobRegisters->getModuleSetNames();
  for(const auto & set : moduleSets){
    ok &= readBackIOBSingleReg(set,false);
  }

  return (ok)? ERROR_OK : ERROR_PARAM_READBACK;
}

int DSSC_PPT::readBackEPCRegisters()
{
  bool ok = true;

  const auto moduleSets = epcRegisters->getModuleSetNames();
  for(const auto & set : moduleSets){
    ok &= readBackEPCRegister(set);
  }
  if(!ok){
    return ERROR_PARAM_READBACK;
  }

  return DSSC_PPT::ERROR_OK;

}


bool DSSC_PPT::readBackEPCRegister(const string & moduleSet, bool overwrite)
{
  if(simulationMode) return true;

  int address = epcRegisters->getRegAddress(moduleSet);
  int numBits = epcRegisters->getNumBitsPerModule(moduleSet);
  int numReads = ceil(numBits/32.0);
  bool ok = true;
  if(numBits != numReads*32){
    WARN_OUT( "WARNING DSSC_PPT: numBits corrected to "<< numReads*32 << " was " << numBits <<"! ModuleSet "<< moduleSet);
    numBits = numReads*32;
  }

  if(numReads==1){
    uint32_t value =  EPCRead(address);
    ok = epcRegisters->compareContent(value,moduleSet,overwrite);
  }else{
    vector<bool> bits(numBits);
    int bit = 0;
    for(int i=0; i<numReads; i++){
      uint32_t value =  EPCRead(address);
      uint32_t comp = 1;
      for(int i=0; i<32; i++){
        bits[bit] = ( (value & comp) != 0 );
        comp<<=1;
        bit++;
      }
    }

    ok = epcRegisters->compareContent(bits,moduleSet,overwrite);
  }

  if(!ok){
    showCompareErrors(epcRegisters,"EPCRegister_"+moduleSet+"_RB.txt");
  }

  return ok;
}

int DSSC_PPT::sendConfigurationFile(const string & fileName)
{
  if(simulationMode){
    cout << "Simulation mode: nothing sent!" << endl;
    return true;
  }

  int ret = sendFileFtp(fileName);
  if(ret != ERROR_OK){
    return ret;
  }

  if(!startJTAGProgramming()){
    ERROR_OUT("Error: startJTAGProgramming not successful!");
    return ERROR_FTP_SEND_FILE_FAILED;
  }
  return DSSC_PPT::ERROR_OK;
}


int DSSC_PPT::sendFileFtp(const string & fileName)
{

  ifstream file(fileName.c_str(), ifstream::in);
  if(!file.is_open()){
    ERROR_OUT("ERROR SendFile: file "+ fileName + " not found");
    return ERROR_FILE_NOT_FOUND;
  }
  file.close();

  DSSC_PPT_FTP ftp;
  if(!ftp.openFtp(pptTcpAddress,ftpPort)){
    ERROR_OUT("openFtp at " + pptTcpAddress +" failed");
    return ERROR_FTP_CONNECT_FAILED;
  }

  if(!ftp.sendFile(fileName)){
    ERROR_OUT("FTP send error");
    return ERROR_FTP_SEND_FILE_FAILED;
  }

  return ERROR_OK;
}


int DSSC_PPT::readFileFtp(const string & fileName)
{
  DSSC_PPT_FTP ftp;
  if(!ftp.openFtp(pptTcpAddress,ftpPort)){
    ERROR_OUT("openFtp at " + pptTcpAddress +" failed");
    return ERROR_FTP_CONNECT_FAILED;
  }

  if(!ftp.readFile(fileName)){
    ERROR_OUT("FTP read error");
    return ERROR_FTP_READ_FILE_FAILED;
  }

  return ERROR_OK;
}


int DSSC_PPT::loadReadbackDataFromPPT()
{

  string rbFileName = "ReadBackJtagCommands";
  readFileFtp(rbFileName);

  moveRBFileOnPPT();

  rbDataVec.clear();

  ifstream infile(rbFileName.c_str());

  string line;
  while (getline(infile, line))
  {
    if(line.length()>0){
      stringstream iss(line);
      int a;
      iss >> hex >> a; // error

      rbDataVec.push_back(a);
    }
  }

  return ERROR_OK;
}



int DSSC_PPT::programmFullASICConfig(bool readBack)
{
  sequencer->compileAllTracks();

  int ret = ERROR_OK;
  for(uint iob=0; iob< numAvailableIOBs(); iob++)
  {
    currentModule = activeIOBs.at(iob);
    ret |= programmASICConfig(readBack);
  }

  return ret;
}


int DSSC_PPT::programmASICConfig(bool readBack)
{
  if(!programJtag(readBack)){
    return ERROR_ASIC_INIT_ERROR;
  }

  if(!programPixelRegs(readBack)){
    return ERROR_ASIC_INIT_ERROR;
  }

  if(!programSequencer(readBack)){
   return ERROR_ASIC_INIT_ERROR;
  }

  calculateRegisterXors(AllRegs);

  return ERROR_OK;
}

// Test connection to ASIC JTAG chain, works only if ASIC is powerd
void DSSC_PPT::jtagBypassTest(uint8_t pattern)
{
   ERROR("Not implemented");

//   initConfigVector();
//
//   configVector.push_back(c_jtagInstrEnReadback | c_jtagInstrBypass);
//   configVector.push_back(30);
//   configVector.push_back(0);
//   configVector.push_back(pattern);
//   configVector.push_back(pattern);
//   configVector.push_back(pattern);
//   configVector.push_back(pattern);
//
// #ifdef SENDDIRECT
//   sendConfigurationDirectly(configVector);
// #endif
//
//   bool ok = sendConfigurationVector(configVector,NoRegs);
//   if(!ok){
//     ERROR_OUT("Error: Configuration File could not be send!");
//     return;
//   }
//
//   loadReadbackDataFromPPT();
//
//   cout << "Got " << rbDataVec.size() << " Words from PPT" << endl;
//   //rbDataVec.erase(rbDataVec.begin());
//   for(const auto & rbData : rbDataVec){
//     cout << "Got 0x" << hex << (unsigned) rbData << endl;
//   }
}


void DSSC_PPT::resetASICJTAGControllers()
{
  for(uint i=0; i<numAvailableIOBs(); i++){
    currentModule = activeIOBs.at(i);
    resetASICJTAGController();
  }
}


bool DSSC_PPT::programSequencers(bool readBack)
{
  bool rc = true;
  for(uint i=0; i<numAvailableIOBs(); i++){
    currentModule = activeIOBs.at(i);
    rc &= programSequencer(readBack);
  }

  return rc;
}


bool DSSC_PPT::programSequencer(bool readBack, bool setJtagEngineBusy, bool reprogramJtag)
{
  ContModeKeeper keeper(this,readBack);

  DEBUG_OUT("++++ Programming Module "+to_string(currentModule) +" Sequencer");

  checkJtagEnabled();

  initConfigVector();

  addSequencerToConfigVector();

  if(readBack)
    addSequencerToConfigVector(true);

  if(reprogramJtag){
    addSingleJTAGRegistersToConfigVector("Master FSM Config Register");
  }

  bool ok = sendConfigurationVector(configVector,SeqRegs);
  if(!ok){
    ERROR_OUT("Error: Configuration File could not be send!");
    return false;
  }

  if(readBack){
    loadReadbackDataFromPPT();

    ok &= checkSequencerRBData();

  }else{
    waitJTAGEngineDone();
  }

  if(!ok){
    showCompareErrors(sequencer,"Sequencer.txt");
  }

  return ok;
}

bool DSSC_PPT::readBackSequencer()
{
  ContModeKeeper keeper(this);

  checkJtagEnabled();

  initConfigVector();

  addSequencerToConfigVector(true);

  bool ok = sendConfigurationVector(configVector,SeqRegs);
  if(!ok){
    ERROR_OUT("Error: Configuration File could not be send!");
    return false;
  }

  errorString = "RB ERROR: Sequencer Register: ";
  loadReadbackDataFromPPT();

  ok &= checkSequencerRBData();
  if(!ok){
    showCompareErrors(sequencer,"Sequencer.txt");
    ERROR_OUT("Sequencer readback errors");
  }

  return ok;
}


bool DSSC_PPT::getContentFromDevice(uint bitStreamLength, vector<bool> &data_vec)
{
  uint byteStreamLength = getByteStreamLength(bitStreamLength);

  if(byteStreamLength>rbDataVec.size()){
    ERROR_OUT("ERROR: Not enough readBack data available: "+to_string(rbDataVec.size())+"/"+to_string(byteStreamLength) );
    return false;
  }
  //generate bool vector from readback data
  data_vec.resize(bitStreamLength);

  uint bit=0;
  for(uint byte=0; byte<byteStreamLength; byte++)
  {
    for(int i=0; i<8; i++)
    {
      data_vec[bit++] = (rbDataVec[byte]&(1<<i)) != 0;

      if(bit==bitStreamLength)
      {//end both loops
        i    = 8;
        byte = byteStreamLength;
      }
    }
  }

  rbDataVec.erase(rbDataVec.begin(),rbDataVec.begin()+byteStreamLength);

  return true;
}


bool DSSC_PPT::checkSingleModuleRegRBData(ConfigReg *configReg, const string & moduleSet, bool overwrite)
{
  if(simulationMode){
    int byteStreamLength = getByteStreamLength(configReg->getNumBitsPerModule(moduleSet));
    simReadbackFromJTAG(byteStreamLength);
    return true;
  }

  return CHIPInterface::checkSingleModuleRegRBData(configReg,moduleSet,overwrite);
}


int DSSC_PPT::programAllPixelRegs(bool readBack)
{
  int rc = ERROR_OK;
  for(uint i=0; i<numAvailableIOBs(); i++){
    currentModule = activeIOBs.at(i);
    if(!programPixelRegs(readBack)){
      rc = ERROR_PARAM_READBACK;
    }
  }

  return rc;
}

bool DSSC_PPT::isSpecialComponentId()
{
  if(getActiveComponentId() == "DSSC-F2-MSDD-Ladder-2-2"){
    return true;
  }else{
    return false;
  }
}

bool DSSC_PPT::runSpecialPixelProgrammingRoutine()
{
  WARN_OUT("For the selected component: " + getActiveComponentId() + " a special pixel register programming routine is called");
  WARN_OUT("For this componentID, the pixel registers can not be readback");
  bool ok = true;
  if(getActiveComponentId() == "DSSC-F2-MSDD-Ladder-2-2"){
    ok = programPixelRegsInChain(false);
    vector<int> asicsForSingleProgramming {0,1,2,3,4,5,6,7,8,9,11};
    for(auto && asic : asicsForSingleProgramming){
      //TODO check readback then set true;
      ok &= programPixelRegsSingleASIC(asic,false);
    }
  }else{
    ERROR_OUT("The component is flagged special but no special pixel register programming routine is implemented yet");
  }
  return ok;
}


bool DSSC_PPT::programPixelRegs(bool readBack, bool setJtagEngineBusy)
{
  bool allSame = !pixelRegisters->moduleSetIsVarious("Control register","all");
  if(allSame)
  {
    return programPixelRegsAllAtOnce(readBack);
  }
  else
  {
    if(isSpecialComponentId())
    {
      return runSpecialPixelProgrammingRoutine();
    }
    else
    {
      return programPixelRegsInChain(readBack);
    }
  }
}


void DSSC_PPT::checkPixelRegsInChain(bool inChain)
{
  uint reqValue = (inChain)? 1 : 0;
  if(jtagRegisters->getSignalValue("Global Control Register","0","SC_EnChainLd") != reqValue){
    DEBUG_OUT("Warning: SC_EnChainLd must be set to " << reqValue << " before Pixel Programming");
    jtagRegisters->setSignalValue("Global Control Register", "all", "SC_EnChainLd",reqValue);
    addSingleJTAGRegistersToConfigVector("Global Control Register");
  }
}


void DSSC_PPT::setPixelRegsInChain(bool inChain)
{
  uint reqValue = (inChain)? 1 : 0;
  jtagRegisters->setSignalValue("Global Control Register", "all", "SC_EnChainLd",reqValue);
}


bool DSSC_PPT::programPixelRegsAllAtOnce(bool readBack)
{
  ContModeKeeper keeper(this,readBack);

  DEBUG_OUT("++++ Programming Module "+to_string(currentModule) +" Pixel registers all at once");

  checkJtagEnabled();

  initConfigVector();

  checkPixelRegsInChain(false);

  addDefaultPixelRegisterToConfigVector("Control register",readBack);

  bool ok = sendConfigurationVector(configVector,PxRegs);
  if(!ok){
    return false;
  }
  else if (readBack)
  {
    loadReadbackDataFromPPT();
    ok &= checkSingleModuleRegRBData(pixelRegisters,"Control register");
    if(!ok){
      showCompareErrors(pixelRegisters,"PixelRegsAllSame_RB.txt");
    }
  }

  usleep(MINJTAGDELAY);

  return ok;
}


bool DSSC_PPT::programPixelRegsSingleASIC(int asic, bool readBack)
{
  ContModeKeeper keeper(this,readBack);

  DEBUG_OUT("++++ Programming Module "+to_string(currentModule) +" Pixel registers of Asic " + to_string(asic));

  checkJtagEnabled();

  initConfigVector();

  checkPixelRegsInChain(true);

  addPixelRegistersToConfigVector(asic,readBack);

  bool ok = sendConfigurationVector(configVector,PxRegs);
  if(!ok){
    ERROR_OUT("Error: Configuration File could not be send!");
    return false;
  }
  else if (readBack)
  {
    loadReadbackDataFromPPT();
    ok &= readBackPixelSingleASIC(asic);
    if(!ok){
      showCompareErrors(pixelRegisters,"PixelRegsInChain_RB.txt");
    }else{
      STATUS_OUT("Single ASIC Pixel Register Readback successful");
    }
  }else{
    waitJTAGEngineDone();

    usleep(MINJTAGDELAY);
  }

  return ok;
}


bool DSSC_PPT::programPixelRegsInChain(bool readBack)
{
  ContModeKeeper keeper(this,readBack);
  DEBUG_OUT("++++ Programming Module " + to_string(currentModule) + " Pixel registers in chain");

  checkJtagEnabled();

  initConfigVector();

  checkPixelRegsInChain(true);

  addPixelRegistersToConfigVector(readBack);

  bool ok = sendConfigurationVector(configVector,PxRegs);
  if(!ok){
    ERROR_OUT("Error: Configuration File could not be send!");
    return false;
  }
  else if (readBack)
  {
    loadReadbackDataFromPPT();
    ok &= readBackPixelChain();
    if(!ok){
      showCompareErrors(pixelRegisters,"PixelRegsInChain_RB.txt");
    }
  }else{
    waitJTAGEngineDone();

    usleep(MINJTAGDELAY);
  }

  DEBUG_OUT("++++ Programming Pixel registers done");

  return ok;
}


/* uses direct pixel access to program a single pixel */
void DSSC_PPT::programPixelRegDirectly(int px, bool setJtagEngineBusy)
{
  if (px < 0 || px > getNumPxs()) {
    ERROR_OUT("Pixel to program (" + to_string(px) + ") is out of range.");
    return;
  }

  checkJtagEnabled();

  initConfigVector();

  checkPixelRegsInChain(false);

  addPixelRegistersToConfigVector("Control register",px);

  bool ok = sendConfigurationVector(configVector,PxRegs);
  if(!ok){
    ERROR_OUT("Error: Configuration File could not be send!");
  }

  usleep(MINJTAGDELAY);
}


bool DSSC_PPT::readBackPixelSingleASIC(int asic)
{
  if(simulationMode){
    int byteStreamLength = getByteStreamLength(pixelRegisters->getNumBitsPerModule("Control register"));
    simReadbackFromJTAG(byteStreamLength);
    return true;
  }

  cout << "rbDataVec.size() = " << rbDataVec.size() << endl;
  cout << "additional Bytes = " << rbDataVec.size() -  (4*1024*c_controlBitsPerPx/8u) << endl;

  //hard to implement without changes in the firmware

  if(rbDataVec.size() < 4*1024*c_controlBitsPerPx/8u){
    ERROR_OUT("Error: Configuration Vector too small!");
    return false;
  }

  int asicIdx = (asic>7)? asic : 7-asic;
  int bitsPerModule = pixelRegisters->getNumBitsPerModule("Control register");
  int bitsPerAsic = bitsPerModule*4096;
  int startBit = bitsPerAsic *  asicIdx;
  int endBit   = bitsPerAsic * (asicIdx+1);
  vector<bool> pxRegsBits = pixelRegisters->printContent("Control register");
  vector<bool> asicPxRegsBits;
  asicPxRegsBits.assign(pxRegsBits.begin() + startBit, pxRegsBits.begin() + endBit);

  int removeBypassBits = (asic>7)? asic : 7-asic;

  std::vector<bool> bits;
  getContentFromDevice(rbDataVec.size()*8,bits);

  for(int i=0; i<removeBypassBits; i++){
    bits.erase(bits.begin());
  }

  int numBitsPerChunk = 1024*bitsPerModule;
  int numChunks = asicPxRegsBits.size()/numBitsPerChunk;
  std::vector<int> numBytes {0,1,2,1,0};
  for(int ch=0; ch<4; ch++){
    for(int bit=0; bit<(8*numBytes[ch]); bit++){
      bits.erase(bits.begin()+numBitsPerChunk*ch);
    }
  }

  bool ok = pixelRegisters->compareContent(bits,"Control register",asic,false);

  {
    ofstream out("RBSingleASICPixelRegs.txt");
    int numBits = std::min(bits.size(),asicPxRegsBits.size());
    int errorCnt = 0;

    for(int idx=0; idx<numBits; idx++){
      out << asicPxRegsBits[idx] << " / " << bits[idx];
      if(asicPxRegsBits[idx] != bits[idx]){
        out << "  error";
        errorCnt++;
      }
      out << "\n";
    }
    out.close();

    cout << "Remove " << removeBypassBits << endl;
    cout << "NumChunks " << numChunks << endl;
    cout << "Error Count " << errorCnt << endl;
    cout << "Remaining Bytes " << rbDataVec.size() << endl;

    rbDataVec.clear();
  }
  return ok;
}


bool DSSC_PPT::readBackPixelChain()
{
  if(simulationMode){
    int byteStreamLength = getByteStreamLength(pixelRegisters->getNumBitsPerModule("Control register") * pixelRegisters->getNumModules("Control register"));
    simReadbackFromJTAG(byteStreamLength);
    return true;
  }

  if(rbDataVec.size() < getNumberOfActiveAsics() * 4*1024*c_controlBitsPerPx/8u){
    ERROR_OUT("Error: Configuration Vector too small!");
    return false;
  }

  return CHIPInterface::readBackPixelChain();
}

bool DSSC_PPT::sendConfigurationVector(vector<uint32_t> &data, RegisterType regs)
{

#ifdef SENDDIRECT
  bool ok = true;
  // configration is sent directly in appendJtagRegAndSend
#else
  bool ok = sendConfigurationVectorAsFile(data);
#endif

  calculateRegisterXors(regs);
  return ok;
}


bool DSSC_PPT::sendConfigurationVectorAsFile(vector<uint32_t> &data)
{
  printVectorToConfigFile(data,"cmdsFromSoftware");
  int ret = sendConfigurationFile("cmdsFromSoftware");
  bool ok = (ret == ERROR_OK);
  return ok;
}


bool DSSC_PPT::readBackJtag(bool overwrite)
{
  checkJtagEnabled();

  initConfigVector();

  addJTAGRegistersToConfigVector(true);

  bool ok = sendConfigurationVector(configVector,JtagRegs);
  if(ok){
    loadReadbackDataFromPPT();
    ok &= checkAllModuleRegsRBData(jtagRegisters,overwrite);
    if(!ok){
      showCompareErrors(jtagRegisters,"JtagRegs.txt");
    }
  }

  return ok;
}

bool DSSC_PPT::readBackSingleJtag(const string & moduleSet, bool overwrite)
{
  checkJtagEnabled();

  initConfigVector();

  addSingleJTAGRegistersToConfigVector(moduleSet,true);

  bool ok = sendConfigurationVector(configVector,JtagRegs);
  if(ok){
    loadReadbackDataFromPPT();
    ok &= checkSingleModuleRegRBData(jtagRegisters,moduleSet,overwrite);
    if(!ok){
      showCompareErrors(jtagRegisters,"JtagRegs.txt");
    }
  }

  return ok;
}



int DSSC_PPT::programJtags(bool readBack)
{
  ContModeKeeper keeper(this,readBack);

  bool ok = true;
  for(uint i=0; i<numAvailableIOBs(); i++){
    currentModule = activeIOBs.at(i);
    ok &= programJtag(readBack);
  }

  return (ok)? ERROR_OK : ERROR_PARAM_READBACK;
}


bool DSSC_PPT::programJtag(bool readBack, bool setJtagEngineBusy, bool recalcXors)
{
  ContModeKeeper keeper(this,readBack);

  DEBUG_OUT("++++ Programming Module "+to_string(currentModule) +" JTAG registers");

  checkJtagEnabled();

  initConfigVector();

  addJTAGRegistersToConfigVector();

  if(readBack){
    addJTAGRegistersToConfigVector(true);
  }

  bool ok = sendConfigurationVector(configVector,JtagRegs);
  if(ok && readBack){
    loadReadbackDataFromPPT();
    ok &= checkAllModuleRegsRBData(jtagRegisters);
    if(!ok){
      showCompareErrors(jtagRegisters,"JtagRegs.txt");
    }
  }else{
    waitJTAGEngineDone();
  }

  return ok;
}

bool DSSC_PPT::programJtagSingle(const string & moduleSet, bool readBack, bool setJtagEngineBusy, bool recalcXors, bool overwrite)
{
  ContModeKeeper keeper(this,readBack);

  DEBUG_OUT("++++ Programming Module "+to_string(currentModule) +" JTAG register " + moduleSet + " active Module " + to_string(currentModule));

  checkJtagEnabled();

  initConfigVector();

  addSingleJTAGRegistersToConfigVector(moduleSet);

  if(readBack){
    addSingleJTAGRegistersToConfigVector(moduleSet,true);
  }

  bool ok = sendConfigurationVector(configVector,JtagRegs);
  if(ok && readBack){
    loadReadbackDataFromPPT();
    ok &= checkSingleModuleRegRBData(jtagRegisters,moduleSet,overwrite);
    if(!ok){
      showCompareErrors(jtagRegisters,"JtagRegs_"+moduleSet+"_.txt");
    }
  }else{
    waitJTAGEngineDone();
  }

  return ok;
}


void DSSC_PPT::addJTAGRegistersToConfigVector(bool readBack)
{
  //add all bytes to same file
  const auto moduleSets = jtagRegisters->getModuleSetNames();
  for(const auto & set : moduleSets) {
    addSingleJTAGRegistersToConfigVector(set,readBack);
  }
}

void DSSC_PPT::addSingleJTAGRegistersToConfigVector(const string & moduleSet,bool readBack)
{
  uint8_t jtagInstr = jtagRegisters->getRegAddress(moduleSet);
  vector<bool> bits = jtagRegisters->printContent(moduleSet);
  vector<string> outputs = jtagRegisters->getOutputs(moduleSet);
  if(outputs.size() != (size_t)getNumberOfActiveAsics()){
    cout << "ERROR: Config file is corrupted!! outputs of JTAG reg " << currentModule << " has wrong size!!" << endl;
    cout << "outputs.size() = " << outputs.size() << " != " << getNumberOfActiveAsics() << " please regenerate config file" << endl;
    exit(0);
  }
  // set all periphery decoupling bits to 0
  if(moduleSet.compare("Global FCSR 0") == 0)
  {
    bits.clear();

    size_t numBits = jtagRegisters->printContent(moduleSet,"0").size();
    bits.reserve((numBits+globalFCSR0Vec.size())*getNumberOfActiveAsics());

    for(int i=0; i<getNumberOfActiveAsics(); i++){
      vector<bool> fcr0_bits = jtagRegisters->printContent(moduleSet,outputs.at(i));
      bits.insert(bits.end(),fcr0_bits.begin(),fcr0_bits.end()); // fcsr -> +1 bit
      bits.insert(bits.end(),globalFCSR0Vec.begin(),globalFCSR0Vec.end()); // fcsr -> +1 bit
    }
    bits.push_back(false); //198*5 + 32 * 16 + 1 =  16353
  }

  if(moduleSet.compare("Global FCSR 1") == 0)
  {
    bits.clear();

    size_t numBits = jtagRegisters->printContent(moduleSet,"0").size();
    bits.reserve((numBits+globalFCSR1Vec.size())*getNumberOfActiveAsics());

    for(int i=0; i<getNumberOfActiveAsics(); i++){
      vector<bool> fcr1_bits = jtagRegisters->printContent(moduleSet,outputs.at(i));
      bits.insert(bits.end(),fcr1_bits.begin(),fcr1_bits.end()); // fcsr -> +1 bit
      bits.insert(bits.end(),globalFCSR1Vec.begin(),globalFCSR1Vec.end()); // fcsr -> +1 bit
    }
    bits.push_back(false); //(148*5 + 32) * 16 +1  = 12352 (0x3040)
  }

  appendAndSendJtagReg(configVector, jtagInstr, bits, readBack);
}

void DSSC_PPT::addDefaultPixelRegisterToConfigVector(const string & moduleSet, bool readBack)
{
  const uint8_t c_jtagInstrPx0Reg = 0b00011110; //30
  const uint8_t c_jtagInstrPx1Reg = 0b00011111; //31

  enableYSelEnableXSel(configVector);

  vector<bool> pxRegsBits = pixelRegisters->printContent(moduleSet,"0");
#ifdef DEBUGPPT
  cout << "bool vector to program: " << utils::boolVecToStdStr(pxRegsBits)  << endl;
#endif
  vector<bool> bits;
  bits.reserve(getNumberOfActiveAsics()*pxRegsBits.size());
  for(int i=0; i<getNumberOfActiveAsics(); i++){
    bits.insert(bits.end(),pxRegsBits.begin(),pxRegsBits.end());
  }
  bits.push_back(false); // extra bit because of gated two phase clocking


  appendAndSendJtagReg(configVector, c_jtagInstrPx0Reg, bits, readBack);
  appendAndSendJtagReg(configVector, c_jtagInstrPx1Reg, bits, readBack);
}

void DSSC_PPT::disableYSelEnableXSel(vector<uint32_t> &data)
{
  const uint8_t c_jtagInstrX0SelReg       = 0b00010001; //17
  const uint8_t c_jtagInstrX1SelReg       = 0b00010010; //18
  const uint8_t c_jtagInstrYSelReg        = 0b00010011; //19

  const int numAsics = getNumberOfActiveAsics();

  const int numXSelBits = (c_totalXSelBits)*numAsics;
  const int numYSelBits = (c_totalYSelBits)*numAsics;

  vector<bool> xBits(numXSelBits,true);
  vector<bool> yBits(numYSelBits,false);

  xBits.push_back(true);
  yBits.push_back(false);

  appendAndSendJtagReg(data,c_jtagInstrX0SelReg,xBits,false);
  appendAndSendJtagReg(data,c_jtagInstrX1SelReg,xBits,false);
  appendAndSendJtagReg(data,c_jtagInstrYSelReg ,yBits,false);
}


void DSSC_PPT::disableYSelEnableXSel(vector<uint32_t> &data, int asic)
{
  const uint8_t c_jtagInstrX0SelReg       = 0b00010001; //17
  const uint8_t c_jtagInstrX1SelReg       = 0b00010010; //18
  const uint8_t c_jtagInstrYSelReg        = 0b00010011; //19

  const int numChipsJtagChain = getNumberOfActiveAsics();
  const int asicIdx = (asic>7)? asic : 7-asic;
  const int numBypassBits = numChipsJtagChain-asicIdx-1;

  int numXSelBits = c_totalXSelBits+numBypassBits + 1;
  int numYSelBits = c_totalYSelBits+numBypassBits + 1;

  vector<bool> xBits(numXSelBits,true);
  vector<bool> yBits(numYSelBits,false);

  appendAndSendJtagReg(data,asic,c_jtagInstrX0SelReg,xBits,false);
  appendAndSendJtagReg(data,asic,c_jtagInstrX1SelReg,xBits,false);
  appendAndSendJtagReg(data,asic,c_jtagInstrYSelReg ,yBits,false);
}


void DSSC_PPT::enableYSelEnableXSel(vector<uint32_t> &data)
{
  const uint8_t c_jtagInstrX0SelReg       = 0b00010001; //17
  const uint8_t c_jtagInstrX1SelReg       = 0b00010010; //18
  const uint8_t c_jtagInstrYSelReg        = 0b00010011; //19

  int numAsics = getNumberOfActiveAsics();

  int numXSelBits = (c_totalXSelBits)*numAsics;
  int numYSelBits = (c_totalYSelBits)*numAsics;

  vector<bool> xBits(numXSelBits,true);
  vector<bool> yBits(numYSelBits,true);

  xBits.push_back(true);
  yBits.push_back(true);

  appendAndSendJtagReg(data,c_jtagInstrX0SelReg,xBits,false);
  appendAndSendJtagReg(data,c_jtagInstrX1SelReg,xBits,false);
  appendAndSendJtagReg(data,c_jtagInstrYSelReg ,yBits,false);
}


void DSSC_PPT::setAllASICSBypass()
{
  vector<bool> zeroPayloadBits(16,false);
  appendAndSendJtagReg(configVector,c_jtagInstrBypass,zeroPayloadBits,false);
}


void DSSC_PPT::programXYSelectRegs(std::vector<uint32_t> &data, int px)
{
  int numAsics = getNumberOfActiveAsics();
  int asic     = numAsics - px/numOfPxs;
  px = px%numOfPxs;

  if (asic >= numAsics){
    ERROR_OUT("programXYSelectRegs(int px): px asic " + to_string(asic) + " out of range");
    return;
  }
  // mapping of pixel number to bit position in xy control chain
  int xPos = px % c_pxsPerRow;
  int yPos = px / c_pxsPerRow;

  int numXSelBits = (c_totalXSelBits)*numAsics;
  int numYSelBits = (c_totalYSelBits)*numAsics;

  vector<bool> xBits0(numXSelBits,false);
  vector<bool> xBits1(numXSelBits,false);

  xBits0.push_back(false);
  xBits1.push_back(false);

  if (xPos < 32){
    xPos = xPos + (c_totalXSelBits)*asic + asic;
    xBits0[xPos] = true;
  }else{
    //is programmed reverse - Last bit is padding - after each asic one padding bit
    xPos = numXSelBits-1-1 - (xPos-c_totalXSelBits) - (c_totalXSelBits)*asic + asic;
    xBits1[xPos] = true;
  }

  appendAndSendJtagReg(data,c_jtagInstrX0SelReg,xBits0,false);
  appendAndSendJtagReg(data,c_jtagInstrX1SelReg,xBits1,false);

  vector<bool> yBits(numYSelBits,false);
  yPos = numYSelBits-1-1 - yPos - c_totalYSelBits*asic - asic;
  yBits[yPos] = true;

  appendAndSendJtagReg(data,c_jtagInstrYSelReg,yBits,false);

  // extra bit and reverse for full custom chain
  //std::reverse(selData.begin(), selData.end());
  //selData.push_back(true);
}


void DSSC_PPT::addPixelRegistersToConfigVector(int asic, bool readBack)
{
  const int numChipsJtagChain = getNumberOfActiveAsics();

  static const uint8_t c_jtagInstrPx0Reg = 0b00011110; //30
  static const uint8_t c_jtagInstrPx1Reg = 0b00011111; //31

  // chain x/y reg programming
  // disableYSelEnableXSel(configVector);

  setAllASICSBypass();

  // single ASIC x/y reg programming
  disableYSelEnableXSel(configVector,asic);


  setAllASICSBypass();


  /* pixel numbering: looking from the top of the chip 0 is in the bottom left corner, 4095 at
    * the top right
    * total bits = 4096*47 = 192512 bits = 24064 bytes
    * max bits per JTAG package: 2^16 = 65536
    * --> 4 JTAG packets
    * output list contains all pixels, and is reversed
    */
  // now fill all pixel registers in chain mode

  int asicIdx = (asic>7)? asic : 7-asic;
  int bitsPerAsic = pixelRegisters->getNumBitsPerModule("Control register")*4096;
  int startBit = bitsPerAsic *  asicIdx;
  int endBit   = bitsPerAsic * (asicIdx+1);
  vector<bool> pxRegsBits = pixelRegisters->printContent("Control register");
  vector<bool> asicPxRegsBits;
  asicPxRegsBits.assign(pxRegsBits.begin() + startBit, pxRegsBits.begin() + endBit);

  int chunksize = 1024*c_controlBitsPerPx;
  int numChunks = asicPxRegsBits.size() / chunksize;

  uint8_t jtagInstrPxReg = c_jtagInstrPx0Reg;
  for(int ch=0; ch<numChunks; ch++)
  {
    jtagInstrPxReg = ((ch%4) < 2)? c_jtagInstrPx1Reg : c_jtagInstrPx0Reg;

    vector<bool> subPxRegsBits = getSubPxRegBits(asicPxRegsBits,ch*chunksize,chunksize);

    vector<bool> bitsToProgram;
    bitsToProgram.insert(bitsToProgram.end(),subPxRegsBits.begin(),subPxRegsBits.end());
    bitsToProgram.push_back(false); // extra bit because of gated two phase clocking

    if((ch%2) == 1){
      int numBypassBits = numChipsJtagChain-asicIdx-1;
      for(int i=0; i<numBypassBits; i++){
        bitsToProgram.push_back(false); // extra bit because of gated two phase clocking
      }
    }
    appendAndSendJtagReg(configVector, asic, jtagInstrPxReg, bitsToProgram, readBack); //
  }
}


void DSSC_PPT::addPixelRegistersToConfigVector(bool readBack)
{
  const uint8_t c_jtagInstrPx0Reg = 0b00011110; //30
  const uint8_t c_jtagInstrPx1Reg = 0b00011111; //31

  disableYSelEnableXSel(configVector);

  /* pixel numbering: looking from the top of the chip 0 is in the bottom left corner, 4095 at
    * the top right
    * total bits = 4096*47 = 192512 bits = 24064 bytes
    * max bits per JTAG package: 2^16 = 65536
    * --> 4 JTAG packets
    * output list contains all pixels, and is reversed
    */
  // now fill all pixel registers in chain mode
  vector<bool> pxRegsBits = pixelRegisters->printContent("Control register");
  int chunksize = 1024*c_controlBitsPerPx;
  int numChunks = pxRegsBits.size() / chunksize;

  uint8_t jtagInstrPxReg = c_jtagInstrPx0Reg;
  for(int ch=0; ch<numChunks; ch++)
  {
    jtagInstrPxReg = ((ch%4) < 2)? c_jtagInstrPx1Reg : c_jtagInstrPx0Reg;

    vector<bool> subPxRegsBits = getSubPxRegBits(pxRegsBits,ch*chunksize,chunksize);
    subPxRegsBits.push_back(false); // extra bit because of gated two phase clocking
    appendAndSendJtagReg(configVector, jtagInstrPxReg, subPxRegsBits, readBack);
  }
}

vector<bool> DSSC_PPT::getSubPxRegBits(const vector<bool> &pxRegsBits, uint start, uint numBits)
{
  vector<bool> subPxRegsBits;
  subPxRegsBits.reserve(numBits);

  if(start+numBits > pxRegsBits.size()){
    ERROR_OUT("Error DSSC_PPT getSubPxRegBits: too much bits specified \n"
              "start=" + to_string(start) + " numBits=" + to_string(numBits) +
              " pxRegsBits.size() = " + to_string(pxRegsBits.size()));
    throw "DSSC_PPT subPixel number error!!";
  }

  auto startIt = pxRegsBits.begin()+start;
  subPxRegsBits.insert(subPxRegsBits.begin(),startIt,startIt+numBits);
  return subPxRegsBits;
}

void DSSC_PPT::addPixelRegistersToConfigVector(const string& moduleSet, int px)
{
  programXYSelectRegs(configVector,px);

  vector<bool> bits = pixelRegisters->printContent(moduleSet,to_string(px));
  bits.push_back(false);

  if (px%c_pxsPerRow < 32){
    appendAndSendJtagReg(configVector,c_jtagInstrPx0Reg,bits,false);
  }else{
    appendAndSendJtagReg(configVector,c_jtagInstrPx1Reg,bits,false);
  }
}



void DSSC_PPT::addSequencerToConfigVector(bool readBack)
{
  { // add hold bits to configVector
    vector<bool> holdBits;
    sequencer->getHoldProgBits(holdBits);

    const uint32_t singleASICBits = holdBits.size();
    for(int i=1; i<getNumberOfActiveAsics(); i++){
      holdBits.insert(holdBits.end(),holdBits.begin(),holdBits.begin()+singleASICBits);
    }

    appendAndSendJtagReg(configVector, c_jtagInstrSeqHoldCnts, holdBits, readBack);
  }


  for (int i=0; i<Sequencer::c_numOfTracks; ++i)
  { // add track bits to configVector

    vector<bool> trackBits;
    sequencer->getTrackDdynProgBits((Sequencer::TrackNum)i, trackBits);
    int jtagSubAddress = sequencer->getTrackJtagSubAddress((Sequencer::TrackNum)i);

    const uint32_t singleASICBits = trackBits.size();
    for(int i=1; i<getNumberOfActiveAsics(); i++){
      trackBits.insert(trackBits.end(),trackBits.begin(),trackBits.begin()+singleASICBits);
    }
    appendAndSendJtagReg(configVector, c_jtagInstrSeqTrack | (uint8_t)jtagSubAddress, trackBits, readBack);
  }
}


int DSSC_PPT::EPCRead(int address)
{
  dataString = "EPCC R " + to_string(address);
  return  sendReadPacket();
}


void DSSC_PPT::EPCWrite(int address, int data)
{
  dataString = "EPCC W " + to_string(address) + " " + to_string(data);
  sendReadPacket();
}


void DSSC_PPT::restartPPT()
{
  string command = "/opt/bin/FPGA_restart";
  executeShellCommand(command,true);

  closeConnection();
}


void DSSC_PPT::moveRBFileOnPPT()
{
  string command = "mv /tmp/ReadBackJtagCommands /tmp/ReadBackJtagCommands_read";
  executeShellCommand(command,true);
}


bool DSSC_PPT::sendFlashFirmware()
{
  runContinuousMode(false);

  dataString = "FLFI";
  if(sendReadPacket() != 0){
    ERROR_OUT("Could not flash firmware - PPT did not receive binary file!");
    return false;
  }
  return true;
}


bool DSSC_PPT::sendFlashLinux()
{
  runContinuousMode(false);

  dataString = "FLLI";
  if(sendReadPacket(true) != 0){
    ERROR_OUT("Could not flash linux - PPT did not receive binary file!");
    return false;
  }
  return true;
}

int DSSC_PPT::uploadIOBFirmwareFile(const string & fileName)
{
  int ret = sendFileFtp(fileName);
  if(ret != ERROR_OK){
    return ret;
  }

  return sendUpdateIOBFirmware();
}

int DSSC_PPT::sendUpdateIOBFirmware()
{
  dataString = "UPIF";
  if(sendReadPacket(true) != 0){
    ERROR_OUT("Could not update IOB firmware - PPT did not receive bitfile!");
    return ERROR_FTP_SEND_FILE_FAILED;
  }
  return ERROR_OK;
}

int DSSC_PPT::sendUpdateSoftware()
{
  dataString = "UPDS";
  if(sendReadPacket(true) != 0){
    ERROR_OUT("Could not update Microblaze programs - PPT did not receive files!");
    return ERROR_FTP_SEND_FILE_FAILED;
  }
  return ERROR_OK;
}

void DSSC_PPT::programIOBFPGA(int iobNumber)
{
  setIOBSerialForTrainData(iobNumber,0);

  iobsNotChecked = true;
  activeIOBs.clear();

  string command = "/opt/bin/xsvf_player -n " + to_string(iobNumber) + " /opt/IOB_Firmware.xsvf";
  executeShellCommand(command,true);
}

void DSSC_PPT::setIOBSerialForTrainData(int iobNumber, uint32_t serial)
{
  setEPCParam("IOB_Serial_Numbers","0","IOB"+to_string(iobNumber)+"_Serial",(serial));
}

void DSSC_PPT::setQuadrantIdForTrainData(uint16_t quadrantId)
{
  setEPCParam("Det_Specific_Data_Register","0","UserSpecificData1",quadrantId);
}

void DSSC_PPT::setQuadrantId(const std::string & quadrantId)
{
  setQuadrantIdForTrainData(utils::DsscModuleInfo::toShortId(quadrantId));
}

void DSSC_PPT::setDACSetting(int DACSetting)
{
  uint8_t word1 = (uint8_t)(DACSetting >> 8);
  uint8_t word2 = (uint8_t) DACSetting;

  stringstream ss;
  ss.width(2);
  ss.fill('0');

  ss << "SPIC 00040002";
  ss << hex << word1 << word2;

  ss  >> dataString;

  ERROR("Check this nothing send: " <<  dataString);

  //sendPacket();
}


void DSSC_PPT::sendRS232command(const string & command)
{
  dataString = "RPSC " + command;
  sendReadPacket();
}


int DSSC_PPT::iobControl_read(int address)
{
  dataString = "IOBC R " + to_string(currentModule) + " " + to_string(address);
  return sendReadPacket();
}


void DSSC_PPT::iobControl_write(int address, int data)
{
  dataString = "IOBC W " + to_string(currentModule) + " " + to_string(address) + " " + to_string(data);
  sendReadPacket();
}


void DSSC_PPT::executeShellCommand(const string & command, bool wait)
{
  dataString = "EXEC " + command;
  if(wait)
    sendReadPacket();
  else
    sendPacket();
}

void DSSC_PPT::resetASICJTAGController()
{
  dataString = "JTGC " + to_string(currentModule) + "ff";
  sendReadPacket(true);
}


void DSSC_PPT::setASICReset(bool reset)
{
  runContinuousMode(false);

  uint value = (reset)? 1u : 0u;

  iobRegisters->setSignalValue("ASIC_reset",to_string(currentModule),"ASIC_reset",value);
  programIOBRegister("ASIC_reset");

  if(actSetup==MANNHEIM){
    setASICReset_TestSystem(reset);
  }
}


void DSSC_PPT::ASICJtagWrite(int chainNumber, vector<uint8_t> data)
{
  stringstream ss;
  ss.width(2);
  ss << hex;
  //setJTAGEngineEnabled(chainNumber, true);
  ss << "JTGC " + to_string(chainNumber) + " ";
  for (uint i=0; i<data.size(); ++i)
  {
    ss << data.at(i);
  }
  dataString = ss.str();

  ERROR("Check if correct: " << dataString);
  //sendReadPacket();

}


uint32_t DSSC_PPT::getIprogCycleLength()
{
  uint32_t cycle_length = sequencer->getCycleLength();
  if(burstParams["single_cap_en"]){
    cycle_length *=2;
  }
  return cycle_length-1;
}

uint32_t DSSC_PPT::getBurstCycleLength()
{
  uint32_t cycle_length = sequencer->getRealCycleLength();
  if(burstParams["single_cap_en"]){
    cycle_length *=2;
  }
  return cycle_length-1;
}


void DSSC_PPT::initEPCRegsFromJTAGConfigFile()
{
  uint cycle_length = getIprogCycleLength();

  uint hold_length  = (sequencer->getHoldEnabled())? sequencer->getHoldCnts() : 0;
  uint value = jtagRegisters->getSignalValue("Master FSM Config Register","0","Burst Length");
  jtagRegisters->setSignalValue("Master FSM Config Register","all","Burst Length",value);
  epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","burst_length",value);

  jtagRegisters->setSignalValue("Master FSM Config Register","all","Cycle Length",cycle_length);
  epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","cycle_length",cycle_length);
  epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","hold_length",hold_length);

  value = jtagRegisters->getSignalValue("Master FSM Config Register","0","Iprog Length");
  jtagRegisters->setSignalValue("Master FSM Config Register","all","Iprog Length",value);
  epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","iprog_length",value);

  value = jtagRegisters->getSignalValue("Master FSM Config Register","0","Refpulse Length");
  jtagRegisters->setSignalValue("Master FSM Config Register","all","Refpulse Length",value);
  epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","refpulse_length",value+1);

  value = jtagRegisters->getSignalValue("Master FSM Config Register","0","Bypass Refpulse");
  jtagRegisters->setSignalValue("Master FSM Config Register","all","Bypass Refpulse",value);
  epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","bypass_refpulse",value);

  value = jtagRegisters->getSignalValue("SRAM Controller Config Register","0","Veto Latency");
  if(value == 0){
    WARN_OUT("VetoLatency was 0 changed to 80");
    value = 80;
  }
  jtagRegisters->setSignalValue("SRAM Controller Config Register","all","Veto Latency",value);
  epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","veto_latency",value);

  uint16_t testPattern = jtagRegisters->getSignalValue("Serializer Config Register","0","Serializer Output Test Pattern");
  jtagRegisters->setSignalValue("Serializer Config Register","all","Serializer Output Test Pattern",testPattern);
  epcRegisters->setSignalValue("DataRecv_to_Eth0_Register","all","expected_test_pattern",testPattern);

  uint32_t pre_burst_wait_time = epcRegisters->getSignalValue("CMD_PROT_ENGINE","0","pre_burst_wait_time");
  uint32_t iob_powerup_time    = epcRegisters->getSignalValue("CMD_PROT_ENGINE","0","iob_powerup_time");

  if(iob_powerup_time >=  pre_burst_wait_time){
    ERROR_OUT("Pre burst wait time smaller than 10. This can cause unexpected behavior!");
  }
}


void DSSC_PPT::initIOBRegsFromJTAGConfigFile()
{
  uint value = jtagRegisters->getSignalValue("Master FSM Config Register","0","Burst Length");
  iobRegisters->setSignalValue("SYSFSM_burst_length","all","SYSFSM_burst_length",value);

  value = getIprogCycleLength();
  iobRegisters->setSignalValue("SYSFSM_iprog_cyclelength","all","SYSFSM_iprog_cyclelength",value);

  value = jtagRegisters->getSignalValue("Master FSM Config Register","0","Iprog Length");
  iobRegisters->setSignalValue("SYSFSM_iprog_length","all","SYSFSM_iprog_length",value);

  value = jtagRegisters->getSignalValue("Master FSM Config Register","0","Refpulse Length");
  iobRegisters->setSignalValue("SYSFSM_refpulse_length","all","SYSFSM_refpulse_length",value);

  value = jtagRegisters->getSignalValue("Master FSM Config Register","0","Bypass Refpulse");
  iobRegisters->setSignalValue("SYSFSM_refpulse_length","all","SYSFSM_bypass_refpulse",value);

  value = getBurstCycleLength();
  iobRegisters->setSignalValue("SYSFSM_burst_cyclelength","all","SYSFSM_burst_cyclelength",value);

  uint32_t iob_powerup_time    = epcRegisters->getSignalValue("CMD_PROT_ENGINE","0","iob_powerup_time");
  uint32_t pre_burst_wait_time = epcRegisters->getSignalValue("CMD_PROT_ENGINE","0","pre_burst_wait_time");
  uint32_t power_on_delay      = iobRegisters->getSignalValue("PRB_power_on_delay","1","PRB_power_on_delay");

  if(pre_burst_wait_time < 10){
    ERROR_OUT("EPCRegs: pre_burst_wait_time smaller than 10. This can cause unexpected behavior!");
   // pre_burst_wait_time = 100;
   // epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","pre_burst_wait_time",pre_burst_wait_time);
  }

  if(iob_powerup_time+5>pre_burst_wait_time){
    iob_powerup_time = pre_burst_wait_time-5;
    ERROR_OUT("EPCRegs: iob_powerup_time to large set to " + to_string(iob_powerup_time));
    epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","iob_powerup_time",iob_powerup_time);
  }

  if(iob_powerup_time + power_on_delay + 5 >= pre_burst_wait_time){
    power_on_delay = pre_burst_wait_time-5-iob_powerup_time;
    ERROR_OUT("IOBRegs: PRB_power_on_delay too large for wait and powerup time. Set to max = " + to_string(power_on_delay));
  //  iobRegisters->setSignalValue("PRB_power_on_delay","all","PRB_power_on_delay",power_on_delay);
  }
}


void DSSC_PPT::setBurstLength(int burstLength)
{
  jtagRegisters->setSignalValue("Master FSM Config Register","all","Burst Length",burstLength);
}


void DSSC_PPT::setCycleLength(int cylceLength)
{
  sequencer->setCycleLength(cylceLength);
}


void DSSC_PPT::setIProgLength(int iprogLength)
{
  jtagRegisters->setSignalValue("Master FSM Config Register","all","Iprog Length",iprogLength);
}


void DSSC_PPT::enableHolds(bool enable)
{
  sequencer->setHoldEnabled(enable);
}

void DSSC_PPT::updateStartWaitOffset(int start_wait_offs)
{
  int iprog_length     = jtagRegisters->getSignalValue("Master FSM Config Register","0","Iprog Length") + 1;
  int refpulse_length  = jtagRegisters->getSignalValue("Master FSM Config Register","0","Refpulse Length") + 1;
  bool bypass_refpulse = jtagRegisters->getSignalValue("Master FSM Config Register","0","Bypass Refpulse") == 1;

  int iprog_cycle_length = jtagRegisters->getSignalValue("Master FSM Config Register","0","Cycle Length") + 1;

  burstParams["start_wait_offs"] = start_wait_offs;

  const int start_wait_time = burstParams["start_wait_time"];  // default = 15000 ??s

  // pre_burst_wait_time is 15 ms after start burst signal:
  // cycle count is 10,0719 ns
  // time to switch on powers with 4 PRBs: 5.6??s
  const double IPROGCNTS      = iprog_length*iprog_cycle_length +
                                (bypass_refpulse? 0 : refpulse_length);    //200*30

  const int pre_burst_wait_time = start_wait_time * MICROSECOND - IPROGCNTS + start_wait_offs;  // 15 ms

  epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","pre_burst_wait_time",pre_burst_wait_time);

  programEPCRegister("CMD_PROT_ENGINE");
  DEBUG_OUT("start_wait_offs set to " <<  start_wait_offs);
}


void DSSC_PPT::updateCounterValues(int start_wait_time, int start_wait_offs, int gdps_on_time, int iprogLength, int burstLength, int refpulseLength,
                                   int fetOnTime, int clrOnTime, int clrCycle, int clrDuty, int powerOnTime, int iprog_clr_duty, int iprog_clr_offset,
           int iprog_clr_en, int single_cap_en)
{
  const map<string,int> mapReminder = burstParams;

  burstParams["start_wait_time"]  = start_wait_time;
  burstParams["start_wait_offs"]  = start_wait_offs;
  burstParams["gdps_on_time"]     = gdps_on_time;
  burstParams["fet_on_time"]      = fetOnTime;
  burstParams["clr_on_time"]      = clrOnTime;
  burstParams["clr_cycle"]        = clrCycle;
  burstParams["SW_PWR_ON"]        = powerOnTime;
  burstParams["iprog_clr_en"]     = iprog_clr_en;
  burstParams["single_cap_en"]    = single_cap_en;
  burstParams["iprog_clr_duty"]   = iprog_clr_duty;
  burstParams["iprog_clr_offset"] = iprog_clr_offset;

  if(clrDuty < sequencer->getRealCycleLength()){
    iobRegisters->setSignalValue("CLR_duty","all","CLR_duty",clrDuty);
  }else{
    ERROR_OUT("CLR_duty is larger than cycle length. Clear duty cycle was not updated!");
  }

  {
    ContModeKeeper keeper(this);
    jtagRegisters->setSignalValue("Master FSM Config Register","all","Burst Length",burstLength);
    jtagRegisters->setSignalValue("Master FSM Config Register","all","Iprog Length",iprogLength);
    jtagRegisters->setSignalValue("Master FSM Config Register","all","Refpulse Length",refpulseLength);

    const bool success = updateAllCounters();
    if(!success){
      burstParams = mapReminder;
    }
  }
}


bool DSSC_PPT::initCycleWaitCounters()
{
  DEBUG_OUT("+++Set Init cycle wait counters to default values");

  int burst_length     = jtagRegisters->getSignalValue("Master FSM Config Register","0","Burst Length") + 1;
  int iprog_length     = jtagRegisters->getSignalValue("Master FSM Config Register","0","Iprog Length") + 1;
  int refpulse_length  = jtagRegisters->getSignalValue("Master FSM Config Register","0","Refpulse Length") + 1;
  bool bypass_refpulse = jtagRegisters->getSignalValue("Master FSM Config Register","0","Bypass Refpulse") == 1;

  int burst_cycle_length = getBurstCycleLength()+1;

  int iprog_cycle_length = jtagRegisters->getSignalValue("Master FSM Config Register","0","Cycle Length") + 1;

  const int start_wait_time  = burstParams["start_wait_time"];  // default = 15000 ?s
  const int start_wait_offs  = burstParams["start_wait_offs"];  // default = 15000 ?s
  const int gdps_on_time     = burstParams["gdps_on_time"];     // default = 10000 ?s
  const int fet_on_time      = burstParams["fet_on_time"];      // default =     2 ?s before iprog
  const int clr_on_time      = burstParams["clr_on_time"];      // default =     2 ?s
  const int clr_cycle        = burstParams["clr_cycle"];        // default =     2 ?s
  const int iprog_clr_en     = burstParams["iprog_clr_en"];     // default =     0
  const int iprog_clr_offset = burstParams["iprog_clr_offset"]; // default =     0
  int iprog_clr_duty         = burstParams["iprog_clr_duty"];   // default =     0


  const int SW_PWR_ON       = burstParams["SW_PWR_ON"];        // default =    20 ?s

  // pre_burst_wait_time is 15 ms after start burst signal:
  // cycle count is 10,0719 ns
  // time to switch on powers with 4 PRBs: 5.6?s
  const double IPROGCNTS      = iprog_length*iprog_cycle_length +
                                (bypass_refpulse? 0 : refpulse_length);    //200*30


  const int pre_burst_wait_time = start_wait_time * MICROSECOND - IPROGCNTS + start_wait_offs;  // 15 ms
  const int iob_wait_time       = start_wait_time - gdps_on_time;
  const int iob_powerup_time    = iob_wait_time * MICROSECOND - PRBPROGTIME + start_wait_offs;  //  5 ms - 5.6?s
  const int power_on_delay      = (start_wait_time - iob_wait_time - SW_PWR_ON)*MICROSECOND - IPROGCNTS;  // 2 ?s before Iprog = 15 - IProg - 2?s
  const int gdps_on_delay       = 5;       //  50 ns after iob_powerup_time 10 ms before burst

  bool ok = true;
  ok &= pre_burst_wait_time>0;
  ok &= iob_powerup_time>0;
  ok &= power_on_delay>0;

  ok |= simulationMode;
  if(ok)
  {
#ifdef DEBUG
    cout << "pre_burst_wait_time: " << pre_burst_wait_time << " BURST 15ms after Start signal" << endl; cout.width(21);
    cout << "power_on_delay: "      << power_on_delay      << " En SW_PWR 2?s before IPROG"    << endl; cout.width(21);
    cout << "gdps_on_time: "        << gdps_on_time        << " En GDPS 10ms before BURST"     << endl; cout.width(21);
    cout << "gdps_on_delay: "       << gdps_on_delay       << endl;
#endif

    epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","pre_burst_wait_time",pre_burst_wait_time);
    epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","iob_powerup_time",iob_powerup_time);

    iobRegisters->setSignalValue("PRB_power_on_delay","all","PRB_power_on_delay",power_on_delay);
    iobRegisters->setSignalValue("PRB_GDPS_delay","all","PRB_GDPS_delay",gdps_on_delay);

  //   int max_power_on_time = iprog_length * iprog_cycle_length + 2700 * cycle_length + refpulse_length + 2 * MICROSECOND;
    int max_power_on_time = 2000 * MICROSECOND;
    iobRegisters->setSignalValue("PRB_max_power_on_time","all","PRB_max_power_on_time",max_power_on_time);

    //FET Driver signals

    int fet_off_delay = MICROSECOND;  //1 us after burst
    int fet_on_delay  = (gdps_on_time-fet_on_time)*MICROSECOND - IPROGCNTS + FETOFFSET; // 2 us before iprog
   // int fet_off_delay_s = MICROSECOND;  //1 us after burst - kalavaku
   // int fet_on_delay_s  = (gdps_on_time-fet_on_time)*MICROSECOND - IPROGCNTS + FETOFFSET; // 2 us before iprog - kalavaku


    // time after start_iob to start clear fsm (clrdis = 1)
    int clr_preclear_delay  = (gdps_on_time-clr_on_time)*MICROSECOND + DISOFFSET - IPROGCNTS;

    //positioning of clr and clr gate in small steps
    int clr_on_offset       = 0; // clr_on_offset       = 2 (BY DESY)
    int clr_off_offset      = 0; // clr_off_offset      = 2 (BY DESY)
    int clr_gate_on_offset  = 1; // clr_gate_on_offset  = 2 (BY DESY)
    int clr_gate_off_offset = 3; // clr_gate_off_offset = 2 (BY DESY)

    //clr cycle counter stops for one cycle and counts to 0. So set counter to cycle_length-2
    int clr_period = sequencer->getRealCycleLength()-2;

    //a clr must be assigned before iprog to settle DEPFET dark current
    int iprog_clr_delay =  iprog_clr_offset + 29;

    if(iprog_clr_en){
      iprog_clr_duty = iprog_clr_delay + IPROGCNTS;
    }else
    {
      int max_iprog_clr_cnts = clr_on_time*MICROSECOND-20;
      if(iprog_clr_delay+iprog_clr_duty > max_iprog_clr_cnts)
      {
        WARN_OUT("iprog_clr_delay too large (" << iprog_clr_delay << "). IProg Clear exceeds available space before IProg: set to = " << (max_iprog_clr_cnts - iprog_clr_duty));
        iprog_clr_delay = max_iprog_clr_cnts - iprog_clr_duty;
      }
    }

    if(iprog_clr_delay<0)
    {
      WARN_OUT("DSSC_PPT Warning: iprog_clr_delay < 0 : " <<  iprog_clr_delay);
      iprog_clr_delay = 0;
    }


    //int clr_duty   = 5; // clear length = 5 (BY DESY)

    //iobRegisters->setSignalValue("FET_controller_enable","all","FET_controller_enable",1);

    iobRegisters->setSignalValue("FET_source_off_delay","all","FET_source_off_delay",fet_off_delay);
    iobRegisters->setSignalValue("FET_gate_off_delay","all","FET_gate_off_delay",fet_off_delay+MICROSECOND*4); // kalavaku

    iobRegisters->setSignalValue("FET_source_on_delay","all","FET_source_on_delay",fet_on_delay);
    iobRegisters->setSignalValue("FET_gate_on_delay","all","FET_gate_on_delay",fet_on_delay+MICROSECOND*4); // kalavaku

    iobRegisters->setSignalValue("CLR_preclear_delay","all","CLR_preclear_delay",clr_preclear_delay);
    iobRegisters->setSignalValue("CLR_on_offset","all","CLR_on_offset",clr_on_offset);
    iobRegisters->setSignalValue("CLR_off_offset","all","CLR_off_offset",clr_off_offset);
    iobRegisters->setSignalValue("CLR_gate_on_offset","all","CLR_gate_on_offset",clr_gate_on_offset);
    iobRegisters->setSignalValue("CLR_gate_off_offset","all","CLR_gate_off_offset",clr_gate_off_offset);
    iobRegisters->setSignalValue("CLR_period","all","CLR_period",clr_period);
    //iobRegisters->setSignalValue("CLR_duty","all","CLR_duty",clr_duty);
    iobRegisters->setSignalValue("IProg_clr_delay","all","IProg_clr_delay",iprog_clr_delay);
    iobRegisters->setSignalValue("IProg_clr_duty","all","IProg_clr_duty",iprog_clr_duty);


    // iprog + burst + refpulse + 2 ?s
    int clrdis_duty     = (burst_length * burst_cycle_length) + clr_on_time * MICROSECOND + CLROFFSET + IPROGCNTS + 2*MICROSECOND;

    //iobRegisters->setSignalValue("CLR_clr_control_en","all","CLR_clr_control_en",1);

    //duty cycle of clrdis signal
    iobRegisters->setSignalValue("CLR_clrdis_duty","all","CLR_clrdis_duty",clrdis_duty);

    // time after clrdis to first clr/clrgate
    // set manually to define correct place
    // for 4.5 MHz mode + 40 ns to set before first integration

    const int clrdis_en_delay = clr_on_time * MICROSECOND + CLROFFSET + IPROGCNTS + clr_cycle;
    iobRegisters->setSignalValue("CLR_clrdis_en_delay","all","CLR_clrdis_en_delay",clrdis_en_delay);

  }else{
    ERROR_OUT("################Could not set Cycle Counters, values do not fit together");
    ERROR_OUT("#### pre_burst_wait_time = " + to_string(pre_burst_wait_time));
    ERROR_OUT("#### iob_powerup_time = " + to_string(iob_powerup_time));
    ERROR_OUT("#### power_on_delay = " + to_string(power_on_delay));
  }

  /* 2017-02-01
   * IPROG in PPT is identical to ASIC now
   * BURST in PPT comes 10ns before start of burst in ASIC,
   *       because one cycle is required in ASIC to switch state
   *       ends at memory full.
   * ASIC_FSM_BURST starts with PPT BURST.
   *                Ends with Burstlength + VetoLatency
   * CYCLE_DONE starts counting with IPROG and ends with ASIC_FSM_BURST end
   * ClrCycle = 1 means 2 cycles after IProg/Burst in PPT,
   *              and 1 slowcycle in ASIC sequence
   *2017-03-22
   * Fix in firmware DEBUG out Iprog was misaligned when Holds are enabled
   * IPROG BURST AND CLEAR signals all checked with holds, worked nicely
   */

  return ok;
}



bool DSSC_PPT::updateAllCounters()
{
  initEPCRegsFromJTAGConfigFile();
  initIOBRegsFromJTAGConfigFile();

  bool ok = initCycleWaitCounters();

  if(!isHardwareReady()){
    return true;
  }

  if(ok)
  {
    programEPCRegister("CMD_PROT_ENGINE");

    if(activeIOBs.size()>0 || simulationMode){

      for(const auto & module : activeIOBs){
        currentModule = module;

       DEBUG_OUT("Update IOB and JTAG Counters of module "<< currentModule);

        if(!simulationMode){
          programIOBRegister("PRB_power_on_delay");
          programIOBRegister("PRB_GDPS_delay");
          programIOBRegister("FET_controller_enable");
          programIOBRegister("FET_source_off_delay");
          programIOBRegister("FET_gate_off_delay");
          programIOBRegister("FET_source_on_delay");
          programIOBRegister("FET_gate_on_delay");
          programIOBRegister("CLR_preclear_delay");
          programIOBRegister("CLR_on_offset");
          programIOBRegister("CLR_off_offset");
          programIOBRegister("CLR_gate_on_offset");
          programIOBRegister("CLR_gate_off_offset");
          programIOBRegister("CLR_period");
          programIOBRegister("CLR_duty");
          programIOBRegister("CLR_clr_control_en");
          programIOBRegister("CLR_clrdis_duty");
          programIOBRegister("CLR_clrdis_en_delay");
          programIOBRegister("IProg_clr_delay");
          programIOBRegister("IProg_clr_duty");
        }
        programIOBRegister("SYSFSM_burst_length");
        programIOBRegister("SYSFSM_iprog_cyclelength");
        programIOBRegister("SYSFSM_iprog_length");
        programIOBRegister("SYSFSM_refpulse_length");
        programIOBRegister("SYSFSM_refpulse_length");
        programIOBRegister("SYSFSM_burst_cyclelength");

        programJtagSingle("SRAM Controller Config Register");
        programJtagSingle("Master FSM Config Register");
      }

      cout << "All System Counters updated" << endl;
    }else{
      ok = false;
      cout << "IOB not programmed" << endl;
    }
  }
  return ok;
}

void DSSC_PPT::setASICReset_TestSystem(bool reset)
{
  uint value = (reset)? 0u : 1u;

  epcRegisters->setSignalValue("JTAG_Control_Register","0","RESET_ASIC1_N",value);
  programEPCRegister("JTAG_Control_Register");
}


// after Reset most registers change value, normally the current configuration in the local variables is valid
// and should overwrite the registers during initIOB and programEPCRegisters,
// during initSystem this different:
// JTAG Engine should be disabled at first, PRB powers switched off, and ASIC Reset set
// to not enable these values at the wrong time the registers have to be set manually after reset
void DSSC_PPT::resetLocalRegisters()
{
  if(getEPCParam("Single_Cycle_Register","all","iterations") != 1){
    WARN_OUT("++++ WARNING: set epc param iterations for single cycle to 1");
    setEPCParam("Single_Cycle_Register","all","iterations",1);
  }

  initEPCRegsAfterReset();
  initIOBRegsAfterReset();

  initEPCRegsFromJTAGConfigFile();
  initIOBRegsFromJTAGConfigFile();
}



void DSSC_PPT::resetCurrentModule(bool reset)
{
  DEBUG_OUT("++++ Reset Current Module: " << currentModule << " : " << reset);

  epcRegisters->setSignalValue("Multi_purpose_Register","0","IOB_RESET"+ to_string(currentModule),((reset)? 1:0 ));
  epcRegisters->setSignalValue("Ethernet_Reset_Register","0","Ethernet Reset channel "+ to_string(currentModule-1),((reset)? 1:0 ));

  iobRegisters->setSignalValue("Aurora_Reset",to_string(currentModule),"Aurora_Reset",((reset)? 1:0 ));
  iobRegisters->setSignalValue("ASIC_reset",to_string(currentModule),"ASIC_reset",((reset)? 1:0 ));
  iobRegisters->setSignalValue("ASIC_reset",to_string(currentModule),"ASIC_pll_reset",((reset)? 1:0 ));

  programEPCRegister("Multi_purpose_Register");
  programEPCRegister("Ethernet_Reset_Register");

  programIOBRegister("Aurora_Reset");
  programIOBRegister("ASIC_reset");

  if(reset)
  {
    iobRegisters->setSignalValue("PRB_control",to_string(currentModule),"PRB_en",0);
    //iobRegisters->setSignalValue("PRB_control",to_string(currentModule),"PRB_self_detect",1);
  }
  else
  {
    initEPCRegsAfterReset();
    initModuleIOBRegsAfterReset();

    initEPCRegsFromJTAGConfigFile();
    initIOBRegsFromJTAGConfigFile();

    resetDataFailRegister();
  }
}


//trigger new check of available IOBs
void DSSC_PPT::resetAll(bool reset)
{
  if (reset){
    activeIOBs.clear();
    iobsNotChecked = true;
    dataString = "RSTA 1";
  }else{
    dataString = "RSTA 0";
  }

  DEBUG_OUT("++++ Reset All: " << reset);

  sendReadPacket();

  //to hold epc registers up to date when using fast direct commands
  epcRegisters->setSignalValue("Multi_purpose_Register","0","EPC_devices_reset",((reset)? 1:0 ));
  epcRegisters->setSignalValue("Multi_purpose_Register","0","ddr3_reset",((reset)? 1:0 ));
  epcRegisters->setSignalValue("Multi_purpose_Register","0","IOB_RESET1",((reset)? 1:0 ));
  epcRegisters->setSignalValue("Multi_purpose_Register","0","IOB_RESET2",((reset)? 1:0 ));
  epcRegisters->setSignalValue("Multi_purpose_Register","0","IOB_RESET3",((reset)? 1:0 ));
  epcRegisters->setSignalValue("Multi_purpose_Register","0","IOB_RESET4",((reset)? 1:0 ));

  epcRegisters->setSignalValue("Ethernet_Reset_Register","0","Ethernet Reset channel 0",((reset)? 1:0 ));
  epcRegisters->setSignalValue("Ethernet_Reset_Register","0","Ethernet Reset channel 1",((reset)? 1:0 ));
  epcRegisters->setSignalValue("Ethernet_Reset_Register","0","Ethernet Reset channel 2",((reset)? 1:0 ));
  epcRegisters->setSignalValue("Ethernet_Reset_Register","0","Ethernet Reset channel 3",((reset)? 1:0 ));

  epcRegisters->setSignalValue("AuroraRX_Control","0","rx_aurora_reset_in",((reset)? 1:0 ));

  iobRegisters->setSignalValue("Aurora_Reset","all","Aurora_Reset",((reset)? 1:0 ));
  iobRegisters->setSignalValue("ASIC_reset","all","ASIC_reset",((reset)? 1:0 ));

  if(reset){
    runContinuousMode(false);

    resetLocalRegisters();

    // programEPCRegister("Multi_purpose_Register"); // is programmed also in runContinuousMode
    programEPCRegister("JTAG_Control_Register");
  }else{

    setEPCParam("CLOCK_FANOUT_CONTROL","0","PLL_DRP_RST",0);
    setEPCParam("CLOCK_FANOUT_CONTROL","0","PLL_RST",0);

    programEPCRegisters();

    resetDataFailRegister();
  }


}


void DSSC_PPT::datapathReset(bool reset)
{
  if (reset)
    dataString = "RSTD 15";
  else
    dataString = "RSTD 0";

  sendReadPacket();

  //to hold epc registers up to date when using fast direct commands
  epcRegisters->setSignalValue("Ethernet_Reset_Register","0","Ethernet Reset channel 0",((reset)? 1:0 ));
  epcRegisters->setSignalValue("Ethernet_Reset_Register","0","Ethernet Reset channel 1",((reset)? 1:0 ));
  epcRegisters->setSignalValue("Ethernet_Reset_Register","0","Ethernet Reset channel 2",((reset)? 1:0 ));
  epcRegisters->setSignalValue("Ethernet_Reset_Register","0","Ethernet Reset channel 3",((reset)? 1:0 ));

  epcRegisters->setSignalValue("AuroraRX_Control","0","rx_aurora_reset_in",((reset)? 1:0 ));

  epcRegisters->setSignalValue("Multi_purpose_Register","0","ddr3_reset",((reset)? 1:0 ));

  iobRegisters->setSignalValue("Aurora_Reset","all","Aurora_Reset",((reset)? 1:0 ));

  programEPCRegister("Multi_purpose_Register");
}


int DSSC_PPT::checkAllASICPllLocked()
{
  int ret = ERROR_OK;
  for(uint i=0; i<numAvailableIOBs(); i++){
    setActiveModule(activeIOBs.at(i));
    ret |= checkASICPllLocked();
  }
  return ret;
}


int DSSC_PPT::checkASICPllLocked()
{
  int cnt = 0;
  while(!isAsicPllLocked() && cnt<5){
    iobAsicPllReset(true);
    usleep(10000);
    iobAsicPllReset(false);
    usleep(100000);
    cnt++;
  }

  if(cnt == 5){
    ERROR_OUT("DSSC_PPT: Module "+ to_string(currentModule) + ": ASIC PLL of could not be locked!");
    return ERROR_ASIC_PLL_NOT_LOCKED;
  }else{
    DEBUG_OUT("DSSC_PPT: Module "+ to_string(currentModule) + ": ASIC PLL locked after " << cnt << " tries");
  }
  return ERROR_OK;
}


void DSSC_PPT::iobAsicPllReset(bool reset)
{
  int value = (reset)? 1 : 0;

  iobRegisters->setSignalValue("ASIC_reset",to_string(currentModule),"ASIC_pll_reset",value);
  programIOBRegister("ASIC_reset");
}


//trigger new check of available IOBs
void DSSC_PPT::iobReset(bool reset)
{
  if (reset){
    runContinuousMode(false);

    activeIOBs.clear();
    iobsNotChecked = true;

    dataString = "RSTI 15";
  }else{
    dataString = "RSTI 0";
  }

  sendReadPacket();

  //to hold epc registers up to date when using fast direct commands
  epcRegisters->setSignalValue("Multi_purpose_Register","0","IOB_RESET1",((reset)? 1:0 ));
  epcRegisters->setSignalValue("Multi_purpose_Register","0","IOB_RESET2",((reset)? 1:0 ));
  epcRegisters->setSignalValue("Multi_purpose_Register","0","IOB_RESET3",((reset)? 1:0 ));
  epcRegisters->setSignalValue("Multi_purpose_Register","0","IOB_RESET4",((reset)? 1:0 ));

  if(reset){
    initIOBRegsAfterReset();
  }
}


void DSSC_PPT::epcReset(bool reset)
{
  if (reset){
    dataString = "RSTE 1";
  }else{
    dataString = "RSTE 0";
  }

  sendReadPacket();

  epcRegisters->setSignalValue("Multi_purpose_Register","0","EPC_devices_reset",((reset)? 1:0 ));

  initEPCRegsAfterReset();
}

void DSSC_PPT::cloneEth0(bool en)
{
  epcRegisters->setSignalValue("DataRecv_to_Eth0_Register","0","clone_eth0_to_eth1",((en)? 1:0 ));
  programEPCRegister("DataRecv_to_Eth0_Register");
}

bool DSSC_PPT::isCloneEth0Enabled()
{
  return epcRegisters->getSignalValue("DataRecv_to_Eth0_Register","0","clone_eth0_to_eth1") == 1;
}



void DSSC_PPT::toggleLMKGlobalEn()
{
  programLMKGlobal(true);
  usleep(100000);
  programLMKGlobal(false);

  DEBUG_OUT("Toggle LMK Global Enable");
}


void DSSC_PPT::programLMKs()
{
  enableLMK();
  for(uint i=0; i<numAvailableIOBs(); i++){
    currentModule = activeIOBs.at(i);
    programLMK();
  }
}

void DSSC_PPT::programLMKsDefault()
{ // double keeper to avoid multiple switching
  CheckSendingKeeper keeper(this);
  for(uint i=0; i<numAvailableIOBs(); i++){
    currentModule = activeIOBs.at(i);
    programLMKDefault();
    //checkCorrectASICReset();
  }
}


void DSSC_PPT::disableLMKs()
{
  disableLMK();
  for(uint i=0; i<numAvailableIOBs(); i++){
    currentModule = activeIOBs.at(i);
    programLMK();
  }
}


void DSSC_PPT::disableLMK()
{
  lmkFastSettings.disableAll();
  lmkSlowSettings.disableAll();
}


void DSSC_PPT::enableLMKs()
{
  enableLMK();
  for(uint i=0; i<numAvailableIOBs(); i++){
    currentModule = activeIOBs.at(i);
    programLMK();
  }
}


void DSSC_PPT::setDefaultLMKConfig()
{
  lmkFastSettings.setDefault();
  lmkSlowSettings.setDefault();
}



void DSSC_PPT::enableLMK()
{
  lmkFastSettings.enableAll();
  lmkSlowSettings.enableAll();
}


void DSSC_PPT::programLMK()
{
  programLMK(true);
  programLMK(false);
}


void DSSC_PPT::programLMK(bool fast)
{
  const auto & lmkSettings = (fast)? lmkFastSettings : lmkSlowSettings;

  for(const auto & setting : lmkSettings){
    programLMKOut(setting);
  }

  programLMKGlobal(false);
}


void DSSC_PPT::programLMKGlobal(bool disable)
{
  vector<uint32_t> lmk_data_vec;
  lmk_data_vec.push_back(LMK_REG9);
  if(disable){
    lmk_data_vec.push_back(LMK_REG14R);
  }else{
    lmk_data_vec.push_back(LMK_REG14N);
  }

  programLMK(0,lmk_data_vec);
}


void DSSC_PPT::setLMKConfig(int asic, bool fast, int div, int delay, int mux, bool disable)
{
  auto & lmkSetting = (fast)? lmkFastSettings[asic] : lmkSlowSettings[asic];

  lmkSetting.div   = div;
  lmkSetting.delay = delay;
  lmkSetting.mux   = mux;
  lmkSetting.en    = (disable? 0 : 1);
}


void DSSC_PPT::programLMKOut(const LMKClockSettings & lmkSettings)
{
  uint32_t lmk_data   = lmkSettings.getRegData();
  uint32_t lmk_number = lmkSettings.lmk_num;

  lmkSettings.print();

  //fist disable and then reenable the lmk
  iobRegisters->setSignalValue("LMK_data",to_string(currentModule),"LMK_data",lmk_data);
  iobRegisters->setSignalValue("LMK_control",to_string(currentModule),"LMK_en",1);
  iobRegisters->setSignalValue("LMK_control",to_string(currentModule),"LMK_dev_sel",lmk_number);
  iobRegisters->setSignalValue("LMK_control",to_string(currentModule),"LMK_valid",1);

  programIOBRegister("LMK_data");
  programIOBRegister("LMK_control");
}


void DSSC_PPT::programLMKASIC(int asic, bool fast, bool disable)
{
  DEBUG_OUT("Module "<< currentModule << " Program LMK ASIC " << asic << (fast?" fast":" slow"));
  //   LMK1 - C_CLK1 FAST
  //   LMK2 -  XCLK1 SLOW
  //   LMK3 -  XCLK2 SLOW
  //   LMK4 - C_CLK2 FAST
  auto & lmkSetting = (fast)? lmkFastSettings[asic] : lmkSlowSettings[asic];

  lmkSetting.en = false;
  programLMKOut(lmkSetting);

  if(!disable)
  {
    usleep(20000);
    lmkSetting.en = true;
    programLMKOut(lmkSetting);
  }

  iobRegisters->clearCompareErrors();
}

void DSSC_PPT::programLMKPowerDown(int lmk_number)
{
  vector<uint32_t> lmk_data_vec(1,LMK_REG14R);
  programLMK(lmk_number,lmk_data_vec);
}


void DSSC_PPT::programLMKDefault()
{
  /** LMK Programming Sequence **/
  /*
    * LMK_Number 1 - 4
    * LMK_CTRL address = 0x100
    * LMK_DATA address = 0x104
    *
    * 1. Enable LMK_Controller LMK_CTRL[0]
    * 2. Set Valid bit         LMK_CTRL[1]
    * 3. Select LMK one hot    LMK_CTRL[5-2]
    *
    * + If Enable bit is toggled from 0 to 1 a reset is send
    *   If one LMK is selected it is resetted
    * + When Valid bit is set, data from datareg is programmed to LMK
    *   After data is programmed Valid bit is set to 0 automatically
    *   Next data is sent when Valid bit is reset by user.
    * + To reset LMK before programming set LMK_CTRL to
    *   0x00 before next LMK is programmed
    *
    * + Select LMK Register to Program:
    *   data + REGNr , e.g to program R4 set data = data + 4;
    */
  vector<uint32_t> lmk_data_vec;

  //lmk_data_vec.push_back(LMK_RES_0); // RES_0 is programmed automatically by firmware module

  uint32_t data = lmkFastSettings.front().getRegData() & 0xFFFFFFF0;

  for(int i=0; i<8; i++){
    lmk_data_vec.push_back(data + i);
  }
  lmk_data_vec.push_back(LMK_REG9);
  lmk_data_vec.push_back(LMK_REG14N);
  //    lmk_data_vec.push_back(LMK_REG14D);
  //    lmk_data_vec.push_back(LMK_REG14N);

  programLMK(0,lmk_data_vec);
}

void DSSC_PPT::programLMK(int lmk_number, const vector<uint32_t> & lmk_data_vec)
{
  if(lmk_number == 0){
    DEBUG_OUT("Program all LMKs at once ");
    lmk_number = 0xF;
  }else{
    DEBUG_OUT("Program LMK " << lmk_number);

    lmk_number = 1<<(lmk_number-1);
  }


  iobRegisters->setSignalValue("LMK_control",to_string(currentModule),"LMK_dev_sel",lmk_number);
  if(lmk_data_vec.size()>5){ // adds a reset to the programming sequence
    iobRegisters->setSignalValue("LMK_control",to_string(currentModule),"LMK_en",0);
  }
  programIOBRegister("LMK_control");

  //deselect enable reset before programming
  iobRegisters->setSignalValue("LMK_control",to_string(currentModule),"LMK_en",1);

  for (const auto & lmk_data : lmk_data_vec){
    iobRegisters->setSignalValue("LMK_data",to_string(currentModule),"LMK_data",lmk_data);
    iobRegisters->setSignalValue("LMK_control",to_string(currentModule),"LMK_valid",1);

    programIOBRegister("LMK_data");
    programIOBRegister("LMK_control");
  }

  iobRegisters->clearCompareErrors();
}


void DSSC_PPT::clockPLLSelect(bool intNotExt)
{
  //default is extern
  if(intNotExt){
    epcRegisters->setSignalValue("CLOCK_FANOUT_CONTROL","0","CLOUT_SEL",1);
    epcRegisters->setSignalValue("Multi_purpose_Register","0","PLL_CE",0);
  }else{
    epcRegisters->setSignalValue("CLOCK_FANOUT_CONTROL","0","CLOUT_SEL",0);
    epcRegisters->setSignalValue("Multi_purpose_Register","0","PLL_CE",1);
  }
  programEPCRegister("CLOCK_FANOUT_CONTROL");
  programEPCRegister("Multi_purpose_Register");
}


void DSSC_PPT::clockSourceSelect(bool xfelNotStandalone)
{
  //default is standalone
  if(xfelNotStandalone){
    epcRegisters->setSignalValue("CLOCK_FANOUT_CONTROL","0","CLIN_SEL",0);
    epcRegisters->setSignalValue("CLOCK_FANOUT_CONTROL","0","PLL_CLKINSEL",0);
  }else{
    epcRegisters->setSignalValue("CLOCK_FANOUT_CONTROL","0","CLIN_SEL",1);
    epcRegisters->setSignalValue("CLOCK_FANOUT_CONTROL","0","PLL_CLKINSEL",1);
  }
  programEPCRegister("CLOCK_FANOUT_CONTROL");
}


string DSSC_PPT::programClocking(bool XFELClockSource, bool internalPLL, int inPLLMult, int exPLLInt, int exPLLFrac, int exPLLMod)
{
  DEBUG_OUT("Program Clocking to " << ((XFELClockSource)? "XFEL Mode" : "Stand Alone Mode"));

  dataString = "PLL0 ";
  if (internalPLL)
    dataString.append("I");
  else
    dataString.append("E");

  if (XFELClockSource)
    dataString.append("X ");
  else
    dataString.append("S ");

  if (internalPLL)
  {
    dataString.append(to_string(inPLLMult));
  }
  else
  {
    dataString.append(to_string(exPLLInt));
    dataString.append(" ");
    dataString.append(to_string(exPLLFrac));
    dataString.append(" ");
    dataString.append(to_string(exPLLMod));
  }
  sendReadPacket();

  double base_frequency = XFELClockSource ? (1300E6/288*22) : 100E6 ;
  double PLLfracNcounters = ((double)exPLLInt)+(((double)exPLLFrac)/((double)exPLLMod));
  double resultingFrequency = internalPLL ?  (base_frequency*inPLLMult/4) : (1.0*base_frequency/10.0/4.0*PLLfracNcounters);
  string resFreqString = std::to_string((int)resultingFrequency) + " Hz";

  return resFreqString;
}


bool DSSC_PPT::readDPReset()
{
  readBackEPCRegister("AuroraRX_Control");

  return epcRegisters->getSignalValue("AuroraRX_Control","0","rx_aurora_reset_in") != 0u;
}


bool DSSC_PPT::readIOBReset()
{
  readBackEPCRegister("Multi_purpose_Register");

  return epcRegisters->getSignalValue("Multi_purpose_Register","0","IOB_RESET1") != 0u;
}

bool DSSC_PPT::readPRBReset()
{
  //dont signal an error if no IOB is available
  if(!isIOBAvailable(currentModule)) return true;

  readBackIOBSingleReg("PRB_control");

  return iobRegisters->getSignalValue("PRB_control",to_string(currentModule),"PRB_power_off") != 0u;
}

bool DSSC_PPT::readAsicReset()
{
  //dont signal an error if no IOB is available
  if(!isIOBAvailable(currentModule)) return true;

  readBackIOBSingleReg("ASIC_reset");

  return iobRegisters->getSignalValue("ASIC_reset",to_string(currentModule),"ASIC_reset") != 0u;
}


bool DSSC_PPT::readEPCReset()
{
  readBackEPCRegister("Multi_purpose_Register");

  return epcRegisters->getSignalValue("Multi_purpose_Register","0","EPC_devices_reset") != 0u;
}


void DSSC_PPT::auroraTXReset()
{
  if(!isIOBAvailable(currentModule)) return;

  iobReset(false);

  iobRegisters->setSignalValue("Aurora_Reset",to_string(currentModule),"Aurora_Reset",1);
  programIOBRegister("Aurora_Reset");
  iobRegisters->setSignalValue("Aurora_Reset",to_string(currentModule),"Aurora_Reset",0);
  programIOBRegister("Aurora_Reset");
}


bool DSSC_PPT::checkAuroraReady()
{
  static const int MAX = 3;

  bool allLocked = true;

  for(uint iob=0; iob<numAvailableIOBs(); iob++)
  {
    currentModule = activeIOBs.at(iob);
    int cnt=0;
    while(!isAuroraReady() && cnt<MAX)
    {
      datapathReset(true);
      usleep(1000);

      datapathReset(false);
      usleep(10000);

      cnt++;
    }

    bool locked = (cnt<MAX);
    if(locked){
      STATUS_OUT("DSSC_PPT: IOB "<< currentModule << " Aurora locked after " << cnt << " tries!");
    }else{
      ERROR_OUT("IOB " + to_string(currentModule) + " Aurora could not be locked!");
    }
    allLocked &= locked;
  }

  return allLocked;
}


bool DSSC_PPT::checkSingleAuroraReady()
{
  static const int MAX = 3;

  int cnt=0;
  while(!isAuroraReady() && cnt<MAX)
  {
    datapathReset(true);
    usleep(1000);

    datapathReset(false);
    usleep(10000);

    cnt++;
  }

  bool locked = (cnt<MAX);
  if(locked){
    STATUS_OUT("DSSC_PPT: IOB "<< currentModule << " Aurora locked after " << cnt << " tries!");
  }else{
    ERROR_OUT("IOB " + to_string(currentModule) + " Aurora could not be locked!");
  }

  return locked;
}


bool DSSC_PPT::isAuroraReady()
{
  readBackEPCRegister("AuroraRX_ReadbackRegister1");
  readBackEPCRegister("AuroraRX_ReadbackRegister2");

  string signalName = "aurora"+to_string(currentModule-1)+" rx_channel_up";

  bool ready = false;
  if(currentModule<=2){
    ready = epcRegisters->getSignalValue("AuroraRX_ReadbackRegister1","0",signalName) == 1u;
  }else{
    ready = epcRegisters->getSignalValue("AuroraRX_ReadbackRegister2","0",signalName) == 1u;
  }

  return ready;
}


void DSSC_PPT::readOutAuroraStatus(vector<bool> &statusBits)
{
  int iobIndex = currentModule - 1;

  readBackEPCRegister("AuroraRX_ReadbackRegister1");
  readBackEPCRegister("AuroraRX_ReadbackRegister2");
  readBackEPCRegister("AuroraRX_Control");
  readBackEPCRegister("Multi_purpose_Register");

  string moduleSetName1 = "AuroraRX_ReadbackRegister";
  string moduleSetName2 = "AuroraRX_ReadbackRegister";

  if(currentModule<4){
    moduleSetName1+= to_string(1);
  }else{
    moduleSetName1+= to_string(2);
  }
  if(currentModule<3){
    moduleSetName2+= to_string(1);
  }else{
    moduleSetName2+= to_string(2);
  }

  string dpSignalString = "datapath_channel_enable" + to_string(currentModule);
  string sigName = "aurora" + to_string(iobIndex) + " ";

  bool pllLocked = epcRegisters->getSignalValue(moduleSetName1,"0",sigName+"pll_locked");
  bool lane0up   = epcRegisters->getSignalValue(moduleSetName1,"0",sigName+"rx_lane0_up");
  bool lane1up   = epcRegisters->getSignalValue(moduleSetName1,"0",sigName+"rx_lane1_up");
  bool lane2up   = epcRegisters->getSignalValue(moduleSetName2,"0",sigName+"rx_lane2_up");
  bool channelUp = epcRegisters->getSignalValue(moduleSetName2,"0",sigName+"rx_channel_up");
  bool softErr   = epcRegisters->getSignalValue(moduleSetName2,"0",sigName+"rx_soft_err");
  bool hardErr   = epcRegisters->getSignalValue(moduleSetName2,"0",sigName+"rx_hard_err");
  bool reset     = epcRegisters->getSignalValue("AuroraRX_Control","0","rx_aurora_reset_in");
  bool channelEn = epcRegisters->getSignalValue("Multi_purpose_Register","0",dpSignalString);

  statusBits.resize(9);
  int idx = 0;
  statusBits[idx++] = channelUp;
  statusBits[idx++] = lane0up;
  statusBits[idx++] = lane1up;
  statusBits[idx++] = lane2up;

  statusBits[idx++] = reset;
  statusBits[idx++] = pllLocked;
  statusBits[idx++] = hardErr;
  statusBits[idx++] = softErr;
  statusBits[idx++] = channelEn;
}


void DSSC_PPT::checkPRBStatus(bool resetPRBAnyWay)
{
  for(uint iob=0; iob<numAvailableIOBs(); iob++){
    currentModule = activeIOBs.at(iob);
    checkCurrentIOBPRBStatus(resetPRBAnyWay);
  }
}


int DSSC_PPT::checkCurrentIOBPRBStatus(bool resetPRBAnyWay)
{
  int  numPRBs    = iobRegisters->getSignalValue("PRB_control",to_string(currentModule),"PRB_num_prbs_found");
  bool selfDetect = iobRegisters->getSignalValue("PRB_control",to_string(currentModule),"PRB_self_detect") == 1;

  if(resetPRBAnyWay || (numPRBs==0 && selfDetect) ){
    runContinuousMode(false);
    iobRegisters->setSignalValue("PRB_control",to_string(currentModule),"PRB_en",0);
    programIOBRegister("PRB_control");
    iobRegisters->setSignalValue("PRB_control",to_string(currentModule),"PRB_en",1);
    programIOBRegister("PRB_control");
  }

  return readNumPRBs();
}


int DSSC_PPT::readNumPRBs()
{
  int numPRBs = 0;
  if(isPRBSelfDetected()){
    readBackIOBSingleReg("PRB_control");
    numPRBs = iobRegisters->getSignalValue("PRB_control",to_string(currentModule),"PRB_num_prbs_found");
    DEBUG_OUT("Found " + to_string(numPRBs) + " Regulators");
  }else{
    numPRBs = iobRegisters->getSignalValue("PRB_number_of_boards",to_string(currentModule),"PRB_number_of_boards");
  }

  STATUS_OUT(numPRBs << " PRBs at IOB " << currentModule << (isPRBSelfDetected() ? " detected" : " specified") );

  return numPRBs;
}


void DSSC_PPT::enableASICPower(uint16_t asic)
{
  if(asic>=0 && asic<16){
    activePowers[currentModule] |= (1<<asic);
  }else{
    ERROR_OUT("DisableASICPower:ASIC out of range:" + to_string(asic));
  }
}


void DSSC_PPT::disableASICPower(uint16_t asic)
{
  if(asic>=0 && asic<16){
    activePowers[currentModule] &= ~(1<<asic);
  }else{
    ERROR_OUT("DisableASICPower:ASIC out of range:" + to_string(asic));
  }
}

void DSSC_PPT::updateStaticASICPowers()
{
  static map<int,string> signalMap {  { 0,"PRB_manual_ctrl_rb1"},{ 1,"PRB_manual_ctrl_rb1"},
                                      { 2,"PRB_manual_ctrl_rb1"},{ 3,"PRB_manual_ctrl_rb1"},
                                      { 4,"PRB_manual_ctrl_rb2"},{ 5,"PRB_manual_ctrl_rb2"},
                                      { 6,"PRB_manual_ctrl_rb2"},{ 7,"PRB_manual_ctrl_rb2"},
                                      { 8,"PRB_manual_ctrl_rb3"},{ 9,"PRB_manual_ctrl_rb3"},
                                      {10,"PRB_manual_ctrl_rb3"},{11,"PRB_manual_ctrl_rb3"},
                                      {12,"PRB_manual_ctrl_rb4"},{13,"PRB_manual_ctrl_rb4"},
                                      {14,"PRB_manual_ctrl_rb4"},{15,"PRB_manual_ctrl_rb4"}};

  static map<int,string> moduleSetMap { { 0,"PRB_manualctrl0"},{ 1,"PRB_manualctrl0"},
                                        { 2,"PRB_manualctrl0"},{ 3,"PRB_manualctrl0"},
                                        { 4,"PRB_manualctrl0"},{ 5,"PRB_manualctrl0"},
                                        { 6,"PRB_manualctrl0"},{ 7,"PRB_manualctrl0"},
                                        { 8,"PRB_manualctrl1"},{ 9,"PRB_manualctrl1"},
                                        {10,"PRB_manualctrl1"},{11,"PRB_manualctrl1"},
                                        {12,"PRB_manualctrl1"},{13,"PRB_manualctrl1"},
                                        {14,"PRB_manualctrl1"},{15,"PRB_manualctrl1"}};

  static map<int,int>    bitMap { { 0,1},{ 1,4},{ 2,7},{ 3,10},
                                  { 4,1},{ 5,4},{ 6,7},{ 7,10},
                                  { 8,1},{ 9,4},{10,7},{11,10},
                                  {12,1},{13,4},{14,7},{15,10}};

  uint16_t comp = 1;
  for(int i=0; i<16; i++){

    if(activePowers[currentModule]&comp){
       uint16_t value = iobRegisters->getSignalValue(moduleSetMap[i],to_string(currentModule),signalMap[i]);
       value |= (1<<bitMap[i]);
       iobRegisters->setSignalValue(moduleSetMap[i],to_string(currentModule),signalMap[i],value);
    }else{
       uint16_t value = iobRegisters->getSignalValue(moduleSetMap[i],to_string(currentModule),signalMap[i]);
       value &= ~(1<<bitMap[i]);
       iobRegisters->setSignalValue(moduleSetMap[i],to_string(currentModule),signalMap[i],value);
    }
    comp<<=1;
  }

}


string DSSC_PPT::getPRBPowerSelect()
{
  const auto value = getIOBParam("PRB_en_static_sel",  to_string(currentModule),"PRB_en_static_sel");
  vector<uint32_t> powerVec;
  for(int i=0; i<16; i++){
    if(value & (1<<i)){
      powerVec.push_back(i);
    }
  }

  return utils::positionVectorToList(powerVec);
}


void DSSC_PPT::setPRBPowerSelect(const std::string & asicsStr, bool program)
{
  std::vector<uint32_t> asicsVec;
  uint16_t gdpsEn = 0;
  if(asicsStr.compare("all") == 0){
    asicsVec = utils::positionListToVector<uint32_t>("0-15");
    gdpsEn = 15;
  }else{
    asicsVec = utils::positionListToVector<uint32_t>(asicsStr);
  }

  cout << "DSSC_PPT: Enable ASIC Powers: " << asicsStr << endl;

  uint16_t switchedSignals = 0;
  uint16_t staticSignals = 0;

  for(const auto & a : asicsVec){
    if(a>15){
      ERROR_OUT("setPRBPowerSelect: ASIC number unknown");
    }else if(a>=12){
      gdpsEn |= 8;
    }else if(a>=8){
      gdpsEn |= 4;
    }else if(a>=4){
      gdpsEn |= 2;
    }else{
      gdpsEn |= 1;
    }

    uint16_t comp = 1<<a;
    switchedSignals |= comp;
    staticSignals   |= comp;
  }

  setIOBParam("PRB_en_static_sel",  to_string(currentModule),"PRB_en_static_sel",staticSignals);
  setIOBParam("PRB_en_switched_sel",to_string(currentModule),"PRB_en_switched_sel",switchedSignals);
  setIOBParam("PRB_en_switched_sel",to_string(currentModule),"PRB_en_gdps_sel",gdpsEn);

  if(program){
    programIOBRegister("PRB_en_static_sel");
    programIOBRegister("PRB_en_switched_sel");
  }
}


void DSSC_PPT::enablePRBStaticVoltages(bool enable)
{
  for(uint iob=0; iob<numAvailableIOBs(); iob++){
    currentModule = activeIOBs.at(iob);
    enablePRBStaticVoltage(enable);
  }
}

//take care of continuous mode when disabeling power
void DSSC_PPT::enablePRBStaticVoltage(bool enable)
{
  int on = (enable)? 0 : 1;

  iobRegisters->setSignalValue("PRB_control",to_string(currentModule),"PRB_en",1);
  iobRegisters->setSignalValue("PRB_control",to_string(currentModule),"PRB_power_off",on);
  programIOBRegister("PRB_control");
}


void DSSC_PPT::setAllVoltagesOn(bool enable)
{
  for(uint iob=0; iob<numAvailableIOBs(); iob++){
    currentModule = activeIOBs.at(iob);
    setCurrentAllVoltagesOn(enable);
  }
}


void DSSC_PPT::setCurrentAllVoltagesOn(bool enable)
{
  int on = (enable)? 1 : 0;

  iobRegisters->setSignalValue("PRB_control_sw_supplies_always_on",to_string(currentModule),"PRB_control_sw_supplies_always_on",on);
  programIOBRegister("PRB_control_sw_supplies_always_on");
}


void DSSC_PPT::enableManualVoltageControl(bool enable)
{
  int value = (enable)? 1: 0;
  iobRegisters->setSignalValue("PRB_control",to_string(currentModule),"PRB_manual_ctrl_en", value);
  programIOBRegister("PRB_control");
}

void DSSC_PPT::resetManualVoltages()
{
  iobRegisters->setSignalValue("PRB_manual_ctrl0",to_string(currentModule),"PRB_manual_ctrl_rb1",0);
  iobRegisters->setSignalValue("PRB_manual_ctrl0",to_string(currentModule),"PRB_manual_ctrl_rb2",0);
  iobRegisters->setSignalValue("PRB_manual_ctrl1",to_string(currentModule),"PRB_manual_ctrl_rb3",0);
  iobRegisters->setSignalValue("PRB_manual_ctrl1",to_string(currentModule),"PRB_manual_ctrl_rb4",0);

  programIOBRegister("PRB_manual_ctrl0");
  programIOBRegister("PRB_manual_ctrl1");
}

//prb from 1 to 4
//supply 1 to 3
void DSSC_PPT::setManualVoltage(int prb, int supply, bool allNotStatic)
{
  if(supply>3){
    return;
  }

  supply--;

  string moduleSetStr;
  string signalString;

  switch(prb)
  {
    case 1: moduleSetStr = "PRB_manual_ctrl0";
            signalString = "PRB_manual_ctrl_rb1";
            break;
    case 2: moduleSetStr = "PRB_manual_ctrl0";
            signalString = "PRB_manual_ctrl_rb2";
            break;
    case 3: moduleSetStr = "PRB_manual_ctrl1";
            signalString = "PRB_manual_ctrl_rb3";
            break;
    case 4: moduleSetStr = "PRB_manual_ctrl1";
            signalString = "PRB_manual_ctrl_rb4";
            break;
  }

  uint16_t value = iobRegisters->getSignalValue(moduleSetStr,to_string(currentModule),signalString);

  //reset supply bits
  value &= ~((uint64_t)7) << (supply*3);

  //set supply bits as required
  if(allNotStatic){
    value += ((uint64_t)7) << (supply*3);
  }else{
    value += ((uint64_t)2) << (supply*3);
  }

  iobRegisters->setSignalValue(moduleSetStr,to_string(currentModule),signalString,value);

  programIOBRegister(moduleSetStr);
}

void DSSC_PPT::checkJtagEnabled()
{
  if(getEPCParam("JTAG_Control_Register","0","EnJTAG"+to_string(currentModule)) == 0){
    cout << "WARNING: JTAG is disabled. Will be enabled to continue" << endl;
    enableJTAGEngines(true);
  }
}


void DSSC_PPT::enableJTAGEngines(bool enable)
{
  int en = (enable)? 1 : 0;

  epcRegisters->setSignalValue("JTAG_Control_Register","all","EnJTAG1",en);
  epcRegisters->setSignalValue("JTAG_Control_Register","all","EnJTAG2",en);
  epcRegisters->setSignalValue("JTAG_Control_Register","all","EnJTAG3",en);
  epcRegisters->setSignalValue("JTAG_Control_Register","all","EnJTAG4",en);

  programEPCRegister("JTAG_Control_Register");

  usleep(10000);
}


void DSSC_PPT::checkJTAGEngine(int chain)
{
  readBackEPCRegister("JTAG_1_Readback_Register");

  const string sigName = "jtag"+to_string(chain)+"_dev_out_fifo_empty";

  const auto isEmpty = getEPCParam("JTAG_1_Readback_Register","0",sigName);

  if(isEmpty == 0){
    ERROR_OUT("++++ JTAG Engine ERROR!!!!!!!!!!!!!!!!");
  }
}


void DSSC_PPT::waitJTAGEngineDone()
{
  dataString = "JTGW " + to_string(currentModule);
  sendReadPacket(true);

  checkJTAGEngine(currentModule-1);
}


bool DSSC_PPT::checkPPTPllLocked()
{
  static const int MAX = 5;
  int cnt = 0;

  setEPCParam("CLOCK_FANOUT_CONTROL","0","PLL_DRP_RST",0);
  setEPCParam("CLOCK_FANOUT_CONTROL","0","PLL_RST",0);

  while(!isPPTPllLocked() && (cnt<MAX) ){
    usleep(10000);
    cnt++;
  }

  bool locked = cnt<MAX;
  if(locked){
    cout<< "DSSC_PPT: PPT PLL locked after " << cnt << " tries" << endl;
  }else{
    ERROR_OUT("PPT PLL not Locked");
  }
  return locked;
}

bool DSSC_PPT::isPPTPllLocked()
{
  readBackEPCRegister("PLLReadbackRegister");
  bool locked = epcRegisters->getSignalValue("PLLReadbackRegister","0","PLL_LD") != 0u;
  readBackEPCRegister("CLOCK_FANOUT_CONTROL");
  locked &= epcRegisters->getSignalValue("CLOCK_FANOUT_CONTROL","0","mmcm_locked") != 0u;

  return locked;
}


void DSSC_PPT::setNumWordsToReceive(int val, bool saveOldVal)
{
  setNumFramesToSend(val/16/4096,saveOldVal);
}


void DSSC_PPT::setNumFramesToSend(int numFrames, bool saveOldVal)
{
  epcRegisters->setSignalValue("AuroraRX_Control","0","num_frames_to_send",numFrames);

  programEPCRegister("AuroraRX_Control");
}


void DSSC_PPT::setBurstVetoOffset(int value)
{
  if(value > 0){
    epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","en_pre_burst_wait_vetos",1);
    epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","num_pre_burst_wait_vetos",value);
  }else{
    epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","en_pre_burst_wait_vetos",0);
  }

  programEPCRegister("CMD_PROT_ENGINE");

  usleep(200000);
}


uint64_t DSSC_PPT::getCellIdsStartByte(int numFramesToSend)
{
  const uint64_t numHeaderBytes = 64;
  const uint64_t numDataBytes = numFramesToSend*2*16*4096;

  return numHeaderBytes + numDataBytes;
}


uint64_t DSSC_PPT::getPulseIdsStartByte(int numFramesToSend)
{
  const uint64_t numHeaderBytes = 64;
  const uint64_t numDataBytes = numFramesToSend*2*16*4096;
  const uint64_t numDescr0Bytes = ((numFramesToSend-1)*2/32+1)*32;

  return numHeaderBytes + numDataBytes + numDescr0Bytes;
}


uint64_t DSSC_PPT::getStatusStartByte(int numFramesToSend)
{
  const uint64_t numHeaderBytes = 64;
  const uint64_t numDataBytes = numFramesToSend*2*16*4096;
  const uint64_t numDescr0Bytes = ((numFramesToSend-1)*2/32+1)*32;
  const uint64_t numDescr1Bytes = ((numFramesToSend-1)*8/32+1)*32;

  return numHeaderBytes + numDataBytes + numDescr0Bytes + numDescr1Bytes;
}


uint64_t DSSC_PPT::getSpecificStartByte(int numFramesToSend)
{
  const uint64_t numHeaderBytes = 64;
  const uint64_t numDataBytes = numFramesToSend*2*16*4096;
  const uint64_t numDescr0Bytes = ((numFramesToSend-1)*2/32+1)*32;
  const uint64_t numDescr1Bytes = ((numFramesToSend-1)*8/32+1)*32;
  const uint64_t numDescr2Bytes = ((numFramesToSend-1)*2/32+1)*32;
  const uint64_t numDescr3Bytes = ((numFramesToSend-1)*4/32+1)*32;
  const uint64_t numDescrBytes = numDescr0Bytes + numDescr1Bytes + numDescr2Bytes + numDescr3Bytes;

  return numHeaderBytes + numDataBytes + numDescrBytes;
}


uint64_t DSSC_PPT::getASICTrailerStartByte(int numFramesToSend)
{
  const uint64_t numHeaderBytes = 64;
  const uint64_t numDataBytes = numFramesToSend*2*16*4096;
  const uint64_t numSpecificBytes = 160;
  const uint64_t numDescr0Bytes = ((numFramesToSend-1)*2/32+1)*32;
  const uint64_t numDescr1Bytes = ((numFramesToSend-1)*8/32+1)*32;
  const uint64_t numDescr2Bytes = ((numFramesToSend-1)*2/32+1)*32;
  const uint64_t numDescr3Bytes = ((numFramesToSend-1)*4/32+1)*32;
  const uint64_t numDescrBytes = numDescr0Bytes + numDescr1Bytes + numDescr2Bytes + numDescr3Bytes;

  return numHeaderBytes + numDataBytes + numDescrBytes + numSpecificBytes;
}


bool DSSC_PPT::inContinuousMode()
{
  if(m_standAlone){
    return getEPCParam("Single_Cycle_Register","0","continuous_mode") == 1;
  }else{
    return getEPCParam("Multi_purpose_Register","0","CmdProtocolEngine_enable") == 1;
  }
}


void DSSC_PPT::runContinuousMode(bool run)
{
  if(m_standAlone){
    if(run) cout << "continuous_mode activated" << endl;
    const int value = (run)? 1 : 0;
    setEPCParam("Single_Cycle_Register","all","continuous_mode",value);
    setEPCParam("Single_Cycle_Register","all","disable_sending",value);
    setEPCParam("Multi_purpose_Register","all","CmdProtocolEngine_enable",0);
  }else{
    const int value = (run)? 1 : 0;
    setEPCParam("Single_Cycle_Register","all","continuous_mode",0);
    //setEPCParam("Single_Cycle_Register","all","disable_sending",value);
    setEPCParam("Multi_purpose_Register","all","CmdProtocolEngine_enable",value);
  }

  programEPCRegister("Multi_purpose_Register");
  programEPCRegister("Single_Cycle_Register");

  if(!run){
    usleep(300);
  }
}


void DSSC_PPT::disableSending(bool disable)
{
  const uint32_t value = (disable)? 1 : 0;
  setEPCParam("Single_Cycle_Register","all","disable_sending",value);
  programEPCRegister("Single_Cycle_Register");
  cout << ((disable)? "DSSC_PPT: Continuous sending disabled" : "DSSC_PPT : Continuous sending enabled") << endl;
}


uint32_t DSSC_PPT::getNumPacketsToReceive()
{
  //  16 Packets per image.
  // 256 16bit words per 8K UDP Packet
  // 8KB Per Image per ASIC
  uint32_t numFrames = epcRegisters -> getSignalValue("AuroraRX_Control","0","num_frames_to_send");
  uint32_t numPackets = utils::getNumPacketsToReceive(numFrames);

  return numPackets;
}


int DSSC_PPT::getBurstVetoOffset()
{
  if(isBurstVetoOffsetActive())
    return epcRegisters->getSignalValue("CMD_PROT_ENGINE","0","num_pre_burst_wait_vetos");
  else
    return 0;
}

bool DSSC_PPT::isBurstVetoOffsetActive()
{
  return epcRegisters->getSignalValue("CMD_PROT_ENGINE","0","en_pre_burst_wait_vetos") == 1;
}


void DSSC_PPT::enableBurstVetoOffset(bool enable)
{
  uint value = (enable)? 1 : 0;
  epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","en_pre_burst_wait_vetos",value);
  programEPCRegister("CMD_PROT_ENGINE");
}


uint64_t DSSC_PPT::getIOBSerialNumber()
{
  readBackIOBSingleReg("SerialNumber_LSB");
  readBackIOBSingleReg("SerialNumber_MSB");

  uint32_t serial_lsb = iobRegisters->getSignalValue("SerialNumber_LSB",to_string(currentModule),"SerialNumber_LSB");
  uint32_t serial_msb = iobRegisters->getSignalValue("SerialNumber_MSB",to_string(currentModule),"SerialNumber_MSB");
  uint64_t serial = serial_msb & 0x00FFFFFF;
  serial = serial << 32;
  serial += serial_lsb;
  serial = serial >> 8;

  return serial;
}

string DSSC_PPT::getIOBSerial()
{
  readBackIOBSingleReg("SerialNumber_LSB");
  readBackIOBSingleReg("SerialNumber_MSB");

  uint32_t serial_lsb = iobRegisters->getSignalValue("SerialNumber_LSB",to_string(currentModule),"SerialNumber_LSB");
  uint32_t serial_msb = iobRegisters->getSignalValue("SerialNumber_MSB",to_string(currentModule),"SerialNumber_MSB");
  uint64_t serial = serial_msb & 0x00FFFFFF;
  serial = serial << 32;
  serial += serial_lsb;
  serial = serial >> 8;

  setIOBSerialForTrainData(currentModule,serial);

  const auto serialStr = utils::getIOBSerialStr(serial);

  cout << "Found IOB " << serialStr << " as module " << currentModule << endl;
  return serialStr;
}


bool DSSC_PPT::writeDataToSocket(const char * data, uint64_t totalLength)
{
  uint64_t anzahl = write(socket_nummer, data, totalLength);
  if(anzahl != totalLength){
    ERROR_OUT("ERROR PPT.sendPacket: " + dataString + " sent " + to_string(anzahl) +"/" + to_string(totalLength) + " Bytes - SlowControlServer on Microblace possibly crashed!");
    return false;
  }
  usleep(500);
  return true;
}


bool DSSC_PPT::sendPacket()
{
  if(simulationMode){
    dumpDataString();
    return false;
  }

  if(!m_opened) return false;

  int64_t length = dataString.length();
  int64_t totalLength = length+8+1;
  char data[totalLength];
  strcpy(data,"cmdXFEL ");
  strcat(data,dataString.c_str());

  data[totalLength-1] = 0;

  return writeDataToSocket(data,totalLength);
}


int DSSC_PPT::sendReadPacket(bool errId)
{
  if(!sendPacket()){
    return 0;
  }
  //read data
  char data[1000];
#ifdef DEBUGPPT
  int anzahl = read(socket_nummer,data,sizeof(data));
#else
  read(socket_nummer,data,sizeof(data));
#endif

  string recvDataStr = string(data);
#ifdef DEBUGPPT
  cout << "INFO PPT read: " << anzahl << " bytes: " << recvDataStr << endl;
#endif

  //check for right origin
  string pptCmd = recvDataStr.substr(0,4);
  if(pptCmd.compare(dataString.substr(0,4))!=0){
    ERROR_OUT("Received Package has unexprected origin: " + pptCmd +  " / " + dataString.substr(0,4));
    return 0;
  }

  try
  {
    //extract return value
    uint32_t retVal = stoul(utils::getSection(recvDataStr,1,' '));
    if(errId && retVal != 0){
      ERROR_OUT("ERROR PPT Server: " +  recvDataStr);
    }
    return retVal;
  }
  catch( ... )
  {
    ERROR_OUT("ERROR DSSC_PPT Read from SlowControlServer: could not convert value " +  utils::getSection(recvDataStr,1,' '));
  }

  return 0;
}


vector<uint32_t> DSSC_PPT::sendReadVectorPacket()
{
  std::vector<uint32_t> values;
  if(!sendPacket()){
    return values;
  }
    //read data
  char data[1000];
  #ifdef DEBUGPPT
  int anzahl = read(socket_nummer,data,sizeof(data));
  #else
  read(socket_nummer,data,sizeof(data));
  #endif

  string recvDataStr = string(data);
  #ifdef DEBUGPPT
  cout << "INFO PPT read: " << anzahl << " bytes: " << recvDataStr << endl;
  #endif

  //check for right origin
  string pptCmd = recvDataStr.substr(0,4);
  if(pptCmd.compare(dataString.substr(0,4))!=0){
    ERROR_OUT("Received Package has unexprected origin: " + pptCmd +  " / " + dataString.substr(0,4));
    return values;
  }

  try
  {
    //extract return value
    uint32_t retVal = stoul(utils::getSection(recvDataStr,1,' '));
    if(retVal != 0){
      ERROR_OUT("ERROR PPT Server: " +  recvDataStr);
    }else{
      //extract number of values to receive
      const uint32_t numValues = stoul(utils::getSection(recvDataStr,1,';'));

      values.reserve(numValues);

      for(uint32_t i=2; i<numValues+2; i++){
        values.push_back(stoul(utils::getSection(recvDataStr,i,';')));
      }
    }

  }
  catch( ... )
  {
    ERROR_OUT("ERROR DSSC_PPT Read from SlowControlServer: could not convert values in string " +  recvDataStr);
  }

  return values;
}


uint64_t DSSC_PPT::sendReadULPacket()
{
  if(!sendPacket()){
    return 0;
  }
  //read data
  char data[1000];

#ifdef DEBUGPPT
  int anzahl = read(socket_nummer,data,sizeof(data));
#else
  read(socket_nummer,data,sizeof(data));
#endif

  string recvDataStr = string(data);
#ifdef DEBUGPPT
  cout << "INFO PPT read: " << anzahl << " bytes: " << recvDataStr << endl;
#endif

  //check for right origin
  string pptCmd = recvDataStr.substr(0,4);
  if(pptCmd.compare(dataString.substr(0,4))!=0){
    ERROR_OUT("Received Package has unexprected origin: " + pptCmd +  " / " + dataString.substr(0,4));
    return 0;
  }

  try
  {
    //extract return value
    uint64_t retval = stoul(getSection(recvDataStr,1));
    return retval;
  }
  catch( ... )
  {
    ERROR_OUT("ERROR DSSC_PPT.sendReadPacket: could not convert value " +  getSection(recvDataStr,1));
  }

  return 0;
}

void DSSC_PPT::iobInitAurora()
{
  datapathReset(true);
  datapathReset(false);
}


bool DSSC_PPT::isAsicPllLocked()
{
  readBackIOBSingleReg("ASIC_status");
  return iobRegisters->getSignalValue("ASIC_status",to_string(currentModule),"ASIC_bufpll_locked") == 1u;
}


bool DSSC_PPT::allDPEnabled()
{
  bool enabled = true;
  for(int i=1; i<5; i++){
    enabled &= isDPEnabled(i);
  }
  return enabled;
}


bool DSSC_PPT::singleDPEnabled()
{
  bool enabled = false;
  for(int i=1; i<5; i++){
    enabled |= isDPEnabled(i);
  }
  return enabled;
}


bool DSSC_PPT::isDPEnabled(int channel)
{
  if(channel>0 && channel<5){
    readBackEPCRegister("Multi_purpose_Register");
    string signalName = "datapath_channel_enable" + to_string(channel);
    return epcRegisters->getSignalValue("Multi_purpose_Register","0",signalName) == 1;
  }else{
    ERROR_OUT("ERROR DSSC_PPT.isDPEnabled: wrong channel defined");
    return false;
  }
}


void DSSC_PPT::setDPEnabled(int channel, bool enable)
{
  if(channel<1 || channel>4){
    ERROR_OUT("ERROR PPT.setDPChannelEnabled: DataChannel not valid. Must be value 1-4");
    return;
  }

  int value = (enable)? 1:0;
  string signalName = "datapath_channel_enable" + to_string(channel);
  epcRegisters->setSignalValue("Multi_purpose_Register","0",signalName,value);

  programEPCRegister("Multi_purpose_Register");

}


void DSSC_PPT::enableDataReadout()
{
  iobRegisters->setSignalValue("ASIC_reset",to_string(currentModule),"ASIC_reset",0);
  programIOBRegister("ASIC_reset");

  iobRegisters->setSignalValue("ASIC_readout_enable",to_string(currentModule),"ASIC_readout_enable",1);
  programIOBRegister("ASIC_readout_enable");
}

string DSSC_PPT::getSection(const string & line, int numPos)
{
  istringstream iss(line);
  string part;
  for(int i=0; i<= numPos; i++){
    iss >> part;
  }

  return part;
}


void DSSC_PPT::iobControl_writeD(int address, int data)
{
  directVector.push_back(0x1c);
  directVector.push_back(0x8);
  directVector.push_back(0xa0);
  directVector.push_back(0x29);

  directVector.push_back(0xFF & (address>>8));
  directVector.push_back(0xFF & (address>>0));

  directVector.push_back(0xFF & (data>>24));
  directVector.push_back(0xFF & (data>>16));
  directVector.push_back(0xFF & (data>> 8));
  directVector.push_back(0xFF & (data>> 0));
}

//DCTX Command implements smart programming of the JTAGDevice which
// ensures that the JTAG IN Fifo never overflows.
// After 10000K Words it is waited, until the fifo is empty again before continuing
// This is the case only during pixel register programming
bool DSSC_PPT::sendDirectWrite(vector<uint32_t> &cmdVec)
{
  if(simulationMode){
    //dumpDataVector(data);
    cmdVec.clear();
    return true;
  }

  ostringstream ss;
  ss << hex << cmdVec.size();
  for(const auto & word : cmdVec){
    ss << ";" << word;
  }
  ss << "$";
  cmdVec.clear();

  ostringstream lengthStream;
  lengthStream << hex << ss.str().length();

  dataString = "DCTW " + lengthStream.str();
  bool ok = sendReadPacket() == 0;

  if(ok){
    usleep(1000);
    dataString = "DCTX " + ss.str();
    ok = sendReadPacket() == 0;
    if(!ok){
      cout << "ERROR: Could not send data packet" << endl;
    }
  }else{
    cout << "ERROR: Did not receive acknowledge" << endl;
  }

#ifdef DEBUG
  cout << dataString << endl;
#endif
  return ok;
}

void DSSC_PPT::EPCWriteD(int address, int data)
{
  directVector.push_back(address);
  directVector.push_back(0x1);
  directVector.push_back(data);
}


// uint DSSC_PPT::getSignalLsb(ConfigReg * registers, int moduleSet, int signal)
// {
//   vector<uint32_t> sigPosVec = registers->getSignalPositions(moduleSet,signal);
//   int sigNumBits = sigPosVec.size();
//   uint sigLsbPos = sigPosVec.front();
//   for(int i=1; i<sigNumBits; i++){
//       if(sigPosVec.at(i)<sigLsbPos){
//         sigLsbPos = sigPosVec.at(i);
//       }
//   }
//   return sigLsbPos;
// }


int DSSC_PPT::getRegSignalValue(int reg, int lsb, int bits)
{
  lsb = lsb%32;
  int msb = lsb+bits;
  if(msb>32){
    cout << "ERROR PPT.getRegSignalValue: signal to long " + to_string(msb) +" larger than 31" << endl;
    return 0;
  }

  return (reg & (((uint64_t)1<<msb)-1) ) >> lsb;
}

/***************************************************************************/


bool DSSC_PPT::sendConfigurationDirectly(vector<uint32_t> &data)
{
  uint32_t totalNumWords = data.size();

  if(debugMode) cout << "++++ Sending JTAG commands directly: " << setw(4) << totalNumWords << " Bytes" << endl;

  data.insert(data.begin(),totalNumWords);
  data.insert(data.begin(),getJTAGCurrentModule());

  return sendDirectWrite(data);
}

void DSSC_PPT::printVectorToConfigFile(vector<uint32_t> &data, const string &fileName)
{
  ios_base::openmode openMode = ofstream::out;
  string configFileName = fileName;

  if(simulationMode){
    configFileName = simFileName;
    openMode = ofstream::out | ofstream::app;
  }

  ofstream out(configFileName,openMode);
  if (!out.is_open()){
    ERROR_OUT("PrintFileERROR could not write File " + fileName);
    return;
  }

  out << hex;
  out << getJTAGCurrentModule() << endl;
  out << data.size() << endl;

//#ifdef DEBUGPPT
  cout << "INFO GenJTAGFile: Num Lines to Send: " << data.size() << endl;
//#endif

  for(const auto & word : data){
    out << word << endl;
  }

  out.flush();
  out.close();

  data.clear();
}


void DSSC_PPT::setRoSerIn(bool bit)
{
  const uint8_t c_jtagInstrX0SelReg = 0b00010001; //17
  const uint8_t c_jtagInstrX1SelReg = 0b00010010; //18

  int numAsics = getNumberOfActiveAsics();

  int numXSelBits = (c_totalXSelBits)*numAsics + 1;

  vector<bool> xBits(numXSelBits,bit);

  xBits.push_back(bit);

  initConfigVector();

  appendAndSendJtagReg(configVector,c_jtagInstrX0SelReg,xBits,false);
  appendAndSendJtagReg(configVector,c_jtagInstrX1SelReg,xBits,false);

  bool ok = sendConfigurationVector(configVector,NoRegs);
  if(!ok){
    ERROR_OUT("SetRoSerIn: could not send data vector");
  }
}

/*


void F1::jtagPassThroughTest(uint8_t pattern)
{
  for(const auto & set : jtagRegisters->getQModuleSetNames()){
    jtagRegsPassThroughTest(set,pattern);
  }

  jtagXYSelPassThroughTest(pattern);
  jtagPixelPassThroughTest(pattern);
}

void F1::jtagRegsPassThroughTest(const QString & jtagModuleSet, uint8_t pattern)
{
  uint8_t jtagInstr = jtagRegisters->getRegAddress(jtagModuleSet);

  int numBits  = jtagRegisters->printContent(jtagModuleSet).size()+7; //8 bits visible at TDO

  bool ok = passThroughTest(pattern,numBits,jtagInstr);

  if (ok) {
    SuS_LOG_STREAM(error, log_id(), "JTAGSet "<< jtagModuleSet.toStdString() << ": Error while reading test pattern.");
  } else {
    SuS_LOG_STREAM(info, log_id(), "JTAGSet "<< jtagModuleSet.toStdString() <<": Read pattern was correct.");
  }
}

void F1::jtagXYSelPassThroughTest(uint8_t pattern)
{
  SuS_LOG_STREAM(warning, log_id(), "Fix F1::jtagXYSelPassThroughTest().");

  uint8_t jtagInstr = c_jtagInstrXYSelReg;

  int numBits  = c_totalPxSelBits+7; //8 bits visible at TDO
  bool ok = passThroughTest(pattern,numBits,jtagInstr);

  if (ok) {
    SuS_LOG_STREAM(error, log_id(), "XYPixelSel: Error while reading test pattern.");
  } else {
    SuS_LOG_STREAM(info, log_id(), "XYPixelSel: Read pattern was correct.");
  }

}

void F1::jtagPixelPassThroughTest(uint8_t pattern)
{
  programXYSelectRegs(false);

  uint8_t jtagInstr = c_jtagInstrPx0Reg;

  int numBits = (numOfPxs*c_controlBitsPerPx)+7;//8 bits visible at TDO //FIXME

  bool ok = passThroughTest(pattern,numBits,jtagInstr);

  if (ok) {
    SuS_LOG_STREAM(error, log_id(), "PixelReg: Error while reading test pattern.");
  } else {
    SuS_LOG_STREAM(info, log_id(), "PixelReg: Read pattern was correct.");
  }
}

bool F1::passThroughTest(uint8_t pattern, int numBits, uint8_t jtagInstr)
{

  bool correct = false;
  int totalBytes = getByteStreamLength(numBits);
  int numBytes = totalBytes;

  int numPackagesToSend = 1;
  if(totalBytes>c_maxBytesPerIOMPack){
      numPackagesToSend = (totalBytes+c_maxBytesPerIOMPack-1)/c_maxBytesPerIOMPack;
      numBytes = totalBytes % c_maxBytesPerIOMPack;
  }

  IomPackage pack(numBytes + 6);
  pack << c_cmdIom_write << c_addrJtagEngine << numBytes + 3;
  pack << (c_jtagInstrEnReadback | jtagInstr) << (uint8_t)numBits << (uint8_t)(numBits>>8);

  for(int i=0; i<numPackagesToSend; ++i){

    for(int i=0; i<numBytes; ++i){
      pack << pattern;
    }
    pack.send();

    numBytes = c_maxBytesPerIOMPack;

    pack = IomPackage(numBytes+3);
    pack << c_cmdIom_write << c_addrJtagEngine << numBytes;
  }

  pack = IomPackage(3);
  pack << c_cmdIom_read << c_addrJtagEngine << 1;
  pack.send();

  uint8_t recvbuf[3];
  IomPackage::readFtdi(recvbuf, 3);
  bool ok = IomPackage::iomPackageOk(recvbuf, c_addrJtagEngine, 1);
  if(ok){
    uint8_t readPattern = (int)recvbuf[2];

    correct = (readPattern == pattern);
    if (!correct) {
      SuS_LOG_STREAM(error, log_id(), "PassThroughTest: Read "<< (int)readPattern << " expected " << (int)pattern);

    }
  }

  return correct;
}

void F1::appendJtagSramCntrl(IomPackage* pack, uint32_t bits) {
  *pack << c_cmdIom_write << c_addrJtagEngine << 9 << 0;
  *pack << (numChipsJtagChain*c_jtagInstrLength-1) << 0;
  *pack << c_jtagInstrSramJtagCntrl << 22 << 0;
  *pack << (uint8_t)bits << (uint8_t)(bits >> 8) << (uint8_t)(bits >> 16);
}
*/

//#############################################################################################
//######## Simulation Functions

void DSSC_PPT::setSimFileName(const string & fileName)
{
  simFileName = fileName;
}


void DSSC_PPT::setSimulationMode(bool enable)
{
  cout << "Simulation Mode " << enable << endl;
  simulationMode = enable;
}


void DSSC_PPT::simReadbackFromJTAG(int byteStreamLength)
{
  ofstream out(simFileName.c_str(), ofstream::out | ofstream::app);
  if (!out.is_open()) {
    cout <<  "Could not open simFile.";
    return;
  }

  out << hex;
  out << (uint32_t)(getJTAGCurrentModule() + 0x1000000) << endl;
  out << (uint32_t)byteStreamLength << endl;

  out.close();
}


void DSSC_PPT::dumpDataString()
{

  ofstream out(simFileName.c_str(), ofstream::out | ofstream::app);
  if (!out.is_open()) {
    cout <<  "Could not open simFile." << endl;
    return;
  }

  cout << dataString << endl;
  //  out << "# " << dataString << endl;
  string cmd    = getSection(dataString,0);
  string option = getSection(dataString,1);

  cout << "SIMMODE: cmd " << cmd;
  cout << " " << option << endl;

  out << hex;

  if(cmd.compare("EPCC")==0){
    uint32_t address = stoul(getSection(dataString,2));
    if(option.compare("W")==0){
      uint32_t data = stoul(getSection(dataString,3));
      out << address << endl;
      out << 1u   << endl;
      out << data  << endl;

    }else if(option.compare("R")==0){
      address += 0x1000000u;
      out << address << endl;
      out << 1u   << endl;
    }
  }else if (cmd.compare("IOBC")==0){
    uint32_t iobNumber = stoul(getSection(dataString,2));
    if(iobNumber==1){
      uint32_t address = IOB_CNTR_1_OFFSET + ((iobNumber-1)<<2);
      uint32_t iob_address = stoul(getSection(dataString,3));
      if(option.compare("W")==0){
        uint32_t data = stoul(getSection(dataString,4));

       //simReadIOBAddress(out,address,iob_address);
        simWriteIOBAddress(out,address,iob_address,data);

      }else if(option.compare("R")==0){
        simReadIOBAddress(out,address,iob_address);
      }
    }
  }else if(cmd.compare("RSTA")==0){
    if(option.compare("1")==0){
            //resetIOB 15
      out << MULTI_PURPOSE_OFFSET << endl;
      out << 1u << endl;
      out << 0x800Fu << endl;     //
            //resetIOB 15
      out << MULTI_PURPOSE_OFFSET << endl;
      out << 1u << endl;
      out << 0x0000u << endl;     //

      //resetAurora 1
      out << AURORA_RX_CONTROL_OFFSET << endl;
      out << 1u << endl;
      out << 0x40000000u << endl;

      simWriteIOBAddress(out,IOB_CNTR_1_OFFSET,c_IOBoardConfRegAddr_aurora_reset,1);

      //resetEthernet 15
      out << ETHERNET_RESET_REGISTER_OFFSET << endl;
      out << 1u << endl;
      out << 0xF << endl;

      //RESET IOB DDR EPC Devices 1
      out << MULTI_PURPOSE_OFFSET << endl;
      out << 1u << endl;
      out << 0x840Fu << endl;


    }else if(option.compare("0")==0){
      //resetEPCDevices 1
      out << MULTI_PURPOSE_OFFSET << endl;
      out << 1u << endl;
      out << 0x0000u << endl;

      //resetAurora 1
      out << AURORA_RX_CONTROL_OFFSET << endl;
      out << 1u << endl;
      out << 0x00000000u << endl;

      simWriteIOBAddress(out,IOB_CNTR_1_OFFSET,c_IOBoardConfRegAddr_aurora_reset,1);

      //resetEthernet 15
      out << ETHERNET_RESET_REGISTER_OFFSET << endl;
      out << 1u << endl;
      out << 0x0u << endl;
    }
  }else if(cmd.compare("JTGC")==0){
    int address = getJTAGCurrentModule();
    string data = getSection(dataString,2).substr(1,2);
    if(data.compare("ff")==0){
      out << address << endl;
      out << 1 << endl;
      out << "ff" << endl;
    }
  }else if(cmd.compare("JTGW")==0){
    uint32_t address = ASIC_JTAG_STATUS_REGISTER_OFFSET;
    address += 0x1000000u;
    out << address << endl;
    out << 1 << endl;
  }else if(cmd.compare("BURS")==0){
    epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","start_burst_from_reg",1);
    programEPCRegister("CMD_PROT_ENGINE");
    epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","start_burst_from_reg",0);
    programEPCRegister("CMD_PROT_ENGINE");
  }else if(cmd.compare("READ")==0){
    epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","start_readout_from_reg",1);
    programEPCRegister("CMD_PROT_ENGINE");
    epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","start_readout_from_reg",0);
    programEPCRegister("CMD_PROT_ENGINE");
  }else if(cmd.compare("TEST")==0){
    epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","send_testpattern_from_reg",1);
    programEPCRegister("CMD_PROT_ENGINE");
    epcRegisters->setSignalValue("CMD_PROT_ENGINE","0","send_testpattern_from_reg",0);
    programEPCRegister("CMD_PROT_ENGINE");
  }else if(cmd.compare("DCTX")==0){

  }

  out.close();

  dataString = "";
}


void DSSC_PPT::simReadIOBAddress(ofstream &out, uint32_t address, uint16_t iob_address)
{
  out << address                                << endl;
  out << "4"                                    << endl;
  out << (uint16_t)c_cmdIOBCNTREngine_startread << endl;
  out << (uint16_t)c_IOBoardCmd_read            << endl;
  out << (uint16_t)((iob_address>>8)&0xFF)      << endl;
  out << (uint16_t)(iob_address&0xFF)           << endl;
  out << address + 0x1000000                    << endl;
  out << "5"                                    << endl;
}


void DSSC_PPT::simWriteIOBAddress(ofstream &out, uint32_t address, uint16_t iob_address, uint32_t data)
{
  out << address                                << endl;
  out << "8"                                    << endl;
  out << (uint16_t)c_cmdIOBCNTREngine_startsend << endl;
  out << (uint16_t)c_IOBoardCmd_write           << endl;
  out << (uint16_t)((iob_address>>8)&0xFF)      << endl;
  out << (uint16_t)(iob_address&0xFF)           << endl;
  out << (uint16_t)((data>>24)&0xFF)            << endl;
  out << (uint16_t)((data>>16)&0xFF)            << endl;
  out << (uint16_t)((data>>8)&0xFF)             << endl;
  out << (uint16_t)(data&0xFF)                  << endl;
}


void DSSC_PPT::clearSimulationFile()
{
  ofstream simFile(simFileName.c_str(), ofstream::out);
  if (!simFile.is_open()) {
    cout <<  "Could not open dumpFile." << endl;
    return;
  }
  cout <<    "Simulation file cleared." << endl;

  simFile.close();
}

