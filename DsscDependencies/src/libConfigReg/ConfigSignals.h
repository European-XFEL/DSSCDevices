#ifndef CONFIGSIGNALS_H
#define CONFIGSIGNALS_H

#include <vector>
#include <unordered_map>
#include <string>
#include <fstream>

namespace SuS{

class ConfigSignals{

  friend class ModuleSet;

  public:
    ConfigSignals();
    void reserve(const int _numSignals, const int _numModules);
    void saveToStream(std::ofstream & out) const;

    void setSignalValue(const std::vector<std::string> & modulesVec , const std::string & signalName, uint32_t value);

    void addOutputs(const std::string & newOutputsList);
    const std::vector<std::string> & getOutputs();

    inline int getNumSignalDigits(const std::string & signalName) {return bitPositions[sigToInt(signalName)].size();}

    std::string getSignalPositionsList(const std::string & signalName);
    std::string getSignalAliases(const std::string & signalName);


  protected:
    void addSignal(const std::string & signalName, const std::vector<uint32_t> & positions, bool ro, bool actLow = false);
    void addSignal(const std::string & signalName, bool ro, bool actLow);

    void renameSignal(unsigned signalIndex, const std::string & signalName);

    inline bool signalNameExists(const std::string & signalName){
      return (sigToIntS(signalName) >= 0);
    }

    inline void changeSignalReadOnly(int signalIndex, bool ro){
        checkRow(signalIndex);
        readOnly[signalIndex] = ro;
    }

     inline void changeSignalActiveLow(int signalIndex, bool al){
        checkRow(signalIndex);
        activeLow[signalIndex] = al;
    }   

    inline void setSignalPositions(int signalIndex, const std::vector<uint32_t> & positions){
        checkRow(signalIndex);
        bitPositions[signalIndex] = positions;
        checkBitPositions();
    }

    void setSignalAliases(int signalIndex, const std::string & _aliases);
    void setSignalAliases(int signalIndex, const std::vector<std::string> & _aliases);

    inline bool isSignalBoolean(int signalIndex) const {
        checkRow(signalIndex);
        return (bitPositions[signalIndex].size() == 1);
    }

    inline bool isSignalReadOnly(int signalIndex) const {
        checkRow(signalIndex);
        return (readOnly[signalIndex]!=0);
    }

    inline bool isSignalReadOnly(const std::string & signalName){
      return readOnly[sigToInt(signalName)];
    }

    inline bool isSignalActiveLow(int signalIndex) const {
        checkRow(signalIndex);
        return (activeLow[signalIndex]!=0);
    }

    inline bool isSignalActiveLow(const std::string & signalName){
      return activeLow[sigToInt(signalName)];
    }    

    std::vector<uint8_t> getReadOnlyMask(bool reverse);


    void removeSignal(int signalIndex, bool check = true);
    void removeSignalFromMap(int signalIndex);

    bool signalIsVarious(const std::string & signalName, const std::vector<std::string> &modulesVec);
    std::vector<bool> printModuleContent(const std::string & module, bool verbose);
    void setBitValue(uint32_t mod, uint32_t position, bool value);

    inline uint32_t getAccessLevel(const std::string & signal){
      int sig = sigToIntS(signal); if(sig < 0) return 0;
      return accessLevels[sig];
    }

    inline uint32_t getSignalValue(const std::string & module, const std::string & signal){
      int sig = sigToIntS(signal); if(sig < 0) return 0;
      return getSignalValue(modToIntS(module),sig);
    }

    int getSignalNumberFromPosition(int position);

    bool checkSignals();

    void updateSignalIsVarious();
    void initSignalValuesDefault();

    void initBitNames(int _numBitsPerModule);

    std::unordered_map<std::string,int> modulesMap;
    std::unordered_map<std::string,int> signalsMap;

    std::vector<bool> isVarious;
    std::vector<uint32_t> readOnly;
    std::vector<uint32_t> activeLow;
    std::vector<uint32_t> accessLevels;

    std::vector<std::string> signalNames;
    std::vector<std::vector <std::string>> aliases;  // signal can have many aliases
    std::vector<std::string> outputs;
    std::vector<std::string> modules;
    std::vector< std::vector<uint32_t> > modSignalValues;
    std::vector< std::vector<uint32_t> > bitPositions;
    std::vector<std::string> bitNames; // bitNames[moduleSet][bitNr]

    int numModules;
    int numSignals;
    int numBitsPerModule;

    int modToIntS(const std::string & module);
    int sigToIntS(const std::string & signalName);

    inline int modToInt(const std::string & module) { return modulesMap[module]; }
    inline int sigToInt(const std::string & signalName) { return signalsMap[signalName]; }

  private:

    bool checkBitPositions();
    void checkModSignalValue();

    inline void checkRow(unsigned signalIndex) const {  if(signalIndex >= signalNames.size()) throw "Row out of signal range";}


    inline bool allModulesSame() const{
      for(auto v: isVarious) if(v) return false;
      return true;
    }


    inline void setSignalValue(int mod, int sig, uint32_t value){ modSignalValues[mod][sig] = value; }
    inline uint32_t getSignalValue(int mod, int sig) const { return modSignalValues[mod][sig]; }


    inline std::vector<int> toIdxVector(const std::vector<std::string> & modules){
      std::vector<int> idxVector;
      idxVector.reserve(modules.size());

      for(const auto & m : modules){
        idxVector.push_back(modToInt(m));
      }
      return idxVector;
    }

    void addToNc(const std::vector<uint32_t> & missingBits);

    void removeModules(const std::string & modulesList);
    void addModules(const std::string & modulesList);

};
}

#endif
