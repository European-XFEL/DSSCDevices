/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DsscConfigHashWriter.cpp
 * Author: samartsev
 * 
 * Created on February 7, 2019, 9:25 AM
 *  
 * 
 */

#include "DsscConfigHashWriter.hh"



namespace karabo {
    
    
#define RUNPATH "RUN.DSSC.configuration." 
    
void addConfiguration(Hash& _resHash, const DsscHDF5ConfigData& _h5config);
void addConfiguration(Hash&, const DsscHDF5RegisterConfigVec&);
void addConfiguration(Hash&, const DsscHDF5RegisterConfig & registerConfig);
void addConfiguration(Hash&, std::string, const DsscHDF5SequenceData&);


void getFullConfigHash(const std::string& _fileName, Hash& _hash) {
    
    const auto h5config = SuS::DSSC_PPT_API::getHDF5ConfigData(_fileName);

    addConfiguration(_hash, h5config);    
}


void addMapData(Hash& _hash, const std::string & node, const std::map<std::string,uint32_t> & mapData)
{
  for(const auto & item: mapData){
    _hash.set<uint32_t>(node + item.first, item.second);
  }
}


void addConfiguration(Hash& _hash, const DsscHDF5ConfigData & configData)
{

  uint32_t numRegisters = configData.getNumRegisters();
  
  std::string baseNodeMain(RUNPATH);

  _hash.set<uint32_t>( baseNodeMain + "NumRegisters", numRegisters);
  _hash.set<std::string>(baseNodeMain + "RegisterNames", configData.getRegisterNames());
  _hash.set<std::string>(baseNodeMain + "timestamp", configData.timestamp);

  addConfiguration(_hash, configData.pixelRegisterDataVec);
  addConfiguration(_hash, configData.jtagRegisterDataVec);
  addConfiguration(_hash, configData.iobRegisterData);
  addConfiguration(_hash, configData.epcRegisterData);

  addConfiguration(_hash, "Sequencer", configData.sequencerData);
  addConfiguration(_hash, "ControlSequence", configData.controlSequenceData);
}


void addConfiguration(Hash& _hash, const DsscHDF5RegisterConfigVec & registerConfigVec)
{
  for(auto && registerConfig : registerConfigVec){
    addConfiguration(_hash, registerConfig);
  }
}


void addConfiguration(Hash& _hash, const DsscHDF5RegisterConfig & registerConfig)
{
  if(registerConfig.numModuleSets == 0) return;
  
  std::string baseNodeMain(RUNPATH);

  const std::string baseNode = baseNodeMain + registerConfig.registerName;

  _hash.set<uint32_t>(baseNode + ".NumModuleSets", registerConfig.numModuleSets);

  std::string moduleSetsStr;

  for(const auto & modSetStr : registerConfig.moduleSets){
    moduleSetsStr += modSetStr + ";";
  }
  _hash.set<std::string>(baseNode + ".ModuleSets", moduleSetsStr);


  int modSet = 0;
  for(const auto & modSetName : registerConfig.moduleSets){
    const std::string setDirName = baseNode + "."+ modSetName + ".";
    
    hsize_t datasize = registerConfig.numberOfModules[modSet];
    
    karabo::util::NDArray modulesvec((uint32_t*)registerConfig.modules[modSet].data(), datasize);
    karabo::util::NDArray outputsvec((uint32_t*)registerConfig.outputs[modSet].data(), datasize);
    
    _hash.set<karabo::util::NDArray>(setDirName + "Modules", modulesvec);
    _hash.set<karabo::util::NDArray>(setDirName + "Outputs", outputsvec);

    _hash.set<uint32_t>(setDirName + "NumBitsPerModule", registerConfig.numBitsPerModule[modSet]);
    _hash.set<uint32_t>(setDirName + "Address", registerConfig.addresses[modSet]);
    _hash.set<uint32_t>(setDirName + "SetIsReverse", registerConfig.setIsReverse[modSet]);
    _hash.set<uint32_t>(setDirName + "NumSignals", registerConfig.numSignals[modSet]);
    _hash.set<uint32_t>(setDirName + "NumModules", registerConfig.numberOfModules[modSet]);


    int sig = 0;
    for(const auto & signalName : registerConfig.signalNames[modSet]){
      const std::string sigDirName = setDirName + "." + signalName + ".";
      _hash.set<std::string>(sigDirName + "BitPositions",registerConfig.bitPositions[modSet][sig]);

      _hash.set<uint32_t>(sigDirName + "ReadOnly", registerConfig.readOnly[modSet][sig]);
      _hash.set<uint32_t>(sigDirName + "ActiveLow", registerConfig.activeLow[modSet][sig]);
      _hash.set<uint32_t>(sigDirName + "AccessLevel", registerConfig.accessLevels[modSet][sig]);
     
      karabo::util::NDArray regdatavec((uint32_t*)registerConfig.registerData[modSet][sig].data(), datasize);
      _hash.set<karabo::util::NDArray>(sigDirName + "ConfigValues", regdatavec);

      sig++;
    }
    modSet++;
  }
}
 
 
void addConfiguration(Hash & _hash, std::string _node,  const DsscHDF5SequenceData & _sequenceData)
{
  if(_sequenceData.empty()) return;
  
   std::string baseNodeMain(RUNPATH);

  const std::string basenode = (_node.back() == '.') ? baseNodeMain + std::string(_node) : baseNodeMain + std::string(_node) + ".";

  for(const auto & mapItem : _sequenceData){
    _hash.set<uint32_t>( basenode + mapItem.first, mapItem.second);
  } 
}


}//karabo namespace