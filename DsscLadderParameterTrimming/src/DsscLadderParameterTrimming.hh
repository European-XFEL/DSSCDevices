/*
 * Author: <kirchgessner>
 *
 * Created on February, 2018, 05:00 PM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_DSSCLADDERPARAMETERTRIMMING_HH
#define KARABO_DSSCLADDERPARAMETERTRIMMING_HH

 
#include <karabo/karabo.hpp>

#include "CHIPTrimmer.h"
#include "DsscHDF5MeasurementInfoWriter.h"
#include "DsscHDF5CalibrationDataGenerator.h"
/**
 * The main Karabo namespace
 */

namespace SuS{
    class DsscTrimPptAPI;
}

class DataPacker;

namespace karabo {

    class DsscLadderParameterTrimming : public karabo::core::Device<>{

    public:
        
        typedef boost::shared_ptr<SuS::DsscTrimPptAPI> TrimPPT_API_Pointer;

        // Add reflection information and Karabo framework compatibility to this class
        KARABO_CLASSINFO(DsscLadderParameterTrimming, "DsscLadderParameterTrimming", "3.1")

        /**
         * Necessary method as part of the factory/configuration system
         * @param expected Will contain a description of expected parameters for this device
         */
        static void expectedParameters(karabo::util::Schema& expected);

        /**
         * Constructor providing the initial configuration in form of a Hash object.
         * If this class is constructed using the configuration system the Hash object will
         * already be validated using the information of the expectedParameters function.
         * The configuration is provided in a key/value fashion.
         */
        DsscLadderParameterTrimming(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed
         */
        virtual ~DsscLadderParameterTrimming();

        /**
         * This function acts as a hook and is called after an reconfiguration request was received,
         * but BEFORE this reconfiguration request is actually merged into this device's state.
         *
         * The reconfiguration information is contained in the Hash object provided as an argument.
         * You have a chance to change the content of this Hash before it is merged into the device's current state.
         *
         * NOTE: (a) The incomingReconfiguration was validated before
         *       (b) If you do not need to handle the reconfigured data, there is no need to implement this function.
         *           The reconfiguration will automatically be applied to the current state.
         * @param incomingReconfiguration The reconfiguration information as was triggered externally
         */
        virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration);
        virtual void preReconfigureGeneral(karabo::util::Hash& incomingReconfiguration);
        virtual void preReconfigureMeanReceiver(karabo::util::Hash & incomingReconfiguration);
        virtual void preReconfigureTrimming(karabo::util::Hash & incomingReconfiguration);
        virtual void preReconfigureHelper(karabo::util::Hash & incomingReconfiguration);

        /**
         * This function acts as a hook and is called after an reconfiguration request was received,
         * and AFTER this reconfiguration request got merged into this device's current state.
         * You may access any (updated or not) parameters using the usual getters and setters.
         * @code
         * int i = get<int>("myParam");
         * @endcode
         */
        virtual void postReconfigure();


        void loadCoarseGainParamsIntoGui();
        void displayInjectionSweep(const std::vector<std::vector<double>> &binValues, const std::vector<unsigned int> & xValues, const std::vector<uint32_t> & measurePixels); // override;
        void displayDataHistos(const utils::DataHistoVec & dataHistoVec, const std::vector<uint32_t> & measurePixels);// override;

    private:

        void initialization();
        void initInjectionModeField();
        void initPixelSortMap();
        void initDataWriter();
        void updateModuleInfo();
        void updateSendingAsics(int moduleNr);
        void updateBaselineValid();

        util::Hash createInputChannelConfig(const std::vector<std::string> & outputChannels, const std::string & onSlowness = "drop");

        // implementation of pure virtual functions of CHIPInterface
    private:
        // Readout Function with Data Receiver Environment

        inline double getPixelMeanValue(int imagePixel) { // gets imagePixel
            //return m_asicMeanValues[m_meanSortMap[imagePixel]];
            return m_asicMeanValues[imagePixel];
        }

        bool waitDataReceived();
        void initDataAcquisition();
        
        bool checkPPTInputConnected();

        bool checkIOBDataFailed();
        
        void acquireDisplayData();
        void acquireImage();
        void acquireHistogram();

        std::vector<uint32_t> getPixelsToChange();

        void importBinningInformationFile();
        void importADCGainMapFile();
        void importSpektrumFitResultsFile();
        void computeCalibratedADCSettings();
        void setGainConfigurationFromFitResultsFile();
        
        void measureMeanSramContent();
        void measureMeanSramContentAllPix();
       
    public:
        
        bool inContinuousMode() {
            return dsscPptState() == util::State::ACQUIRING;
        }
        
        int initSystem();// override;
        bool updateAllCounters();// override;
        void updateStartWaitOffset(int value);// override;

