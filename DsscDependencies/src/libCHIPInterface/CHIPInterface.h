#ifndef CHIPINTERFACE_H
#define CHIPINTERFACE_H

#include <mutex>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <map>

#include "CHIPFullConfig.h"

#include "ConfigReg.h"
#include "Sequencer.h"
#include "utils.h"
#include "DataHisto.h"
#include "DsscProgressBar.h"
#include "DsscSramBlacklist.h"
#include "PixelDataArray.h"

#define MINJTAGDELAY 100000

class DataPacker;

namespace SuS{


  class CHIPInterface
  {
    friend class ASICTestWidget;
    friend class CHIPTrimmer;

    public:
      CHIPInterface();
      CHIPInterface(CHIPFullConfig *fullConfig);

      ~CHIPInterface();

      int numOfPxs;
      int totalNumPxs;
      int sramSize;
      int pixelRegsBits;
      int c_controlBitsPerPx;
      int c_totalPxSelBits;
      int c_WordsInTrailer;
      int c_wordsInHeader;
      int maxCoarseCompToSet;

      bool m_leaveMySettings;
      uint32_t m_iterations;
      uint32_t m_numRuns;
      uint32_t m_trimStartAddr;
      uint32_t m_trimEndAddr;
      uint32_t m_trimPixelDelayMode;

      static const std::vector<int> asicDONumber;
      static const std::vector<int> asicInJTAGChain;
      static const std::vector<int> prbPowerInJTAGChain;

      static const uint16_t GCCWRAP = 6;
      static const uint16_t DEFAULTGCCSTART = 4;

      static const uint8_t c_jtagInstrBypass         = 0b00000011; //3
      static const uint8_t c_jtagInstrEnReadback     = 0b10000000;
      static const uint8_t c_jtagInstrLength         = 5;

      static const uint8_t c_jtagInstrSeqHoldCnts    = 0b00000100; // 4
      static const uint8_t c_jtagInstrSeqTrack       = 0b00001000; // 8
      static const uint8_t c_jtagInstrX0SelReg       = 0b00010001; //17
      static const uint8_t c_jtagInstrX1SelReg       = 0b00010010; //18
      static const uint8_t c_jtagInstrYSelReg        = 0b00010011; //19
      static const uint8_t c_jtagInstrPx0Reg         = 0b00011110; //30
      static const uint8_t c_jtagInstrPx1Reg         = 0b00011111; //31

      enum DECCAPSETTING {GLBL_NONE=0,GLBL_VDDA=1,GLBL_VDDDADC=2,GLBL_BOTH=3};
      enum CALIBERROR {ROERR=-10, RANGEERR = -2, RETRIM = -1, UNTRIMMED = 0, PARTLY = 1, FULL = 2};
      enum PXINJGAIN{LowGain=0,MediumGain=1,HighGain=2};

      virtual void setPixelInjectionGain(const std::string & pixelStr, PXINJGAIN injGain, bool program);
      virtual std::string getPixelInjectionGain();

      void setGGCStartValue(uint32_t value, bool program = true);

      virtual void setGlobalDecCapSetting(DECCAPSETTING newSetting);
      inline DECCAPSETTING getGlobalDecCapSetting(){return globalDeCCAPSetting;}
      bool compareGlobalDecCapBits(std::vector<bool> &bits, const std::string & moduleSet);

      virtual bool getContentFromDevice(uint32_t bitStreamLength, std::vector<bool> &data_vec) = 0;
      bool checkAllModuleRegsRBData(ConfigReg *configReg, bool overwrite = false);
      bool checkSingleModuleRegRBData(ConfigReg *configReg, const std::string & moduleSet, bool overwrite = false);

      bool checkSequencerTrackRBData(int seqTrackAddr);
      bool checkSequencerHoldRBData();
      bool checkSequencerRBData();

      void setSequencerMode(Sequencer::OpMode newMode);
      virtual void setExtLatchMode();
      virtual void setIntDACMode();
      virtual void setNormalMode();
      virtual void setPixelInjectionMode();

      void setSingleIntegrationMode(bool enable);

      // different for F2, enables the correct injection according to the mode set by setInjectionMode
      virtual void enableInjection(bool en, const std::string & pixelStr, bool program = true);
      virtual void disableInjectionBits();

      virtual bool hasFixedDecoupling() const {return false;}
      // only valid for F2, in F1 we only have PX_BGDAC in D0 (charge) mode
      // in F2, we have all four, in DEPFET F2, charge injection modes are not valid
      enum InjectionMode {CURRENT_BGDAC       = 0,  // current injection via Bergamo
                          CURRENT_SUSDAC      = 1,  // charge injection via HD
                          CHARGE_PXINJ_BGDAC  = 2,  // charge injection via Bergamo
                          CHARGE_PXINJ_SUSDAC = 3,  // charge injection via Bergamo pixel injection and HD peripheriy (signal) DAC
                          CHARGE_BUSINJ       = 4,
                          ADC_INJ             = 5,
                          ADC_INJ_LR          = 6,
                          EXT_LATCH           = 7,
                          NORM                = 8,
                          NORM_D0             = 9,
                          NORM_DEPFET         = 10
      };

      static bool isNormMode(const InjectionMode & mode){
        return (mode == NORM) || (mode == NORM_D0) || (mode == NORM_DEPFET);
      }

      virtual std::vector<std::string> getInjectionModes() {
        return getActiveNames(getActiveModes());
      }

      virtual std::vector<InjectionMode> getActiveModes() const {
        return {CURRENT_BGDAC,CHARGE_PXINJ_BGDAC,
                ADC_INJ,EXT_LATCH,NORM,ADC_INJ_LR};
      }

      std::vector<std::string> getActiveNames(const std::vector<InjectionMode> & activeModes){
        std::vector<std::string> activeNames;
        for(auto && mode : activeModes){
          activeNames.push_back(getInjectionModeName(mode));
        }
        return activeNames;
      }

      InjectionMode injectionMode;
      virtual void setInjectionMode(InjectionMode mode);
      void setInjectionMode(const std::string & modeStr);
      InjectionMode getInjectionMode();
      InjectionMode getInjectionMode(const std::string & modeName);
      std::string getInjectionModeName(InjectionMode m);
      virtual void setInjectionDAC(int setting);           // sets DACs according to the selected injection mode
      virtual int getInjectionDACsetting();
      inline void setBothDacsForPixelInjection(bool set)   // enables setting of both DACs to reduce a DAC effect,
             {setBothDacsForPixelInjectionVal = set;}      // should always be set to true, false
                                                           // setting only intended for testing
      virtual bool readBackPixelChain();

