#ifndef CHIPFULLCONFIG_H
#define CHIPFULLCONFIG_H

#include <vector>
#include <string>
#include <cassert>

#include "Sequencer.h"
#include "ConfigReg.h"

namespace SuS
{

class CHIPFullConfig
{
  public:
    CHIPFullConfig();
    CHIPFullConfig(int numModules);
    CHIPFullConfig(const std::string & fullConfigFileName);

    virtual ~CHIPFullConfig();

    inline std::string getFullConfigFileName()  const { return configFileName;}
    inline std::string getSequencerFileName()  const { return sequencerFileName;}
    inline std::string getPixelRegsFileName()  const { return pixelFileNames.empty()? "" : pixelFileNames.front();}
    inline std::string getJtagRegsFileName()  const { return jtagFileNames.empty()? "" : jtagFileNames.front();}

    virtual void readFullConfigFile(const std::string & fileName);
    void loadFullConfig(const std::string & fileName);

    Sequencer * getSequencer(){return sequencer;}
    ConfigReg * getPixelReg(size_t idx) {assert(idx<pixelRegs.size()); return pixelRegs[idx];}
    ConfigReg * getJtagReg(size_t idx) {assert(idx<jtagRegs.size()); return jtagRegs[idx];}

    std::vector<ConfigReg*> getJtagRegVec() {
      std::vector<ConfigReg*> vec;
      for(size_t i=0; i<jtagRegs.size(); i++) vec.push_back(jtagRegs[i]);
      return vec;
    }

    std::vector<ConfigReg*> getPixelRegVec() {
      std::vector<ConfigReg*> vec;
      for(size_t i=0; i<pixelRegs.size(); i++) vec.push_back(pixelRegs[i]);
      return vec;
    }


    int numPixelRegs() const {return pixelRegs.size();}
    int numJtagRegs() const {return jtagRegs.size();}

    std::map<std::string,std::string> readFullConfigFileOld(const std::string & fileName);

    virtual void storeFullConfigFile();
    virtual void storeFullConfigFile(std::string fileName, bool keepName = false);

    enum RegType{UNKNOWN = 0, SEQ = 1, PXS = 2, JTG =3, EPC =4, IOB = 5};

    static RegType getType(const std::string & line);
    virtual void addFileName(RegType type, std::string line);

    bool isGood() const {
      if(!good) std::cout << "Full Config " << configFileName << " could not be loaded. Config is not Good!" << std::endl;
      return good;
    }

  private:

  protected:
    void sortFileNames();

    virtual void newSequencer();
    virtual void newPixelReg(size_t idx);
    virtual void newJtagReg(size_t idx);

    virtual void loadConfig();
    virtual void clear();
    virtual bool checkConfig();
    virtual void generateFullConfigFile();

    static const std::string SEQTAG;
    static const std::string PXSTAG;
    static const std::string JTGTAG;
    static const std::string EPCTAG;
    static const std::string IOBTAG;

    bool good;

    std::string configFileName;
    std::string lastFileName;

    std::string sequencerFileName;
    std::vector<std::string> jtagFileNames;
    std::vector<std::string> pixelFileNames;

    Sequencer *sequencer;
    std::vector<ConfigReg*> pixelRegs;
    std::vector<ConfigReg*> jtagRegs;
};

}


#endif // CHIPFULLCONFIG_H
