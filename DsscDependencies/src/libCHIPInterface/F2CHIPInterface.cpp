#include "F2CHIPInterface.h"

using namespace SuS;
using namespace std;


#define log_id() "F2CHIPIF"

#define SuS_LOG_STREAM(type,id,output)\
  std::cout <<"-- ["#type"  ] [F2CHIPIF  ] " << output << std::endl; std::cout.flush();

std::string CHIPInterface::chipName = "DSSC_F2";

F2CHIPInterface::F2CHIPInterface(CHIPFullConfig * fullConfig)
 : CHIPInterface()
{
  m_injModeNeedsMonBus = true;

  fullChipConfig = fullConfig;
  jtagRegisters  = fullConfig->getJtagReg(0);
  pixelRegisters = fullConfig->getPixelReg(0);
  sequencer      = fullConfig->getSequencer();

  checkF2Registers();

  setD0Mode(checkD0Mode(),checkBypCompression());

  SuS_LOG_STREAM(info, log_id(), "Sequencer track allocation.");
  sequencer->setJtagSubAddressAndName(Sequencer::ADC_RMP,   0, "ADC_RMP");
  sequencer->setJtagSubAddressAndName(Sequencer::FCF_SwIn,  2, "FCF_SwIn");
  sequencer->setJtagSubAddressAndName(Sequencer::FCF_Flip,  1, "FCF_Flip");
  sequencer->setJtagSubAddressAndName(Sequencer::FCF_Res_B, 3, "FCF_Res_B");
  sequencer->setJtagSubAddressAndName(Sequencer::Inject,    4, "Inject");
  sequencer->compileAndCheckAllTracks();
}


F2CHIPInterface::F2CHIPInterface()
: CHIPInterface()
{
}


bool F2CHIPInterface::checkF2Registers()
{
  if(!pixelRegisters->signalNameExists("Control register","CSA_FbCap")){
    cerr << "F2CHIPInterface: ERROR no F2 configuration found for pixel registers" << endl;
    return false;
  }


  if(!jtagRegisters->signalNameExists("Global Control Register","PxInj_UseBG_NotHD_DAC")){
    cerr << "F2CHIPInterface: ERROR no F2 configuration found for jtag registers" << endl;
    return false;
  }

  return true;
}



bool F2CHIPInterface::checkRegisterTypes()
{
  if(!pixelRegisters->signalNameExists("Control register","CSA_FbCap")){
    cerr << "F2CHIPInterface: ERROR no F2 configuration found for pixel registers" << endl;
    return false;
  }

  if(!jtagRegisters->signalNameExists("Global Control Register","PxInj_UseBG_NotHD_DAC")){
    cerr << "F2CHIPInterface: ERROR no F2 configuration found for jtag registers" << endl;
    return false;
  }

  return true;
}

void F2CHIPInterface::disableInjectionBits()
{
  CHIPInterface::setPixelRegisterValue( "all", "InjBusEn", 0);
  CHIPInterface::setPixelRegisterValue( "all", "QInjEnCs", 0);
  CHIPInterface::setPixelRegisterValue( "all", "InjPxQ", 0);
  CHIPInterface::setPixelRegisterValue( "all", "EnPxInj", 0);
  CHIPInterface::setPixelRegisterValue( "all", "PxInjEnAmpNMirr", 0);
  CHIPInterface::setPixelRegisterValue( "all", "InvertInj", 0);
  CHIPInterface::setPixelRegisterValue( "all", "FCF_EnIntAmpRes",0);
  CHIPInterface::setPixelRegisterValue( "all", "FCF_SelExtRef",0);
  CHIPInterface::setPixelRegisterValue( "all", "ADC_EnExtLatch",0);

  m_injectionEnabled = false;
}


