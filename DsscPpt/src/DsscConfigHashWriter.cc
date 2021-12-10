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

#include <algorithm>
#include "DsscConfigHashWriter.hh"


using namespace karabo::util;
using namespace karabo::log;
using namespace karabo::io;
using namespace karabo::net;
using namespace karabo::xms;
using namespace karabo::core;

namespace karabo {


    DsscH5ConfigToSchema::DsscH5ConfigToSchema() {
    };


    DsscH5ConfigToSchema::~DsscH5ConfigToSchema() {
    };

    void DsscH5ConfigToSchema::HashToSchema(const karabo::util::Hash& hash, karabo::util::Schema& expected, const std::string& path) {
        for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
            switch (it->getType()) {

                case Types::HASH:
                    NODE_ELEMENT(expected).key(path + it->getKey())
                            .commit();
                    HashToSchema(it->getValue<Hash>(), expected, path + it->getKey() + ".");
                    break;

                case Types::UINT32:
                    UINT32_ELEMENT(expected).key(path + it->getKey())
                            .daqPolicy(DAQPolicy::OMIT)
                            .init()
                            .assignmentOptional()
                            .noDefaultValue()
                            .commit();
                    break;

                case Types::STRING:
                    STRING_ELEMENT(expected).key(path + it->getKey())
                            .daqPolicy(DAQPolicy::OMIT)
                            .init()
                            .assignmentOptional()
                            .noDefaultValue()
                            .commit();
                    break;
                case Types::VECTOR_UINT32:
                    VECTOR_UINT32_ELEMENT(expected).key(path + it->getKey())
                            .daqPolicy(DAQPolicy::OMIT)
                            .init()
                            .assignmentOptional()
                            .noDefaultValue()
                            .commit();
            }
        }
    }


    void DsscH5ConfigToSchema::HashToSchemaDetConf(const karabo::util::Hash& hash, karabo::util::Schema& expected,\
            const std::string& path, bool _readonly) {
        
        bool ReadOnly = true;
        if (hash.has("ReadOnly")){            
            ReadOnly = hash.get<unsigned int>("ReadOnly");    
        }
        
        bool moduledata = false;
        if(path.length()>1){
            std::size_t dotpos = path.substr(0, path.length()-1).rfind('.');
            if(dotpos != std::string::npos){
                if(path.substr(dotpos+1, path.length()-2 - dotpos) == "ModulesData"){
                    moduledata = true;
                } 
            }
        }
      
        for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
            
            switch (it->getType()) {

                case Types::HASH:
                    
                    NODE_ELEMENT(expected).key(path + it->getKey())
                            .commit();                    
                    HashToSchemaDetConf(it->getValue<Hash>(), expected, path + it->getKey() + ".", ReadOnly);
                    break;

                case Types::UINT32:
                    if((!_readonly) && moduledata){                         
                        UINT32_ELEMENT(expected).key(path + it->getKey())
                                .daqPolicy(DAQPolicy::OMIT)
                                .init()
                                .assignmentOptional()
                                .noDefaultValue()
                                .reconfigurable()
                                .commit();
                    }else{                        
                        UINT32_ELEMENT(expected).key(path + it->getKey())
                                .daqPolicy(DAQPolicy::OMIT)
                                .init()
                                .assignmentOptional()
                                .noDefaultValue()                                
                                .commit();                            
                    }
                    break;

                case Types::STRING:
                    if((!_readonly) && moduledata){
                        STRING_ELEMENT(expected).key(path + it->getKey())
                                .daqPolicy(DAQPolicy::OMIT)
                                .init()
                                .assignmentOptional()
                                .noDefaultValue()
                                .reconfigurable()
                                .commit();
                    }else{
                        STRING_ELEMENT(expected).key(path + it->getKey())
                                .daqPolicy(DAQPolicy::OMIT)
                                .init()
                                .assignmentOptional()
                                .noDefaultValue()                                
                                .commit();                        
                    }
                    break;
                case Types::VECTOR_UINT32:
                    if((!_readonly) && moduledata){
                        VECTOR_UINT32_ELEMENT(expected).key(path + it->getKey())
                                .daqPolicy(DAQPolicy::OMIT)
                                .init()
                                .assignmentOptional()
                                .noDefaultValue()
                                .reconfigurable()
                                .commit();
                    }else{
                        VECTOR_UINT32_ELEMENT(expected).key(path + it->getKey())
                               .daqPolicy(DAQPolicy::OMIT)
                               .init()
                               .assignmentOptional()
                               .noDefaultValue()                               
                               .commit();                       
                    }

            }
        }//*/
    }


    void DsscH5ConfigToSchema::addMapData(Hash& hash, const std::string & node, const std::map<std::string, uint32_t> & mapData) {
        for (const auto & item : mapData) {
            hash.set<uint32_t>(node + item.first, item.second);
        }
    }
    
    
    std::vector<std::pair<std::string, unsigned int>> DsscH5ConfigToSchema::compareConfigHashData(karabo::util::Hash& hash_old, karabo::util::Hash& hash_new){
        paths_diffVals.clear();
        compareConfigHashData_rec(hash_old, hash_new, std::string());
        return paths_diffVals;
        //
    }
    
    void DsscH5ConfigToSchema::compareConfigHashData_rec(karabo::util::Hash& hash_old, karabo::util::Hash& hash_new,\
            std::string path){
        //
        
        for (Hash::const_iterator it = hash_new.begin(); it != hash_new.end(); ++it){

            switch (it->getType()) {                

                case Types::HASH:
                    compareConfigHashData_rec(hash_old.get<Hash>(it->getKey()), it->getValue<Hash>(), path + it->getKey() + ".");                    
                    break;
                    
                case Types::UINT32:                    
                    if(hash_old.get<unsigned int>(it->getKey()) != it->getValue<unsigned int>()){
                        paths_diffVals.emplace_back(path + it->getKey(), it->getValue<unsigned int>());
                    }
            }
            
        }
        
    }


    void DsscH5ConfigToSchema::addConfiguration(Hash& hash, DsscHDF5ConfigData & configData) {

        uint32_t numRegisters = configData.getNumRegisters();

        hash.set<uint32_t>("NumRegisters", numRegisters);
        hash.set<std::string>("RegisterNames", configData.getRegisterNames());
        hash.set<std::string>("timestamp", configData.timestamp);

        addConfiguration(hash, configData.pixelRegisterDataVec);
        addConfiguration(hash, configData.jtagRegisterDataVec);
        addConfiguration(hash, configData.iobRegisterData);
        addConfiguration(hash, configData.epcRegisterData);

        addConfiguration(hash, "Sequencer", configData.sequencerData);
        addConfiguration(hash, "ControlSequence", configData.controlSequenceData);
    }


    void DsscH5ConfigToSchema::addConfiguration(Hash& hash, DsscHDF5RegisterConfigVec & registerConfigVec) {
        for (auto && registerConfig : registerConfigVec) {
            addConfiguration(hash, registerConfig);
        }
    }


    void DsscH5ConfigToSchema::addConfiguration(Hash& hash, DsscHDF5RegisterConfig & registerConfig) {
        if (registerConfig.numModuleSets == 0) {
            return;
        }

        std::string baseNode = registerConfig.registerName;
        baseNode = removeSpaces(baseNode);
        
        hash.set<Hash>(baseNode, Hash());
        hash.setAttribute<std::string>(baseNode, "origKey", registerConfig.registerName, '.');

        hash.set<uint32_t>(baseNode + ".NumModuleSets", registerConfig.numModuleSets);        

        std::string moduleSetsStr;

        for (const auto & modSetStr : registerConfig.moduleSets) {
            moduleSetsStr += modSetStr + ";";
        }
        hash.set<std::string>(baseNode + ".ModuleSets", moduleSetsStr);

        int modSet = 0;
        for (auto & modSetName : registerConfig.moduleSets) {
            std::string setDirName = baseNode + "." + modSetName;
            setDirName = removeSpaces(setDirName);
            
            hash.set<Hash>(setDirName, Hash());
            hash.setAttribute<std::string>(setDirName, "origKey", modSetName, '.');
            setDirName += ".";

            hsize_t datasize = registerConfig.numberOfModules[modSet];

            hash.set(setDirName + "Modules", registerConfig.modules[modSet]);
            hash.set(setDirName + "Outputs", registerConfig.outputs[modSet]);

            hash.set<uint32_t>(setDirName + "NumBitsPerModule", registerConfig.numBitsPerModule[modSet]);
            hash.set<uint32_t>(setDirName + "Address", registerConfig.addresses[modSet]);
            hash.set<uint32_t>(setDirName + "SetIsReverse", registerConfig.setIsReverse[modSet]);
            hash.set<uint32_t>(setDirName + "NumSignals", registerConfig.numSignals[modSet]);
            //hash.set<string>(setDirName + "signalNames", registerConfig.signalNames[modSet]);
            hash.set<uint32_t>(setDirName + "NumModules", registerConfig.numberOfModules[modSet]);            

            int sig = 0;
            for (auto & signalName : registerConfig.signalNames[modSet]) {
                std::string sigDirName = setDirName + signalName;
                sigDirName = removeSpaces(sigDirName);
                
                hash.set<Hash>(sigDirName, Hash());
                hash.setAttribute<std::string>(sigDirName, "origKey", signalName, '.');
                sigDirName += ".";
                
                hash.set<std::string>(sigDirName + "BitPositions", registerConfig.bitPositions[modSet][sig]);

                hash.set<uint32_t>(sigDirName + "ReadOnly", registerConfig.readOnly[modSet][sig]);
                hash.set<uint32_t>(sigDirName + "ActiveLow", registerConfig.activeLow[modSet][sig]);
                hash.set<uint32_t>(sigDirName + "AccessLevel", registerConfig.accessLevels[modSet][sig]);

                hash.set<Hash>(sigDirName + "ModulesData", Hash());
                
                for(int i = 0 ; i < datasize; i++){
                    hash.set(sigDirName + "ModulesData." + std::to_string(registerConfig.modules[modSet][i]),\
                            registerConfig.registerData[modSet][sig][i] );
                }

                sig++;
            }
            modSet++;
        }
    }


    void DsscH5ConfigToSchema::addConfiguration(Hash & hash, const std::string& node, const DsscHDF5SequenceData & sequenceData) {
        if (sequenceData.empty()) return;

        const std::string basenode = (node.back() == '.') ? node : node + ".";

        for (const auto & mapItem : sequenceData) {
            std::string path = basenode + mapItem.first;
            path = removeSpaces(path);
            hash.set<uint32_t>(path, mapItem.second);
            hash.setAttribute<std::string>(path, "origKey", mapItem.first, '.');            
        }
    }
}//karabo namespace
