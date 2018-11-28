#ifndef CONFIGREG_H
#define CONFIGREG_H

#include <fstream>
#include <vector>
#include <map>
#include <string>

#include "utils.h"
#include "ModuleSet.h"

namespace SuS {

  class ConfigReg {

    public:

      ConfigReg();
      ConfigReg(std::string _filename);

      virtual ~ConfigReg();

      inline std::string getFilename() const {return filename;}

      inline bool isLoaded(){
        if(!loaded) std::cout << "Config Register file not loaded" << std::endl;
        return loaded;
      }

      bool initFromFile(const std::string &_filename);
      void initModuleSets(std::ifstream & ifs);

      void save();
      void saveToFile(const std::string &_filename);

      inline void init(){ moduleSetsMap.clear(); loaded=false;}
      inline int getNumModuleSets() const {return moduleSetsMap.size();}
      inline int getNumBitsPerModule(const std::string & moduleSet){
        return moduleSetsMap.find(moduleSet)->second.numBitsPerModule;
      }

      inline uint32_t getMaxSignalValue(const std::string & moduleSet, const std::string & signalName){
        return moduleSetsMap.find(moduleSet)->second.getMaxSignalValue(signalName);
      }


      inline void addSignal(const std::string & moduleSetName, const std::string & signalName, const std::string & positionList, bool readOnly = false, bool actLow = false){
          auto & moduleSet = moduleSetsMap.find(moduleSetName)->second;
          moduleSet.removeNCSignal();
          moduleSet.addSignal(signalName,positionList,readOnly,actLow);
          loaded = true;
      }

      inline void removeSignal(const std::string & moduleSet, int signalIndex){
          moduleSetsMap.find(moduleSet)->second.removeSignal(signalIndex);
      }

      inline void renameSignal(const std::string & moduleSet, int signalIndex, const std::string & signalName){
        moduleSetsMap.find(moduleSet)->second.renameSignal(signalIndex,signalName);
      }

      inline bool signalNameExists(const std::string & moduleSet, const std::string & signalName)
      {
        return moduleSetsMap.find(moduleSet)->second.signalNameExists(signalName);
      }

      inline bool moduleSetExists(const std::string & moduleSet)
      {
        return ( moduleSetsMap.find(moduleSet) != moduleSetsMap.end() );
      }

      inline void changeSignalReadOnly(const std::string & moduleSet, int signalIndex, bool readOnly = false){
        moduleSetsMap.find(moduleSet)->second.changeSignalReadOnly(signalIndex,readOnly);
      }

       inline void changeSignalActiveLow(const std::string & moduleSet, int signalIndex, bool readOnly = false){
        moduleSetsMap.find(moduleSet)->second.changeSignalActiveLow(signalIndex,readOnly);
      }

      inline void setSignalPositions(const std::string & moduleSet, int signalIndex, const std::string & positionList){
        moduleSetsMap.find(moduleSet)->second.setSignalPositions(signalIndex,positionList);
      }

      inline void setSignalAliases(const std::string & moduleSet, int signalIndex, const std::string & aliases){
        moduleSetsMap.find(moduleSet)->second.setSignalAliases(signalIndex,aliases);
      }

      inline std::string getSignalPositionsList(const std::string & moduleSet, const std::string & signalName){
        return moduleSetsMap.find(moduleSet)->second.getSignalPositionsList(signalName);
      }

      inline std::string getSignalAliases(const std::string & moduleSet, const std::string & signalName){
        return moduleSetsMap.find(moduleSet)->second.getSignalAliases(signalName);
      }

      inline uint32_t getAccessLevel(const std::string & moduleSet, const std::string & signalName){
        return moduleSetsMap.find(moduleSet)->second.getAccessLevel(signalName);
      }

      inline bool isSignalBoolean(const std::string & moduleSet, int signalIndex){
        return moduleSetsMap.find(moduleSet)->second.isSignalBoolean(signalIndex);
      }

      inline bool isModuleSetReadOnly(const std::string & moduleSet){
        return moduleSetsMap.find(moduleSet)->second.isModuleSetReadOnly();
      }