      std::vector<std::string> getPixelRegOutputList();

      std::vector<std::string> getJtagRegOutputList();

      std::string getFullConfigFileName() const {return fullChipConfig->getFullConfigFileName();}
      virtual void storeFullConfigFile(const std::string & fileName, bool keepName = true);
      virtual void loadFullConfig(const std::string & fileName, bool program = true);
      virtual bool checkRegisterTypes();

      inline uint32_t getOnPixelsSize() const {return onPixels.size();}
      inline int getSramSize() const {return sramSize;}
      inline int getNumPxs() const {return numOfPxs;}
      inline int getPixelRegsBits() const {return pixelRegsBits;}
      virtual inline void setPixelRegisterValue(const std::string & moduleStr, const std::string & signalName, uint32_t value){
        pixelRegisters->setSignalValue("Control register", moduleStr, signalName, value);}
      virtual inline uint32_t getPixelRegisterValue(const std::string & moduleStr, const std::string & signalName){
        return pixelRegisters->getSignalValue("Control register",  moduleStr, signalName);}
      inline std::vector<bool> getPixelRegisterBits() {
        return pixelRegisters->printContent("Control register");}
      virtual inline const std::vector<std::string> & getPixelRegisterSignalNames(int px=0){ // pixel argument needed for MM7
        return pixelRegisters->getSignalNames("Control register");}
      virtual inline bool pixelRegistersSignalIsVarious(const std::string & signalName, const std::string & moduleStr) {
        return pixelRegisters->signalIsVarious("Control register", signalName, moduleStr);}
      inline bool initPixelRegistersFromFile(const std::string &_filename){
        return pixelRegisters->initFromFile(_filename);
      }
      inline bool savePixelRegistersToFile(const std::string &_filename){
        pixelRegisters->saveToFile(_filename);
        return true;
      }

      inline void setJTAGParam(const std::string& moduleSet, const std::string& moduleStr, const std::string& signalName, uint32_t value) {
        jtagRegisters->setSignalValue(moduleSet,moduleStr,signalName,value);
      }

      inline uint32_t getJTAGParam(const std::string& moduleSet, const std::string& moduleStr, const std::string& signalName) const  {
        std::string module = moduleStr;
        if(moduleStr.compare("all")== 0){
            module = "0";
        }
        return jtagRegisters->getSignalValue(moduleSet,module,signalName);
      }

      uint16_t getExpectedTestPattern() const;
      virtual uint16_t getTestPattern() const {return getExpectedTestPattern();}

      virtual void resetChip() = 0;
      virtual void enBusInj(bool enable, std::string pixel = "all") = 0;
      virtual void enBusInjRes(bool enable, std::string pixel = "all") = 0;
      virtual void enPxInjDC(bool enable, std::string pixel = "all") = 0;

      virtual const uint16_t * getPixelSramData(int pixel) = 0;
      virtual const uint16_t * getPixelSramData(int asic, int pixel){
        if(asic > 0){
          std::cout << "ChipInterface - getPixelSramData : ASIC " << asic << " out of range.";
        }
        return getPixelSramData(pixel);
      }

      virtual const uint16_t  *getTrailerData() = 0;

      static uint16_t getASICTrailerWord(const std::vector<uint8_t> & trailerData, int asic, int wordNum);

      uint16_t getMaxADU(){return sequencer->getRampLength()*2;}
      bool setCompDacConfigCalibrated();

      virtual std::vector<std::string> getBurstParamNames() const = 0;
      virtual void setBurstParam(const std::string & paramName, int value, bool program = true) = 0;
      virtual int getBurstParamValue(const std::string & paramName) = 0;

      bool calibrateCurrCompIntDAC();
      virtual bool calibrateCurrCompDAC(bool log=true, int singlePx=-1, int startSetting=0, int defaultValue = 15);
      void calibrateCurrCompDAC(std::vector<uint32_t> unsettledPxs, int startSetting, int singlePx, bool log);
      bool calibrateCurrCompDAC(std::vector<std::pair<int, double> >& vholdVals, bool log=true, int singlePx=-1, int startSetting=0, int finalIterations=1);

      int trimPixelParamInOutlayers(const std::string & signalName, int STARTADDR, int ENDADDR, int lowerLimit, int upperLimit);
      int trimPixelParamToCenter(const std::string & signalName, int STARTADDR, int ENDADDR, int centerValue);

      std::string showOutlayers(int STARTADDR, int ENDADDR, int lowerLimit, int upperLimit, bool baseline);

      void trimCompCoarseSingleIntegration(int STARTADDR, int ENDADDR);
      bool trimCompCoarseByPedestal(int STARTADDR, int ENDADDR, int numRuns);
      std::vector<uint32_t> trimCompCoarseByPedestal(const std::vector<uint32_t> & pixelsToTrim,
                                                     std::vector<std::vector<double>> & binValues, int STARTADDR, int ENDADDR);
      std::vector<uint32_t> measurePedestalsOverCompCoarse(std::vector<uint32_t> measurePixels,
                                                           std::vector<std::vector<double>> & binValues, int STARTADDR, int ENDADDR);
      void showTrimmingResultsByPedestal(const std::vector<std::vector<double>> &binValues);

      bool findAndSetInjectionEventOffset(const std::vector<uint32_t> & pixels, int injectSetting, int sramStartAddr=0);
      inline int getInjectionPeriod() {return sequencer->getSequencerParameter(Sequencer::EmptyInjectCycles)+1;}
      std::vector<std::vector<double>> measurePixelParam(const std::string & signalName,
                                                         std::vector<uint32_t> measurePixels,
                                                         const std::vector<uint32_t> & paramSettings, int STARTADDR, int ENDADDR);
      virtual std::vector<double> measureBurstData(int STARTADDR, int ENDADDR, bool subtract); // function for external usage
      virtual std::vector<double> & measureMeanSramContent(uint32_t pixel, const int STARTADDR, const int ENDADDR, bool onlyStartEnd = false );
      virtual std::vector<double> & measureMeanSramContent(const std::vector<uint32_t> & measurePixels, const int STARTADDR, const int ENDADDR, bool onlyStartEnd = false );
      virtual std::vector<double> & measureDacSweepForCurvature(std::vector<uint32_t> & measurePixels,
                                                                const std::vector<uint32_t> & dacSettings, const int STARTADDR, const int ENDADDR);
      virtual std::vector<double> & measureSramDriftMap(const int STARTADDR, const int ENDADDR );
      virtual std::vector<double> & measureSramSlopesMap(const int STARTADDR, const int ENDADDR );

