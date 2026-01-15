/*
 * File:   DsscPptRegsInit.hh
 * Author: kirchgessner
 *
 * Created on 14. M?rz 2014, 15:37
 */

#ifndef DSSCPPTREGSINIT_HH
#define	DSSCPPTREGSINIT_HH

#include <karabo/karabo.hpp>
using namespace karabo::data;
using namespace karabo::xms;

#define MPR_SIGS "IOB_RESET1,IOB_RESET2,IOB_RESET3,IOB_RESET4,CmdProtocolEngine_enable,PLL_CE,datapath_channel_enable1,datapath_channel_enable2,datapath_channel_enable3,datapath_channel_enable4,EPC_devices_res_n,jtag_engine_enable1,jtag_engine_enable2,jtag_engine_enable3,jtag_engine_enable4,ddr3_reset,ddr_throttle_divider"
#define SEL_ETH0_SIGS "sel_eth0_datareceiver,ethernet_traffic_throttle_div,send_dummy_packets,enable_continous_readout"
#define AURORARX_CTL_SIGS "num_frames_to_send,send_converted_data,send_reordered_data,send_raw_data,send_trailer,discard_missing_pixels,nc,rx_aurora_reset_in,send_dummy_dr_data"
#define CONFIG_PATH "../../packages/controlDevices/dsscPpt"

/* Maybe obsolete, was never invoked when was a macro
 * TODO: check in a few months if anyone complained and delete this function.
void init_epc_register(karabo::data::Schema& schema) {
        STRING_ELEMENT(schema).key("epcRegs")
          .assignmentOptional().defaultValue("Multi_purpose_Register")
          .commit();

        NODE_ELEMENT(schema).key("epcRegs.Multi_purpose_Register")
          .tags("epcRegs")
          .displayedName("Multi_purpose_Register")
          .description("Multi_purpose_Register")
          .commit();

        STRING_ELEMENT(schema)
          .key("epcRegs.Multi_purpose_Register.signals")
          .displayedName("Multi_purpose_Register Signals")
          .description("Multi_purpose_Register Signals")
          .assignmentOptional().defaultValue("CmdProtocolEngine_enable")
          .reconfigurable()
          .options(MPR_SIGS)
          .commit();

        UINT32_ELEMENT(schema).key("epcRegs.Multi_purpose_Register.value")
          .displayedName("Register Value")
          .description("Selected Register Value")
          .assignmentOptional().defaultValue(0).reconfigurable()
          .allowedStates(State::ON, State::STOPPED)
          .commit();

        NODE_ELEMENT(schema).key("epcRegs.DataRecv_to_Eth0_Register")
          .tags("epcRegs")
          .displayedName("DataRecv_to_Eth0_Register")
          .description("DataRecv_to_Eth0_Register")
          .commit();

        STRING_ELEMENT(schema)
          .key("epcRegs.DataRecv_to_Eth0_Register.signals")
          .displayedName("DataRecv_to_Eth0_Register Signals")
          .description("DataRecv_to_Eth0_Register Signals")
          .assignmentOptional().defaultValue("send_dummy_packets")
          .reconfigurable()
          .options(SEL_ETH0_SIGS)
          .commit();

        UINT32_ELEMENT(schema).key("epcRegs.DataRecv_to_Eth0_Register.value")
          .displayedName("Gegister Value")
          .description("Selected Register Value")
          .assignmentOptional().defaultValue(0).reconfigurable()
          .allowedStates(State::ON, State::STOPPED)
          .commit();

        NODE_ELEMENT(schema).key("epcRegs.AuroraRX_Control")
          .tags("epcRegs")
          .displayedName("AuroraRX_Control")
          .description("AuroraRX_Control")
          .commit();

        STRING_ELEMENT(schema)
          .key("epcRegs.AuroraRX_Control.signals")
          .displayedName("AuroraRX_Control Signals")
          .description("AuroraRX_Control Signals")
          .assignmentOptional().defaultValue("send_dummy_dr_data")
          .reconfigurable()
          .options(AURORARX_CTL_SIGS)
          .commit();

        UINT32_ELEMENT(schema).key("epcRegs.AuroraRX_Control.value")
          .displayedName("Register Value")
          .description("Selected Register Value")
          .assignmentOptional().defaultValue(0).reconfigurable()
          .allowedStates(State::ON, State::STOPPED)
          .commit();
}
*/