        bool doSingleCycle(DataPacker* packer = NULL, bool testPattern = false); //override;
        bool doSingleCycle(int numTries, DataPacker* packer = NULL, bool testPattern = false); //override;
       
        const uint16_t *getPixelSramData(int pixel);// override; // gets imagePixel

        std::vector<double> measureBurstData(const std::vector<uint32_t> & measurePixels, int STARTADDR, int ENDADDR, bool subtract); // override;

        std::vector<double> measureRMSData(const std::vector<uint32_t> & measurePixels, const int STARTADDR, const int ENDADDR);// override;

        // configuration function with DsscPpt Device
        bool isHardwareReady(); //override;

        bool fillSramAndReadout(uint16_t pattern, bool init, bool jtagMode = false); //override;
        
        bool fastInitChip(); //override;
        void initChip(); //override;

        bool getContentFromDevice(uint32_t bitStreamLength, std::vector<bool> &data_vec); //override;

        bool programJtag(bool readBack = false, bool setJtagEngineBusy = true, bool recalcXors = true); //override;
        bool programJtagSingle(const std::string & moduleSetName, bool readBack = false, bool setJtagEngineBusy = true, bool recalcXors = true, bool overwrite = false); //override;
        bool programPixelRegs(bool readBack = false, bool setJtagEngineBusy = true); //override;
        void programPixelRegDirectly(int px, bool setJtagEngineBusy = true); //override;
        bool programSequencer(bool readBack = false, bool setJtagEngineBusy = true, bool program = true); //override;

        /*void programEPCRegister(const std::string & moduleSet) override {
        }

        virtual void resetChip() override {
        }//*/

        void setBurstVetoOffset(int val); //override;
        int getBurstVetoOffset(); //override;

        void setNumFramesToSend(int val, bool saveOldVal = true); //override;
        void setNumWordsToReceive(int val, bool saveOldVal = true); //override;

        void runContinuousMode(bool run); //override;
        
        void setRoSerIn(bool bit); //override;

        void setSendRawData(bool enable, bool reordered = false, bool converted = false); //override;

        void waitJTAGEngineDone(); //override;

        /*int runTestPatternAcquisition(uint16_t _testPattern) override {
            return 0;
        }

        bool isLadderReadout() override {
            return true;
        }

        void setLadderReadout(bool enable) override {
        }//*/

        void loadPxInjCalibData(SuS::CHIPTrimmer * trimmer);

        int getNumberOfActiveAsics() const; // override;
        int getNumberOfSendingAsics() const; // override;
        int getActiveASICToReadout() const; // override;

        //functions without functionality

        void resetDataReceiver();// override;        
        
        /*void testJtagInChain(bool inChain) override {
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
        }//*/

        void setSendingAsics(uint16_t asics); //override;
        void setActiveModule(int modNumber); //override;
        
        // is this function required?
        void sramTest(int iterations, bool init = false); // override;
        
    private:

        void sendBurstParams();

        void setSelectedGainConfig();
        void setCoarseGainSettings();

        void setSelectedInjectionMode();
        void setSelectedInjectionMode(const std::string & nextMode);

        void measureBurstOffsetSweep();
        void measureInjectionSweepSlopes();
        void measureBinningInformation();
        void measureADCGainMap();
        void generateSramBlacklist();

        void displayBinValuesOfPixel();

        // device slots
    public:

        void test();
        void runPixelDelayTrimming();
        void runGainTrimming();
        void measureBurstData();
        void setBaseline();
        void clearBaseline();

        void doSingleCycle2() {
            doSingleCycle();
        }

        void enableInjectionInSelPixels();
        void powerUpSelPixels();
        void addValueToPxRegSignal();
        void enableMonBusInCols();

        // device own functions
    private:

        void onMeanData(const util::Hash& data,
                        const xms::InputChannel::MetaData& meta);
        void onPixelData(const util::Hash& data,
                         const xms::InputChannel::MetaData& meta);

        void saveDataVector(const std::string & outputName, const std::vector<double> & dataVector);
        void saveDataHisto(const std::string & outputName, const utils::DataHisto & dataHisto, double scaleFactor = 1.0);
        void saveGainTrimmingOutputs();

        void showLadderImage(const std::vector<double> & values);
        void showLadderHisto(const std::vector<double> & values);

        static util::Hash getModuleSetHash(SuS::ConfigReg *, const std::string & moduleSetName);
        static util::Hash getSequencerHash(SuS::Sequencer *seq);

        bool allDevicesAvailable();
        bool isPPTDeviceAvailable();
        bool allReceiverDevicesAvailable();

        void enablePixelValueAcquisition();
        void enableMeanValueAcquisition(bool showRms);

