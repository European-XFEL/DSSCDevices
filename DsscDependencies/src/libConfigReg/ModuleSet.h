#ifndef MODULESET_H
#define MODULESET_H

#include <fstream>
#include <vector>
#include <stdint.h>

#include <string>

#include "ConfigSignals.h"
#include "utils.h"
namespace SuS {

  class ModuleSet {

    friend class ConfigReg;

    public:

      ModuleSet();
      ModuleSet(const std::string & setName);
      ModuleSet(std::ifstream & in);

      inline void rename(const std::string & newName){name = newName;}

      inline const std::vector<std::string> & getSignalNames(){ return cSignals.signalNames; }
      inline bool signalNameExists(const std::string & signalName){
        return cSignals.signalNameExists(signalName);
      }

      const std::vector<std::string> & getOutputs(){ return cSignals.getOutputs(); }
      std::string getModuleAtOutputNumber(uint32_t number);
      void setOutputs(const std::string & outputList);
      void setOutputs(const std::vector<std::string> & outputs);
      inline void addOutputs(const std::string & outputList){cSignals.addOutputs(outputList);}


      std::string removeModules(const std::string & modulesList);
      std::string addModules(const std::string & modulesList);
      inline const std::vector<std::string> & getModules(){ return cSignals.modules; }

      uint32_t getIntegerValue(const std::string & module);
      void setSignalValue(std::string modulesStr, const std::string & signalName, uint32_t value);
      uint32_t getSignalValue(std::string modulesStr, const std::string & signalName);
      std::vector<uint32_t> getSignalValues(const std::string & moduleStr, const std::string & signalName);

      std::vector<bool> printContent();
      std::vector<bool> printModuleContent(const std::string & module){
        return cSignals.printModuleContent(module,setIsReverse);
      }

      std::string printContentToString(bool verbose);
      std::string printModuleContentToString(const std::string & moduleList, bool verbose);

      inline void setBitValue(std::string module, uint position, bool value){
        cSignals.setBitValue(cSignals.modToInt(module),position,value);
      }

      inline void setBitValue(int mod, uint position, bool value){
        cSignals.setBitValue(mod,position,value);
      }

      inline uint32_t getMaxSignalValue(const std::string & signalName){
        //uint64_t required for Karabo
        return (((uint64_t)1)<<cSignals.getNumSignalDigits(signalName) ) - 1;
      }

      inline uint32_t getAccessLevel(const std::string & signalName){
        return cSignals.getAccessLevel(signalName);
      }

      bool compareContent(const std::vector<bool> &data_vec, bool overwrite);
      bool compareContent(const std::vector<bool> &data_vec, int asic, bool overwrite);
      bool compareContent(const std::vector<bool> &data_vec, const std::string & module, bool overwrite);
      bool compareContent(uint32_t data, bool overwrite);
      bool compareContent(uint32_t data, const std::string & module, bool overwrite);

      void saveToStream(std::ofstream & out);

      bool moduleSetIsVarious(std::string modulesStr);
      bool signalIsVarious(const std::string & signalName, std::string modulesStr);
      inline bool isSignalReadOnly(const std::string & signalName){return cSignals.isSignalReadOnly(signalName);}
      inline bool isSignalActiveLow(const std::string & signalName){return cSignals.isSignalActiveLow(signalName);}

      inline std::vector<uint8_t> getReadOnlyMask(){
        return cSignals.getReadOnlyMask(setIsReverse);
      }

      inline void changeSetReverse(bool reverse){setIsReverse = reverse;}

      inline void addSignal(const std::string & signalName, const std::string & positionList, bool readOnly, bool actLow = false)
      {
          if(positionList.compare(0,4,"init")==0){
            cSignals.addSignal(signalName,readOnly,actLow);
          }else{
            cSignals.addSignal(signalName, utils::positionListToVector<uint32_t>(positionList),readOnly,actLow);
          }
          if(cSignals.numBitsPerModule < numBitReminder){
            cSignals.addToNc(utils::getCountingVector<uint32_t>(cSignals.numBitsPerModule,numBitReminder-1));
          }
          numBitsPerModule = cSignals.numBitsPerModule;
          numSignals = cSignals.numSignals;
      }

      inline void removeSignal(int signalIndex){
          cSignals.removeSignal(signalIndex);
          numBitsPerModule = cSignals.numBitsPerModule;
          numSignals = cSignals.numSignals;
      }

      inline void renameSignal(int signalIndex, const std::string & signalName){
          cSignals.renameSignal(signalIndex,signalName);
      }

      inline void changeSignalReadOnly(int signalIndex, bool readOnly){
          cSignals.changeSignalReadOnly(signalIndex,readOnly);
      }

       inline void changeSignalActiveLow(int signalIndex, bool readOnly){
          cSignals.changeSignalActiveLow(signalIndex,readOnly);
      }

      inline void setSignalPositions(int signalIndex, const std::string & positionList){
          cSignals.setSignalPositions(signalIndex,utils::positionListToVector<uint32_t>(positionList));
          numBitsPerModule = cSignals.numBitsPerModule;
      }

      inline void setSignalAliases(int signalIndex, const std::string & aliases){
          cSignals.setSignalAliases(signalIndex,aliases);
      }

      inline std::string getSignalPositionsList(const std::string & signalName) {
          return cSignals.getSignalPositionsList(signalName);
      }

      inline std::string getSignalAliases(const std::string & signalName) {
          return cSignals.getSignalAliases(signalName);
      }

      inline bool isSignalBoolean(int signalIndex) const {
          return cSignals.isSignalBoolean(signalIndex);
      }

      bool isBitReadOnly(int bit);
      bool isModuleSetReadOnly();
      const std::string & getBitName(int pos) const;
      int getBitNumber(int pos) const;


      inline void setRegAddress(uint32_t newAddress){ address = newAddress; }
      inline uint32_t getRegAddress() const { return address; }


      void removeNCSignal();

    private:
      void readSignalsAndOutputs(std::ifstream & in);
      void readModuleValues(std::ifstream & in);
      std::string findNextNewBit() const ;

    protected:

      std::string name;
      std::string modulesString;
      int numBitsPerModule;
      int numBitReminder;
      int numSignals;
      int numModules;
      bool setIsReverse;
      uint32_t address;

      bool changed;

      std::vector<std::string> compareErrors;

    private:

      ConfigSignals cSignals;
  };


}




#endif