      virtual utils::DataHistoVec measureIntDacSweepForHistograms(const std::vector<uint32_t> & dacValues,
                                                                            const std::vector<uint32_t> & measurePixels, const int STARTADDR, const int ENDADDR );
      virtual bool measureIntDacSweepforSlopes(const std::vector<uint32_t> & dacValues,
                                               std::vector<uint32_t> & measurePixels,
                                               std::vector<std::vector<double>> & binValues, const int STARTADDR, const int ENDADDR);
      virtual bool measurePxInjSweepforSlopes(const std::vector<uint32_t> & injectionSettings,
                                              std::vector<std::vector<double>> & binValues, const int STARTADDR, const int ENDADDR);
      virtual std::vector<double> measurePixelInjectionSweepSlopes( const std::vector<uint32_t> & injectionSettings, const int STARTADDR, const int ENDADDR);

      // virtual required for reimplementation in Karabo
      virtual std::vector<double> measureBurstData(const std::vector<uint32_t> & measurePixels, int STARTADDR, int ENDADDR, bool subtract);
      // virtual required for reimplementation in Karabo
      virtual void measureHistoData(const std::vector<uint32_t> & measurePixels,
                                    utils::DataHistoVec & pixelHistograms, const int STARTADDR, const int ENDADDR );
      virtual utils::DataHistoVec measureHistoData(const std::vector<uint32_t> & measurePixels, const int STARTADDR, const int ENDADDR );

      virtual std::vector<double> measureRMSData(const std::vector<uint32_t> & measurePixels, const int STARTADDR, const int ENDADDR );

      virtual void displayInjectionSweep(const std::vector<std::vector<double>> & binValues, const std::vector<unsigned int> & xValues, const std::vector<uint32_t> & measurePixels){}
      virtual void displayDataHistos(const utils::DataHistoVec & dataHistoVec, const std::vector<uint32_t> & measurePixels){}

      //optimized function s_x s_xx is constant so dont compute for every pixel
      static double linearRegression(const uint32_t s_x, const uint32_t s_xx,
                                     const std::vector<uint32_t> & xValues,
                                     const std::vector<double> & yValues);

    private:
      void addMeasuredDataToMeanSramContent(const std::vector<uint32_t> & measurePixels, const int STARTADDR, const int ENDADDR, bool onlyStartEnd);
      void calcMeanSramContent(double numIterations, const std::vector<uint32_t> & measurePixels, const int STARTADDR, const int ENDADDR, bool onlyStartEnd);
      static double calcCurvatureGraph(const std::vector<uint32_t> & xValues,
                                       const std::vector<double> & yValues);
      static double calcSlopeOfSection(int step, const std::vector<uint32_t> & xValues,
                                                 const std::vector<double> & yValues);

      static uint32_t findBestCompCoarseSettingFromSingleInt(std::vector<double> values);
      static uint32_t findBestCompCoarseSettingFromPedestal(const std::vector<double> & binValues);

      void getBestDeskewSetting(const std::vector<uint32_t> & poweredAsics,
                                const std::vector<std::vector<double>> & asicDeskew0DataVec,
                                const std::vector<std::vector<double>> & asicDeskew1DataVec);

    public:
      std::vector<uint32_t> getPixelsToTrimForCompCoarseByPedestal(int run, const int NUM);

      void setIntegrationTimeBaseline(int STARTADDR, int ENDADDR, int value);
      void setPixelInjectionBaseline(int STARTADDR, int ENDADDR, int baselineSetting);

      void setBaseline(int STARTADDR, int ENDADDR);
      void setBaseline(const std::vector<uint32_t> & measurePixels, int STARTADDR, int ENDADDR);
      inline void clearBaseLine() {baselineValues.assign(totalNumPxs,0);   baselineValuesValid = false; }

      int getBestSetting(const std::vector<int> & eval);

      void selectBestCurrentCompSetting(std::ofstream & logStream, const std::vector< std::vector<uint16_t> > & vHoldMap,
                                        const std::vector<uint32_t> & unsettledPixels);

      void exportSingleASIC(int ASIC, const std::string & outFileName);

      enum TCDM {SLOW=0,DOUBLE=1,BOTH=2};

      void setTrimCurrentDoubleMode(TCDM mode){trimCurrentDoubleMode = mode;}
      TCDM getTrimCurrentDoubleMode(){return trimCurrentDoubleMode;}
      TCDM trimCurrentDoubleMode;

      void addMeasuredDataToHisto(const std::vector<uint32_t> & measurePixels,  utils::DataHistoVec & pixelHistograms, const int STARTADDR, const int ENDADDR );
      double getMeanDNLRMSFromHistos(const  utils::DataHistoVec & pixelHistograms, int asic, int asicHalf);

      std::string getGoodTrimmedPixelsStr(std::vector<double> & trimmedSlopes, std::string & rangeStr, const double aimSlope, const double maxAbsDiff, int & goodSlopesRel) const;
      void removeErrorPixelsFromVector(const std::vector<bool> & errorPixels, std::vector<uint32_t> & unsettledPixels);

      bool findBestDACSetting(const std::vector<uint32_t> & dacValues,
                              const std::vector<uint32_t> & injectionSettings,
                              int startAddr, int endAddr);

      bool measureFBCapGains(const std::vector<uint32_t> & fbCaps,
                             const std::vector<uint32_t> & injectionSettings,
                             int startAddr, int endAddr);

      void findBestDay0FEGain(const std::vector<std::vector<double>> & pixelSlopes, const std::vector<uint32_t> & dacValues);

      void writeDay0FEGainTrimmingLog(const std::vector<uint32_t> & activePixels, const std::vector<std::vector<double>> & pixelSlopes, const std::vector<uint32_t> & dacValues);
      void writeFEGainTrimmingLog(const std::vector<uint32_t> & activePixels, const std::vector<std::vector<double>> & pixelSlopes, const std::vector<uint32_t> & fbCaps);
      void exportRunSlopesStats(const std::vector<double> &pixelSlopes, const std::string & slopesStatsFileName);


      void trimForMaxDynamicRange(const std::string & pixelStr , int startSRAM, int endSRAM, int pixelInjectionSignal, bool alsoIramp);

      std::vector<std::vector<uint16_t>> findCoarseDACSettings(const std::vector<uint32_t> & dacValues);
      void setCoarseDACSettings(const std::vector<std::vector<uint16_t>> & coarseDACSettings, int dacIdx);
      void trimFEGains(const std::vector<std::vector<double>> & pixelSlopes);

      int calcRampDiff(double slopeDiff, double aimSlope);

      bool itWillBeHardToGetValidValues(const std::vector<uint32_t> & unsettledPxs, const std::vector<uint32_t> & dacValues);

