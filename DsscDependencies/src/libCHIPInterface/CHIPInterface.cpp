#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <unistd.h>
#include <math.h>
#include <numeric>

#include "CHIPInterface.h"
#include "ConfigReg.h"
#include "Sequencer.h"


#define log_id() "CHIPIF"

#define SuS_LOG_STREAM(type,id,output)\
  std::cout <<"-- ["#type"  ] [CHIPIF  ] " << output << std::endl; std::cout.flush();

using namespace SuS;
using namespace std;

bool CHIPInterface::debugMode = false;
bool CHIPInterface::emitReadoutDone = false;

//std::string CHIPInterface::chipName = "Unknown";
std::mutex CHIPInterface::pixelDataMutex;

const std::vector<int> CHIPInterface::asicDONumber{0,1,2,3,4,5,6,7,15,14,13,12,11,10,9,8};
const std::vector<int> CHIPInterface::asicInJTAGChain{15,14,13,12,11,10,9,8,0,1,2,3,4,5,6,7};
const std::vector<int> CHIPInterface::prbPowerInJTAGChain{15,14,11,10,7,6,3,2,0,1,4,5,8,9,12,13};

template std::vector<uint32_t> CHIPInterface::getColumnPixels<uint32_t>(int col);

CHIPInterface::CHIPInterface(CHIPFullConfig * fullConfig)
 : CHIPInterface()
{
  fullChipConfig = fullConfig;
  jtagRegisters  = fullConfig->getJtagReg(0);
  pixelRegisters = fullConfig->getPixelReg(0);
  sequencer      = fullConfig->getSequencer();

  setD0Mode(checkD0Mode(),checkBypCompression());
}


CHIPInterface::CHIPInterface()
:  m_iterations(1),
   m_numRuns(5),
   m_trimStartAddr(0),
   m_trimEndAddr(7), // default for the old USB2 board, can set it to more but need to be carfull if there is a long FT
   m_trimPixelDelayMode(0),
   injectionMode(CHARGE_PXINJ_BGDAC),
   trimCurrentDoubleMode(TCDM::DOUBLE),
   untrimmablePixelsFound(false),
   reticleTestMode(false),
   runTrimming(true),
   singlePxPowered(-1),
   fullFReadout(false),
   windowReadout(false),
   d0Mode(false),
   d0BypCompr(false),
   d0ModeRem(false),
   d0BypComprRem(false),
   recalCurrCompOnPowerUp(false),
   sweepParamsOnly(false),
   activeComponentId("DSSC-TestDevice"),
   m_injectionEventOffset(0),
   baselineValuesValid(false),
   xorMask(0x93FFF),// mask out FCSRs except pixel FCSR
   singlePixelToReadout(0),
   singlePixelReadoutMode(false),
   setBothDacsForPixelInjectionVal(true)
{
#if defined(MM3IO) || defined(F1IO)
//      enum TrackNum {RMP=0, FCF_SwIn=1, FCF_Res_B=2, FCF_Flip_Inject=3, ISubPulse=4
// not used in MM3     ADC_RMP=0, FCF_Sw1=3, Inject=3, FCF_ResAmp_B=2};
#endif

  setGlobalDecCapSetting(GLBL_NONE);

  sendingAsics = 0x0001;
  activeAsics  = 0x0001;
}


CHIPInterface::~CHIPInterface()
{
  delete fullChipConfig;
}


void CHIPInterface::updateNumXors()
{
  expectedRegisterXors.resize(getNumberOfActiveAsics(),0);
}


void CHIPInterface::calculateRegisterXors(RegisterType regs)
{
  if(regs == NoRegs) return;

  bool XOR_in = false;

  if (regs == AllRegs || regs == JtagRegs) {
    calculateJtagXors(XOR_in);
  }

  if (regs == AllRegs || regs == PxRegs) {
    calculatePixelRegisterXors(XOR_in);
  }

  if (regs == AllRegs|| regs == SeqRegs) {
    calculateSequencerXors(XOR_in);
  }

  for(auto && asicXors : expectedRegisterXors){
    asicXors &= xorMask; // mask out FCSR except pixel FCSR
  }

  if (debugMode){
    SuS_LOG_STREAM(debug, log_id(), "Expected register XORs: " << utils::intToBitString(expectedRegisterXors.front(),21));
  }
}

void CHIPInterface::calculateJtagXors(bool XOR_in)
{ // names valid for F1
  //dont compare FCSR
  static unordered_map<string,int> jtagXorMap{
                            //     {"FCSR YSel",20},{"FCSR PXRegs1",19},{"FCSR XSel1",18},
                            //     {"Global FCSR 1",17},
                            //     {"FCSR PXRegs0",16},{"FCSR XSel0",15},
                            //     {"Global FCSR 0",14},
                                   {"XorIns",13},
                                   {"Temperature ADC Controller",12},
                                   {"Serializer Config Register",11},
                                   {"SRAM Controller Config Register",10},
                                   {"JTAG SRAM Control",9},
                            //     {"ISubPulse",8},{"FCF_Flip_Inject",7},
                            //     {"FCF_Res_B",6},{"FCF_SwIn",5},
                            //     {"ADC_RMP",4},{"Hold",3},
                                   {"Sequencer Config Register",2},
                                   {"Master FSM Config Register",1},
                                   {"Global Control Register",0}      };

  if (debugMode) {
    SuS_LOG_STREAM(debug, log_id(), "Calculating JTAG Registers Xors.");
  }

  uint32_t xorIns = jtagRegisters->getSignalValue("XorIns","0","nc");
  if(xorIns != 0){
    SuS_LOG_STREAM(warning, log_id(), "XorIns not zero, computation will not be correct");
  }
  //init XORs with 0
  for(auto && asicXors : expectedRegisterXors)
  {
    for(const auto & bit : jtagXorMap){
      asicXors &= ~(1 << bit.second);
    }
  }

  const auto moduleSets = jtagRegisters->getModuleSetNames();
  const auto outputs = jtagRegisters->getOutputs(moduleSets.front());

  for (const auto & set : moduleSets) {
    for (const auto & out : outputs) {
      int asic = stoi(out);
      const auto it = jtagXorMap.find(set);
      if(it != jtagXorMap.end()){
        bool expectedXor = XOR_in;
        const auto bits = jtagRegisters->printContent(set,out);
        for (const auto & bit :bits){
          expectedXor = expectedXor^bit;
        }
        if(debugMode){
          SuS_LOG_STREAM(debug, log_id(), set + " Expected XOR: " + to_string(expectedXor) + " bit " + to_string(it->second) );
        }
        expectedRegisterXors[asic] |= (uint32_t)(expectedXor << it->second);
      }
    }
  }
}

void CHIPInterface::calculatePixelRegisterXors(bool XOR_in)
{
  if (debugMode) {
    SuS_LOG_STREAM(debug, log_id(), "Calculating Pixel Registers Xors.");
  }
  const auto jtagOutputs  = jtagRegisters->getOutputs("Global Control Register");
  const auto pxRegOutputs = pixelRegisters->getOutputs("Control register");
  const auto outputsSize  = pxRegOutputs.size();

  for(const auto jtagOut : jtagOutputs)
  {
    int asic = stoi(jtagOut);
    uint32_t xorIns = jtagRegisters->getSignalValue("XorIns",jtagOut,"nc");
    if(xorIns != 0){
      SuS_LOG_STREAM(warning, log_id(), "XorIns not zero, computation will not be correct");
    }

    vector<bool> asicPixelRegsBits;
    for(int i=0; i<numOfPxs; i++)
    {
      uint32_t addr = asic*numOfPxs+i;
      if(addr<outputsSize){
        const auto pxRegsBits = pixelRegisters->printContent("Control register",pxRegOutputs[asic*numOfPxs+i]);
        asicPixelRegsBits.insert(asicPixelRegsBits.end(),pxRegsBits.begin(),pxRegsBits.end());
      }else{
        SuS_LOG_STREAM(error, log_id(), "Address out of range, could not recalc xors: " << addr << "/" << outputsSize << " asic: " << asic);
      }
    }

    unsigned matrix_half_bits_size = asicPixelRegsBits.size()/2;

    //left matrix half - (0) - Orientation: looking at the layout, periphery block at bottom
    bool expectedPixelSR_XOR_0 = XOR_in;
    for (unsigned j=0; j<matrix_half_bits_size; ++j){
      expectedPixelSR_XOR_0 = expectedPixelSR_XOR_0^asicPixelRegsBits[j];
    }

    //right matrix half - (1) - Orientation: looking at the layout, periphery block at bottom
    bool expectedPixelSR_XOR_1 = XOR_in;
    for (unsigned j=matrix_half_bits_size; j<asicPixelRegsBits.size(); ++j){
      expectedPixelSR_XOR_1 = expectedPixelSR_XOR_1^asicPixelRegsBits[j];
    }
    expectedRegisterXors[asic] &=   ~ ( (1 << 19)
                                | (1 << 16));
    expectedRegisterXors[asic] |= ((uint32_t)expectedPixelSR_XOR_1 << 19)  //right matrix half
                               |  ((uint32_t)expectedPixelSR_XOR_0 << 16); //left matrix half
  }
}

void CHIPInterface::calculateSequencerXors(bool XOR_in)
{
  if (debugMode) {
    SuS_LOG_STREAM(debug, log_id(), "Calculating Sequencer Registers Xors.");
  }

  //because all asic sequencers are programmed equally the xor also has to be the same
  const auto jtagModules  = jtagRegisters->getModules("Global Control Register");
  for(const auto & asicMod : jtagModules){
    uint32_t xorIns = jtagRegisters->getSignalValue("XorIns",asicMod,"nc");
    if(xorIns != 0){
      SuS_LOG_STREAM(warning, log_id(), "XorIns not zero, computation will not be correct");
    }
  }

  std::vector<bool> bits;
  sequencer->getHoldProgBits(bits);
  bool expectedSeqHold_Xor = XOR_in;
  for (const auto & bit : bits){
    expectedSeqHold_Xor = expectedSeqHold_Xor^bit;
  }

  vector<bool> expectedSeqTrack_Xors;
  for (int i=0; i<Sequencer::c_numOfTracks; ++i) {
    bool expectedXor = XOR_in;
    sequencer->getTrackDdynProgBits(sequencer->getSeqTrackIndex(i), bits);
    for (const auto & bit : bits){
      expectedXor = expectedXor^bit;
    }
    expectedSeqTrack_Xors.push_back(expectedXor);
  }

  static unordered_map<string,int> seqXorMap{
                            //     {"FCSR YSel",20},{"FCSR PXRegs1",19},{"FCSR XSel1",18},
                            //     {"Global FCSR 1",17},{"FCSR PXRegs0",16},
                            //     {"FCSR XSel0",15},{"Global FCSR 0",14},
                            //     {"JTAG XorIns",13},
                            //     {"Temperature ADC Controller",12},
                            //     {"Serializer Config Register",11},
                            //     {"SRAM Controller Config Register",10},
                            //     {"JTAG SRAM Control",9},
                                   {sequencer->getTrackName(sequencer->getSeqTrackIndex(4)), 8},
                                   {sequencer->getTrackName(sequencer->getSeqTrackIndex(3)), 7},
                                   {sequencer->getTrackName(sequencer->getSeqTrackIndex(2)), 6},
                                   {sequencer->getTrackName(sequencer->getSeqTrackIndex(1)), 5},
                                   {sequencer->getTrackName(sequencer->getSeqTrackIndex(0)), 4},
                                   {"Hold", 3}
                            //     {"Sequencer Config Register",2},
                            //     {"Master FSM Config Register",1},
                            //     {"Global Control Register",0}
                                   };

  for(const auto & bit : seqXorMap){
    for(uint16_t asic=0;asic<jtagModules.size();asic++){
      expectedRegisterXors[asic] &= ~(1 << bit.second);
    }
  }

  for(uint16_t asic=0;asic<jtagModules.size();asic++){
    expectedRegisterXors[asic] |= (expectedSeqHold_Xor << seqXorMap["Hold"]);
  }

  for(unsigned i=0;i<expectedSeqTrack_Xors.size();i++){
    const string trackName = string(sequencer->trackNumToName(sequencer->getSeqTrackIndex(i)));
    const auto seqXor = seqXorMap[trackName];
    const auto expSeqXor = expectedSeqTrack_Xors[i];

    for(uint16_t asic=0;asic<jtagModules.size();asic++){
      expectedRegisterXors[asic] |= (expSeqXor<<seqXor);
    }

    if(debugMode){
      SuS_LOG_STREAM(debug, log_id(), trackName + " Expected XOR: " + to_string(expSeqXor) + " bit " + to_string(seqXor) );
    }
  }
}


std::string CHIPInterface::getPixelInjectionGain()
{
  bool highGain = getPixelRegisterValue("0","InjHG");
#ifdef F1IO
  uint32_t d0MimCap = getPixelRegisterValue("0","D0_EnMimCap");
#else
  uint32_t d0MimCap = getPixelRegisterValue("0","CSA_FbCap");
#endif
  bool lsbSet = (d0MimCap & 0x1);
  if(highGain){
    if(lsbSet){
      return "HIGH";
    }else{
      return "MEDIUM";
    }
  }
  return "LOW";
}


void CHIPInterface::setPixelInjectionGain(const std::string & pixelStr,PXINJGAIN injGain, bool program)
{
  cout << "Set Pixel Injection Gain to " << (injGain == PXINJGAIN::LowGain? "LowGain" : (injGain == PXINJGAIN::MediumGain)? "MediumGain" : "HighGain") << endl;

  uint32_t D0MimCap = pixelRegisters->getSignalValue("Control register", pixelStr,"D0_EnMimCap");
  uint32_t highGain = 0;
  switch(injGain){
    case PXINJGAIN::LowGain :
    {
      D0MimCap &= 0x6;
      highGain  = 0;
      break;
    }
    case PXINJGAIN::MediumGain :
    {
      D0MimCap &= 0x6;
      highGain  = 1;
      break;
    }
    case PXINJGAIN::HighGain :
    {
      D0MimCap |=0x1;
      highGain  = 1;
      break;
    }
  }

  setPixelRegisterValue(pixelStr,"InjHG",highGain);
#ifdef F1IO
  setPixelRegisterValue(pixelStr,"D0_EnMimCap",D0MimCap);
#else
  setPixelRegisterValue(pixelStr,"CSA_FbCap",D0MimCap);
#endif
  if(program){
    programPixelRegs();
  }
}


void CHIPInterface::setGGCStartValue(uint32_t value, bool program)
{
  cout << "Setting GCC Start Value to " << value << endl;

  jtagRegisters->setSignalValue("Global Control Register","all","GCC_StartVal_0",value);
  jtagRegisters->setSignalValue("Global Control Register","all","GCC_StartVal_1",value);

  if(program){
    programJtagSingle("Global Control Register");
  }
}


void CHIPInterface::setGlobalDecCapSetting(DECCAPSETTING newSetting)
{
  globalDeCCAPSetting = newSetting;

  if(globalFCSR0Vec.size() != (198u*5)){
    globalFCSR0Vec = vector<bool>(198*5,false);
  }

  if(globalFCSR1Vec.size() != (148u*5)){
    globalFCSR1Vec = vector<bool>(148*5,false);
  }

  bool enVDDA    = (globalDeCCAPSetting&1) != 0 ;
  bool enVDDDADC = (globalDeCCAPSetting&2) != 0 ;

  for(int i=0; i<198; i++){
    globalFCSR0Vec[i*5+4]   = enVDDA;
    globalFCSR0Vec[i*5+3]   = enVDDDADC;
    if(i<148){
      globalFCSR1Vec[i*5+4] = enVDDA;
      globalFCSR1Vec[i*5+3] = enVDDDADC;
    }
  }
}


bool CHIPInterface::compareGlobalDecCapBits(std::vector<bool> &bits, const std::string & moduleSet)
{
  uint numBitsToCompare = (moduleSet.compare("Global FCSR 0") == 0)? globalFCSR0Vec.size() : globalFCSR1Vec.size();

  cout << "Total Bits: " << bits.size() << endl;

  cout << "Compare Global DecCapBits Num bits: " << numBitsToCompare << endl;
  if(bits.size() < numBitsToCompare){
    cout << "Compare Global DecCapBits Failed: input vector too small!" << endl;
    return false;
  }

  const bool isVDDA     = (globalDeCCAPSetting&1) != 0;
  const bool isVDDD_ADC = (globalDeCCAPSetting&2) != 0;
  const uint numModulesToCompare = numBitsToCompare/5;
  int errorCnt = 0;
  for(auto i=0u; i<numModulesToCompare; i++){
    if(!bits[i*5]){
      cout << "RBCompareError: Global Reg " << moduleSet << " : module " << i << " DECCAP GOOD 0" << endl;
      errorCnt++;
    }

    if(isVDDA ^ bits[i*5+4]){
      cout << "RBCompareError: Global Reg " << moduleSet << " : module " << i << " DECCAP_VDDA was " <<  bits[i*5+4] << endl;
    }
    if(isVDDD_ADC ^ bits[i*5+3]){
      cout << "RBCompareError: Global Reg " << moduleSet << " : module " << i << " DECCAP_VDDD_ADC was " << bits[i*5+3] << endl;
    }
    if(bits[i*5+1]){
      cout << "RBCompareError: Global Reg " << moduleSet << " : module " << i << " DECCAP_VDDD was " << bits[i*5+1] << endl;
    }
  }

  if(errorCnt > 5){
    cout << "Global DecCap Bits with errors: #" << errorCnt << std::endl;
  }

  bits.erase(bits.begin(),bits.begin()+numBitsToCompare-1);

  return true;
}


int CHIPInterface::checkDecouplingCaps()
{
#ifdef F1IO
  bool vdddadc_wasstatic = getStaticVDDDADC();

  setPixelRegisterValue( "all", "DisableFilterCC", 1);
  setPixelRegisterValue( "all", "EnDecCapVDDA", 0);
  setPixelRegisterValue( "all", "EnDecCapVDDD", 0);
  setPixelRegisterValue( "all", "EnDecCapVDDD_ADC", 1);
  programPixelRegs(false);

  if (!vdddadc_wasstatic)
    setStaticVDDDADC(true);

  cout << "Checking decoupling caps." << endl;

  resetChip();

  if (!vdddadc_wasstatic)
    setStaticVDDDADC(false);

  programPixelRegs(true, true);

  int numErrors = reticleTestErrorList.size();

  setPixelRegisterValue( "all", "EnDecCapVDDD_ADC", 0);
  setPixelRegisterValue( "all", "EnDecCapVDDA", 0);
  setPixelRegisterValue( "all", "EnDecCapVDDD", 0);
  programPixelRegs(false);

  return numErrors;
#else
  bool vdddadc_wasstatic = getStaticVDDDADC();
  bool vdda_wasstatic = getStaticVDDA();

  setPixelRegisterValue( "all", "EnDecCap", 1);
  programPixelRegs(false);

  cout << "Checking decoupling caps." << endl;

  setVDDAandVDDDADCSuppliesStatic(true);

  resetChip();

  if (!vdddadc_wasstatic)
    setStaticVDDDADC(false);
  if (!vdda_wasstatic)
    setStaticVDDA(false);

  programPixelRegs(true, true);

  int numErrors = reticleTestErrorList.size();

  setPixelRegisterValue( "all", "EnDecCap", 0);
  programPixelRegs(false);

  return numErrors;
#endif
}


void CHIPInterface::showCompareErrors(ConfigReg *configReg, const string & fileName)
{
  ofstream out(fileName,ofstream::out);
  if(!out.is_open()){
    return;
  }

  auto errors = configReg->getCompareErrors();
  for(const auto & error : errors){
    out << error << endl;
  }

  out.close();

  showCompareErrors(configReg);
}


void CHIPInterface::showCompareErrors(Sequencer *seq, const std::string & fileName)
{
  ofstream out(fileName,ofstream::out);
  if(!out.is_open()){
    return;
  }

  auto errors = seq->getCompareErrors();
  for(const auto & error : errors){
    out << error << endl;
  }

  out.close();

  showCompareErrors(seq);
}


void CHIPInterface::showCompareErrors(Sequencer * seq)
{
  vector<string> errors = seq->getCompareErrors();
  if(!errors.empty()) SuS_LOG_STREAM(error, log_id(), "Readback with errors");

  for(auto && error : errors){
    SuS_LOG_STREAM(error,log_id(), error);
  }

  if(reticleTestMode){
    addErrorBitsToList(seq);
  }

  seq->clearCompareErrors();
}


void CHIPInterface::showCompareErrors(ConfigReg * configReg)
{
  const auto errorVec = configReg->getCompareErrors();
  cout << "Error Vec Size = " << errorVec.size() << endl;
  if(!errorVec.empty()) SuS_LOG_STREAM(error, log_id(), "Readback with errors");

  for(auto && errorStr : errorVec){
    SuS_LOG_STREAM(error,log_id(), errorStr);
  }

  if(reticleTestMode){
    addErrorBitsToList(configReg);
  }

  configReg->clearCompareErrors();
}


int CHIPInterface::addErrorBitsToList(ConfigReg* configReg)
{
  const auto errorList = configReg->getCompareErrors();
  const uint32_t numErrors = errorList.size();

  reticleTestErrorList.insert(reticleTestErrorList.end(),errorList.begin(),errorList.end());

  return numErrors;
}


int CHIPInterface::addErrorBitsToList(Sequencer* seq)
{
  const auto errorList = seq->getCompareErrors();
  const uint32_t numErrors = errorList.size();

  reticleTestErrorList.insert(reticleTestErrorList.end(),errorList.begin(),errorList.end());

  return numErrors;
}


int CHIPInterface::addErrorBitsToList(const vector<int>& errBits,const string & regName)
{
  for(const auto & errBit : errBits){
   reticleTestErrorList.push_back("Error in ModuleSet " + regName + " bit " +to_string(errBit));
  }
  return errBits.size();
}