void F2CHIPInterface::enableInjection(bool en, const std::string & pixelStr, bool program)
{
  disableInjectionBits();

  if(en){
    // TODO: check consistency: QInjEnCs is also needed for power down...
    switch (injectionMode) {
      case CURRENT_BGDAC :
      case CURRENT_SUSDAC :
        CHIPInterface::setPixelRegisterValue( pixelStr, "EnPxInj",1);
        CHIPInterface::setPixelRegisterValue( pixelStr, "PxInjEnAmpNMirr",1);

        break;
      case CHARGE_PXINJ_SUSDAC: // identical to BGDAC -> no break
      case CHARGE_PXINJ_BGDAC:
        CHIPInterface::setPixelRegisterValue( pixelStr, "QInjEnCs",1);
        CHIPInterface::setPixelRegisterValue( pixelStr, "EnPxInj",1);
        CHIPInterface::setPixelRegisterValue( pixelStr, "PxInjEnAmpNMirr",1);
        CHIPInterface::setPixelRegisterValue( pixelStr, "InjPxQ",1);
        CHIPInterface::setPixelRegisterValue( pixelStr, "InvertInj",1);

        SuS_LOG_STREAM(info, log_id(), "Enabling pixel charge injection in (Bergamo pixel mode with Bergamo DAC)");
        break;
      case CHARGE_BUSINJ:
        CHIPInterface::setPixelRegisterValue( pixelStr, "QInjEnCs", 1);
        CHIPInterface::setPixelRegisterValue( pixelStr, "InjBusEn", 1);
        // CHIPInterface::setPixelRegisterValue( pixelStr, "PxInjEnAmpNMirr", leaveAsItIs);
        // PxInjEnAmpNMirr needs to be set for the CSA saturation detection, do not change here.
        CHIPInterface::setPixelRegisterValue( pixelStr, "InvertInj",1);
        break;
      case ADC_INJ:
      case ADC_INJ_LR:
        CHIPInterface::setPixelRegisterValue( pixelStr,"FCF_SelExtRef",1);
        break;
      case EXT_LATCH :
        setExtLatchMode();
        break;

      case NORM :
      case NORM_D0 :
      case NORM_DEPFET :
        cout << "CHIPInterface: no injection mode  selected: " << getInjectionModeName(injectionMode) << endl;
    }
  }

  m_injectionEnabled = en;

  if (program) programPixelRegs();
}


void F2CHIPInterface::setInjectionDAC(int setting)
{
  cout << "Set Injection DAC = " << setting << endl;
  if(!m_injectionEnabled){
    utils::CoutColorKeeper keeper(utils::STDRED);
    cout << "+========== ERROR Injection DAC is changed without previous enabling ======" << endl;
  }

  if (!setBothDacsForPixelInjectionVal) {
    SuS_LOG_STREAM(warning, log_id(), "setInjectionDAC(): Only setting the selected DAC. For better results set setBothDacsForPixelInjectionVal = true!");
  }
  int hdDacSettingToMinBugEffect = ((1 << 12) - 1) + (setting << 4);
  int bgDacSettingToMinBugEffect = setting/16;
  // HD DAC has twice the max current as Bg DAC
  // compl output current should match Bg compl output current.
  switch (injectionMode) {
    case CURRENT_BGDAC :
    case CHARGE_PXINJ_BGDAC:
      if (setting < 0 || setting > 255) {
        SuS_LOG_STREAM(warning, log_id(), "Bergamo DAC is used but setting is out of range ( " << setting << ".");
      }
      jtagRegisters->setSignalValue("Global Control Register", "all", "Pixel Injection Signal Trim", setting);
      if (debugMode) {
        SuS_LOG_STREAM(debug, log_id(), "Setting Bergamo DAC to " << setting);
      }
      if (setBothDacsForPixelInjectionVal) {
        setIntDACValue(hdDacSettingToMinBugEffect);
        if (debugMode) {
          SuS_LOG_STREAM(debug, log_id(), "Setting HD DAC to " << hdDacSettingToMinBugEffect);
        }
      }
      programJtagSingle("Global Control Register");
      break;
    case CURRENT_SUSDAC:
    case CHARGE_PXINJ_SUSDAC:
      if (setting < 0 || setting > 4095) {
        SuS_LOG_STREAM(warning, log_id(), "HD DAC is used but setting is out of range ( " << setting << ". Max setting for the pixel injection is 4095 (half DAC range).");
      }
      if (setBothDacsForPixelInjectionVal) {
        jtagRegisters->setSignalValue("Global Control Register", "all", "Pixel Injection Signal Trim", bgDacSettingToMinBugEffect);
        if (debugMode) {
          SuS_LOG_STREAM(debug, log_id(), "Setting Bergamo DAC to " << bgDacSettingToMinBugEffect);
        }
      }
      if (debugMode) {
        SuS_LOG_STREAM(debug, log_id(), "Setting HD DAC to " << setting);
      }
      setIntDACValue(setting);
      programJtagSingle("Global Control Register");
      break;
    case CHARGE_BUSINJ:
      setIntDACValue(setting);
      break;
    default :
      CHIPInterface::setInjectionDAC(setting);
      break;
  }
}