void init_sequencer_control_elements(karabo::data::Schema& schema) {
        NODE_ELEMENT(schema).key("sequencer")
          .displayedName("Sequencer Configuration")
          .description("Sequencer configuration")
          .expertAccess()
          .commit();

        STRING_ELEMENT(schema).key("sequencer.opMode")
                  .displayedName("Operation Mode")
                  .description("opMode")
                  .assignmentOptional().defaultValue("NORM").reconfigurable()
                  .options("NORM,SINGLEINT,BUFFER,RESET,MANUAL,EXTLATCH,DEPFET")
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.cycleLength")
                  .displayedName("Cycle Length")
                  .description("cycle length")
                  .tags("record")
                  .assignmentOptional().defaultValue(50).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.integrationTime")
                  .displayedName("Integration Time")
                  .description("Integration Time")
                  .assignmentOptional().defaultValue(100).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.flattopLength")
                  .displayedName("Flattop")
                  .description("Flattop")
                  .assignmentOptional().defaultValue(20).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.rampLength")
                  .displayedName("Ramp Length")
                  .description("Ramp length")
                  .assignmentOptional().defaultValue(250).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.resetLength")
                  .displayedName("Reset Length")
                  .description("Reset length")
                  .assignmentOptional().defaultValue(21).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.resetIntegOffset")
                  .displayedName("Reset Integration Offset")
                  .description("Reset Integration Offset")
                  .assignmentOptional().defaultValue(15).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.resetHoldLength")
                  .displayedName("Reset Hold Length")
                  .description("Reset Hold Length")
                  .assignmentOptional().defaultValue(0).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.flattopHoldLength")
                  .displayedName("Flattop Hold Length")
                  .description("Flattop Hold Length")
                  .assignmentOptional().defaultValue(0).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.rampIntegOffset")
                  .displayedName("Ramp Integration Offset")
                  .description("Ramp Integration Offset")
                  .assignmentOptional().defaultValue(100).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.backFlipAtReset")
                  .displayedName("Back Flip at Reset")
                  .description("cycle length")
                  .assignmentOptional().defaultValue(0).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.backFlipToResetOffset")
                  .displayedName("Back Flip To Reset Offset")
                  .description("cycle length")
                  .assignmentOptional().defaultValue(10).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.singleCapLoadLength")
                  .displayedName("Single SH Cap Load Length")
                  .description("duration of programming the second SH Cap")
                  .assignmentOptional().defaultValue(20).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        BOOL_ELEMENT(schema).key("sequencer.singleSHCapMode")
                  .displayedName("Single SH Cap Mode")
                  .description("Program Sequencer to use onle one SH Cap")
                  .assignmentOptional().defaultValue(true).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.injectRisingEdgeOffset")
                  .displayedName("Inject Rising Edge Offset")
                  .description("Offset of Inject rising Edge to Flip? ")
                  .assignmentOptional().defaultValue(20).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.emptyInjectCycles")
                  .displayedName("Empty Inject Cycles")
                  .description("Number of Cycles without signal between two injections")
                  .assignmentOptional().defaultValue(20).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.ftInjectOffset")
                  .displayedName("FT Inject Offset")
                  .description("Flattop Inject Offset")
                  .assignmentOptional().defaultValue(7).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.integrationLength")
                  .displayedName("Integration Length")
                  .description("")
                  .assignmentOptional().defaultValue(0).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.holdPos")
                  .displayedName("")
                  .description("")
                  .assignmentOptional().defaultValue(0).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.holdLength")
                  .displayedName("")
                  .description("")
                  .assignmentOptional().defaultValue(0).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.rampOffset")
                  .displayedName("")
                  .description("")
                  .assignmentOptional().defaultValue(0).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.lastIntPhase")
                  .displayedName("")
                  .description("")
                  .assignmentOptional().defaultValue(0).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.rightShift")
                  .displayedName("")
                  .description("")
                  .assignmentOptional().defaultValue(0).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.ftFlipOffset")
                  .displayedName("")
                  .description("")
                  .assignmentOptional().defaultValue(0).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.dumpDelta")
                  .displayedName("")
                  .description("")
                  .assignmentOptional().defaultValue(0).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();

        UINT32_ELEMENT(schema).key("sequencer.injectionMode")
                  .displayedName("")
                  .description("")
                  .assignmentOptional().defaultValue(0).reconfigurable()
                  .allowedStates(State::ON, State::STOPPED)
                  .commit();
}


