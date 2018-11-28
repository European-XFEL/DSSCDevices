#ifndef MULTIMODULEINTERFACE_H
#define MULTIMODULEINTERFACE_H

#include <iostream>
#include <map>
#include <string>
#include <array>
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>


#ifdef F1IO
#include "CHIPInterface.h"
#elif F2IO
#include "F2CHIPInterface.h"
#else
#include "F2BCHIPInterface.h"
#endif

#include "PPTFullConfig.h"

#include "ConfigReg.h"
#include "Sequencer.h"

#ifdef HAVE_HDF5
#include "DsscHDF5Writer.h"
#include "DsscHDF5RegisterConfig.hh"
#endif


namespace SuS {

#ifdef F1IO
class MultiModuleInterface : public CHIPInterface
#elif F2IO
class MultiModuleInterface : public F2CHIPInterface
#else
class MultiModuleInterface : public F2BCHIPInterface
#endif
{
  public:
    MultiModuleInterface(PPTFullConfig *fullConfig);

    /**
      * Sets an EPC parameter - sets the parameter specified by name to a given value. This are
      *
      * @param moduleSet ModuleSet Name Containing the Signal to be set
      * @param moduleStr Module Name Containing the Value to be set
      * @param signalName Signal to be set
      * @param value value to set
      * @return DSSC_PPT error code, ERROR_OK or other error condition
      */
    inline void setEPCParam(const std::string& moduleSet, const std::string& moduleStr, const std::string& signalName, uint32_t value)
    {
        //std::cout << "Program EPC Register Signal " << signalName << " to " << value << std::endl;
        epcRegisters->setSignalValue(moduleSet,moduleStr,signalName,value);
    }

    /**
      * Gets an EPC parameter - get the parameter specified by name to a given value. This are
      *
      * @param moduleSet ModuleSet Name Containing the Signal to be set
      * @param moduleStr Module Name Containing the Value to be set
      * @param signal Signal to be set
      * @return value retrieved as uint32_t object
      */
    inline uint32_t getEPCParam(const std::string& moduleSet, const std::string& moduleStr, const std::string& signalName) const
    {
      uint32_t value = epcRegisters->getSignalValue(moduleSet,moduleStr,signalName);
      //std::cout << "Readout EPC Register Signal " << signalName << ": "<< value << std::endl;
      return value;
    }

    /**
      * Sets an IOB parameter - sets the parameter specified by name to a given value. This is
      * moduleStr is iobNumber 1-4
      * @param moduleSet ModuleSet Name Containing the Signal to be set
      * @param moduleStr Module Name Containing the Value to be set
      * @param signalName Signal to be set
      * @param value value to set
      * @return DSSC_PPT error code, ERROR_OK or other error condition
      */
    inline void setIOBParam(const std::string& moduleSet, const std::string& moduleStr, const std::string& signalName, uint32_t value) const
    {
      //std::cout << "Program IOB "<< moduleStr << " Register Signal " << signalName << " to " << value << std::endl;
      iobRegisters->setSignalValue(moduleSet,moduleStr,signalName,value);
    }

    inline int getBurstParamValue(const std::string &paramName) override {
      return getBurstParam(paramName);
    }

    inline int getBurstParam(const std::string &paramName) {
      const auto & param =  burstParams.find(paramName);
      if(param != burstParams.end()){
        return param->second;
      }

      std::cout << "DSSC_PPT BurstParamError: " << paramName << " unknown!" << std::endl;
      return 0;
    }

    void setBurstParam(const std::string &paramName, int value, bool program = true);

    void setDefaultBurstParamValues();


    std::vector<std::string> getBurstParamNames() const ;

    /**
      * Gets an IOB parameter - retrieves the value of a given parameter specified by name in
      * the arguments.
      * moduleStr is iobNumber 1-4
      * @param moduleSet ModuleSet Name Containing the Signal to be set
      * @param moduleStr Module Name Containing the Value to be set
      * @param signal Signal to be set
      * @return value retrieved as uint32_t object
      */
    inline uint32_t getIOBParam(const std::string& moduleSet, const std::string& moduleStr, const std::string& signalName)
    {
      uint32_t value = iobRegisters->getSignalValue(moduleSet,moduleStr,signalName);
      //std::cout << "Readout EPC Register Signal " << signalName << ": "<< value << std::endl;
      return value;
    }


    /**
      * Sets a parameter - sets the parameter specified by name to a given value. This are
      *
      * @param moduleSet ModuleSet Name Containing the Signal to be set
      * @param moduleStr Module Name Containing the Value to be set
      * @param signalName Signal to be set
      * @param value value to set
      * @return DSSC_PPT error code, ERROR_OK or other error condition
      */
    inline void setPixelParam(const std::string& moduleSet, const std::string& moduleStr, const std::string& signalName, uint32_t value)
    {
      if(moduleSet.compare("Control register") != 0) std::cout << "Module Set " << moduleSet << "unknown" <<  std::endl;

      setPixelRegisterValue(moduleStr,signalName,value);
    }

    /**
      * Gets a Pixel parameter - retrieves the value of a given parameter specified by name in
      * the arguments. If ASIC is ready to readout.
      * moduleStr is iobNumber 1-4.
      * Nothing is read back from the hardware. If it is a readback value the value can be updated by readBackPixelChain()
      *
      * @param moduleSet ModuleSet Name Containing the Signal to be set
      * @param moduleStr Module Name Containing the Value to be set
      * @param signalName Signal to be get
      * @see readBackPixelChain()
      * @see programPixelRegs()
      * @return DSSC_PPT error code, ERROR_OK or other error condition
       */
    inline uint32_t getPixelParam(const std::string& moduleSet, const std::string& moduleStr, const std::string& signalName)
    {
      if(moduleSet.compare("Control register") != 0) std::cout << "Module Set " << moduleSet << "unknown" <<  std::endl;

      std::string module = moduleStr;
      if(moduleStr.compare("all")== 0){
          module = "0";
      }

      return getPixelRegisterValue(module,signalName);
    }

    ConfigReg * getRegisters(const std::string & regName)
    {
      if (regName.compare("epc") == 0){
        return epcRegisters;
      }else if (regName.compare("iob") == 0){
        return iobRegisters;
      }else if (regName.compare("jtag") == 0){
        return jtagRegisters;
      }else if (regName.compare("pixel") == 0){
        return pixelRegisters;
      }
      return nullptr;
    }

    /** Get the epcRegisters Object
      * @see epcRegisters
      * @return ConfigReg holding the EPC configuration
      */
    ConfigReg * getEPCRegisters(){return epcRegisters;}

    /** Get the iobRegisters Object
      * @see iobRegisters
      * @return ConfigReg holding the IOB configuration
      */
    ConfigReg * getIOBRegisters(){return iobRegisters;}

    /** Get the jtagRegisters Object
      * @see jtagRegisters
      * @return ConfigReg holding the ASIC JTAG configuration
      */
    ConfigReg * getJTAGRegisters(){return jtagRegisters;}

    /** Get the pixelRegisters Object
      * @see pixelRegisters
      * @return ConfigReg holding the ASIC Pixel configuration
      */
    ConfigReg * getPixelRegisters(){return pixelRegisters;}

    /** Get the sequencer Object
      * @see sequencer
      * @return Sequencer holding the ASIC Sequencer configuration
      */
    Sequencer * getSequencer(){return sequencer;}

    bool isSingleAsicMode() const override;

    virtual void setSendingAsics(uint16_t asics);
    virtual void setActiveModule(int modNumber);
    virtual int getActiveModule() const {return currentModule;}

    void setActiveAsics(uint16_t asics);

    void storeFullConfigFile(const std::string & fileName, bool keepName=true) override;
    void loadFullConfig(const std::string & fileName, bool program) override;

    void updateNumberOfModulesInRegisters();
    void updateNumberOfModulesInJtagRegisters();
    void updateNumberOfModulesInPixelRegisters();

    PPTFullConfig const * getFullConfig() const {return pptFullConfig;}

    uint32_t getJTAGCurrentModule();

    bool setLogoConfig(const std::string & signalName, uint32_t value);

    void getOnlySendingASICPixels(std::vector<int> & sendingPixels) const override
    {
      for(int idx=sendingPixels.size()-1; idx>=0; idx--){
        int asic = getASICNumberFromPixel(sendingPixels[idx]);
        if(!isAsicSending(asic)){
          sendingPixels.erase(sendingPixels.begin()+idx);
        }
      }
    }

    void getOnlySendingASICPixels(std::vector<uint32_t> & sendingPixels) const override
    {
      for(int idx=sendingPixels.size(); idx>=0; idx--){
        int asic = getASICNumberFromPixel(sendingPixels[idx]);
        if(!isAsicSending(asic)){
          sendingPixels.erase(sendingPixels.begin()+idx);
        }
      }
    }

    bool measureBurstParamSweep(const std::string & burstParamName,
                                const std::vector<int> & paramValues,
                                const std::vector<uint32_t> & measurePixels,
                                std::vector<std::vector<double>> & binValues, const int STARTADDR, const int ENDADDR);

    std::vector<double> sweepBurstWaitOffset(uint32_t pixel, const std::vector<int> & paramValues, const int STARTADDR, const int ENDADDR);

    std::string getETHIP  (int channel, bool recvNotSend);
    std::string getETHMAC (int channel, bool recvNotSend);
    uint32_t    getETHPort(int channel, bool recvNotSend);

    void setETHIP  (int channel, bool recvNotSend, const std::string & addr, bool program=true);
    void setETHMAC (int channel, bool recvNotSend, const std::string & addr, bool program=true);
    void setETHPort(int channel, bool recvNotSend,    uint32_t port, bool program=true);

    void enableLastLoadedETHConfig();
    void fillLastLoadedETHConfig();

    struct ETHCONFIG{
        std::string mac;
        std::string ip;
        uint16_t port;
    };
    // Sender,Recevier
    using PPTETHConfig = std::array<std::pair<ETHCONFIG,ETHCONFIG>,4>;

    virtual void setNumberOfActiveAsics(int numAsics);
    int getNumberOfActiveAsics() const override;
    int getNumberOfSendingAsics() const override;
    int getFirstActiveAsic() const;

    std::string getAllPixelStringOfASIC(int asic);

    void updateEPCRegister(const std::string & fileName);
    void updateIOBRegister(const std::string & fileName);
    void updateJTAGRegister(const std::string & fileName);
    void updatePixelRegister(const std::string & fileName);
    void updateSequencer(const std::string & fileName);

    virtual void programEPCRegister(const std::string & moduleSet) = 0;

#ifdef HAVE_HDF5
    static DsscHDF5RegisterConfig getRegisterConfig(const std::string & registerName, SuS::ConfigReg *reg);
    static DsscHDF5RegisterConfig getRegisterConfig(const std::string & registerName, const std::string & configFileName);

    void updateConfigRegister(const DsscHDF5RegisterConfig & newRegisterConfig);
    void updateConfigRegister(ConfigReg * reg, const DsscHDF5RegisterConfig & newRegisterConfig);

    static DsscHDF5ConfigData getHDF5ConfigData(SuS::MultiModuleInterface * mmi);
    static DsscHDF5ConfigData getHDF5ConfigData(const std::string & fullConfigFileName);
#endif

  public:
    virtual int initSystem() = 0;
    virtual bool updateAllCounters() = 0;
    virtual void updateStartWaitOffset(int value) = 0;

  public:
    static constexpr double PPTCYCLELENGTH =  7.0 / 695000.0; // in ms           //1e-5
    static constexpr double MICROSECOND    =  1.0 / PPTCYCLELENGTH / 1000.0;       //99.285
    static constexpr int    PRBPROGTIME    = 14.0 * MICROSECOND;                 //6 us to programm 13.5 until power is up
    static constexpr int    FETOFFSET      = 9 * MICROSECOND + 44;
    static constexpr int    DISOFFSET      = PRBPROGTIME - 14;
    static constexpr int    CLROFFSET      = 1;

    static constexpr uint32_t ASIC_JTAG_ENGINE_1_OFFSET = 52;

    std::string errorString;
    std::vector<std::string> errorMessages;

  protected:
    static std::ofstream debugOut;


    PPTFullConfig *pptFullConfig;

    ConfigReg *epcRegisters;
    ConfigReg *iobRegisters;

    int currentModule;
    std::map<std::string, int> burstParams;
    PPTETHConfig m_lastPPTETHConfig;
};

}

#endif // MULTIMODULEINTERFACE_H