void F2CHIPInterface::setInjectionMode(InjectionMode mode)
{
  m_injModeNeedsMonBus = true;
  injectionMode = mode;

  disableInjectionBits();

  SuS_LOG_STREAM(info, log_id(), "F2 Setting injection mode to " << getInjectionModeName(getInjectionMode()));
  switch (injectionMode) {
    case CURRENT_BGDAC :
    case CHARGE_PXINJ_BGDAC:
      m_injModeNeedsMonBus = false;
      setJTAGParam("Global Control Register", "all", "PxInj_UseBG_NotHD_DAC", 1);
      setJTAGParam("Global Control Register", "all", "VDAC_highrange", 0);
      setJTAGParam("Global Control Register", "all", "VDAC_lowrange", 0);
      setSequencerMode(Sequencer::NORM);
      break;

    case CURRENT_SUSDAC :
    case CHARGE_PXINJ_SUSDAC:
      setJTAGParam("Global Control Register", "all", "PxInj_UseBG_NotHD_DAC", 0);
      setJTAGParam("Global Control Register", "all", "VDAC_highrange", 0);
      setJTAGParam("Global Control Register", "all", "VDAC_lowrange", 0);
      setSequencerMode(Sequencer::NORM);
      break;

    case CHARGE_BUSINJ:
      setJTAGParam("Global Control Register", "all", "PxInj_UseBG_NotHD_DAC", 1);
      setJTAGParam("Global Control Register", "all", "VDAC_highrange", 0);
      setJTAGParam("Global Control Register", "all", "VDAC_lowrange", 1);
      setSequencerMode(Sequencer::NORM);
      break;

    case ADC_INJ_LR : // uses low range DAC
      setJTAGParam("Global Control Register", "all", "PxInj_UseBG_NotHD_DAC", 1);
      setJTAGParam("Global Control Register","all","VDAC_lowrange",1);
      setJTAGParam("Global Control Register","all","VDAC_highrange",0);
      CHIPInterface::setGGCStartValue(DEFAULTGCCSTART); // programs global control register
      setSequencerMode(Sequencer::BUFFER);
      break;

    case ADC_INJ :
      setJTAGParam("Global Control Register", "all", "PxInj_UseBG_NotHD_DAC", 1);
      setJTAGParam("Global Control Register","all","VDAC_lowrange",0);
      setJTAGParam("Global Control Register","all","VDAC_highrange",1);
      CHIPInterface::setGGCStartValue(DEFAULTGCCSTART); // programs global control register
      setSequencerMode(Sequencer::BUFFER);
      break;

    default:
      m_injModeNeedsMonBus = false;
      setJTAGParam("Global Control Register","all","PxInj_UseBG_NotHD_DAC",0);
      setJTAGParam("Global Control Register","all","VDAC_highrange",0);
      setJTAGParam("Global Control Register","all","VDAC_lowrange",0);
      setSequencerMode(Sequencer::NORM);
      break;
  }
  programJtagSingle("Global Control Register");
  if(!m_injModeNeedsMonBus){
    disableMonBusCols();
  }
}


void F2CHIPInterface::setExtLatchMode()
{
    setInjectionMode(InjectionMode::EXT_LATCH);
}


