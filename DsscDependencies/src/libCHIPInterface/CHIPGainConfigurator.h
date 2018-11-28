#ifndef CHIPGAINCONFIGURATOR_H
#define CHIPGAINCONFIGURATOR_H

#include <string>
#include <vector>
#include <array>
#include <map>

#include "CHIPInterface.h"
//#include "DsscGainParamMap.h"

namespace SuS{

class CHIPInterface;

class CHIPGainConfigurator
{
  public:
    CHIPGainConfigurator();
    CHIPGainConfigurator(CHIPInterface * chip);

    ~CHIPGainConfigurator();

    void setChipInterface(CHIPInterface * chip);

    using GainConfig = std::array<int,4>;

    static const std::vector<std::string> s_gainModeNames;
    static const std::vector<std::string> s_gainModeParamNames;
    static const std::map<std::string,GainConfig> s_typicalConfigs;

    static double getGainModeEV(const std::string & gainMode);
    static std::string getGainModeName(double gainEv);

    void updateModeFromChipConfig();
//    bool configEqual(const DsscGainParamMap &map1, const DsscGainParamMap & map2);

    void activateGainMode(const std::string & newModeStr)
    {
      setGainMode(newModeStr);
      activateGainMode();
    }

    void loadGainConfiguration(const std::string & gainParamName, const std::string & gainConfigFileName, bool program);

    void setGainMode(const std::string & newModeStr)
    {
      auto it = s_typicalConfigs.find(newModeStr);
      if(it != s_typicalConfigs.end()){
        m_currentGainMode = newModeStr;
      }else{
        std::cout << " Gain Mode Not Found" << std::endl;
      }
    }

    GainConfig getActiveConfigMap() const { return s_typicalConfigs.at(m_currentGainMode);}
    //double getTypicalSlope()              const { return s_typicalSlopes.at(m_currentGainMode);}
    std::string getCurrentGainMode()      const { return m_currentGainMode;}

    GainConfig getActiveConfigMap(const std::string & newModeStr) const { return s_typicalConfigs.at(newModeStr);}
    //double getTypicalSlope(const std::string & newModeStr)        const { return s_typicalSlopes.at(newModeStr);}

    static int getConfigValue(const std::string & gainConfigName, const std::string & paramName);

  private:
    void activateGainMode();

    CHIPInterface * m_chip;

    std::string m_currentGainMode;

};
}

#endif // CHIPGAINCONFIGURATOR_H