bool CHIPInterface::checkSequencerRBData()
{
  bool ok = checkSequencerHoldRBData();
  if (!ok)
    SuS_LOG_STREAM(error, log_id(), "checkSequencerHoldRBData() returned false");

  for(int addr=0;addr<Sequencer::c_numOfTracks; addr++){
    bool trackOK = checkSequencerTrackRBData(addr);
    if (!trackOK)
      SuS_LOG_STREAM(error, log_id(), "Track no. " << addr << " faulty");
    ok &= trackOK;
  }

  return ok;
}


bool CHIPInterface::checkSequencerHoldRBData()
{
  bool ok = true;
  uint bitsPerAsic     = sequencer->getNumHoldProgBits();
  uint bitStreamLength = bitsPerAsic*getNumberOfActiveAsics();

  vector<bool> data_vec;
  getContentFromDevice(bitStreamLength,data_vec);

  for(int asic = 0; asic< getNumberOfActiveAsics(); asic++){
    ok &= sequencer->compareHoldContent(data_vec);
    data_vec.erase(data_vec.begin(),data_vec.begin()+bitsPerAsic);
  }

  return ok;
}


bool CHIPInterface::checkSequencerTrackRBData(int seqTrackAddr)
{
  uint bitsPerAsic     = sequencer->getNumTrackBits(sequencer->getSeqTrackIndex(seqTrackAddr));
  uint bitStreamLength = bitsPerAsic*getNumberOfActiveAsics();
  bool ok = true;

  vector<bool> data_vec;
  getContentFromDevice(bitStreamLength,data_vec);

  for(int asic = 0; asic<getNumberOfActiveAsics(); asic++){
    ok &= sequencer->compareTrackContent(data_vec,sequencer->getSeqTrackIndex(seqTrackAddr));
    data_vec.erase(data_vec.begin(),data_vec.begin()+bitsPerAsic);
  }
  return ok;
}


bool CHIPInterface::checkAllModuleRegsRBData(ConfigReg *configReg, bool overwrite)
{
  bool ok = true;
  const auto moduleSets = configReg->getModuleSetNames();
  for(const auto & set : moduleSets){
    ok &= checkSingleModuleRegRBData(configReg,set,overwrite);
  }
  return ok;
}


bool CHIPInterface::checkSingleModuleRegRBData(ConfigReg *configReg, const std::string & moduleSet, bool overwrite)
{
  uint32_t bitStreamLength = configReg->getNumBitsPerModule(moduleSet);

  if (chipName.compare(5,1,"F") == 0)
  {
    if(moduleSet.compare("Global FCSR 0") == 0) {
      bitStreamLength += globalFCSR0Vec.size(); // fcsr -> +1 bit
    }else if(moduleSet.compare("Global FCSR 1") == 0) {
      bitStreamLength += globalFCSR1Vec.size(); // fcsr -> +1 bit
    }
  }

  uint32_t numModules = configReg->getNumModules(moduleSet);

  // this function is called if all pixel regs are programmed identically
  if(moduleSet.compare("Control register") == 0){
    numModules /= numOfPxs;
    bitStreamLength = bitStreamLength * numModules + 1;
  }else{
    bitStreamLength *= numModules;
  }

  vector<bool> bits;
  getContentFromDevice(bitStreamLength,bits);

  bool ok = true;

  if( ((moduleSet.compare("Global FCSR 0") == 0) || (moduleSet.compare("Global FCSR 1") == 0)) && chipName.compare(5,1,"F") == 0)
  {
    for(uint32_t i=0; i<numModules; i++){
      ok &= configReg->compareContent(bits,moduleSet,to_string(i),overwrite);
      cout << "Module " << i << endl;
      bits.erase(bits.begin(),bits.begin()+configReg->getNumBitsPerModule(moduleSet));
      ok &= compareGlobalDecCapBits(bits,moduleSet);
    }
  }
  else if(moduleSet.compare("Control register") == 0)
  {
    for(uint32_t i=0; i<numModules; i++){
      ok &= configReg->compareContent(bits,moduleSet,"0",false); // never overwrite pixel registers
      if(!ok)
        cout << "ASIC " << i << endl;
      showCompareErrors(configReg);
      bits.erase(bits.begin(),bits.begin()+configReg->getNumBitsPerModule(moduleSet));
    }
  }
  else
  {
    ok &= configReg->compareContent(bits,moduleSet,overwrite);
  }

  return ok;
}


bool CHIPInterface::readBackPixelChain()
{
  const string pixelModuleSet = "Control register";
  uint32_t numModules = pixelRegisters->getNumModules(pixelModuleSet);
  uint32_t numBitsPerPixel = pixelRegisters->getNumBitsPerModule(pixelModuleSet);
  uint32_t bitStreamLength = numModules * numBitsPerPixel;
  uint32_t numPacks = ceil(numModules/1024.0);
  //uint32_t numASICs = numModules/numOfPxs;

  if(numPacks>=4){
    bitStreamLength += numPacks*8;
  }else{
    bitStreamLength++;
  }

  std::vector<bool> bits;
  getContentFromDevice(bitStreamLength,bits);

  const uint32_t bitShift = 1024*c_controlBitsPerPx;
  for(uint32_t i=1; i<numPacks; i++){
    bits.erase(bits.begin() + i*bitShift,bits.begin() + i*bitShift+8); //remove one byte
  }

  return pixelRegisters->compareContent(bits,pixelModuleSet);
}


uint32_t CHIPInterface::readXORRegister()
{
  programJtagSingle("XorIns");
  programJtagSingle("XorIns",true,true,true,true);

  uint32_t xors = jtagRegisters->getSignalValue("XorIns","0","nc");

  jtagRegisters->changeSignalReadOnly("XorIns",0,false);
  jtagRegisters->setSignalValue("XorIns","all","nc",0);
  jtagRegisters->changeSignalReadOnly("XorIns",0,true);

  return xors & 0x1FFFFF;
}


std::vector<int> CHIPInterface::getRowPixels(int row)
{
  int numAsicRows = 1;
  int numRowPixels = c_pxsPerRow;
  if(getNumberOfActiveAsics() == 16){
    numAsicRows  = 2;
    numRowPixels = 512;
  }

  std::vector<int> pixels;
  for(int asicRow = 0; asicRow < numAsicRows; asicRow++){
    for (int i=0; i<numRowPixels; ++i) {
      pixels.push_back(i + row*numRowPixels);
    }
    row += 64;
  }

  return pixels;
}


std::pair<int,int> CHIPInterface::getSensorPixelCoordinates(int pixel)
{
  //Returns xy coordinate of pixel center in ??m
  //Origin is in lower left corner of the matrix close to pixel 0, x0 at the left edge of pixel 64, y at the lower edge of pixel 0
  std::pair<int,int> retval;
  if (numOfPxs < 4096)
  {
    SuS_LOG_STREAM(warning, log_id(), "getSensorPixelCoordinates(): To be checked for MM matrices!");
  }
  if (pixel > 4095 || pixel < 0)
  {
    SuS_LOG_STREAM(error, log_id(), "Pixel value (" << pixel << ") error: Must be >= 0 and <= 4095");
    retval = std::make_pair(-1,-1);
    return retval;
  }
  int row = pixel / c_pxsPerRow;
  int col = pixel % c_pxsPerRow;

  //Hexagonal pixel structure constants
  int a = 136;
  int b = 236;
  int c = 204;

  int xoffset = (row % 2) ? b/2 : b;
  int yoffset = a;

  retval = std::make_pair(xoffset + col*b,yoffset + row*c);
  return retval;
}


void CHIPInterface::setIntDACValue(int setting, bool log)
{
  if (log) {
    SuS_LOG_STREAM(info, log_id(), "Setting IntDAC to " << setting);
  }
  if(setting > 8192){
    SuS_LOG_STREAM(warning, log_id(), "Int DAC setting too large, setting to 8191");
    setting = 8191;
  }

  uint8_t VDAC_i1a_B = 255;
  //int shift = setting & 7;
  //for(int i=0; i<shift; i++){
  //  VDAL_i1a_B &= ~(1<<i);
  //}
  uint8_t lsbs = setting & 7;
  switch(lsbs) {
    //case 0: VDAC_i1a_B = 0b11111111;
    //case 1: VDAC_i1a_B = 0b11110111;
    //case 2: VDAC_i1a_B = 0b11011011;
    //case 3: VDAC_i1a_B = 0b11010011;
    //case 4: VDAC_i1a_B = 0b00111100;
    //case 5: VDAC_i1a_B = 0b00110100;
    //case 6: VDAC_i1a_B = 0b00011000;
    //case 7: VDAC_i1a_B = 0b00010000;

    case 0: VDAC_i1a_B = 0b11111111; break;
    case 1: VDAC_i1a_B = 0b11110111; break;
    case 2: VDAC_i1a_B = 0b11100111; break;
    case 3: VDAC_i1a_B = 0b11000111; break;
    case 4: VDAC_i1a_B = 0b11000011; break;
    case 5: VDAC_i1a_B = 0b10000011; break;
    case 6: VDAC_i1a_B = 0b10000001; break;
    case 7: VDAC_i1a_B = 0b10000000; break;
  }

  int invert = (setting>>3)&7;
  int VDAC_BB = 7 - invert;
  int VDAC_B = (setting>>6)&127;

  jtagRegisters->setSignalValue("Global Control Register","all","VDAC_En_i1a_B",VDAC_i1a_B);
  jtagRegisters->setSignalValue("Global Control Register","all","VDAC_Bin_B",VDAC_BB);
  jtagRegisters->setSignalValue("Global Control Register","all","VDAC_Bin",VDAC_B);

  for (auto px: pixelCurrCompCalibratedVec) {
    px = false;
  }

  programJtagSingle("Global Control Register");

  usleep(MINJTAGDELAY);
}


string CHIPInterface::getDNLTrimPixels(int quarter)
{
  if (quarter < 0 || quarter > 3){
    cout << "ERROR: quarter out of range: " << quarter << " 0-3 allowed" << endl;
    return "0";
  }

  const int numASICs = getNumberOfActiveAsics();
  if(numASICs == 1){
    int start = quarter * 1024;
    int end   = ((quarter+1)*1024)-1;
    return to_string(start)+"-"+to_string(end);
  }

  if(numASICs == 16){
    int start1 = quarter * 8192;
    int end1   = ((quarter+1)*8192)-1;
    int start2 = 32768 + (3-quarter)*8192;
    int end2   = 32768 + (4-quarter)*8192-1;
    return to_string(start1)+"-"+to_string(end1) + ";" +
           to_string(start2)+"-"+to_string(end2)    ;
  }

  cout << "WARNING!! DNLDeskew trimming function only written for 1 or 16 ASICS!!" << endl;
  return "all";
}


string CHIPInterface::powerUpQuarterPixels(int quarter)
{
  const auto inactiveStr = utils::positionVectorToList(getInactivePixels());
  const auto  nextPixelStr = getDNLTrimPixels(quarter);
  powerDownPixels("all");
  powerUpPixels(nextPixelStr);
  powerDownPixels(inactiveStr);
  programPixelRegs();

  return nextPixelStr;
}


string CHIPInterface::restorePoweredPixels()
{
  const auto inactiveStr = utils::positionVectorToList(getInactivePixels());
  //restore power down pattern
  powerUpPixels("all");
  powerDownPixels(inactiveStr);
  programPixelRegs();

  return inactiveStr;
}


string CHIPInterface::getQuarterASICPixelNumbers(int quarter, bool rowWise)
{
  if(quarter < 0 || quarter > 4){
    std::cout << "quarter out of range can not create string "<<std::endl;
    return "0";
  }

  string qurterASICStr;

  if(rowWise)
  {
    if(totalNumPxs==65536){
      qurterASICStr  = to_string((quarter+0)*(totalNumPxs/8)) + "-" + to_string((quarter+1)*(totalNumPxs/8)-1) + ";";
      qurterASICStr += to_string((quarter+4)*(totalNumPxs/8)) + "-" + to_string((quarter+5)*(totalNumPxs/8)-1);
    }else{
      qurterASICStr  = to_string((quarter+0)*(totalNumPxs/4)) + "-" + to_string((quarter+1)*(totalNumPxs/4)-1);
    }
  }
  else
  {
    static const int quarterRow = c_pxsPerRow/4;
    const int endPixel = (totalNumPxs-quarterRow);
    int startPixel = quarter * quarterRow;
    while(startPixel<=endPixel){
      qurterASICStr += to_string(startPixel) + "-" + to_string(startPixel+quarterRow-1) + ";";
      startPixel+=c_pxsPerRow;
    }
  }

  return qurterASICStr;
}


string CHIPInterface::getHalfASICPixelNumbers(int half)
{
  static const int halfRow = c_pxsPerRow/2;
  const int endPixel = (totalNumPxs-halfRow);
  string halfASICStr = "";
  int startPixel = (half==0)? 0 : halfRow;
  while(startPixel<=endPixel){
    halfASICStr += to_string(startPixel) + "-" + to_string(startPixel+halfRow-1) + ";";
    startPixel+=c_pxsPerRow;
  }
  return halfASICStr;
}



vector<uint32_t> CHIPInterface::getPoweredAsics()
{
  int numAsics = getNumberOfActiveAsics();

  if(numAsics==1){return vector<uint32_t>(1,0);}

  vector<uint32_t> poweredAsics;
  for(int asic = 0; asic<numAsics; asic++)
  {
    if(!isASICPoweredDown(asic)){
      poweredAsics.push_back(asic);
    }
  }

  return poweredAsics;
}


bool CHIPInterface::isASICPoweredDown(int asic)
{
  uint32_t imagePixel = getImagePixelNumber(asic,0);
  if(getPixelRegisterValue(to_string(imagePixel),"LOC_PWRD")==0){
    return false;
  }

  // if all pixels same value then whole ASIC is in power down
  const auto asicPixelsStr = utils::positionVectorToList(getASICPixels(to_string(asic),"all"));
  return pixelRegistersSignalIsVarious("LOC_PWRD",asicPixelsStr);
}


void CHIPInterface::powerUpSingleAsic(int asic, bool program)
{
  SuS_LOG_STREAM(info, log_id(), "Powering up ASIC " << asic << " while setting others in powerdown");

  if(getNumberOfActiveAsics() == 1){
    powerUpPixels("all", program);
    return;
  }

  powerDownPixels("all",false);
  powerUpAsic(asic,program);
}


void CHIPInterface::powerUpAsic(int asic, bool program)
{
  string asicPixelStr = utils::positionVectorToList<uint32_t>(getASICPixels(to_string(asic), "all"));
  powerUpPixels(asicPixelStr,program, nullptr);
}


/*
 * The Day0 input branch is only powered down when not in Day0 mode.
 */
void CHIPInterface::powerUpPixels(const string & pixelStr, bool program, DataPacker* packer)
{
  SuS_LOG_STREAM(debug, log_id(), "powering up pixels " << pixelStr);

  if (pixelStr.compare("all")==0) {
    poweredPixels.assign(totalNumPxs,1);
    singlePxPowered = -1;
  } else {
    auto pixels = utils::positionListToVector(pixelStr);
    for (auto px: pixels) {
      poweredPixels[px] = 1;
    }
    if (pixels.size()==1) {
      //if (singlePxPowered<0 || singlePxPowered==pixels[0]) // do nothing
      if (singlePxPowered<0) {
        singlePxPowered = pixels[0];
        SuS_LOG_STREAM(info, log_id(), "singlePxPowered = " << singlePxPowered);
      }
      else if (singlePxPowered != (int)pixels[0]) {
        SuS_LOG_STREAM(info, log_id(), "several pixels powered");
        singlePxPowered = -1;
      }
    } else if (pixels.size()>1) {
      singlePxPowered = -1;
      SuS_LOG_STREAM(info, log_id(), "pixels.size()>1 --> several pixels powered");
    }
  }
  setPowerDownBits(pixelStr, false);
  //setPixelRegisterValue(pixelStr,"LOC_PWRD", 0);
  //setPixelRegisterValue(pixelStr,"EnD0", d0Mode ? 1 : 0);
  //setPixelRegisterValue(pixelStr,"D0_BypComprResistor", d0BypCompr? 1 : 0);
  if (singlePxPowered > 0 && pixelCurrCompCalibratedVec[singlePxPowered]) {
    SuS_LOG_STREAM(info, log_id(), "Pixel " << singlePxPowered << " current compensation is already trimmed.");
    programPixelRegDirectly(singlePxPowered);
  }
  else if (recalCurrCompOnPowerUp) {
    SuS_LOG_STREAM(info, log_id(), "Retrimming pixel bias current compensation DAC. D0 mode is " << d0Mode);
    calibrateCurrCompDAC(false, -1, 0);
  } else if (program) {
    programPixelRegs();
  }
}


void CHIPInterface::powerDownPixels(const string & pixelStr, bool program)
{
  int singlePxProg = -1;
  if (pixelStr.empty()){
    return;
  }else if (pixelStr.compare("all")==0) {
    poweredPixels.assign(totalNumPxs,0);
    singlePxPowered = -1;
  } else {
    auto pixels = utils::positionListToVector(pixelStr);
    if (pixels.size()==1) singlePxProg = pixels[0];
    for (auto px: pixels) {
      poweredPixels[px] = 0;
      if ((int)px == singlePxPowered) {
        singlePxPowered = -1;
      }
    }
  }

  setPowerDownBits(pixelStr, true);

  // measurements with power down of unused pixels dont program on power down but only
  // on power up. therefore we need to program if we have a single pixel here since
  // the power up function also uses the direct function on a single pixel. otherwise
  // we would not power down this pixel.
  if (singlePxProg>-1) {
    SuS_LOG_STREAM(info, log_id(), "powering down pixel " << singlePxProg);
    if (singlePxProg > 4031) programPixelRegs();
    else programPixelRegDirectly(singlePxProg);
  }
  else if (program) {
    programPixelRegs();
  }
}


void CHIPInterface::setPowerDownBits(const std::string & pixelStr, bool powerDown)
{
  // these are the F1 bits
  if (powerDown) {
    setPixelRegisterValue(pixelStr,"LOC_PWRD", 1);
    setPixelRegisterValue(pixelStr,"EnD0", 0);
    setPixelRegisterValue(pixelStr,"D0_BypComprResistor", 0);
  } else {
    setPixelRegisterValue(pixelStr,"LOC_PWRD", 0);
    setPixelRegisterValue(pixelStr,"EnD0", d0Mode ? 1 : 0);
    setPixelRegisterValue(pixelStr,"D0_BypComprResistor", d0BypCompr? 1 : 0);
  }
}


bool CHIPInterface::checkD0Mode()
{
  d0ModeRem = pixelRegisters->getSignalValue("Control register","0","EnD0") != 0;
  return d0ModeRem;
}

bool CHIPInterface::checkBypCompression()
{
  d0BypComprRem = pixelRegisters->getSignalValue("Control register","0","D0_BypComprResistor") != 0;
  return d0BypComprRem;
}



int CHIPInterface::getIntDACValue()
{
  uint8_t VDAC_i1a_B = jtagRegisters->getSignalValue("Global Control Register","0","VDAC_En_i1a_B");
  uint8_t VDAC_Bin_B = jtagRegisters->getSignalValue("Global Control Register","0","VDAC_Bin_B");
  uint8_t VDAC_Bin   = jtagRegisters->getSignalValue("Global Control Register","0","VDAC_Bin");

  int val = 0;

  switch(VDAC_i1a_B) {
    case 0b11111111 : val += 0; break;
    case 0b11110111 : val += 1; break;
    case 0b11100111 : val += 2; break;
    case 0b11000111 : val += 3; break;
    case 0b11000011 : val += 4; break;
    case 0b10000011 : val += 5; break;
    case 0b10000001 : val += 6; break;
    case 0b10000000 : val += 7; break;
    default : SuS_LOG_STREAM(warning, log_id(), "Error in internal DAC LSB coding.");
  }

  val += (uint8_t)((7 & ~VDAC_Bin_B) << 3);
  val += VDAC_Bin << 6;

  return val;
}