void F2CHIPInterface::setIntDACMode()
{
  setInjectionMode(InjectionMode::ADC_INJ);
}


void F2CHIPInterface::setNormalMode()
{
  setInjectionMode(InjectionMode::NORM_D0);
}


void F2CHIPInterface::setPixelInjectionMode()
{
  setInjectionMode(InjectionMode::CHARGE_BUSINJ);
}


void F2CHIPInterface::setPowerDownBits(const std::string & pixelStr, bool powerDown)
{
  //TODO: on power up we need to restore the state of the bits
  // (setting to 0 disconnects the CSA from the FCF, test if this is needed)
  
  if (powerDown) {
    SuS_LOG_STREAM(info, log_id(), "Setting power down bits for pixels " << pixelStr);
    CHIPInterface::setPixelRegisterValue(pixelStr,"LOC_PWRD", 1);
    CHIPInterface::setPixelRegisterValue(pixelStr,"CSA_Href", 0);
    CHIPInterface::setPixelRegisterValue(pixelStr,"QInjEnCs", 1);  // power down switch for the CSA input node is hidden in the injection
    CHIPInterface::setPixelRegisterValue(pixelStr,"CSA_DisSatDet", 1);
    CHIPInterface::setPixelRegisterValue(pixelStr,"CSA_Resistor", 0);
  } else {
    CHIPInterface::setPixelRegisterValue(pixelStr,"LOC_PWRD", 0);
    //CHIPInterface::setPixelRegisterValue(pixelStr,"QInjEnCs", 0); //TODO: check pixel injection mode and set accordingly
    CHIPInterface::setPixelRegisterValue(pixelStr,"CSA_Resistor", 1);
  }
}


void F2CHIPInterface::enableMonBusCols(const std::vector<int> & cols)
{
  std::stringstream sstrm;
  for (auto c : cols) sstrm << c << ";";
  SuS_LOG_STREAM(info, log_id(), "Enabling MonBusCols " << sstrm.str());
  unsigned int reg0ProgVal = 0;
  unsigned int reg1ProgVal = 0;
  for (auto c : cols) {
    if (c < 32) {
      reg0ProgVal += pow(2,31-c);
    } else {
      reg1ProgVal += pow(2,c-32);
    }
  }

  if(jtagRegisters->getNumModules("Global FCSR 0") != 16){
    jtagRegisters->setSignalValue("Global FCSR 0", "all", "MonBusSwitch", reg0ProgVal);
    jtagRegisters->setSignalValue("Global FCSR 0", "all", "GndInjSwitch", reg0ProgVal);
    jtagRegisters->setSignalValue("Global FCSR 1", "all", "MonBusSwitch", reg1ProgVal);
    jtagRegisters->setSignalValue("Global FCSR 1", "all", "GndInjSwitch", reg1ProgVal);
  }else{
    jtagRegisters->setSignalValue("Global FCSR 0", "0-7", "MonBusSwitch", reg0ProgVal);
    jtagRegisters->setSignalValue("Global FCSR 0", "0-7", "GndInjSwitch", reg0ProgVal);
    jtagRegisters->setSignalValue("Global FCSR 1", "0-7", "MonBusSwitch", reg1ProgVal);
    jtagRegisters->setSignalValue("Global FCSR 1", "0-7", "GndInjSwitch", reg1ProgVal);

    jtagRegisters->setSignalValue("Global FCSR 0", "8-15", "MonBusSwitch", reg1ProgVal);
    jtagRegisters->setSignalValue("Global FCSR 0", "8-15", "GndInjSwitch", reg1ProgVal);
    jtagRegisters->setSignalValue("Global FCSR 1", "8-15", "MonBusSwitch", reg0ProgVal);
    jtagRegisters->setSignalValue("Global FCSR 1", "8-15", "GndInjSwitch", reg0ProgVal);
  }

  programJtagSingle("Global FCSR 0");
  programJtagSingle("Global FCSR 1");

  //usleep(100000);
}