      bool calibrateDay0CurrCompDAC();
      bool calibratePXInjCurrCompDAC();

      int checkCurrCompDACCalibration(std::string & status, std::vector<uint32_t> & unsettledPixels);


      std::vector<double> measureDNLRMSValues(const std::vector<uint32_t> & dacValues, int startAddr, int endAddr, int quarter);
      utils::DataHistoVec measureDNLValues(const std::vector<uint32_t> & pixelsToMeasure, const std::vector<uint32_t> & dacValues, int startAddr, int endAddr);

      void calibrateClockDeskewDNL(const std::vector<uint32_t> & dacValues, int startAddr, int endAddr, int quarter);
      void calibrateClockDeskewDarkFrame(int startAddr, int endAddr);
      double checkClockDeskewHalfDarkFrame(bool deskew0, int asic, int startAddr, int endAddr);
      void measureIterationsClockDeskewDarkFrame(int startAddr, int endAddr,
                                                 const std::vector<uint32_t> & poweredAsics,
                                                 std::vector<double> & asicIterationsDeskew0MeanRMSVec ,
                                                 std::vector<double> & asicIterationsDeskew1MeanRMSVec);

      double calcSramPixelDataRMS(uint32_t pixel, uint32_t startAddr, uint32_t endAddr);
      utils::StatsAcc accSramDataStats(uint32_t pixel, uint32_t startAddr, uint32_t endAddr);


      void findBestPixelDelayValue(const std::vector<uint32_t> & pixelsToTrim, int mode, const std::vector<std::vector<double>> & pixelDelayMeanRMSVec);
      std::vector<std::vector<double>> measurePixelDelays(const std::vector<uint32_t> & pixelsToTrim, int mode, uint32_t startAddr, uint32_t endAddr);
      void calibratePixelDelay(int mode, int startAddr, int endAddr, int numRuns);
      void calibratePixelDelay();
      std::vector<uint32_t> getNotPixelDelayTrimmedPixels(uint32_t startAddr, uint32_t endAddr);


      void writeDACCalibrateLog();

      inline int getVholdVal(int px) {return currentVholdVals[px];}
      virtual void generateInitialChipData(DataPacker *packer) = 0;
      virtual bool doSingleCycle(DataPacker* packer=NULL, bool testPattern = false) = 0;
      virtual bool doSingleCycle(int numTries, DataPacker* packer=NULL, bool testPattern = false) = 0;


      virtual bool fastInitChip() = 0;
      virtual void initChip() = 0;
      virtual bool programJtag(bool readBack=false, bool setJtagEngineBusy=true, bool recalcXors = true) = 0;
      virtual bool programJtagSingle(const std::string & moduleSetName, bool readBack = false, bool setJtagEngineBusy = true, bool recalcXors = true, bool overwrite = false) = 0;
      virtual bool programPixelRegs(bool readBack=false, bool setJtagEngineBusy=true) = 0;
      virtual void programPixelRegDirectly(int px, bool setJtagEngineBusy=true) = 0;
      virtual bool programSequencer(bool readBack=false, bool setJtagEngineBusy=true, bool programJtag=true) = 0;


      enum RegisterType {AllRegs=0,JtagRegs=1,PxRegs=2,SeqRegs=3, NoRegs=4};

      virtual void calculateRegisterXors(RegisterType regs = AllRegs);
      virtual void calculatePixelRegisterXors(bool XOR_in);
              void calculateSequencerXors(bool XOR_in);
              void calculateJtagXors(bool XOR_in);


      enum ChipReadout {M_RO=0,F_RO=1,L_RO=2};

      virtual bool isHardwareReady() = 0;

      virtual void setCHIPReadout(ChipReadout mode) = 0;
      virtual int getCHIPReadout() = 0;
      virtual void setFullFReadoutMode(bool en, bool program = false) = 0;
      virtual void setWindowReadoutMode(bool en, bool program = false) = 0;

      virtual bool getSinglePixelReadoutMode() = 0;
      virtual void setSinglePixelReadoutMode(bool en, bool program = true) = 0;
      virtual void setSinglePixelToReadout(int px, bool program = false) = 0;

      virtual void setMultiCyclesNum(int num, bool program = false) = 0;
      virtual inline int getMultiCyclesNum() {return 0;}
      virtual void setBurstVetoOffset(int val) = 0;
      virtual int  getBurstVetoOffset() = 0;
      virtual void flushFtdi() = 0;

      void printCompCoarseStats();

      void decrCompCoarseForVHOLDValue(uint32_t startCompCoarse, int vHoldMax, int singlePx = -1);
      uint32_t decrCompCoarseForVHOLDValue(int vHoldMax, const std::vector<uint32_t> & calibPixels);
      void checkVholdBeforeTrimming(std::vector<uint32_t> & calibPixels, int vHoldMax);
      void decreaseAllCompCoarseByOne(const std::vector<uint32_t> & calibPixels);

      virtual inline bool hasCurrCompDac(int px) {return true;} // needed for self calibrating pixels in D0M1

      virtual bool writeFpgaRegisters() = 0;
      const std::vector<uint16_t> & measureVHoldValues(bool & ok);
      void updateVholdVals();

      inline uint32_t getByteStreamLength(uint32_t bitStreamLength) const {
        return (uint32_t)(bitStreamLength/8) + (bitStreamLength%8==0 ? 0 : 1); // ugly way of ceiling()...
      }

      virtual void setNumFramesToSend(int val, bool saveOldVal = true) = 0;
      virtual inline uint32_t getNumFramesToSend(){return getNumWordsToReceive()/numOfPxs;}
      virtual inline uint32_t getNumWordsToReceive(){return 0;}
      virtual void setNumWordsToReceive(int val, bool saveOldVal = true) = 0;

      inline void setSweepParamsOnly(bool keep){ sweepParamsOnly = keep;}
      inline bool onlySetSweepParams(){ return sweepParamsOnly;}

      virtual void runContinuousMode(bool run) = 0;
      virtual bool inContinuousMode() = 0;

      virtual void setRoSerIn(bool bit) = 0;
      virtual void setSendRawData(bool enable, bool reordered = false, bool converted = false) = 0;
      virtual void waitJTAGEngineDone() = 0;
      virtual void testJtagInChain(bool inChain) = 0;
      virtual int runTestPatternAcquisition(uint16_t _testPattern) = 0;

      virtual void sramTest(int iterations, bool init = false) = 0;
      virtual int saveSramTestResult(const std::string & filePath, const std::string & moduleInfo, uint16_t testpattern );
      virtual bool fillSramAndReadout(uint16_t pattern, bool init, bool jtagMode=false);
      virtual void jtagClockInSramTestPattern(uint16_t testPattern);