        bool startTestDataGeneratorInstance();
        bool startDsscDataGeneratorInstance();
        bool startMainProcessorInstance();
        bool startDsscPptInstance();
        void startPptDevice();
        void triggerPptInitFunction(util::State originState, const std::string & function, int timeout = 3, int TRY_CNT = 3);
        void fillSramAndReadoutPattern();

        void computeTargetGainADCConfiguration();

        void setMinSram(unsigned int value);
        void setMaxSram(unsigned int value);

        void initTrimming();
        void stopTrimming();

        void setBufferMode();

        unsigned int getLastValidTrainId();
        util::State dsscPptState();
        util::State deviceState(const std::string & deviceId);

        bool isDeviceExisting(const std::string & deviceId) {
            return (m_deviceInitialized ? remote().exists(deviceId).first : false);
        }

        bool isTestData();
        bool isDsscData();
        bool isXFELData();

    private: // members

        enum RecvStatus {

            OK = 0, TESTPATTERN = 1, TIMEOUT = 2, DROP = 3
        };

        enum RecvMode {

            RAW = 0, PIXEL = 1, MEAN = 2, RMS = 3, NO = 4
        };

        unsigned long long m_lastTrainId;

        std::map<std::string, int> m_burstParams;
        std::string m_quadrantId;
        std::string m_quadrantServerId;
        std::string m_pptDeviceId;


        std::string m_recvServerId;
        std::string m_testDataGeneratorId;
        std::string m_dsscDataReceiverId;
        std::string m_mainProcessorId;

        unsigned int m_lastPptTrainId;
        RecvStatus m_recvStatus;
        RecvMode m_recvMode;

        bool m_asicChannelDataValid;
        bool m_startTrimming;
        bool m_runFastAcquisition;


        std::vector<std::vector<double>> m_binValues;

        std::vector<double> m_asicMeanValues;
        std::vector<uint16_t> m_pixelData;
        std::vector<uint32_t> m_pixelSortMap;

        SuS::CHIPTrimmer getNewChipTrimmer(bool &ok);

        SuS::CHIPTrimmer *m_currentTrimmer;

        void initSaveRawDataParams();
        void finishSaveRawDataParams();

        void updateActiveModule(int newActiveModule);

        bool m_setColumns;
        bool m_saveRawData;

        uint32_t m_runSettingIdx;
        std::string m_runDirectory;
        std::string m_runBaseDirectory;
        std::vector<std::string> m_runDirectoryVec;
        std::vector<uint32_t> m_runSettingsVec;

        DsscHDF5MeasurementInfoWriter::MeasurementConfig m_currentMeasurementConfig;
        DsscHDF5CalibrationDataGenerator m_calibGenerator;

        bool m_deviceInitialized;

        void changeDeviceState(const util::State & newState);

        bool matrixSRAMTest(int patternID, int &errCnt);
        bool matrixSRAMTest();

        void setNumIterations(uint iterations);
        
        TrimPPT_API_Pointer m_trimppt_api;

        class StateChangeKeeper {

        public:

            StateChangeKeeper(core::Device<> *device) : m_dev(device), m_lastState(m_dev->getState()) {
                try {
                    m_dev->updateState(util::State::CHANGING);
                    m_dev->set<std::string>("status", "Measuring");
                } catch (...) {
                    std::cout << "DsscLadderParameterTrimming.h : Could not change device state!!!" << std::endl; // sometimes state is not existing
                }
            }

            StateChangeKeeper(core::Device<> *device, const util::State & afterState) : m_dev(device), m_lastState(afterState) {
                try {
                    m_dev->updateState(util::State::CHANGING);
                    m_dev->set<std::string>("status", "Measuring");
                } catch (...) {
                    std::cout << "DsscLadderParameterTrimming.h : Could not change device state!!!" << std::endl; // sometimes state is not existing
                }
            }

            ~StateChangeKeeper() {
                try {
                    m_dev->updateState(m_lastState);
                } catch (...) {
                    std::cout << "DsscLadderParameterTrimming.h : Could not change device state!!!" << std::endl; // sometimes state is not existing
                }
            }

            void change(const util::State & newState) {
                m_lastState = newState;
            }

        private:
            core::Device<> *m_dev;
            util::State m_lastState;
        };

        enum ConfigState {

            VALID = 0, CHANGED = 1, CHANGING = 2
        };

        ConfigState m_deviceConfigState;

        class ConfigStateKeeper {

        public:

            ConfigStateKeeper(ConfigState & configState) : m_activeState(configState), m_lastState(configState) {
                m_activeState = ConfigState::CHANGING;
            }

            ~ConfigStateKeeper() {
                if (m_lastState != ConfigState::CHANGING) {
                    m_activeState = ConfigState::CHANGED;
                }
            }

        private:
            ConfigState & m_activeState;
            ConfigState m_lastState;
        };
    };
}

#endif