int CHIPInterface::checkCurrCompDACCalibration(string & status, vector<uint32_t> & unsettledPxs)
{
  checkLadderReadout();

  ofstream currentCompTrimLogStream("CalibrateCurrCompDACLog.txt",std::ofstream::app);

  unsettledPxs.clear();

//   if(chipName.compare("DSSC_F1") == 0){
//     if(!pixelRegisters->signalIsVarious("Control register","CompCoarse","all")){
//         status = "All Regs Same Value!";
//         currentCompTrimLogStream << status << endl;
//         return 0;
//     }
//   }

  bool someTrimmed = false;
  bool allTrimmed = true;

  vector<int>  rmpFineTrm(totalNumPxs,0);        // to restore ADC gain
  vector<bool> rmpCurrDouble(totalNumPxs,false);  // to restore ADC gain

  if(!m_leaveMySettings)
  {
    for (int px=0; px<totalNumPxs; ++px) {
      rmpCurrDouble[px] = getPixelRegisterValue(to_string(px),"RmpCurrDouble");
      rmpFineTrm[px]    = getPixelRegisterValue(to_string(px),"RmpFineTrm");
    }

    setPixelRegisterValue("all","RmpCurrDouble",1);
    setPixelRegisterValue("all","RmpFineTrm",31);

    programPixelRegs();
  }

  {
    ROModeKeeper keepMeUntilEndOfScope(false,false,false,1/*800*/,0,this);
    // check 10 times
    for(int it=0; (it<10 && runTrimming); it++)
    {
      if (!doSingleCycle()) {
        status = "+++++ DATA RECEIVER ERROR check Currentcompensation aborted";
        currentCompTrimLogStream << status << endl;
        return -10;
      }

      {
        lock_guard<mutex> lock (pixelDataMutex);
        int i=0;
        for (int px = 0; px < totalNumPxs; px++)
        {
          const auto pixelData = getPixelSramData(px);
          const uint16_t & vhold = *pixelData;
          int rank = getRanking(vhold,px,true);
          if( rank > 0 ) {
            someTrimmed = true;
          }
          else if (!untrimmablePixels[px] && poweredPixels[px] && hasCurrCompDac(px))
          {
            if(i<10){
              int regValue = getPixelRegisterValue(to_string(px),"CompCoarse");
              stringstream ss;
              ss << "It " << it << " Pixel " << setw(4) << px << ": " << "VHold not settled at " << setw(3) << vhold <<  " ADU with setting " << regValue;
              currentCompTrimLogStream    << ss.str() << endl;
              SuS_LOG_STREAM(info, log_id(), ss.str());
            }
            allTrimmed = false;
            i++;
            unsettledPxs.push_back(px);
          }
        }
      } // release lock
    }
  } // destroy keeper

  //remove duplicates
  utils::removeDuplicates(unsettledPxs);

  if(!m_leaveMySettings)
  {
    for (int px=0; px<totalNumPxs; ++px) {
      setPixelRegisterValue(to_string(px),"RmpCurrDouble",rmpCurrDouble[px]);
      setPixelRegisterValue(to_string(px),"RmpFineTrm",rmpFineTrm[px]);
    }
    programPixelRegs();
  }

  if(untrimmablePixelsFound){
    status = "Untrimmable Pixels found : " + utils::positionVectorToList(getUntrimmablePixels());
    return -3;
  }

  if(allTrimmed){
    status = "All Pixels trimmed";
    currentCompTrimLogStream << status << endl;
    return 2;
  }

  if(someTrimmed){
    status = to_string(unsettledPxs.size()) + " Untrimmed Pixels found: " + utils::positionVectorToList(unsettledPxs);
    currentCompTrimLogStream << status << endl;
    return 1;
  }

  status = "Not trimmed";
  currentCompTrimLogStream << status << endl;
  return -1;
}

const std::vector<uint16_t> & CHIPInterface::measureVHoldValues(bool & ok)
{
  SuS_LOG_STREAM(info, log_id(), "Measure VHold Values");
  {
    VoldMeasurementKeeper keeper(this);
    ok = doSingleCycle();
    if(ok){
      updateVholdVals();
    }
  }
  return currentVholdVals;
}


void CHIPInterface::updateVholdVals()
{
  lock_guard<mutex> lock (pixelDataMutex);
  for (int px=0; px<totalNumPxs; ++px) {
    //currentVholdVals[px] = unpoweredPxs[px] ? -1 : *(getPixelSramData(px));
    if (poweredPixels[px]) {
      currentVholdVals[px] = *(getPixelSramData(px));
      if(currentVholdVals[px] < GCCWRAP){
        currentVholdVals[px] += 256;
      }
    }
  }
}


bool CHIPInterface::calibratePXInjCurrCompDAC()
{
  if(!m_leaveMySettings){
    setPixelRegisterValue( "all", "RmpCurrDouble", 1);
    setPixelRegisterValue( "all", "RmpFineTrm", 31);
  }
  setPixelRegisterValue( "all", "ADC_EnExtLatch", 0);
  setPixelRegisterValue( "all", "BypassFE", 0);
  setPixelRegisterValue( "all", "FCF_EnCap", 3);

  setPixelRegisterValue( "all", "EnPxInjDC", 1);
  setPixelRegisterValue( "all", "EnD0", 0);
  setPixelRegisterValue( "all", "InvertInject", 0);

  programPixelRegs();

  return calibrateCurrCompDAC();
}


string CHIPInterface::getPowerDwnPixelsStr()
{
  return utils::positionVectorToList(getPowerDwnPixels());
}


string CHIPInterface::getPoweredPixelsStr()
{
  return utils::positionVectorToList(getPoweredPixels());
}


const std::vector<uint32_t> CHIPInterface::getPoweredPixelsFromConfig()
{
  std::vector<uint32_t> poweredConfigPixels;
  for (int px=0; px<totalNumPxs; ++px){
    if((getPixelRegisterValue(to_string(px),"LOC_PWRD")==1)? 0 : 1){
      poweredConfigPixels.push_back(px);
    }
  }
  return poweredConfigPixels;
}


const vector<uint32_t> & CHIPInterface::updatePoweredPixels()
{
  onPixels.clear();
  if (getSinglePixelReadoutMode()) {
    onPixels.push_back(singlePixelToReadout);
  } else {
    for (int px=0; px<totalNumPxs; ++px){
      poweredPixels[px] = (getPixelRegisterValue(to_string(px),"LOC_PWRD")==1)? 0 : 1;
      if(poweredPixels[px]){
        onPixels.push_back(px);
      }
    }
  }
  return onPixels;
}


vector<uint32_t> CHIPInterface::getInactivePixels()
{
  vector<bool> offPixels(totalNumPxs,true);
  for(const auto & onPix : onPixels){
    offPixels[onPix] = false;
  }

  vector<uint32_t> inactivePixels;
  int idx = 0;
  for(const auto & offPixel : offPixels){
    if(offPixel){
      inactivePixels.push_back(idx);
    }
    idx++;
  }

  return inactivePixels;
}


vector<uint32_t> CHIPInterface::getPowerDwnPixels()
{
  vector<uint32_t> offPixels;
  int px = 0;
  for(const auto & poweredPix : poweredPixels){
    if(!poweredPix){
      offPixels.push_back(px);
    }
    px++;
  }
  return offPixels;
}


vector<uint32_t> CHIPInterface::getUntrimmablePixels() const
{
  vector<uint32_t> unTrimPixels;
  int px = 0;
  for(const auto & untrimPix : untrimmablePixels){
    if(untrimPix){
      unTrimPixels.push_back(px);
    }
    px++;
  }
  return unTrimPixels;
}


int CHIPInterface::getBestSetting(const vector<int> & eval)
{
  int max = 0;
  int idx = 0;
  int bestIdx = 0;

  for(const auto value : eval){
    if(value>max){
      max = value;
      bestIdx = idx;
    }
    idx++;
  }

  return bestIdx;
}

// function is required in large vhold trimming function to remember upper limit for vhold
void CHIPInterface::setUpperVHoldVal(uint16_t vhold, int px)
{
  const int maxADU = getMaxADU() - 10;

  if(vhold<GCCWRAP){
    vhold += 256;
  }

  if(vhold < 150){
    vhold = 150;
  }

  if(maxADU < vhold ){
    vhold = maxADU;
  }else{
    vhold -= 10; //margin
  }

  upperVHoldVals[px] = vhold;
}


// is called at startup in CHIPCalibrationBox
void CHIPInterface::initUpperVHoldVals()
{
  int maxADU = getMaxADU();
  if(maxADU > 350){
    maxADU = 350;
  }

  for(auto & value : upperVHoldVals){
    value = maxADU;
  }
}


void CHIPInterface::checkADCLimit()
{
  const uint32_t numPxsToTrim = pixelRegisters->getModules("Control register").size();

  setPixelRegisterValue( "all", "CompCoarse",15);

  programPixelRegs();

  if (!doSingleCycle()) {
    SuS_LOG_STREAM(error, log_id(), "+++++ DATA RECEIVER ERROR checkADCLimit aborted.");
    return;
  }

  {
    lock_guard<mutex> lock (pixelDataMutex);

    for (uint32_t px = 0; px < numPxsToTrim; px++)
    {
      if (poweredPixels[px] && hasCurrCompDac(px)) {
        uint16_t vhold = *(getPixelSramData(px));
        setUpperVHoldVal(vhold,px);
      }
    }
  }


  int minVHold = *(std::min(upperVHoldVals.begin(), upperVHoldVals.end()));

  ofstream currentCompTrimLogStream("CalibrateCurrCompDACLog.txt");
  currentCompTrimLogStream << "DAC Trimming lowest Limit for all pixels is " << minVHold << endl;
  SuS_LOG_STREAM(info, log_id(), "DAC Trimming lowest Limit for all pixels is " << minVHold);

  if(minVHold<350){
    SuS_LOG_STREAM(info, log_id(), "WARNING: Really small trimming range found. Will probaby find untrimmable pixels! Set small ADC gain or longer Iramp phase");
  }
}


int CHIPInterface::getRanking(uint16_t value, int pixel, bool check)
{
  const int GCCOFFS = jtagRegisters->getSignalValue("Global Control Register","0","GCC_StartVal_0")*2;

  int TOP = 100;
  int margin = (check)? 10 : 0;

  int maxLimit = upperVHoldVals[pixel];

  int interValSize = (maxLimit-TOP)/4;
  int limit = interValSize+TOP;

  //dynamic limits
  vector<int> limits;
  for(int i=0; i<3; i++){
    limits.push_back(limit);
    limit+=interValSize;
  }

  int rank = 0;
  if(value<GCCWRAP){// 0 - GCCWRAP
    rank = 2;
  }else if(value<(15+GCCOFFS)){ // GCCWRAP - 14
    rank = -1;
  }else if(value<(30+GCCOFFS)){ // 15 - 30
    rank = 8;
  }else if(value<(TOP+GCCOFFS)){ // 30 - 100
    rank = 10;
  }else if(value<(limits[0]+GCCOFFS)){ // 100 - 134
    rank = 9;
  }else if(value<(limits[1]+GCCOFFS)){ // 135 - 189
    rank = 7;
  }else if(value<(limits[2]+GCCOFFS)){ // 190 - 244
    rank = 6;
  }else if(value<(maxLimit+margin)){ // 244 - 499
    rank = 5;
  }else{ // 400 - 511
    rank = -2;
  }

  return rank;
}


void CHIPInterface::checkVholdBeforeTrimming(vector<uint32_t> & calibPixels, int vHoldMax)
{
  int numPixelsStart = calibPixels.size();

  bool ok;
  measureVHoldValues(ok);
  if(ok){
    for(int pxIdx = calibPixels.size()-1; pxIdx >= 0; --pxIdx)
    {
      if(currentVholdVals[calibPixels[pxIdx]]<=vHoldMax){
        calibPixels.erase(calibPixels.begin()+pxIdx);
      }
    }
  }

  SuS_LOG_STREAM(info, log_id(), calibPixels.size() <<" of " << numPixelsStart << " pixels are not trimmed");
}



void CHIPInterface::decrCompCoarseForVHOLDValue(uint32_t startCompCoarse, int vHoldMax, int singlePx)
{
  SuS_LOG_STREAM(info, log_id(), "\n\nTrim CurrCompDAC by decreasing setting CompCoarse");

  checkLadderReadout();

  //to avoid False GCC Wrap application for low values start with 5 or above
  setGGCStartValue(DEFAULTGCCSTART);

  const uint32_t numFrames = (isLadderReadout()? getNumFramesToSend() : 1);
  const uint32_t numPxsToTrim = pixelRegisters->getModules("Control register").size();

  untrimmablePixels.assign(numPxsToTrim,false);
  untrimmablePixelsFound = false;

  updatePoweredPixels();

  vector<uint32_t> calibPixels;
  if (singlePx==-1) { // all pixels
    for (uint32_t px=0; px<numPxsToTrim; ++px) {
      if (poweredPixels[px] && hasCurrCompDac(px)) {
        calibPixels.emplace_back(px);
      }
    }
  }else if(singlePx >= (int)numPxsToTrim){
    SuS_LOG_STREAM(error, log_id(), "Trim CurrCompDAC() pixel (" << singlePx << ") out of range. Nothing done.");
  }else{
    calibPixels.emplace_back(singlePx);
  }

  checkVholdBeforeTrimming(calibPixels,vHoldMax);

  int run=0;

  if(!calibPixels.empty())
  {
    const string pxStr = utils::positionVectorToList(calibPixels);
    setPixelRegisterValue( pxStr, "CompCoarse",startCompCoarse);

    {//keep readout modes and reset them during destruction
      ROModeKeeper keepMeUntilEndOfScope(false,false,false,numFrames,0,this);

      //loop until all pixels settled
      int toTrim = calibPixels.size();
      while(toTrim > 0 && run < 20)
      {
        toTrim = decrCompCoarseForVHOLDValue(vHoldMax+run,calibPixels);
        // increase vholMax to settle faster,
        // limit is then not reached completely but small variations are ok
        SuS_LOG_STREAM(info, log_id(), "Compcoarse Trimming run " << run++ << " pixels left: " << toTrim);
      }

    }//ROKeeper is destroyed here

    decreaseAllCompCoarseByOne(calibPixels);

    bool ok;
    measureVHoldValues(ok);
    if(!ok){
      calibPixels.clear();
    }
  }

  if(run == 0){
    SuS_LOG_STREAM(info, log_id(), "Nothing to trim");
  }

  string status = "Current Compensation done after " +to_string(run) + " runs with vHoldrange = ";
  status += utils::getMinMaxStatsStr(currentVholdVals);
  SuS_LOG_STREAM(info, log_id(), status);

  printCompCoarseStats();
}

void CHIPInterface::printCompCoarseStats()
{
  vector<uint32_t> compCoarseVals = pixelRegisters->getSignalValues("Control register","all","CompCoarse");
  string status = "CompCoarse Values in range = " + utils::getMinMaxStatsStr(compCoarseVals);
  SuS_LOG_STREAM(info, log_id(), status);
}


void CHIPInterface::decreaseAllCompCoarseByOne(const vector<uint32_t> & calibPixels)
{
  for(const auto & px:calibPixels){
    auto value = getPixelRegisterValue(to_string(px),"CompCoarse");
    if(value > 0){
      setPixelRegisterValue(to_string(px),"CompCoarse",value-1);
    }
  }
  programPixelRegs();
}



uint32_t CHIPInterface::decrCompCoarseForVHOLDValue(int vHoldMax, const std::vector<uint32_t> & calibPixels)
{
  const string pxStr = utils::positionVectorToList(calibPixels);

  bool ok = false;
  measureVHoldValues(ok);
  if(!ok) return 0;

  int showCnt = 0;
  uint32_t changedPixels = 0;
  vector<uint32_t> limitPixels;
  for(const auto & px : calibPixels){
    if(currentVholdVals[px]>vHoldMax){
      int value = getPixelRegisterValue( to_string(px), "CompCoarse") - 1;
      if(value<0){
        limitPixels.push_back(px);
      }else{
        changedPixels++;
        setPixelRegisterValue( to_string(px), "CompCoarse",value);
      }
      if(showCnt<10){
        cout << "Not settled pixel: "<< setw(5) << px;
        cout << " vHold: "           << setw(3) << currentVholdVals[px];
        cout << " CompCoarse: "      << setw(3) << value +1 << endl;
        showCnt++;
      }
    }
  }

  programPixelRegs();

  if(limitPixels.size()>0){
    SuS_LOG_STREAM(warning, log_id(), to_string(limitPixels.size()) + "Pixels already at lower limit");
  }

  SuS_LOG_STREAM(info, log_id(), "Decreased CompCoarse in " << changedPixels << " pixels");

  return changedPixels;
}


void CHIPInterface::selectBestCurrentCompSetting(ofstream & logStream, const vector< vector<uint16_t> > & vHoldMap, const vector<uint32_t> & unsettledPixels)
{
  ofstream currentCompTrimLogStream("CalibrateCurrCompDACLog.txt",std::ofstream::app);

  int idx = 0;
  for(const auto & vHoldVec : vHoldMap)
  {
    const uint32_t px  = unsettledPixels[idx];
    setUpperVHoldVal(vHoldVec[15],px);

    vector<int> eval(16,0);
    stringstream ss;
    ss << "VHold Map Px " << setw(4) << px;
    ss << " max " << setw(3) << upperVHoldVals[px] << " : ";

    for(int i=0; i<16; i++)
    {
      uint16_t vHold = vHoldVec[i];
      eval[i] = getRanking(vHold,px);

      ss << setw(2) << i << ":" << setw(3) << vHold <<  "/" << setw(2) << eval[i] << "  ";
    }

    currentCompTrimLogStream << ss.str()  << endl;

    int bestSetting = getBestSetting(eval);
    setPixelRegisterValue( to_string(px), "CompCoarse",bestSetting);

    if(eval[bestSetting] < 0)
    {
      stringstream out;
      out << "+++ Warning no valid setting for Pixel " << setw(4) << px << " found. Will be disabled during further analysis";
      currentCompTrimLogStream << out.str()  << endl;
      SuS_LOG_STREAM(warn, log_id(), out.str());

      untrimmablePixelsFound = true;
      untrimmablePixels[px] = true;
    }

    idx++;
  }
}


void CHIPInterface::calibrateCurrCompDAC(std::vector<uint32_t> unsettledPxs, int startSetting, int singlePx, bool log)
{
  runTrimming = true;
  ofstream currentCompTrimLogStream("CalibrateCurrCompDACLog.txt",std::ofstream::app);

  vector< vector<uint16_t> > vHoldMap(unsettledPxs.size());
  const string pxStr = utils::positionVectorToList(unsettledPxs);

  for(int compCoarse = 0; (compCoarse < 16 && runTrimming) ; compCoarse++)
  {
    SuS_LOG_STREAM(info, log_id(), "Current Compensation trimming measure setting " << compCoarse);

    setPixelRegisterValue( pxStr, "CompCoarse",compCoarse);
    // program Pixel registers
    if (singlePx>-1) programPixelRegDirectly(singlePx);
    else if (singlePxPowered>-1) programPixelRegDirectly(singlePxPowered);
    else
    {
      programPixelRegs();
    }

    if (!doSingleCycle()) {
      currentCompTrimLogStream     << "+++++ DATA RECEIVER ERROR Current Compensation DAC trimming aborted." << endl;
      SuS_LOG_STREAM(error, log_id(), "+++++ DATA RECEIVER ERROR Current Compensation DAC trimming aborted.");
      return;
    }

    {
      lock_guard<mutex> lock (pixelDataMutex);
      int idx = 0;
      for (const auto & px : unsettledPxs)
      {
        const uint16_t & vhold = *(getPixelSramData(px));
        vHoldMap[idx++].push_back(vhold);
      }
    }
  }

  selectBestCurrentCompSetting(currentCompTrimLogStream,vHoldMap,unsettledPxs);

  // program Pixel registers
  if (singlePx>-1) programPixelRegDirectly(singlePx);
  else if (singlePxPowered>-1) programPixelRegDirectly(singlePxPowered);
  else
  {
    programPixelRegs();
    usleep(100000);
  }
}


bool CHIPInterface::calibrateCurrCompDAC(bool log, int singlePx, int startSetting, int defaultValue)
{
  runTrimming = true;

  static const int TO = 5;

  checkLadderReadout();

  //to avoid False GCC Wrap application for low values start with 5 or above
  setGGCStartValue(DEFAULTGCCSTART);

  int notSettledPixels;
  if (singlePxPowered>-1 && log) {
    SuS_LOG_STREAM(error, log_id(), "Only a single pixel is powered, using direct pixel programming.");
  }

  untrimmablePixels.assign(totalNumPxs,false);
  untrimmablePixelsFound = false;

  updatePoweredPixels();

  {//keep readout modes and reset them during destruction
    ROModeKeeper keepMeUntilEndOfScope(false,false,false,1/*800*/,0,this);

    vector<uint32_t> unsettledPxs;
    vector<int>  rmpFineTrm(totalNumPxs,0);        // to restore ADC gain
    vector<bool> rmpCurrDouble(totalNumPxs,false);  // to restore ADC gain
    if (singlePx==-1) { // all pixels
      for (int px=0; px<totalNumPxs; ++px) {
        if (poweredPixels[px] && hasCurrCompDac(px)) {
          rmpCurrDouble[px] = getPixelRegisterValue(to_string(px),"RmpCurrDouble");
          rmpFineTrm[px]    = getPixelRegisterValue(to_string(px),"RmpFineTrm");
          unsettledPxs.push_back(px);
        }
      }
    }else if(singlePx >= (int)totalNumPxs){
      SuS_LOG_STREAM(error, log_id(), "Trim CurrCompDAC() pixel (" << singlePx << ") out of range. Nothing done.");
    }else{
      unsettledPxs.push_back(singlePx);
    }

    if(unsettledPxs.size()> 0)
    {
      const string pxStr = utils::positionVectorToList(unsettledPxs);
      //SuS_LOG_STREAM(debug, log_id(), "Trim CurrCompDAC(): pxStr is " << pxStr);

      if(!m_leaveMySettings)
      {
        setPixelRegisterValue( pxStr, "RmpCurrDouble", 1);
        setPixelRegisterValue( pxStr, "RmpFineTrm", 31);
        //programPixelRegs(); // is already done later
      }

      //clears also LOGFile
      checkADCLimit();

    }else{
      SuS_LOG_STREAM(warning, log_id(), "Trim CurrCompDAC(): no pixels to process.");
    }

    int run=0;
    // check calibration status sometimes early-settled pixels do not remain in valid range
    while(unsettledPxs.size() > 0 && run < TO && runTrimming)
    {
      calibrateCurrCompDAC(unsettledPxs,startSetting,singlePx,log);

      std::string status;
      checkCurrCompDACCalibration(status,unsettledPxs);

      if(untrimmablePixelsFound){
        const auto untrimPixels = getUntrimmablePixels();
        SuS_LOG_STREAM(warning, log_id(), "+++++ Untrimmable pixels found after run " << run <<" ignoring "<< untrimPixels.size() <<" pixels in next round: " << utils::positionVectorToList(untrimPixels));
      }

      run ++;

      if(unsettledPxs.size() > 0){
        if(run < TO){
          SuS_LOG_STREAM(warning, log_id(), "+++++ Unsettled pixels found after run " << run <<". Sending "<< unsettledPxs.size() << " pixels in next run : " << utils::positionVectorToList(unsettledPxs));
        }else{
          SuS_LOG_STREAM(warning, log_id(), "+++++ Unsettled pixels found after run " << run <<". Could not trim " << unsettledPxs.size() << " pixels: " << utils::positionVectorToList(unsettledPxs));
        }
      }else{
        SuS_LOG_STREAM(info, log_id(), "+++++ All pixels settled after run " << run);
      }
    }

    notSettledPixels = unsettledPxs.size();

    if (notSettledPixels > 0  &&  log)
    {
      SuS_LOG_STREAM(warning, log_id(), unsettledPxs.size() << " pixels have not settled.");
      SuS_LOG_STREAM(warning, log_id(), "Unsettled pixels are: " << utils::positionVectorToList(unsettledPxs));

      setPixelRegisterValue( utils::positionVectorToList(unsettledPxs), "CompCoarse", defaultValue);

      if (singlePx > -1){
        programPixelRegDirectly(singlePx);
      }else if (singlePxPowered > -1){
        programPixelRegDirectly(singlePxPowered);
      }else{
        programPixelRegs();
      }
    }

    // final check
    if (!doSingleCycle()) {
      SuS_LOG_STREAM(error, log_id(), "+++++ DATA RECEIVER ERROR Current Compensation DAC trimming aborted.");
      return false;
    }
    updateVholdVals();

    //QString shutDownPixels;
    //if (singlePx<0) {
    //  for (int px=0; px<totalNumPxs; ++px) {
    //    const uint16_t vhold = *(getPixelSramData(px));
    //    if (!(vhold > 50 && vhold < 200) && !unpoweredPxs[px]) {
    //      auto coarseSetting = getPixelRegisterValue(to_string(px),"CompCoarse");
    //      SuS_LOG_STREAM(warning, log_id(), "Pixel " << px << ": " << "VHold settled at " << vhold << " with setting " << coarseSetting);
    //    } else if (debugMode || log) {
    //      if (unpoweredPxs[px]) {
    //        shutDownPixels += to_string(px) + ";";
    //        //SuS_LOG_STREAM(info, log_id(), "Pixel " << px << ": is shut down.");
    //      } else {
    //        auto coarseSetting = getPixelRegisterValue(to_string(px),"CompCoarse");
    //        SuS_LOG_STREAM(info, log_id(), "Pixel " << px << ": " << "VHold settled at " << vhold << " with setting " << coarseSetting);
    //      }
    //    }
    //  }
    //}
    //if (debugMode || log) {
    //  SuS_LOG_STREAM(info, log_id(), "Pixels " << shutDownPixels.toStdString() << " are shut down.");
    //}

    if(!m_leaveMySettings)
    {
      // restore ADC gain
      for (int px=0; px<totalNumPxs; ++px) {
        if (poweredPixels[px] && hasCurrCompDac(px)) {
          setPixelRegisterValue(to_string(px),"RmpCurrDouble",rmpCurrDouble[px]);
          setPixelRegisterValue(to_string(px),"RmpFineTrm",rmpFineTrm[px]);
        }
      }

      if (singlePx>-1) programPixelRegDirectly(singlePx);
      else if (singlePxPowered>-1) programPixelRegDirectly(singlePxPowered);
      else programPixelRegs();
    }
  }//ROKeeper is destroyed here

  if (log) {
    writeDACCalibrateLog();

  } // if (log)

  return notSettledPixels==0;
}