      virtual void setStaticVDDA(bool enVDDA){}
      virtual void setStaticVDDDADC(bool enVDDDADC){}
      virtual void setStaticVDDD(bool enVDDD){}
      virtual void setVDDAandVDDDADCSuppliesStatic(bool en){}

      virtual int getStaticVDDA() = 0;
      virtual int getStaticVDDDADC() = 0;
      virtual int getStaticVDDD() = 0;


      virtual void setDebugOutputSelect(int SMA_1_val, int SMA_2_val, bool enableSMA_1 = true, bool enableSMA_2 = true) = 0;

      // implemented functions
      void powerDownPixels(const std::string &pixelStr, bool program = false);
      virtual void powerUpPixels(const std::string &pixelStr, bool program = false, DataPacker *packer = nullptr);
      void updatePoweredPx();
      virtual void setPowerDownBits(const std::string & pixelStr, bool powerDown);

      bool isPoweredPixel(int px){return getPixelRegisterValue(std::to_string(px),"LOC_PWRD") == 0;}
      void removeUnpoweredPixels(std::vector<uint32_t> & pixels);

      std::vector<uint32_t> poweredPixels; //vector of size numPixels, holds if pixel is on or off
      std::vector<uint32_t> onPixels;      //vector of size of pixels that are on, holds pixelnumber of on pixels

      void checkADCLimit();
      void initUpperVHoldVals();

      bool untrimmablePixelsFound;
      std::vector<bool> untrimmablePixels;
      std::vector<uint32_t> getUntrimmablePixels() const;
      std::vector<uint32_t> getPowerDwnPixels(); // generated from powerDwnPxs alsways up to date
      std::vector<uint32_t> getInactivePixels(); // generated from on pixels updated with updatePowerDownPixels
      std::vector<uint32_t> errorCodePixels;


      const std::vector<uint32_t> & updatePoweredPixels();
      const std::vector<uint32_t> & getPoweredPixels(){return onPixels;}
      virtual const std::vector<uint32_t> getPoweredPixelsFromConfig();

      std::string getPoweredPixelsStr();
      std::string getPowerDwnPixelsStr();
      bool isASICPoweredDown(int asic);
      std::vector<uint32_t> getPoweredAsics();

      template<typename INT_T = uint32_t>
      std::vector<INT_T> getASICPixels(const std::string & asics, const std::string & pixels)
      {
        auto selASICs = asics;
        auto pixelsStr = pixels;

        if(asics.substr(0,1)  == "a"){
          if(getNumberOfActiveAsics() == 1) selASICs = "0";
          else selASICs = "0-15";
        }
        if(pixels.substr(0,1) == "a") pixelsStr = "0-4095";

        const auto asicsVec  = utils::positionListToVector(selASICs);
        const auto pixelsVec = utils::positionListToVector(pixelsStr);

        std::vector<INT_T> asicPixels;
        asicPixels.reserve(asicsVec.size()*pixelsVec.size());

        for(const auto & asic : asicsVec){
          for(const auto & pixel : pixelsVec){
            asicPixels.push_back(getImagePixelNumber(asic,pixel));
          }
        }
        return asicPixels;
      }

      template <typename T=int> std::vector<T> getPixels(const std::string & chipPart)
      {
        if (chipPart.compare(0,3,"col")==0) {
          std::string chipPartNew =  chipPart.substr(3);
          //std::cout << "chipPart.substr(3) = " << chipPartNew << std::endl;
          return getColumnPixels<T>(chipPartNew);
        } else {
          std::cout << "No other chipParts implemented yet." << std::endl;
          return std::vector<T>();
        }
      }

      template <typename T=int> std::vector<T> getAllSendingPixels(const std::string & chipPartsStr)
      {
        const auto chipPartsVec = utils::getChipParts(chipPartsStr);
        std::vector<T> allPixels;
        for(auto && part : chipPartsVec){
          const auto pixels = getSendingPixels(part);
          allPixels.insert(allPixels.end(),pixels.begin(),pixels.end());
        }
        std::sort(allPixels.begin(),allPixels.end());
        return allPixels;
      }


      template <typename T=int> std::vector<T> getSendingPixels(const std::string & chipPart)
      {
        if (chipPart.compare(0,3,"col")==0) {
          std::string chipPartNew =  chipPart.substr(3);
          //std::cout << "chipPart.substr(3) = " << chipPartNew << std::endl;
          return getSendingColumnPixels<T>(chipPartNew);
        } else {
          std::cout << "No other chipParts implemented yet." << std::endl;
          return std::vector<T>();
        }
      }




      template <typename T=int> std::vector<T> getSendingColumnPixels(const std::string & colsStr)
      {
        std::string remColsStr;
        auto colonPos = colsStr.find(":");
        int offset = 0;
        int count  = 0;
        if (colonPos!=std::string::npos) {
          remColsStr = colsStr.substr(0,colonPos);
          int offsetPos = colsStr.find("o");
          count  = stoi(colsStr.substr(colonPos+1,colonPos-offsetPos));
          offset = stoi(colsStr.substr(offsetPos+1));
        } else {
          remColsStr = colsStr;
          if(colsStr.find("col") != std::string::npos){
            remColsStr = colsStr.substr(3);
          }
        }

        std::vector<int> cols = utils::positionListToVector<int>(remColsStr);
        std::vector<T> pixels;
        for (auto c: cols) {
          std::vector<T> colPxs = getSendingColumnPixels<T>(c);
          pixels.insert(pixels.end(),colPxs.begin()+offset,
                        //(count==0 || count+offset>colPxs.size() ? colPxs.end()
                        (count==0 ? colPxs.end() : colPxs.begin()+offset+count)
                       );
        }
        return pixels;
      }


      template <typename T=int> std::vector<T> getSendingColumnPixels(int col)
      {
        int numAsics = 1;
        int numColPixels = c_pxsPerCol;
        int numRowPixels = c_pxsPerRow;

        if(getNumberOfActiveAsics() == 16){
          numAsics = 16;
          numRowPixels = 512;
        }

        const auto sendingAsics = getSendingAsicsVec();

        std::vector<T> pixels;
        for(auto && sendingAsic : sendingAsics){
          int asic = (numAsics==1)? 0 : sendingAsic;
          int asicColOffs = asic%8 * 64;
          int asicRowOffs = asic/8 * 64;
          int asicOffs   = asicRowOffs * numRowPixels + asicColOffs;
          for (int row=0; row<numColPixels; ++row) {
            T imagePixel = asicOffs + col + row*numRowPixels;
            pixels.push_back(imagePixel);
          }
        }
        std::sort(pixels.begin(),pixels.end());
        return pixels;
      }