// col0-5 or 0-5
void F2CHIPInterface::enableMonBusCols(std::string colsStr)
{
  if (colsStr.compare(0,3,"col")==0) colsStr = colsStr.substr(3);
  if (colsStr.compare(0,4,"skip")==0) colsStr = colsStr.substr(4);
  if (colsStr.compare(0,5,"split")==0) colsStr = colsStr.substr(5);

  if (colsStr.find(':')) colsStr = colsStr.substr(0,colsStr.find(':')-1);
  std::vector<int> cols;
  if (!colsStr.empty()) cols = utils::positionListToVector<int>(colsStr);
  enableMonBusCols(cols);
}


void F2CHIPInterface::disableMonBusCols()
{
  enableMonBusForPixels(std::vector<int>());
}


void F2CHIPInterface::enableMonBusForPixels(const std::vector<int> & pixels)
{
  if(!m_injModeNeedsMonBus){
    enableMonBusCols(std::vector<int>());
    return;
  }

  std::vector<bool> cols2enable(64,false);
  for (auto p : pixels) {
    cols2enable[p % 64] = true;
  }
  std::vector<int> cols;
  int c = 0;
  for (auto enCol : cols2enable) {
    if (enCol) cols.push_back(c);
    c++;
  }
  enableMonBusCols(cols);
}


const std::vector<uint32_t> F2CHIPInterface::getPoweredPixelsFromConfig()
{
  // TODO
  vector<uint32_t> pixelsToRet = getColumnPixels<uint32_t>(currPixels);
  return pixelsToRet;
}


bool F2CHIPInterface::calibrateCurrCompDAC(bool log, int singlePx, int startSetting, int defaultValue)
{
  if(chipName == "DSSC_MM9"){
    return CHIPInterface::calibrateCurrCompDAC(log,singlePx,startSetting,defaultValue);
  }

  SuS_LOG_STREAM(info, log_id(), "calibrateCurrCompDAC() not needed in MM9. Nothing done.");
  return true;
}

void F2CHIPInterface::setPixelInjectionGain(const std::string & pixelStr,PXINJGAIN injGain, bool program)
{
  cout << "Set Pixel Injection Gain to " << (injGain == PXINJGAIN::LowGain? "LowGain" : (injGain == PXINJGAIN::MediumGain)? "MediumGain" : "HighGain") << endl;
  bool hg = false;
  if (injGain == MediumGain) {
    SuS_LOG_STREAM(info, log_id(), "Medium gain not implemented setting to high gain, not applicable for the bus injection.");
  } else if (injGain == HighGain) {
    hg = true;
  }
  cout << "Set pixel injection gain to " << (injGain == PXINJGAIN::LowGain? "LowGain" : (injGain == PXINJGAIN::MediumGain)? "MediumGain" : "HighGain") << endl;

  setPixelRegisterValue("all", "InjHG", hg ? 1 : 0);
  setPixelRegisterValue("all", "CSA_Cin_200fF", hg ? 1 : 0);

  if(program){
    programPixelRegs();
  }
}

int F2CHIPInterface::getInjectionDACsetting()
{
  switch (injectionMode) {
    case CHARGE_PXINJ_SUSDAC:
    case CURRENT_SUSDAC :
    case CHARGE_BUSINJ:
      return getIntDACValue();
    default:
      return CHIPInterface::getInjectionDACsetting();
  }
  return CHIPInterface::getInjectionDACsetting();
}


//void F2::setGlobalDecCapSetting(DECCAPSETTING newSetting)
void F2CHIPInterface::setGlobalDecCapSetting(DECCAPSETTING newSetting)
{
  SuS_LOG_STREAM(info, log_id(), "Using F2::setGlobalDecCapSetting(), connections are hard wired, only on/off implemented for now");
  globalDeCCAPSetting = newSetting;

  if(globalFCSR0Vec.size() != (198u*5)){
    globalFCSR0Vec = vector<bool>(198*5,newSetting > 0 ? true : false);
  }

  if(globalFCSR1Vec.size() != (148u*5)){
    globalFCSR1Vec = vector<bool>(148*5,newSetting > 0 ? true : false);
  }
}
