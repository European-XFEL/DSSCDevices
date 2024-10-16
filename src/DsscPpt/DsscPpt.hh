/*
 * $Id: DsscPpt.hh 13090 2014-03-07 12:12:04Z heisenb $
 *
 * Author: EuXFEL WP76
 *
 * Created on March, 2014, 04:06 PM
 *
 * Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_DSSCPPT_HH
#define KARABO_DSSCPPT_HH

#include <karabo/karabo.hpp>

#include "DsscPptAPI.hh"
#include "DsscRegisterConfiguration.hh"
#include "DsscConfigHashWriter.hh"

#include <atomic>
#include <vector>


/**
 * The main Karabo namespace
 */
//#define STATE_RUN karabo::util::State::STARTED,karabo::util::State::ACQUIRING
//#define STATE_ON karabo::util::State::ON,karabo::util::State::STOPPED
//#define STATE_INIT karabo::util::State::OFF
//#define STATE_OFF karabo::util::State::UNKNOWN

#include "../version.hh"  // provides PACKAGE_VERSION common to all devices

/**
 * States: UNKNOWN -> connect -> OFF -> progIOB/initSystem ->
 *         STOPPED -> xfelMode/ContMode -> ON ->
 *         sendData -> ACQUIRING
 */



namespace karabo {

   std::vector<std::string> tocamelCase(const std::vector<std::string> vector) {
        /* Change the PascalCase to camelCase for sequencer parameters. */
        std::vector<std::string> camelCased;
        for (auto s: vector) {
            s[0] = std::tolower(s.at(0));
            camelCased.push_back(s);
        }
        return camelCased;
   }
        
    class SmartMutex : public boost::mutex {

    public:
        using boost::mutex::mutex;

        void unlock() {
            m_origin = "";
            boost::mutex::unlock();         
        }

        void trylock(const std::string & info) {
            std::chrono::milliseconds interval(100);
            int ncounts = 0;
            while(!try_lock()){
                std::this_thread::sleep_for(interval);
                ++ncounts;
                if(ncounts>50){
                    std::cout << "---- SmarMutex could not lock mutex during 5 sec at " << info << ". Has been reserved by " << m_origin << std::endl;
                    ncounts = 0;
                }
            }
            m_origin = info;
        }
        std::string m_origin;
    };

    class DsscScopedLock {

    public:

        DsscScopedLock(SmartMutex * mutex, const std::string & info = "")
            : m_mutex(mutex) {
            m_mutex->trylock(info);
        }

        ~DsscScopedLock() {
            m_mutex->unlock();
        }

    private:
        SmartMutex * m_mutex;
    };

    /** DSSC Patch Panel Transceiver C++ Karabo device
     */
    class DsscPpt : public karabo::core::Device<> {

    public:

