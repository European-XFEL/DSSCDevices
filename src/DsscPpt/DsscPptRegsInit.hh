/*
 * File:   DsscPptRegsInit.hh
 * Author: kirchgessner
 *
 * Created on 14. M?rz 2014, 15:37
 */

#ifndef DSSCPPTREGSINIT_HH
#define	DSSCPPTREGSINIT_HH

#define MPR_SIGS "IOB_RESET1,IOB_RESET2,IOB_RESET3,IOB_RESET4,CmdProtocolEngine_enable,PLL_CE,datapath_channel_enable1,datapath_channel_enable2,datapath_channel_enable3,datapath_channel_enable4,EPC_devices_res_n,jtag_engine_enable1,jtag_engine_enable2,jtag_engine_enable3,jtag_engine_enable4,ddr3_reset,ddr_throttle_divider"
#define SEL_ETH0_SIGS "sel_eth0_datareceiver,ethernet_traffic_throttle_div,send_dummy_packets,enable_continous_readout"
#define AURORARX_CTL_SIGS "num_frames_to_send,send_converted_data,send_reordered_data,send_raw_data,send_trailer,discard_missing_pixels,nc,rx_aurora_reset_in,send_dummy_dr_data"
#define CONFIG_PATH "../../packages/controlDevices/dsscPpt"

#define EPC_REGS_INIT \
        CHOICE_ELEMENT(expected).key("epcRegs")                                 \
          .assignmentOptional().defaultValue("Multi_purpose_Register")          \
          .commit();                                                            \
                                                                                \
        NODE_ELEMENT(expected).key("epcRegs.Multi_purpose_Register")            \
          .tags("epcRegs")                                                      \
          .displayedName("Multi_purpose_Register")                              \
          .description("Multi_purpose_Register")                                \
          .commit();                                                            \
                                                                                \
        STRING_ELEMENT(expected)                                                \
          .key("epcRegs.Multi_purpose_Register.signals")                        \
          .displayedName("Multi_purpose_Register Signals")                      \
          .description("Multi_purpose_Register Signals")                        \
          .assignmentOptional().defaultValue("CmdProtocolEngine_enable")        \
          .reconfigurable()                                                     \
          .options(MPR_SIGS)                                                    \
          .commit();                                                            \
                                                                                \
        UINT32_ELEMENT(expected).key("epcRegs.Multi_purpose_Register.value")    \
          .displayedName("Register Value")                                      \
          .description("Selected Register Value")                               \
          .assignmentOptional().defaultValue(0).reconfigurable()                \
          .allowedStates(State::ON, State::STOPPED)                                              \
          .commit();                                                            \
                                                                                \
        NODE_ELEMENT(expected).key("epcRegs.DataRecv_to_Eth0_Register")         \
          .tags("epcRegs")                                                      \
          .displayedName("DataRecv_to_Eth0_Register")                           \
          .description("DataRecv_to_Eth0_Register")                             \
          .commit();                                                            \
                                                                                \
        STRING_ELEMENT(expected)                                                \
          .key("epcRegs.DataRecv_to_Eth0_Register.signals")                     \
          .displayedName("DataRecv_to_Eth0_Register Signals")                   \
          .description("DataRecv_to_Eth0_Register Signals")                     \
          .assignmentOptional().defaultValue("send_dummy_packets")              \
          .reconfigurable()                                                     \
          .options(SEL_ETH0_SIGS)                                               \
          .commit();                                                            \
                                                                                \
        UINT32_ELEMENT(expected).key("epcRegs.DataRecv_to_Eth0_Register.value") \
          .displayedName("Gegister Value")                                      \
          .description("Selected Register Value")                               \
          .assignmentOptional().defaultValue(0).reconfigurable()                \
          .allowedStates(State::ON, State::STOPPED)                                              \
          .commit();                                                            \
                                                                                \
        NODE_ELEMENT(expected).key("epcRegs.AuroraRX_Control")                  \
          .tags("epcRegs")                                                      \
          .displayedName("AuroraRX_Control")                                    \
          .description("AuroraRX_Control")                                      \
          .commit();                                                            \
                                                                                \
        STRING_ELEMENT(expected)                                                \
          .key("epcRegs.AuroraRX_Control.signals")                              \
          .displayedName("AuroraRX_Control Signals")                            \
          .description("AuroraRX_Control Signals")                              \
          .assignmentOptional().defaultValue("send_dummy_dr_data")              \
          .reconfigurable()                                                     \
          .options(AURORARX_CTL_SIGS)                                           \
          .commit();                                                            \
                                                                                \
        UINT32_ELEMENT(expected).key("epcRegs.AuroraRX_Control.value")          \
          .displayedName("Register Value")                                      \
          .description("Selected Register Value")                               \
          .assignmentOptional().defaultValue(0).reconfigurable()                \
          .allowedStates(State::ON, State::STOPPED)                                              \
          .commit();