void CHIPInterface::writeDACCalibrateLog()
{
  ofstream out("CurrCompMap.txt");
  if (!out.is_open()){
    SuS_LOG_STREAM(error, log_id(), "File error: file not opened!");
    return;
  }

  out << "#### Columns\t";
  out << ((numOfPxs>505)? "64" : "8") << "\t" << endl;
  out << "#Pixel\t";
  out << "CoarseComp\t";
  out << "DigVHold\t" << endl;
  for (int px=0; px<totalNumPxs; ++px) {
    const auto coarse = getPixelRegisterValue(to_string(px),"CompCoarse");
    out << px << "\t";
    out << coarse << "\t";
    out << currentVholdVals[px];
    out << "\t\n";
  }
  out.close(); // is also called by destructor
}


bool CHIPInterface::calibrateCurrCompDAC(std::vector<std::pair<int, double> >& vholdVals, bool log, int singlePx, int startSetting, int finalIterations)
{
  vholdVals.clear();
  bool retVal = calibrateCurrCompDAC(log, singlePx, startSetting);

  lock_guard<mutex> lock (pixelDataMutex);
  if (singlePx < 0) {
    for (int px=0; px<totalNumPxs; ++px) {
      const uint16_t vhold = *(getPixelSramData(px));
      int currCompSetting = getPixelRegisterValue(to_string(px),"CompCoarse");
      vholdVals.push_back(std::pair<int, double>(currCompSetting, vhold));
    }
  } else {
    double vhold = 0;
    for (int i=0; i<finalIterations; ++i) {
      if (!doSingleCycle()){
        SuS_LOG_STREAM(error, log_id(), "+++++ DATA RECEIVER ERROR calibrateCurrCompDAC aborted");
        return false;
      }
      //SuS_LOG_STREAM(info, log_id(), "VHold was " << *(getPixelSramData(singlePx)));
      auto value = *(getPixelSramData(singlePx));
      if(value < GCCWRAP){
        value +=256;
      }
      vhold += value;
    }
    vhold /= finalIterations;
    int currCompSetting = getPixelRegisterValue(to_string(singlePx),"CompCoarse");
    vholdVals.push_back(std::pair<int, double>(currCompSetting, vhold));
  }

  return retVal;
}


void CHIPInterface::removeErrorPixelsFromVector(const std::vector<bool> & errorPixels, std::vector<uint32_t> & unsettledPixels)
{
  sort(unsettledPixels.begin(),unsettledPixels.end());

  std::vector<uint32_t> removedPixels;
  for(uint32_t px=0; px<errorPixels.size(); px++)
  {
    if(errorPixels[px]){
      auto it = find(unsettledPixels.begin(),unsettledPixels.end(),px);
      if(it != unsettledPixels.end()){
        unsettledPixels.erase(it);
        removedPixels.push_back(px);
      }
    }
  }

  if(!removedPixels.empty()){
    cout << "+++++ Sweep data problems: removed pixels from trimming because of falling slope or error code in data: " << utils::positionVectorToList(removedPixels) << endl;
  }else{
    cout << "+++++ Sweep data correct!!" << endl;
  }
}


std::vector<std::vector<double>> CHIPInterface::measurePixelParam(const string & signalName, vector<uint32_t> measurePixels, const vector<uint32_t> & paramSettings, int STARTADDR, int ENDADDR)
{
  const string pixelStr = utils::positionVectorToList(measurePixels);

  vector<vector<double>> binValues(measurePixels.size(),std::vector<double>(paramSettings.size(),0.0));

  int settingIdx = 0;
  for(const auto & setting : paramSettings)
  {
    setPixelRegisterValue(pixelStr,signalName,setting);
    programPixelRegs();

    const auto measuredData = measureBurstData(measurePixels,STARTADDR,ENDADDR,baselineValuesValid);
    // init all binVals to 0
    int idx = 0;
    for(const auto & px : measurePixels){
      binValues[px][settingIdx] = measuredData[idx++];
    }

    settingIdx++;

    if(!runTrimming)break;
  }

  return binValues;
}


vector<uint32_t> CHIPInterface::measurePedestalsOverCompCoarse(vector<uint32_t> measurePixels, vector<vector<double>> & binValues, int STARTADDR, int ENDADDR)
{
  const double numVals  = (ENDADDR - STARTADDR + 1) * m_iterations;
  const string pixelStr = utils::positionVectorToList(measurePixels);
  vector<uint32_t> untrimPixels;

  for(int compCoarse = 0; compCoarse < 16; compCoarse++)
  {
    setPixelRegisterValue(pixelStr,"CompCoarse",compCoarse);
    programPixelRegs();

    usleep(300000);

    for(uint32_t it=0; it < m_iterations ; it++)
    {
      if(!doSingleCycle()){
        SuS_LOG_STREAM(error, log_id(), "+++++ DATA RECEIVER ERROR measurePedestalsOverCompCoarse aborted");
        return untrimPixels;
      }

      {
        lock_guard<mutex> lock (pixelDataMutex);
        for(const auto & px : measurePixels)
        {
          const auto sramData = getPixelSramData(px);
          double mean = 0.0;
          for(int sram=STARTADDR; sram<=ENDADDR; sram++){
            const auto & value = sramData[sram];
            if(value < GCCWRAP){
              mean += value + 256;
            }else if (value == 511 && compCoarse > 3){

            }else{
              mean += value;
            }
          }
          binValues[px][compCoarse] += mean;
        }
      }

      if(!runTrimming){compCoarse = 16; break;}
    }

    for(const auto & px : measurePixels){
      binValues[px][compCoarse] /= numVals;
    }

    if(compCoarse == 0)
    {
      // check for untrimmable pixels
      for(const auto & px : measurePixels){
        if(binValues[px][compCoarse]<15){
          untrimPixels.push_back(px);
        }
      }
      if(untrimPixels.empty()){
        SuS_LOG_STREAM(info, log_id(), "IntDACValue " << getIntDACValue() << " good, no untrimmable pixels found");
      }else{
        SuS_LOG_STREAM(error, log_id(), "Can not trim all pixels for current int dac value " << getIntDACValue() << ". " << untrimPixels.size() << " untrimmable pixels found: " << utils::positionVectorToList(untrimPixels));
      }
    }
    else
    {
      // check if pixel trimming can be aborted:
      for(int pxIdx=measurePixels.size()-1; pxIdx>=0; --pxIdx)
      {
        uint32_t px = measurePixels[pxIdx];

        if(binValues[px][compCoarse]<15){
          measurePixels.erase(measurePixels.begin()+pxIdx);
        }
      }
    }

    if(measurePixels.empty()){
      SuS_LOG_STREAM(info, log_id(), "All Pixels Pedestals at minimum start to find best setting");
      break;
    }
  }

  return untrimPixels;
}

//not really working, can maybe improved by several runs
void CHIPInterface::trimCompCoarseSingleIntegration(int STARTADDR, int ENDADDR)
{
  setSequencerMode(Sequencer::SINGLEINT);

  setIntegrationTimeBaseline(STARTADDR,ENDADDR,0);

  updatePoweredPixels();

  string pixelStr = utils::positionVectorToList(onPixels);
  vector<vector<double>> remainingCurrentValues(totalNumPxs,vector<double>(16,0.0));

  for(int compCoarse = 0; compCoarse < 16; compCoarse++)
  {
    setPixelRegisterValue(pixelStr,"CompCoarse",compCoarse);
    programPixelRegs();
    const auto binValues = measureBurstData(onPixels,STARTADDR,ENDADDR,true);

    int idx=0;
    for(const auto & px : onPixels){
      remainingCurrentValues[px][compCoarse] = binValues[idx++];
    }
    if(!runTrimming) break;
  }

  utils::DataHisto diffHisto;

  int cnt = 0;
  for(const auto & px : onPixels)
  {
    auto bestSetting = findBestCompCoarseSettingFromSingleInt(remainingCurrentValues[px]);

    if(cnt++ < 64){
      cout << "Pixel " << setw(5) << px << " best Setting = " << setw(2) << bestSetting << ": ";
      for(const auto & value : remainingCurrentValues[px]){
        cout << setw(8) << value << " ";
      }
      cout << endl;
    }

    diffHisto.add(remainingCurrentValues[px][bestSetting]);
    setPixelRegisterValue(to_string(px),"CompCoarse",bestSetting);
  }

  programPixelRegs();

  diffHisto.print();

  setSequencerMode(Sequencer::NORM);
}


uint32_t CHIPInterface::findBestCompCoarseSettingFromSingleInt(vector<double> values)
{
  for(auto && value : values){
    value = fabs(value);
  }
  return std::min_element(values.begin(),values.end())-values.begin();
}


vector<uint32_t> CHIPInterface::trimCompCoarseByPedestal(const vector<uint32_t> & pixelsToTrim, vector<vector<double>> & binValues, int STARTADDR, int ENDADDR)
{
  runTrimming = true;

  auto untrimPixels = measurePedestalsOverCompCoarse(pixelsToTrim,binValues,STARTADDR,ENDADDR);
  untrimPixels.clear();

  int cnt=0;
  for(const auto & px : pixelsToTrim)
  {
    auto bestSetting = findBestCompCoarseSettingFromPedestal(binValues[px]);

    if(cnt++ < 64){
      cout << "Pixel " << setw(5) << px << " best Setting = " << setw(2) << bestSetting << ": ";
      for(const auto & value : binValues[px]){
        cout << setw(8) << value << " ";
      }
      cout << endl;
    }

    if(bestSetting>15){
      untrimPixels.push_back(px);
    }else{
      setPixelRegisterValue(to_string(px),"CompCoarse",bestSetting);
    }
    if(!runTrimming) break;
  }

  return untrimPixels;
}


vector<uint32_t> CHIPInterface::getPixelsToTrimForCompCoarseByPedestal(int run, const int NUM)
{
  vector<uint32_t> runPixels;
  for(int px = run; px<totalNumPxs; px+=NUM){
    runPixels.push_back(px);
  }
  return runPixels;
}


//main comp coarse pedestal trimming function
bool CHIPInterface::trimCompCoarseByPedestal(int STARTADDR, int ENDADDR, int numRuns)
{
  runTrimming = true;
  untrimmablePixels.assign(totalNumPxs,false);
  updatePoweredPixels();

  vector<vector<double>> binValues(totalNumPxs,vector<double>(16,0.0));

  vector<uint32_t> untrimPixels;
  for(int part =0; part<numRuns; part++)
  {
    cout << endl << "Trimming CompCoarse by Pedestal run " << part << "/" << numRuns << endl;
    vector<uint32_t> pixelsToTrim = getPixelsToTrimForCompCoarseByPedestal(part,numRuns);

    auto nextUntrimPixels = trimCompCoarseByPedestal(pixelsToTrim,binValues,STARTADDR,ENDADDR);
    untrimPixels.insert(untrimPixels.end(),nextUntrimPixels.begin(),nextUntrimPixels.end());
    if(!runTrimming) break;
  }

  int cnt = 0;
  cout << endl;
  for(const auto & px : untrimPixels)
  {
    if(cnt++ < 20){
      cout << "Untimmable Pixel " << setw(5) << px << ": ";
      for(const auto & value : binValues[px]){
        cout << setw(8) << value << " ";
      }
      cout << endl;
    }
  }

  if(!untrimPixels.empty()){
    SuS_LOG_STREAM(info, log_id(), "Untimmable Pixels found: " << untrimPixels.size());
  }

  programPixelRegs();

  showTrimmingResultsByPedestal(binValues);
  return true;
}


void CHIPInterface::showTrimmingResultsByPedestal(const vector<vector<double>> &binValues)
{
  vector<double> bestValues;
  for(const auto & px : onPixels){
     auto bestSetting = getPixelRegisterValue(to_string(px),"CompCoarse");
     bestValues.push_back( binValues[px][bestSetting] );
  }

  if(bestValues.empty()){
    SuS_LOG_STREAM(info, log_id(),"Trimming went wrong:");
    return;
  }

  SuS_LOG_STREAM(info, log_id(),"Trimming results:");
  SuS_LOG_STREAM(info, log_id(), bestValues.size() << " of " << onPixels.size() << " in range: " << utils::getMinMaxStatsStr(bestValues));
}


uint32_t CHIPInterface::findBestCompCoarseSettingFromPedestal(const std::vector<double> & binValues)
{
  vector<double> diffValues(binValues.size(),1000.0);
  if(binValues[4]<15)
  {
    for(uint32_t i=0; i<4; i++){
      diffValues[i] = fabs(binValues[i] - 42);
    }
    return std::min_element(diffValues.begin(),diffValues.begin()+4)-diffValues.begin();
  }

  if(binValues[0]<100){
    diffValues[0] = binValues[0] - binValues[1];
  }


  for(uint32_t i=1; i<binValues.size()-1; i++){
    double value = binValues[i];

    //value below 15 is out of range
    if(value < 15){
      break;
    }

    const double diff = value - binValues[i+1];
    diffValues[i] = diff;

    if(diffValues[i] == 0){
      return i;
    }

    // find first local minimum after little change
    if( (diffValues[i-1] < diffValues[i]) && (diffValues[i-1] < 5) && (binValues[i-1]<150)){
      uint32_t bestIdx = i-1;

      // if binValues is too low find closest to 42
      if(binValues[bestIdx] > 20){
        return bestIdx;
      }

      for(uint32_t j=0; j<bestIdx; j++){
        diffValues[j] = fabs(binValues[j] - 42);
      }
      return std::min_element(diffValues.begin(),diffValues.begin()+bestIdx)-diffValues.begin();
    }
  }

  int minIdx = std::min_element(diffValues.begin(),diffValues.end())-diffValues.begin();

  // if binValues is too low find closest to 42
  if(binValues[minIdx] > 20){
    return minIdx;
  }

  for(int j=0; j<minIdx; j++){
    diffValues[j] = fabs(binValues[j] - 42);
  }
  return std::min_element(diffValues.begin(),diffValues.begin()+minIdx)-diffValues.begin();
}


vector<double> CHIPInterface::measureBurstData(int STARTADDR, int ENDADDR, bool subtract)
{
  updatePoweredPixels();
  return measureBurstData(onPixels,STARTADDR,ENDADDR,subtract);
}


bool CHIPInterface::findAndSetInjectionEventOffset(const std::vector<uint32_t> & pixels, int injectSetting, int sramStartAddr)
{
  m_injectionEventOffset = 0;

  int signalPeriod = getInjectionPeriod();
  if(signalPeriod == 1){
    return true;
  }

  SuS_LOG_STREAM(info, log_id(), "+++++ Empty Inject Cycles used - checking automatically for offset");

  setInjectionDAC(injectSetting);
  if(!doSingleCycle()){
    SuS_LOG_STREAM(error, log_id(), "+++++ DATA RECEIVER ERROR findAndSetInjectionPeriod aborted");
    return false;
  }

  SuS_LOG_STREAM(info, log_id(), "+++++ Sequencer::EmptyInjectCycles = " << signalPeriod-1);

  bool maxFound = false;
  for (auto p : pixels) {
    const auto sramData = getPixelSramData(p);
    if (signalPeriod>0) {
      for(int addr=sramStartAddr; addr<=sramStartAddr+signalPeriod; addr++){
        //SuS_LOG_STREAM(error, log_id(), "+++++ pixel " << p << ", addr[" << addr << "]=" << sramData[addr]);
        if(sramData[m_injectionEventOffset] < sramData[addr]-10) {
          m_injectionEventOffset = addr;
          maxFound = true;
        }
      }
    }
    if (maxFound) break;
  }
  if (!maxFound) {
    m_injectionEventOffset = 0;
    SuS_LOG_STREAM(error, log_id(), "+++++ ERROR: findAndSetInjectionEventOffset() failed!");
  }
  SuS_LOG_STREAM(error, log_id(), "+++++ set injectionEventOffset = " << m_injectionEventOffset);
  return true;
}


// function cumputes mean value for all pixels over given sram range, certain optimizastions are added:
// - empty inject cycles are handled
// - baseline subtraction can be enabled
// - gcc code is converted below GCCWRAP
// - no error codes are added to the mean value
vector<double> CHIPInterface::measureBurstData(const vector<uint32_t> & measurePixels, int STARTADDR, int ENDADDR, bool subtract)
{
  int signalPeriod = getInjectionPeriod();

  errorCodePixels.clear();

  uint32_t numPixels = measurePixels.size();
  vector<double> binValues(numPixels,0.0);
  vector<double> numPxValues(numPixels,0.0);

  const bool sramBlacklistValid = m_sramBlacklist.isValid();
  static const std::vector<bool> allValid(ENDADDR+1,true);

  for(uint32_t it=0; (it < m_iterations && runTrimming); it++)
  {
    if(!doSingleCycle()){
      SuS_LOG_STREAM(warning, log_id(), "+++++ DATA RECEIVER ERROR measureBurstData aborted");
      return binValues;
    }

#pragma omp parallel for
    for(uint32_t idx =0; idx < numPixels; idx++)
    {
      const auto & pxAddresses = sramBlacklistValid? m_sramBlacklist.getValidSramAddresses(idx) : allValid;
      const auto sramData = getPixelSramData(measurePixels[idx]);
      double mean = 0.0;
      bool pixelAdded = false;

      for(int sram=STARTADDR+m_injectionEventOffset; sram<=ENDADDR; sram=sram+signalPeriod)
      {
        if(!pxAddresses[sram]) continue;
        const auto & value = sramData[sram];
        if(value < GCCWRAP){ // gcc correction
          mean += value + 256;
          numPxValues[idx]++;
        }else if (value != 511){
          mean += value;
          numPxValues[idx]++;
        }else{ // error code
          if(!pixelAdded){
#pragma omp critical
            errorCodePixels.push_back(measurePixels[idx]);
            pixelAdded = true;
          }
#pragma omp critical
          m_errorCode = true;
        }
      }
      binValues[idx] += mean;
    }
  }

#pragma omp parallel for
  for(uint32_t idx = 0; idx<numPixels; idx++){
    binValues[idx] /= numPxValues[idx];
  }

  if(subtract)
  {
#pragma omp parallel for
    for(uint32_t idx =0; idx < numPixels; idx++){
      binValues[idx] -= baselineValues[measurePixels[idx]];
    }
  }
  return binValues;
}