        // Add reflection and version information to this class
        KARABO_CLASSINFO(DsscPpt, "DsscPpt", PACKAGE_VERSION)
        typedef boost::shared_ptr<SuS::DSSC_PPT_API> PPT_Pointer;
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
        DsscPpt(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed (i.e. the event-loop returns)
         */
        virtual ~DsscPpt();


        void initialize();

        const static std::string s_dsscConfBaseNode;


    private: // State-machine call-backs (override)

        void receiveRegisterConfiguration(const util::Hash& data,
                                          const xms::InputChannel::MetaData& meta);

        void receiveSequencerConfig(const util::Hash& data);
        void receiveBurstParams(const util::Hash& data);
        void receiveConfigRegister(const util::Hash& data);

        void readPLLStatus();
        void programPLL();
        void programPLLFine();

        void setLogoConfig(bool en);
        bool checkIOBDataFailed();
        bool checkPPTDataFailed();
        void checkASICReset();

        void readLastPPTTrainID();

        bool checkConfigFilePaths(const util::Hash& config);

        int checkAllIOBStatus();
        bool checkIOBReady(int iobNumber);
        bool checkIOBVoltageEnabled(int iobNumber);
        void checkIOBAuroraReady(int iobNumber);
        bool isIOBAvailable(int iobNumber);

        void programEPCConfig();

        void readEPCRegisters();
        void readEPCPLLRegisters();

        void readIOBRegisters();
        void readIOBRegisters(int iobNumber);

        void fillSramAndReadout();

        void startPolling();
        void stopPolling();

        void open();
        void close();
        void stopAcquisition();
        void startAcquisition();
        void startBurstAcquisition();
        void runStandAlone();
        void stopStandalone();
        void runXFEL();

        void start();
        void stop();

        void runAcquisition(bool run);
        void runContMode(bool run);

        void doFastInit();
        void initSystem();
        void initSystem_impl();
        void initSingleModule();
        void initGui();
        bool initIOBs();
        bool initChip();

        void setInjectionValue();
        void setPixelInjectionMode();
        void setNormalMode();
        void setIntDACMode();
        void setInjectionMode();

        void resetAll();
        void resetAllBtn();
        void resetIOBs();
        void resetEPC();
        void resetDatapath();
        void resetASICs();

        void programAllIOBFPGAs(); //FPGA
        void programIOBFPGA(int iobNumber);
        void programIOB1FPGA();
        void programIOB2FPGA();
        void programIOB3FPGA();
        void programIOB4FPGA();

        void resetAurora(int iobNumber);
        void checkPRBs(int iobNumber);
        void resetAurora1();
        void resetAurora2();
        void resetAurora3();
        void resetAurora4();

        void preProgSelReg();
        void progSelReg();
        void readSelReg();
        bool setActiveModule(int iobNumber);

        void programActiveIOB();
        void programActiveJTAG();
        void programActivePixelregisters();

        void programAvailableIOBsConfig();
        void programIOBConfig(int iobNumber);
        void programIOB1Config();
        void programIOB2Config();
        void programIOB3Config();
        void programIOB4Config();

        void programJTAG();
        void programPixelRegister();
        void programPixelRegisterDefault();
        void programSequencers(); 
        void updateSequencer();

        bool readbackConfigIOB(int iobNumber);
        void readIOBRegisters1();
        void readIOBRegisters2();
        void readIOBRegisters3();
        void readIOBRegisters4();

        void programLMKOutput();
        void programAvailableLMKs();
        void programLMK(int iobNumber);
        void programLMK1();
        void programLMK2();
        void programLMK3();
        void programLMK4();

        void updateFirmwareFlash();
        void updateLinuxFlash();
        void updateIOBFirmware();
        void _updateIOBFirmware();
        void configure();
        void errorFound();

        void storePoweredPixels();
        void restorePoweredPixels();
        void setCurrentQuarterOn();
        void setCurrentColSkipOn();
        std::string getCurrentColSelectString();


        bool checkPathExists(const std::string & filePath);

        void startManualMode();
        void stopManualMode();
        void startManualReadout();
        void startManualBurst();
        void startManualBurstBtn();
        void readoutTestPattern();

        void startSingleCycle();

        void updateNumFramesToSend();
        void setSendingASICs();
        void programLMKsAuto();

        void updateTestEnvironment();
        void updateTestEnvironment(const std::string &environment);
        void readSerialNumber();

        void loadLastFileETHConfig();

        void setBurstParameter();
        void setSequencerParameter();

        void disableAllDummyData();

        void startAllChannelsDummyData();
        void setQuadrantSetup();

        //void idleStateOnEntry();
        void acquisitionStateOnEntry();
        void manualAcquisitionStateOnEntry();
        void acquisitionStateOnExit();

        void preReconfigure(karabo::util::Hash& incomingReconfiguration);
        void preReconfigureEPC(karabo::util::Hash & incomingReconfiguration);
        void preReconfigureETH(karabo::util::Hash & incomingReconfiguration);
        void preReconfigureLoadIOBConfig(karabo::util::Hash & incomingReconfiguration);
        void preReconfigureLoadEPCConfig(karabo::util::Hash & incomingReconfiguration);
        void preReconfigureEnableDatapath(karabo::util::Hash & incomingReconfiguration);
        void preReconfigureEnablePLL(karabo::util::Hash & incomingReconfiguration);
        void preReconfigureEnableOthers(karabo::util::Hash & incomingReconfiguration);
        void preReconfigureEnableMeasurement(karabo::util::Hash & incomingReconfiguration);
        void preReconfigureLoadASICConfig(karabo::util::Hash & incomingReconfiguration);
        void preReconfigurePixel(karabo::util::Hash & incomingReconfiguration);
        void preReconfigureJTAG(karabo::util::Hash & incomingReconfiguration);
        void preReconfigureIOB(karabo::util::Hash & incomingReconfiguration);
        void preReconfigureFullConfig(karabo::util::Hash & incomingReconfiguration);
        void preReconfigureFastInit(karabo::util::Hash & incomingReconfiguration);
        void preReconfigureRegAccess(karabo::util::Hash & incomingReconfiguration);


        void preDestruction();

        void getEPCParamsIntoGui();
        void getEPCParamsIntoGui(const std::string & moduleSetName);
        void getIOBSpecialParamsIntoGui(int iobNumber);
        void getIOBParamsIntoGui(int iobNumber);
        void getIOBParamsIntoGui();
        void getJTAGParamsIntoGui();
        void getPixelParamsIntoGui();
        void getSequencerParamsIntoGui();
        void getEthernetConfigIntoGui();
        void getSequenceCountersIntoGui();
        void getCoarseGainParamsIntoGui();

        void updateGuiPLLParameters();
        void updateGuiOtherParameters();
        void updateGuiMeasurementParameters();
        void updateGuiEnableDatapath();

        void updateEPCConfigSchema(const std::string & configFileName);
        void updateIOBConfigSchema(const std::string & configFileName);
        void updateJTAGConfigSchema(const std::string & configFileName);
        void updatePixelConfigSchema(const std::string & configFileName);
        void updateSeqConfigSchema(const std::string & configFileName);

        void getIOBSerialIntoGui(int iobNumber);
        void getIOBTempIntoGui(int iobNumber);
        void getNumPRBsIntoGui(int iobNumber);
        void setASICChannelReadoutFailure(int iobNumber);
        // 'acquire' thread
        void acquire();
        // 'pollHardware' thread
        void pollHardware();
        void updateGuiRegisters();

        void enableDPChannels(uint16_t enOneHot);

        int anyToInt(const boost::any anyVal, bool &ok);

        void checkQSFPConnected();

        void readFullConfigFile(const std::string & fileName);
        void storeFullConfigFile();
        void storeFullConfigUnder();

        void saveConfiguration();
        void saveEPCRegisters();
        void saveConfigIOB();
        void saveJTAGRegisters();
        void savePixelRegisters();
        void saveSequencer();

        void pptSendFile(const std::string & fileName);

        bool isProgramState(bool withAcquiring = false);

        void generateAllConfigRegElements();

        void generateConfigRegElements(karabo::util::Schema &schema, SuS::ConfigReg * reg,\
                    std::string regName, std::string tagName, std::string rootNode = "");

        std::string getIOBTag(int iobNumber);

        bool printPPTErrorMessages(bool printRBCorrect = false);
        void setQSFPEthernetConfig();

        void setInernalDAC();
        void calibrateCurrentCompDac();
        void calibrateIramp();
        void checkDACCalibration();
        void checkIrampCalibration();

        void updateStartWaitOffset();
        void updateSequenceCounters(); // chip interface cont mode keeper

        void waitJTAGEngineDone();

        void LoadQSFPNetConfig();
        void SaveQSFPNetConfig();

        void setThrottleDivider();
        void updateGainHashValue();  // Karabo slot
        void updateGainHashValue_impl();   // Background task implementation
        void updateConfigSchema();
        void updateConfigHash();  // Karabo slot
        void updateConfigHash_impl();  // Background task implementation
        void updateConfigFromHash();
        void updateDetRegistryGui(SuS::ConfigReg * reg, std::string regName, \
                std::string tagName, std::string rootNode);

        class ContModeKeeper {

        public:

            ContModeKeeper(DsscPpt *ppt) : dsscPpt(ppt), lastState(ppt->getState()) {
                if (lastState == util::State::ACQUIRING || lastState == util::State::STARTED) {
                    if (lastState == util::State::ACQUIRING) {
                        dsscPpt->runAcquisition(false);
                    }
                    dsscPpt->runContMode(false);
                }
                dsscPpt->updateState(util::State::CHANGING);
            }

            ~ContModeKeeper() {
                if (lastState == util::State::ACQUIRING || lastState == util::State::STARTED) {
                    dsscPpt->runContMode(true);
                    if (lastState == util::State::ACQUIRING) {
                        dsscPpt->runAcquisition(true);
                    }
                }
                dsscPpt->updateState(lastState);
            }

        private:
            
            DsscPpt *dsscPpt;
            util::State lastState;
        };

    private:
        
        inline std::string removeSpaces(std::string& p){
              std::string res(p);
              std::replace(res.begin(), res.end(), ' ', '_');
          return res;
        }
        
        bool m_keepAcquisition;
        bool m_keepPolling;
        boost::shared_ptr<boost::thread> m_pollThread;
        SmartMutex m_accessToPptMutex;
        boost::mutex m_outMutex;
        PPT_Pointer m_ppt; // Use your main PPT class here
        karabo::util::Schema m_schema;
        std::string m_epcTag;
        std::string m_ethTag;

        std::string m_iobCurrIOBNumber;
        std::string m_jtagCurrIOBNumber;
        std::string m_pixelCurrIOBNumber;

        DsscConfigToSchema m_dsscConfigtoSchema;

        
        std::atomic<bool> m_burstAcquisition;
        
        karabo::util::Hash m_last_config_hash;
        
        void burstAcquisitionPolling();
        bool getConfigurationFromRemote();
    };
}

#endif
