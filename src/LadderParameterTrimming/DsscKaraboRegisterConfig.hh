#ifndef DSSCKARABOREGISTERCONFIG_HPP
#define DSSCKARABOREGISTERCONFIG_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <utility>

namespace karabo {
  
  struct DsscKaraboRegisterConfig
  {
    DsscKaraboRegisterConfig() : numModuleSets(0),numberOfModules(0){}

    std::string registerName;
    unsigned int numModuleSets;

    std::vector<std::string> moduleSets;
    std::vector<unsigned int> addresses;
    std::vector<unsigned int> numBitsPerModule;

    std::vector<unsigned int> numberOfModules;
    std::vector<std::vector<unsigned int>> modules;
    std::vector<std::vector<unsigned int>> outputs;
    std::vector<unsigned int> setIsReverse;

    std::vector<unsigned int> numSignals;
    std::vector<std::vector<std::string>> signalNames;
    std::vector<std::vector<std::string>> bitPositions;
    std::vector<std::vector<unsigned int>> readOnly;
    std::vector<std::vector<unsigned int>> activeLow;
    std::vector<std::vector<unsigned int>> accessLevels;

    // image like data vector of config data numModuleSets x numSignals x modules
    std::vector<std::vector<std::vector<unsigned int>>> registerData;
  };

  using DsscKaraboRegisterConfigVec = std::vector<DsscKaraboRegisterConfig>;
  using DsscKaraboSequenceData = std::map<std::string,unsigned int>;

  struct DsscKaraboConfigData
  {
    std::string timestamp;

    DsscKaraboSequenceData sequencerData;
    DsscKaraboSequenceData controlSequenceData;

    DsscKaraboRegisterConfigVec pixelRegisterDataVec;
    DsscKaraboRegisterConfigVec jtagRegisterDataVec;
    DsscKaraboRegisterConfig epcRegisterData;
    DsscKaraboRegisterConfig iobRegisterData;

    unsigned int getSequencerValue(const std::string & paramName) const {
      if(sequencerData.find(paramName) != sequencerData.end()){
        return sequencerData.at(paramName);
      }
      return 0;
    }

    unsigned int getCtrlSeqValue(const std::string & paramName) const {
      if(controlSequenceData.find(paramName) != controlSequenceData.end()){
        return controlSequenceData.at(paramName);
      }
      return 0;
    }

    unsigned int getNumRegisters() const {
      unsigned int numRegisters = 0;
      if(!sequencerData.empty()) numRegisters++;
      if(!controlSequenceData.empty()) numRegisters++;
      for(auto && pixelRegisterData : pixelRegisterDataVec)
        if(pixelRegisterData.numModuleSets > 0) numRegisters++;
      for(auto && jtagRegisterData : jtagRegisterDataVec)
        if(jtagRegisterData.numModuleSets  > 0) numRegisters++;
      if(epcRegisterData.numModuleSets   > 0) numRegisters++;
      if(iobRegisterData.numModuleSets   > 0) numRegisters++;
      return numRegisters;
    }

    std::string getRegisterNames() const {
      std::string registerNames = "";
      if(!sequencerData.empty())       registerNames += "Sequencer;";
      if(!controlSequenceData.empty()) registerNames += "ControlSequence;";
      for(auto && pixelRegisterData : pixelRegisterDataVec)
        if(pixelRegisterData.numModuleSets > 0) registerNames += pixelRegisterData.registerName +";";
      for(auto && jtagRegisterData : jtagRegisterDataVec)
        if(jtagRegisterData.numModuleSets  > 0) registerNames += jtagRegisterData.registerName +";";
      if(epcRegisterData.numModuleSets   > 0) registerNames += epcRegisterData.registerName +";";
      if(iobRegisterData.numModuleSets   > 0) registerNames += iobRegisterData.registerName +";";
      return registerNames;
    }

  };

}

#endif //DSSCKARABOREGISTERCONFIG_HPP