int CHIPInterface::trimPixelParamInOutlayers(const string & signalName, int STARTADDR, int ENDADDR, int lowerLimit, int upperLimit)
{
  const bool isSingleIntMode = sequencer->getOpMode() == Sequencer::SINGLEINT;
  const bool isFBCap = signalName.compare("FCF_EnCap") == 0;
  const bool isCompcoarse = signalName.compare("CompCoarse") == 0;
  const bool isIRamp = signalName.compare("RmpFineTrm") == 0;

  updatePoweredPixels();

  runTrimming = true;
  bool subtract = (isCompcoarse && isSingleIntMode) || isIRamp || isFBCap;
  const auto binValues = measureBurstData(onPixels,STARTADDR,ENDADDR,subtract);

  const uint32_t maxSetting = pixelRegisters->getMaxSignalValue("Control register",signalName);

  vector<uint32_t> limitPixels;
  int cnt=0,idx = 0;
  for(const auto & px : onPixels)
  {
    const auto & value = binValues[idx++];

    if(value < lowerLimit)
    {
      uint32_t newValue = getPixelRegisterValue(to_string(px),signalName)-1;

      if(newValue<=maxSetting && !(isFBCap && (newValue==0))){
        setPixelRegisterValue(to_string(px),signalName,newValue);
        cnt++;
      }else{
        limitPixels.push_back(px);
      }
    }
    else if(value > upperLimit)
    {
      uint32_t newValue = getPixelRegisterValue(to_string(px),signalName)+1;

      if(newValue<=maxSetting){
        setPixelRegisterValue(to_string(px),signalName,newValue);
        cnt++;
      }else{
        limitPixels.push_back(px);
      }
    }
  }

  if(signalName.compare("CompCoarse") == 0){
    for(const auto & errorCodePixel : errorCodePixels){
      int randomCompCoarseSetting = utils::getRandomNumber(0,8);
      pixelRegisters->setSignalValue("Control register",to_string(errorCodePixel),"CompCoarse",randomCompCoarseSetting);
    }
  }

  cout << signalName << " setting changed in " << cnt << " pixels" << endl;
  const auto pixelStr = utils::positionVectorToList(errorCodePixels);
  cout << errorCodePixels.size() << " Pixels with error code found: " << pixelStr.substr(0,std::min((size_t)30,pixelStr.length())) << endl;

  if(!limitPixels.empty()){
    cout << limitPixels.size() << " pixels at limit: " << utils::positionVectorToList(limitPixels) << endl;
  }

  programPixelRegs();
  return cnt;
}


int CHIPInterface::trimPixelParamToCenter(const string & signalName, int STARTADDR, int ENDADDR, int centerValue)
{
  const bool isSingleIntMode = sequencer->getOpMode() == Sequencer::SINGLEINT;
  const bool isFBCap = signalName.compare("FCF_EnCap") == 0;
  const bool isCompcoarse = signalName.compare("CompCoarse") == 0;
  const bool isIRamp = signalName.compare("RmpFineTrm") == 0;

  updatePoweredPixels();

  runTrimming = true;
  bool subtract = (isCompcoarse && isSingleIntMode) || isIRamp || isFBCap;
  const auto oldValues = measureBurstData(onPixels,STARTADDR,ENDADDR,subtract);

  const uint32_t maxSetting = pixelRegisters->getMaxSignalValue("Control register",signalName);

  vector<uint32_t> oldPixelParam(onPixels.size());
  vector<uint32_t> newPixelParam(onPixels.size());
  vector<uint32_t> limitPixels;

  int lowerLimit = (isCompcoarse) ? centerValue - 15 : centerValue;
  int upperLimit = (isCompcoarse) ? centerValue + 15 : centerValue;

  cout << "Starting to loop over " << onPixels.size() << " pixels." << endl;
  uint32_t cnt=0,idx = 0;
  for(const auto & px : onPixels)
  {
    if (px < 64) {
      cout << "Pixel " << px;
      cout << "\toldVal " << oldValues[idx];
      cout << endl;
    }

    const auto & value = oldValues[idx];
    oldPixelParam[idx] = getPixelRegisterValue(to_string(px),signalName);

    uint32_t newSetting = oldPixelParam[idx];
    if(value < lowerLimit){
      if(isIRamp){
        double relDistance = 100.0 - 100.0*value/centerValue;
        int numSteps = min(5,max(1,(int)floor(relDistance / 10.0))); // increase trimming step. Assume stepsize of 3%
        newSetting -= numSteps;
      }else{
        newSetting--;
      }

      if(newSetting<=maxSetting && !(isFBCap && (newSetting==0))){
        cnt++;
      }else{
        newSetting = (isFBCap)? 1 : 0;
        limitPixels.push_back(px);
      }
    }else if(value > upperLimit){
      if(isIRamp){
        double relDistance = 100.0*value/centerValue - 100.0;
        int numSteps = min(5,max(1,(int)floor(relDistance / 10.0))); // increase trimming step. Assume stepsize of 3%
        newSetting += numSteps;
      }else{
        newSetting++;
      }

      if(newSetting<=maxSetting){
        cnt++;
      }else{
        newSetting= maxSetting;
        limitPixels.push_back(px);
      }
    }
    newPixelParam[idx++] = newSetting;
  }

  idx = 0;
  for(const auto & setting : newPixelParam){
    setPixelRegisterValue(to_string(onPixels[idx]),signalName,setting);
    idx++;
  }

  if(signalName.compare("CompCoarse") == 0){
    for(const auto & errorCodePixel : errorCodePixels){
      int randomCompCoarseSetting = utils::getRandomNumber(0,15);
      pixelRegisters->setSignalValue("Control register",to_string(errorCodePixel),"CompCoarse",randomCompCoarseSetting);
    }
  }

  cout << errorCodePixels.size() << " pixels with error code found: " << utils::positionVectorToList(errorCodePixels) << endl;


  programPixelRegs();

  const auto newValues = measureBurstData(onPixels,STARTADDR,ENDADDR,subtract);
  cnt=0;idx=0;
  for(const auto & px : onPixels){
    if(fabs(oldValues[idx]-centerValue) < fabs(newValues[idx]-centerValue)){
      setPixelRegisterValue(to_string(px),signalName,oldPixelParam[idx]);
    }else{
      cnt++;
    }
    idx++;
  }

  if(signalName.compare("CompCoarse") == 0){
    for(const auto & errorCodePixel : errorCodePixels){
      int randomCompCoarseSetting = utils::getRandomNumber(0,15);
      pixelRegisters->setSignalValue("Control register",to_string(errorCodePixel),"CompCoarse",randomCompCoarseSetting);
    }
  }

  if(cnt != onPixels.size()){
    programPixelRegs();
  }

  doSingleCycle();

  cout << signalName << " setting changed in " << cnt << " pixels" << endl;
  cout << errorCodePixels.size() << " pixels with error code found: " << utils::positionVectorToList(errorCodePixels) << endl;

  if(!limitPixels.empty()){
    cout << limitPixels.size() << " pixels at limit: " << utils::positionVectorToList(limitPixels) << endl;
  }

  return cnt;
}


string CHIPInterface::showOutlayers(int STARTADDR, int ENDADDR, int lowerLimit, int upperLimit, bool baseline)
{
  runTrimming = true;

  updatePoweredPixels();

  const auto binValues = measureBurstData(onPixels,STARTADDR,ENDADDR,baseline);

  vector<uint32_t> outPixels;
  int idx = 0;
  for(const auto & px : onPixels)
  {
    const auto & value = binValues[idx++];

    if(value < lowerLimit){
      outPixels.push_back(px);
    }else if(value > upperLimit){
      outPixels.push_back(px);
    }
  }

  outPixels.insert(outPixels.end(),errorCodePixels.begin(),errorCodePixels.end());

  utils::removeDuplicates(outPixels);

  string outLayers = utils::positionVectorToList(outPixels);

  cout << outPixels.size() << " outlayers: " << outLayers << endl;

  return outLayers;
}


void CHIPInterface::setBaseline(int STARTADDR, int ENDADDR)
{
  setBaseline(onPixels,STARTADDR,ENDADDR);
}


void CHIPInterface::setBaseline(const std::vector<uint32_t> & measurePixels,int STARTADDR, int ENDADDR)
{
  clearBaseLine();

  updatePoweredPixels();

  vector<double> binValues = measureBurstData(measurePixels,STARTADDR,ENDADDR,false);
  const int numPixels = measurePixels.size();

#pragma omp parallel for
  for(int idx=0; idx<numPixels; idx++){
    baselineValues[measurePixels[idx]] = binValues[idx];
  }

  baselineValuesValid = true;

  cout << "Baseline of powered pixels updated" << endl;
}


void CHIPInterface::setPixelInjectionBaseline(int STARTADDR, int ENDADDR, int baselineSetting)
{
  uint32_t pixelInjectionRem = 0;
  runTrimming = true;
  pixelInjectionRem = getInjectionDACsetting();
  setInjectionDAC(baselineSetting);
  usleep(200000);
  setBaseline(STARTADDR,ENDADDR);
  setInjectionDAC(pixelInjectionRem);
}


void CHIPInterface::setIntegrationTimeBaseline(int STARTADDR, int ENDADDR, int value)
{
  runTrimming = true;

  IntegrationRemoveKeeper keeper(this,value);

  setBaseline(STARTADDR,ENDADDR);
}


std::vector<double> &CHIPInterface::measureDacSweepForCurvature(std::vector<uint32_t> &measurePixels,
                                                               const std::vector<uint32_t> &dacSettings,
                                                               const int STARTADDR,
                                                               const int ENDADDR)
{
  curvatureValues.resize(totalNumPxs);
  vector<vector<double> > binValues(totalNumPxs,vector<double>(dacSettings.size(),0));

  if(!measureIntDacSweepforSlopes(dacSettings,measurePixels,binValues,STARTADDR,ENDADDR)){
    return curvatureValues;
  }

  const int numPxs = measurePixels.size();
#pragma omp parallel for
  for(int idx = 0; idx<numPxs; idx++)
  {
    int px = measurePixels[idx];
    curvatureValues[px] = calcCurvatureGraph(dacSettings,binValues[px]);
  }
  return curvatureValues;
}


bool CHIPInterface::measureIntDacSweepforSlopes(const vector<uint32_t> & dacValues, vector<uint32_t> & measurePixels, vector<vector<double>> & binValues, const int STARTADDR, const int ENDADDR )
{
  static const int TO = 3;

  std::vector<bool> errorPixels;

  bool risingSlope;
  int cnt = 0;

  do{

    errorPixels.assign(totalNumPxs,false);

    risingSlope = true;

    int dacIdx = 0;

    for(const auto & dac : dacValues)
    {
      setInjectionDAC(dac);

      const auto measuredData = measureBurstData(measurePixels,STARTADDR,ENDADDR,baselineValuesValid);
      // init all binVals to 0
      int idx = 0;
      for(const auto & px : measurePixels){
        binValues[px][dacIdx] = measuredData[idx++];
      }

      dacIdx++;
      if(!runTrimming){cnt=TO; break;}

      //virtual function which might remain unimplemented
      displayInjectionSweep(binValues,dacValues,measurePixels);
    }

    // check if all measurement points are in ascending order:
    // sort and check if anything changed
    if(dacValues.size() < 12)
    {
      int errPxCnt = 0;
      for(const auto & px : measurePixels)
      {
        // check ascending order only if slope is steep enouph
        auto binValuesToSort = binValues[px];

        sort(binValuesToSort.begin(),binValuesToSort.end());
        for(uint32_t i=0; i<binValuesToSort.size()-1;i++)
        {
          if(binValues[px][i] != binValuesToSort[i])
          {
            if(errPxCnt<10)
            {
              std::cout << "ERROR Pixel "<< setw(5) << px;
              std::cout << " at setting " << i << ":"  << binValues[px][i] << " != " << binValuesToSort[i];
              for(uint32_t j=0; j<binValues[px].size(); j++){
                std:: cout << " [" << setw(5) << to_string(j) << "]=" << setw(8) << binValues[px][j];
              }
              std::cout << std::endl;
            }
            risingSlope = false;
            errorPixels[px] = true;
            errPxCnt++;
            i=binValuesToSort.size();
          }
        }
      }
      if(errPxCnt>=10){
        std::cout << "ERROR "<< errPxCnt << " range errors in total" << endl;
      }
    }

    cnt++;
  }while(!risingSlope && cnt<TO);


  if(cnt==TO){
    std:: cout << "PxsIrampTrim Data Error, could not generate rising slopes in all pixels" << endl;
  }

  removeErrorPixelsFromVector(errorPixels,measurePixels);

  return true;
}


 utils::DataHistoVec CHIPInterface::measureIntDacSweepForHistograms(const vector<uint32_t> & dacValues, const vector<uint32_t> & measurePixels, const int STARTADDR, const int ENDADDR )
{
  const int numPixels = measurePixels.size();
  utils::DataHistoVec pixelHistograms(numPixels);

  int idx = 0;
  for(const auto & dac : dacValues)
  {
    cout << endl << endl;
    cout << "##################################################################" << endl;
    cout << "#####    Measure DAC Setting " << dac << " : " << idx++ << "/" << dacValues.size() << endl;
    cout << "##################################################################" << endl << endl;

    setIntDACValue(dac);

    measureHistoData(measurePixels,pixelHistograms,STARTADDR,ENDADDR);

    m_progressBar.addValue();

    if(!runTrimming){
      SuS_LOG_STREAM(warn, log_id(), "Laddertrimming was aborted, will exit at setting " << dac << " : " << idx << "/" << dacValues.size() );
      break;
    }
  }

  return pixelHistograms;
}


std::vector<double> CHIPInterface::measureRMSData(const std::vector<uint32_t> & measurePixels, const int STARTADDR, const int ENDADDR )
{
  auto pixelHistograms = measureHistoData(measurePixels,STARTADDR,ENDADDR);
  return utils::getRMSVector(pixelHistograms);
}


utils::DataHistoVec CHIPInterface::measureHistoData(const vector<uint32_t> & measurePixels, const int STARTADDR, const int ENDADDR )
{
  utils::DataHistoVec pixelHistograms(measurePixels.size());

  measureHistoData(measurePixels,pixelHistograms,STARTADDR,ENDADDR);

  return pixelHistograms;
}


void CHIPInterface::measureHistoData(const vector<uint32_t> & measurePixels, utils::DataHistoVec & pixelHistograms, const int STARTADDR, const int ENDADDR )
{
  for(uint32_t it=0; (it < m_iterations && runTrimming); it++)
  {
    try{
      if(!doSingleCycle()){
        return;
      }
      addMeasuredDataToHisto(measurePixels,pixelHistograms,STARTADDR,ENDADDR);
    }catch(...){
      cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
      cout << "Exception in do Single Cycle, will rerun this iteration" << endl;
      it--;
    }
  }

  displayDataHistos(pixelHistograms,measurePixels);
}


void CHIPInterface::addMeasuredDataToHisto(const vector<uint32_t> & measurePixels, utils::DataHistoVec & pixelHistograms, const int STARTADDR, const int ENDADDR )
{
  lock_guard<mutex> lock (pixelDataMutex);
  const size_t numPxs = measurePixels.size();

#pragma omp parallel for
  for(size_t idx = 0; idx < numPxs; idx++)
  {
    int px = measurePixels[idx];
    auto & pixelHisto = pixelHistograms[idx];
    const auto sramData = getPixelSramData(px);
    bool logPxErr = true;
    for(int sram=STARTADDR; sram<=ENDADDR; sram++){
      auto value = sramData[sram];
      if(value < GCCWRAP){
        value += 256;
      }else if (value == 511 && logPxErr){
        SuS_LOG_STREAM(warning, log_id(), "+++++ ErrorCount 511 measured in pixel " << px << ".");
#pragma omp critical
        m_errorCode = true;
        logPxErr = false;
      }
      pixelHisto.add(value);
    }
  }
}


std::vector<double> & CHIPInterface::measureMeanSramContent(uint32_t pixel, const int STARTADDR, const int ENDADDR, bool onlyStartEnd)
{
  std::vector<uint32_t> measurePixels(1,pixel);
  return measureMeanSramContent(measurePixels,STARTADDR,ENDADDR,onlyStartEnd);
}


std::vector<double> & CHIPInterface::measureMeanSramContent(const vector<uint32_t> & measurePixels, const int STARTADDR, const int ENDADDR, bool onlyStartEnd)
{
  meanSramContent.assign(totalNumPxs*sramSize,0.0);

  for(uint32_t it=0; it < m_iterations; it++)
  {
    if(!doSingleCycle()){
      if(it>0){
        calcMeanSramContent(it,measurePixels,STARTADDR,ENDADDR,onlyStartEnd);
      }
      return meanSramContent;
    }
    addMeasuredDataToMeanSramContent(measurePixels,STARTADDR,ENDADDR,onlyStartEnd);
    SuS_LOG_STREAM(info, log_id(), "Measure Mean SRAM Content: " << it+1  << "/" << m_iterations);
  }

  calcMeanSramContent(m_iterations,measurePixels,STARTADDR,ENDADDR,onlyStartEnd);

  return meanSramContent;
}


void CHIPInterface::addMeasuredDataToMeanSramContent(const vector<uint32_t> & measurePixels, const int STARTADDR, const int ENDADDR , bool onlyStartEnd)
{
  const int sramStep = (onlyStartEnd)? ENDADDR-STARTADDR : 1;
  const int numPxs = measurePixels.size();

#pragma omp parallel for
  for(int idx=0; idx<numPxs; idx++){
    const size_t px = measurePixels[idx];
    double * pxMeanSramContent = meanSramContent.data() + px*sramSize;
    const auto sramData = getPixelSramData(px);
    for(int sram=STARTADDR; sram<=ENDADDR; sram += sramStep){
      pxMeanSramContent[sram] += sramData[sram];
    }
  }
}


void CHIPInterface::calcMeanSramContent(double numIterations, const vector<uint32_t> & measurePixels, const int STARTADDR, const int ENDADDR , bool onlyStartEnd)
{
  if(numIterations < 1){
    meanSramContent.assign(totalNumPxs*sramSize,0.0);
    return;
  }

  const int sramStep = (onlyStartEnd)? ENDADDR-STARTADDR : 1;
  const int numPxs = measurePixels.size();

#pragma omp parallel for
  for(int idx = 0; idx<numPxs; idx++){
    const size_t px = measurePixels[idx];
    double * pxMeanSramContent = meanSramContent.data() + px*sramSize;
    for(int sram=STARTADDR; sram<=ENDADDR; sram += sramStep){
      pxMeanSramContent[sram] /= numIterations;
    }
  }
}


double CHIPInterface::calcCurvatureGraph(const vector<uint32_t> & xValues, const vector<double> & yValues)
{
  const int n = xValues.size();

  if(n <= 2) return 0.0;

  double lastSection = calcSlopeOfSection(1,xValues,yValues);
  double curvation = 1.0;
  for(int i = 2; i<n; i++){
    double thisSection = calcSlopeOfSection(i,xValues,yValues);
    curvation *= lastSection/thisSection;
    lastSection = thisSection;
  }
  //return sqrt(curvation);
  return curvation;
}


std::vector<double> & CHIPInterface::measureSramDriftMap(const int STARTADDR, const int ENDADDR )
{
  const auto sendingPixels = getSendingPixels();
  measureMeanSramContent(sendingPixels,STARTADDR,ENDADDR,true);

  int numPxs = sendingPixels.size();
#pragma omp parallel for
  for(int idx = 0; idx<numPxs; idx++){
    const int px = sendingPixels[idx];
    const double * pxMeanSramContent = meanSramContent.data() + px * sramSize;
    sramDrift[px] = pxMeanSramContent[ENDADDR] - pxMeanSramContent[STARTADDR];
  }
  return sramDrift;
}


std::vector<double> & CHIPInterface::measureSramSlopesMap(const int STARTADDR, const int ENDADDR )
{
  // initialize variables
  const int numSram = ENDADDR - STARTADDR + 1;

  const auto sendingPixels = getSendingPixels();
  const int numPxs = sendingPixels.size();

  vector<uint32_t> xValues;
  for(int i=STARTADDR; i<=ENDADDR; i++){
    xValues.push_back(i);
  }

  const uint32_t LINSUM_X  = std::accumulate(xValues.begin(), xValues.end(), 0.0);
  const uint32_t LINSUM_XX = std::inner_product(xValues.begin(), xValues.end(), xValues.begin(), 0.0);

  //  measure data
  measureMeanSramContent(sendingPixels,STARTADDR,ENDADDR,false);

  // evaluate data
#pragma omp parallel for
  for(int idx = 0; idx<numPxs; idx++){
    const int px = sendingPixels[idx];
    const double * pxMeanSramContent = meanSramContent.data() + px * sramSize + STARTADDR;
    vector<double> yValues;
    yValues.assign(pxMeanSramContent,pxMeanSramContent+numSram);
    sramDrift[px] = linearRegression(LINSUM_X,LINSUM_XX,xValues,yValues)*numSram;
  }
  return sramDrift;
}



double CHIPInterface::calcSlopeOfSection(int step, const vector<uint32_t> & xValues, const vector<double> & yValues)
{
  const int n = xValues.size();

  if( step >= n) return 0.0;
  if( step <  1) return 0.0;

  return (yValues[step]-yValues[step-1])/(xValues[step]-xValues[step-1]);
}


//optimized function s_x s_xx is constant so dont compute for every pixel
double CHIPInterface::linearRegression(const uint32_t s_x, const uint32_t s_xx, const vector<uint32_t> & xValues, const vector<double> & yValues)
{
  return utils::linearRegression(s_x,s_xx,xValues,yValues);
}


bool CHIPInterface::measurePxInjSweepforSlopes( const vector<uint32_t> & injectionSettings,
                                                vector<vector<double>> & binValues,
                                                const int STARTADDR, const int ENDADDR)
{
  updatePoweredPixels();

  static const int TO = 3;

  vector<bool> errorPixels;

  bool risingSlope;
  int cnt = 0;
  int numActivePixels = onPixels.size();

  do
  {
    errorPixels.assign(totalNumPxs,false);

    risingSlope = true;

    int sigIdx = 0;

    for(const auto & injection : injectionSettings)
    {
      jtagRegisters->setSignalValue("Global Control Register","all","Pixel Injection Signal Trim",injection);
      programJtagSingle("Global Control Register");

      const auto settingData = measureBurstData(onPixels,STARTADDR,ENDADDR,false);
      // init all binVals to 0

#pragma omp parallel for
      for(int pxIdx = 0; pxIdx < numActivePixels; pxIdx++){
        binValues[pxIdx][sigIdx] = settingData[pxIdx];
      }

      sigIdx++;
      if(!runTrimming){cnt=TO; break;}
    }

    cnt++;
  }while(!risingSlope && cnt<TO);


  if(cnt>=TO){
    cout << "PxsInjectionSweep Data Error, could not finsish measuring data for all settings" << endl;
  }

  return true;
}