#define INIT_SEQUENCER_CONTROL_ELEMENTS                                         \
        NODE_ELEMENT(expected).key("sequencer")                                 \
          .displayedName("Sequencer Configuration")                             \
          .description("Sequencer configuration")                               \
          .commit();                                                            \
                                                                                \
        STRING_ELEMENT(expected).key("sequencer.opMode")                        \
                  .displayedName("Operation Mode")                              \
                  .description("opMode")                                        \
                  .assignmentOptional().defaultValue("NORM").reconfigurable()   \
                  .options("NORM,SINGLEINT,BUFFER,RESET,MANUAL,EXTLATCH,DEPFET")\
                  .allowedStates(State::ON, State::STOPPED)                     \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.cycleLength")                   \
                  .displayedName("Cycle Length")                                \
                  .description("cycle length")                                  \
                  .tags("record")                                               \
                  .assignmentOptional().defaultValue(50).reconfigurable()       \
                  .allowedStates(State::ON, State::STOPPED)                     \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.integrationTime")               \
                  .displayedName("Integration Time")                            \
                  .description("Integration Time")                              \
                  .assignmentOptional().defaultValue(100).reconfigurable()      \
                  .allowedStates(State::ON, State::STOPPED)                                      \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.flattopLength")                 \
                  .displayedName("Flattop")                                     \
                  .description("Flattop")                                       \
                  .assignmentOptional().defaultValue(20).reconfigurable()       \
                  .allowedStates(State::ON, State::STOPPED)                                      \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.rampLength")                    \
                  .displayedName("Ramp Length")                                 \
                  .description("Ramp length")                                   \
                  .assignmentOptional().defaultValue(250).reconfigurable()      \
                  .allowedStates(State::ON, State::STOPPED)                                      \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.resetLength")                   \
                  .displayedName("Reset Length")                                \
                  .description("Reset length")                                  \
                  .assignmentOptional().defaultValue(21).reconfigurable()       \
                  .allowedStates(State::ON, State::STOPPED)                                      \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.resetIntegOffset")              \
                  .displayedName("Reset Integration Offset")                    \
                  .description("Reset Integration Offset")                      \
                  .assignmentOptional().defaultValue(15).reconfigurable()       \
                  .allowedStates(State::ON, State::STOPPED)                                      \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.resetHoldLength")               \
                  .displayedName("Reset Hold Length")                           \
                  .description("Reset Hold Length")                             \
                  .assignmentOptional().defaultValue(0).reconfigurable()        \
                  .allowedStates(State::ON, State::STOPPED)                                      \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.flattopHoldLength")             \
                  .displayedName("Flattop Hold Length")                         \
                  .description("Flattop Hold Length")                           \
                  .assignmentOptional().defaultValue(0).reconfigurable()        \
                  .allowedStates(State::ON, State::STOPPED)                                      \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.rampIntegOffset")               \
                  .displayedName("Ramp Integration Offset")                     \
                  .description("Ramp Integration Offset")                       \
                  .assignmentOptional().defaultValue(100).reconfigurable()      \
                  .allowedStates(State::ON, State::STOPPED)                                      \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.backFlipAtReset")               \
                  .displayedName("Back Flip at Reset")                          \
                  .description("cycle length")                                  \
                  .assignmentOptional().defaultValue(0).reconfigurable()        \
                  .allowedStates(State::ON, State::STOPPED)                                      \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.backFlipToResetOffset")         \
                  .displayedName("Back Flip To Reset Offset")                   \
                  .description("cycle length")                                  \
                  .assignmentOptional().defaultValue(10).reconfigurable()       \
                  .allowedStates(State::ON, State::STOPPED)                                      \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.singleCapLoadLength")           \
                  .displayedName("Single SH Cap Load Length")                   \
                  .description("duration of programming the second SH Cap")     \
                  .assignmentOptional().defaultValue(20).reconfigurable()       \
                  .allowedStates(State::ON, State::STOPPED)                                      \
                  .commit();                                                    \
                                                                                \
        BOOL_ELEMENT(expected).key("sequencer.singleSHCapMode")                 \
                  .displayedName("Single SH Cap Mode")                          \
                  .description("Program Sequencer to use onle one SH Cap")      \
                  .assignmentOptional().defaultValue(true).reconfigurable()     \
                  .allowedStates(State::ON, State::STOPPED)                                      \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.injectRisingEdgeOffset")        \
                  .displayedName("Inject Rising Edge Offset")                   \
                  .description("Offset of Inject rising Edge to Flip? ")        \
                  .assignmentOptional().defaultValue(20).reconfigurable()       \
                  .allowedStates(State::ON, State::STOPPED)                                      \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.emptyInjectCycles")             \
                  .displayedName("Empty Inject Cycles")                        \
                  .description("Number of Cycles without signal between two injections") \
                  .assignmentOptional().defaultValue(20).reconfigurable()       \
                  .allowedStates(State::ON, State::STOPPED)                     \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.ftInjectOffset")                \
                  .displayedName("FT Inject Offset")                            \
                  .description("Flattop Inject Offset")                         \
                  .assignmentOptional().defaultValue(7).reconfigurable()        \
                  .allowedStates(State::ON, State::STOPPED)                     \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.integrationLength")             \
                  .displayedName("Integration Length")                          \
                  .description("")                                              \
                  .assignmentOptional().defaultValue(0).reconfigurable()         \
                  .allowedStates(State::ON, State::STOPPED)                     \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.holdPos")                       \
                  .displayedName("")                                            \
                  .description("")                                              \
                  .assignmentOptional().defaultValue(0).reconfigurable()         \
                  .allowedStates(State::ON, State::STOPPED)                     \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.holdLength")                    \
                  .displayedName("")                                            \
                  .description("")                                              \
                  .assignmentOptional().defaultValue(0).reconfigurable()         \
                  .allowedStates(State::ON, State::STOPPED)                     \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.rampOffset")                    \
                  .displayedName("")                                            \
                  .description("")                                              \
                  .assignmentOptional().defaultValue(0).reconfigurable()         \
                  .allowedStates(State::ON, State::STOPPED)                     \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.lastIntPhase")                  \
                  .displayedName("")                                            \
                  .description("")                                              \
                  .assignmentOptional().defaultValue(0).reconfigurable()         \
                  .allowedStates(State::ON, State::STOPPED)                     \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.rightShift")                    \
                  .displayedName("")                                            \
                  .description("")                                              \
                  .assignmentOptional().defaultValue(0).reconfigurable()         \
                  .allowedStates(State::ON, State::STOPPED)                     \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.ftFlipOffset")                  \
                  .displayedName("")                                            \
                  .description("")                                              \
                  .assignmentOptional().defaultValue(0).reconfigurable()         \
                  .allowedStates(State::ON, State::STOPPED)                     \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.dumpDelta")                     \
                  .displayedName("")                                            \
                  .description("")                                              \
                  .assignmentOptional().defaultValue(0).reconfigurable()         \
                  .allowedStates(State::ON, State::STOPPED)                     \
                  .commit();                                                    \
                                                                                \
        UINT32_ELEMENT(expected).key("sequencer.injectionMode")                 \
                  .displayedName("")                                            \
                  .description("")                                              \
                  .assignmentOptional().defaultValue(0).reconfigurable()         \
                  .allowedStates(State::ON, State::STOPPED)                     \
                  .commit();                                                    \


