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

auto NOSPACES = [](std::string& str)->std::string& {std::replace(str.begin(), str.end(), ' ', '_'); return str;};

namespace karabo {
    
        //DsscH5ConfigToSchema::DsscH5ConfigToSchema() {
        //}
        

        bool DsscH5ConfigToSchema::getFullConfigHash(const std::string& filename, Hash& hash) {

            auto h5config = SuS::DSSC_PPT_API::getHDF5ConfigData(filename);
            addConfiguration(hash, h5config);

            if (!karabo::util::similar(hash, m_lastHash)) { // check on similarity of structure, not content
                m_lastHash = hash;
                return true;
            }

            return false;
        }


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
                                .assignmentOptional().noDefaultValue()
                                .commit();
                        break;

                    case Types::STRING:
                        STRING_ELEMENT(expected).key(path + it->getKey())
                                .daqPolicy(DAQPolicy::OMIT)
                                .init()
                                .assignmentOptional().noDefaultValue()
                                .commit();
                        break;
                    case Types::VECTOR_UINT32:
                        VECTOR_UINT32_ELEMENT(expected).key(path + it->getKey())
                                .daqPolicy(DAQPolicy::OMIT)
                                .init()
                                .assignmentOptional().noDefaultValue()
                                .commit();
                        break;

                    default:
                        std::clog << "Data type " << toString(it->getType()) << " not supported!";

                }
            }
        }


        karabo::util::Schema DsscH5ConfigToSchema::getUpdatedSchema() {
            Schema expected;
            HashToSchema(m_lastHash, expected, "");
            return expected;
        }


        void DsscH5ConfigToSchema::addMapData(Hash& hash, const std::string & node, const std::map<std::string, uint32_t> & mapData) {
            for (const auto & item : mapData) {
                hash.set<uint32_t>(node + item.first, item.second);
            }
        }


        void DsscH5ConfigToSchema::addConfiguration(Hash& hash, DsscHDF5ConfigData & configData) {

            uint32_t numRegisters = configData.getNumRegisters();

            const std::string baseNodeMain(s_dsscConfBaseNode + ".");

            hash.set<uint32_t>(baseNodeMain + "NumRegisters", numRegisters);
            hash.set<std::string>(baseNodeMain + "RegisterNames", configData.getRegisterNames());
            hash.set<std::string>(baseNodeMain + "timestamp", configData.timestamp);

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
            if (registerConfig.numModuleSets == 0) return;

            const std::string baseNodeMain(s_dsscConfBaseNode + ".");

            const std::string baseNode = baseNodeMain + NOSPACES(registerConfig.registerName);

            hash.set<uint32_t>(baseNode + ".NumModuleSets", registerConfig.numModuleSets);

            std::string moduleSetsStr;

            for (const auto & modSetStr : registerConfig.moduleSets) {
                moduleSetsStr += modSetStr + ";";
            }
            hash.set<std::string>(baseNode + ".ModuleSets", moduleSetsStr);

            int modSet = 0;
            for (auto & modSetName : registerConfig.moduleSets) {
                const std::string setDirName = baseNode + "." + NOSPACES(modSetName) + ".";

                hsize_t datasize = registerConfig.numberOfModules[modSet];

                hash.set(setDirName + "Modules", registerConfig.modules[modSet]);
                hash.set(setDirName + "Outputs", registerConfig.outputs[modSet]);

                hash.set<uint32_t>(setDirName + "NumBitsPerModule", registerConfig.numBitsPerModule[modSet]);
                hash.set<uint32_t>(setDirName + "Address", registerConfig.addresses[modSet]);
                hash.set<uint32_t>(setDirName + "SetIsReverse", registerConfig.setIsReverse[modSet]);
                hash.set<uint32_t>(setDirName + "NumSignals", registerConfig.numSignals[modSet]);
                hash.set<uint32_t>(setDirName + "NumModules", registerConfig.numberOfModules[modSet]);


                int sig = 0;
                for (auto & signalName : registerConfig.signalNames[modSet]) {
                    const std::string sigDirName = setDirName + NOSPACES(signalName) + ".";
                    hash.set<std::string>(sigDirName + "BitPositions", registerConfig.bitPositions[modSet][sig]);

                    hash.set<uint32_t>(sigDirName + "ReadOnly", registerConfig.readOnly[modSet][sig]);
                    hash.set<uint32_t>(sigDirName + "ActiveLow", registerConfig.activeLow[modSet][sig]);
                    hash.set<uint32_t>(sigDirName + "AccessLevel", registerConfig.accessLevels[modSet][sig]);

                    hash.set(sigDirName + "ConfigValues", registerConfig.registerData[modSet][sig]);

                    sig++;
                }
                modSet++;
            }
        }


        void DsscH5ConfigToSchema::addConfiguration(Hash & hash, const std::string& node, const  DsscHDF5SequenceData & sequenceData) {
            if (sequenceData.empty()) return;

            const std::string baseNodeMain(s_dsscConfBaseNode + ".");
            const std::string basenode = (node.back() == '.') ? baseNodeMain + node : baseNodeMain + node + ".";
            
            for (const auto & mapItem : sequenceData) {
                std::string path = mapItem.first;
                hash.set<uint32_t>(basenode + NOSPACES(path), mapItem.second);
            }
        }

 //*/
}//karabo namespace