vector<double> CHIPInterface::measurePixelInjectionSweepSlopes( const vector<uint32_t> & injectionSettings,
                                                                const int STARTADDR, const int ENDADDR)
{
  runTrimming = true;

  const uint32_t LINSUM_X  = std::accumulate(injectionSettings.begin(), injectionSettings.end(), 0.0);
  const uint32_t LINSUM_XX = std::inner_product(injectionSettings.begin(), injectionSettings.end(), injectionSettings.begin(), 0.0);

  updatePoweredPixels();

  int numActivePixels = onPixels.size();

  vector<double> pixelSlopes(totalNumPxs,0.0);
  vector<vector<double>> binValues(numActivePixels,vector<double>(injectionSettings.size(),0.0));

  measurePxInjSweepforSlopes(injectionSettings,binValues,STARTADDR,ENDADDR);

  for (int pxIdx = 0; pxIdx < numActivePixels; ++pxIdx){
    double slope = linearRegression(LINSUM_X,LINSUM_XX,injectionSettings,binValues[pxIdx]);
    pixelSlopes[onPixels[pxIdx]] = slope;
  }

  return pixelSlopes;
}



void CHIPInterface::trimForMaxDynamicRange(const std::string & pixelStr , int startSRAM, int endSRAM, int pixelInjectionSignal, bool alsoIramp)
{
  runTrimming = true;

  updatePoweredPixels();

  vector<uint32_t> pixelsToTrim;
  if(pixelStr[0] == 'a')
  {
    pixelsToTrim = onPixels;
  }
  else
  {
    pixelsToTrim = utils::positionListToVector(pixelStr);

    for(int idx = pixelsToTrim.size()-1; idx >= 0; --idx){
      if(poweredPixels[pixelsToTrim[idx]] == false){
        pixelsToTrim.erase(pixelsToTrim.begin() + idx);
      }
    }
  }

  const auto pixelInjectionRem = jtagRegisters->getSignalValue("Global Control Register", "0" , "Pixel Injection Signal Trim");
  jtagRegisters->setSignalValue("Global Control Register", "0" , "Pixel Injection Signal Trim",pixelInjectionSignal);
  programJtagSingle("Global Control Register");

  std::string pixelToTrimStr = utils::positionVectorToList(pixelsToTrim);

#ifdef F1IO
  int mimCapLSB = getPixelRegisterValue(pixelToTrimStr,"D0_EnMimCap") & 1;
  setPixelRegisterValue(pixelToTrimStr,"D0_EnMimCap",(6 + mimCapLSB));
#else
  int mimCapLSB = getPixelRegisterValue(pixelToTrimStr,"CSA_FbCap") & 1;
  setPixelRegisterValue(pixelToTrimStr,"CSA_FbCap",(6 + mimCapLSB));
#endif

  setPixelRegisterValue(pixelToTrimStr,"FCF_EnCap",1);

  if(alsoIramp){
    setPixelRegisterValue(pixelToTrimStr,"RmpFineTrm",63);
  }

  vector<int> trimMode(totalNumPxs,0);
  vector<bool> irampDoneVec(totalNumPxs,false);

  int maxADU = sequencer->getRampLength() * 2;

  untrimmablePixels.clear();

  while(!pixelsToTrim.empty() && runTrimming)
  {
    cout << "Optimizing dynamic range for " << pixelsToTrim.size() << " pixels " << endl;


    onPixels.assign(pixelsToTrim.begin(),pixelsToTrim.end());

    programPixelRegs();

    const auto valuesVec = measureBurstData(onPixels,startSRAM,endSRAM,false);

    int incFCFCnt = 0;
    int decIrampCnt = 0;
    int incIrampCnt = 0;
    int irampDoneCnt = 0;

    for(int idx = pixelsToTrim.size()-1; idx >= 0; --idx)
    {
      const auto & px =pixelsToTrim[idx];

      if(idx < 10){
        cout << "Pixel " << setw(5) << px << " mode " << trimMode[px] << " value = " << setw(3) << valuesVec[idx] << "/" << maxADU;
        cout << " irampDone = " << irampDoneVec[px] << endl;
      }

      if(valuesVec[idx] < 1.0 || valuesVec[idx] > maxADU)
      {
        if(trimMode[px] == 0){
          const auto newFCFCap = pixelRegisters->getSignalValue("Control register",to_string(px),"FCF_EnCap") + 1 ;
          if(newFCFCap<16){
            pixelRegisters->setSignalValue("Control register",to_string(px),"FCF_EnCap",newFCFCap);
            incFCFCnt++;
          }else if(alsoIramp){
            trimMode[px] = 1;
          }
        }else if(alsoIramp){
          const int newRmpFineTrm = ((int)pixelRegisters->getSignalValue("Control register",to_string(px),"RmpFineTrm")) + 5 ;
          if(newRmpFineTrm < 64){
            incIrampCnt++;
            irampDoneVec[px] = true;
            pixelRegisters->setSignalValue("Control register",to_string(px),"RmpFineTrm",newRmpFineTrm);
          }else{
            untrimmablePixels.push_back(pixelsToTrim[idx]);
            pixelsToTrim.erase(pixelsToTrim.begin()+idx);
          }
        }
      }else{
        if(trimMode[px] == 0){
          if(alsoIramp){
            trimMode[px] = 1;
          }else{
            pixelsToTrim.erase(pixelsToTrim.begin()+idx);
          }
        }else{
          if(!irampDoneVec[px]){
            const int newRmpFineTrm = ((int)pixelRegisters->getSignalValue("Control register",to_string(px),"RmpFineTrm")) - 1 ;
            if(newRmpFineTrm>=0){
              decIrampCnt++;
              pixelRegisters->setSignalValue("Control register",to_string(px),"RmpFineTrm",newRmpFineTrm);
            }else{
              pixelsToTrim.erase(pixelsToTrim.begin()+idx);
            }
          }else{
            irampDoneCnt++;
            pixelsToTrim.erase(pixelsToTrim.begin()+idx);
          }
        }
      }
    }

    // print status
    cout << endl;
    cout << "incFCFCnt    = " << setw(5) << incFCFCnt    << endl;
    cout << "decIrampCnt  = " << setw(5) << decIrampCnt  << endl;
    cout << "incIrampCnt  = " << setw(5) << incIrampCnt  << endl;
    cout << "irampDoneCnt = " << setw(5) << irampDoneCnt << endl << endl;
  }

  programPixelRegs();

  jtagRegisters->setSignalValue("Global Control Register", "0" , "Pixel Injection Signal Trim",pixelInjectionRem);
  programJtagSingle("Global Control Register");

  if(untrimmablePixels.size() > 0){
    cout << untrimmablePixels.size() << " untrimmable pixels found: " << utils::positionVectorToList(untrimmablePixels) << endl;
  }
}


// -----------------------------------------------
// TRIM3 Day0 IntDac trimming

vector<vector<uint16_t>> CHIPInterface::findCoarseDACSettings(const vector<uint32_t> & dacValues)
{
  vector<vector<uint16_t>> compCoarseValues(onPixels.size(),vector<uint16_t>(dacValues.size(),0));

  int dacIdx = 0;
  for(const auto & dac : dacValues)
  {
    setIntDACValue(dac);
    decrCompCoarseForVHOLDValue(12, 150, -1);
    int idx=0;
    for(const auto & px:onPixels){
      uint16_t ccVal = getPixelRegisterValue(to_string(px),"CompCoarse");
      compCoarseValues[idx][dacIdx] = ccVal;
      idx++;
    }
    dacIdx++;
  }

  return compCoarseValues;
}


void CHIPInterface::setCoarseDACSettings(const vector<vector<uint16_t> > & coarseDACSettings, int dacIdx)
{
  int pxIdx = 0;
  for(const auto & px : onPixels){
    setPixelRegisterValue(to_string(px),"CompCoarse",coarseDACSettings[pxIdx][dacIdx]);
    pxIdx++;
  }
  programPixelRegs();
}



bool CHIPInterface::findBestDACSetting( const vector<uint32_t> & dacValues,
                                        const vector<uint32_t> & injectionSettings,
                                        int startAddr, int endAddr)
{
  runTrimming = true;
  SuS_LOG_STREAM(info, log_id(), "\n\n++++++++ Looking for best IntDAC Setting ++++++++");

  updatePoweredPixels();

  const string slopesStatsFileName = "Day0GainSlopeStats.txt";
  {
    ofstream clearFile(slopesStatsFileName);
  }

  const int numActivePixels = onPixels.size();
  vector<vector<double> > pixelSlopes(numActivePixels,vector<double>(dacValues.size(),0.0));

  const auto coarseDACSettings = findCoarseDACSettings(dacValues);

  for(uint32_t dacIdx = 0; (dacIdx < dacValues.size() && runTrimming); ++dacIdx)
  {
    uint32_t dacValue = dacValues[dacIdx];
    setIntDACValue(dacValue);
    setCoarseDACSettings(coarseDACSettings,dacIdx);

    vector<double> measurementSlopes = measurePixelInjectionSweepSlopes(injectionSettings,startAddr,endAddr);

    exportRunSlopesStats(measurementSlopes,slopesStatsFileName);

    for(int idx=0; idx<numActivePixels; idx++){
      pixelSlopes[idx][dacIdx] = measurementSlopes[idx];
    }
  }

  writeDay0FEGainTrimmingLog(onPixels,pixelSlopes,dacValues);

  findBestDay0FEGain(pixelSlopes,dacValues);

//  decrCompCoarseForVHOLDValue(12, 150, -1);

  return true;
}


void CHIPInterface::findBestDay0FEGain(const vector<vector<double>> & pixelSlopes, const vector<uint32_t> & dacValues)
{
  uint64_t bestDACAcc = 0;
  for(uint32_t i=0; i<onPixels.size(); i++){
    const auto & slopeVec = pixelSlopes[i];
    int bestIdx = max_element(slopeVec.begin(),slopeVec.end())-slopeVec.begin();
    bestDACAcc += dacValues[bestIdx];
  }

  uint32_t bestDAC = bestDACAcc/onPixels.size();

  cout << "Best DAC Value found = " << bestDAC << endl;
  setIntDACValue(bestDAC);
}


void CHIPInterface::writeDay0FEGainTrimmingLog(const vector<uint32_t> & activePixels, const vector<vector<double>> & pixelSlopes, const vector<uint32_t> & dacValues)
{
  ofstream out("Day0FEGainLog.txt",ofstream::out);
  if (!out.is_open()){
    SuS_LOG_STREAM(error, log_id(), "File error: file not opened!");
    return;
  }

  out << left;
  out << "#### Columns\t" << ((numOfPxs>505)? "64" : "8") << endl;
  out << setw(6) << "#Pixel\t";
  out << setw(4) << "ASIC\t";
  for(const auto & dac : dacValues){
    out << "INTDAC" << setw(4) << dac << "\t";
  }
  out << endl;

  const int numPixels = activePixels.size();
  for(int pxIdx = 0; pxIdx < numPixels; pxIdx++ )
  {
    const auto px = activePixels[pxIdx];
    out << setw(6) << px << "\t";
    out << setw(4) << getASICNumberFromPixel(px) << "\t";
    for(const auto & slope : pixelSlopes[pxIdx]){
      out << setw(10) << slope << "\t";
    }
    out << endl;
  }
}

//---------------------------------------------------------------
// TRIM4 FB CAp trimming

bool CHIPInterface::measureFBCapGains(const std::vector<uint32_t> & fbCaps,
                                      const std::vector<uint32_t> & injectionSettings,
                                      int startAddr, int endAddr)
{
  runTrimming = true;
  SuS_LOG_STREAM(info, log_id(), "\n\n++++++++ Measure FE Gains using PixelInjection ++++++++");

  updatePoweredPixels();

  const string pixelStr = utils::positionVectorToList(onPixels);
  const string slopesStatsFileName = "FEGainSlopeStats.txt";
  {
    ofstream clearFile(slopesStatsFileName);
  }

  const int numActivePixels = onPixels.size();
  vector<vector<double> > pixelSlopes(numActivePixels,vector<double>(fbCaps.size(),0.0));

  for(uint32_t fbIdx = 0; (fbIdx < fbCaps.size() && runTrimming); ++fbIdx)
  {
    uint32_t FBCap = fbCaps[fbIdx];
    setPixelRegisterValue(pixelStr,"FCF_EnCap",FBCap);
    cout << "Programming pixel regs to FBCap " << FBCap << endl;
    programPixelRegs();

    vector<double> measurementSlopes = measurePixelInjectionSweepSlopes(injectionSettings,startAddr,endAddr);

    exportRunSlopesStats(measurementSlopes,slopesStatsFileName);

    for(int idx=0; idx<numActivePixels; idx++){
      pixelSlopes[idx][fbIdx] = measurementSlopes[idx];
    }
  }

  writeFEGainTrimmingLog(onPixels,pixelSlopes,fbCaps);

  //trimFEGains(activePixelsactivePixels, pixelSlopes);
  return true;
}


void CHIPInterface::exportRunSlopesStats(const vector<double> & pixelSlopes, const string & slopesStatsFileName)
{
  ofstream out(slopesStatsFileName,ofstream::app);
  out << "Meas " << " = " << utils::getVectorStatsStr(pixelSlopes) << endl;
}


void CHIPInterface::trimFEGains(const vector<vector<double> > & pixelSlopes)
{
  updatePoweredPixels();
  const int numSlopes = pixelSlopes.size();
  const int numPixels = onPixels.size();

  for(int run = 0; run < numSlopes; run++)
  {
    for(int pxIdx = 0; pxIdx < numPixels; pxIdx++){

    }
  }
}


void CHIPInterface::writeFEGainTrimmingLog(const vector<uint32_t> & activePixels, const vector<vector<double>> & pixelSlopes, const vector<uint32_t> & fbCaps)
{
  ofstream out("FBCapTrimmingLog.txt",ofstream::out);
  if (!out.is_open()){
    SuS_LOG_STREAM(error, log_id(), "File error: file not opened!");
    return;
  }

  out << left;
  out << "#### Columns\t" << ((numOfPxs>505)? "64" : "8") << endl;
  out << setw(6) << "#Pixel\t";
  out << setw(4) << "ASIC\t";
  for(const auto & fbCap : fbCaps){
    out << "FBCap" << setw(2) << fbCap << "\t";
  }
  out << endl;

  const int numPixels = activePixels.size();
  for(int pxIdx = 0; pxIdx < numPixels; pxIdx++ )
  {
    const auto px = activePixels[pxIdx];
    out << setw(6) << px << "\t";
    out << setw(4) << getASICNumberFromPixel(px) << "\t";
    for(const auto & slope : pixelSlopes[pxIdx]){
      out << setw(7) << slope << "\t";
    }
    out << endl;
  }
}


//----------------------------------------------------------------------------


// real slope change per setting = 0.00059
// larger change decreases speed but will improve result
// because both sides of the minimum will be tested
int CHIPInterface::calcRampDiff(double slopeDiff, double aimSlope)
{
  double slopeFactor = 0.0006 * (aimSlope/0.015);
  int rampDiff = slopeDiff/slopeFactor;
  if(rampDiff==0){
    rampDiff = (slopeDiff < 0)? -1 : 1;
  }
  return rampDiff;
}


double CHIPInterface::calcSramPixelDataRMS(uint32_t pixel, uint32_t startAddr, uint32_t endAddr)
{
  const auto data = getPixelSramData(pixel) + startAddr;
  const uint32_t numValues = endAddr - startAddr + 1;
  vector<uint16_t> sramValues(numValues);
  std::copy(data,data+numValues,sramValues.begin());

  return utils::getMeandAndRMS<uint16_t>(sramValues).rms;
}


utils::StatsAcc CHIPInterface::accSramDataStats(uint32_t pixel, uint32_t startAddr, uint32_t endAddr)
{
  const auto data = getPixelSramData(pixel);
  utils::StatsAcc pixelAcc;
  for (uint32_t i=startAddr; i<=endAddr; ++i){
    pixelAcc.addValue(data[i]);
  }
  return pixelAcc;
}


vector<vector<double>> CHIPInterface::measurePixelDelays(const vector<uint32_t> & pixelsToTrim, int mode, uint32_t startAddr, uint32_t endAddr)
{
  int startFine = (mode==0)? 1 : 0;
  int endFine   = (mode==1)? 1 : 2;

  vector<vector<double>> pixelDelayMeanRMSVec(pixelsToTrim.size());
  const string pixelStr = utils::positionVectorToList(pixelsToTrim);

  cout << "++++ Measure Pixel delays for pixels: " << pixelStr << endl;

  for(int fine = startFine; (fine < endFine && runTrimming); fine ++)
  {
    setPixelRegisterValue(pixelStr,"RmpEnFineDelayCntrl",fine);

    for(int delay=0; (delay<16 && runTrimming); delay++)
    {
      cout << "++++ Measureing Pixel delay/fine: " << delay << "/" << fine << endl;

      setPixelRegisterValue(pixelStr,"RmpDelayCntrl",delay);
      programPixelRegs();

      auto pixelRMSValues = measureRMSData(pixelsToTrim,startAddr,endAddr);

      const uint32_t numValues = pixelRMSValues.size();
#pragma omp parallel for
      for(uint32_t idx = 0; idx<numValues; idx++){
        pixelDelayMeanRMSVec[idx].push_back(pixelRMSValues[idx]);
      }
    }
  }

  return pixelDelayMeanRMSVec;
}


void CHIPInterface::findBestPixelDelayValue(const vector<uint32_t> & pixelsToTrim, int mode, const vector<vector<double>> & pixelDelayMeanRMSVec)
{
  int bestDelay = 0;
  int idx       = 0;

  for(const auto & pixel : pixelsToTrim)
  {
    auto & rmsMeanValues = pixelDelayMeanRMSVec[idx++];
    if(utils::countZeroes<double>(rmsMeanValues) > 2){
     bestDelay = utils::getMiddleZero<double>(rmsMeanValues);
    }else{
     bestDelay = (std::min_element(rmsMeanValues.begin(), rmsMeanValues.end())-rmsMeanValues.begin());
    }

    if(idx<=10){
      //add final decision to trimming stats
      stringstream status;
      status << "Pixel "        << setw(5) << pixel;
      status <<" best delay : " << setw(2) << bestDelay;
      status <<" values :";

      for(const auto & rms : rmsMeanValues){
        status << " " << setprecision(3) << setw(8) << rms;
      }
      trimmingStats.push_back(status.str());
    }

    int fineDelay = (mode == 0)? 1 : (mode == 1)? 0 : bestDelay/16;
    int rmpDelay  = bestDelay%16;
    setPixelRegisterValue(to_string(pixel),"RmpEnFineDelayCntrl",fineDelay);
    setPixelRegisterValue(to_string(pixel),"RmpDelayCntrl",rmpDelay);
  }

  programPixelRegs();

  showTrimmingStats();
}


// helper function to call from Measurement class, uses CHIPInterface class members
// the class members can be set from the CHIPCalibrationBox, also from a measurement batch file
void CHIPInterface::calibratePixelDelay()
{
  cout << "Using m_: ";
  cout << "mode = " << m_trimPixelDelayMode << ", ";
  cout << "sramStartAddr = " << m_trimStartAddr<< ", ";
  cout << "sramEndAddr = " << m_trimEndAddr<< ", ";
  cout << "iterations = " << m_iterations << ", ";
  cout << "numRuns = " << m_numRuns<< endl;
  calibratePixelDelay(m_trimPixelDelayMode, m_trimStartAddr, m_trimEndAddr, m_numRuns);
}


// main pixel delay trimming routine
void CHIPInterface::calibratePixelDelay(int mode, int startAddr, int endAddr, int numRuns)
{
  runTrimming = true;

  checkLadderReadout();

  CHIPInterface::updatePoweredPixels();

  vector<uint32_t> pixelsToTrim;
  if (getSinglePixelReadoutMode()) pixelsToTrim.push_back(singlePixelToReadout);
  else pixelsToTrim = onPixels;

  // make consistent with m_iterations to use all class emembers ?
  cout << "calibratePixelDelay(): trimming " << pixelsToTrim.size() << " pixels." << endl;
  cout << "Using: ";
  cout << "mode = " << mode << ", ";
  cout << "sramStartAddr = " << startAddr << ", ";
  cout << "sramEndAddr = " << endAddr << ", ";
  cout << "iterations = " << m_iterations << ", ";
  cout << "numRuns = " << numRuns << endl;

  for(int run=0; (run<numRuns && runTrimming); run++)
  {
    cout << "Pixel Delay trim run: " << run << endl;
    const auto pixelDelayMeanRMSVec = measurePixelDelays(pixelsToTrim,mode,startAddr,endAddr);

    findBestPixelDelayValue(pixelsToTrim,mode,pixelDelayMeanRMSVec);

    pixelsToTrim = getNotPixelDelayTrimmedPixels(startAddr,endAddr);

    if(pixelsToTrim.empty()){
      break;
    }else{
      cout << "After run " << run << ": " << pixelsToTrim.size() << " pixels not trimmed" << endl;
    }
  }
}


vector<uint32_t> CHIPInterface::getNotPixelDelayTrimmedPixels(uint32_t startAddr, uint32_t endAddr)
{
  const uint32_t limit = totalNumPxs/4;

  vector<uint32_t> reTrimPixels;
  auto pixelRMSValues = measureRMSData(onPixels,startAddr,endAddr);
  uint32_t idx=0,cnt=0;
  for(const auto & px : onPixels)
  {
    if(pixelRMSValues[idx++] > 1.0){
      reTrimPixels.emplace_back(px);
      cnt++;
    }

    if(cnt >= limit) break;
  }
  return reTrimPixels;
}



