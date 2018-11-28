#include "CHIPGainConfigurator.h"

using namespace SuS;
using namespace std;

#ifdef F1IO
const std::vector<std::string> CHIPGainConfigurator::s_gainModeParamNames = {"D0_EnMimCap","FCF_EnIntRes","FCF_EnCap","IntegrationTime"};
#else
const std::vector<std::string> CHIPGainConfigurator::s_gainModeParamNames = {"CSA_FbCap","CSA_Resistor","FCF_EnCap","IntegrationTime"};
#endif

const std::vector<std::string> CHIPGainConfigurator::s_gainModeNames = {"Gain_0.5keV/1Bin",
                                                                        "Gain_0.7keV/1Bin",
                                                                        "Gain_0.7keV/1alt",
                                                                        "Gain_1.0keV/1Bin",
                                                                        "Gain_2keV/2Bin",
                                                                        "Gain_1.5keV/1Bin",
                                                                        "Gain_1.4keV/2Bin",
                                                                        "Gain_10keV/1Bin"};

// PArameter names are definded in DsscGainParamMap :                              {"CSA_FbCap","CSA_Resistor","FCF_EnCap","IntegrationTime"};
const std::map<std::string,CHIPGainConfigurator::GainConfig> SuS::CHIPGainConfigurator::s_typicalConfigs = {
                                                                                        {"Gain_0.5keV/Bin",{0,3,1,35}},
                                                                                        {"Gain_0.7keV/Bin",{0,1,1,35}},
                                                                                        {"Gain_0.7keV/alt",{0,3,2,35}},
                                                                                        {"Gain_1.0keV/Bin",{0,3,3,35}},
                                                                                        {"Gain_2keV/2Bin", {0,3,5,35}},
                                                                                        {"Gain_1.5keV/Bin",{0,1,3,35}},
                                                                                        {"Gain_1.4keV/2Bin",{0,3,4,35}},
                                                                                        {"Gain_10keV/Bin", {0,3,8,35}}
                                                                                      };
/*
const std::map<std::string,double> SuS::CHIPGainConfigurator::s_typicalSlopes = { {"Gain_0.5keV/Bin",0.008},
                                                                                  {"Gain_0.7keV/Bin",0.008},
                                                                                  {"Gain_0.7keV/alt",0.008},
                                                                                  {"Gain_1.0keV/Bin",0.008},
                                                                                  {"Gain_2keV/Bin",  0.008},
                                                                                  {"Gain_5keV/Bin",  0.008},
                                                                                  {"Gain_10keV/Bin", 0.008}
                                                                                };
 */
CHIPGainConfigurator::CHIPGainConfigurator()
  : m_chip(nullptr)
{}

CHIPGainConfigurator::CHIPGainConfigurator(CHIPInterface * chip)
  : m_chip(chip)
{
}

CHIPGainConfigurator::~CHIPGainConfigurator()
{}

void CHIPGainConfigurator::setChipInterface(CHIPInterface * chip)
{
  m_chip = chip;
  updateModeFromChipConfig();
}

double CHIPGainConfigurator::getGainModeEV(const std::string & gainMode)
{
  size_t _pos = gainMode.find_first_of('_')+1;
  size_t kpos = gainMode.find("keV");
  std::string value = gainMode.substr(_pos,kpos-_pos);
  double gain;
  std::istringstream iss(value);
  iss >> gain;
  return (gain*1000.0);
}

std::string CHIPGainConfigurator::getGainModeName(double gainEv)
{
  for(auto && gainStr : s_gainModeNames){
    double ev = getGainModeEV(gainStr);
    if(ev == gainEv){
      return gainStr;
    }
  }
  return "Gain_"+to_string((int)gainEv)+ "eV/1Bin";
}



int CHIPGainConfigurator::getConfigValue(const std::string & gainConfigName, const std::string & paramName)
{
  const auto config = s_typicalConfigs.at(gainConfigName);
  int idx = 0;
  for(auto && gainModeParamName : s_gainModeParamNames)
  {
    if(paramName == gainModeParamName){
      return config[idx];
    }
    idx++;
  }
  return 0;
}


void CHIPGainConfigurator::activateGainMode()
{
  cout << "ActivateGainMode = " << m_currentGainMode << endl;

  const auto currentGainConfig = getActiveConfigMap();

  int intTimeRem = m_chip->sequencer->getIntegrationTime(true);
  bool programSequencer = true;

  for(int idx = 0; idx<4; idx++){
    auto value = currentGainConfig[idx];
    auto name  = s_gainModeParamNames[idx];
    if(name == "IntegrationTime"){
      programSequencer = (value != intTimeRem);
      m_chip->sequencer->setIntegrationTime(value);
    }else{
      m_chip->setPixelRegisterValue("all",name,value);
    }
  }

  m_chip->programPixelRegs();
  if(programSequencer){
   m_chip->programSequencer();
  }
}


void CHIPGainConfigurator::updateModeFromChipConfig()
{
//  DsscGainParamMap newMap;
//  cout << m_chip->pixelRegisters->getSignalNames("Control register").front() << endl;
//  for(auto && paramName : DsscGainParamMap::s_gainModeParamNames){
//    if(paramName == "IntegrationTime"){
//      newMap[paramName] = m_chip->sequencer->getIntegrationTime();
//    }else{
//      newMap[paramName] = m_chip->getPixelRegisterValue("0",paramName);
//    }
//  }
//
//  for(auto && elem : s_typicalConfigs)
//  if(newMap == elem.second){
//    setGainMode(elem.first);
//    return;
//  }

  setGainMode("Gain_10keV/Bin");
}


void CHIPGainConfigurator::loadGainConfiguration(const std::string & gainParamName, const std::string & gainConfigFileName, bool program)
{
  const auto spectrumGainMapResults = utils::importSpectrumGainMapFitResultsFromASCII(gainConfigFileName);
  for(auto && pixelGainMapResult : spectrumGainMapResults){
    m_chip->setPixelRegisterValue(to_string(pixelGainMapResult.pixel),gainParamName,pixelGainMapResult.gainSetting);
  }
  if(program){
    m_chip->programPixelRegs();
  }
  int pos = gainConfigFileName.find_last_of(".");
  std::string newFullConfigFileName = gainConfigFileName.substr(0,gainConfigFileName.length()-pos) + ".conf";
  m_chip->storeFullConfigFile(newFullConfigFileName);
}