void init_sequence_elements(karabo::data::Schema& schema) {
        NODE_ELEMENT(schema).key("sequence")
          .displayedName("Sequence Box")
          .description("PPT Control Sequence")
          .expertAccess()
          .commit();

        INT32_ELEMENT(schema).key("sequence.start_wait_time")
          .displayedName("Start Wait Time [?s] ")
          .description("Wait after start signal received until burst")
          .assignmentOptional().defaultValue(13000).reconfigurable()
          .allowedStates(State::ON, State::STOPPED)
          .commit();

        INT32_ELEMENT(schema).key("sequence.start_wait_offs")
          .displayedName("Start Wait Offset [?s] ")
          .description("Offset shift for fine alignement of start signal")
          .assignmentOptional().defaultValue(-50).reconfigurable()
          .allowedStates(State::ON, State::STOPPED,State::STARTED,State::ACQUIRING)
          .commit();

        INT32_ELEMENT(schema).key("sequence.gdps_on_time")
          .displayedName("GDPS On Time [?s]")
          .description("Time GDPS is active before Burst")
          .assignmentOptional().defaultValue(3000).reconfigurable()
          .allowedStates(State::ON, State::STOPPED)
          .commit();

        INT32_ELEMENT(schema).key("sequence.iprogLength")
          .displayedName("Iprog Length [cyc]")
          .description("Iprog Cycles")
          .assignmentOptional().defaultValue(200).reconfigurable()
          .allowedStates(State::ON, State::STOPPED)
          .commit();

        INT32_ELEMENT(schema).key("sequence.burstLength")
          .displayedName("Burst Length [cyc]")
          .description("Burst Cycles")
          .assignmentOptional().defaultValue(1500).reconfigurable()
          .allowedStates(State::ON, State::STOPPED)
          .commit();

        INT32_ELEMENT(schema).key("sequence.refpulseLength")
          .displayedName("Refpulse Length [cyc]")
          .description("Refpulse Cycles in 100 MHz cycles")
          .assignmentOptional().defaultValue(5).reconfigurable()
          .allowedStates(State::ON, State::STOPPED)
          .commit();

        INT32_ELEMENT(schema).key("sequence.fet_on_time")
          .displayedName("Fet On Time [?s]")
          .description("Time GATE and SOURCE is ON before IProg ")
          .assignmentOptional().defaultValue(8).reconfigurable()
          .allowedStates(State::ON, State::STOPPED)
          .commit();

        INT32_ELEMENT(schema).key("sequence.clr_on_time")
          .displayedName("CLR On Time [?s]")
          .description("Time CLRDIS is low before Iprog")
          .assignmentOptional().defaultValue(5).reconfigurable()
          .allowedStates(State::ON, State::STOPPED)
          .commit();

        INT32_ELEMENT(schema).key("sequence.clr_cycle")
          .displayedName("CLR On Cycle")
          .description("Sequence Cycle number to activate CLR")
          .assignmentOptional().defaultValue(3).reconfigurable()
          .allowedStates(State::ON, State::STOPPED)
          .commit();

        INT32_ELEMENT(schema).key("sequence.clrDuty")
          .displayedName("CLR Duty [10ns]")
          .description("Clear Duty Cycle")
          .assignmentOptional().defaultValue(4).reconfigurable()
          .allowedStates(State::ON, State::STOPPED)
          .commit();

        INT32_ELEMENT(schema).key("sequence.iprog_clr_en")
          .displayedName("Iprog Clr Enable")
          .description("Extend Clear signal into IProg phase")
          .assignmentOptional().defaultValue(0).reconfigurable()
          .allowedStates(State::ON, State::STOPPED)
          .commit();

        INT32_ELEMENT(schema).key("sequence.iprog_clr_duty")
          .displayedName("IProg Clear Duty")
          .description("Length of Clear Signal durin IProg phase")
          .assignmentOptional().defaultValue(20).reconfigurable()
          .allowedStates(State::ON, State::STOPPED)
          .commit();

        INT32_ELEMENT(schema).key("sequence.iprog_clr_offset")
          .displayedName("IProg clear Offset")
          .description("Move Clear signal inside IProg phase")
          .assignmentOptional().defaultValue(10).reconfigurable()
          .allowedStates(State::ON, State::STOPPED)
          .commit();

        INT32_ELEMENT(schema).key("sequence.SW_PWR_ON")
          .displayedName("Power On Time [?s]")
          .description("Time Switched Powers are active before Iprog")
          .assignmentOptional().defaultValue(20).reconfigurable()
          .allowedStates(State::ON, State::STOPPED)
          .commit();
}

