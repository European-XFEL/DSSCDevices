/*
 * $Id: DsscPpt.cc 13090 2014-03-07 12:12:04Z heisenb $
 *
 * Author: EuXFEL WP76
 *
 * Created on March, 2014, 04:06 PM
 *
 * Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/filesystem.hpp>
#include <boost/assign/std/vector.hpp> // for 'operator+=()'
#include <boost/functional/hash.hpp>
#include <boost/algorithm/string/split.hpp>
#include <chrono>

#include "DsscPpt.hh"
#include "DsscPptRegsInit.hh"
#include "DsscDependencies.h"
#include "CHIPGainConfigurator.h"
#include "ConfigReg.h"
#include "CHIPTrimmer.h"
#include "utils.h"
#include "DsscModuleInfo.h"
#include "DsscConfigHashWriter.hh"
#include "PPTFullConfig.h"

using namespace std;
using namespace karabo::util;
using namespace karabo::log;
using namespace karabo::io;
using namespace karabo::net;
using namespace karabo::xms;
using namespace karabo::core;

// cwd is karabo/var/data/
#define DEFAULTCONF "ConfigFiles/session.conf"

#ifdef F1IO
#define INITIALCONF "ConfigFiles/Init.conf"
#elif F2IO
#define INITIALCONF "ConfigFiles/F2Init.conf"
#endif

#define TESTSYSTEM

#define INT_CAST boost::lexical_cast<int>

#define BOOL_CAST boost::lexical_cast<bool>

#define DEVICE_ERROR(messsage)           \
        KARABO_LOG_ERROR << messsage;    \
        set<string>("status", messsage);  \
        this->updateState(State::ERROR);

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, DsscPpt)

    void DsscPpt::expectedParameters(Schema& expected) {

        cout << "Started Expected Parameters " << endl;

        STRING_ELEMENT(expected).key("selEnvironment")
                .displayedName("Environment")
                .description("Write MANNHEIM or HAMBURG to select test setup environment")
                .tags("other")
                .assignmentOptional().defaultValue("HAMBURG")
                .options("HAMBURG,MANNHEIM", ",")
                .reconfigurable()
                .commit();

        STRING_ELEMENT(expected).key("pptHost")
                .displayedName("PPT Host")
                .description("PPT hostname or IP address")
                .assignmentOptional().defaultValue("192.168.0.125").reconfigurable()
                .allowedStates(karabo::util::State::UNKNOWN)
                .commit();

        UINT32_ELEMENT(expected).key("pptPort")
                .displayedName("PPT Port")
                .description("PPT port number")
                .assignmentOptional().defaultValue(2384).reconfigurable()
                .allowedStates(State::UNKNOWN)
                .commit();

        SLOT_ELEMENT(expected)
                .key("open").displayedName("Connect PPT").description("Open connection to PPT")
                .allowedStates(State::UNKNOWN)
                .commit();

        SLOT_ELEMENT(expected)
                .key("close").displayedName("Disconnect PPT").description("Close connection to PPT")
                .allowedStates(State::ON, State::STOPPED, State::OFF)
                .commit();

        SLOT_ELEMENT(expected)
                .key("startAllChannelsDummyData").displayedName("Start all channels dummy data").description("After connection start dummy data from all channels")
                .allowedStates(State::ON, State::STOPPED, State::OFF)
                .commit();

        SLOT_ELEMENT(expected)
                .key("updateFirmwareFlash")
                .displayedName("Update PPT Firmware")
                .description("Update PPT Firmware Flash - takes about 15 minutes ")
                .allowedStates(State::OFF)
                .commit();

        SLOT_ELEMENT(expected)
                .key("updateLinuxFlash")
                .displayedName("Update PPT Linux")
                .description("Update PPT Linux Flash containing kernel and control software - takes about 10 minutes ")
                .allowedStates(State::OFF)
                .commit();

        SLOT_ELEMENT(expected)
                .key("updateIOBFirmware")
                .displayedName("Update IOB Firmware")
                .description("Update IOB Firmware, override bitfile in PPT with new version - takes about 10 minutes ")
                .allowedStates(State::OFF)
                .commit();

        SLOT_ELEMENT(expected)
                .key("initSystem").displayedName("Init System").description("Full PPT IOB and ASIC Init")
                .allowedStates(State::ON, State::STOPPED, State::OFF)
                .commit();

        SLOT_ELEMENT(expected)
                .key("initSingleModule").displayedName("Init Single Module").description("Full PPT IOB and ASIC Init of selected Module")
                .allowedStates(State::ON, State::STOPPED, State::OFF)
                .commit();

        PATH_ELEMENT(expected).key("fullConfigFileName")
                .description("Path Full Config File")
                .displayedName("Full Config File")
                .isInputFile()
                .tags("FullConfig")
                .assignmentOptional().defaultValue(INITIALCONF).reconfigurable()
                // do not limit states
                .commit();
        
        PATH_ELEMENT(expected).key("saveConfigFileToName")
                .description("Full Config save under")
                .displayedName("Full Config save under")
                .isInputFile()
                .tags("FullConfig")
                .assignmentOptional().defaultValue(INITIALCONF).reconfigurable()
                // do not limit states
                .commit();
                       

        SLOT_ELEMENT(expected)
                .key("updateGuiRegisters").displayedName("Update GUI Registers").description("Releoad all Reigsters")
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::STARTED, State::ACQUIRING)
                .commit();


        SLOT_ELEMENT(expected)
                .key("storeFullConfigFile").displayedName("Save Full Config").description("Store full config file")
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::UNKNOWN, State::STARTED, State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected)
                .key("storeFullConfigUnder").displayedName("Full Config save under").description("Save full config to path")
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::UNKNOWN, State::STARTED, State::ACQUIRING)
                .commit();
        
        SLOT_ELEMENT(expected)
                .key("storeFullConfigHDF5").displayedName("Save HDF5 Config").description("Store Configuration as HDF5")
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::UNKNOWN, State::STARTED, State::ACQUIRING)
                .commit();

        /*SLOT_ELEMENT(expected)
                .key("updateFullConfigHash").displayedName("Update Config Data Hash").description("Update full config data hash")
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::STARTED, State::UNKNOWN)
                .commit();//*/

        SLOT_ELEMENT(expected)
                .key("sendConfigHashOut").displayedName("Send Config Data Hash").description("Send full config data hash through p2p channel")
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::STARTED, State::UNKNOWN)
                .commit();

        PATH_ELEMENT(expected).key("linuxBinaryName")
                .description("Path to linux binary")
                .displayedName("Linux Binary Filename")
                .isInputFile()
                .assignmentOptional().defaultValue("ConfigFiles/simpleImage.xilinx.bin").reconfigurable()
                .commit();

        PATH_ELEMENT(expected).key("firmwareBinaryName")
                .description("Path to ppt fpga firmware binary")
                .displayedName("PPT Firmware Binary Filename")
                .isInputFile()
                .assignmentOptional().defaultValue("ConfigFiles/DSSC_PPT_TOP.bin").reconfigurable()
                .commit();

        PATH_ELEMENT(expected).key("iobFirmwareBitfile")
                .description("Path to iob fpga firmware bitfile")
                .displayedName("IOB Firmware Bitfile Filename")
                .isInputFile()
                .assignmentOptional().defaultValue("ConfigFiles/IOB_Firmware.xsvf").reconfigurable()
                .commit();

        STRING_ELEMENT(expected).key("pptSerial")
                .displayedName("PPT Serial Nr")
                .description("PPT Serial number read from PPT")
                .readOnly()
                .initialValue("connect to PPT")
                .commit();

        STRING_ELEMENT(expected).key("firmwareRev")
                .displayedName("Firmware Built")
                .description("PPT Firmware Built Revision")
                .readOnly()
                .initialValue("nA")
                .commit();

        STRING_ELEMENT(expected).key("linuxRev")
                .displayedName("Linux Built")
                .description("Linux Built Revision")
                .readOnly()
                .initialValue("nA")
                .commit();

        UINT32_ELEMENT(expected).key("pptTemp")
                .displayedName("PPT FPGA Temperature")
                .description("PPT FPGA Temperature")
                .unit(Unit::DEGREE_CELSIUS)
                .readOnly()
                .initialValue(30)
                .warnLow(10).needsAcknowledging(false).alarmLow(5).needsAcknowledging(false)
                .warnHigh(75).needsAcknowledging(false).alarmHigh(80).needsAcknowledging(false)
                .commit();

        STRING_ELEMENT(expected).key("ethOutputRate")
                .displayedName("SFP Output Rate")
                .description("Ouput rate of the QSFP link, measured in MBit/s, averaged over one second")
                .readOnly()
                .initialValue("nA")
                .commit();

        UINT32_ELEMENT(expected).key("initDistance")
                .displayedName("Init Distance")
                .description("Wait cycles after power up digital voltage and start jtag programming")
                .tags("fastInit")
                .assignmentOptional().defaultValue(120).reconfigurable()
                .allowedStates(State::UNKNOWN)
                .commit();

        UINT32_ELEMENT(expected).key("fastInitJtagSpeed")
                .displayedName("Fast Init JTAG Speed")
                .description("JTAG Speed during fast initialization")
                .tags("fastInit")
                .assignmentOptional().defaultValue(14).reconfigurable()
                .allowedStates(State::UNKNOWN)
                .commit();

        SLOT_ELEMENT(expected)
                .key("doFastInit").displayedName("Fast Init").description("Start Fast Init sequence")
                .allowedStates(State::ON, State::STOPPED, State::OFF)
                .commit();

        SLOT_ELEMENT(expected)
                .key("initChip").displayedName("reprogram ASICs").description("Program ASICs without powercycling")
                .allowedStates(State::ON, State::STOPPED, State::OFF)
                .commit();

        SLOT_ELEMENT(expected)
                .key("startManualBurstBtn").displayedName("Start Burst").description("Trigger one single Burst")
                .allowedStates(State::ON, State::STOPPED)
                .commit();

        SLOT_ELEMENT(expected)
                .key("readoutTestPattern").displayedName("Readout Testpattern").description("Trigger full readout with Testpattern")
                .allowedStates(State::ON, State::STOPPED)
                .commit();

        UINT16_ELEMENT(expected).key("sramPattern")
                .displayedName("Sram Pattern")
                .description("Pattern to program into asic via Jtag (9 bit valid)")
                .assignmentOptional().defaultValue(120).reconfigurable()
                .commit();

        SLOT_ELEMENT(expected)
                .key("fillSramAndReadout").displayedName("Fill Sram and Readout").description("Clock in testpattern into ASIc via Jtag and trigger one readout")
                .allowedStates(State::STOPPED, State::ON)
                .commit();

        UINT32_ELEMENT(expected).key("lastTrainId")
                .displayedName("Last Train ID from PPT")
                .description("Holds 4LSB of last train ID from PPT registers. Trigger readout before use")
                .readOnly().initialValue(0)
                .commit();

        SLOT_ELEMENT(expected)
                .key("readLastPPTTrainID").displayedName("read Last PPT Train ID").description("Reads last Train ID from PPT registers")
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::STARTED, State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected)
                .key("startSingleCycle").displayedName("Start Single Cycles").description("Start Single Cylce, module and num cycles should be enabled")
                .allowedStates(State::ON, State::STOPPED, State::OFF)
                .commit();

        NODE_ELEMENT(expected).key("singleCycleFields")
                .displayedName("Single Cycle Fields")
                .commit();

        UINT32_ELEMENT(expected).key("singleCycleFields.moduloValue")
                .displayedName("Modulo Value")
                .description("")
                .assignmentOptional().defaultValue(0).reconfigurable()
                .commit();

        UINT32_ELEMENT(expected).key("singleCycleFields.iterations")
                .displayedName("Number of SingleCycles")
                .description("Number of Single Cycles")
                .assignmentOptional().defaultValue(1000).reconfigurable()
                .commit();


        SLOT_ELEMENT(expected)
                .key("resetAllBtn").displayedName("Reset All").description("Full PPT and IOB Reset")
                .allowedStates(State::ON, State::STOPPED, State::OFF)
                .commit();

        SLOT_ELEMENT(expected)
                .key("resetEPC").displayedName("Reset EPC").description("EPC Reset, reprogramms EPC registers after reset")
                .allowedStates(State::ON, State::STOPPED, State::OFF)
                .commit();

        SLOT_ELEMENT(expected)
                .key("resetDatapath").displayedName("Reset Datapath").description("Datapath reset, use if aurora is not locked")
                .allowedStates(State::ON, State::STOPPED, State::OFF)
                .commit();

        SLOT_ELEMENT(expected)
                .key("resetIOBs").displayedName("Reset IOBoards").description("Full IOB Reset, reprogramms IOB registers after reset")
                .allowedStates(State::ON, State::STOPPED, State::OFF)
                .commit();

        SLOT_ELEMENT(expected)
                .key("resetASICs").displayedName("Reset ASICs").description("Enable Asic and disable after e few milliseconds")
                .allowedStates(State::ON, State::STOPPED, State::OFF)
                .commit();

        SLOT_ELEMENT(expected)
                .key("runXFEL").displayedName("Run XFEL Mode").description("Activate Continuous Acquistione")
                .allowedStates(State::ON)
                .commit();

        SLOT_ELEMENT(expected)
                .key("stopStandalone").displayedName("Stop Running").description("Stop cuntinuous burst operation")
                .allowedStates(State::STARTED)
                .commit();

        SLOT_ELEMENT(expected)
                .key("runStandAlone").displayedName("Run Standalone").description("Activate Continuous Acquisition")
                .allowedStates(State::ON)
                .commit();

        SLOT_ELEMENT(expected)
                .key("stopAcquisition").displayedName("Stop Acquisition").description("Disable continuous data sending")
                .allowedStates(State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected)
                .key("startAcquisition").displayedName("Start Acquisition").description("Enable continuous data sending")
                .allowedStates(State::STARTED)
                .commit();
        
        SLOT_ELEMENT(expected)
                .key("startBurstAcquisition").displayedName("Start Burst Acquisition").description("Send burst of trains")
                .allowedStates(State::ON)
                .commit();
        
        NODE_ELEMENT(expected)
                .key("burstData")
                .displayedName("burstData")
                .description("Burst measurement data")
                .commit();

        UINT64_ELEMENT(expected).key("burstData.startTrainId")
                .displayedName("startTrainId")
                .description("start train of burst measurement")
                .assignmentOptional().defaultValue(0).reconfigurable()
                .commit();
                
        UINT64_ELEMENT(expected).key("burstData.endTrainId")
                .displayedName("endTrainId")
                .description("end train of burst measurement")
                .assignmentOptional().defaultValue(0).reconfigurable()
                .commit();

        UINT32_ELEMENT(expected).key("numBurstTrains")
                .displayedName("Number of Trains")
                .description("Number of trains in the burst")
                .assignmentOptional().defaultValue(10).reconfigurable()
                .minInc(1)
                .commit();

        STRING_ELEMENT(expected).key("selRegName")
                .displayedName("Register")
                .description("Select Register to program")
                .tags("regAccess")
                .assignmentOptional().defaultValue("pixel").reconfigurable()
                .options("pixel,jtag")
                .commit();

        STRING_ELEMENT(expected).key("selPixelSignal")
                .displayedName("Signal")
                .description("Select ModuleSet Signal to program")
                .tags("regAccess")
                .assignmentOptional().defaultValue("FCF_EnCap").reconfigurable()
                .options("LOC_PWRD,MonVP,MonVNEnDecCap,FCF_SelLowVref,FCF_SelExtRef,CSA_IBoost,CSA_DisSatDet,CSA_FbCap,CSA_Resistor,CSA_Cin_200fF,CSA_Href,FCF_EnCap,FCF_EnIntAmpRes,FCF_HDR,QInjEn10fF,InvertInj,EnPxInjDC,InjBusEn,InjPxQ,DepfetSelCascVref,DepfetUnSampVCasc_B,ShortIconRes,EnPxInj,InjHG,IProgEnAllCaps,PxInjEnAmpNMirr,IconSlower,RmpFineTrm,RmpCurrDouble,RmpEnFineDelayCntrl,RmpDelayCntrl,ADC_EnExtLatch,LocSubcToVSSA,QInjEnCs")
                .commit();

        STRING_ELEMENT(expected).key("selJtagMainRegSignal")
                .displayedName("Signal")
                .description("Select ModuleSet Signal to program")
                .tags("regAccess")
                .assignmentOptional().defaultValue("GCC_StartVal_1").reconfigurable()
                .options("PxInj_UseBG_NotHD_DAC,Pixel Injection Signal Trim,Pixel Injection Bias Current Trim,VDAC_lowrange,VDAC_highrange,LVDS_TX_Vref,LVDS_TX_Ibias,LVDS_RX_Ibias,ClkDeskew_1,ClkDeskew_0,GCC_LT,GCC_StartVal_1,GCC_StartVal_0,SC_EnChainLd,2 Phase Gen - SelDelay")
                .commit();

        STRING_ELEMENT(expected).key("selPixels")
                .displayedName("Pixels/ASICs")
                .description("PixelRegs: Select Pixels to program, 0-4095 JtagRegs: Select ASIC to program, both: all to select all")
                .tags("regAccess")
                .assignmentOptional().defaultValue("all").reconfigurable()
                .commit();

        UINT32_ELEMENT(expected).key("selModule")
                .displayedName("Module Number")
                .description("Select Active Module Number 1-4")
                .tags("regAccess")
                .assignmentOptional().defaultValue(1).reconfigurable()
                .minExc(0).maxExc(5)
                .commit();

        UINT32_ELEMENT(expected).key("selValue")
                .displayedName("Value")
                .description("Signal value")
                .tags("regAccess")
                .assignmentOptional().defaultValue(0).reconfigurable()
                .commit();

        BOOL_ELEMENT(expected)
                .key("setLogoConfig")
                .displayedName("Set LOGO Config")
                .description("Program a logo using powerdown pixel register bit")
                .tags("regAccess")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .commit();

        NODE_ELEMENT(expected).key("gain")
                .description("Coarse Gain Settings")
                .displayedName("Coarse Gain Settings")
                .commit();

        UINT32_ELEMENT(expected).key("gain.fcfEnCap")
                .displayedName("FCF_EnCap")
                .description("FCF Feedback Capacity setting")
                .tags("coarseGain")
                .readOnly()
                .commit();

        UINT32_ELEMENT(expected).key("gain.csaFbCap")
                .displayedName("CSA_FbCap")
                .description("CSA Feedback Capacitor setting")
                .tags("coarseGain")
                .readOnly()
                .commit();

        UINT32_ELEMENT(expected).key("gain.csaResistor")
                .displayedName("CSA Resistor")
                .description("Resistor setting of the CSA, the larger the higher the gain")
                .tags("coarseGain")
                .readOnly()
                .commit();

        UINT32_ELEMENT(expected).key("gain.csaInjCap")
                .displayedName("CSA Injection Cap")
                .description("Injection Cap setting of the CSA")
                .tags("coarseGain")
                .readOnly()
                .commit();

        BOOL_ELEMENT(expected).key("gain.csaInjCap200")
                .displayedName("En CSA Injection Cap 200")
                .description("enable large 200 Injection Cap setting of the CSA")
                .readOnly()
                .commit();

        BOOL_ELEMENT(expected).key("gain.qInjEnCs")
                .displayedName("En QInjEnCs")
                .description("enable Charge Injection Caps")
                .readOnly()
                .commit();

        UINT32_ELEMENT(expected).key("gain.integrationTime")
                .displayedName("IntegrationTime")
                .description("Selected Integration Time 35 = 50ns is nominal")
                .readOnly()
                .commit();

        STRING_ELEMENT(expected).key("gain.irampFineTrm")
                .displayedName("Iramp Fine Trim")
                .description("Selected ADC Gain Setting")
                .readOnly()
                .commit();

        STRING_ELEMENT(expected).key("gain.rmpCurrDouble")
                .displayedName("Iramp Current Double")
                .description("Selected ADC Gain Setting")
                .readOnly()
                .commit();


        STRING_ELEMENT(expected).key("gain.pixelDelay")
                .displayedName("Pixel Delay")
                .description("Selected Pixel Delay Setting")
                .readOnly()
                .commit();
        
        UINT64_ELEMENT(expected).key("gain.gainHash")
                .displayedName("Gain settings hash")
                .description("Hash value estimated from gain settings")
                .readOnly()
                .commit();


        SLOT_ELEMENT(expected)
                .key("preProgSelReg").displayedName("Set Register")
                .description("Set register value only in computer and don't program selected Register")
                .allowedStates(State::ON, State::STOPPED, State::STARTED, State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected)
                .key("progSelReg").displayedName("Program Register")
                .description("Program selected Register")
                .allowedStates(State::ON, State::STOPPED, State::STARTED, State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected)
                .key("readSelReg").displayedName("Read Register Value")
                .description("Read selected Register from locals")
                .allowedStates(State::ON, State::STOPPED, State::STARTED, State::ACQUIRING)
                .commit();


        INT32_ELEMENT(expected).key("sequencerParameterValue")
                .displayedName("Sequencer Parameter Value")
                .description("Sequencer Parameter Value to change")
                .assignmentOptional().defaultValue(0).reconfigurable()
                .commit();

        STRING_ELEMENT(expected).key("sequencerParameterName")
                .displayedName("Sequencer Parameter Name")
                .description("Sequencer Parameter Name to change")
                .assignmentOptional().defaultValue(SuS::Sequencer::getParameterNames().front()).reconfigurable()
                .options(SuS::Sequencer::getParameterNames())
                .commit();

        SLOT_ELEMENT(expected)
                .key("setSequencerParameter").displayedName("Set Sequencer Parameter")
                .description("Select Sequencer parameter by name and change its value")
                .commit();


        INT32_ELEMENT(expected).key("burstParameterValue")
                .displayedName("Burst Parameter Value")
                .description("Value of the burst parameter to change")
                .assignmentOptional().defaultValue(0).reconfigurable()
                .commit();

        STRING_ELEMENT(expected).key("burstParameterName")
                .displayedName("Burst Parameter Name")
                .description("PPT Serial number read from PPT")
                .assignmentOptional().defaultValue("start_wait_offs").reconfigurable()
                // .options("start_wait_offs,start_wait_time")
                .commit();

        SLOT_ELEMENT(expected)
                .key("setBurstParameter").displayedName("Set Burst Parameter")
                .description("Select burst parameter by name and change its value")
                .commit();

        STRING_ELEMENT(expected).key("selPRBActivePowers")
                .displayedName("Active PRB Powers")
                .description("Select ASIC power to enable during operation")
                .tags("other")
                .assignmentOptional().defaultValue("all").reconfigurable()
                .commit();

        PATH_ELEMENT(expected).key("epcRegisterFilePath")
                .description("Name of the epc configuration file")
                .displayedName("EPC Register Filename")
                .isInputFile()
                .tags("EPCConfigPath")
                .assignmentOptional().defaultValue("~/karabo/devices/DsscPpt/ConfigFiles/PPT_EPCRegs.txt").reconfigurable()
                .commit();

        SLOT_ELEMENT(expected)
                .key("programEPCConfig").displayedName("Program EPC Regs")
                .description("Programming all EPC Regs with defined values")
                .allowedStates(State::ON, State::STOPPED, State::STARTED, State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected)
                .key("readEPCRegisters").displayedName("Readback EPC Regs")
                .description("Read all EPC regs and compare with defined ")
                .allowedStates(State::ON, State::STOPPED, State::STARTED, State::ACQUIRING)
                .commit();


        UINT32_ELEMENT(expected).key("injectionValue")
                .displayedName("Injection Value")
                .description("Setting for the injection DAC")
                .assignmentOptional().defaultValue(4000).reconfigurable()
                .commit();

        SLOT_ELEMENT(expected)
                .key("setInjectionValue").displayedName("Set Injection Value")
                .description("Set the value current injection setting")
                .allowedStates(State::ON, State::STOPPED, State::STARTED, State::ACQUIRING, State::OFF)
                .commit();

        SLOT_ELEMENT(expected)
                .key("setIntDACMode").displayedName("Set IntDAC Mode")
                .description("Set filter in buffer mode and connect internal DAC to Monbus in high range mode")
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::STARTED, State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected)
                .key("setNormalMode").displayedName("Enable Normal Mode")
                .description("Connect IntDAC to Monbus and enable sequencer normal mode")
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::STARTED, State::ACQUIRING)
                .commit();

        STRING_ELEMENT(expected).key("injectionMode")
                .displayedName("Injection Mode")
                .description("Select Injection Mode, can be used for trimming")
                .tags("measurements")
                .assignmentOptional().defaultValue("NORM").reconfigurable()
                .options({"CURRENT_BGDAC", "CURRENT_SUSDAC", "CHARGE_PXINJ_BGDAC", "CHARGE_PXINJ_SUSDAC", "CHARGE_BUSINJ", "ADC_INJ", "ADC_INJ_LR", "EXT_LATCH", "NORM"})
        .commit();

        SLOT_ELEMENT(expected)
                .key("setInjectionMode").displayedName("Set Injection Mode")
                .description("Enable selected Injection Mode")
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::STARTED, State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected)
                .key("setPixelInjectionMode").displayedName("En Pixel Injection Mode")
                .description("Enable Pixel Injection in all pixels")
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::STARTED, State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected)
                .key("storePoweredPixels").displayedName("Store Powered Pixels")
                .description("Remember currently powered pixels for lated restoring")
                .commit();

        SLOT_ELEMENT(expected)
                .key("restorePoweredPixels").displayedName("Restore Powered Pixels")
                .description("Restore currently remembered powered pixels")
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::STARTED, State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected)
                .key("setCurrentQuarterOn").displayedName("Set Current Quarter On")
                .description("Enable currently selected quarter of pixels")
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::STARTED, State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected)
                .key("setCurrentColSkipOn").displayedName("Set Current Col Skip Pattern On")
                .description("Enable currently selected colskip pattern")
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::STARTED, State::ACQUIRING)
                .commit();

        STRING_ELEMENT(expected)
                .key("colSelectMode").displayedName("Injection Column Selection Mode")
                .description("Enable special column selection modes")
                .assignmentOptional().defaultValue("SKIP").reconfigurable()
                .options({"BLOCK", "SKIP", "SKIPSPLIT"})
        .commit();

        STRING_ELEMENT(expected)
                .key("columnSelect").displayedName("Select Columns to Enable")
                .description("Enable column to monbus")
                .assignmentOptional().defaultValue("0-63").reconfigurable()
                .commit();

        UINT32_ELEMENT(expected)
                .key("numParallelColumns").displayedName("Num Parallel Columns")
                .description("define number of columns which are activated in parallel")
                .assignmentOptional().defaultValue(8).reconfigurable()
                .minExc(0).maxInc(64)
                .commit();

        UINT32_ELEMENT(expected)
                .key("pixelsColSelect").displayedName("Pixels Column Select")
                .description("define quarter number of pixels to be programmed")
                .assignmentOptional().defaultValue(0).reconfigurable()
                .minInc(0).maxInc(63)
                .commit();

        BOOL_ELEMENT(expected)
                .key("enD0Mode")
                .displayedName("En D0 Mode")
                .description("Set D0 Mode")
                .tags("measurements")
                .assignmentOptional().defaultValue(true).reconfigurable()
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::STARTED, State::ACQUIRING)
                .commit();

        BOOL_ELEMENT(expected)
                .key("bypassCompression")
                .displayedName("Bypass Compression")
                .description("Bypass Compression")
                .tags("measurements")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::STARTED, State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected)
                .key("waitJTAGEngineDone")
                .displayedName("Wait JTAG Done")
                .commit();

        INIT_PPT_PLL_ELEMENTS


        //Initiate IOB and enable datapath Elements
        //Defined in DsscPptRegsInit.hh

        INIT_PROGRAM_IOB_FPGA_ELEMENTS

        PATH_ELEMENT(expected).key("iobRegisterFilePath")
                .description("Name of the IOB configuration file")
                .displayedName("IOB ConfigFile Name")
                .isInputFile()
                .tags("IOBConfigPath")
                .assignmentOptional().defaultValue("~/karabo/devices/DsscPpt/ConfigFiles/IOBConfig.txt").reconfigurable()
                .commit();

        SLOT_ELEMENT(expected)
                .key("saveConfigIOB")
                .displayedName("Save IOB Config")
                .description("Save current IOB sysconfig register to selected file")
                .commit();


        INIT_IOB_ELEMENTS

        INIT_CONFIG_REGISTER_ELEMENTS

        INIT_SEQUENCER_CONTROL_ELEMENTS

        INIT_SEQUENCE_ELEMENTS

        SLOT_ELEMENT(expected)
                .key("updateSequenceCounters").displayedName("Update Counters")
                .description("Set cycle counters in device")
                .allowedStates(State::ON, State::STOPPED, State::STARTED, State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected)
                .key("updateStartWaitOffset").displayedName("Update Burst Wait Offset")
                .description("Update Burst Wait Offset")
                .allowedStates(State::ON, State::STOPPED, State::STARTED, State::ACQUIRING)
                .commit();



        UINT32_ELEMENT(expected)
                .key("numActiveASICs").displayedName("Number of Active ASICs")
                .description("define number of ASICs in jtag chain")
                .tags("other")
                .minInc(1).maxInc(16)
                .assignmentOptional().defaultValue(1).reconfigurable()
                .allowedStates(State::UNKNOWN)
                .commit();


        STRING_ELEMENT(expected)
                .key("sendingASICs").displayedName("Sending ASICs")
                .description("define ASICs which are sending data")
                .assignmentOptional().defaultValue("11111111_11111111").reconfigurable()
                .commit();

        SLOT_ELEMENT(expected)
                .key("setSendingASICs").displayedName("Set Sending ASICs")
                .description("Define ASICs which are sending and which are not sending data: 11111111_11111111")
                .commit();


        UINT32_ELEMENT(expected)
                .key("lmkOutputToProgram").displayedName("LMK Clock Output")
                .description("Select LMK output for ASIC to reprogram. Check Active Module")
                .tags("other")
                .assignmentOptional().defaultValue(10).reconfigurable()
                .allowedStates(State::ON, State::STOPPED, State::STARTED, State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected)
                .key("programLMKOutput").displayedName("Program ASIC LMK Out")
                .description("Reprogram LMK Output to restart ASIC")
                .allowedStates(State::ON, State::STOPPED, State::STARTED, State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected)
                .key("programLMKsAuto").displayedName("Program LMKs Auto")
                .description("Automatically initalize the ASIC clocks for correct data transmission")
                .allowedStates(State::ON, State::STOPPED, State::STARTED, State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected)
                .key("setQuadrantSetup").displayedName("Set Quadrant Setup")
                .description("Select number of ASICs and DPs, disables dummy data")
                .allowedStates(State::UNKNOWN, State::OFF)
                .commit();

        STRING_ELEMENT(expected)
                .key("quadrantId").displayedName("QuadrantId")
                .description("Quadrant Id")
                .tags("other")
                .assignmentMandatory()
                .options(utils::DsscModuleInfo::getQuadrantIdList(), ",")
                .reconfigurable()
                .commit();

        UINT32_ELEMENT(expected)
                .key("numFramesToSendOut").displayedName("Num Frames in Train")
                .description("Number of Frames to Send Out. Value * 4096 * 16 * 2 = number of Bytes send per ethernet channel")
                .tags("other")
                .assignmentOptional().defaultValue(800).reconfigurable()
                .commit();

        UINT32_ELEMENT(expected)
                .key("numPreBurstVetos").displayedName("Num Pre Burst Vetos")
                .description("Number of bursts at the beginning of a train, can be used to dismiss a number of unwanted events.")
                .tags("other")
                .assignmentOptional().defaultValue(10).reconfigurable()
                .allowedStates(State::ON, State::STOPPED, State::STARTED, State::ACQUIRING)
                .commit();

        BOOL_ELEMENT(expected)
                .key("xfelMode").displayedName("XFEL Control")
                .description("Enable XFEL Mode. Get main clock from C&C network")
                .tags("other")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::ON, State::STOPPED, State::OFF)
                .commit();

        BOOL_ELEMENT(expected)
                .key("continuous_mode").displayedName("Continuous Mode")
                .description("Set device in continuous running mode")
                .tags("other")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .commit();

        BOOL_ELEMENT(expected)
                .key("disable_sending").displayedName("Disable Sending")
                .description("Select if all frames are sent out in continuous mode ")
                .tags("other")
                .assignmentOptional().defaultValue(true).reconfigurable()
                .commit();

        BOOL_ELEMENT(expected)
                .key("send_dummy_packets").displayedName("Send dummy packets")
                .description("Enable sending of dummy data packets directly from ethernet engine in continous mode")
                .tags("other")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .commit();

        BOOL_ELEMENT(expected)
                .key("send_dummy_dr_data").displayedName("Send dummy data")
                .description("Send dummy data generated in the aurora core as received from the IOBoard")
                .tags("other")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .commit();

        BOOL_ELEMENT(expected)
                .key("ASIC_send_dummy_data").displayedName("Send dummy ASIC data")
                .description("Send dummy data generated in the IOBoard as received from the readoutASIC")
                .tags("other")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .commit();

        BOOL_ELEMENT(expected)
                .key("send_raw_data").displayedName("Send Raw Data")
                .description("Send raw data or converted data.")
                .tags("other")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .commit();

        UINT16_ELEMENT(expected)
                .key("enableDPChannels").displayedName("Enable datapath channels")
                .description("One hot coded datapath enable")
                .tags("other")
                .assignmentOptional().defaultValue(1).reconfigurable()
                .minInc(0).maxInc(15)
                .commit();

        BOOL_ELEMENT(expected)
                .key("clone_eth0_to_eth1").displayedName("Clone Eth1 to Eth2")
                .description("Enable sending of ASIC data from Ladder1 to Ethernet Output 2")
                .tags("other")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .commit();

        SLOT_ELEMENT(expected)
                .key("checkQSFPConnected")
                .displayedName("Check QSFP Links")
                .description("checks the number of connecte links")
                .commit();

        STRING_ELEMENT(expected).key("connectedETHChannels")
                .displayedName("Connected ETH Channels")
                .description("PPT can see if a fiber of the QSFP link is connected or not")
                .readOnly()
                .initialValue("nA")
                .commit();


        SLOT_ELEMENT(expected)
                .key("checkIOBDataFailed").displayedName("Check IOB Data Failed")
                .description("Read data receive status register in IOB")
                .commit();


        UINT16_ELEMENT(expected)
                .key("activeChannelReadoutFailure").displayedName("active channel readout failure")
                .description("Parameter to read from remote. Indicates which channels have not been sending data.")
                .readOnly()
                .commit();

        SLOT_ELEMENT(expected)
                .key("checkPPTDataFailed").displayedName("Check PPT Data Failed")
                .description("Read data receive status register in PPT")
                .commit();

        SLOT_ELEMENT(expected)
                .key("checkASICReset").displayedName("Check ASIC Reset")
                .description("Check and initialize the readout ASICs of the current Module")
                .commit();

        PPT_CHANNEL_FAILED_ELEMENTS


        SLOT_ELEMENT(expected)
                .key("loadLastFileETHConfig").displayedName("Take over last ETH config")
                .description("If a new config file was loaded one can take the ETH Config from the file as new valid config")
                .commit();

        //Initial Ethernet Elements
        //Defined in DsscPptRegsInit.hh
        INIT_ETH_ELEMENTS

        UINT32_ELEMENT(expected)
                .key("ethThrottleDivider").displayedName("Eth. throttle divider")
                .description("Ethernet engine throttle divider parameter")
                .assignmentOptional().defaultValue(0).reconfigurable()
                .commit();

        SLOT_ELEMENT(expected)
                .key("setThrottleDivider")
                .displayedName("Set eth. throttle divider")
                .description("Set ethernet engine throttle divider")
                .commit();


        INIT_ENABLE_DATAPATH_ELEMENTS

        SLOT_ELEMENT(expected)
                .key("saveConfiguration")
                .displayedName("Save configuration")
                .description("Save current configuration to selected files")
                .commit();

        INPUT_CHANNEL(expected).key("registerConfigInput")
                .displayedName("Input")
                .commit();

        BOOL_ELEMENT(expected)
                .key("iobProgrammed")
                .displayedName("IOB programmed")
                .description("IOB programmed")
                .assignmentOptional()
                .defaultValue(false)
                .allowedStates(State::ON, State::STOPPED, State::OFF, State::STARTED, State::ACQUIRING)
                .commit();

        
        NODE_ELEMENT(expected).key(s_dsscConfBaseNode)
                .description("EPC, IOB and JTAG detector registry")
                .displayedName(s_dsscConfBaseNode)
                .commit();

        SLOT_ELEMENT(expected)
                .key("updateConfigHash").displayedName("Read Config Data")
                .description("Read Configuration Data")
                .commit();//*/
        
        SLOT_ELEMENT(expected)
                .key("updateConfigFromHash").displayedName("Write Config Data")
                .description("Write Configuration Data")
                .commit();//*/
    }

    const std::string DsscPpt::s_dsscConfBaseNode = "DetectorRegisters";
    
    DsscPpt::DsscPpt(const karabo::util::Hash& config)
        : Device<>(config),
        m_keepAcquisition(false), m_keepPolling(false), m_burstAcquisition(false),
        m_pollThread(),
        m_ppt(),
        m_epcTag("epcParam"), m_dsscConfigtoSchema() {
        
        EventLoop::addThread(16);

        KARABO_INITIAL_FUNCTION(initialize);

        KARABO_SLOT(open);
        KARABO_SLOT(close);
        KARABO_SLOT(runXFEL);
        KARABO_SLOT(runStandAlone);
        KARABO_SLOT(stopStandalone);
        KARABO_SLOT(stopAcquisition);
        KARABO_SLOT(startManualBurstBtn);
        KARABO_SLOT(startAcquisition);
        KARABO_SLOT(startBurstAcquisition);
        KARABO_SLOT(startAllChannelsDummyData);

        KARABO_SLOT(updateFirmwareFlash);
        KARABO_SLOT(updateLinuxFlash);
        KARABO_SLOT(updateIOBFirmware);
        KARABO_SLOT(initSystem);
        KARABO_SLOT(initSingleModule);
        KARABO_SLOT(updateGuiRegisters);


        KARABO_SLOT(programAllIOBFPGAs);
        KARABO_SLOT(programIOB1FPGA);
        KARABO_SLOT(programIOB2FPGA);
        KARABO_SLOT(programIOB3FPGA);
        KARABO_SLOT(programIOB4FPGA);
        KARABO_SLOT(checkAllIOBStatus);

        KARABO_SLOT(preProgSelReg);
        KARABO_SLOT(progSelReg);
        KARABO_SLOT(readSelReg);

        KARABO_SLOT(programJTAG);
        KARABO_SLOT(programPixelRegister);
        KARABO_SLOT(programPixelRegisterDefault);
        KARABO_SLOT(programSequencers);
        KARABO_SLOT(programSequencer);
        KARABO_SLOT(updateSequencer);

        KARABO_SLOT(doFastInit);
        KARABO_SLOT(initChip);

        KARABO_SLOT(updateStartWaitOffset);
        KARABO_SLOT(updateSequenceCounters);
        KARABO_SLOT(programLMKOutput);

        KARABO_SLOT(resetAllBtn);
        KARABO_SLOT(resetEPC);
        KARABO_SLOT(resetIOBs);
        KARABO_SLOT(resetDatapath);
        KARABO_SLOT(resetASICs);
        KARABO_SLOT(programEPCConfig);
        KARABO_SLOT(readEPCRegisters);

        KARABO_SLOT(setBurstParameter);
        KARABO_SLOT(setSequencerParameter);
        KARABO_SLOT(storePoweredPixels);
        KARABO_SLOT(restorePoweredPixels);
        KARABO_SLOT(setCurrentQuarterOn);
        KARABO_SLOT(setCurrentColSkipOn);

        KARABO_SLOT(setInjectionValue);
        KARABO_SLOT(setIntDACMode);
        KARABO_SLOT(setNormalMode);
        KARABO_SLOT(setPixelInjectionMode);
        KARABO_SLOT(setInjectionMode);

        KARABO_SLOT(setQuadrantSetup);
        //to pass key values with nodes use own Makro
        PROG_IOBSLOTS

        KARABO_SLOT(saveConfigIOB);
        KARABO_SLOT(saveConfiguration);
        KARABO_SLOT(storeFullConfigFile);
        KARABO_SLOT(storeFullConfigUnder);
        KARABO_SLOT(storeFullConfigHDF5);
        KARABO_SLOT(sendConfigHashOut);

        KARABO_SLOT(setSendingASICs);
        KARABO_SLOT(programLMKsAuto);

        KARABO_SLOT(checkIOBDataFailed);
        KARABO_SLOT(checkPPTDataFailed);
        KARABO_SLOT(checkASICReset);
        KARABO_SLOT(fillSramAndReadout);
        KARABO_SLOT(readLastPPTTrainID);

        KARABO_SLOT(waitJTAGEngineDone);
        KARABO_SLOT(loadLastFileETHConfig);
        KARABO_SLOT(checkQSFPConnected);

        KARABO_SLOT(setThrottleDivider);

        KARABO_SLOT(startSingleCycle);
        
       // KARABO_SLOT(updateConfigSchema);
        
        KARABO_SLOT(updateConfigHash);
        KARABO_SLOT(updateConfigFromHash);
    }

    void DsscPpt::preDestruction() {

        this->signalEndOfStream("daqOutput");
        const string defaultConfigPath = DEFAULTCONF;
        m_ppt->storeFullConfigFile(defaultConfigPath);
        
        stop();
        
        if (m_pollThread && m_pollThread->joinable()) {
            m_keepPolling = false;
            m_pollThread->join();
        }
      
    }
    
    DsscPpt::~DsscPpt() {
        EventLoop::removeThread(16);
    }

    void DsscPpt::initialize() {
        KARABO_ON_DATA("registerConfigInput", receiveRegisterConfiguration);
        SuS::PPTFullConfig* fullconfig = new SuS::PPTFullConfig(get<string>("fullConfigFileName"));     

        if(fullconfig->isGood()){
                  m_ppt = PPT_Pointer(new SuS::DSSC_PPT_API(fullconfig));
        }else{
                delete fullconfig;
                DEVICE_ERROR("FullConfigFile invalid");
                return;
        }
        

        {        
            DsscScopedLock lock(&m_accessToPptMutex, __func__);

            setQSFPEthernetConfig();

            m_iobCurrIOBNumber = "1";
            m_jtagCurrIOBNumber = "1";

            m_ppt->setInitDist(120);
            m_ppt->setFastInitConfigSpeed(30);
            m_ppt->setEPCParam("JTAG_Control_Register", "all", "ASIC_JTAG_Clock_Divider", 30);

            const auto quadrantId = this->get<string>("quadrantId");
            m_ppt->setQuadrantId(quadrantId);

            set<string>("epcRegisterFilePath", m_ppt->getEPCRegisters()->getFileName());
            set<string>("iobRegisterFilePath", m_ppt->getIOBRegisters()->getFileName());
            set<string>("pixelRegisterFilePath", m_ppt->getPixelRegisters()->getFileName());
            set<string>("jtagRegisterFilePath", m_ppt->getJTAGRegisters()->getFileName());
            set<string>("sequencerFilePath", m_ppt->getSequencer()->getFilename());
        }

        updateGuiMeasurementParameters();

        getSequencerParamsIntoGui();

        getSequenceCountersIntoGui();

        getCoarseGainParamsIntoGui();
        
        updateGainHashValue();
        updateConfigHash();

        KARABO_LOG_INFO << "init done";

    }


    void DsscPpt::receiveRegisterConfiguration(const Hash& data,
                                               const InputChannel::MetaData& meta) {
        DSSC::StateChangeKeeper keeper(this);

        KARABO_LOG_INFO << "DsscPpt: received new configuration from " << meta.getSource();

        if (!data.has("regType")) {
            KARABO_LOG_WARN << "Receive Configuration: no regType defined";
            return;
        }

        const string& regType = data.get<string>("regType");

        if (regType == "burstParams") {
            receiveBurstParams(data);
        } else if (regType == "sequencer") {
            receiveSequencerConfig(data);
        } else {
            receiveConfigRegister(data);
        }

        KARABO_LOG_INFO << "getSequencerParamsIntoGui";
        getSequencerParamsIntoGui();

        KARABO_LOG_INFO << "getSequenceCountersIntoGui";
        getSequenceCountersIntoGui();

        KARABO_LOG_INFO << "getCoarseGainParametersIntoGui";
        getCoarseGainParamsIntoGui();
    }


    void DsscPpt::receiveConfigRegister(const Hash& data) {
        if (!data.has("currentModule")) {
            KARABO_LOG_WARN << "Receive Configuration: no currentModule defined";
            return;
        }

        const string regType = data.get<string>("regType");
        const int module = data.get<int>("currentModule");

        cout << "DsscPpt: reg Type " << regType << " on module " << module << endl;

        m_ppt->setActiveModule(module);

        auto * currentReg = m_ppt->getRegisters(regType);

        vector<string> moduleSetNames;
        utils::split(data.get<string>("moduleSets"), ';', moduleSetNames, 0);

        if (moduleSetNames.empty()) {
            KARABO_LOG_ERROR << "ERROR: no ModuleSet found in incoming configuration";
            return;
        }

        bool programDefault = true;

        for (auto && moduleSet : moduleSetNames) {
            string modulesList = data.get<string>(moduleSet + ".moduleNumbers");
            std::vector<string> modules = utils::positionListToStringVector(modulesList);
            vector<string> signalNames;
            utils::split(data.get<string>(moduleSet + ".signalNames"), ';', signalNames, 0);

            for (auto && signalName : signalNames) {
                //const auto signalsData = data.get<util::NDArray>(moduleSet + "." + signalName);
                const vector<uint32_t> signalsData = data.get<vector<uint32_t> >(moduleSet + "." + signalName);
                size_t data_size = signalsData.size();

                //auto signalValues = signalsData.getData<unsigned int>();

                if (data_size == 1) {
                    //currentReg->setSignalValue(moduleSet, "all", signalName, signalValues[0]);
                    currentReg->setSignalValue(moduleSet, "all", signalName, signalsData[0]);
                } else {
                    if (data_size != modules.size()) {
                        KARABO_LOG_ERROR << "ERROR: Number of signal values does not fit to number of modules: " << data_size << "/" << modules.size();
                        continue;
                    }

                    programDefault = false;

                    for (size_t idx = 0; idx < data_size; idx++) {
                        //currentReg->setSignalValue(moduleSet, modules[idx], signalName, signalValues[idx]);
                        currentReg->setSignalValue(moduleSet, modules[idx], signalName, signalsData[idx]);
                    }
                }
            }
        }

        if (regType == "jtag") {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            if (moduleSetNames.size() > 1) {
                m_ppt->programJtag();
            } else {
                m_ppt->programJtagSingle(moduleSetNames.front());
            }
        } else if (regType == "pixel") {
            if (programDefault) {
                DsscScopedLock lock(&m_accessToPptMutex, __func__);
                m_ppt->programPixelRegsAllAtOnce();
            } else {
                DsscScopedLock lock(&m_accessToPptMutex, __func__);
                m_ppt->programPixelRegs();
            }
        }
        KARABO_LOG_INFO << regType << "Configuration Received";
    }


    void DsscPpt::waitJTAGEngineDone() {
        int cnt = 0;
        while ((getState() == util::State::CHANGING) && cnt < 500) {
            usleep(20000);
            cnt++;
        }

        KARABO_LOG_DEBUG << "DsscPptDevice receive register cofnig wait count = " << cnt;

        if (cnt == 500) {
            KARABO_LOG_WARN << "DsscPptDevice state does not change ";
        }

        cnt = 0;
        do {
            usleep(20000);
            cnt++;
        } while ((getState() == util::State::CHANGING) && cnt < 1e5);

    }


    void DsscPpt::receiveSequencerConfig(const Hash& data) {
        if (!data.has("sequencerParams")) return;

        auto paramData = data.get<Hash>("sequencerParams");
        for (auto && path : paramData) {
            string seqParamName = path.getKey();
            if (seqParamName == "opMode") {
                SuS::Sequencer::OpMode seqOpMode = (SuS::Sequencer::OpMode)paramData.get<unsigned int>("opMode");
                m_ppt->getSequencer()->setOpMode(seqOpMode);

                const auto seqOpModeStr = SuS::Sequencer::getOpModeStr(seqOpMode);
                set<string>("sequencer.opMode", seqOpModeStr);
                KARABO_LOG_INFO << "Sequencer OpMode is now " << seqOpModeStr;
            } else {
                auto value = paramData.get<unsigned int>(seqParamName);
                m_ppt->getSequencer()->setSequencerParameter(seqParamName, value, false);
            }
        }

        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->programSequencers();
        }

        bool cycleLengthChanged = (get<unsigned int>("sequencer.cycleLength") != (unsigned int) m_ppt->getSequencer()->getCycleLength());
        if (cycleLengthChanged) {
            KARABO_LOG_WARN << "!!!!CYCLE LENGTH CHANGED!!!!";
            updateSequenceCounters();
        }
    }


    void DsscPpt::receiveBurstParams(const Hash& data) {
        if (!data.has("paramValues")) return;

        auto paramData = data.get<Hash>("paramValues");
        for (auto && path : paramData) {
            string burstParamName = path.getKey();
            int paramValue = paramData.get<int>(burstParamName);
            set<int>("sequence." + burstParamName, paramValue);
        }

        updateSequenceCounters();
    }


    void DsscPpt::updateGuiRegisters() {
        updateGuiMeasurementParameters();

        getSequencerParamsIntoGui();

        getSequenceCountersIntoGui();

        getCoarseGainParamsIntoGui();

        getEPCParamsIntoGui();

        getIOBParamsIntoGui();

        getJTAGParamsIntoGui();

        getPixelParamsIntoGui();
    }


    void DsscPpt::disableAllDummyData() {
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->disableAllDummyData();
        }
        updateGuiOtherParameters();
    }


    void DsscPpt::startAllChannelsDummyData() {
        enableDPChannels(0xF);

        {
            set<bool>("send_dummy_dr_data", true);
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->enableDummyDRData(true);
        }

        runContMode(true);
        runAcquisition(true);
    }


    void DsscPpt::setQuadrantSetup() {
        disableAllDummyData();
        enableDPChannels(0xF);

        //set<string>("pptHost","192.168.0.125");
        set<unsigned int>("numActiveASICs", 16);

        const auto environment = "HAMBURG";
        set<string>("selEnvironment", environment);
        updateTestEnvironment();
    }


    bool DsscPpt::checkConfigFilePaths(const karabo::util::Hash& config) {
        bool ok = true;

        if (!boost::filesystem::exists(config.get<string>("fullConfigFileName"))) {
            KARABO_LOG_ERROR << "FullConfigFile not found: check defined FullConfigFileName";
            ok = false;
        }

        return ok;
    }


    bool DsscPpt::isProgramState(bool withAcquiring) {
        const auto currentState = getState();
        if (currentState == util::State::ACQUIRING) {
            return withAcquiring;
        }

        if (currentState == util::State::ON) {
            return true;
        }

        if (currentState == util::State::STARTED) {
            return true;
        }

        if (currentState == util::State::STOPPED) {
            return true;
        }
        return false;
    }


    void DsscPpt::generateAllConfigRegElements() {
        
        
        
        Schema schema;        

        generateConfigRegElements(schema, m_ppt->getEPCRegisters(), "EPCRegisters", m_epcTag);

        generateConfigRegElements(schema, m_ppt->getIOBRegisters(), "IOBRegisters", "IOBConfig", "1");

        generateConfigRegElements(schema, m_ppt->getJTAGRegisters(), "JTAGRegisters", "JTAGConfig", "0");

        generateConfigRegElements(schema, m_ppt->getPixelRegisters(), "PixelRegisters", "PixelConfig", "0");

        updateSchema(schema);
    }


    void DsscPpt::generateConfigRegElements(Schema &schema, SuS::ConfigReg * reg,\
            string regName, string tagName, std::string rootNode) {
        // Build schema using the Config Reg Structure
        
        std::string rootRegName = rootNode + "." + regName;        
        
        NODE_ELEMENT(schema).key(removeSpaces(rootRegName))
                .description(regName)
                .displayedName(regName)
                .commit();
        
        const auto moduleSets = reg->getModuleSetNames();

        for (const auto & modSetName : moduleSets) { 
            string keySetName(rootRegName + "." + modSetName);

            NODE_ELEMENT(schema).key(removeSpaces(keySetName))
                    .description(keySetName)
                    .displayedName(modSetName)
                    .commit();
            
            std::string keyModuleSet_modules = keySetName + ".modules";
            STRING_ELEMENT(schema).key(removeSpaces(keyModuleSet_modules))
                .displayedName("modules")
                .description("modules in signalName")
                .tags(tagName)
                .assignmentOptional().defaultValue(reg->getModuleNumberList(modSetName))
                .allowedStates(State::UNKNOWN, State::ON, State::STOPPED)
                .commit();

            std::vector<std::string> modules_strvec = reg->getModules(modSetName);
            const auto signalNames = reg->getSignalNames(modSetName);
            for (const auto & sigName : signalNames) {
                //  KARABO_LOG_INFO << "Add Signal " + sigName;
                if (sigName.find("_nc") != string::npos) {
                    continue;
                }
                
                std::string keySignalName(keySetName + "." + sigName);
                
                NODE_ELEMENT(schema).key(removeSpaces(keySignalName))
                    .description(sigName)
                    .displayedName(sigName)
                    .commit();              
                
                std::vector<uint32_t> signalVals = reg->getSignalValues(modSetName,"all",sigName);
                int i = 0;
                for(auto module_str : modules_strvec){

                  std::string modSignalName(keySignalName + "." + module_str);
                
                  bool readOnly = reg->isSignalReadOnly(modSetName, sigName);

                  //uint32_t accessLevel = reg->getAccessLevel(modSetName,sigName);
                  // description (for tooltip)
                  // TODO compute min/max from numBits
                  // allowedStates??
                  string keyModuleName(regName + "." + modSetName + "." + sigName);
                  unsigned int maxValue = reg->getMaxSignalValue(modSetName, sigName);

                  if (readOnly) {
                    UINT32_ELEMENT(schema).key(removeSpaces(modSignalName))
                        .description(modSignalName)
                        .tags(tagName)
                        .displayedName(module_str)
                        .allowedStates(State::UNKNOWN, State::ON, State::STOPPED)
                        .readOnly()
                        .initialValue(signalVals[i])
                        .commit();
                  } else {
                    UINT32_ELEMENT(schema).key(removeSpaces(modSignalName))
                        .description(modSignalName)
                        .tags(tagName)
                        .displayedName(module_str)
                        .reconfigurable()
                        .allowedStates(State::UNKNOWN, State::ON, State::STOPPED)
                        .maxInc(maxValue)
                        .assignmentOptional().defaultValue(signalVals[i])
                        .commit();
                    }//*/
                  i++;
                }
            }
        }
    }


    void DsscPpt::startPolling() {
        if (m_keepPolling) return;

        m_keepPolling = true;
        m_pollThread.reset(new boost::thread(boost::bind(&DsscPpt::pollHardware, this)));
        KARABO_LOG_INFO << "PollThread started...";
    }


    void DsscPpt::stopPolling() {
        m_keepPolling = false;
        if (m_pollThread) {
            m_pollThread->join();
            m_pollThread.reset();
        }
        KARABO_LOG_INFO << "PollThread joined...";
    }


    /*void DsscPpt::idleStateOnEntry() {
        if (m_ppt->isOpen()) {
            startPolling();
        }
    }//*/


    void DsscPpt::acquisitionStateOnEntry() {
        DsscScopedLock lock(&m_accessToPptMutex, __func__);
        m_ppt->enableXFELControl(true);
        updateGuiPLLParameters();
        runXFEL();
    }


    void DsscPpt::acquisitionStateOnExit() {
        DsscScopedLock lock(&m_accessToPptMutex, __func__);
        stopAcquisition();
    }


    void DsscPpt::manualAcquisitionStateOnEntry() {
        DsscScopedLock lock(&m_accessToPptMutex, __func__);
        m_ppt->enableXFELControl(false);
        updateGuiPLLParameters();
    }


    void DsscPpt::setLogoConfig(bool en) {
        int value = en ? 1 : 0;
        DsscScopedLock lock(&m_accessToPptMutex, __func__);
        m_ppt->setLogoConfig("LOC_PWRD", value);
    }


    void DsscPpt::open() { //is triggered by Connect to PPT Button

        if (!m_ppt->isOpen()) {
            this->updateState(State::OPENING);

            // Open client connection to the PPT
            string host = "Blubb";
            unsigned int port = 10;
            host = get<string>("pptHost");
            port = get<unsigned int>("pptPort");
            KARABO_LOG_INFO << "About to open PPT using host: " + host + " and port: " + toString(port);
            m_ppt->setPPTAddress(host, port);
            int rc = m_ppt->openConnection();
            KARABO_LOG_INFO << "Just opened PPT: " << rc;
            if (rc != SuS::DSSC_PPT::ERROR_OK) {
                close();
                this->updateState(State::UNKNOWN);
                std::string message = "Failed to connect to PPT: " + m_ppt->errorString;
                set<string>("status", message);
                KARABO_LOG_ERROR << message;
                //throw KARABO_NETWORK_EXCEPTION("Failed to connect to PPT: " + m_ppt->errorString);
            }
        }

        if (m_ppt->isOpen()) {
            DSSC::StateChangeKeeper keeper(this, State::OFF);

            resetAll();

            programPLL();

            programPLLFine();

            readSerialNumber();

            updateTestEnvironment();

            checkQSFPConnected();
            
            set<string>("status", "PPT is connected");

        } 
        /*else {
            this->updateState(State::ERROR);
            KARABO_LOG_ERROR << "Open failure -- attempt to open a connection to PPT failed";
            //throw KARABO_NETWORK_EXCEPTION("Open failure -- attempt to open a connection to PPT failed");
        }*/
    }


    void DsscPpt::runContMode(bool run) {
        State endState = run ? State::STARTED : State::ON;
        DSSC::StateChangeKeeper keeper(this, endState);

        set<bool>("continuous_mode", run);
        {
            KARABO_LOG_INFO << "runContMode mutex";
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->runContinuousMode(run);
        }
    }


    void DsscPpt::runAcquisition(bool run) {
        State endState = run ? State::ACQUIRING : State::STARTED;
        DSSC::StateChangeKeeper keeper(this, endState);
        set<bool>("disable_sending", false);
        {
            KARABO_LOG_INFO << "runAcquisition mutex";
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->disableSending(false);
        }

        if (run) {
            startPolling();
        }
    }


    void DsscPpt::start() {
        if (get<bool>("xfelMode")) {
            runXFEL();
        } else {
            runStandAlone();
        }
    }


    void DsscPpt::stop() {
        stopAcquisition();
        runContMode(false);
    }


    void DsscPpt::startAcquisition() {
        runAcquisition(true);
    }
    
    void DsscPpt::burstAcquisitionPolling() {
        
        try {
            KARABO_LOG_INFO << "Hardware polling started";
       
          unsigned int num_trains = get<unsigned int>("numBurstTrains");
          assert(num_trains);

          //const auto currentState = getState();
          
          start();
          
          //updateState(State::ACQUIRING);

          unsigned long long last_trainId;       
          unsigned long long first_burstTrainId;
          {
            //boost::mutex::scoped_lock lock(m_accessToPptMutex);
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            first_burstTrainId = m_ppt->getCurrentTrainID();
          }
        
          set<unsigned long long>("burstData.startTrainId", 0);
          set<unsigned long long>("burstData.endTrainId", 0);
          bool first_train = true;
        
          static unsigned int wait_time = 30000;

          unsigned long long current_trainId;        
          while (m_burstAcquisition.load()) {
              
              {
                  //boost::mutex::scoped_lock lock(m_accessToPptMutex);
                  DsscScopedLock lock(&m_accessToPptMutex, __func__);
                  current_trainId = m_ppt->getCurrentTrainID();
              }

              if(first_train){
                  if(current_trainId != first_burstTrainId){
                    unsigned long long elapsedTrains = current_trainId - first_burstTrainId;
                    if( (elapsedTrains > 0) && elapsedTrains < (unsigned long long)(3) ){
                        last_trainId = current_trainId;
                        first_train = false;
                    }
                    first_burstTrainId = current_trainId;
                  }
                  usleep(wait_time);
                  continue;
              }


              if(current_trainId > last_trainId){
                  unsigned long long train_diff = current_trainId - first_burstTrainId;
                  if(train_diff >= num_trains){
                      std::cout << "stopped acquisition, current/first trainId: " << current_trainId << "  " << first_burstTrainId << std::endl;
                      m_burstAcquisition.store(false); 
                      set<unsigned long long>("burstData.startTrainId", first_burstTrainId);
                      set<unsigned long long>("burstData.endTrainId", current_trainId);
                      stop();
                  }else{
                      last_trainId = current_trainId;
                      if(train_diff > 10){
                          wait_time = 250000;// to prevent often hw polling
                      }else{
                          wait_time = 30000;
                      }
                  }
              }else{
                 if(current_trainId < first_burstTrainId){
                   std::cout << "current_trainId is less than first_burstTrainId: " << current_trainId << "  " << first_burstTrainId << std::endl; 
                 }
              }
              usleep(wait_time);
          }
        
          set<unsigned long long>("burstData.startTrainId", first_burstTrainId);
          set<unsigned long long>("burstData.endTrainId", current_trainId);
          
          //updateState(currentState);
          //updateState(State::ON);
         
        } catch (const Exception& e) {
            KARABO_LOG_ERROR << e;
        } catch (...) {
            KARABO_LOG_ERROR << "Unknown exception was raised in poll thread";
        }
    }


    void DsscPpt::startBurstAcquisition() {
       
        m_burstAcquisition.store(true);
        EventLoop::getIOService().post(karabo::util::bind_weak(&DsscPpt::burstAcquisitionPolling, this));       
   }
    
    void DsscPpt::stopAcquisition() {
        
        m_burstAcquisition.store(false);

        runAcquisition(false);
        if (m_ppt->isXFELMode()){
            runContMode(false);
        }
    }


    void DsscPpt::runStandAlone() {
        runContMode(true);
        runAcquisition(true);
    }


    void DsscPpt::stopStandalone() {
        runAcquisition(false);
        runContMode(false);
    }


    void DsscPpt::runXFEL() {
        runContMode(true);
        runAcquisition(true);
    }


    void DsscPpt::updateTestEnvironment() {
        string environment = get<string>("selEnvironment");
        updateTestEnvironment(environment);
    }


    void DsscPpt::updateTestEnvironment(const string &environment) {
        if (environment.find("MANNHEIM") != string::npos) {
            KARABO_LOG_INFO << "TestSetup Environment set to MANNHEIM";

            cout << "Test Environment Set to Mannheim" << endl;

            SuS::DSSC_PPT::actSetup = SuS::DSSC_PPT::MANNHEIM;

            m_ppt->setEPCParam("JTAG_Control_Register", "all", "JTAG_Test_System", 1);
            m_ppt->setIOBParam("ASIC_invert_chan11", "all", "ASIC_invert_chan11", 1);

            uint16_t asics = 1 << (15 - 11 + 8);
            m_ppt->setActiveAsics(asics);
            m_ppt->setPRBPowerSelect("3", false);

            startManualMode();
            if (get<string>("qsfp.chan1.recv.macaddr") == "00:1b:21:55:1f:c9" ||
                get<string>("qsfp.chan1.recv.macaddr") == "0:1b:21:55:1f:c9") {
                set<string>("qsfp.chan1.recv.macaddr", "00:1b:21:55:1f:c8");
                set<string>("qsfp.chan2.recv.macaddr", "00:1b:21:55:1f:c8");
                set<string>("qsfp.chan3.recv.macaddr", "00:1b:21:55:1f:c8");
                set<string>("qsfp.chan4.recv.macaddr", "00:1b:21:55:1f:c8");
                KARABO_LOG_WARN << "Set QSFP Receiver MAC to 00:1b:21:55:1f:c8";
            }
        } else {
            KARABO_LOG_INFO << "TestSetup Environment set to HAMBURG";

            SuS::DSSC_PPT::actSetup = SuS::DSSC_PPT::HAMBURG;

            m_ppt->setEPCParam("JTAG_Control_Register", "all", "JTAG_Test_System", 0);
            m_ppt->setIOBParam("ASIC_invert_chan11", "all", "ASIC_invert_chan11", 0);

            stopManualMode();

            m_ppt->setNumberOfActiveAsics(get<unsigned int>("numActiveASICs"));

            cout << "Test Environment is set to Hamburg" << endl;
            cout << "QSFP and transceiver have to be defined according to setup" << endl;
        }

        updateNumFramesToSend();

        setSendingASICs();
        setQSFPEthernetConfig();
    }


    void DsscPpt::updateNumFramesToSend() {
        DsscScopedLock lock(&m_accessToPptMutex, __func__);
        m_ppt->setNumFramesToSend(get<unsigned int>("numFramesToSendOut"));
    }


    void DsscPpt::readSerialNumber() {
        unsigned long long sern;

        string firmware;
        string linux;
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);

            sern = m_ppt->readSerialNumber();
            firmware = m_ppt->readBuildStamp();
            linux = m_ppt->readLinuxBuildStamp();

            printPPTErrorMessages();
        }

        stringstream ss;
        ss << hex << "0x" << sern;

        string serialStr = ss.str();
        if (sern > 0x180000) {
            serialStr += " PPTv2";
        } else {
            serialStr += " PPTv1";
        }
        set<string>("pptSerial", serialStr);
        set<string>("firmwareRev", firmware);
        set<string>("linuxRev", linux);
    }


    void DsscPpt::readFullConfigFile(const std::string & fileName) {
        KARABO_LOG_INFO << "Load Full Config File : " << fileName;

        {            
            ContModeKeeper keeper(this);
            m_ppt->loadFullConfig(fileName, false);
            string defaultConfigPath = DEFAULTCONF;
            m_ppt->storeFullConfigFile(defaultConfigPath);

            //updateSequenceCounters();
        }

        const auto * fullConfigInfo = m_ppt->getFullConfig();
        set<string>("jtagRegisterFilePath", fullConfigInfo->getJtagRegsFileName());
        set<string>("pixelRegisterFilePath", fullConfigInfo->getPixelRegsFileName());
        set<string>("sequencerFilePath", fullConfigInfo->getSequencerFileName());
        set<string>("epcRegisterFilePath", fullConfigInfo->getIOBRegsFileName());
        set<string>("iobRegisterFilePath", fullConfigInfo->getEPCRegsFileName());

        {
            const auto currentState = getState();
            bool program = (currentState == State::ON ||
                            currentState == State::STOPPED ||
                            currentState == State::ACQUIRING);
            if (program) {
                initChip();
            }
        }

        updateGuiMeasurementParameters();
        getCoarseGainParamsIntoGui();
        updateNumFramesToSend();
        //updateSequenceCounters();
        
        updateGainHashValue();
        updateConfigHash();
    }
    


    void DsscPpt::storeFullConfigFile() {
        auto fileName = get<string>("fullConfigFileName");
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            checkPathExists(fileName);
            m_ppt->storeFullConfigFile(fileName);
        }
    }
    
        void DsscPpt::storeFullConfigUnder() {
        auto fileName = get<string>("saveConfigFileToName");
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            checkPathExists(fileName);
            m_ppt->storeFullConfigFile(fileName);
        }
    }
    



    bool DsscPpt::checkPathExists(const std::string & fileName) {
        const string filePath = utils::getFilePath(fileName);
        boost::filesystem::path data_dir(filePath);
        if (!boost::filesystem::exists(data_dir)) {
            if (boost::filesystem::create_directories(data_dir)) {
                KARABO_LOG_INFO << "Created new directory: " << filePath;
                return true;
            } else {
                KARABO_LOG_ERROR << "Could not create output directory: " << filePath;
                return false;
            }
        }
        return true;
    }


    void DsscPpt::storeFullConfigHDF5() {
        const auto fileName = get<string>("fullConfigFileName");
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            checkPathExists(fileName);
            const auto h5config = m_ppt->getHDF5ConfigData(fileName); // no need to use an object of class for calling static function,\
                                                                  // could be resolved like SuS::DSSC_PPT_API::getHDF5ConfigData(fileName)          
            DsscHDF5Writer::saveConfiguration(utils::getFilePath(fileName) + "/Measurement_config.h5", h5config);
        }
    }

    void DsscPpt::sendConfigHashOut() {
        if (m_last_config_hash != Hash()) {
            std::cout << "Sending config data" << std::endl;
            const karabo::util::Timestamp& actualTimestamp = this->getActualTimestamp();
            std::cout << "Writing data to daqOutput" << std::endl;
            this->writeChannel("daqOutput", m_last_config_hash, actualTimestamp); //*/
        }
    }
    

    void DsscPpt::updateGainHashValue() {
        EventLoop::getIOService().post(karabo::util::bind_weak(&DsscPpt::updateGainHashValue_impl, this)); 
    }
    
    void DsscPpt::updateGainHashValue_impl() {

        auto configData = m_ppt->getHDF5ConfigData();
        
        std::size_t seed = 0;
        boost::hash<int> hasher;
        
        for(DsscHDF5RegisterConfigVec::iterator register_data = configData.pixelRegisterDataVec.begin();
                register_data != configData.pixelRegisterDataVec.end(); register_data++)
            for(std::vector<std::vector<std::vector<uint32_t>>>::iterator module_set = (*register_data).registerData.begin();
                    module_set != (*register_data).registerData.end(); module_set++) 
                for(std::vector<std::vector<uint32_t>>::iterator signal = (*module_set).begin();
                        signal != (*module_set).end(); signal++)
                    for(std::vector<uint32_t>::iterator module_signal_value = (*signal).begin();
                            module_signal_value != (*signal).end(); module_signal_value++){
                        seed ^= hasher(*module_signal_value) + 0x9e3779b9 + (seed<<6) + (seed>>2); 
                    }
        
        for(DsscHDF5SequenceData::iterator sequencer_data = configData.sequencerData.begin();
                sequencer_data != configData.sequencerData.end(); sequencer_data++){
            seed ^= hasher(sequencer_data->second) + 0x9e3779b9 + (seed<<6) + (seed>>2); 
        }  
        set<unsigned long long>("gain.gainHash", static_cast<unsigned long long>(seed));
    }
    
    void DsscPpt::updateDetRegistryGui(SuS::ConfigReg * reg,\
            std::string regName, std::string tagName, std::string rootNode){
        
        
        std::string rootRegName = rootNode + "." + regName;        
        
        const auto moduleSets = reg->getModuleSetNames();

        for (const auto & modSetName : moduleSets) { 
            std::string keyModuleSetName(rootRegName + "." + modSetName);

            std::vector<std::string> modules_strvec = reg->getModules(modSetName);
            const auto signalNames = reg->getSignalNames(modSetName);
            for (const auto & sigName : signalNames) {
                //  KARABO_LOG_INFO << "Add Signal " + sigName;
                if (sigName.find("_nc") != string::npos) {
                    continue;
                }
                
                std::string keySignalName(keyModuleSetName + "." + sigName);
                
               
                std::vector<uint32_t> signalVals = reg->getSignalValues(modSetName,"all",sigName);
                int i = 0;
                for(auto module_str : modules_strvec){

                  std::string modSignalName(keySignalName + "." + module_str);
                  
                  this->set<unsigned int>(removeSpaces(modSignalName), signalVals[i]);

                  //string keyModuleName(regName + "." + modSetName + "." + sigName);

                  i++;
                }
            }
        }
        
        
    }
    
    
    void DsscPpt::updateConfigHash(){
        
        SuS::PPTFullConfig* full_conf = m_ppt->getPPTFullConfig();
        karabo::util::Schema theschema = this->getFullSchema();
        if(!theschema.subSchema(s_dsscConfBaseNode).empty()){            
            
            updateDetRegistryGui(m_ppt->getEPCRegisters(), "EPCRegisters", "EPCRegisters", s_dsscConfBaseNode);
            updateDetRegistryGui(m_ppt->getIOBRegisters(), "IOBRegisters", "IOBConfig", s_dsscConfBaseNode);
            for(int idx=0; idx<full_conf->numJtagRegs(); idx++){
              updateDetRegistryGui(full_conf->getJtagReg(idx), "JtagRegister_Module_" + to_string(idx+1),\
                      "JtagRegister_Module", s_dsscConfBaseNode);    
            }
            return;
        }
        //Schema schema;
        
        karabo::util::Schema schema;
        
        NODE_ELEMENT(schema).key(s_dsscConfBaseNode)
            .description("EPC, IOB and JTAG detector registry")
            .displayedName(s_dsscConfBaseNode)
            .commit();                  
          
        for(int idx=0; idx<full_conf->numJtagRegs(); idx++){
          generateConfigRegElements(schema, full_conf->getJtagReg(idx), \
                  "JtagRegister_Module_" + to_string(idx+1), "JtagRegister_Module", s_dsscConfBaseNode);    
        }

        generateConfigRegElements(schema, m_ppt->getEPCRegisters(), "EPCRegisters", "EPCRegisters", s_dsscConfBaseNode);

        generateConfigRegElements(schema, m_ppt->getIOBRegisters(), "IOBRegisters", "IOBConfig", s_dsscConfBaseNode);

        this->appendSchema(schema, true); 
        
        m_last_config_hash = this->get<Hash>(s_dsscConfBaseNode);
    }
    
    void DsscPpt::updateConfigFromHash(){

        DsscH5ConfigToSchema dsscH5ConfChObj;
        Hash  read_config_hash = this->get<Hash>(s_dsscConfBaseNode);
        std::vector<std::pair<std::string, unsigned int>> diff_entries = \
                dsscH5ConfChObj.compareConfigHashData(m_last_config_hash, read_config_hash);
        if(diff_entries.empty()) std::cout << "No chenges in config found" << std::endl;
        
        karabo::util::Schema theschema = this->getFullSchema();
        for(auto it : diff_entries){
            //std::cout << it.first <<std::endl;
            vector< std::string > SplitVec;
            boost::split( SplitVec, it.first, boost::is_any_of("."));            
           
            std::string selModSet = theschema.getDisplayedName(s_dsscConfBaseNode + "." \
                    + SplitVec[0]+"."+SplitVec[1]);
            std::string sigName = theschema.getDisplayedName(s_dsscConfBaseNode + "." + \
                    SplitVec[0]+"."+SplitVec[1]+ "."+SplitVec[2]);
            
            
            SuS::ConfigReg* configRegister;
            KARABO_LOG_INFO << selModSet + "\t" +  SplitVec.back() + "\t" + sigName + " :\t" << it.second;
            try{
                if(it.first.substr(0,4) == "EPCR"){
                    configRegister =  m_ppt->getRegisters("epc");
                    configRegister->setSignalValue(selModSet, SplitVec.back(), sigName, it.second);
                    {
                        DsscScopedLock lock(&m_accessToPptMutex, __func__);
                        m_ppt->programEPCRegister(selModSet);
                    }
                }else if(it.first.substr(0,4) == "IOBR"){
                    configRegister =  m_ppt->getRegisters("iob");
                    configRegister->setSignalValue(selModSet, SplitVec.back(), sigName, it.second);
                    const uint32_t module = get<uint32_t>("selModule");
                    setActiveModule(module);
                    {
                        DsscScopedLock lock(&m_accessToPptMutex, __func__);
                        m_ppt->programIOBRegister(selModSet);
                    }
                }else if(it.first.substr(0,4) == "Jtag"){
                    configRegister =  m_ppt->getRegisters("jtag");
                    uint32_t module = std::stoul(SplitVec[0].substr(SplitVec[0].length()-1, SplitVec[0].length()-1));
                    setActiveModule(module);
                    configRegister->setSignalValue(selModSet, SplitVec.back(), sigName, it.second);                
                    {
                        DsscScopedLock lock(&m_accessToPptMutex, __func__);
                        m_ppt->programJtagSingle(selModSet);
                    }                
                }else{
                    KARABO_LOG_DEBUG << "registry is not EPC, IOB, or Jtag";
                }
            }catch (std::logic_error){                
            }


        }
        m_last_config_hash = read_config_hash;               
    }
    
    void DsscPpt::doFastInit() {
        DSSC::StateChangeKeeper keeper(this, State::ON);
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->enablePRBStaticVoltage(false);
            m_ppt->fastASICInitTestSystem();
        }

        if (!printPPTErrorMessages()) {
            KARABO_LOG_INFO << "ASIC Initialized successfully";
        }
    }


    void DsscPpt::initSingleModule() {
        DSSC::StateChangeKeeper keeper(this, State::ON);

        int currentModule = get<uint32_t>("activeModule");
        if (m_ppt->isIOBAvailable(currentModule)) {
            programIOBFPGA(currentModule);
        }

        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);

            int rc = m_ppt->initSingleModule(currentModule);
            if (rc != SuS::DSSC_PPT::ERROR_OK) {
                printPPTErrorMessages();
            }
        }

        updateGuiRegisters();

        checkQSFPConnected();
    }


    void DsscPpt::initSystem() {
        DSSC::StateChangeKeeper keeper(this, State::ON);        
        std::cout << "initSystem->resetAll()" << std::endl;
        try{
            resetAll();
        }catch (const std::exception& e) { // caught by reference to base
            std::cout << "exception was caught in initSystem->resetAll, with message:"
                  << e.what() << std::endl;
        }

        std::cout << "initSystem->programPLL()" << std::endl;
        try{
          programPLL();
        }catch (const std::exception& e) { // caught by reference to base
            std::cout << "exception was caught in initSystem->programPLL, with message:"
                  << e.what() << std::endl;
        }

        
        if (checkAllIOBStatus() == 0) {
            KARABO_LOG_INFO << "No IOBs detected. Will try to program IOB FPGAs";
            this->set<bool>("iobProgrammed", false);
            std::cout << "initSystem->programAllIOBFPGAs()" <<std::endl;
            try{
                programAllIOBFPGAs();
            }catch (const std::exception& e) { // caught by reference to base
                std::cout << "exception was caught in initSystem->programAllIOBFPGAs, with message:"
                    << e.what() << std::endl;
            }
        }else{
            this->set<bool>("iobProgrammed", true);
        }

        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);

            m_ppt->setGlobalDecCapSetting((SuS::DSSC_PPT::DECCAPSETTING)1);
            
            std::cout << "initSystem->initSystem()" <<std::endl;

            int rc;
            
            try{
                rc = m_ppt->initSystem();
            }catch (const std::exception& e) { // caught by reference to base
                std::cout << "exception was caught in initSystem->initSystem, with message:"
                     << e.what() << std::endl;
            }
           
            if (rc != SuS::DSSC_PPT::ERROR_OK) {
                printPPTErrorMessages();
            }
        }

        updateGuiRegisters();

        checkQSFPConnected();

        updateSequenceCounters();
        
        std::cout << "initSystem finished" <<std::endl;
    }


    bool DsscPpt::initIOBs() {
        DSSC::StateChangeKeeper keeper(this);
        if (checkAllIOBStatus() == 0) return false;

        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->initIOBs();
        }

        return true;
    }


    bool DsscPpt::initChip() {
        KARABO_LOG_INFO << "initChip";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->initChip();
        }

        return printPPTErrorMessages(true);
    }


    void DsscPpt::resetAllBtn() {
        DSSC::StateChangeKeeper keeper(this);

        KARABO_LOG_INFO << "resetAll";

        resetAll();

        //reprogramm all EPC register to reset to last configuration
        programEPCConfig();

        checkAllIOBStatus();

        if (m_ppt->activeIOBs.size() == 0) return;

        //set Everything in Ready state

        programAvailableIOBsConfig();
    }


    void DsscPpt::resetAll() {
        DSSC::StateChangeKeeper keeper(this, State::OFF);

        stopPolling();

        enableDPChannels(0);

        DsscScopedLock lock(&m_accessToPptMutex, __func__);

        m_ppt->resetAll(true);
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        m_ppt->resetAll(false);
    }


    void DsscPpt::resetDatapath() {
        DSSC::StateChangeKeeper keeper(this);
        KARABO_LOG_INFO << "resetDatapath";

        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->datapathReset(true);
            boost::this_thread::sleep(boost::posix_time::seconds(1));
            m_ppt->datapathReset(false);
        }

        checkAllIOBStatus();
    }


    void DsscPpt::resetEPC() {
        KARABO_LOG_INFO << "resetEPC";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->epcReset(true);
            boost::this_thread::sleep(boost::posix_time::seconds(1));
            m_ppt->epcReset(false);
        }
        //reprogramm all EPC register to reset to last configuration
        programEPCConfig();
    }


    void DsscPpt::resetIOBs() {
        KARABO_LOG_INFO << "resetIOBs";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);

            //m_ppt->setASICReset(true); this is wrong // ALSO CHECKS IF TEST SYSTEM IN mANNHEIM
            m_ppt->iobReset(true);
            boost::this_thread::sleep(boost::posix_time::seconds(1));
            m_ppt->iobReset(false);
            //m_ppt->setASICReset(false); this is wrong
        }
    }


    void DsscPpt::resetASICs() {
        KARABO_LOG_INFO << "resetASICs";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);

            m_ppt->setASICReset_TestSystem(true); // required in test system important to minimize current consumption
            //m_ppt->iobReset(true); // ???????????????????????? Nonsense. wrong
            m_ppt->setASICReset(true);
            boost::this_thread::sleep(boost::posix_time::seconds(1));
            m_ppt->setASICReset(false);
            //m_ppt->iobReset(false); wrong.
        }
    }


    void DsscPpt::close() {
        this->updateState(State::CLOSING);
        KARABO_LOG_INFO << "close";
        // stop polling before closing the connection
        this->stopPolling();
        int rc = m_ppt->closeConnection();
        if (rc != SuS::DSSC_PPT::ERROR_OK) {
            this->updateState(State::ERROR);
            //throw KARABO_NETWORK_EXCEPTION("PPT close() failure: " + m_ppt->errorString);
        }
        if (m_ppt->isOpen()) {
            this->updateState(State::ERROR);

            //throw KARABO_NETWORK_EXCEPTION("FEM close() failure: attempt to close the existing connection failed");
        }
        this->updateState(State::UNKNOWN);
    }


    void DsscPpt::saveConfiguration() {
        saveEPCRegisters();
        saveConfigIOB();
        saveJTAGRegisters();
        savePixelRegisters();
        saveSequencer();
    }


    void DsscPpt::saveEPCRegisters() {
        string filename = get<string>("epcRegisterFilePath");
        m_ppt->getEPCRegisters()->saveToFile(filename);
    }


    void DsscPpt::saveConfigIOB() {
        string filename = get<string>("iobRegisterFilePath");
        m_ppt->getIOBRegisters()->saveToFile(filename);
    }


    void DsscPpt::saveJTAGRegisters() {
        string filename = get<string>("jtagRegisterFilePath");
        m_ppt->getJTAGRegisters()->saveToFile(filename);
    }


    void DsscPpt::savePixelRegisters() {
        string filename = get<string>("pixelRegisterFilePath");
        m_ppt->getPixelRegisters()->saveToFile(filename);
    }


    void DsscPpt::saveSequencer() {
        string filename = get<string>("sequencerFilePath");
        m_ppt->getSequencer()->saveToFile(filename);
    }


    void DsscPpt::programAllIOBFPGAs() {
        KARABO_LOG_INFO << "Program all available IOB FPGAs";
        DSSC::StateChangeKeeper keeper(this);
        for (int i = 1; i <= 4; i++) {
            programIOBFPGA(i);
        }
        initIOBs();
    }


    void DsscPpt::programIOBFPGA(int iobNumber) {
        if (iobNumber < 1 || iobNumber > 4) {
            KARABO_LOG_ERROR << "Program IOB " << iobNumber << " not possible. Wrong ion number (1-4)";
            return;
        }

        if (isIOBAvailable(iobNumber)) {
            KARABO_LOG_WARN << "IOB " << iobNumber << " was already programmed!";
        }

        KARABO_LOG_INFO << "Programming IOB  FPGA!";

        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->setASICReset(true); //important to minimize current consumption
            m_ppt->programIOBFPGA(iobNumber);
        }
    }


    void DsscPpt::programIOB1FPGA() {
        programIOBFPGA(1);
        resetIOBs();
    }


    void DsscPpt::programIOB2FPGA() {
        programIOBFPGA(2);
        resetIOBs();
    }


    void DsscPpt::programIOB3FPGA() {
        programIOBFPGA(3);
        resetIOBs();
    }


    void DsscPpt::programIOB4FPGA() {
        programIOBFPGA(4);
        resetIOBs();
    }


    void DsscPpt::programAvailableLMKs() {
        DsscScopedLock lock(&m_accessToPptMutex, __func__);
        m_ppt->programLMKs();
    }


    void DsscPpt::programLMK(int iobNumber) {
        CHECK_IOB(iobNumber)

        KARABO_LOG_INFO << "Programing LMK of IOB " + toString(iobNumber);
        m_ppt->setActiveModule(iobNumber);
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->programLMK();
        }

    }


    void DsscPpt::programLMK1() {
        programLMK(1);
    }


    void DsscPpt::programLMK2() {
        programLMK(2);
    }


    void DsscPpt::programLMK3() {
        programLMK(3);
    }


    void DsscPpt::programLMK4() {
        programLMK(4);
    }


    void DsscPpt::checkPRBs(int iobNumber) {
        CHECK_IOB(iobNumber)

        KARABO_LOG_INFO << "Check PRBs";
        int numPRBsfound = 0;
        m_ppt->setActiveModule(iobNumber);
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            numPRBsfound = m_ppt->checkCurrentIOBPRBStatus(false);
        }
        string keyName = "iob" + toString(iobNumber) + "Status.numPRBsFound";
        set<int>(keyName, numPRBsfound);
    }


    void DsscPpt::resetAurora(int iobNumber) {
        CHECK_IOB(iobNumber)

        KARABO_LOG_INFO << "Reset Aurora";
        m_ppt->setActiveModule(iobNumber);
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->auroraTXReset();
        }
        checkIOBAuroraReady(iobNumber);
    }


    void DsscPpt::resetAurora1() {
        resetAurora(1);
        checkPRBs(1);
    }


    void DsscPpt::resetAurora2() {
        resetAurora(2);
        checkPRBs(2);
    }


    void DsscPpt::resetAurora3() {
        resetAurora(3);
        checkPRBs(3);
    }


    void DsscPpt::resetAurora4() {
        resetAurora(4);
        checkPRBs(4);
    }


    void DsscPpt::programEPCConfig() {
        KARABO_LOG_INFO << "Program EPC Config ";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->programEPCRegisters();
        }

        printPPTErrorMessages(true);

        getEPCParamsIntoGui();
    }


    void DsscPpt::programAvailableIOBsConfig() {

        if (m_ppt->activeIOBs.size() == 0) {
            return;
        }

        KARABO_LOG_INFO << "Program IOB " << toString(m_ppt->activeIOBs) << " config";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->programIOBRegisters(); // includes already the readback
        }

        printPPTErrorMessages(true);

        getIOBParamsIntoGui();

    }


    void DsscPpt::programIOBConfig(int iobNumber) {

        CHECK_IOB(iobNumber)

        KARABO_LOG_INFO << "Program IOB Config " << iobNumber;
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->programIOBRegister(to_string(iobNumber)); // includes already the readback
        }

        printPPTErrorMessages(true);

        getIOBParamsIntoGui();
    }


    void DsscPpt::programIOB1Config() {
        programIOBConfig(1);
    }


    void DsscPpt::programIOB2Config() {
        programIOBConfig(2);
    }


    void DsscPpt::programIOB3Config() {
        programIOBConfig(3);
    }


    void DsscPpt::programIOB4Config() {
        programIOBConfig(4);
    }


    void DsscPpt::preProgSelReg() {
        string moduleStr = get<string>("selPixels");
        string selRegStr = get<string>("selRegName");
        string selModSet = (selRegStr == "pixel") ? "Control register" : "Global Control Register";
        string selSigStr = (selRegStr == "pixel") ? get<string>("selPixelSignal") : get<string>("selJtagMainRegSignal");
        uint32_t module = get<uint32_t>("selModule");
        uint32_t value = get<uint32_t>("selValue");

        setActiveModule(module);
        SuS::ConfigReg *configRegister = m_ppt->getRegisters(selRegStr);
        if (configRegister == nullptr) {
            return;
        }

        if (!configRegister->moduleSetExists(selModSet)) {
            KARABO_LOG_ERROR << "ProgSelReg: Given ModuleSet " << selModSet << " invalid";
            return;
        }

        if (!configRegister->signalNameExists(selModSet, selSigStr)) {
            KARABO_LOG_ERROR << "ProgSelReg: Given SignalName " << selSigStr << " invalid";
            return;
        }

        configRegister->setSignalValue(selModSet, moduleStr, selSigStr, value);

        getCoarseGainParamsIntoGui();
    }


    void DsscPpt::progSelReg() {
        preProgSelReg();

        const string selRegStr = get<string>("selRegName");
        string selModSet = (selRegStr == "pixel") ? "Control register" : "Global Control Register";
        const uint32_t module = get<uint32_t>("selModule");

        if (selRegStr.compare("epc") == 0) {
            {
                DsscScopedLock lock(&m_accessToPptMutex, __func__);
                m_ppt->programEPCRegister(selModSet);
            }
        } else if (selRegStr.compare("iob") == 0) {
            if (setActiveModule(module)) {
                DsscScopedLock lock(&m_accessToPptMutex, __func__);
                m_ppt->programIOBRegister(selModSet);
            }
        } else if (selRegStr.compare("jtag") == 0) {
            if (setActiveModule(module)) {
                DsscScopedLock lock(&m_accessToPptMutex, __func__);
                m_ppt->programJtagSingle(selModSet);
            }
        } else if (selRegStr.compare("pixel") == 0) {
            if (setActiveModule(module)) {
                DsscScopedLock lock(&m_accessToPptMutex, __func__);
                m_ppt->programPixelRegs();
            }
        }
    }


    void DsscPpt::readSelReg() {
        SuS::ConfigReg *configRegister = NULL;
        string moduleStr;
        string selRegStr = get<string>("selRegName");
        string selModSet = (selRegStr == "pixel") ? "Control register" : "Global Control Register";
        string selSigStr = (selRegStr == "pixel") ? get<string>("selPixelSignal") : get<string>("selJtagMainRegSignal");
        uint32_t module = get<uint32_t>("selModule");

        if (selRegStr.compare("epc") == 0) {
            configRegister = m_ppt->getEPCRegisters();
            moduleStr = "0";
        } else if (selRegStr.compare("iob") == 0) {
            configRegister = m_ppt->getIOBRegisters();
            moduleStr = to_string(module);
        } else if (selRegStr.compare("jtag") == 0) {
            moduleStr = "0";
            configRegister = m_ppt->getJTAGRegisters();
        } else if (selRegStr.compare("pixel") == 0) {
            configRegister = m_ppt->getPixelRegisters();
            moduleStr = get<string>("selPixels");
        } else {
            KARABO_LOG_ERROR << "Register unknown: valid values 'epc', 'iob', 'jtag' or 'pixel'";
            return;
        }

        if (!configRegister->signalNameExists(selModSet, selSigStr)) {
            KARABO_LOG_ERROR << "ProgSelReg: Given Parameters invalid";
            return;
        }

        set<uint32_t>("selValue", configRegister->getSignalValue(selModSet, moduleStr, selSigStr));

    }


    void DsscPpt::programJTAG() {
        bool readBack = get<bool>("jtagReadBackEnable");
        int iobNumber = get<uint32_t>("activeModule");

        CHECK_IOB(iobNumber)

        if (!checkIOBVoltageEnabled(iobNumber)) {
            KARABO_LOG_WARN << "IOB " + toString(iobNumber) + " static power not enabled!";
            return;
        }

        KARABO_LOG_INFO << "Program ASIC JTAG Chain " + toString(iobNumber);
        m_ppt->setActiveModule(iobNumber);
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            int rc = m_ppt->programJtag(readBack);
        }

        printPPTErrorMessages(true);
    }


    void DsscPpt::programPixelRegisterDefault() {
        int iobNumber = get<uint32_t>("activeModule");
        if (!checkIOBVoltageEnabled(iobNumber)) {
            KARABO_LOG_WARN << "IOB " + toString(iobNumber) + " static power not enabled!";
            return;
        }

        KARABO_LOG_INFO << "Program Pixel Registers to Default Values at IOB " << iobNumber;
        m_ppt->setActiveModule(iobNumber);
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->programPixelRegsAllAtOnce(false);
        }

        printPPTErrorMessages(true);
    }


    void DsscPpt::programPixelRegister() {
        bool readBack = get<bool>("pixelReadBackEnable");
        int iobNumber = get<uint32_t>("activeModule");

        if (!checkIOBVoltageEnabled(iobNumber)) {
            KARABO_LOG_WARN << "IOB " + toString(iobNumber) + " static power not enabled!";
            return;
        }

        KARABO_LOG_INFO << "Program Pixel Registers at IOB " << iobNumber;
        m_ppt->setActiveModule(iobNumber);

        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->programPixelRegs(readBack);
        }

        printPPTErrorMessages(true);
    }


    void DsscPpt::updateSequencer() {
        const int cycleLength = get<unsigned int>("sequencer.cycleLength");
        if (cycleLength != m_ppt->getSequencer()->getCycleLength()) {
            KARABO_LOG_WARN << "Cycle length changed, this should not be done in karabo, load different sequencer file instead";
        }

        const auto integrationTime = get<unsigned int>("sequencer.integrationTime");
        const auto flattopLength = get<unsigned int>("sequencer.flattopLength");
        const auto rampLength = get<unsigned int>("sequencer.rampLength");
        const auto resetLength = get<unsigned int>("sequencer.resetLength");
        const auto resetIntegOffset = get<unsigned int>("sequencer.resetIntegOffset");
        const auto resetHoldLength = get<unsigned int>("sequencer.resetHoldLength");
        const auto flattopHoldLength = get<unsigned int>("sequencer.flattopHoldLength");
        const auto rampIntegOffset = get<unsigned int>("sequencer.rampIntegOffset");
        const auto backFlipAtReset = get<unsigned int>("sequencer.backFlipAtReset");
        const auto backFlipToResetOffset = get<unsigned int>("sequencer.backFlipToResetOffset");
        const auto singleCapLoadLength = get<unsigned int>("sequencer.singleCapLoadLength");
        const auto emptyInjectCycles = get<unsigned int>("sequencer.emptyInjectCycles");

        const auto singleSHCapMode = get<bool>("sequencer.singleSHCapMode");
        if (emptyInjectCycles) {
            cout << "ATTENTION: Empty Inject cycles Activated" << endl;
        }

        m_ppt->getSequencer()->setSingleSHCapMode(singleSHCapMode);
        m_ppt->getSequencer()->setSequencerParameter(SuS::Sequencer::EmptyInjectCycles, emptyInjectCycles, false);

        m_ppt->getSequencer()->generateSignals(integrationTime, flattopLength, flattopHoldLength,
                                               resetLength, resetIntegOffset, resetHoldLength,
                                               rampLength, rampIntegOffset, backFlipAtReset, backFlipToResetOffset, singleCapLoadLength);

        programSequencers();

        getSequencerParamsIntoGui();
    }
    
    void DsscPpt::programSequencers() {
        bool readBack = get<bool>("sequencerReadBackEnable");

        KARABO_LOG_INFO << "Program Sequencers";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->programSequencers(readBack);
        }

        printPPTErrorMessages(true);    
    }


    void DsscPpt::programSequencer() {
        bool readBack = get<bool>("sequencerReadBackEnable");
        int iobNumber = get<uint32_t>("activeModule");

        if (!checkIOBVoltageEnabled(iobNumber)) {
            KARABO_LOG_WARN << "IOB " + toString(iobNumber) + " static power not enabled!";
            return;
        }

        KARABO_LOG_INFO << "Program Sequencer at IOB " << iobNumber;
        m_ppt->setActiveModule(iobNumber);
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->programSequencer(readBack);
        }

        printPPTErrorMessages(true);
    }


    bool DsscPpt::readbackConfigIOB(int iobNumber) {
        CHECK_IOB_B(iobNumber)

        int rc;

        KARABO_LOG_INFO << "Readback IOB " + toString(iobNumber) + " Config Registers";
        m_ppt->setActiveModule(iobNumber);
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            rc = m_ppt->readBackIOBRegister();
        }

        printPPTErrorMessages(true);

        if (rc != SuS::DSSC_PPT::ERROR_OK) {
            KARABO_LOG_ERROR << "Readback IOB Regiser not correct! " + m_ppt->errorString;
            return false;
        }

        return true;
    }


    void DsscPpt::readIOBRegisters1() {
        readIOBRegisters(1);
    }


    void DsscPpt::readIOBRegisters2() {
        readIOBRegisters(2);
    }


    void DsscPpt::readIOBRegisters3() {
        readIOBRegisters(3);
    }


    void DsscPpt::readIOBRegisters4() {
        readIOBRegisters(4);
    }


    int DsscPpt::checkAllIOBStatus() {
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->getAvailableIOBoards();
        }

        int numIOBs = 0;
        for (int i = 1; i <= 4; i++) {
            if (checkIOBReady(i)) {
                numIOBs++;
            }
        }
        return numIOBs;
    }


    bool DsscPpt::checkIOBDataFailed() {
        KARABO_LOG_INFO << "Check IOB Data failed";
        bool allOk = true;

        for (auto && activeIOB : m_ppt->activeIOBs) {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->setActiveModule(activeIOB);
            allOk &= (m_ppt->checkIOBDataFailed() == 0);
            setASICChannelReadoutFailure(activeIOB);
        }
        return allOk;
    }


    void DsscPpt::readLastPPTTrainID() {
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            set<unsigned int>("lastTrainId", m_ppt->getCurrentTrainID());
        }
        KARABO_LOG_INFO << "Read last Train ID: " << get<unsigned int>("lastTrainId");
    }


    bool DsscPpt::checkPPTDataFailed() {
        bool allOk = true;
        for (auto && activeIOB : m_ppt->activeIOBs) {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->setActiveModule(activeIOB);
            unsigned short failed = m_ppt->getActiveChannelFailure();
            set<unsigned short>("pptChannelFailed.failedChannel" + to_string(activeIOB), failed);
            allOk &= (failed == 0);
        }
        return allOk;
    }


    void DsscPpt::checkASICReset() {
        KARABO_LOG_INFO << "Check correct ASIC Reset";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->checkCorrectASICReset();
        }
        if (!checkPPTDataFailed()) {
            KARABO_LOG_WARN << "Some ASICs are still not correctly be initialized";
        } else {
            KARABO_LOG_INFO << "All ASICs could correctly be initialized";
        }

    }


    bool DsscPpt::checkIOBReady(int iobNumber) {
        bool found = isIOBAvailable(iobNumber);

        if (found) {

            KARABO_LOG_INFO << "IOB " + toString(iobNumber) + " Found";
            {
                DsscScopedLock lock(&m_accessToPptMutex, __func__);
                m_ppt->setDPEnabled(iobNumber, true);
            }

            //update Karabo Gui Values
            getIOBSpecialParamsIntoGui(iobNumber);

            string keyName = "enDataPath.dp" + toString(iobNumber) + "Enable";
            set<bool>(keyName, m_ppt->isDPEnabled(iobNumber));

            checkIOBAuroraReady(iobNumber);

        } else {
            KARABO_LOG_WARN << "IOB " + toString(iobNumber) + " not found, probably not programmed";
        }

        set<bool>("iob" + toString(iobNumber) + "Status.iob" + toString(iobNumber) + "Available", found);

        return found;
    }


    void DsscPpt::fillSramAndReadout() {
        unsigned short pattern = get<unsigned short>("sramPattern");
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->fillSramAndReadout(pattern, true);
        }
    }


    bool DsscPpt::checkIOBVoltageEnabled(int iobNumber) {
        bool off = m_ppt->getIOBParam("PRB_control", toString(iobNumber), "PRB_power_off");

        return !off;
    }


    bool DsscPpt::isIOBAvailable(int iobNumber) {
        bool isAvailable;
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            isAvailable = m_ppt->isIOBAvailable(iobNumber);
        }
        return isAvailable;
    }


    void DsscPpt::checkIOBAuroraReady(int iobNumber) {
        KARABO_LOG_INFO << "checkIOBAuroraReady";
        bool ready = false;
        m_ppt->setActiveModule(iobNumber);
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            ready = m_ppt->isAuroraReady();
        }

        set<bool>("iob" + toString(iobNumber) + "Status.iob" + toString(iobNumber) + "Ready", ready);

        if (ready) {
            KARABO_LOG_INFO << "IOB " + toString(iobNumber) + " Aurora Locked, ready to receive data";
        } else {
            KARABO_LOG_WARN << "IOB " + toString(iobNumber) + " Aurora NOT Locked, unable to receive data";
        }

    }


    bool DsscPpt::setActiveModule(int iobNumber) {
        CHECK_IOB_B(iobNumber);

        m_ppt->setActiveModule(iobNumber);
        return true;
    }


    void DsscPpt::updateFirmwareFlash() {
        DSSC::StateChangeKeeper keeper(this);

        stopPolling();

        KARABO_LOG_INFO << "Update PPT Firmware Flash, wait 20 minutes before proceeding";
        string fileName = get<string>("firmwareBinaryName");

        if (m_ppt->checkPPTRevision(fileName)) {
            //Copy File is required to guarantee correct fileName
            utils::fileCopy(fileName, "DSSC_PPT_TOP.bin");

            pptSendFile("DSSC_PPT_TOP.bin");

            {
                KARABO_LOG_INFO << "Updating flash memory...please wait";
                DsscScopedLock lock(&m_accessToPptMutex, __func__);
                m_ppt->sendFlashFirmware();
            }
        }

        if (printPPTErrorMessages()) {
            KARABO_LOG_INFO << "Firmware flash memory successfully updated";
        }
    }


    void DsscPpt::updateLinuxFlash() {
        DSSC::StateChangeKeeper keeper(this);

        stopPolling();

        KARABO_LOG_INFO << "Update PPT Linux Flash, wait 20 minutes before proceeding";
        string fileName = get<string>("linuxBinaryName");

        if (m_ppt->checkLinuxRevision(fileName)) {
            //Copy File is required to guarantee correct fileName
            utils::fileCopy(fileName, "simpleImage.xilinx.bin");

            pptSendFile("simpleImage.xilinx.bin");

            {
                KARABO_LOG_INFO << "Updating flash memory...please wait";
                DsscScopedLock lock(&m_accessToPptMutex, __func__);
                m_ppt->sendFlashLinux();
            }
        }

        if (printPPTErrorMessages()) {
            KARABO_LOG_INFO << "Linux flash memory successfully updated";
        }
    }


    void DsscPpt::updateIOBFirmware() {
        DSSC::StateChangeKeeper keeper(this);

        stopPolling();
        KARABO_LOG_INFO << "Update IOB Firmware";
        string fileName = get<string>("iobFirmwareBitfile");
        //      if(fileName.find("IOB_Firmware.xsvf")==string::npos){
        //        KARABO_LOG_ERROR << "IOB Firmware Bitfile name must exactly be named 'IOB_Firmware.xsvf'";
        //        return;
        //      }

        if (m_ppt->checkIOBRevision(fileName)) {
            //Copy File is required to guarantee correct fileName
            utils::fileCopy(fileName, "IOB_Firmware.xsvf");

            pptSendFile("IOB_Firmware.xsvf");

            {
                DsscScopedLock lock(&m_accessToPptMutex, __func__);
                m_ppt->sendUpdateIOBFirmware();
            }
        }

        if (printPPTErrorMessages()) {
            KARABO_LOG_INFO << "IOB Firmware file successfully updated";
        }
    }


    void DsscPpt::pptSendFile(const string & fileName) {
        KARABO_LOG_INFO << "FTP Send File: " << fileName;
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->sendFileFtp(fileName);
        }

        printPPTErrorMessages();
    }


    void DsscPpt::readEPCRegisters() {
        KARABO_LOG_INFO << "ReadBack EPC Registers";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->readBackEPCRegisters();
        }

        printPPTErrorMessages(true);

        getEPCParamsIntoGui();
    }


    void DsscPpt::readEPCPLLRegisters() {
        KARABO_LOG_INFO << "ReadBack EPC PLL Registers";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->readBackEPCRegister("PLLReadbackRegister");
            m_ppt->readBackEPCRegister("CLOCK_FANOUT_CONTROL");
        }

        printPPTErrorMessages(true);

        getEPCParamsIntoGui("PLLReadbackRegister");
        getEPCParamsIntoGui("CLOCK_FANOUT_CONTROL");
    }


    void DsscPpt::readIOBRegisters() {
        //KARABO_LOG_INFO << "ReadBack IOB Registers";
        for (int i = 1; i <= 4; i++) {
            if (isIOBAvailable(i)) {
                readIOBRegisters(i);
            }
        }
    }


    void DsscPpt::readIOBRegisters(int iobNumber) {
        if (readbackConfigIOB(iobNumber)) {
            getIOBParamsIntoGui();
        }
    }


    void DsscPpt::getCoarseGainParamsIntoGui() {
        set<unsigned int>("gain.fcfEnCap", m_ppt->getPixelRegisterValue("0", "FCF_EnCap"));
        set<unsigned int>("gain.csaFbCap", m_ppt->getPixelRegisterValue("0", "CSA_FbCap"));
        set<unsigned int>("gain.csaResistor", m_ppt->getPixelRegisterValue("0", "CSA_Resistor"));
        set<unsigned int>("gain.qInjEnCs", m_ppt->getPixelRegisterValue("0", "QInjEnCs"));
        set<unsigned int>("gain.csaInjCap", m_ppt->getPixelRegisterValue("0", "QInjEn10fF"));
        set<bool>("gain.csaInjCap200", m_ppt->getPixelRegisterValue("0", "CSA_Cin_200fF") != 0);
        set<unsigned int>("gain.integrationTime", m_ppt->sequencer->getIntegrationTime(true));


        string value;
        if (m_ppt->getPixelRegisters()->signalIsVarious("Control register", "RmpFineTrm", "all")) {
            value = "Various";
        } else {
            value = to_string(m_ppt->getPixelRegisterValue("0", "RmpFineTrm"));
        }
        set<string>("gain.irampFineTrm", value);

        if (m_ppt->getPixelRegisters()->signalIsVarious("Control register", "RmpDelayCntrl", "all")) {
            value = "Various";
        } else {
            value = to_string(m_ppt->getPixelRegisterValue("0", "RmpDelayCntrl"));
        }
        set<string>("gain.pixelDelay", value);


        if (m_ppt->getPixelRegisters()->signalIsVarious("Control register", "RmpCurrDouble", "all")) {
            value = "Various";
        } else {
            value = to_string(m_ppt->getPixelRegisterValue("0", "RmpCurrDouble"));
        }
        set<string>("gain.rmpCurrDouble", value);
    }


    void DsscPpt::getEPCParamsIntoGui(const string & moduleSetName) {

    }


    void DsscPpt::getEPCParamsIntoGui() {

        updateGuiEnableDatapath();

        updateGuiPLLParameters();

        updateGuiOtherParameters();


    }


    void DsscPpt::getIOBTempIntoGui(int iobNumber) {
        m_ppt->setActiveModule(iobNumber);
        string keyName = "iob" + toString(iobNumber) + "Status.iobTemp";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            set<int>(keyName, m_ppt->readIOBTemperature_TestSystem());
        }
    }


    void DsscPpt::getIOBSerialIntoGui(int iobNumber) {
        //if(m_ppt->numAvailableIOBs() == 0) return;
        string serialkey = "iob" + toString(iobNumber) + "Status.iob" + toString(iobNumber) + "Serial";
        string buildkey = "iob" + toString(iobNumber) + "Status.iob" + toString(iobNumber) + "BuiltStamp";

        m_ppt->setActiveModule(iobNumber);
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);

            string iobSerial = m_ppt->getIOBSerial();
            set<string>(serialkey, iobSerial);
            string iobBuild = m_ppt->readIOBBuildStamp();
            set<string>(buildkey, iobBuild);
        }
    }


    void DsscPpt::getNumPRBsIntoGui(int iobNumber) {
        int numPRBsFound = m_ppt->getIOBParam("PRB_control", toString(iobNumber), "PRB_num_prbs_found");
        string key = "iob" + toString(iobNumber) + "Status.numPRBsFound";
        set<int>(key, numPRBsFound);
    }


    void DsscPpt::setASICChannelReadoutFailure(int iobNumber) {
        unsigned short readoutFail = m_ppt->checkIOBDataFailed();
        string key = "iob" + toString(iobNumber) + "Status.asicChannelReadoutFailure";
        set<string>(key, utils::bitEnableValueToString(readoutFail));
        set<unsigned short>("activeChannelReadoutFailure", readoutFail);
    }


    void DsscPpt::getIOBParamsIntoGui() {
        int iob = 1;
        if (!m_iobCurrIOBNumber.compare("all") == 0) {
            iob = INT_CAST(m_iobCurrIOBNumber);
        }

        getIOBParamsIntoGui(iob);

        getIOBSpecialParamsIntoGui(iob);

    }


    void DsscPpt::getIOBSpecialParamsIntoGui(int iobNumber) {
        getIOBSerialIntoGui(iobNumber);

        getNumPRBsIntoGui(iobNumber);

        setASICChannelReadoutFailure(iobNumber);

    }


    void DsscPpt::getIOBParamsIntoGui(int iobNumber) {
        Hash hash = this->getCurrentConfiguration("IOBConfig");
        vector<string> paths;
        hash.getPaths(paths);
        Hash tmp;


        BOOST_FOREACH(string path, paths) {
            vector<string> tokens;
            boost::split(tokens, path, boost::is_any_of("."));
            if (tokens.size() == 3) {

                uint32_t value = m_ppt->getIOBParam(tokens[1], toString(iobNumber), tokens[2]);

                //KARABO_LOG_DEBUG << "IOBParam " + path + ": " + toString(value);

                //update only if not _nc signal"
                if (tokens[2].find("_nc") == string::npos)
                    tmp.set(path, value);
            }
        }
        set(tmp);
    }


    void DsscPpt::getJTAGParamsIntoGui() {
        string iobNumber = m_jtagCurrIOBNumber;
        if (iobNumber.compare("all") == 0) {
            iobNumber = "0";
        }

        Hash hash = this->getCurrentConfiguration("JTAGConfig");

        vector<string> paths;
        hash.getPaths(paths);
        Hash tmp;


        BOOST_FOREACH(string path, paths) {
            vector<string> tokens;
            boost::split(tokens, path, boost::is_any_of("."));
            if (tokens.size() == 3) {

                uint32_t value = m_ppt->getJTAGParam(tokens[1], iobNumber, tokens[2]);

                //KARABO_LOG_DEBUG << "JTAGParam " + path + ": " + toString(value);

                //update only if not _nc signal"
                if (tokens[2].find("_nc") == string::npos)
                    tmp.set(path, value);
            }
        }
        set(tmp);
    }


    void DsscPpt::getPixelParamsIntoGui() {
        string iobNumber = "0";

        Hash hash = this->getCurrentConfiguration("PixelConfig");

        vector<string> paths;
        hash.getPaths(paths);
        Hash tmp;


        BOOST_FOREACH(string path, paths) {
            vector<string> tokens;
            boost::split(tokens, path, boost::is_any_of("."));
            if (tokens.size() == 3) {

                uint32_t value = m_ppt->getPixelParam(tokens[1], iobNumber, tokens[2]);

                //KARABO_LOG_DEBUG << "PixelParam " + path + ": " + toString(value);

                //update only if not _nc signal"
                if (tokens[2].find("_nc") == string::npos)
                    tmp.set(path, value);
            }
        }
        set(tmp);
        updateGuiMeasurementParameters();
    }


    void DsscPpt::getSequencerParamsIntoGui() {
        //KARABO_LOG_INFO << "Load Sequencer parameters into Gui";

        const auto seq = m_ppt->getSequencer();

        set<string>("sequencer.opMode", SuS::Sequencer::getOpModeStr(seq->mode));

        set<unsigned int>("sequencer.cycleLength", seq->getCycleLength());
        set<unsigned int>("sequencer.integrationTime", seq->integrationLength);
        set<unsigned int>("sequencer.flattopLength", seq->flattopLength);
        set<unsigned int>("sequencer.rampLength", seq->rampLength);
        set<unsigned int>("sequencer.resetLength", seq->resetLength);
        set<unsigned int>("sequencer.resetIntegOffset", seq->resetIntegOffset);
        set<unsigned int>("sequencer.resetHoldLength", seq->resetHoldLength);
        set<unsigned int>("sequencer.flattopHoldLength", seq->flattopHoldLength);
        set<unsigned int>("sequencer.rampIntegOffset", seq->rampIntegOffset);
        set<unsigned int>("sequencer.backFlipAtReset", seq->backFlipAtReset);
        set<unsigned int>("sequencer.backFlipToResetOffset", seq->backFlipToResetOffset);
        set<unsigned int>("sequencer.singleCapLoadLength", seq->singleCapLoadLength);
        set<unsigned int>("sequencer.injectRisingEdgeOffset", seq->injectRisingEdgeOffset);
        set<unsigned int>("sequencer.emptyInjectCycles", seq->emptyInjectCycles);
        set<bool>("sequencer.singleSHCapMode", seq->singleSHCapMode);
    }


    void DsscPpt::loadLastFileETHConfig() {
        m_ppt->enableLastLoadedETHConfig();
        getEthernetConfigIntoGui();
    }


    void DsscPpt::getEthernetConfigIntoGui() {
        string keyString = "qsfp.chan";
        for (int i = 1; i < 5; i++) {

            string recvKeyString = keyString + toString(i) + ".recv.";

            string sIp = m_ppt->getETHIP(i, true);
            set<string>(recvKeyString + "ipaddr", sIp);

            string sMac = m_ppt->getETHMAC(i, true);
            set<string>(recvKeyString + "macaddr", sMac);

            uint16_t sPort = m_ppt->getETHPort(i, true);
            set<int>(recvKeyString + "port", sPort);

            string sendKeyString = keyString + toString(i) + ".send.";

            string rIp = m_ppt->getETHIP(i, false);
            set<string>(sendKeyString + "ipaddr", rIp);

            string rMac = m_ppt->getETHMAC(i, false);
            set<string>(sendKeyString + "macaddr", rMac);

            uint16_t rPort = m_ppt->getETHPort(i, false);
            set<int>(sendKeyString + "port", rPort);
        }
    }


    void DsscPpt::setQSFPEthernetConfig() {
        // Receiver
        for (int i = 1; i < 5; i++) {
            m_ppt->setETHMAC(i, true, get<string>("qsfp.chan" + to_string(i) + ".recv.macaddr"), false);
            m_ppt->setETHIP(i, true, get<string>("qsfp.chan" + to_string(i) + ".recv.ipaddr"), false);
            m_ppt->setETHPort(i, true, get<unsigned int>("qsfp.chan" + to_string(i) + ".recv.port"), false);

            cout << "Receiver IP: " << get<string>("qsfp.chan" + to_string(i) + ".recv.ipaddr") << endl;
        }

        // Sender
        for (int i = 1; i < 5; i++) {
            m_ppt->setETHMAC(i, false, get<string>("qsfp.chan" + to_string(i) + ".send.macaddr"), false);
            m_ppt->setETHIP(i, false, get<string>("qsfp.chan" + to_string(i) + ".send.ipaddr"), false);
            m_ppt->setETHPort(i, false, get<unsigned int>("qsfp.chan" + to_string(i) + ".send.port"), false);
        }

        getEthernetConfigIntoGui();
    }


    void DsscPpt::getSequenceCountersIntoGui() {
        set<int>("sequence.start_wait_time", m_ppt->getBurstParam("start_wait_time"));
        set<int>("sequence.start_wait_offs", m_ppt->getBurstParam("start_wait_offs"));
        set<int>("sequence.gdps_on_time", m_ppt->getBurstParam("gdps_on_time"));
        set<int>("sequence.iprogLength", m_ppt->getJTAGParam("Master FSM Config Register", "all", "Iprog Length") + 1);
        set<int>("sequence.burstLength", m_ppt->getJTAGParam("Master FSM Config Register", "all", "Burst Length") + 1);
        set<int>("sequence.refpulseLength", m_ppt->getJTAGParam("Master FSM Config Register", "all", "Refpulse Length") + 1);
        set<int>("sequence.fet_on_time", m_ppt->getBurstParam("fet_on_time"));
        set<int>("sequence.clr_on_time", m_ppt->getBurstParam("clr_on_time"));
        set<int>("sequence.iprog_clr_offset", m_ppt->getBurstParam("iprog_clr_offset"));
        set<int>("sequence.iprog_clr_duty", m_ppt->getBurstParam("iprog_clr_duty"));
        set<int>("sequence.iprog_clr_en", m_ppt->getBurstParam("iprog_clr_en"));
        set<int>("sequence.clr_cycle", m_ppt->getBurstParam("clr_cycle"));
        set<int>("sequence.clrDuty", m_ppt->getIOBParam("CLR_duty", "1", "CLR_duty") - 1);
        set<int>("sequence.SW_PWR_ON", m_ppt->getBurstParam("SW_PWR_ON"));
    }


    void DsscPpt::updateGuiEnableDatapath() {
        uint8_t enOneHot = 0;
        for (int i = 1; i < 5; i++) {
            string keyName = "enDataPath.dp" + toString(i) + "Enable";
            bool en = m_ppt->isDPEnabled(i);
            if (en) {
                enOneHot += 1 << (i - 1);
            }
            set<bool>(keyName, en);
        }
        set<unsigned short>("enableDPChannels", enOneHot);
        KARABO_LOG_DEBUG << "OneHotvalue = " << enOneHot;
    }


    void DsscPpt::updateGuiPLLParameters() {
        bool chip_pll_locked = BOOL_CAST(m_ppt->getEPCParam("PLLReadbackRegister", "0", "PLL_LD"));
        bool fpga_pll_locked = BOOL_CAST(m_ppt->getEPCParam("CLOCK_FANOUT_CONTROL", "0", "mmcm_locked"));
        bool internal_Pll_used = BOOL_CAST(m_ppt->getEPCParam("PLLReadbackRegister", "0", "CLOUT_SEL"));
        bool xfelClock = !BOOL_CAST(m_ppt->getEPCParam("CLOCK_FANOUT_CONTROL", "0", "CLIN_SEL"));

        if (internal_Pll_used) {
            set<bool>("pptPLL.locked", fpga_pll_locked);
            if (!fpga_pll_locked) {
                KARABO_LOG_WARN << "PPT PLL not locked! Reprogram PLL";
            }
        } else {
            set<bool>("pptPLL.locked", chip_pll_locked);
            if (!chip_pll_locked) {
                KARABO_LOG_WARN << "PPT PLL not locked! Reprogram PLL";
            }
        }

        set<bool>("pptPLL.internalPLL", internal_Pll_used);
        set<bool>("pptPLL.XFELClk", xfelClock);
    }


    void DsscPpt::updateGuiOtherParameters() {
        bool value = m_ppt->getEPCParam("DataRecv_to_Eth0_Register", "0", "send_dummy_packets") == 1;
        set<bool>("send_dummy_packets", value);

        value = m_ppt->getEPCParam("AuroraRX_Control", "0", "send_dummy_dr_data") == 1;
        set<bool>("send_dummy_dr_data", value);

        value = m_ppt->getEPCParam("AuroraRX_Control", "0", "send_raw_data") == 1;
        set<bool>("send_raw_data", value);

        value = m_ppt->getIOBParam("ASIC_send_dummy_data", "0", "ASIC_send_dummy_data") == 1;
        set<bool>("ASIC_send_dummy_data", value);

        uint32_t throttleratio = m_ppt->getEthernetOutputThrottleDivider();
        set<uint32_t>("ethThrottleDivider", throttleratio);

        /*
        value = BOOL_CAST(m_ppt->getEPCParam("Single_Cycle_Register","0","disable_sending"));
        set<bool>("disable_sending", value);
        value = BOOL_CAST(m_ppt->inContinuousMode());
        set<bool>("continuous_mode", value);

        value = BOOL_CAST(m_ppt->getEPCParam("DataRecv_to_Eth0_Register","0","clone_eth0_to_eth1"));
        set<bool>("clone_eth0_to_eth1", value);

        unsigned int numFrames = m_ppt->getEPCParam("AuroraRX_Control","0","num_frames_to_send");
        set<unsigned int>("numFramesToSendOut", numFrames);

        unsigned int numVetos = m_ppt->getBurstVetoOffset();
        set<unsigned int>("numPreBurstVetos", numVetos);

        string prbActivePowers = m_ppt->getPRBPowerSelect();
        set<string>("selPRBActivePowers", prbActivePowers);
         */
    }


    void DsscPpt::updateGuiMeasurementParameters() {
        bool enD0Mode = m_ppt->getD0Mode();
        set<bool>("enD0Mode", enD0Mode);

        bool bypCompr = m_ppt->getByPassMode();
        set<bool>("bypassCompression", bypCompr);
    }


    void DsscPpt::configure() {
        DSSC::StateChangeKeeper keeper(this);
        KARABO_LOG_INFO << "Configure System";
        int rc = SuS::DSSC_PPT::ERROR_OK;
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            rc = m_ppt->initSystem();
        }
        if (rc == SuS::DSSC_PPT::ERROR_IOB_NOT_FOUND) {
            KARABO_LOG_ERROR << "No IOB Found, init IOBs before init system";
        } else if (rc != SuS::DSSC_PPT::ERROR_OK) {
            throw KARABO_IO_EXCEPTION("PPT upload failure: " + m_ppt->errorString);
        }

        if (printPPTErrorMessages()) {
            KARABO_LOG_INFO << "Configuration done!";
        }
    }


    void DsscPpt::startManualMode() {
        KARABO_LOG_INFO << "Start Stand Alone Mode";
        set<bool>("xfelMode", false);
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->enableXFELControl(false);
        }
        getEPCParamsIntoGui("Multi_purpose_Register");
        updateGuiPLLParameters();
    }


    void DsscPpt::stopManualMode() {
        KARABO_LOG_INFO << "Start XFEL Mode";
        set<bool>("xfelMode", true);
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->enableXFELControl(true);
        }

        getEPCParamsIntoGui("Multi_purpose_Register");
        updateGuiPLLParameters();
    }


    void DsscPpt::startManualReadout() {
        KARABO_LOG_INFO << "Start manual readout";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->startSingleReadout();
        }
    }



    void DsscPpt::startManualBurstBtn() {
        KARABO_LOG_INFO << "Start manual burst";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->startBurst();
        }
    }


    void DsscPpt::readoutTestPattern() {
        KARABO_LOG_INFO << "Start readout TestPattern";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->startTestPattern();
        }
    }


    void DsscPpt::setIntDACMode() {
        DSSC::StateChangeKeeper keeper(this);

        KARABO_LOG_INFO << "Enable Internal DAC Mode";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->setIntDACMode();
        }
    }


    void DsscPpt::setNormalMode() {
        DSSC::StateChangeKeeper keeper(this);

        KARABO_LOG_INFO << "Enable Normal Mode";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->setNormalMode();
        }
    }


    void DsscPpt::setPixelInjectionMode() {
        DSSC::StateChangeKeeper keeper(this);

        KARABO_LOG_INFO << "Enable Pixel Injection Mode";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->setPixelInjectionMode();
        }
    }


    void DsscPpt::setInjectionMode() {
        DSSC::StateChangeKeeper keeper(this);

        const string injectionModeStr = get<string>("injectionMode");
        KARABO_LOG_INFO << "Enable InjectionMode " << injectionModeStr;
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->setInjectionMode(m_ppt->getInjectionMode(injectionModeStr));
        }

    }


    void DsscPpt::setInjectionValue() {
        DSSC::StateChangeKeeper keeper(this);
        const auto value = get<unsigned int>("injectionValue");
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->setInjectionDAC(value);
        }

        getJTAGParamsIntoGui();

        KARABO_LOG_INFO << "Injection DAC: " << m_ppt->getInjectionModeName(m_ppt->getInjectionMode()) << " set to " << value;
    }


    void DsscPpt::calibrateCurrentCompDac() {
        KARABO_LOG_WARN << "Current Comp Dac Calibration not implemented yet";
    }


    void DsscPpt::calibrateIramp() {
        KARABO_LOG_WARN << "Iramp Calibration not implemented yet";
    }


    void DsscPpt::checkDACCalibration() {
        KARABO_LOG_WARN << "Check not implemented yet";
    }


    void DsscPpt::checkIrampCalibration() {
        KARABO_LOG_WARN << "Check not implemented yet";
    }


    void DsscPpt::programLMKOutput() {
        DSSC::StateChangeKeeper keeper(this);

        const uint32_t lmkNumber = get<uint32_t>("lmkOutputToProgram");
        const uint32_t module = get<uint32_t>("activeModule");

        KARABO_LOG_INFO << "Update LMK Output for ASIC " << lmkNumber << " on iob " << module;
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            m_ppt->setActiveModule(module);
            m_ppt->programLMKASIC(lmkNumber, true, false);
            m_ppt->programLMKASIC(lmkNumber, false, false);
            m_ppt->resetDataFailRegister();
        }
    }


    void DsscPpt::setBurstParameter() {
        const auto paramName = get<string>("burstParameterName");
        const auto paramValue = get<int>("burstParameterValue");

        const auto burstParamNames = m_ppt->getBurstParamNames();
        if (std::find(burstParamNames.begin(), burstParamNames.end(), paramName) == burstParamNames.end()) {
            return;
        }
        string fieldName = "sequence." + paramName;

        set<int>(fieldName, paramValue);

        KARABO_LOG_INFO << "Set Burst Param Value : " << paramName << " = " << paramValue;
    }


    void DsscPpt::setSequencerParameter() {
        const auto paramName = get<string>("sequencerParameterName");
        const auto paramValue = get<int>("sequencerParameterValue");

        m_ppt->getSequencer()->setSequencerParameter(paramName, paramValue, true);

        string fieldName = "sequencer." + paramName;

        set<int>(fieldName, paramValue);

        KARABO_LOG_INFO << "Set Sequencer Param Value : " << paramName << " = " << paramValue;
    }


    void DsscPpt::updateStartWaitOffset() {
        int start_wait_offset = get<int>("sequence.start_wait_offs");

        DsscScopedLock lock(&m_accessToPptMutex, __func__);

        m_ppt->updateStartWaitOffset(start_wait_offset);
    }


    void DsscPpt::updateSequenceCounters() {
        DSSC::StateChangeKeeper keeper(this);
        const int start_wait_time = get<int>("sequence.start_wait_time");
        const int start_wait_offs = get<int>("sequence.start_wait_offs");
        const int gdps_on_time = get<int>("sequence.gdps_on_time");
        const int iprogLength = get<int>("sequence.iprogLength") - 1;
        const int burstLength = get<int>("sequence.burstLength") - 1;
        const int refpulseLength = get<int>("sequence.refpulseLength") - 1;
        const int fet_on_time = get<int>("sequence.fet_on_time");
        const int clr_on_time = get<int>("sequence.clr_on_time");
        const int clr_cycle = get<int>("sequence.clr_cycle");
        const int clrDuty = get<int>("sequence.clrDuty") + 1;
        const int iprog_clr_duty = get<int>("sequence.iprog_clr_duty");
        const int iprog_clr_offset = get<int>("sequence.iprog_clr_offset");
        const int iprog_clr_en = get<int>("sequence.iprog_clr_en");
        const int SW_PWR_ON = get<int>("sequence.SW_PWR_ON");

        KARABO_LOG_INFO << "Update Sequence Counters";
        {
            {
                DsscScopedLock lock(&m_accessToPptMutex, __func__);

                m_ppt->updateCounterValues(start_wait_time, start_wait_offs, gdps_on_time, iprogLength, burstLength, refpulseLength,
                                           fet_on_time, clr_on_time, clr_cycle, clrDuty, SW_PWR_ON,
                                           iprog_clr_duty, iprog_clr_offset, iprog_clr_en, false);
            }
        }

        getIOBParamsIntoGui();

        getJTAGParamsIntoGui();

        getSequencerParamsIntoGui();
    }


    void DsscPpt::errorFound() {

    }


    void DsscPpt::preReconfigure(karabo::util::Hash & incomingReconfiguration) {

        preReconfigureEPC(incomingReconfiguration);

        preReconfigureETH(incomingReconfiguration);

        preReconfigureEnableDatapath(incomingReconfiguration);

        preReconfigureEnablePLL(incomingReconfiguration);

        preReconfigureEnableOthers(incomingReconfiguration);

        preReconfigureEnableMeasurement(incomingReconfiguration);

        preReconfigureIOB(incomingReconfiguration);

        preReconfigureFullConfig(incomingReconfiguration);

        preReconfigureLoadEPCConfig(incomingReconfiguration);

        preReconfigureLoadIOBConfig(incomingReconfiguration);

        preReconfigureLoadASICConfig(incomingReconfiguration);

        preReconfigureJTAG(incomingReconfiguration);

        preReconfigureRegAccess(incomingReconfiguration);
    }


    void DsscPpt::preReconfigureEPC(karabo::util::Hash & incomingReconfiguration) {
        Hash filtered = this->filterByTags(incomingReconfiguration, m_epcTag);
        vector<string> paths;
        filtered.getPaths(paths);


        BOOST_FOREACH(string path, paths) {
            vector<string> tokens;
            boost::split(tokens, path, boost::is_any_of("."));
            if (tokens.size() == 3) {
                int rc = m_ppt->setEPCParam(tokens[1], "0", tokens[2], filtered.getAs<uint32_t>(path));
                if (rc != SuS::DSSC_PPT::ERROR_OK)
                    KARABO_LOG_WARN << "Failure while setting " << path << " : " << m_ppt->errorString;
            }
        }
    }


    void DsscPpt::preReconfigurePixel(karabo::util::Hash & incomingReconfiguration) {
        Hash filtered = this->filterByTags(incomingReconfiguration, "PixelConfig");
        vector<string> paths;
        filtered.getPaths(paths);


        BOOST_FOREACH(string path, paths) {
            vector<string> tokens;
            boost::split(tokens, path, boost::is_any_of("."));
            if (tokens.size() == 3) {
                int rc = m_ppt->setPixelParam(tokens[1], "0", tokens[2], filtered.getAs<uint32_t>(path));
                if (rc != SuS::DSSC_PPT::ERROR_OK)
                    KARABO_LOG_WARN << "Failure while setting " << path << " : " << m_ppt->errorString;
            }
        }
    }


    void DsscPpt::preReconfigureJTAG(karabo::util::Hash & incomingReconfiguration) {
        Hash filtered = this->filterByTags(incomingReconfiguration, "JTAGConfig");
        vector<string> paths;
        filtered.getPaths(paths);


        BOOST_FOREACH(string path, paths) {
            vector<string> tokens;
            boost::split(tokens, path, boost::is_any_of("."));
            if (tokens.size() == 3) {
                int rc = m_ppt->setJTAGParam(tokens[1], m_jtagCurrIOBNumber, tokens[2], filtered.getAs<uint32_t>(path));
                if (rc != SuS::DSSC_PPT::ERROR_OK)
                    KARABO_LOG_WARN << "Failure while setting " << path << " : " << m_ppt->errorString;
            } else if (tokens.size() == 2) {
                m_jtagCurrIOBNumber = filtered.getAs<string>(path);
                getJTAGParamsIntoGui();
                KARABO_LOG_INFO << "JTAG IOB Changed to " << m_jtagCurrIOBNumber;
            }
        }
    }


    void DsscPpt::preReconfigureIOB(karabo::util::Hash & incomingReconfiguration) {
        Hash filtered = this->filterByTags(incomingReconfiguration, "IOBConfig");
        vector<string> paths;
        filtered.getPaths(paths);


        BOOST_FOREACH(string path, paths) {
            vector<string> tokens;
            boost::split(tokens, path, boost::is_any_of("."));
            if (tokens.size() == 3) {
                int rc = m_ppt->setIOBParam(tokens[1], m_iobCurrIOBNumber, tokens[2], filtered.getAs<uint32_t>(path));
                if (rc != SuS::DSSC_PPT::ERROR_OK)
                    KARABO_LOG_WARN << "Failure while setting " << path << " : " << m_ppt->errorString;
            } else if (tokens.size() == 2) {
                m_iobCurrIOBNumber = filtered.getAs<string>(path);
                getIOBParamsIntoGui();
                KARABO_LOG_INFO << "IOB Changed to " << m_iobCurrIOBNumber;
            }
        }
    }


    void DsscPpt::preReconfigureETH(karabo::util::Hash & incomingReconfiguration) {


        Hash filtered = this->filterByTags(incomingReconfiguration, "ethParam");
        vector<string> paths;
        filtered.getPaths(paths);

        if (!paths.empty()) {
            std::vector<int> channelsVec;


            BOOST_FOREACH(string path, paths) {
                vector<string> tokens;
                boost::split(tokens, path, boost::is_any_of("."));

                int channel = INT_CAST(tokens[1].at(4));
                channelsVec.push_back(channel);
                bool recv = std::strcmp(tokens[2].c_str(), "recv") == 0;

                if (std::strcmp(tokens[3].c_str(), "macaddr") == 0) {
                    string value = filtered.getAs<string>(path);
                    m_ppt->setETHMAC(channel, recv, value);
                } else if (std::strcmp(tokens[3].c_str(), "ipaddr") == 0) {
                    string value = filtered.getAs<string>(path);
                    m_ppt->setETHIP(channel, recv, value);
                    cout << recv << " Is receiver Preconfig IP, channel " << channel << ": " << value << endl;
                } else if (std::strcmp(tokens[3].c_str(), "port") == 0) {
                    int value = filtered.getAs<int>(path);
                    m_ppt->setETHPort(channel, recv, value);
                } else {
                    KARABO_LOG_ERROR << "Failure while setting " << path << " : Wrong signal";
                }
            }

            //remove double entries
            channelsVec.erase(std::unique(channelsVec.begin(), channelsVec.end()), channelsVec.end());

            {
                DsscScopedLock lock(&m_accessToPptMutex, __func__);
                for (size_t i = 0; i < channelsVec.size(); i++) {
                    m_ppt->programEPCRegister("10GE_Engine" + toString(channelsVec.at(i)) + "_Control");
                }
            }

            for (size_t i = 0; i < channelsVec.size(); i++) {
                getEPCParamsIntoGui("10GE_Engine" + toString(channelsVec.at(i)) + "_Control");
            }
        }
    }


    void DsscPpt::preReconfigureEnableDatapath(karabo::util::Hash & incomingReconfiguration) {
        Hash filtered = this->filterByTags(incomingReconfiguration, "enableDatapath");
        vector<string> paths;
        filtered.getPaths(paths);

        if (!paths.empty()) {


            BOOST_FOREACH(string path, paths) {
                vector<string> tokens;
                boost::split(tokens, path, boost::is_any_of("."));

                int channel = INT_CAST(tokens[1].at(2));
                bool enable = filtered.getAs<bool>(path);
                {
                    DsscScopedLock lock(&m_accessToPptMutex, __func__);
                    m_ppt->setDPEnabled(channel, enable);
                }
            }
            getEPCParamsIntoGui("Multi_purpose_Register");
        }
    }


    void DsscPpt::preReconfigureEnablePLL(karabo::util::Hash & incomingReconfiguration) {
        Hash filtered = this->filterByTags(incomingReconfiguration, "PLL");
        vector<string> paths;
        filtered.getPaths(paths);

        if (!paths.empty()) {

            KARABO_LOG_INFO << "Pre Reconfigure PLL";

            programPLL();
            readEPCPLLRegisters();
        }
    }


    void DsscPpt::preReconfigureEnableOthers(karabo::util::Hash & incomingReconfiguration) {
        Hash filtered = this->filterByTags(incomingReconfiguration, "other");
        vector<string> paths;
        filtered.getPaths(paths);

        if (!paths.empty()) {


            BOOST_FOREACH(string path, paths) {
                if (path.compare("numFramesToSendOut") == 0) {
                    unsigned int numFrames = filtered.getAs<unsigned int>(path);
                    {
                        DsscScopedLock lock(&m_accessToPptMutex, __func__);
                        m_ppt->setNumFramesToSend(numFrames);
                    }
                } else if (path.compare("ethernetOutputRate") == 0) {
                    unsigned int megabits = filtered.getAs<unsigned int>(path);
                    {
                        DsscScopedLock lock(&m_accessToPptMutex, __func__);
                        m_ppt->setEthernetOutputDatarate(megabits);
                    }
                } else if (path.compare("numPreBurstVetos") == 0) {
                    unsigned int numVetos = filtered.getAs<unsigned int>(path);
                    cout << "numPreBurstVetos changed" << numVetos << endl;
                    {
                        DsscScopedLock lock(&m_accessToPptMutex, __func__);
                        m_ppt->setBurstVetoOffset(numVetos);
                    }
                } else if (path.compare("selEnvironment") == 0) {
                    string setupName = filtered.getAs<string>(path);
                    updateTestEnvironment(setupName);
                } else if (path.compare("selPRBActivePowers") == 0) {
                    DsscScopedLock lock(&m_accessToPptMutex, __func__);
                    m_ppt->setPRBPowerSelect(filtered.getAs<string>(path), true);
                } else if (path.compare("numActiveASICs") == 0) {
                    int numASICs = filtered.getAs<int>(path);
                    DsscScopedLock lock(&m_accessToPptMutex, __func__);
                    m_ppt->setNumberOfActiveAsics(numASICs);


                } else if (path.compare("lmkOutputToProgram") == 0) {

                } else if (path.compare("enableDPChannels") == 0) {
                    uint16_t dpEn = filtered.getAs<unsigned short>(path);
                    enableDPChannels(dpEn);
                } else {

                    bool enable = filtered.getAs<bool>(path);
                    if (path.compare("xfelMode") == 0) {
                        {
                            DsscScopedLock lock(&m_accessToPptMutex, __func__);
                            m_ppt->enableXFELControl(enable);
                        }
                    } else if (path.compare("disable_sending") == 0) {
                        {
                            KARABO_LOG_INFO << (enable ? "Disable Data Sending" : "Enable Data Sending");
                            bool disable = enable;
                            DsscScopedLock lock(&m_accessToPptMutex, __func__);
                            m_ppt->disableSending(disable);
                            //  if(disable){
                            //    this->updateState(karabo::util::State::STARTED);
                            //  }else{
                            //    this->updateState(karabo::util::State::ACQUIRING);
                            //  }
                        }
                    } else if (path.compare("continuous_mode") == 0) {
                        {
                            KARABO_LOG_INFO << (enable ? "Enable Continuous Mode" : "Disable Continuous Mode");
                            DsscScopedLock lock(&m_accessToPptMutex, __func__);
                            m_ppt->runContinuousMode(enable);
                            //  m_ppt->disableSending(true);
                            //  if(enable){
                            //    this->updateState(karabo::util::State::STARTED);
                            //  }else{
                            //    this->updateState(karabo::util::State::ON);
                            //  }
                        }
                    } else if (path.compare("clone_eth0_to_eth1") == 0) {
                        DsscScopedLock lock(&m_accessToPptMutex, __func__);
                        m_ppt->setEPCParam("DataRecv_to_Eth0_Register", "0", "clone_eth0_to_eth1", enable);
                        m_ppt->programEPCRegister("DataRecv_to_Eth0_Register");
                    } else if (path.compare("send_dummy_packets") == 0) {
                        {
                            DsscScopedLock lock(&m_accessToPptMutex, __func__);
                            m_ppt->enableDummyPackets(enable);
                        }
                    } else if (path.compare("send_dummy_dr_data") == 0) {
                        if (!m_ppt->singleDPEnabled()) {
                            KARABO_LOG_WARN << "No Datapath enabled, enable at least one datapath to recieve dummy data from datareciever";
                        }
                        {
                            DsscScopedLock lock(&m_accessToPptMutex, __func__);
                            m_ppt->enableDummyDRData(enable);
                        }
                    } else if (path.compare("send_raw_data") == 0) {
                        {
                            DsscScopedLock lock(&m_accessToPptMutex, __func__);
                            m_ppt->setSendRawData(enable, false, !enable);
                        }
                    } else if (path.compare("ASIC_send_dummy_data") == 0) {

                        if (!m_ppt->singleDPEnabled()) {
                            KARABO_LOG_WARN << "No Datapath enabled, enable at least one datapath to recieve dummy data from IOBoard";
                        }

                        int iobFoundCnt = 0;

                        for (int i = 1; i <= 4; i++) {
                            if (isIOBAvailable(i)) {

                                iobFoundCnt++;
                                m_ppt->setActiveModule(i);
                                {
                                    DsscScopedLock lock(&m_accessToPptMutex, __func__);
                                    m_ppt->enableDummyAsicData(enable);
                                }
                            }
                        }

                        if (iobFoundCnt == 0) {
                            KARABO_LOG_ERROR << "No IOB Found, program IOBoards before activating dummy data.";
                        } else {
                            getIOBParamsIntoGui();
                        }
                    }
                }
            }
            getEPCParamsIntoGui();
        }
    }


    void DsscPpt::preReconfigureEnableMeasurement(karabo::util::Hash & incomingReconfiguration) {
        Hash filtered = this->filterByTags(incomingReconfiguration, "measurement");
        vector<string> paths;
        filtered.getPaths(paths);

        if (!paths.empty()) {


            BOOST_FOREACH(string path, paths) {
                if (path.compare("enD0Mode") == 0) {
                    bool enD0Mode = filtered.getAs<bool>(path);
                    bool bypCompr = get<bool>("bypassCompression");
                    {
                        DsscScopedLock lock(&m_accessToPptMutex, __func__);
                        m_ppt->setD0Mode(enD0Mode, bypCompr);
                    }
                } else if (path.compare("bypassCompression") == 0) {
                    bool bypCompr = filtered.getAs<bool>(path);
                    bool enD0Mode = get<bool>("enD0Mode");
                    {
                        DsscScopedLock lock(&m_accessToPptMutex, __func__);
                        m_ppt->setD0Mode(enD0Mode, bypCompr);
                    }
                }
            }
        }
    }


    void DsscPpt::preReconfigureFullConfig(karabo::util::Hash & incomingReconfiguration) {
        Hash filtered = this->filterByTags(incomingReconfiguration, "FullConfig");
        vector<string> paths;
        filtered.getPaths(paths);

        if (!paths.empty()) {


            BOOST_FOREACH(string path, paths) {
                if (path.compare("fullConfigFileName") == 0) {
                    KARABO_LOG_INFO << "Karabo::DsscPpt set full config file";
                    const string fullConfigFileName = filtered.getAs<string>(path);
                    boost::filesystem::path myfile(fullConfigFileName);
                    if (boost::filesystem::exists(myfile)) {
                        readFullConfigFile(fullConfigFileName);
                        setQSFPEthernetConfig();
                    }
                }
            }
        }
    }


    void DsscPpt::preReconfigureFastInit(karabo::util::Hash & incomingReconfiguration) {
        Hash filtered = this->filterByTags(incomingReconfiguration, "fastInit");
        vector<string> paths;
        filtered.getPaths(paths);

        if (!paths.empty()) {


            BOOST_FOREACH(string path, paths) {
                if (path.compare("initDistance") == 0) {
                    KARABO_LOG_INFO << "Set Init distance";
                    m_ppt->setInitDist(filtered.getAs<unsigned int>(path));
                } else if (path.compare("fastInitJTAGSpeed") == 0) {
                    KARABO_LOG_INFO << "Fast Init ConfigSpeed";
                    m_ppt->setFastInitConfigSpeed(filtered.getAs<unsigned int>(path));
                }
            }
        }
    }


    void DsscPpt::preReconfigureRegAccess(karabo::util::Hash & incomingReconfiguration) {
        Hash filtered = this->filterByTags(incomingReconfiguration, "regAccess");
        vector<string> paths;
        filtered.getPaths(paths);


        BOOST_FOREACH(string path, paths) {
            if (path.compare("setLogoConfig") == 0) {
                auto enable = filtered.getAs<bool>(path);
                setLogoConfig(enable);
            }
        }
    }


    void DsscPpt::preReconfigureLoadIOBConfig(karabo::util::Hash & incomingReconfiguration) {
        Hash filtered = this->filterByTags(incomingReconfiguration, "IOBConfigPath");
        vector<string> paths;
        filtered.getPaths(paths);


        BOOST_FOREACH(string path, paths) {
            if (path.compare("iobRegisterFilePath") == 0) {
                updateIOBConfigSchema(filtered.getAs<string>(path));
            }
        }
    }


    void DsscPpt::preReconfigureLoadEPCConfig(karabo::util::Hash & incomingReconfiguration) {
        Hash filtered = this->filterByTags(incomingReconfiguration, "EPCConfigPath");
        vector<string> paths;
        filtered.getPaths(paths);


        BOOST_FOREACH(string path, paths) {
            if (path.compare("epcRegisterFilePath") == 0) {
                updateEPCConfigSchema(filtered.getAs<string>(path));
            }
        }
    }


    void DsscPpt::preReconfigureLoadASICConfig(karabo::util::Hash & incomingReconfiguration) {
        Hash filtered = this->filterByTags(incomingReconfiguration, "ASICConfigPath");
        vector<string> paths;
        filtered.getPaths(paths);


        BOOST_FOREACH(string path, paths) {
            if (path.compare("jtagRegisterFilePath") == 0) {
                updateJTAGConfigSchema(filtered.getAs<string>(path));
            } else if (path.compare("sequencerFilePath") == 0) {
                updateSeqConfigSchema(filtered.getAs<string>(path));
            } else if (path.compare("pixelRegisterFilePath") == 0) {
                updatePixelConfigSchema(filtered.getAs<string>(path));
            }
        }
    }


    void DsscPpt::updateEPCConfigSchema(const std::string & configFileName) {

        if (!boost::filesystem::exists(configFileName)) {
            KARABO_LOG_ERROR << "File not found: " << configFileName;
            return;
        }
        KARABO_LOG_INFO << "EPC Register File reloaded! ";
        m_ppt->getEPCRegisters()->initFromFile(configFileName);

        getEPCParamsIntoGui();
    }


    void DsscPpt::updateIOBConfigSchema(const std::string & configFileName) {
        //TODO:
        //Find a way to delete existing IOBConfig to regenerate it
        string iobFileName = get<string>("iobRegisterFilePath");
        if (!boost::filesystem::exists(iobFileName)) {
            KARABO_LOG_ERROR << "File not found: " << iobFileName;
            return;
        }
        KARABO_LOG_INFO << "IOB Register File reloaded! ";
        m_ppt->getIOBRegisters()->initFromFile(iobFileName);

        getIOBParamsIntoGui();
    }


    void DsscPpt::updateJTAGConfigSchema(const std::string & configFileName) {
        if (!boost::filesystem::exists(configFileName)) {
            KARABO_LOG_ERROR << "File not found: " << configFileName;
            return;
        }
        KARABO_LOG_INFO << "JTAG Register File reloaded! ";
        m_ppt->getJTAGRegisters()->initFromFile(configFileName);

        //getJTAGParamsIntoGui();

        if (isProgramState()) {
            programJTAG();
        }
    }


    void DsscPpt::updatePixelConfigSchema(const std::string & configFileName) {
        if (!boost::filesystem::exists(configFileName)) {
            KARABO_LOG_ERROR << "File not found: " << configFileName;
            return;
        }
        KARABO_LOG_INFO << "Pixel Register File reloaded! ";
        m_ppt->getPixelRegisters()->initFromFile(configFileName);

        updateGuiMeasurementParameters();

        if (isProgramState()) {
            programPixelRegister();
        }
        //getPixelParamsIntoGui();
    }


    void DsscPpt::updateSeqConfigSchema(const std::string & configFileName) {
        if (!boost::filesystem::exists(configFileName)) {
            KARABO_LOG_ERROR << "File not found: " << configFileName;
            return;
        }
        KARABO_LOG_INFO << "Sequencer File reloaded! ";
        m_ppt->getSequencer()->loadFile(configFileName);

        getSequencerParamsIntoGui();

        if (isProgramState()) {
            programSequencer();
        }
    }


    void DsscPpt::setSendingASICs() {
        const auto text = get<string>("sendingASICs");
        if (text.length() == 17) {
            uint16_t actASICs = utils::bitEnableStringToValue(text);
            m_ppt->setSendingAsics(actASICs);
        } else {
            KARABO_LOG_INFO << "String wrong format: ASIC 15-> 11111111_11111111 <- ASIC 0  " << text;
        }
    }


    void DsscPpt::programLMKsAuto() {
        KARABO_LOG_INFO << "Program LMKs automatically";
        DsscScopedLock lock(&m_accessToPptMutex, __func__);

        m_ppt->programLMKsDefault();
    }


    void DsscPpt::acquire() {

        KARABO_LOG_INFO << "Acquisition started";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            int rc = m_ppt->start();
            if (rc != SuS::DSSC_PPT::ERROR_OK) {
                KARABO_LOG_WARN << "DSSC failed to start: " << m_ppt->errorString;
                return;
            }
        }
        while (m_keepAcquisition) {
            {
                DsscScopedLock lock(&m_accessToPptMutex, __func__);
                cout << '-';
                cout.flush();
            }
            boost::this_thread::sleep(boost::posix_time::seconds(2));
        }
        KARABO_LOG_INFO << "Acquisition stopped";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            int rc = m_ppt->stop();
            if (rc != SuS::DSSC_PPT::ERROR_OK)
                KARABO_LOG_WARN << "PPT failed to stop: " << m_ppt->errorString;
        }
    }


    void DsscPpt::storePoweredPixels() {
        KARABO_LOG_INFO << "Powered pixels are = " << utils::positionVectorToList(m_ppt->updatePoweredPixels());
    }


    void DsscPpt::restorePoweredPixels() {
        DsscScopedLock lock(&m_accessToPptMutex, __func__);
        m_ppt->restorePoweredPixels();
    }


    void DsscPpt::setCurrentQuarterOn() {
        DSSC::StateChangeKeeper keeper(this);

        static const vector<string> quarterStr{"0-3", "4-7", "8-11", "12-15", "16-19", "20-23", "24-27", "28-31", "32-35", "36-39", "40-43", "44-47", "48-51", "52-55", "56-59", "60-63"};

        const auto quarter = get<unsigned int>("pixelsColSelect");
        if (quarter > quarterStr.size()) {
            KARABO_LOG_ERROR << "Pixels Quarter out of range" << quarter;
            return;
        }

        const auto quarterPixels = m_ppt->getColumnPixels(quarterStr[quarter]);
        const auto quarterPixelsStr = utils::positionVectorToList(quarterPixels);
        KARABO_LOG_INFO << "Enable " << m_ppt->getInjectionModeName(m_ppt->getInjectionMode()) << " in columns " << quarterStr[quarter];

        DsscScopedLock lock(&m_accessToPptMutex, __func__);
        m_ppt->enableMonBusForPixels(quarterPixels);
        m_ppt->enableInjection(true, quarterPixelsStr, true);
    }


    void DsscPpt::setCurrentColSkipOn() {
        DSSC::StateChangeKeeper keeper(this);

        string colString = getCurrentColSelectString();

        const auto quarterPixels = m_ppt->getPixels(colString);
        const auto quarterPixelsStr = utils::positionVectorToList(quarterPixels);
        KARABO_LOG_INFO << "Enable " << m_ppt->getInjectionModeName(m_ppt->getInjectionMode()) << " in columns " << colString;
        KARABO_LOG_INFO << "Enable in pixels " << quarterPixelsStr.substr(0, 30) << "  ...";

        DsscScopedLock lock(&m_accessToPptMutex, __func__);
        m_ppt->enableMonBusCols(colString);
        m_ppt->enableInjection(true, quarterPixelsStr, true);
    }


    std::string DsscPpt::getCurrentColSelectString() {
        const string enableMode = get<string>("colSelectMode");
        const auto numParallelCols = get<unsigned int>("numParallelColumns");

        string selCHIPParts = "col";
        if (enableMode == "SKIP") {
            selCHIPParts = "colskip";
        } else if (enableMode == "SKIPSPLIT") {
            selCHIPParts = "colskipsplit";
        }
        selCHIPParts += get<string>("columnSelect") + ":";
        if (numParallelCols == 0) {
            selCHIPParts = "";
        } else {
            selCHIPParts += to_string(64 / numParallelCols);
        }

        SuS::CHIPTrimmer trimmer;
        trimmer.setChipParts(selCHIPParts);
        auto chipParts = trimmer.getChipParts();
        for (auto && part : chipParts) {
            std::cout << part << endl;
        }

        const auto selIdx = get<unsigned int>("pixelsColSelect");
        if (selIdx >= chipParts.size()) {
            KARABO_LOG_ERROR << "Chip Part Pixel Selection Out of range:" << selIdx;
            return "";
        }

        return chipParts[selIdx];
    }


    void DsscPpt::pollHardware() {
        try {
            KARABO_LOG_INFO << "Hardware polling started";
            while (m_keepPolling) {
                
                int value;
                {
                    //boost::mutex::scoped_lock lock(m_accessToPptMutex);
                    DsscScopedLock lock(&m_accessToPptMutex, __func__);
                    m_ppt->readBackEPCRegister("Eth_Output_Data_Rate");
                    value = m_ppt->readFPGATemperature();                  
                 }

                uint32_t outputRate = m_ppt->getEPCParam("Eth_Output_Data_Rate", "0", "Eth_Output_Data_Rate")/(1E6/128.0);
                set<string>("ethOutputRate", to_string(outputRate) + " MBit/s");
                set<int>("pptTemp", value);

                boost::this_thread::sleep(boost::posix_time::seconds(5));
            }
        } catch (const Exception& e) {
            KARABO_LOG_ERROR << e;
        } catch (...) {
            KARABO_LOG_ERROR << "Unknown exception was raised in poll thread";
        }
    }


    void DsscPpt::programPLL() {
        KARABO_LOG_INFO << "Program PLL";
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            if (get<bool>("pptPLL.internalPLL")) {
                m_ppt->clockPLLSelect(true);
            } else {
                m_ppt->clockPLLSelect(false);
            }
        }
        if (get<bool>("xfelMode")) {
            stopManualMode();
        } else {
            startManualMode();
        }
    }


    void DsscPpt::programPLLFine() {
        bool useInternalPLL = get<bool>("pptPLL.internalPLL");

        bool fineTuningEnable = get<bool>("pptPLL.fineTuning.enableFineTuning");

        if (fineTuningEnable) {
            if (useInternalPLL) {
                KARABO_LOG_INFO << "Use PLLMultiplier, not int, frac nor mod value";
            } else {
                KARABO_LOG_INFO << "Use int, frac and mod value, not PLLMultiplier";
            }
        } else {
            KARABO_LOG_INFO << "Set Default PLL settings";

            set<unsigned int>("pptPLL.fineTuning.intValue", 280);
            set<unsigned int>("pptPLL.fineTuning.fracValue", 0);
            set<unsigned int>("pptPLL.fineTuning.modValue", 25);

            set<unsigned int>("pptPLL.fineTuning.PLLMultiplier", 28);
        }

        bool XFELClockSource = get<bool>("pptPLL.XFELClk");
        if (!XFELClockSource) {
            KARABO_LOG_WARN << "PPT PLL Programmed in Standalone mode, XFEL C&C not used.";
        }

        unsigned int multiplier = get<unsigned int>("pptPLL.fineTuning.PLLMultiplier");
        unsigned int intVal = get<unsigned int>("pptPLL.fineTuning.intValue");
        unsigned int fracVal = get<unsigned int>("pptPLL.fineTuning.fracValue");
        unsigned int modVal = get<unsigned int>("pptPLL.fineTuning.modValue");

        string resFreqString;
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            resFreqString = m_ppt->programClocking(XFELClockSource, useInternalPLL,
                                                   multiplier, intVal, fracVal, modVal);
        }

        KARABO_LOG_INFO << "Set PLL Frequency to " + resFreqString;
        set<string>("pptPLL.fineTuning.resultingClockSpeed", resFreqString);

        updateGuiPLLParameters();
    }


    bool DsscPpt::printPPTErrorMessages(bool printRBCorrect) {
        bool ok = true;

        ok &= (m_ppt->errorMessages.size() == 0);

        for (size_t i = 0; i < m_ppt->errorMessages.size(); i++) {
            KARABO_LOG_ERROR << m_ppt->errorMessages.at(i);
        }
        m_ppt->errorMessages.clear();

        if (printRBCorrect && ok) {
            KARABO_LOG_INFO << "Readback Correct!";
        }

        return ok;
    }


    string DsscPpt::getIOBTag(int iobNumber) {
        switch (iobNumber) {
            case 1: return "IOB1Config";
            case 2: return "IOB2Config";
            case 3: return "IOB3Config";
            case 4: return "IOB4Config";
        }
        return "";
    }


    void DsscPpt::checkQSFPConnected() {
        //boost::mutex::scoped_lock lock(m_accessToPptMutex);
        DsscScopedLock lock(&m_accessToPptMutex, __func__);
        set<string>("connectedETHChannels", m_ppt->getConnectedETHChannels());
    }


    int DsscPpt::anyToInt(const boost::any anyVal, bool &ok) {
        int value = 0;
        ok = true;
        if (anyVal.type() == typeid (int)) {
            KARABO_LOG_INFO << "Any is int";
            value = boost::any_cast<int>(anyVal);

        } else if (anyVal.type() == typeid (bool)) {
            KARABO_LOG_INFO << "Any is bool";
            value = (boost::any_cast<bool>(anyVal)) ? 1 : 0;

        } else if (anyVal.type() == typeid (string)) {
            try {
                KARABO_LOG_INFO << "Any is string";
                string s = boost::any_cast<string>(anyVal);
                value = INT_CAST(s);
            } catch (boost::bad_lexical_cast const&) {
                KARABO_LOG_WARN << "Error converting string to int";
                ok = false;
            }
        } else {
            KARABO_LOG_WARN << "Error converting any value";
            ok = false;
        }

        return value;
    }


    void DsscPpt::setThrottleDivider() {
        //
        uint32_t throttle_divider = get<uint32_t>("ethThrottleDivider");
        m_ppt->setEthernetOutputThrottleDivider(throttle_divider);
    }


    void DsscPpt::enableDPChannels(uint16_t enOneHot) {
        {
            DsscScopedLock lock(&m_accessToPptMutex, __func__);
            for (int i = 0; i < 4; i++) {
                bool enable = (enOneHot & (1 << i)) != 0;
                m_ppt->setDPEnabled(i + 1, enable);
            }
        }
        updateGuiEnableDatapath();
    }


    void DsscPpt::startSingleCycle() {
        const auto iterations = get<unsigned int>("singleCycleFields.iterations");
        const auto slow_mode = get<unsigned int>("singleCycleFields.moduloValue");

        DsscScopedLock lock(&m_accessToPptMutex, __func__);
        m_ppt->setEPCParam("Single_Cycle_Register", "all", "iterations", iterations);
        m_ppt->setEPCParam("Single_Cycle_Register", "all", "slow_mode", slow_mode);
        m_ppt->setEPCParam("Single_Cycle_Register", "all", "continuous_mode", 0);
        m_ppt->setEPCParam("Single_Cycle_Register", "all", "disable_sending", 0);
        m_ppt->setEPCParam("Single_Cycle_Register", "all", "doSingleCycle", 1);
        m_ppt->programEPCRegister("Single_Cycle_Register");

        m_ppt->setEPCParam("Single_Cycle_Register", "all", "doSingleCycle", 0);
        m_ppt->programEPCRegister("Single_Cycle_Register");
    }



}
