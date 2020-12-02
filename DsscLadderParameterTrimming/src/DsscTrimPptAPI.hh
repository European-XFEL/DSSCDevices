/* 
 * File:   DsscTrimPptAPI.h
 * Author: samartse
 *
 * Created on June 11, 2019, 2:35 PM
 */

#ifndef DSSCTRIMPPTAPI_H
#define	DSSCTRIMPPTAPI_H


#include "MultiModuleInterface.h"

namespace karabo{
class DsscLadderParameterTrimming;
}


namespace SuS{

class DsscTrimPptAPI: public SuS::MultiModuleInterface {
public:
    
    friend class karabo::DsscLadderParameterTrimming;
    
    DsscTrimPptAPI(karabo::DsscLadderParameterTrimming* _karaboDevice, const std::string& configFile);
    DsscTrimPptAPI(const DsscTrimPptAPI& orig) = delete;
    virtual ~DsscTrimPptAPI();
    
    void displayInjectionSweep(const std::vector<std::vector<double>> &binValues, const std::vector<unsigned int> & xValues, const std::vector<uint32_t> & measurePixels) override;
    void displayDataHistos(const utils::DataHistoVec & dataHistoVec, const std::vector<uint32_t> & measurePixels) override;   
    
private:
    const uint16_t *getPixelSramData(int pixel) override;
   
    int initSystem() override;
    
    bool updateAllCounters() override;
    
    void updateStartWaitOffset(int value) override;
    
    bool doSingleCycle(DataPacker* packer = NULL, bool testPattern = false) override;
    
    bool doSingleCycle(int numTries, DataPacker* packer = NULL, bool testPattern = false) override;
    
    std::vector<double> measureBurstData(const std::vector<uint32_t> & measurePixels, int STARTADDR, int ENDADDR, bool subtract) override;

    std::vector<double> measureRMSData(const std::vector<uint32_t> & measurePixels, const int STARTADDR, const int ENDADDR) override;
    
        // configuration function with DsscPpt Device
    bool isHardwareReady() override;

    bool fillSramAndReadout(uint16_t pattern, bool init, bool jtagMode = false) override;

    bool fastInitChip() override;
    
    void initChip() override;
    
    
    bool getContentFromDevice(uint32_t bitStreamLength, std::vector<bool> &data_vec) override;

    bool programJtag(bool readBack = false, bool setJtagEngineBusy = true, bool recalcXors = true) override;
    
    bool programJtagSingle(const std::string & moduleSetName, bool readBack = false, bool setJtagEngineBusy = true, bool recalcXors = true, bool overwrite = false) override;
    
    bool programPixelRegs(bool readBack = false, bool setJtagEngineBusy = true) override;
    
    void programPixelRegDirectly(int px, bool setJtagEngineBusy = true) override;
    
    bool programSequencer(bool readBack = false, bool setJtagEngineBusy = true, bool program = true) override;
   
    int programIOBRegister(const std::string & moduleSetStr, bool overwrite = false) override {
        return 0;
    };
    
    void setBurstVetoOffset(int val) override;
    
    int getBurstVetoOffset() override;

    void setNumFramesToSend(int val, bool saveOldVal = true) override;
    
    void setNumWordsToReceive(int val, bool saveOldVal = true) override;

    void runContinuousMode(bool run) override;

    void setRoSerIn(bool bit) override;

    void setSendRawData(bool enable, bool reordered = false, bool converted = false) override;

    void waitJTAGEngineDone() override;

    int getNumberOfActiveAsics() const override;
    
    int getNumberOfSendingAsics() const override;
    
    int getActiveASICToReadout() const override;
    
    void setSendingAsics(uint16_t asics) override;
    
    void setActiveModule(int modNumber) override;
    
    void sramTest(int iterations, bool init = false) override;

    void resetDataReceiver() override;
    
    bool inContinuousMode() override;
   
    //functions without functionality
    const uint16_t *getTrailerData() override {
        return nullptr;
    }
    
    void programEPCRegister(const std::string & moduleSet) override {
    }

    virtual void resetChip() override {
    }
    
    int runTestPatternAcquisition(uint16_t _testPattern) override {
        return 0;
    }

    bool isLadderReadout() override {
        return true;
    }
    
    void setLadderReadout(bool enable) override {
    }

    void testJtagInChain(bool inChain) override {
    }
    
    bool writeFpgaRegisters() override {
        return true;
    }

    void setDebugOutputSelect(int SMA_1_val, int SMA_2_val, bool enableSMA_1 = true, bool enableSMA_2 = true) override {
    }

    int getStaticVDDA() override {
        return 0;
    }

    int getStaticVDDDADC() override {
        return 0;
    }
    
    int getStaticVDDD() override {
        return 0;
    }
    
    void updateDataPackerPixelOffsetsMap() override {
    }

    void flushFtdi() override {
    }

    void setCHIPReadout(ChipReadout mode) override {
    }

    int getCHIPReadout() override {
        return 0;
    }

    void setMultiCyclesNum(int num, bool program = false) override {
    }

    void generateInitialChipData(DataPacker *packer) override {
    }

    void setFullFReadoutMode(bool en, bool program = false) override {
    }

    void setWindowReadoutMode(bool en, bool program = false) override {
    }

    bool getSinglePixelReadoutMode() override {
        return false;
    }
    
    void setSinglePixelReadoutMode(bool en, bool program = true) override {
    }

    void setSinglePixelToReadout(int px, bool program = false) override {
    }

    //only used in MeasurementWidget

    void enBusInj(bool enable, std::string pixel = "all") override {
    }

    void enBusInjRes(bool enable, std::string pixel = "all") override {
    }

    void enPxInjDC(bool enable, std::string pixel = "all") override {
    }
    
    bool calibrateCurrCompDACForReticleTest(bool log = true, int singlePx = -1, int startSetting = 0, int defaultValue = 3) override {
        return true;
    }
  
    karabo::DsscLadderParameterTrimming* m_karaboDevice;    

};

}

#endif	/* DSSCTRIMPPTAPI_H */