#define INIT_SEQUENCE_ELEMENTS                                          \
        NODE_ELEMENT(expected).key("sequence")                          \
          .displayedName("Sequence Box")                                \
          .description("PPT Control Sequence")				\
          .commit();                                                    \
                                                                        \
        INT32_ELEMENT(expected).key("sequence.start_wait_time")           \
          .displayedName("Start Wait Time [?s] ")                       \
          .description("Wait after start signal received until burst")  \
          .assignmentOptional().defaultValue(13000).reconfigurable()    \
          .allowedStates(State::ON, State::STOPPED)                                      \
          .commit();                                                    \
                                                                        \
        INT32_ELEMENT(expected).key("sequence.start_wait_offs")           \
          .displayedName("Start Wait Offset [?s] ")                     \
          .description("Offset shift for fine alignement of start signal")  \
          .assignmentOptional().defaultValue(-50).reconfigurable()      \
          .allowedStates(State::ON, State::STOPPED,State::STARTED,State::ACQUIRING)                            \
          .commit();                                                    \
                                                                        \
        INT32_ELEMENT(expected).key("sequence.gdps_on_time")            \
          .displayedName("GDPS On Time [?s]")                           \
          .description("Time GDPS is active before Burst")              \
          .assignmentOptional().defaultValue(3000).reconfigurable()     \
          .allowedStates(State::ON, State::STOPPED)                                      \
          .commit();                                                    \
                                                                        \
        INT32_ELEMENT(expected).key("sequence.iprogLength")             \
          .displayedName("Iprog Length [cyc]")                          \
          .description("Iprog Cycles")                                  \
          .assignmentOptional().defaultValue(200).reconfigurable()      \
          .allowedStates(State::ON, State::STOPPED)                                      \
          .commit();                                                    \
                                                                        \
        INT32_ELEMENT(expected).key("sequence.burstLength")             \
          .displayedName("Burst Length [cyc]")                          \
          .description("Burst Cycles")                                  \
          .assignmentOptional().defaultValue(1500).reconfigurable()     \
          .allowedStates(State::ON, State::STOPPED)                                      \
          .commit();                                                    \
                                                                        \
        INT32_ELEMENT(expected).key("sequence.refpulseLength")          \
          .displayedName("Refpulse Length [cyc]")                       \
          .description("Refpulse Cycles in 100 MHz cycles")             \
          .assignmentOptional().defaultValue(5).reconfigurable()        \
          .allowedStates(State::ON, State::STOPPED)                                      \
          .commit();                                                    \
                                                                        \
        INT32_ELEMENT(expected).key("sequence.fet_on_time")               \
          .displayedName("Fet On Time [?s]")                            \
          .description("Time GATE and SOURCE is ON before IProg ")      \
          .assignmentOptional().defaultValue(8).reconfigurable()        \
          .allowedStates(State::ON, State::STOPPED)                                      \
          .commit();                                                    \
                                                                        \
        INT32_ELEMENT(expected).key("sequence.clr_on_time")               \
          .displayedName("CLR On Time [?s]")                            \
          .description("Time CLRDIS is low before Iprog")               \
          .assignmentOptional().defaultValue(5).reconfigurable()        \
          .allowedStates(State::ON, State::STOPPED)                                      \
          .commit();                                                    \
                                                                        \
        INT32_ELEMENT(expected).key("sequence.clr_cycle")                \
          .displayedName("CLR On Cycle")                                \
          .description("Sequence Cycle number to activate CLR")         \
          .assignmentOptional().defaultValue(3).reconfigurable()        \
          .allowedStates(State::ON, State::STOPPED)                                      \
          .commit();                                                    \
                                                                        \
        INT32_ELEMENT(expected).key("sequence.clrDuty")                 \
          .displayedName("CLR Duty [10ns]")                             \
          .description("Clear Duty Cycle")                              \
          .assignmentOptional().defaultValue(4).reconfigurable()        \
          .allowedStates(State::ON, State::STOPPED)                                      \
          .commit();                                                    \
                                                                        \
        INT32_ELEMENT(expected).key("sequence.iprog_clr_en")            \
          .displayedName("Iprog Clr Enable")                            \
          .description("Extend Clear signal into IProg phase")          \
          .assignmentOptional().defaultValue(0).reconfigurable()        \
          .allowedStates(State::ON, State::STOPPED)                                      \
          .commit();                                                    \
                                                                        \
        INT32_ELEMENT(expected).key("sequence.iprog_clr_duty")          \
          .displayedName("IProg Clear Duty")                            \
          .description("Length of Clear Signal durin IProg phase")      \
          .assignmentOptional().defaultValue(20).reconfigurable()       \
          .allowedStates(State::ON, State::STOPPED)                                      \
          .commit();                                                    \
                                                                        \
        INT32_ELEMENT(expected).key("sequence.iprog_clr_offset")        \
          .displayedName("IProg clear Offset")                          \
          .description("Move Clear signal inside IProg phase")          \
          .assignmentOptional().defaultValue(10).reconfigurable()       \
          .allowedStates(State::ON, State::STOPPED)                                      \
          .commit();                                                    \
                                                                        \
        INT32_ELEMENT(expected).key("sequence.SW_PWR_ON")               \
          .displayedName("Power On Time [?s]")                          \
          .description("Time Switched Powers are active before Iprog")  \
          .assignmentOptional().defaultValue(20).reconfigurable()       \
          .allowedStates(State::ON, State::STOPPED)                                      \
          .commit();