/*
TODO: is this thing used? Delete in a few month if no one complains
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
        SLOT_ELEMENT(schema)                                                  \
                .key("calibration.checkIrampCalibSlot")                         \
                .displayedName("Check Iramp Calibration")                       \
                .description("Check all pixels if Iramp is calibrated")         \
                .allowedStates(State::ON, State::STOPPED)                                        \
                .commit();                                                      \
        STRING_ELEMENT(schema)                                                \
                .key("calibration.irampCalibStatus")                            \
                .displayedName("Iramp Calibration Status")                      \
                .description("Displays calibration status of Iramp")            \
                .readOnly()                                                     \
                .defaultValue("not calibrated")                                 \
                .commit();                                                      \
*/


void init_program_iob_fpga_elements(karabo::data::Schema& schema) {
            SLOT_ELEMENT(schema)
                .key("programAllIOBFPGAs")
                .displayedName("Program IOB FPGAs")
                .description("Program all available IOBs firmware ")
                .allowedStates(State::OFF)
                .expertAccess()
                .commit();

        SLOT_ELEMENT(schema)
                .key("programIOB1FPGA")
                .displayedName("Program IOB 1 FPGA")
                .description("Program IOB 1 firmware")
                .allowedStates(State::OFF)
                .expertAccess()
                .commit();

        SLOT_ELEMENT(schema)
                .key("programIOB2FPGA")
                .displayedName("Program IOB 2 FPGA")
                .description("Program IOB 2 firmware")
                .allowedStates(State::OFF)
                .expertAccess()
                .commit();

        SLOT_ELEMENT(schema)
                .key("programIOB3FPGA")
                .displayedName("Program IOB 3 FPGA")
                .description("Program IOB 3 firmware")
                .allowedStates(State::OFF)
                .expertAccess()
                .commit();

        SLOT_ELEMENT(schema)
                .key("programIOB4FPGA")
                .displayedName("Program IOB 4 FPGA")
                .description("Program IOB 4 firmware")
                .allowedStates(State::OFF)
                .expertAccess()
                .commit();

        SLOT_ELEMENT(schema)
                .key("checkAllIOBStatus")
                .displayedName("Check IOB Status")
                .description("Checks if IOBoards are available")
                .allowedStates(State::ON, State::STOPPED)
                .expertAccess()
                .commit();
}