      template <typename T=int> std::vector<T> getColumnPixels(int col)
      {
        int numAsicCols = 1;
        int numColPixels = c_pxsPerCol;
        int numRowPixels = c_pxsPerRow;

        if(getNumberOfActiveAsics() == 16){
          numAsicCols = 8;
          numColPixels = 128;
          numRowPixels = 512;
        }

        std::vector<T> pixels;
        for(int asicCol = 0; asicCol < numAsicCols; asicCol++){
          for (int i=0; i<numColPixels; ++i) {
            pixels.push_back((T)(col + i*numRowPixels));
          }
          col += 64;
        }
        return pixels;
      }

      template <typename T=int> std::vector<T> getColumnPixels(const std::string & colsStr)
      {
        std::string remColsStr;
        auto colonPos = colsStr.find(":");
        int offset = 0;
        int count  = 0;
        if (colonPos!=std::string::npos) {
          remColsStr = colsStr.substr(0,colonPos);
          int offsetPos = colsStr.find("o");
          count  = stoi(colsStr.substr(colonPos+1,colonPos-offsetPos));
          offset = stoi(colsStr.substr(offsetPos+1));
        } else {
          remColsStr = colsStr;
          if(colsStr.find("col") != std::string::npos){
            remColsStr = colsStr.substr(3);
          }
        }

        std::vector<int> cols = utils::positionListToVector<int>(remColsStr);
        std::vector<T> pixels;
        for (auto c: cols) {
          std::vector<T> colPxs = getColumnPixels<T>(c);
          pixels.insert(pixels.end(),colPxs.begin()+offset,
                        //(count==0 || count+offset>colPxs.size() ? colPxs.end()
                        (count==0 ? colPxs.end() : colPxs.begin()+offset+count)
                       );
        }
        return pixels;
      }

      std::vector<int> getRowPixels(int row);
      int getPxsPerCol() {return c_pxsPerCol;}
      int getPxsPerRow() {return c_pxsPerRow;}

      std::pair<int,int> getSensorPixelCoordinates(int pixel);

      virtual bool checkD0Mode();
      virtual bool checkBypCompression();

      bool getByPassMode() {return d0BypCompr;}
      bool getD0Mode() {return d0Mode;}
      inline void setD0Mode(bool en, bool bypCompr=false) {
          //std::cout << ((en)? "Enable " : "Disable ") << "Day0 Mode" << (bypCompr ? " with compression bypass" : "") << std::endl;
          d0Mode=en; d0BypCompr=bypCompr;
      }

      virtual void resetDataReceiver() = 0;
      virtual uint32_t readXORRegister();

      virtual bool calibrateCurrCompDACForReticleTest(bool log=true, int singlePx=-1, int startSetting=0, int defaultValue = 3)=0;

      void setReticleTestMode(bool enable){reticleTestMode = enable;}
      const std::vector<std::string>& getReticleTestErrorList(){return reticleTestErrorList;}
      void clearReticleErrorList(){reticleTestErrorList.clear();}

      int addErrorBitsToList(const std::vector<int>& errBits,const std::string & regName);
      int addErrorBitsToList(ConfigReg* configReg);
      int addErrorBitsToList(Sequencer* seq);

      bool reticleTestMode;
      std::vector<std::string> reticleTestErrorList;

      inline void setRecalCurrCompOnPowerUp(bool en) {recalCurrCompOnPowerUp=en;}

      inline static bool getDebugMode() {return debugMode;}
      static void setDebugMode(bool _debugMode) {debugMode = _debugMode;}

      void setIntDACValue(int setting, bool log = false);
      int getIntDACValue();

      static void setEmitReadoutDone(bool val){emitReadoutDone = val;}

      void updateNumXors();
      void stopTrimming() { runTrimming = false;}
      void startTrimming(){ runTrimming = true;}
      bool runTrimming;

      int checkDecouplingCaps();

      void setActiveComponentId(const std::string & name){
        std::cout << "Active Component ID Set to " << name << std::endl;
        activeComponentId = name;
      }
      std::string getActiveComponentId() const { return activeComponentId;}

      void showCompareErrors(ConfigReg *configReg, const std::string & fileName);
      void showCompareErrors(Sequencer *seq, const std::string & fileName);
      void showCompareErrors(ConfigReg * configReg);
      void showCompareErrors(Sequencer * seq);

      int getImagePixelNumber(int asic, int asicPixel) const;
      int getDataPixelNumber(int pixel);
      int getASICNumberFromPixel(int pixel) const;
      int calcAsicHalf(uint32_t asic, uint32_t pixel);


      inline uint32_t getTotalNumPixels(){return numOfPxs * getNumberOfActiveAsics();}
      inline uint32_t getTotalNumSendingPixels(){return numOfPxs * getNumberOfSendingAsics();}
      virtual void updateDataPackerPixelOffsetsMap() = 0;
      virtual int getNumberOfActiveAsics() const {return 1;}
      virtual int getNumberOfSendingAsics() const { return 1;}
      virtual int getActiveASICToReadout() const {return 0;}

      // should not be implemented here since no single asic setup will change this
      //void setActiveAsics(uint16_t asics);
      //void setSendingAsics(uint16_t asicDOs);

      static std::string getASICSInfoStr(uint16_t asics);

      std::string getActiveAsicsStr() {return getASICSInfoStr(activeAsics);}
      std::string getSendingAsicsStr(){return getASICSInfoStr(sendingAsics);}
      std::string getSendingAsicDOsStr(){return getASICSInfoStr(getAsicDOs(sendingAsics));}

      static uint16_t getAsicDOs(uint16_t asics);
      inline bool isAsicActive(uint32_t asic) const {return activeAsics & ( 1 << asic);}
      inline bool isAsicSending(uint32_t asic) const {return sendingAsics & ( 1 << asic);}
      uint32_t getSendingASICIdx(uint32_t asic) const;
      std::vector<uint32_t> getSendingAsicsVec() const;
      std::vector<uint32_t> getSendingPixels() const;
      uint16_t getSendingAsics() const {return sendingAsics;}

      bool isPixelSending(uint32_t pixel) const {
        auto asic = getASICNumberFromPixel(pixel);
        return isAsicSending(asic);
      }

      virtual void getOnlySendingASICPixels(std::vector<int> & sendingPixels) const { }
      virtual void getOnlySendingASICPixels(std::vector<uint32_t> & sendingPixels) const { }

      void checkSendingASICs(std::string & selASICsStr) const ;
      void checkSendingASICs(std::vector<uint32_t> & selASICs) const;