void CHIPInterface::measureIterationsClockDeskewDarkFrame(int startAddr, int endAddr, const vector<uint32_t> & poweredAsics, vector<double> & asicDeskew0MeanRMSVec ,vector<double> & asicDeskew1MeanRMSVec)
{
  int numAsics = poweredAsics.size();

  asicDeskew0MeanRMSVec.assign(numAsics,0.0);
  asicDeskew1MeanRMSVec.assign(numAsics,0.0);

  // accumulate mean rms values for every iteration
  for(uint32_t it=0; (it<m_iterations && runTrimming); it++){
    if (!doSingleCycle()) {
      const string status = "+++++ DATARECEIVER ERROR: Clock deskew trimming aborted - readout error";
      trimmingStats.push_back(status);
      SuS_LOG_STREAM(error, log_id(), status);
      return;
    }

    int idx = 0;
    for(const auto & asic : poweredAsics)
    {
      asicDeskew0MeanRMSVec[idx] += checkClockDeskewHalfDarkFrame(true, asic,startAddr,endAddr);
      asicDeskew1MeanRMSVec[idx] += checkClockDeskewHalfDarkFrame(false,asic,startAddr,endAddr);
      idx++;
    }
  }

  //calculate mean values from accumulators for every asic half
  for(uint32_t idx = 0; idx < poweredAsics.size(); idx++){
    asicDeskew0MeanRMSVec[idx] /= m_iterations;
    asicDeskew1MeanRMSVec[idx] /= m_iterations;
  }
}

// mean value of all DNLRMS values for all pixels in selected asic half
double CHIPInterface::getMeanDNLRMSFromHistos(const  utils::DataHistoVec & pixelHistograms, int asic, int asicHalf)
{
  int idx = 0;
  double meanDNLRMS = 0.0;
  int numValues = 0;
  for(const auto & onPix : onPixels){
    if(calcAsicHalf(asic,onPix) == asicHalf){
      meanDNLRMS += pixelHistograms[idx].calcDNLRMS();
      numValues++;
    }
    idx++;
  }
  return meanDNLRMS/numValues;
}


//checks if whole ASICS are not powered.
//unpowered pixels are skipped
void CHIPInterface::calibrateClockDeskewDNL(const vector<uint32_t> & dacValues, int startAddr, int endAddr, int quarter)
{
  runTrimming = true;

  const string dnlTrimPixels = getDNLTrimPixels(quarter);
  static const int MAXDESKEW = 16;

  checkLadderReadout();

  updatePoweredPixels();
  const auto inactivePixels = getPowerDwnPixels();
  const auto inactiveStr = utils::positionVectorToList(inactivePixels);

  vector<uint32_t> poweredAsics = getPoweredAsics();
  if(poweredAsics.empty()){
    SuS_LOG_STREAM(info, log_id(), "No powered ASICs found - abort trimming");
    return;
  }

  int numAsics = poweredAsics.size();
  vector<vector<double>> asicDeskew0DNLRMSVec(numAsics);
  vector<vector<double>> asicDeskew1DNLRMSVec(numAsics);

  //enable only best dnl pixels, others can be ignored
  powerDownPixels("all");
  powerUpPixels(dnlTrimPixels);
  powerDownPixels(inactiveStr);
  updatePoweredPixels(); //init on pixels

  CalibIrampConfigurationKeeper calibIrampConfigKeeper(this);

  for(int deskew=0; deskew<MAXDESKEW && runTrimming; deskew++)
  {
    jtagRegisters->setSignalValue("Global Control Register","all","ClkDeskew_0",deskew);
    jtagRegisters->setSignalValue("Global Control Register","all","ClkDeskew_1",deskew);

    auto pixelHistograms = measureIntDacSweepForHistograms(dacValues,onPixels,startAddr,endAddr);

    // add the mean rms value for the measured clock deskew to the vector
    // to find the best value at the end
    int idx = 0;
    for(const auto & asic : poweredAsics)
    {
      double half0DNLRMS = getMeanDNLRMSFromHistos(pixelHistograms,asic,0);
      double half1DNLRMS = getMeanDNLRMSFromHistos(pixelHistograms,asic,1);

      asicDeskew0DNLRMSVec[idx].push_back(half0DNLRMS);
      asicDeskew1DNLRMSVec[idx].push_back(half1DNLRMS);
      idx++;

      //add status to trimming stats
      stringstream status;
      status << "Asic "           << setw(2) << asic;
      status <<" Deskew "         << setw(2) << deskew;
      status <<" DNLRMS 0 / 1 : " << setw(8) << half1DNLRMS << " / " << setw(8) << half1DNLRMS;
      trimmingStats.push_back(status.str());
    }
  }

  getBestDeskewSetting(poweredAsics,asicDeskew0DNLRMSVec,asicDeskew1DNLRMSVec);

  showTrimmingStats();

  powerUpPixels("all");
  powerDownPixels(inactiveStr);
}

//checks if whole ASICS are not powered.
//unpowered pixels are skipped
vector<double>  CHIPInterface::measureDNLRMSValues(const vector<uint32_t> & dacValues, int startAddr, int endAddr, int quarter)
{
  const string dnlTrimPixels = getDNLTrimPixels(quarter);

  checkLadderReadout();

  updatePoweredPixels();
  const auto inactivePixels = getPowerDwnPixels();
  const auto inactiveStr = utils::positionVectorToList(inactivePixels);

  vector<uint32_t> poweredAsics = getPoweredAsics();
  if(poweredAsics.empty()){
    SuS_LOG_STREAM(info, log_id(), "No powered ASICs found - abort trimming");
    return vector<double>();
  }

  int numAsics = poweredAsics.size();
  vector<vector<double>> asicDeskew0DNLRMSVec(numAsics);
  vector<vector<double>> asicDeskew1DNLRMSVec(numAsics);

  //enable only best dnl pixels, others can be ignored
  powerDownPixels("all");
  powerUpPixels(dnlTrimPixels);
  powerDownPixels(inactiveStr);
  updatePoweredPixels(); //init on pixels

  CalibIrampConfigurationKeeper calibIrampConfigKeeper(this);

  const auto pixelHistograms = measureIntDacSweepForHistograms(dacValues,onPixels,startAddr,endAddr);

  auto pixelDNLValues = utils::calcDNLRMSValuesFromHistograms(pixelHistograms);

  powerUpPixels("all");
  powerDownPixels(inactiveStr);

  return pixelDNLValues;
}


utils::DataHistoVec CHIPInterface::measureDNLValues(const std::vector<uint32_t> & pixelsToMeasure, const vector<uint32_t> & dacValues, int startAddr, int endAddr)
{
  dnlEvalsVec.resize(totalNumPxs);

  const auto pixelHistograms = measureIntDacSweepForHistograms(dacValues,pixelsToMeasure,startAddr,endAddr);

  cout << "Int DAC Sweep MEasured" << endl;
  const auto pixelDNLEvals = utils::calcDNLEvalsFromHistograms(pixelsToMeasure,pixelHistograms);
  cout << "DNL Computed" << endl;

  int idx = 0;
  for(auto && px: pixelsToMeasure){
    dnlEvalsVec[px] = pixelDNLEvals[idx++];
  }
  cout << "Evals assigned" << endl;

  return pixelHistograms;
}


void CHIPInterface::getBestDeskewSetting(const vector<uint32_t> & poweredAsics, const vector<vector<double>> & asicDeskew0DataVec, const vector<vector<double>> & asicDeskew1DataVec)
{
  int idx = 0;
  for(const auto & asic : poweredAsics)
  {
    int bestDeskew0 = (std::min_element(asicDeskew0DataVec[idx].begin(), asicDeskew0DataVec[idx].end())-asicDeskew0DataVec[idx].begin());
    int bestDeskew1 = (std::min_element(asicDeskew1DataVec[idx].begin(), asicDeskew1DataVec[idx].end())-asicDeskew1DataVec[idx].begin());

    //add final decision to trimming stats
    stringstream status;
    status << "Asic "               << setw(2) << asic;
    status <<" best deskew 0 / 1: " << setw(2) << bestDeskew0 << " / " << setw(2) << bestDeskew1;
    trimmingStats.push_back(status.str());

    jtagRegisters->setSignalValue("Global Control Register",to_string(asic),"ClkDeskew_0",bestDeskew0);
    jtagRegisters->setSignalValue("Global Control Register",to_string(asic),"ClkDeskew_1",bestDeskew1);
    idx++;
  }

  programJtagSingle("Global Control Register");
}


//checks if whole ASICS are not powered.
//unpowered pixels are skipped
void CHIPInterface::calibrateClockDeskewDarkFrame(int startAddr, int endAddr)
{
  runTrimming = true;

  static const int MAXDESKEW = 16;

  checkLadderReadout();

  updatePoweredPixels();

  vector<uint32_t> poweredAsics = getPoweredAsics();
  if(poweredAsics.empty()){
    SuS_LOG_STREAM(info, log_id(), "No powered ASICs found - abort trimming");
    return;
  }

  int numAsics = poweredAsics.size();

  vector<vector<double>> asicDeskew0MeanRMSVec(numAsics);
  vector<vector<double>> asicDeskew1MeanRMSVec(numAsics);

  std::vector<double> asicDeskew0RMSVec;
  std::vector<double> asicDeskew1RMSVec;

  for(int deskew=0; deskew<MAXDESKEW; deskew++)
  {
    jtagRegisters->setSignalValue("Global Control Register","all","ClkDeskew_0",deskew);
    jtagRegisters->setSignalValue("Global Control Register","all","ClkDeskew_1",deskew);

    programJtagSingle("Global Control Register");

    // measure m_iterations to gain better statistics
    // returns vectors of size of numAsics for both asic halfs
    // that contain the mean rms of all pixels of each half
    measureIterationsClockDeskewDarkFrame(startAddr,endAddr,poweredAsics,
                                          asicDeskew0RMSVec,
                                          asicDeskew1RMSVec);


    // add the mean rms value for the measured clock deskew to the vector
    // to find the best value at the end
    int idx = 0;
    for(const auto & asic : poweredAsics)
    {
      double half0RMSMean = asicDeskew0RMSVec[idx];
      double half1RMSMean = asicDeskew1RMSVec[idx];

      asicDeskew0MeanRMSVec[idx].push_back(half0RMSMean);
      asicDeskew1MeanRMSVec[idx].push_back(half1RMSMean);
      idx++;

      //add status to trimming stats
      stringstream status;
      status << "Asic "        << setw(2) << asic;
      status <<" Deskew "      << setw(2) << deskew;
      status <<" RMS 0 / 1 : " << setw(8) << half0RMSMean << " / " << setw(8) << half1RMSMean;
      trimmingStats.push_back(status.str());
    }

    if(!runTrimming) break;
  }

  getBestDeskewSetting(poweredAsics,asicDeskew0MeanRMSVec,asicDeskew1MeanRMSVec);

  showTrimmingStats();
}


double CHIPInterface::checkClockDeskewHalfDarkFrame(bool deskew0, int asic, int startAddr, int endAddr)
{
  std::vector<double> pixelRMSValues;

  for(uint32_t row = 0; row < 64; row++)
  {
    for(uint32_t col = 0; col < 32; col++)
    {
      uint32_t px = row*64+col;
      if(deskew0 == (asic>7) ){
        px+=32;
      }

      uint32_t imagePixel = getImagePixelNumber(asic,px);

      if(poweredPixels[imagePixel]){
        pixelRMSValues.emplace_back(calcSramPixelDataRMS(imagePixel,startAddr,endAddr));
      }
    }
  }

  return utils::getMeandAndRMS<double>(pixelRMSValues).mean;
}


std::vector<uint32_t> CHIPInterface::getSendingAsicsVec() const
{
  return utils::bitEnableValueToVector(sendingAsics);
}


std::vector<uint32_t> CHIPInterface::getSendingPixels() const
{
  const auto sendingAsicsVec =  getSendingAsicsVec();
  vector<uint32_t> sendingPixels;

  for(auto && asic : sendingAsicsVec){
    vector<uint32_t> asicPixels(numOfPxs);
    for(int px = 0; px < numOfPxs; px++){
      asicPixels[px] = getImagePixelNumber(asic,px);
    }
    sendingPixels.insert(sendingPixels.end(),asicPixels.begin(),asicPixels.end());
  }
  std::sort(sendingPixels.begin(),sendingPixels.end());
  return sendingPixels;
}


uint32_t CHIPInterface::getSendingASICIdx(uint32_t asic) const
{
  const auto sendingAsicsVec = utils::bitEnableValueToVector(sendingAsics);
  int idx =0;
  for(auto && sendingAsic : sendingAsicsVec){
    if(asic == sendingAsic) return idx;
    idx++;
  }
  cout << "asic number " << asic << "not sending " << endl;
  return 0;
}


void CHIPInterface::checkSendingASICs(std::string & selASICsStr) const
{
   auto selASICs = utils::positionListToVector(selASICsStr);
   checkSendingASICs(selASICs);
   selASICsStr = utils::positionVectorToList(selASICs);
}


void CHIPInterface::checkSendingASICs(std::vector<uint32_t> & selASICs) const
{
  const auto sendingASICs = utils::bitEnableValueToVector(sendingAsics);
  for(int i = selASICs.size() - 1; i>=0; --i){
    if(std::find(sendingASICs.begin(), sendingASICs.end(), selASICs[i]) == sendingASICs.end()) {
      selASICs.erase(selASICs.begin() + i);
    }
  }
}


//get number of ASIC Pixel in pixel registers and image
int CHIPInterface::getImagePixelNumber(int asic, int asicPixel) const
{
  if(getNumberOfActiveAsics() != 16 ) return asic * numOfPxs + asicPixel;

  int rowOffset = (asic>7)?  64 : 0;
  int colOffset = (asic%8) * 64;

  int row = asicPixel/64;
  int col = asicPixel%64;

  return (row + rowOffset)*512 + col + colOffset;
}


int CHIPInterface::getDataPixelNumber(int pixel)
{
  if(!isLadderReadout()) return pixel;

  if(getNumberOfActiveAsics() != 16 ) return pixel;

  int row = pixel/512;
  int col = pixel%512;

  int asicRow = row/64;
  int asicCol = col/64;

  row -= asicRow*64;
  col -= asicCol*64;

  int asic = asicRow * 8 + asicCol;
  int asicPixel = row*64+col;

  return getSendingASICIdx(asic) * numOfPxs + asicPixel;
}


int CHIPInterface::getASICNumberFromPixel(int pixel) const
{
  int row = pixel/512;
  int col = pixel%512;

  int asicRow = row/64;
  int asicCol = col/64;

  int asic = asicRow * 8 + asicCol;

  return asic;
}


int CHIPInterface::calcAsicHalf(uint32_t givenAsic, uint32_t pixel)
{
  uint32_t row = pixel/512;
  uint32_t col = pixel%512;

  uint32_t asicRow = row/64;
  uint32_t asicCol = col/64;

  uint32_t asic = asicRow * 8 + asicCol;

  if(asic != givenAsic) return -1;

  col -= asicCol*64;
  bool zeroHalf = false;
  if(asicRow == 0){
    if(col <  32){ zeroHalf = true; }
  }else{
    if(col >= 32){ zeroHalf = true; }
  }

  return (zeroHalf)? 0 : 1;
}


uint16_t CHIPInterface::getAsicDOs(uint16_t asics)
{
  uint16_t activeAsic = 0;
  for(int i=0; i<16; i++){
    if(asics&(1<<i)){
      activeAsic |= 1<<(asicDONumber[i]);
    }
  }
  return activeAsic;
}

uint16_t CHIPInterface::getExpectedTestPattern() const
{
  return getJTAGParam("Serializer Config Register","0","Serializer Output Test Pattern");
}


std::string CHIPInterface::getASICSInfoStr(uint16_t asics)
{
  string info = "00000000_00000000";
  for(int i=0; i<16; i++){
    int pos = 16-i-((i>=8)? 1 : 0);
    if((1<<i) & asics)  info[pos] = '1';
  }
  return info;
}


//no extension required in PPT
void CHIPInterface::loadFullConfig(const string & fileName, bool program)
{
  fullChipConfig->loadFullConfig(fileName);

  sequencer = fullChipConfig->getSequencer();
  pixelRegisters = fullChipConfig->getPixelReg(0);
  jtagRegisters = fullChipConfig->getJtagReg(0);

  setD0Mode(checkD0Mode(),checkBypCompression());

  if(program){
    initChip();
  }
}

bool CHIPInterface::checkRegisterTypes()
{
  if(pixelRegisters->signalNameExists("Control register","CSA_FbCap")){
    cerr << "F1CHIPInterface: ERROR no F1 configuration found for pixel registers" << endl;
    return false;
  }


  if(jtagRegisters->signalNameExists("Global Control Register","PxInj_UseBG_NotHD_DAC")){
    cerr << "F1CHIPInterface: ERROR no F1 configuration found for jtag registers" << endl;
    return false;
  }

  return true;
}



void CHIPInterface::storeFullConfigFile(const string & fileName, bool keepName)
{
  fullChipConfig->storeFullConfigFile(fileName,keepName);
}


vector<string> CHIPInterface::getJtagRegOutputList()
{
  const int numAsics = getNumberOfActiveAsics();
  vector<string> outList;

  if(numAsics == 16){
    for(int i=asicInJTAGChain.size()-1; i>=0; --i){
      outList.push_back(to_string(asicInJTAGChain[i]));
    }
  }else{
    for(int i=0; i<numAsics; ++i){
      outList.push_back(to_string(i));
    }
  }
  return outList;
}


vector<string> CHIPInterface::getPixelRegOutputList()
{
  //asicInJTAGChain{15,14,13,12,11,10,9,8,0,1,2,3,4,5,6,7}; <- first to program

  const int numAsics = getNumberOfActiveAsics();
  vector<string> outList(numAsics*numOfPxs,"66666");

  int nextOut = 0;
  if( getNumberOfActiveAsics() == 16)
  {
    int i=0;
    for(int asicIdx = 0; asicIdx < 16; asicIdx++){
      int asic = asicInJTAGChain[asicIdx];
      bool flip = (asic > 7);
      for(int asicOut=0; asicOut<numOfPxs; asicOut++){
        int col = asicOut / c_pxsPerCol;
        if(col>31) col = c_pxsPerCol-col +31;
        if(flip)   col = c_pxsPerCol-col - 1;

        int row = (flip)? asicOut % c_pxsPerCol : c_pxsPerCol - (asicOut % c_pxsPerCol) - 1;

        int asicPixel = row*c_pxsPerRow + col;

        nextOut = getImagePixelNumber(asic,asicPixel);
        outList[i++] = to_string(nextOut);
      }
    }
  }
  else
  {
    int i=0;
    for(int asicIdx = 0; asicIdx < numAsics; asicIdx++){
      for(int asicOut=0; asicOut<numOfPxs; asicOut++){
        int col = asicOut / c_pxsPerCol;
        if(col>31){
          col = 64-col+31;
        }
        int row = c_pxsPerCol - (asicOut % c_pxsPerCol) - 1;
        nextOut = asicIdx*numOfPxs + row*c_pxsPerRow + col;
        outList[i++] = to_string(nextOut);
      }
    }
  }

  return outList;
}


void CHIPInterface::showTrimmingStats()
{
  for(const auto & stat : trimmingStats){
    cout << stat << endl;
  }
  trimmingStats.clear();
}


std::string CHIPInterface::getGoodTrimmedPixelsStr(std::vector<double> & trimmedSlopes, string & rangeStr, const double aimSlope, const double maxAbsDiff, int & goodSlopesRel) const
{
  const auto finalSlopes = getFinalSlopes();
  trimmedSlopes.clear();
  int goodSlopes = 0;
  for(const double & slope : finalSlopes)
  {
    //if(slope != 0.0)
    if(slope > 0.0)
    {
      trimmedSlopes.push_back(slope);

      if(fabs(slope-aimSlope) < maxAbsDiff){
        goodSlopes++;
      }
    }
  }

  int numTrimmed = trimmedSlopes.size();
  goodSlopesRel = 100.0*goodSlopes/numTrimmed;

  stringstream rangess;
  rangess << numTrimmed << " Pixels trimmed - ";
  rangess << goodSlopes << " (" << goodSlopesRel << "%) in " << utils::setPrecision(utils::calcPerCent(maxAbsDiff,aimSlope),2) << "% range. ";
  rangeStr = rangess.str();

  stringstream ss;
  ss << onPixels.size() << " Pixels powered. ";
  ss << rangeStr;
  ss << getUntrimmablePixels().size() << " untrimmable Pixels found";

  return ss.str();
}



bool CHIPInterface::setCompDacConfigCalibrated()
{
  jtagRegisters->setSignalValue("Global Control Register","all","Pixel Injection Bias Current Trim",2);

  initChip();

  //remember pixel configuration since it is changed in calibration function
  vector<vector<int>> configRem;
  for(int px = 0 ; px < totalNumPxs; px++)
  {
    int FCF_EnCap     = getPixelRegisterValue(to_string(px),"FCF_EnCap");
    int enPXInjDC     = getPixelRegisterValue(to_string(px), "EnPxInjDC");
    int EnD0          = getPixelRegisterValue(to_string(px), "EnD0");
    int InvertInject  = getPixelRegisterValue(to_string(px), "InvertInject");
    vector<int> pxConfig {FCF_EnCap,enPXInjDC,EnD0,InvertInject};
    configRem.push_back(pxConfig);
  }

  bool ok = calibratePXInjCurrCompDAC();

  for(int px = 0 ; px < totalNumPxs; px++)
  {
    int i=0;
    setPixelRegisterValue(to_string(px),"FCF_EnCap",configRem[px][i++]);
    setPixelRegisterValue(to_string(px),"EnPxInjDC",configRem[px][i++]);
    setPixelRegisterValue(to_string(px),"EnD0",configRem[px][i++]);
    setPixelRegisterValue(to_string(px),"InvertInject",configRem[px][i++]);
  }
  return ok;
}