void init_config_register_elements(karabo::data::Schema& schema) {
            STRING_ELEMENT(schema)
                .key("jtagRegisterFilePath")
                .displayedName("JTAG Config File")
                .description("configuration File for ASIC JTAG registers")
                .tags("ASICConfigPath")
                .assignmentOptional()
                .defaultValue("PixelInjectionGainTrimmedLin_jtagRegs.xml")
                .reconfigurable()
                .expertAccess()
                .commit();

            BOOL_ELEMENT(schema)
                .key("jtagReadBackEnable").displayedName("Readback JTAG")
                .description("Enable JTAG Readback")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .expertAccess()
                .commit();

            STRING_ELEMENT(schema)
                .key("pixelRegisterFilePath")
                .displayedName("Pixel Register Config File")
                .description("configuration File for ASIC Pixel registers")
                .tags("ASICConfigPath")
                .assignmentOptional()
                .defaultValue("PixelInjectionGainTrimmedLin_pxRegs.xml")
                .reconfigurable()
                .expertAccess()
                .commit();

            BOOL_ELEMENT(schema)
                .key("pixelReadBackEnable").displayedName("Readback PixelRegister")
                .description("Enable PixelRegister Readback")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .expertAccess()
                .commit();

            STRING_ELEMENT(schema)
                .key("sequencerFilePath")
                .displayedName("Sequencer Config File")
                .description("configuration File for Sequencer")
                .tags("ASICConfigPath")
                .assignmentOptional()
                .defaultValue( "PixelInjectionGainTrimmedLin_seq.xml")
                .reconfigurable()
                .expertAccess()
                .commit();

            BOOL_ELEMENT(schema)
                .key("sequencerReadBackEnable").displayedName("Readback Sequencer")
                .description("Enable Sequencer Readback")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .expertAccess()
                .commit();

            UINT32_ELEMENT(schema).key("activeModule")
                .displayedName("Active Module")
                .description("global control for active IOB module,  1 - 4")
                .minInc(1).maxInc(4)
                .assignmentOptional()
                .defaultValue(1)
                .reconfigurable()
                .expertAccess()
                .commit();

            SLOT_ELEMENT(schema)
                .key("programJTAG")
                .displayedName("Program Active JTAG")
                .description("Program JTAG via selected Module")
                .allowedStates(State::ON, State::STOPPED)
                .expertAccess()
                .commit();

            SLOT_ELEMENT(schema)
                .key("programPixelRegisterDefault")
                .displayedName("Program PixelRegister Default")
                .description("Program Pixelregister to default Content via selected Module")
                .allowedStates(State::ON, State::STOPPED)
                .expertAccess()
                .commit();

            SLOT_ELEMENT(schema)
                .key("programPixelRegister")
                .displayedName("Program PixelRegister")
                .description("Program Pixelregister via selected Module")
                .allowedStates(State::ON, State::STOPPED)
                .expertAccess()
                .commit();

            SLOT_ELEMENT(schema)
                .key("updateSequencer")
                .displayedName("Update Sequencer")
                .description("Update sequencer counters from fields and program")
                .allowedStates(State::ON, State::STOPPED)
                .expertAccess()
                .commit();
}