      void addModuleSet(const std::string & moduleSetName, const std::string & modList, uint32_t address);
      void addModuleSet(const std::string & moduleSetName);
      void removeModuleSet(const std::string & moduleSetName);
      inline void renameModuleSet(const std::string & moduleSet,const std::string & newName){
        const auto mapIt = moduleSetsMap.find(moduleSet);
        mapIt->second.rename(newName);
        moduleSetsMap[newName] = std::move(mapIt->second);
        moduleSetsMap.erase(mapIt);

      }

      inline std::string removeModules(const std::string & moduleSet, const std::string & modulesList){
        return moduleSetsMap.find(moduleSet)->second.removeModules(modulesList);
      }

      inline std::string addModules(const std::string & moduleSet, const std::string & modulesList){
        auto & set = moduleSetsMap.find(moduleSet)->second;
        auto list = set.addModules(modulesList);
        set.addOutputs(modulesList);
        return list;
      }

      inline int getNumModules(const std::string & moduleSet){ return moduleSetsMap.find(moduleSet)->second.numModules;}
      inline int getNumSignals(const std::string & moduleSet){ return moduleSetsMap.find(moduleSet)->second.numSignals;}
      inline bool signalIsVarious(const std::string & moduleSet, const std::string & signalName, const std::string & moduleStr){
        return moduleSetsMap.find(moduleSet)->second.signalIsVarious(signalName,moduleStr);
      }
      inline bool moduleSetIsVarious(const std::string & moduleSet, const std::string & moduleStr){
        return moduleSetsMap.find(moduleSet)->second.moduleSetIsVarious(moduleStr);
      }

      inline void setSignalValue(const std::string & moduleSet, const std::string & moduleStr, const std::string & signalName, uint32_t value){
        auto setIt = moduleSetsMap.find(moduleSet);
         if(setIt == moduleSetsMap.end()){
           std::cout << "++++ ERROR: ConfigReg ModuleSet unknown " << moduleSet << std::endl;
         }else{
           setIt->second.setSignalValue(moduleStr,signalName,value);
         }
      }

      inline uint32_t getSignalValue(const std::string & moduleSet, const std::string & moduleStr, const std::string & signalName){
        return moduleSetsMap.find(moduleSet)->second.getSignalValue(moduleStr,signalName);
      }

      inline std::vector<uint32_t> getSignalValues(const std::string & moduleSet, const std::string & moduleStr, const std::string & signalName){
        return moduleSetsMap.find(moduleSet)->second.getSignalValues(moduleStr,signalName);
      }

      inline void setRegAddress(const std::string & moduleSet, uint32_t address){moduleSetsMap.find(moduleSet)->second.setRegAddress(address); }
      inline uint32_t getRegAddress(const std::string & moduleSet){ return moduleSetsMap.find(moduleSet)->second.getRegAddress(); }

      inline bool isSetReverse(const std::string & moduleSet) { return moduleSetsMap.find(moduleSet)->second.setIsReverse;}
      inline void changeSetReverse(const std::string & moduleSet, bool reverse) {moduleSetsMap.find(moduleSet)->second.setIsReverse = reverse;}

      inline uint32_t isSignalReadOnly(const std::string & moduleSet, const std::string & signalName){
        return moduleSetsMap.find(moduleSet)->second.isSignalReadOnly(signalName);
      }

      inline uint32_t isSignalActiveLow(const std::string & moduleSet, const std::string & signalName){
        return moduleSetsMap.find(moduleSet)->second.isSignalActiveLow(signalName);
      }

      inline std::vector<uint8_t> getReadOnlyMask(const std::string & moduleSet){
        return moduleSetsMap.find(moduleSet)->second.getReadOnlyMask();
      }

      std::vector<bool> printContent(int moduleSet);

      inline std::vector<bool> printContent(const std::string & moduleSet){
        return moduleSetsMap.find(moduleSet)->second.printContent();
      }

      inline std::vector<bool> printContent(const std::string & moduleSet, const std::string & module){
        return moduleSetsMap.find(moduleSet)->second.printModuleContent(module);
      }

