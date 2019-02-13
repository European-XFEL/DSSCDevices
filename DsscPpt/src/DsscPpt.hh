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
#include "DsscConfigHashWriter.hh"

/**
 * The main Karabo namespace
 */
//#define STATE_RUN karabo::util::State::STARTED,karabo::util::State::ACQUIRING
//#define STATE_ON karabo::util::State::ON,karabo::util::State::STOPPED
//#define STATE_INIT karabo::util::State::OFF
//#define STATE_OFF karabo::util::State::UNKNOWN

#define HDF5

#ifdef HDF5
#include "DsscHDF5Writer.h"
#endif

/**
 * States: UNKNOWN -> connect -> OFF -> progIOB/initSystem ->
 *         STOPPED -> xfelMode/ContMode -> ON ->
 *         sendData -> ACQUIRING
 */
namespace karabo {

    class SmartMutex : public boost::mutex{
    public:
        using boost::mutex::mutex;

        void unlock(){
          m_origin = "";
          boost::mutex::unlock();
        }
        void trylock(const std::string & info){
          if(!try_lock()){
            std::cout << "---- SmarMutex could not lock mutex at " << info << ". Has been reserved by " << m_origin << std::endl;
          }else{
           // this->lock();
          }
          m_origin = info;
          //std::cout << "---- SmarMutex locked mutex at " << info << "." << std::endl;
        }
        std::string m_origin;
    };

    class DsscScopedLock{
    public:
        DsscScopedLock(SmartMutex * mutex, const std::string & info = "")
         : m_mutex(mutex)
         {
           m_mutex->trylock(info);
         }

        ~DsscScopedLock(){
          m_mutex->unlock();
          //std::cout << "---- SmarMutex unlocked mutex at " << m_mutex->m_origin << "." << std::endl;
        }
    private:
        SmartMutex * m_mutex;
    };

    /** DSSC Patch Panel Transceiver C++ Karabo device
     */
    class DsscPpt : public karabo::core::Device<> {

    public:

        // Add reflection and version information to this class
        KARABO_CLASSINFO(DsscPpt, "DsscPpt", "2.4")
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

        void test1();


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
        void programSequencer();
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
        void setPetraIIISetup();
        void setQuadrantSetup();

        void idleStateOnEntry();
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

        void updateEPCConfigSchema( const std::string & configFileName );
        void updateIOBConfigSchema( const std::string & configFileName );
        void updateJTAGConfigSchema( const std::string & configFileName );
        void updatePixelConfigSchema( const std::string & configFileName );
        void updateSeqConfigSchema( const std::string & configFileName );

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
        void storeFullConfigHDF5();

        void saveConfiguration();
        void saveEPCRegisters();
        void saveConfigIOB();
        void saveJTAGRegisters();
        void savePixelRegisters();
        void saveSequencer();

        void pptSendFile(const std::string & fileName);

        bool isProgramState(bool withAcquiring = false);

        void generateAllConfigRegElements();

        void generateConfigRegElements(karabo::util::Schema &schema, SuS::ConfigReg * reg, std::string regName, std::string tagName, std::string moduleStr = "0");

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
        void writeFullConfigHashOut();



        class ContModeKeeper{
          public:
            ContModeKeeper(DsscPpt *ppt) : dsscPpt(ppt),lastState(ppt->getState()){
              if(lastState == util::State::ACQUIRING || lastState == util::State::STARTED)
              {
                if(lastState == util::State::ACQUIRING){
                  dsscPpt->runAcquisition(false);
                }
                dsscPpt->runContMode(false);
              }
              dsscPpt->updateState(util::State::CHANGING);
            }


            ~ContModeKeeper(){
              if(lastState == util::State::ACQUIRING || lastState == util::State::STARTED)
              {
                dsscPpt->runContMode(true);
                if(lastState == util::State::ACQUIRING){
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
        bool m_keepAcquisition;
        bool m_keepPolling;
        boost::shared_ptr<boost::thread> m_pollThread;
        boost::shared_ptr<boost::thread> m_acquisitionThread;
        SmartMutex                       m_accessToPptMutex;
        boost::mutex                     m_outMutex;
        PPT_Pointer                      m_ppt;   // Use your main PPT class here
        karabo::util::Schema             m_schema;
        std::string epcTag;
        std::string ethTag;

        std::string iob_CurrIOBNumber;
        std::string jtag_CurrIOBNumber;
        std::string pixel_CurrIOBNumber;

        bool m_lastTrainIdPolling = false;
        DsscH5ConfigToSchema m_dsscConfigtoSchema;

    };
}

#endif