void init_ppt_pll_elements(karabo::data::Schema& schema) {
        SLOT_ELEMENT(schema)
                .key("programPLL")
                .displayedName("Reprogram PLL")
                .description("Programs the PLL with the specified parameters")
                .allowedStates(State::OFF,State::ON, State::STOPPED,State::STARTED,State::ACQUIRING)
                .expertAccess()
                .commit();

        NODE_ELEMENT(schema).key("pptPLL")
                .displayedName("PLL")
                .description("Control of PPT PLL")
                .expertAccess()
                .commit();

        BOOL_ELEMENT(schema)
                .key("pptPLL.locked").displayedName("PLL Locked")
                .description("Shows if PLL is locked")
                .readOnly()
                .commit();

        BOOL_ELEMENT(schema)
                .key("pptPLL.XFELClk")
                .displayedName("XFEL Mode")
                .description("Activates clocks form XFEL, standalone mode deactivated = clock from xfel")
                .tags("PLL")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .commit();

        BOOL_ELEMENT(schema)
                .key("pptPLL.internalPLL")
                .displayedName("Internal PLL Chip")
                .description("Activates clocks form internal PLL, deactivated = clock from external pll")
                .tags("PLL")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .commit();

        NODE_ELEMENT(schema).key("pptPLL.fineTuning")
                .displayedName("FineTuning")
                .description("Fine Tuning of the PLL")
                .commit();

        BOOL_ELEMENT(schema)
                .key("pptPLL.fineTuning.enableFineTuning")
                .displayedName("Enable Fine Tuning")
                .description("Enables Fine Tuning of clock in standalone mode - not recommended")
                .tags("PLL")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .commit();

        STRING_ELEMENT(schema)
                .key("pptPLL.fineTuning.resultingClockSpeed")
                .displayedName("Resulting Clock")
                .description("Resulting clock speed of current settings")
                .readOnly()
                .defaultValue("700000000 Hz")
                .commit();

        UINT32_ELEMENT(schema).key("pptPLL.fineTuning.intValue")
                .displayedName("Int Value")
                .description("Int Value")
                .tags("PLL")
                .assignmentOptional().defaultValue(280).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .commit();

        UINT32_ELEMENT(schema).key("pptPLL.fineTuning.fracValue")
                .displayedName("Frac Value")
                .description("Frac Value")
                .tags("PLL")
                .assignmentOptional().defaultValue(0).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .commit();

        UINT32_ELEMENT(schema).key("pptPLL.fineTuning.modValue")
                .displayedName("Mod Value")
                .description("Mod Value")
                .tags("PLL")
                .assignmentOptional().defaultValue(25).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .commit();

        UINT32_ELEMENT(schema).key("pptPLL.fineTuning.PLLMultiplier")
                .displayedName("Multiplier Value")
                .description("Multiplier Value")
                .tags("PLL")
                .assignmentOptional().defaultValue(28).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .commit();
}