      // only needed for F2
      virtual inline void enableMonBusForPixels(const std::vector<int> & pixels) {}
      virtual void enableMonBusCols(std::string colsStr) {} // string may start with "col"
      virtual void enableMonBusCols(const std::vector<int> & cols) {}
      virtual void disableMonBusCols() {}

      std::string powerUpQuarterPixels(int quarter);
      std::string restorePoweredPixels();
      std::string getDNLTrimPixels(int quarter);
      std::string getQuarterASICPixelNumbers(int quarter, bool rowWise = true); //returns string containing quarter of all asic pixels, distributed on all asics
      virtual std::string getHalfASICPixelNumbers(int half);

      void powerUpSingleAsic(int asic, bool program = false);
      void powerUpAsic(int asic, bool program = false);

      // single ASIC mode can be used with or without Ladder readout, if just one ASIC is configured
      // in multi module systems it is true if pixel registers have just one ASIC 4096 entries
      virtual bool isSingleAsicMode() const {return true;}

      virtual bool isLadderReadout() = 0;
      virtual void setLadderReadout(bool enable) = 0;
      void checkLadderReadout();

      bool isSingleSHCapMode() const {
        return sequencer->isSingleSHCapMode();
      }

      void setSingleSHCapMode(bool en){
        if(sequencer->isSingleSHCapMode() != en){
          sequencer->setSingleSHCapMode(en);
          sequencer->generateSignals();
        }
      }

      inline uint32_t getExpXors(uint16_t asic=0) const{
        if(getNumberOfActiveAsics()==1){
          return expectedRegisterXors[0];
        }
        if(asic>=expectedRegisterXors.size()){
          std::cout << "CHIPInterface ERROR: getExpXors asic number out of range:" << asic <<"/"<< expectedRegisterXors.size() << std::endl;
          return expectedRegisterXors[0];
        }
        return expectedRegisterXors[asic];
      }

      virtual bool isDataXorCorrect() const { return true; }
      virtual uint32_t getDataXor() const {return expectedRegisterXors.front();}

      std::string getXorStr(bool readNotExpected) const {
        const uint32_t xorValue = readNotExpected? getDataXor() : getExpXors();
        return utils::intToBitString(xorValue,21);
      }


      inline double baseLineValue(uint32_t pixel) const {
        return baselineValues[pixel];
      }

      void showTrimmingStats();

      int c_pxsPerCol;
      int c_pxsPerRow;
      int singlePxPowered;

      bool fullFReadout;
      bool windowReadout;

      static bool debugMode;
      static bool emitReadoutDone;

      bool m_errorCode;
      bool d0Mode;
      bool d0BypCompr;
      bool d0ModeRem;
      bool d0BypComprRem;
      bool recalCurrCompOnPowerUp;
      bool sweepParamsOnly;
      bool m_injectionEnabled;

      std::vector<uint32_t> expectedRegisterXors;

      std::string fpgaBoardType;
      static std::string chipName;
      std::string activeComponentId;

      int getRanking(uint16_t value, int pixel, bool check = false);
      const std::vector<double> & getFinalSlopes() const  {return finalSlopes;}
      const std::vector<double> & getSramDriftValues() const {return sramDrift;}
      const std::vector<double> & getMeanSramContentValues() const  {return meanSramContent;}
      const std::vector<double> & getCurvatureValues() const {return curvatureValues;}
      const utils::DataHisto::DNLEvalVec & getDNLEvalsVector() const {return dnlEvalsVec;}
      void setFinalSlopes(const std::vector<double> & slopes) {finalSlopes = slopes;}
      void setCurvatureValues(const std::vector<double> & values) {curvatureValues = values;}
      void setSramBlacklistForTrimming(const std::string &blacklistFileName);

      std::vector<double> finalSlopes;
      std::vector<double> sramDrift;
      std::vector<double> meanSramContent;
      std::vector<double> curvatureValues;
      utils::DataHisto::DNLEvalVec dnlEvalsVec;

      // offset if emptyInjectCycles are set in the sequencer, call
      // findAndSetInjectionEventOffset() to set is
      int m_injectionEventOffset;

      void setUpperVHoldVal(uint16_t vhold, int px);
      std::vector<uint16_t> upperVHoldVals;

      std::vector<uint16_t> currentVholdVals;
      std::vector<bool> pixelCurrCompCalibratedVec;
      std::vector<double> baselineValues;
      bool baselineValuesValid;

      DECCAPSETTING globalDeCCAPSetting;

      std::vector<bool> globalFCSR0Vec;
      std::vector<bool> globalFCSR1Vec;

      static std::mutex pixelDataMutex;

      std::vector<std::string> trimmingStats;

    public:
      CHIPFullConfig *fullChipConfig;

      ConfigReg *jtagRegisters;
      Sequencer *sequencer;
      ConfigReg *pixelRegisters;

      utils::DsscProgressBar m_progressBar;

    protected:
      uint16_t sendingAsics;
      uint16_t activeAsics;

      void updateMemberVectorSizes();
      void initChipDataParams(int pixel, std::vector<std::string> &paramNames, std::vector<int> & params);

      uint32_t xorMask;

      std::vector<std::string> trimmingChipParts;

      utils::DsscSramBlacklist m_sramBlacklist;

   public:
      // Guarantee switched off continuous mode during existance of an object
      class ContModeKeeper{
        public:
          ContModeKeeper(CHIPInterface *_chipIf, bool enableSwitch = true)
          : chipIf(_chipIf),
            wasContMode(chipIf->inContinuousMode()),
            enableSwitch(enableSwitch)
          {
            if(wasContMode && enableSwitch)
              chipIf->runContinuousMode(false);
          }

          ~ContModeKeeper()
          {
            if(wasContMode && enableSwitch) chipIf->runContinuousMode(true);
          }

        private:
          CHIPInterface *chipIf;
          bool wasContMode;
          bool enableSwitch;
        };

      class ROModeKeeper{
        public:
          ROModeKeeper(bool newFullF, bool newSinglePix, bool newWindowMode, int newFramesToSend, int newBurstVetoOffset, CHIPInterface *_chip)
          : chip(_chip),
            fullFModeRem(chip->fullFReadout),
            singlePixelModeRem(chip->getSinglePixelReadoutMode()),
            windowModeRem(chip->windowReadout),
            numWordsRem(chip->getNumWordsToReceive()),
            burstVetoOffsetRem(chip->getBurstVetoOffset())
          {
            chip->setFullFReadoutMode(newFullF);
            chip->setSinglePixelReadoutMode(newSinglePix);
            chip->setWindowReadoutMode(newWindowMode);
            chip->setBurstVetoOffset(newBurstVetoOffset);
            chip->setNumFramesToSend(newFramesToSend, false);
            //chip->setNumWordsToReceive(newWordsToReceive);
            chip->writeFpgaRegisters();
          }