#define INIT_CALIBRATION_ELEMENTS                                               \
        NODE_ELEMENT(expected).key("calibration")                               \
          .displayedName("Calibration Box")                                     \
          .description("PPT Calibration")				        \
          .commit();                                                            \
                                                                                \
        UINT32_ELEMENT(expected).key("calibration.intDACValue")                 \
                .displayedName("Int Dac Value")                                 \
                .description("Value of internal DAC")                           \
                .tags("calibration")                                            \
                .assignmentOptional().defaultValue(5000).reconfigurable()       \
                .allowedStates(State::ON, State::STOPPED)                                        \
                .commit();                                                      \
                                                                                \
        STRING_ELEMENT(expected).key("calibration.powerDownPxs")                \
                .displayedName("Power Down Pixels")                             \
                .description("Select pixels that wont be calibrated")           \
                .tags("calibration")                                            \
                .assignmentOptional().defaultValue("")                          \
                .reconfigurable()                                               \
                .allowedStates(State::ON, State::STOPPED)                                        \
                .commit();                                                      \
                                                                                \
        BOOL_ELEMENT(expected)                                                  \
                .key("calibration.bindUDP")                                     \
                .displayedName("Connect UDP Socket")                            \
                .description("Connect UDP Socket to uses Calibration routines") \
                .tags("calibration")                                            \
                .assignmentOptional().defaultValue(false).reconfigurable()      \
                .allowedStates(State::ON, State::STOPPED)                                        \
                .commit();                                                      \
                                                                                \
        SLOT_ELEMENT(expected)                                                  \
                .key("calibration.calibCurrentCompDacSlot")                     \
                .displayedName("Calibrate Current Comp DAC")                    \
                .description("Set all DAC values to operation-range")           \
                .allowedStates(State::ON, State::STOPPED)                                        \
                .commit();                                                      \
        SLOT_ELEMENT(expected)                                                  \
                .key("calibration.checkCurrentCompDacCalibSlot")                \
                .displayedName("Check DAC Calibration")                         \
                .description("Check all pixels if DAC is calibrated")           \
                .allowedStates(State::ON, State::STOPPED)                                        \
                .commit();                                                      \
        STRING_ELEMENT(expected)                                                \
                .key("calibration.dacCalibStatus")                              \
                .displayedName("Current Comp DAC Status")                       \
                .description("Displays calibration status of Current Comp DAC") \
                .readOnly()                                                     \
                .defaultValue("not calibrated")                                 \
                .commit();                                                      \
        SLOT_ELEMENT(expected)                                                  \
                .key("calibration.calibIrampSlot")                              \
                .displayedName("Calibrate Iramp")                               \
                .description("Calibrate Iramp in all pixels")                   \
                .allowedStates(State::ON, State::STOPPED)                                        \
                .commit();                                                      \
        STRING_ELEMENT(expected).key("calibration.irampCalibSetup")             \
                .displayedName("Iramp Calib Setup")                             \
                .description("AimSlope;StartSRAM;EndSRAM")                      \
                .tags("calibration")                                            \
                .assignmentOptional().defaultValue("0.0362;0;799")              \
                .reconfigurable()                                               \
                .allowedStates(State::ON, State::STOPPED)                                        \
                .commit();                                                      \
        SLOT_ELEMENT(expected)                                                  \
                .key("calibration.checkIrampCalibSlot")                         \
                .displayedName("Check Iramp Calibration")                       \
                .description("Check all pixels if Iramp is calibrated")         \
                .allowedStates(State::ON, State::STOPPED)                                        \
                .commit();                                                      \
        STRING_ELEMENT(expected)                                                \
                .key("calibration.irampCalibStatus")                            \
                .displayedName("Iramp Calibration Status")                      \
                .description("Displays calibration status of Iramp")            \
                .readOnly()                                                     \
                .defaultValue("not calibrated")                                 \
                .commit();                                                      \