void init_iob_element(karabo::data::Schema& schema, int iobNum) {
        std::string iobIdx = std::to_string(iobNum);

        SLOT_ELEMENT(schema)
                .key("resetAurora" + iobIdx)
                .displayedName("IOB " + iobIdx + " Reset Aurora")
                .description("Reset Aurora and gives the channel a new chance to lock")
                .allowedStates(State::ON, State::STOPPED)
                .expertAccess()
                .commit();

        SLOT_ELEMENT(schema)
                .key("programLMK" + iobIdx)
                .displayedName("IOB " + iobIdx + " Program LMK")
                .description("Initializes clock buffers")
                .allowedStates(State::ON, State::STOPPED)
                .expertAccess()
                .commit();

        SLOT_ELEMENT(schema)
                .key("programIOBConfig" + iobIdx)
                .displayedName("IOB " + iobIdx + " Program Config")
                .description("Program IOB sysconfig register")
                .allowedStates(State::ON, State::STOPPED)
                .expertAccess()
                .commit();

        SLOT_ELEMENT(schema)
                .key("readbackIOBConfig" + iobIdx)
                .displayedName("IOB " + iobIdx + " Readback Config")
                .description("Readback IOB sysconfig register")
                .allowedStates(State::ON, State::STOPPED)
                .expertAccess()
                .commit();

        NODE_ELEMENT(schema).key("iob" + iobIdx + "Status")
                .displayedName("IOB " + iobIdx)
                .description("Status of IOB " + iobIdx)
                .expertAccess()
                .commit();

        UINT32_ELEMENT(schema)
                .key("iob" + iobIdx + "Status.iobTemp")
                .displayedName("IOB Temperature")
                .description("Calculated temperature from IOB sensor")
                .readOnly()
                .commit();

        STRING_ELEMENT(schema)
                .key("iob" + iobIdx + "Status.iob" + iobIdx + "Serial")
                .displayedName("Serial Number")
                .description("Serial Number of IOBoard")
                .readOnly()
                .defaultValue("program IOB FPGA")
                .commit();

        STRING_ELEMENT(schema)
               .key("iob" + iobIdx + "Status.iob" + iobIdx + "BuiltStamp")
               .displayedName("Firmware Buildstamp")
               .description("Firmware Buildstamp of IOBoard")
               .readOnly()
               .defaultValue("program IOB FPGA")
               .commit();

       BOOL_ELEMENT(schema)
                .key("iob" + iobIdx + "Status.iob" + iobIdx + "Available")
                .displayedName("IOB " + iobIdx + " Available")
                .description("IOB " + iobIdx + " Accessible")
                .readOnly()
                .commit();

        UINT32_ELEMENT(schema)
                .key("iob" + iobIdx + "Status.numPRBsFound")
                .displayedName("Num PRBs found")
                .description("Number of PRBs detected")
                .readOnly()
                .commit();

        BOOL_ELEMENT(schema)
                .key("iob" + iobIdx + "Status.iob" + iobIdx + "Ready")
                .displayedName("IOB " + iobIdx + " Ready")
                .description("IOB " + iobIdx + " Accessible and Aurora Locked")
                .readOnly()
                .commit();

        STRING_ELEMENT(schema)
                .key("iob" + iobIdx + "Status.asicChannelReadoutFailure")
                .displayedName("Channel Failure")
                .description("Asic channel readout failure, no data was detekted from channel after readout command, maybe no asic connected")
                .readOnly()
                .commit();
}


void init_iob_elements(karabo::data::Schema& schema) {
            init_iob_element(schema, 1);
            init_iob_element(schema, 2);
            init_iob_element(schema, 3);
            init_iob_element(schema, 4);
}


void init_ppt_channel_failed_elements(karabo::data::Schema& schema) {
         NODE_ELEMENT(schema).key("pptChannelFailed")
                .displayedName("Failed status channel 1 to 4")
                .expertAccess()
                .commit();
        UINT16_ELEMENT(schema)
                .key("pptChannelFailed.failedChannel1").displayedName("PPT Channel 1 Failed Status")
                .description("Status Register indicating which ASIC channel in module 1 failed")
                .readOnly().defaultValue(0)
                .commit();
        UINT16_ELEMENT(schema)
                .key("pptChannelFailed.failedChannel2").displayedName("PPT Channel 2 Failed Status")
                .description("Status Register indicating which ASIC channel in module 2 failed")
                .readOnly().defaultValue(0)
                .commit();
        UINT16_ELEMENT(schema)
                .key("pptChannelFailed.failedChannel3").displayedName("PPT Channel 3 Failed Status")
                .description("Status Register indicating which ASIC channel in module 3 failed")
                .readOnly().defaultValue(0)
                .commit();
        UINT16_ELEMENT(schema)
                .key("pptChannelFailed.failedChannel4").displayedName("PPT Channel 4 Failed Status")
                .description("Status Register indicating which ASIC channel in module 4 failed")
                .readOnly().defaultValue(0)
                .commit();
}


