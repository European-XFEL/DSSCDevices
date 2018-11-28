#ifndef PPTFULLCONFIGFILE_H
#define PPTFULLCONFIGFILE_H

#include "CHIPFullConfig.h"

namespace SuS{
  class PPTFullConfig : public CHIPFullConfig
  {
    public:
      PPTFullConfig(const std::string & fullConfigFileName);
      PPTFullConfig();

      virtual ~PPTFullConfig();

      inline std::string getEPCRegsFileName()  const { return epcFileName;}
      inline std::string getIOBRegsFileName()  const { return iobFileName;}

      void readFullConfigFile(const std::string & fileName);

      ConfigReg * getEPCReg(){ return epcReg;}
      ConfigReg * getIOBReg(){ return iobReg;}

      virtual void storeFullConfigFile();
      virtual void storeFullConfigFile(std::string fileName, bool keepName = false);

      virtual void addFileName(RegType type, std::string line);

    private:

      virtual void loadConfig();
      virtual void clear();
      virtual bool checkConfig();
      virtual void generateFullConfigFile();
      virtual void newEPCReg();
      virtual void newIOBReg();

      void checkNewRegisterEntries();
      bool checkNewEntry(ConfigReg * reg, const std::string & moduleSet, const std::string & signalName,const std::string & posList, int address, uint32_t defaultValue, bool readOnly = false);
      bool removeEntry(ConfigReg * reg, const std::string & moduleSet, const std::string & signalName);

      void checkImportantRegisterValues();

  protected:

      ConfigReg *epcReg;
      ConfigReg *iobReg;

      std::string epcFileName;
      std::string iobFileName;
  };
}

#endif // PPTFULLCONFIGFILE_H
