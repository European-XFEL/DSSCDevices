#ifndef DSSCHDF5REGISTERCONFIG_H
#define DSSCHDF5REGISTERCONFIG_H

#include <string>
#include <vector>
#include <map>

class DsscHDF5RegisterConfig
{
  public:
    DsscHDF5RegisterConfig() : numModuleSets(0),numberOfModules(0){}

    std::string registerName;
    uint32_t numModuleSets;

    std::vector<std::string> moduleSets;
    std::vector<uint32_t> addresses;
    std::vector<uint32_t> numBitsPerModule;

    std::vector<uint32_t> numberOfModules;
    std::vector<std::vector<uint32_t>> modules;
    std::vector<std::vector<uint32_t>> outputs;
    std::vector<uint32_t> setIsReverse;

    std::vector<uint32_t> numSignals;
    std::vector<std::vector<std::string>> signalNames;
    std::vector<std::vector<std::string>> bitPositions;
    std::vector<std::vector<uint32_t>> readOnly;
    std::vector<std::vector<uint32_t>> activeLow;
    std::vector<std::vector<uint32_t>> accessLevels;

    // image like data vector of config data numModuleSets x numSignals x modules
    std::vector<std::vector<std::vector<uint32_t>>> registerData;
};

using DsscHDF5RegisterConfigVec = std::vector<DsscHDF5RegisterConfig>;
using DsscHDF5SequenceData = std::map<std::string,uint32_t>;

class DsscHDF5ConfigData
{
  public:

    std::string timestamp;

    DsscHDF5SequenceData sequencerData;
    DsscHDF5SequenceData controlSequenceData;

    DsscHDF5RegisterConfigVec pixelRegisterDataVec;
    DsscHDF5RegisterConfigVec jtagRegisterDataVec;
    DsscHDF5RegisterConfig epcRegisterData;
    DsscHDF5RegisterConfig iobRegisterData;

  uint32_t getSequencerValue(const std::string & paramName) const {
    if(sequencerData.find(paramName) != sequencerData.end()){
      return sequencerData.at(paramName);
    }
    return 0;
  }

  uint32_t getCtrlSeqValue(const std::string & paramName) const {
    if(controlSequenceData.find(paramName) != controlSequenceData.end()){
      return controlSequenceData.at(paramName);
    }
    return 0;
  }

  uint32_t getNumRegisters() const {
    uint32_t numRegisters = 0;
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


#endif // DSSCHDF5REGISTERCONFIG_H