void init_enable_datapath_elements(karabo::data::Schema& schema) {
            NODE_ELEMENT(schema).key("enDataPath")
                .displayedName("Enable Datapath 1 to 4")
                .description("Enables the datapath to receive data from IOBoard 1 to 4")
                .expertAccess()
                .commit();

            BOOL_ELEMENT(schema)
                .key("enDataPath.dp1Enable").displayedName("Enable DP 1")
                .description("Enables datapath 1 to receive data from IOBoard 1")
                .tags("enableDatapath")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .commit();

            BOOL_ELEMENT(schema)
                .key("enDataPath.dp2Enable").displayedName("Enable DP 2")
                .description("Enables datapath 2 to receive data from IOBoard 2")
                .tags("enableDatapath")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .commit();

            BOOL_ELEMENT(schema)
                .key("enDataPath.dp3Enable").displayedName("Enable DP 3")
                .description("Enables datapath 3 to receive data from IOBoard 3")
                .tags("enableDatapath")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .commit();

            BOOL_ELEMENT(schema)
                .key("enDataPath.dp4Enable").displayedName("Enable DP 4")
                .description("Enables datapath 4 to receive data from IOBoard 4")
                .tags("enableDatapath")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::ON, State::STOPPED)
                .commit();
}


void init_ethernet_element(karabo::data::Schema& schema, int CH, int ADDR) {
        std::string ch = std::to_string(CH);

        NODE_ELEMENT(schema).key("qsfp.chan" + ch)
                .displayedName("ETH" + ch)
                .description("Channel " + ch + " config")
                .commit();

        NODE_ELEMENT(schema).key("qsfp.chan" + ch + ".recv")
                .displayedName("Receiver")
                .description("Receiver side (trainbuilder) config")
                .commit();

        STRING_ELEMENT(schema).key("qsfp.chan" + ch + ".recv.macaddr")
                .displayedName("MAC")
                .description("PPT MAC address")
                .tags("ethParam")
                .assignmentOptional()
                .defaultValue("00:02:c9:1f:4e:60")
                .reconfigurable()
                .commit();

        STRING_ELEMENT(schema).key("qsfp.chan" + ch + ".recv.ipaddr")
                .displayedName("IP")
                .description("PPT hostname or IP address")
                .tags("ethParam")
                .assignmentOptional()
                .defaultValue("192.168.142.165")
                .reconfigurable()
                .commit();

        UINT32_ELEMENT(schema).key("qsfp.chan" + ch + ".recv.port")
                .displayedName("UDP Port")
                .description("PPT port number")
                .tags("ethParam")
                .assignmentOptional()
                .defaultValue(4321)
                .reconfigurable()
                .commit();

        NODE_ELEMENT(schema).key("qsfp.chan" + ch + ".send")
                .displayedName("Sender")
                .description("Receiver side (trainbuilder) config")
                .commit();

        STRING_ELEMENT(schema).key("qsfp.chan" + ch + ".send.macaddr")
                .displayedName("MAC")
                .description("PPT MAC address")
                .tags("ethParam")
                .assignmentOptional()
                .defaultValue("aa:bb:cc:dd:ee:0" + std::to_string(ADDR))
                .reconfigurable()
                .commit();

        STRING_ELEMENT(schema).key("qsfp.chan" + ch + ".send.ipaddr")
                .displayedName("IP")
                .description("PPT hostname or IP address")
                .tags("ethParam")
                .assignmentOptional()
                .defaultValue("192.168.142.5" + std::to_string(ADDR))
                .reconfigurable()
                .commit();

        UINT32_ELEMENT(schema).key("qsfp.chan" + ch + ".send.port")
                .displayedName("UDP Port")
                .description("PPT port number")
                .tags("ethParam")
                .assignmentOptional()
                .defaultValue(8000)
                .reconfigurable()
                .commit();
}


void init_eth_elements(karabo::data::Schema& schema) {
        NODE_ELEMENT(schema).key("qsfp")
                .displayedName("QSFP")
                .description("Configuration of QSFP control registers: MAC, IP, Port")
                .expertAccess()
                .commit();

        init_ethernet_element(schema, 1, 0);
        init_ethernet_element(schema, 2, 1);
        init_ethernet_element(schema, 3, 2);
        init_ethernet_element(schema, 4, 3);
}

#endif	/* DSSCPPTREGSINIT_HH */