// MAKROS for IOB FPGA Programming elements
#define INIT_PROGRAM_IOB_ELEMENT(iobNum)                                  \
        SLOT_ELEMENT(expected)                                            \
                .key("programIOB"#iobNum"FPGA")                           \
                .displayedName("Program IOB "#iobNum " FPGA")             \
                .description("Program IOB "#iobNum" firmware")            \
                .allowedStates(State::OFF)                                \
                .commit();                                                \


#define INIT_PROGRAM_IOB_FPGA_ELEMENTS                                    \
            SLOT_ELEMENT(expected)                                        \
                .key("programAllIOBFPGAs")                                \
                .displayedName("Program IOB FPGAs")                       \
                .description("Program all available IOBs firmware ")      \
                .allowedStates(State::OFF)                                \
                .commit();                                                \
                                                                          \
            INIT_PROGRAM_IOB_ELEMENT(1)                                   \
            INIT_PROGRAM_IOB_ELEMENT(2)                                   \
            INIT_PROGRAM_IOB_ELEMENT(3)                                   \
            INIT_PROGRAM_IOB_ELEMENT(4)                                   \
                                                                          \
            SLOT_ELEMENT(expected)                                        \
                .key("checkAllIOBStatus")                                 \
                .displayedName("Check IOB Status")                        \
                .description("Checks if IOBoards are available")          \
                .allowedStates(State::ON, State::STOPPED)                                  \
                .commit();


#define INIT_CONFIG_REGISTER_ELEMENTS                                                   \
                                                                                        \
            STRING_ELEMENT(expected)                                                      \
                .key("jtagRegisterFilePath")                                            \
                .displayedName("JTAG Config File")                                      \
                .description("configuration File for ASIC JTAG registers")              \
                .tags("ASICConfigPath")                                                 \
                .assignmentOptional().defaultValue( "/home/dssc/Desktop/HeraSud_June/PixelInjectionGainTrimmedLin_jtagRegs.xml") \
                .reconfigurable()                                                       \
                .commit();                                                              \
                                                                                        \
            BOOL_ELEMENT(expected)                                                      \
                .key("jtagReadBackEnable").displayedName("Readback JTAG")               \
                .description("Enable JTAG Readback")                                    \
                .assignmentOptional().defaultValue(false).reconfigurable()              \
                .allowedStates(State::ON, State::STOPPED)                                                \
                .commit();                                                                      \
                                                                                                \
            STRING_ELEMENT(expected)                                                              \
                .key("pixelRegisterFilePath")                                                   \
                .displayedName("Pixel Register Config File")                                    \
                .description("configuration File for ASIC Pixel registers")                     \
                .tags("ASICConfigPath")                                                         \
                .assignmentOptional().defaultValue( "/home/dssc/Desktop/HeraSud_June/PixelInjectionGainTrimmedLin_pxRegs.xml")           \
                .reconfigurable()                                                               \
                .commit();                                                                      \
            BOOL_ELEMENT(expected)                                                              \
                .key("pixelReadBackEnable").displayedName("Readback PixelRegister")             \
                .description("Enable PixelRegister Readback")                                   \
                .assignmentOptional().defaultValue(false).reconfigurable()                      \
                .allowedStates(State::ON, State::STOPPED)                                                        \
                .commit();                                                                      \
                                                                                                \
            STRING_ELEMENT(expected)                                                              \
                .key("sequencerFilePath")                                                       \
                .displayedName("Sequencer Config File")                                         \
                .description("configuration File for Sequencer")                                \
                .tags("ASICConfigPath")                                                         \
                .assignmentOptional().defaultValue( "/home/dssc/Desktop/HeraSud_June/PixelInjectionGainTrimmedLin_seq.xml")              \
                .reconfigurable()                                                               \
                .commit();                                                                      \
                                                                                                \
            BOOL_ELEMENT(expected)                                                              \
                .key("sequencerReadBackEnable").displayedName("Readback Sequencer")             \
                .description("Enable Sequencer Readback")                                       \
                .assignmentOptional().defaultValue(false).reconfigurable()                      \
                .allowedStates(State::ON, State::STOPPED)                                                        \
                .commit();                                                                      \
                                                                                                \
            UINT32_ELEMENT(expected).key("activeModule")                                        \
                .displayedName("Active Module")                                                 \
                .description("global control for active IOB module,  1 - 4")                    \
                .minInc(1).maxInc(4)                                                            \
                .assignmentOptional().defaultValue(1).reconfigurable()                          \
                .commit();                                                                      \
                                                                                                \
            SLOT_ELEMENT(expected)                                                              \
                .key("programJTAG")                                                             \
                .displayedName("Program Active JTAG")                                           \
                .description("Program JTAG via selected Module")                                \
                .allowedStates(State::ON, State::STOPPED)                                       \
                .commit();                                                                      \
                                                                                                \
            SLOT_ELEMENT(expected)                                                              \
                .key("programPixelRegisterDefault")                                             \
                .displayedName("Program PixelRegister Default")                                 \
                .description("Program Pixelregister to default Content via selected Module")    \
                .allowedStates(State::ON, State::STOPPED)                                       \
                .commit();                                                                      \
                                                                                                \
            SLOT_ELEMENT(expected)                                                              \
                .key("programPixelRegister")                                                    \
                .displayedName("Program PixelRegister")                                         \
                .description("Program Pixelregister via selected Module")                       \
                .allowedStates(State::ON, State::STOPPED)                                       \
                .commit();                                                                      \
                                                                                                \
            SLOT_ELEMENT(expected)                                                              \
                .key("updateSequencer")                                                         \
                .displayedName("Update Sequencer")                                              \
                .description("Update sequencer counters from fields and program")               \
                .allowedStates(State::ON, State::STOPPED)                                       \
                .commit();                                                                      \



// MAKROS for PPT PLL Control
#define INIT_PPT_PLL_ELEMENTS                   								\
        SLOT_ELEMENT(expected)                                                                                  \
                .key("programPLL")                                                                              \
                .displayedName("Reprogram PLL")                                                                 \
                .description("Programs the PLL with the specified parameters")                                  \
                .allowedStates(State::OFF,State::ON, State::STOPPED,State::STARTED,State::ACQUIRING)                                                                        \
                .commit();                                                                                      \
        NODE_ELEMENT(expected).key("pptPLL")                                                                    \
                .displayedName("PLL")                                                                           \
                .description("Control of PPT PLL")								\
                .commit();                                                                                      \
        BOOL_ELEMENT(expected)                                                                                  \
                .key("pptPLL.locked").displayedName("PLL Locked")                                               \
                .description("Shows if PLL is locked")                                                          \
                .readOnly()                                                                                     \
                .commit();                                                                                      \
        BOOL_ELEMENT(expected)                                                                                  \
                .key("pptPLL.XFELClk")                                                                          \
                .displayedName("XFEL Mode")								        \
                .description("Activates clocks form XFEL, standalone mode deactivated = clock from xfel")  	\
                .tags("PLL")                                                                                    \
                .assignmentOptional().defaultValue(false).reconfigurable()					\
                .allowedStates(State::ON, State::STOPPED)                                                       \
                .commit();                                                                                      \
        BOOL_ELEMENT(expected)                                                                                  \
                .key("pptPLL.internalPLL")                                                                      \
                .displayedName("Internal PLL Chip")								\
                .description("Activates clocks form internal PLL, deactivated = clock from external pll")	\
                .tags("PLL")                                                                                    \
                .assignmentOptional().defaultValue(false).reconfigurable()					\
                .allowedStates(State::ON, State::STOPPED)                                                       \
                .commit();                                                                                      \
        NODE_ELEMENT(expected).key("pptPLL.fineTuning")                                                         \
                .displayedName("FineTuning")                                                                    \
                .description("Fine Tuning of the PLL")								\
                .commit();                                                                                      \
        BOOL_ELEMENT(expected)                                                                                  \
                .key("pptPLL.fineTuning.enableFineTuning")                                                      \
                .displayedName("Enable Fine Tuning")								\
                .description("Enables Fine Tuning of clock in standalone mode - not recommended")  		\
                .tags("PLL")                                                                                    \
                .assignmentOptional().defaultValue(false).reconfigurable()					\
                .allowedStates(State::ON, State::STOPPED)                                                       \
                .commit();                                                                                      \
        STRING_ELEMENT(expected)                                                                                \
                .key("pptPLL.fineTuning.resultingClockSpeed")                                                   \
                .displayedName("Resulting Clock")                                                               \
                .description("Resulting clock speed of current settings")                                       \
                .readOnly()                                                                                     \
                .defaultValue("700000000 Hz")                                                                   \
                .commit();                                                                                      \
        UINT32_ELEMENT(expected).key("pptPLL.fineTuning.intValue")						\
                .displayedName("Int Value")                                                                     \
                .description("Int Value")                                                                       \
                .tags("PLL")                                                                                    \
                .assignmentOptional().defaultValue(280).reconfigurable()     					\
                .allowedStates(State::ON, State::STOPPED)                                                       \
                .commit();                                                                                      \
        UINT32_ELEMENT(expected).key("pptPLL.fineTuning.fracValue")                                             \
                .displayedName("Frac Value")                                                                    \
                .description("Frac Value")                                                                      \
                .tags("PLL")                                                                                    \
                .assignmentOptional().defaultValue(0).reconfigurable()     					\
                .allowedStates(State::ON, State::STOPPED)                                                       \
                .commit();                                                                                      \
        UINT32_ELEMENT(expected).key("pptPLL.fineTuning.modValue")						\
                .displayedName("Mod Value")                                                                     \
                .description("Mod Value")                                                                       \
                .tags("PLL")                                                                                    \
                .assignmentOptional().defaultValue(25).reconfigurable()     					\
                .allowedStates(State::ON, State::STOPPED)                                                       \
                .commit();                                                                                      \
        UINT32_ELEMENT(expected).key("pptPLL.fineTuning.PLLMultiplier")                                         \
                .displayedName("Multiplier Value")								\
                .description("Multiplier Value")								\
                .tags("PLL")                                                                                    \
                .assignmentOptional().defaultValue(28).reconfigurable()     					\
                .allowedStates(State::ON, State::STOPPED)                                                       \
                .commit();                                                                                      \


// MAKROS for IOB initialisation elements
#define INIT_IOB_ELEMENT(iobNum)                                                                                \
        SLOT_ELEMENT(expected)                                                                                  \
                .key("resetAurora"#iobNum)                                                                      \
                .displayedName("IOB "#iobNum" Reset Aurora")                                                    \
                .description("Reset Aurora and gives the channel a new chance to lock")                         \
                .allowedStates(State::ON, State::STOPPED)                                                       \
                .commit();                                                                                      \
        SLOT_ELEMENT(expected)                                                                                  \
                .key("programLMK"#iobNum)                                                                       \
                .displayedName("IOB "#iobNum" Program LMK")                                                     \
                .description("Initializes clock buffers")                                                       \
                .allowedStates(State::ON, State::STOPPED)                                                       \
                .commit();                                                                                      \
        SLOT_ELEMENT(expected)                                                                                  \
                .key("programIOBConfig"#iobNum)                                                                 \
                .displayedName("IOB "#iobNum" Program Config")                                                  \
                .description("Program IOB sysconfig register")                                                  \
                .allowedStates(State::ON, State::STOPPED)                                                       \
                .commit();                                                                                      \
        SLOT_ELEMENT(expected)                                                                                  \
                .key("readbackIOBConfig"#iobNum)                                                                \
                .displayedName("IOB "#iobNum" Readback Config")                                                 \
                .description("Readback IOB sysconfig register")                                                 \
                .allowedStates(State::ON, State::STOPPED)                                                       \
                .commit();                                                                                      \
        NODE_ELEMENT(expected).key("iob"#iobNum"Status")                                                        \
                .displayedName("IOB "#iobNum)                                                                   \
                .description("Status of IOB "#iobNum)                                                           \
                .commit();                                                                                      \
        UINT32_ELEMENT(expected)                                                                                \
                .key("iob"#iobNum"Status.iobTemp")                                                              \
                .displayedName("IOB Temperature")                                                               \
                .description("Calculated temperature from IOB sensor")                                          \
                .readOnly()                                                                                     \
                .commit();                                                                                      \
        STRING_ELEMENT(expected)                                                                                \
                .key("iob"#iobNum"Status.iob"#iobNum"Serial")                                                   \
                .displayedName("Serial Number")                                                                 \
                .description("Serial Number of IOBoard")                                                        \
                .readOnly()                                                                                     \
                .defaultValue("program IOB FPGA")                                                               \
                .commit();                                                                                      \
        STRING_ELEMENT(expected)                                                                                \
               .key("iob"#iobNum"Status.iob"#iobNum"BuiltStamp")                                                \
               .displayedName("Firmware Buildstamp")                                                            \
               .description("Firmware Buildstamp of IOBoard")                                                   \
               .readOnly()                                                                                      \
               .defaultValue("program IOB FPGA")                                                                \
               .commit();                                                                                       \
       BOOL_ELEMENT(expected)                                                                                   \
                .key("iob"#iobNum"Status.iob"#iobNum"Available").displayedName("IOB "#iobNum" Available")       \
                .description("IOB "#iobNum" Accessible")                                                        \
                .readOnly()                                                                                     \
                .commit();                                                                                      \
        UINT32_ELEMENT(expected)                                                                                \
                .key("iob"#iobNum"Status.numPRBsFound")                                                         \
                .displayedName("Num PRBs found")                                                                \
                .description("Number of PRBs detected")                                                         \
                .readOnly()                                                                                     \
                .commit();                                                                                      \
        BOOL_ELEMENT(expected)                                                                                  \
                .key("iob"#iobNum"Status.iob"#iobNum"Ready").displayedName("IOB "#iobNum" Ready")               \
                .description("IOB "#iobNum" Accessible and Aurora Locked")                                      \
                .readOnly()                                                                                     \
                .commit();                                                                                      \
        STRING_ELEMENT(expected)                                                                                \
                .key("iob"#iobNum"Status.asicChannelReadoutFailure")                                            \
                .displayedName("Channel Failure")                                                               \
                .description("Asic channel readout failure, no data was detekted from channel after readout command, maybe no asic connected") \
                .readOnly()                                                                                     \
                .commit();


#define PPT_CHANNEL_FAILED_ELEMENTS                                                                         \
            NODE_ELEMENT(expected).key("pptChannelFailed")                                                  \
                .displayedName("Failed status channel 1 to 4")                                              \
                .commit();                                                                                  \
        UINT16_ELEMENT(expected)                                                                            \
                .key("pptChannelFailed.failedChannel1").displayedName("PPT Channel 1 Failed Status")        \
                .description("Status Register indicating which ASIC channel in module 1 failed")            \
                .readOnly().defaultValue(0)                                                                 \
                .commit();                                                                                  \
        UINT16_ELEMENT(expected)                                                                            \
                .key("pptChannelFailed.failedChannel2").displayedName("PPT Channel 2 Failed Status")        \
                .description("Status Register indicating which ASIC channel in module 2 failed")            \
                .readOnly().defaultValue(0)                                                                 \
                .commit();                                                                                  \
        UINT16_ELEMENT(expected)                                                                            \
                .key("pptChannelFailed.failedChannel3").displayedName("PPT Channel 3 Failed Status")        \
                .description("Status Register indicating which ASIC channel in module 3 failed")            \
                .readOnly().defaultValue(0)                                                                 \
                .commit();                                                                                  \
        UINT16_ELEMENT(expected)                                                                            \
                .key("pptChannelFailed.failedChannel4").displayedName("PPT Channel 4 Failed Status")        \
                .description("Status Register indicating which ASIC channel in module 4 failed")            \
                .readOnly().defaultValue(0)                                                                 \
                .commit();                                                                                  \


#define INIT_IOB_ELEMENTS       \
            INIT_IOB_ELEMENT(1) \
            INIT_IOB_ELEMENT(2) \
            INIT_IOB_ELEMENT(3) \
            INIT_IOB_ELEMENT(4) \


// MAKROS for Enable Datapath elements
#define INIT_ENABLE_DATAPATH_ELEMENT(dp)                                                     \
            BOOL_ELEMENT(expected)                                                           \
                .key("enDataPath.dp"#dp"Enable").displayedName("Enable DP "#dp)              \
                .description("Enables datapath "#dp" to receive data from IOBoard "#dp)      \
                .tags("enableDatapath")                                                      \
                .assignmentOptional().defaultValue(false).reconfigurable()                   \
                .allowedStates(State::ON, State::STOPPED)                                    \
                .commit();                                                                   \

#define INIT_ENABLE_DATAPATH_ELEMENTS                                                        \
            NODE_ELEMENT(expected).key("enDataPath")                                         \
                .displayedName("Enable Datapath 1 to 4")                                     \
                .description("Enables the datapath to receive data from IOBoard 1 to 4")     \
                .commit();                                                                   \
                                                                                             \
            INIT_ENABLE_DATAPATH_ELEMENT(1)                                                  \
            INIT_ENABLE_DATAPATH_ELEMENT(2)                                                  \
            INIT_ENABLE_DATAPATH_ELEMENT(3)                                                  \
            INIT_ENABLE_DATAPATH_ELEMENT(4)                                                  \


// MAKROS for Insert Ethernet elements
#define INIT_ETHERNET_ELEMENT(CH,ADDR)                                                       \
        NODE_ELEMENT(expected).key("qsfp.chan"#CH)                                           \
                .displayedName("ETH"#CH)                                                     \
                .description("Channel "#CH" config")                                         \
                .commit();                                                                   \
        NODE_ELEMENT(expected).key("qsfp.chan"#CH".recv")                                    \
                .displayedName("Receiver")                                                   \
                .description("Receiver side (trainbuilder) config")                          \
                .commit();                                                                   \
        STRING_ELEMENT(expected).key("qsfp.chan"#CH".recv.macaddr")                          \
                .displayedName("MAC")                                                        \
                .description("PPT MAC address")                                              \
                .tags("ethParam")                                                            \
                .assignmentOptional().defaultValue("00:02:c9:1f:4e:60").reconfigurable()     \
                .commit();                                                                   \
        STRING_ELEMENT(expected).key("qsfp.chan"#CH".recv.ipaddr")                           \
                .displayedName("IP")                                                         \
                .description("PPT hostname or IP address")                                   \
                .tags("ethParam")                                                            \
                .assignmentOptional().defaultValue("192.168.142.165").reconfigurable()       \
                .commit();                                                                   \
        UINT32_ELEMENT(expected).key("qsfp.chan"#CH".recv.port")                             \
                .displayedName("UDP Port")                                                   \
                .description("PPT port number")                                              \
                .tags("ethParam")                                                            \
                .assignmentOptional().defaultValue(4321).reconfigurable()                    \
                .commit();                                                                   \
        NODE_ELEMENT(expected).key("qsfp.chan"#CH".send")                                    \
                .displayedName("Sender")                                                     \
                .description("Receiver side (trainbuilder) config")                          \
                .commit();                                                                   \
        STRING_ELEMENT(expected).key("qsfp.chan"#CH".send.macaddr")                          \
                .displayedName("MAC")                                                        \
                .description("PPT MAC address")                                              \
                .tags("ethParam")                                                            \
                .assignmentOptional().defaultValue("aa:bb:cc:dd:ee:0"#ADDR).reconfigurable() \
                .commit();                                                                   \
        STRING_ELEMENT(expected).key("qsfp.chan"#CH".send.ipaddr")                           \
                .displayedName("IP")                                                         \
                .description("PPT hostname or IP address")                                   \
                .tags("ethParam")                                                            \
                .assignmentOptional().defaultValue("192.168.142.5"#ADDR).reconfigurable()    \
                .commit();                                                                   \
        UINT32_ELEMENT(expected).key("qsfp.chan"#CH".send.port")                             \
                .displayedName("UDP Port")                                                   \
                .description("PPT port number")                                              \
                .tags("ethParam")                                                            \
                .assignmentOptional().defaultValue(8000).reconfigurable()                    \
                .commit();



#define INIT_ETH_ELEMENTS                                                                    \
        NODE_ELEMENT(expected).key("qsfp")                                                   \
                .displayedName("QSFP")                                                       \
                .description("Configuration of QSFP control registers: MAC,IP,Port")         \
                .commit();                                                                   \
                                                                                             \
        INIT_ETHERNET_ELEMENT(1,0)                                                           \
        INIT_ETHERNET_ELEMENT(2,1)                                                           \
        INIT_ETHERNET_ELEMENT(3,2)                                                           \
        INIT_ETHERNET_ELEMENT(4,3)                                                           \


    #define IOBSLOT1  \
            this->registerSlot(std::bind(&Self::resetAurora1,this),"resetAurora1");                \
            this->registerSlot(std::bind(&Self::programLMK1,this),"programLMK1");                  \
            this->registerSlot(std::bind(&Self::programIOB1Config,this),"programIOBConfig1");      \
            this->registerSlot(std::bind(&Self::readIOBRegisters1,this),"readbackIOBConfig1");     \

    #define IOBSLOT2  \
            this->registerSlot(std::bind(&Self::resetAurora2,this),"resetAurora2");                \
            this->registerSlot(std::bind(&Self::programLMK2,this),"programLMK2");                  \
            this->registerSlot(std::bind(&Self::programIOB2Config,this),"programIOBConfig2");      \
            this->registerSlot(std::bind(&Self::readIOBRegisters2,this),"readbackIOBConfig2");     \

    #define IOBSLOT3  \
            this->registerSlot(std::bind(&Self::resetAurora3,this),"resetAurora3");                \
            this->registerSlot(std::bind(&Self::programLMK3,this),"programLMK3");                  \
            this->registerSlot(std::bind(&Self::programIOB3Config,this),"programIOBConfig3");      \
            this->registerSlot(std::bind(&Self::readIOBRegisters3,this),"readbackIOBConfig3");     \

    #define IOBSLOT4  \
            this->registerSlot(std::bind(&Self::resetAurora4,this),"resetAurora4");                \
            this->registerSlot(std::bind(&Self::programLMK4,this),"programLMK4");                  \
            this->registerSlot(std::bind(&Self::programIOB4Config,this),"programIOBConfig4");      \
            this->registerSlot(std::bind(&Self::readIOBRegisters4,this),"readbackIOBConfig4");     \

    #define PLLSLOT  \
            this->registerSlot(std::bind(&Self::programPLL,this),"programPLL");

    #define PROG_IOBSLOTS  \
            IOBSLOT1  \
            IOBSLOT2  \
            IOBSLOT3  \
            IOBSLOT4  \
            PLLSLOT

    #define CHECK_IOB(iobNumber)                                                                    \
            if (iobNumber < 1 || iobNumber > 4) {                                                   \
                KARABO_LOG_FRAMEWORK_ERROR << getInstanceId() << " IOB "  << iobNumber << " unknown!";                            \
                return;                                                                             \
            }                                                                                       \
                                                                                                    \
            if(!isIOBAvailable(iobNumber)){                                                         \
                KARABO_LOG_FRAMEWORK_ERROR << getInstanceId() << " IOB " << iobNumber << " not available, nothing programmed ";   \
                return;                                                                             \
            }

    #define CHECK_IOB_B(iobNumber)                                                                  \
            if (iobNumber < 1 || iobNumber > 4) {                                                   \
                KARABO_LOG_FRAMEWORK_ERROR << getInstanceId() << " IOB "  << iobNumber << " unknown!";                            \
                return false;                                                                       \
            }                                                                                       \
                                                                                                    \
            if(!isIOBAvailable(iobNumber)){                                                         \
                KARABO_LOG_FRAMEWORK_ERROR << getInstanceId() << " IOB " << iobNumber << " not available, nothing programmed ";   \
                return false;                                                                       \
            }

#endif	/* DSSCPPTREGSINIT_HH */