bool CHIPInterface::calibrateCurrCompIntDAC()
{
  checkLadderReadout();

  ROModeKeeper keepMeUntilEndOfScope(false,false,false,1/*numOfFrames*/,0,this);

  jtagRegisters->setSignalValue("Global Control Register","all","VDAC_lowrange",0);
  jtagRegisters->setSignalValue("Global Control Register","all","VDAC_highrange",1);

  int maxOK = 0;
  int maxOKIntDAC = 0;

  for(int intDAC=3600;intDAC<8000;intDAC+=20)
  {
    setIntDACValue(intDAC);

    if (!doSingleCycle()) {
      SuS_LOG_STREAM(error, log_id(), "+++++ DATA RECEIVER ERROR Current Compensation DAC trimming aborted.");
      return false;
    }
    int pxTooSmall=0;
    int pxTooLarge=0;
    int pxOK = 0;
    for(int px=0; px<totalNumPxs; px++){
      const uint16_t vhold = *(getPixelSramData(px));
      if (vhold > 210 && vhold < 260){
        pxTooSmall++;
      }else if(vhold == 511){
        pxTooLarge++;
      }else{
        pxOK++;
      }
    }
    if(pxOK>maxOK){
      maxOK = pxOK;
      maxOKIntDAC = intDAC;
    }

    if(pxTooLarge==totalNumPxs){
      break;
    }
  }
  setIntDACValue(maxOKIntDAC);

  return true;
}


bool CHIPInterface::calibrateDay0CurrCompDAC()
{

  setPixelRegisterValue( "all", "RmpCurrDouble", 1);
  setPixelRegisterValue( "all", "RmpFineTrm", 31);
  setPixelRegisterValue( "all", "ADC_EnExtLatch", 0);
  setPixelRegisterValue( "all", "BypassFE", 0);
  setPixelRegisterValue( "all", "FCF_EnCap", 3);

  setPixelRegisterValue( "all", "CompCoarse", 4);
  setPixelRegisterValue( "all", "EnPxInjDC", 0);
  setPixelRegisterValue( "all", "EnPxInj", 0);
  setPixelRegisterValue( "all", "EnD0", 1);
  setPixelRegisterValue( "all", "InvertInject", 1);

  programPixelRegs();

  calibrateCurrCompIntDAC();

  return calibrateCurrCompDAC();
}


void CHIPInterface::jtagClockInSramTestPattern(uint16_t testPattern)
{
  cout << "Clocking test pattern into the pixel readout registers." << endl;

  for(int row=0; row<c_pxsPerCol; ++row){
    for (int i=8; i>=0; --i) {
      setRoSerIn(((uint16_t)(testPattern) & (1 << i)) != 0);
      usleep(1000);
      jtagRegisters-> setSignalValue("JTAG SRAM Control", "all", "Enable SRAM Control through JTAG", 1);
      jtagRegisters-> setSignalValue("JTAG SRAM Control", "all", "RO_Phi1", 1);
      programJtagSingle("JTAG SRAM Control");
      jtagRegisters-> setSignalValue("JTAG SRAM Control", "all", "RO_Phi1", 0);
      programJtagSingle("JTAG SRAM Control");
      jtagRegisters-> setSignalValue("JTAG SRAM Control", "all", "RO_Phi2", 1);
      programJtagSingle("JTAG SRAM Control");
      jtagRegisters-> setSignalValue("JTAG SRAM Control", "all", "RO_Phi2", 0);
      programJtagSingle("JTAG SRAM Control");
      usleep(1000);
    }
  }

  setRoSerIn(0);
  jtagRegisters-> setSignalValue("JTAG SRAM Control", "all", "Enable SRAM Control through JTAG", 0);
  programJtagSingle("JTAG SRAM Control");

  waitJTAGEngineDone();
}


bool CHIPInterface::fillSramAndReadout(uint16_t pattern, bool init, bool jtagMode)
{
  bool ok = true;
  int cnt = 0;
  do {
    runContinuousMode(false);
    if(init){
      setSendRawData(true);
      writeFpgaRegisters();
      jtagRegisters-> setSignalValue("SRAM Controller Config Register", "all", "Send Test Data", 1);
      programJtagSingle("SRAM Controller Config Register");
    }
    if(getNumberOfActiveAsics()>1){
      setLadderReadout(true);
    }

    jtagClockInSramTestPattern(pattern);
    //CHIPInterface::jtagClockInSramTestPattern(pattern);
    //waitJTAGEngineDone(); already in jtagClockInSramTestPattern

    ok = doSingleCycle(1);
    cnt++;
    if(!ok){
      cout << "Error: could not read data at try " << cnt << endl;
    }
  } while(ok==false && cnt<10);

  if(init){
    jtagRegisters-> setSignalValue("SRAM Controller Config Register", "all", "Send Test Data", 0);
    programJtagSingle("SRAM Controller Config Register");
    setSendRawData(false,false,true);
    writeFpgaRegisters();
  }

  return ok;
}


void CHIPInterface::setExtLatchMode()
{
  setPixelRegisterValue("all","ADC_EnExtLatch",1);
  programPixelRegs();

  setSequencerMode(Sequencer::EXTLATCH);
}


void CHIPInterface::setIntDACMode()
{
  setPixelRegisterValue("all","EnPxInj",0);
  setPixelRegisterValue("all","FCF_SelExtRef",1);
  setPixelRegisterValue("all","ADC_EnExtLatch",0);
  setPixelRegisterValue("all","InvertInject",0);
  setPixelRegisterValue("all","EnD0",0);
  setPixelRegisterValue("all","D0_BypComprResistor",0);
  programPixelRegs();

  setSequencerMode(Sequencer::BUFFER);
}


void CHIPInterface::setNormalMode()
{
  setPixelRegisterValue("all","EnPxInj",0);
  setPixelRegisterValue("all","FCF_SelExtRef",0);

  setPixelRegisterValue("all","ADC_EnExtLatch",0);

  setPixelRegisterValue("all","InvertInject",d0Mode);
  setPixelRegisterValue("all","EnD0",d0Mode);
  setPixelRegisterValue("all","D0_BypComprResistor",d0BypCompr);
  programPixelRegs();

  setSequencerMode(Sequencer::NORM);
}

void CHIPInterface::setPixelInjectionMode()
{
  setPixelRegisterValue("all","EnPxInj",1);
  setPixelRegisterValue("all","FCF_SelExtRef",0);

  setPixelRegisterValue("all","ADC_EnExtLatch",0);

  setPixelRegisterValue("all","InvertInject",d0Mode);
  setPixelRegisterValue("all","EnD0",d0Mode);
  setPixelRegisterValue("all","D0_BypComprResistor",d0BypCompr);

  programPixelRegs();

  setSequencerMode(Sequencer::NORM);
}


void CHIPInterface::setSingleIntegrationMode(bool enable)
{
  const auto newMode = enable? Sequencer::SINGLEINT : Sequencer::NORM;
  setSequencerMode(newMode);
}

void CHIPInterface::setSequencerMode(Sequencer::OpMode newMode)
{
  if(sequencer->getOpMode() != newMode){
    sequencer->setOpMode(newMode);
    programSequencer();
  }
}



void CHIPInterface::checkLadderReadout()
{
  if(getNumberOfActiveAsics()==16){
    if(!isLadderReadout()){
      SuS_LOG_STREAM(warning, log_id(), "16 ASICs in configuration found, enabeling LAdderReadout for correct data assignment...please wait");
      setLadderReadout(true);
    }
  }
}


void CHIPInterface::updateMemberVectorSizes()
{
  untrimmablePixels.resize(totalNumPxs,false);
  upperVHoldVals.resize(totalNumPxs,0);
  currentVholdVals.resize(totalNumPxs,0);
  poweredPixels.resize(totalNumPxs);
  pixelCurrCompCalibratedVec.resize(totalNumPxs,false);
  finalSlopes.assign(totalNumPxs,0.0);
  sramDrift.assign(totalNumPxs,0.0);
  baselineValuesValid = false;
  baselineValues.assign(totalNumPxs,0.0);
  curvatureValues.assign(totalNumPxs,0.0);
  meanSramContent.assign(totalNumPxs*sramSize,0.0);
}


uint16_t CHIPInterface::getASICTrailerWord(const std::vector<uint8_t> & trailerData, int asic, int wordNum)
{
  constexpr size_t numTrailerBytes = 8*16*2;
  if(trailerData.size() != numTrailerBytes){
    cout << "ChipInterface: Trailerdata has wrong size: " << trailerData.size() << "/" << numTrailerBytes << endl;
    return 0;
  }

  int asicDO = asicDONumber[asic];
  int byteOffs = (wordNum*16 + asicDO)*2;

  const uint8_t *data  = trailerData.data();
  uint16_t value = (*((uint16_t *)&data[byteOffs]));

  return value;
}


void CHIPInterface::exportSingleASIC(int ASIC, const std::string & outFileName)
{
  ConfigReg newPixelRegs(*pixelRegisters);
  auto numModules = newPixelRegs.getNumModules("Control register");

  newPixelRegs.removeModules("Control register", to_string(numOfPxs) + "-" + to_string(numModules-1));

  vector<uint32_t> asicPixels(numOfPxs,0);

  for(int i=0; i<numOfPxs; i++){
    asicPixels[i] = getImagePixelNumber(ASIC,i);
  }

  if(ASIC>=8){
    sort(asicPixels.begin(),asicPixels.end());
  }



  const auto setSignalNames = pixelRegisters->getSignalNames("Control register");

  for(const auto & signalName : setSignalNames)
  {
    int idx = 0;
    for(const auto & pixel : asicPixels){
      const auto value = pixelRegisters->getSignalValue("Control register",to_string(pixel),signalName);
      newPixelRegs.setSignalValue("Control register",to_string(idx++),signalName,value);
    }
  }

  newPixelRegs.saveToFile(outFileName);
}



void CHIPInterface::initChipDataParams(int pixel, vector<string> &paramNames, vector<int> & params)
{
  auto pixelSignalNames = getPixelRegisterSignalNames(pixel);
  paramNames.insert(paramNames.end(),pixelSignalNames.begin(),pixelSignalNames.end());
  for(const auto & signalName : pixelSignalNames){
    params.push_back(getPixelRegisterValue(to_string(pixel),signalName));
  }

  paramNames.push_back("IntVdacSetting");
  params.push_back(getIntDACValue());

  for(auto const & p : sequencer->getSequencerParameterMap()) {
    params.push_back(p.second);
    paramNames.push_back(p.first);
  }
  //paramNames.push_back("IntegrationLength");
  //params.push_back(sequencer->getSequencerParameter(Sequencer::IntegrationLength));
  //paramNames.push_back("FlattopLength");
  //params.push_back(sequencer->getSequencerParameter(Sequencer::FlattopLength));
  //paramNames.push_back("FlattopHoldLength");
  //params.push_back(sequencer->getSequencerParameter(Sequencer::FlattopHoldLength));
  //paramNames.push_back("ResetLength");
  //params.push_back(sequencer->getSequencerParameter(Sequencer::ResetLength));
  //paramNames.push_back("ResetIntegOffset");
  //params.push_back(sequencer->getSequencerParameter(Sequencer::ResetIntegOffset));
  //paramNames.push_back("ResetHoldLength");
  //params.push_back(sequencer->getSequencerParameter(Sequencer::ResetHoldLength));
  //paramNames.push_back("RampLength");
  //params.push_back(sequencer->getSequencerParameter(Sequencer::RampLength));
  //paramNames.push_back("BackFlipAtReset");
  //params.push_back(sequencer->getSequencerParameter(Sequencer::BackFlipAtReset));
  //paramNames.push_back("BackFlipToResetOffset");
  //params.push_back(sequencer->getSequencerParameter(Sequencer::BackFlipToResetOffset));
  //paramNames.push_back("HoldPos");
  //params.push_back(sequencer->getSequencerParameter(Sequencer::HoldPos));
  //paramNames.push_back("HoldLength");
  //params.push_back(sequencer->getSequencerParameter(Sequencer::HoldLength));
  //paramNames.push_back("RampOffset");
  //params.push_back(sequencer->getSequencerParameter(Sequencer::RampOffset));
  //paramNames.push_back("SingleSHCapMode");

  params.push_back(sequencer->isSingleSHCapMode());
  paramNames.push_back("SeqOperationMode");
  params.push_back((int)sequencer->getOpMode());

  paramNames.push_back("Vhold");
  params.push_back(currentVholdVals[pixel]);

  paramNames.push_back("TempADC");
  params.push_back(0);

  paramNames.push_back("TestsystemMainboardTemp_MSB");
  params.push_back(0);
  paramNames.push_back("TestsystemMainboardTemp_LSB");
  params.push_back(0);
  paramNames.push_back("TestsystemFPGAboardTemp_MSB");
  params.push_back(0);
  paramNames.push_back("TestsystemFPGAboardTemp_LSB");
  params.push_back(0);
  paramNames.push_back("TestsystemSensorTemp_MSB");
  params.push_back(0);
  paramNames.push_back("TestsystemSensorTemp_LSB");
  params.push_back(0);

  paramNames.push_back("TrainID");
  params.push_back(0);

  paramNames.push_back("BurstTime");
  params.push_back(0);

  auto jtagSignalNames = jtagRegisters->getSignalNames("Global Control Register");
  paramNames.insert(paramNames.end(),jtagSignalNames.begin(),jtagSignalNames.end());
  for(const auto & signalName : jtagSignalNames){
    params.push_back(jtagRegisters->getSignalValue("Global Control Register","0",signalName));
  }
    SuS_LOG_STREAM(debug, log_id(), "initChipDataParams() done");
}


void CHIPInterface::enableInjection(bool en, const string & pixelStr, bool program)
{
  disableInjectionBits();

  if(en){
    setPixelRegisterValue(pixelStr,"EnPxInj",1);
  }

  m_injectionEnabled = en;

  if (program) programPixelRegs();
}


void CHIPInterface::disableInjectionBits()
{
  m_injectionEnabled = false;

  cout << "######injection bits disabled" << endl;
  setPixelRegisterValue( "all", "BypassFE", 0);
  setPixelRegisterValue( "all", "ADC_EnExtLatch",0);
  setPixelRegisterValue( "all", "EnPxInj", 0);
  setPixelRegisterValue( "all", "FCF_SelExtRef", 0);
}


void CHIPInterface::setInjectionDAC(int setting)
{

  switch (injectionMode) {
    case CURRENT_BGDAC :
      jtagRegisters->setSignalValue("Global Control Register", "all", "Pixel Injection Signal Trim", setting);
      programJtagSingle("Global Control Register");
      break;
    case ADC_INJ_LR :
    case ADC_INJ :
      setIntDACValue(setting);
      break;
    case EXT_LATCH :
      sequencer->setExtLatchSlot(setting);
      break;
    case NORM :
    case NORM_D0 :
    case NORM_DEPFET :
      cout << "CHIPInterface: no injection mode selected:" << getInjectionModeName(injectionMode) << endl;
      break;
    default :
      cout << "CHIPInterface: injection mode " << getInjectionModeName(injectionMode) << " not available" << endl;
  }
}


void CHIPInterface::setInjectionMode(const std::string & modeStr)
{
    auto mode = getInjectionMode(modeStr);
    if(mode != injectionMode){
      setInjectionMode(mode);
    }
}


CHIPInterface::InjectionMode CHIPInterface::getInjectionMode()
{
  return injectionMode;
}


CHIPInterface::InjectionMode CHIPInterface::getInjectionMode(const std::string & modeName)
{
  auto activeModes = getActiveModes();
  for(auto && mode : activeModes){

    if(modeName == getInjectionModeName(mode)){
      return mode;
    }
  }

  return InjectionMode::NORM;
}


std::string CHIPInterface::getInjectionModeName(CHIPInterface::InjectionMode m) {
  switch (m) {
    case CURRENT_BGDAC       : return "CURRENT_BGDAC";
    case CURRENT_SUSDAC      : return "CURRENT_SUSDAC";
    case CHARGE_PXINJ_BGDAC  : return "CHARGE_PXINJ_BGDAC";
    case CHARGE_PXINJ_SUSDAC : return "CHARGE_PXINJ_SUSDAC";
    case CHARGE_BUSINJ       : return "CHARGE_BUSINJ";
    case ADC_INJ             : return "ADC_INJ";
    case ADC_INJ_LR          : return "ADC_INJ_LR";
    case EXT_LATCH           : return "EXT_LATCH";
    // operation modes without injection abilities
    // NORM mode can be used to return to previous mode
    // other two modes are setting defined modes
    case NORM                : return (d0ModeRem) ? "NORM_D0" : "NORM_DEPFET";
    case NORM_D0             : return "NORM_D0";
    case NORM_DEPFET         : return "NORM_DEPFET";
  }
  return "unknown";
}


int CHIPInterface::getInjectionDACsetting()
{
  int value = 0;
  switch (injectionMode) {
    case CURRENT_BGDAC :
    case CHARGE_PXINJ_BGDAC:
      value = jtagRegisters->getSignalValue("Global Control Register", "all", "Pixel Injection Signal Trim");
      break;
    case ADC_INJ :
      value = getIntDACValue();
      break;
    case EXT_LATCH :
      value = sequencer->getExtLatchSlot();
      break;
    case NORM :
    case NORM_D0 :
    case NORM_DEPFET :
    default :
      cout << "CHIPInterface: injection mode " << getInjectionModeName(injectionMode) << " not available" << endl;
  }
  return value;
}


void CHIPInterface::setInjectionMode(InjectionMode mode)
{
  injectionMode = mode;

  disableInjectionBits();

  switch (mode) {
    case CURRENT_BGDAC :
      setD0Mode(false);
      setPixelInjectionMode();
      break;

    case CHARGE_PXINJ_BGDAC :
      setD0Mode(true);
      setPixelInjectionMode();
      break;

    case ADC_INJ :
      setPixelRegisterValue( "all", "FCF_EnCap",15);
      setJTAGParam("Global Control Register","all","VDAC_lowrange",0);
      setJTAGParam("Global Control Register","all","VDAC_highrange",1);
      setGGCStartValue(DEFAULTGCCSTART); // programs global control register
      setSequencerMode(Sequencer::BUFFER);
      setIntDACMode();
      break;

    case ADC_INJ_LR :
      setPixelRegisterValue( "all", "FCF_EnCap",15);
      setJTAGParam("Global Control Register","all","VDAC_lowrange",1);
      setJTAGParam("Global Control Register","all","VDAC_highrange",0);
      setGGCStartValue(DEFAULTGCCSTART); // programs global control register
      setSequencerMode(Sequencer::BUFFER);
      setIntDACMode();
      break;

    case EXT_LATCH :
      setExtLatchMode();
      break;

    case NORM :
      setD0Mode(d0ModeRem,d0BypComprRem);
      setNormalMode();
      break;

    case NORM_D0 :
      d0ModeRem = true;
      setD0Mode(d0ModeRem,d0BypComprRem);
      setNormalMode();
      break;

    case NORM_DEPFET :
      d0ModeRem = false;
      setD0Mode(d0ModeRem,d0BypComprRem);
      setNormalMode();
      break;

    default :
      SuS_LOG_STREAM(warning, log_id(), "Injection mode " << getInjectionModeName(injectionMode) << " not available");
      //noop;
  }

  SuS_LOG_STREAM(info, log_id(), "F1 Setting injection mode to " << getInjectionModeName(injectionMode));
}


void CHIPInterface::removeUnpoweredPixels(std::vector<uint32_t> & pixels)
{
  for (auto it=pixels.begin(); it!=pixels.end(); it++) {
    if (!isPoweredPixel(*it)) {
      pixels.erase(it);
      --it;
    }
  }
  if (pixels.empty()) {
    SuS_LOG_STREAM(warning, log_id(), "All pixels removed, no pixels were powered.");
  }
}


void CHIPInterface::setSramBlacklistForTrimming(const std::string &blacklistFileName)
{
  m_sramBlacklist.initFromFile(blacklistFileName);
}


int CHIPInterface::saveSramTestResult(const std::string & filePath, const std::string & moduleInfo, uint16_t testpattern )
{
  const int numSram = getNumFramesToSend();

  utils::makePath(filePath);

  // testPath should be used to store Measurement Output files
  const string fileName = filePath + "/" + utils::getLocalTimeStr() + "_SRAMTestResult.txt";
  ofstream out(fileName);
  out << "#BadSramCells File\n";
  out << "#From CHIPInterface\n";
  out << moduleInfo << "\n";

  int errCnt_thisPattern = 0;
  const auto asicsVec = getSendingAsicsVec();
  const int numASICs = asicsVec.size();
  for(int asicIdx = 0; asicIdx < numASICs; asicIdx++)
  {
    const uint asic = asicsVec[asicIdx];
    const string pxInfo = (numASICs>1)? "ASIC " + to_string(asic) + " pixel " : "pixel ";
    const auto * asicOffs = utils::s_imagePixelMap.data() + asic * utils::s_numAsicPixels;
    for (uint px = 0; px < utils::s_numAsicPixels; ++px){
      const uint imagePixel = asicOffs[px];
      const auto data = getPixelSramData(imagePixel);
      for (int sramaddr = 0; sramaddr < numSram; ++sramaddr)
      {
        if (data[sramaddr] != testpattern){
          out << "Error in " << pxInfo << px << ", address " << sramaddr << ". Read value: " << data[sramaddr] << "\n";
          errCnt_thisPattern++;
        }
      }
    }
  }

  out << "#" << errCnt_thisPattern <<" errors found" << endl;
  out.close();

  return errCnt_thisPattern;
}