      inline std::string printContentToString(const std::string & moduleSet){
        return moduleSetsMap.find(moduleSet)->second.printContentToString(false);
      }

      inline std::string printModuleContentToString(const std::string & moduleSet, const std::string & moduleList){
        return moduleSetsMap.find(moduleSet)->second.printModuleContentToString(moduleList,false);
      }

      inline void setBitValue(const std::string & moduleSet, const std::string & module, uint position, bool value){
        return moduleSetsMap.find(moduleSet)->second.setBitValue(module,position,value);
      }

      inline bool compareContent(const std::vector<bool> &data_vec, const std::string & moduleSet, bool overwrite = false){
        if(!loaded) return false;
        return moduleSetsMap.find(moduleSet)->second.compareContent(data_vec,overwrite);
      }

      inline bool compareContent(const std::vector<bool> &data_vec, const std::string & moduleSet, int asic, bool overwrite = false){
        if(!loaded) return false;
        return moduleSetsMap.find(moduleSet)->second.compareContent(data_vec,asic,overwrite);
      }

      inline bool compareContent(const std::vector<bool> &data_vec, const std::string & moduleSet, const std::string & module, bool overwrite = false){
        return moduleSetsMap.find(moduleSet)->second.compareContent(data_vec,module,overwrite);
      }

      inline bool compareContent(uint32_t data, const std::string & moduleSet, bool overwrite = false){
        if (!loaded) return false;
        return moduleSetsMap.find(moduleSet)->second.compareContent(data,overwrite);
      }

      inline bool compareContent(uint32_t data, const std::string & moduleSet, const std::string & module, bool overwrite = false){
        if (!loaded) return false;
        return moduleSetsMap.find(moduleSet)->second.compareContent(data,module,overwrite);
      }

      inline uint32_t getModuleSetValue(const std::string & moduleSet, const std::string & module){
        return moduleSetsMap.find(moduleSet)->second.getIntegerValue(module);
      }


      std::vector<std::string> getModuleSetNames() const;
      inline std::vector<uint32_t> getModuleNumbers(const std::string & moduleSet){
        return utils::positionListToVector(moduleSetsMap.find(moduleSet)->second.modulesString);
      }

      inline const std::vector<std::string> & getSignalNames(const std::string & moduleSet){ return moduleSetsMap.find(moduleSet)->second.getSignalNames();}
      inline const std::string getOutputList(const std::string & moduleSet){ return utils::positionStringVectorToList(moduleSetsMap.find(moduleSet)->second.getOutputs());}
      inline const std::vector<std::string> & getOutputs(const std::string & moduleSet){ return moduleSetsMap.find(moduleSet)->second.getOutputs();}
      inline const std::string getModuleNumberList(const std::string & moduleSet){ return utils::positionStringVectorToList(moduleSetsMap.find(moduleSet)->second.getModules());}
      inline std::string getModuleAtOutputNumber(const std::string & moduleSet, uint32_t number){ return moduleSetsMap.find(moduleSet)->second.getModuleAtOutputNumber(number);}

      inline void setOutputList(const std::string & moduleSet, const std::string & outputList){ moduleSetsMap.find(moduleSet)->second.setOutputs(outputList);}
      inline void setOutputs(const std::string & moduleSet, const std::vector<std::string> & outputs){ moduleSetsMap.find(moduleSet)->second.setOutputs(outputs);}

      inline const std::vector<std::string> & getModules(const std::string & moduleSet){ return moduleSetsMap.find(moduleSet)->second.getModules();}

      inline std::string getFileName() const {return filename;}

      const std::vector<std::string> & getCompareErrors();
      inline void clearCompareErrors() { compareErrors.clear(); }
      inline const std::string & getBitName(const std::string & moduleSet,int pos) const {
        return moduleSetsMap.find(moduleSet)->second.getBitName(pos);
      }

    private:
      std::string filename;
      bool loaded;

      std::vector<std::string> compareErrors;
      std::map<std::string,ModuleSet> moduleSetsMap;
  };


}




#endif