          ~ROModeKeeper()
          {
            if (!singlePixelModeRem) // otherwise numWordsToReceiveNotSinglePxMode is overwritten with sramSize
              chip->setNumWordsToReceive(numWordsRem);
            chip->setBurstVetoOffset(burstVetoOffsetRem);
            chip->setFullFReadoutMode(fullFModeRem);
            chip->setWindowReadoutMode(windowModeRem);
            chip->setSinglePixelReadoutMode(singlePixelModeRem);
            chip->writeFpgaRegisters();
          }

        private:
          CHIPInterface *chip;
          bool fullFModeRem,singlePixelModeRem,windowModeRem;
          int numWordsRem,burstVetoOffsetRem;
        };

      // Guarantee switched off vetos and single SHCapMode during existance of object
      class VoldMeasurementKeeper{
        public:
          VoldMeasurementKeeper(CHIPInterface *chip)
           : chip(chip),
             burstVetoOffset(chip->getBurstVetoOffset()),
             singleSHCapMode(chip->sequencer->isSingleSHCapMode())
          {
            chip->setBurstVetoOffset(0);
            chip->setSingleSHCapMode(false);
          }

          ~VoldMeasurementKeeper(){
            chip->setBurstVetoOffset(burstVetoOffset);
            chip->setSingleSHCapMode(singleSHCapMode);
          }
        private:
          CHIPInterface *chip;
          int burstVetoOffset;
          bool singleSHCapMode;
      };


      class InjectionModeKeeper{
        public:
          InjectionModeKeeper(CHIPInterface *_chip, InjectionMode _mode, bool pwrUp = false)
           : chip(_chip),
             mode(chip->getInjectionMode())
          {
            chip->setInjectionMode(_mode);

            if(pwrUp){
              chip->powerUpPixels("all");
            }
          }

          ~InjectionModeKeeper()
          {
            chip->setInjectionMode(mode);
          }

        private:
          CHIPInterface *chip;
          InjectionMode mode;
      };

      class CalibIrampConfigurationKeeper{
        public:
          CalibIrampConfigurationKeeper(CHIPInterface *_chip, bool pwrUp = false, InjectionMode _mode = ADC_INJ)
          : chip(_chip),
            seqKeeper(new Sequencer::ConfigKeeper(chip->sequencer,(_mode == ADC_INJ) ? Sequencer::BUFFER : Sequencer::NORM )), // does not program sequencer
            extRefRem( chip->getPixelRegisterValue( "all", "FCF_SelExtRef")),
            fcfCapRem( chip->getPixelRegisterValue( "all", "FCF_EnCap")),
            enDay0Rem( chip->getPixelRegisterValue( "all", "EnD0")),
            BypComRem( chip->getPixelRegisterValue( "all", "D0_BypComprResistor")),
            gccStaRem( chip->jtagRegisters->getSignalValue("Global Control Register","all","GCC_StartVal_0")),
            lowRanRem( chip->jtagRegisters->getSignalValue("Global Control Register","all","VDAC_lowrange")),
            higRanRem( chip->jtagRegisters->getSignalValue("Global Control Register","all","VDAC_highrange")),
            d0Mode(chip->getD0Mode())
          {
            chip->programSequencer();

            chip->setD0Mode(false);

            if(pwrUp){
              chip->powerUpPixels("all");
            }

            chip->setInjectionMode(_mode);
            //chip->setPixelRegisterValue( "all", "ADC_EnExtLatch", 0);
            //chip->setPixelRegisterValue( "all", "FCF_SelExtRef",1);
            //chip->setPixelRegisterValue( "all", "BypassFE", 0);
            //chip->setPixelRegisterValue( "all", "EnD0",0);
            //chip->setPixelRegisterValue( "all", "D0_BypComprResistor",0);
            //chip->setPixelRegisterValue( "all", "FCF_EnCap",15);

            //chip->jtagRegisters->setSignalValue("Global Control Register","all","VDAC_lowrange",0);
            //chip->jtagRegisters->setSignalValue("Global Control Register","all","VDAC_highrange",1);
            //chip->setGGCStartValue(DEFAULTGCCSTART); // programs global control register

            //chip->programPixelRegs();
          }

          ~CalibIrampConfigurationKeeper()
          {
            //chip->setD0Mode(d0Mode);

            //chip->setPixelRegisterValue( "all", "FCF_SelExtRef",extRefRem);
            //chip->setPixelRegisterValue( "all", "BypassFE", 0);
            //chip->setPixelRegisterValue( "all", "EnD0",enDay0Rem);
            //chip->setPixelRegisterValue( "all", "D0_BypComprResistor",BypComRem);
            //chip->setPixelRegisterValue( "all", "FCF_EnCap",fcfCapRem);

            //chip->jtagRegisters->setSignalValue("Global Control Register","all","VDAC_lowrange",lowRanRem);
            //chip->jtagRegisters->setSignalValue("Global Control Register","all","VDAC_highrange",higRanRem);
            //chip->setGGCStartValue(gccStaRem); // programs global control register

            //chip->programPixelRegs();

            //delete seqKeeper;
            //chip->programSequencer();
          }

        public:
          CHIPInterface *chip;
          Sequencer::ConfigKeeper *seqKeeper;

          int extRefRem,fcfCapRem,enDay0Rem,BypComRem;
          int gccStaRem,lowRanRem,higRanRem;
          bool d0Mode;
        };


    protected:
      int singlePixelToReadout;
      bool singlePixelReadoutMode;

      bool setBothDacsForPixelInjectionVal;

      class IntegrationRemoveKeeper
      {
        public:
          IntegrationRemoveKeeper(CHIPInterface *_chip, int time)
            : chip(_chip),
              intTime(chip->sequencer->getIntegrationTime()),
              mode(chip->sequencer->getOpMode())
          {
            if(time==0){
              chip->sequencer->setOpMode(Sequencer::RESET,true);
            }else{
              chip->sequencer->setIntegrationTime(time);
              chip->sequencer->generateSignals();
            }
            chip->programSequencer();

            usleep(200000);
          }

          ~IntegrationRemoveKeeper(){
            chip->sequencer->setIntegrationTime(intTime);
            chip->sequencer->setOpMode(mode,true);
            chip->programSequencer();
          }

        private:
          CHIPInterface *chip;
          int intTime;
          Sequencer::OpMode mode;
      };

  };

}


#endif
