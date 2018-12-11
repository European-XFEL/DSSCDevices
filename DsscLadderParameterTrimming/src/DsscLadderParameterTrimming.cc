/*
 * Author: <kirchgessner>
 *
 * Created on February, 2018, 05:00 PM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "DsscLadderParameterTrimming.hh"
#include "CHIPGainConfigurator.h"
#include "DsscHDF5TrimmingDataWriter.h"
#include "DsscHDF5TrimmingDataReader.h"
#include "FitUtils.h"
#include "DsscModuleInfo.h"

#define DSSCSTATUS(statStr) \
      KARABO_LOG_INFO << statStr;\
      set<string>("status",statStr);

//#ifdef F1IO
//#define INITIALCONF "ConfigFiles/Init.conf"
//#elif F2IO
#define INITIALCONF "ConfigFiles/F2Init.conf"
//#endif

using namespace std;

USING_KARABO_NAMESPACES

namespace karabo {

KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, DsscLadderParameterTrimming)

void DsscLadderParameterTrimming::expectedParameters(Schema& expected)
{
  STRING_ELEMENT(expected).key("environment")
    .displayedName("Environment")
    .description("Select environment where device is operated")
    .assignmentMandatory().options("MANNHEIM,FENICE", ",")
    .commit();

  STRING_ELEMENT(expected).key("quadrantId")
    .displayedName("Dssc Quadrant Id")
    .description("Id Qualified for all Devices operating this Ladder")
    .assignmentMandatory().options(utils::DsscModuleInfo::getQuadrantIdList(),",")
    .commit();

  STRING_ELEMENT(expected).key("pptDeviceServerId")
    .displayedName("Dssc Device Server Id")
    .description("Id Qualified for all Devices operating this Ladder")
    .assignmentMandatory()
    .commit();

  STRING_ELEMENT(expected).key("recvDeviceServerId")
    .displayedName("DataReceiver Server Id")
    .description("Id Qualified for all Devices which handle received train data")
    .assignmentMandatory()
    .commit();

  UINT32_ELEMENT(expected).key("recvDeviceUDPPort")
    .displayedName("Receiver UDP Port")
    .description("UDP Port of data receiver device")
    .assignmentMandatory().reconfigurable()
    .minInc(8000).maxExc(9000)
    .commit();

  STRING_ELEMENT(expected).key("gainSelection")
    .displayedName("Select Gain Settings")
    .description("Gain Options")
    .tags("general")
    .assignmentOptional().defaultValue("Gain_0.7keV/1Bin").reconfigurable()
    .options(SuS::CHIPGainConfigurator::s_gainModeNames)
    .commit();

  SLOT_ELEMENT(expected)
    .key("setSelectedGainConfig")
    .displayedName("Enable Selected Gain Config")
    .description("Enable Selected Gain Config")
    .commit();

  NODE_ELEMENT(expected).key("gain")
    .description("Coarse Gain Settings")
    .displayedName("Coarse Gain Settings")
    .commit();

  SLOT_ELEMENT(expected)
    .key("setCoarseGainSettings")
    .displayedName("Set Coarse Gain Settings")
    .description("Update Configuration and set coarse Gain Settings")
    .commit();

  UINT32_ELEMENT(expected).key("gain.fcfEnCap")
    .displayedName("FCF_EnCap")
    .description("FCF Feedback Capacity setting")
    .tags("coarseGain")
    .assignmentOptional().defaultValue(1).reconfigurable()
    .minExc(0).maxExc(16)
    .commit();

  UINT32_ELEMENT(expected).key("gain.csaFbCap")
    .displayedName("CSA_FbCap")
    .description("CSA Feedback Capacitor setting")
    .tags("coarseGain")
    .assignmentOptional().defaultValue(0).reconfigurable()
    .minInc(0).maxExc(8)
    .commit();

  UINT32_ELEMENT(expected).key("gain.csaResistor")
    .displayedName("CSA Resistor")
    .description("Resistor setting of the CSA, the larger the higher the gain")
    .tags("coarseGain")
    .assignmentOptional().defaultValue(3).reconfigurable()
    .minInc(0).maxInc(7)
    .commit();

  UINT32_ELEMENT(expected).key("gain.csaInjCap")
    .displayedName("CSA Injection Cap")
    .description("Injection Cap setting of the CSA")
    .tags("coarseGain")
    .assignmentOptional().defaultValue(0).reconfigurable()
    .minInc(0).maxInc(7)
    .commit();

  BOOL_ELEMENT(expected).key("gain.csaInjCap200")
    .displayedName("En CSA Injection Cap 200")
    .description("enable large 200 Injection Cap setting of the CSA")
    .tags("coarseGain")
    .assignmentOptional().defaultValue(false).reconfigurable()
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

  STRING_ELEMENT(expected).key("gain.pixelDelay")
      .displayedName("Pixel Delay")
      .description("Selected Pixel Delay Setting")
      .readOnly()
      .commit();

  NODE_ELEMENT(expected).key("moduleInfo")
    .description("Current ModuleInfo as filled into HDF5 files")
    .displayedName("ModuleInfo")
    .commit();

  STRING_ELEMENT(expected).key("moduleInfo.quadrantId")
      .displayedName("Quadrant ID")
      .readOnly()
      .commit();

  UINT32_ELEMENT(expected).key("moduleInfo.moduleNr")
    .displayedName("ModuleNr")
    .readOnly()
    .commit();

  STRING_ELEMENT(expected).key("moduleInfo.iobSerial")
      .displayedName("IobSerial")
      .readOnly()
      .commit();

  PATH_ELEMENT(expected).key("fullConfigFileName")
    .description("Path Full Config File")
    .displayedName("Full Config File")
    .isInputFile()
    .tags("general")
    .assignmentMandatory().reconfigurable()
    .commit();


  BOOL_ELEMENT(expected).key("isCalibrationDataConfig")
    .displayedName("Is Calibration Data Config")
    .description("enable before calibration data config is loaded")
    .assignmentOptional().defaultValue(false).reconfigurable()
    .commit();


  UINT64_ELEMENT(expected).key("currentTrainId")
    .displayedName("Current Train Id")
    .description("Last received train Id")
    .readOnly()
    .commit();

  INT32_ELEMENT(expected).key("activeModule")
    .displayedName("Active Module")
    .description("Module or Ladder number which is currently active")
    .tags("general")
    .assignmentOptional().defaultValue(1).reconfigurable()
    .maxExc(5).minExc(0)
    .commit();


  BOOL_ELEMENT(expected).key("saveTrimmingData")
    .displayedName("Save Trimming Data")
    .description("Save Data from diffenrent knobs to HDF5 files into output directory")
    .tags("general")
    .assignmentOptional().defaultValue(false).reconfigurable()
    .commit();

  PATH_ELEMENT(expected).key("outputDir")
    .displayedName("Output Directory")
    .description("Output directory for HDF5 files")
    .isDirectory()
    .assignmentOptional().defaultValue("./").reconfigurable()
    .commit();

  UINT32_ELEMENT(expected).key("numRuns")
    .displayedName("NumRuns")
    .description("Used in Trimming routines")
    .tags("trimming")
    .assignmentOptional().defaultValue(2).reconfigurable()
    .minExc(0)
    .commit();

  UINT32_ELEMENT(expected).key("numIterations")
    .displayedName("Iterations")
    .description("Iterations for each sample point during trimming")
    .tags("meanReceiver")
    .assignmentOptional().defaultValue(10).reconfigurable()
    .minExc(0)
    .commit();

  UINT16_ELEMENT(expected).key("minSram")
    .displayedName("Min Sram")
    .description("Minimum SRAM address for mean value computation")
    .tags("meanReceiver")
    .assignmentOptional().defaultValue(0).reconfigurable()
    .maxExc(800)
    .commit();

  UINT16_ELEMENT(expected).key("maxSram")
    .displayedName("Max Sram")
    .description("Maximum (included) SRAM address for mean value computation")
    .tags("meanReceiver")
    .assignmentOptional().defaultValue(799).reconfigurable()
    .maxExc(800)
    .commit();

  STRING_ELEMENT(expected).key("dataSource")
    .displayedName("DataSource for trimming")
    .description("Select Datasource for trimming")
    .assignmentMandatory().options({"DummyData", "DsscDataReceiver", "XFELDAQSystem"})
  .commit();

  STRING_ELEMENT(expected).key("sendingASICs")
    .displayedName("SendingASICs")
    .description("Indicates which of the 16 ladder ASICs are sending data: 11110000_11110000")
    .tags("general")
    .assignmentMandatory()
    .commit();

  STRING_ELEMENT(expected).key("injectionMode")
    .displayedName("Injection Mode")
    .description("Select Injection Mode, can be used for trimming")
    .tags("trimming")
    .assignmentOptional().defaultValue("NORM").reconfigurable()
    .options({"CURRENT_BGDAC", "CURRENT_SUSDAC", "CHARGE_PXINJ_BGDAC", "CHARGE_PXINJ_SUSDAC", "CHARGE_BUSINJ", "ADC_INJ", "ADC_INJ_LR", "EXT_LATCH", "NORM"})
    .commit();


  SLOT_ELEMENT(expected)
    .key("setSelectedInjectionMode")
    .displayedName("Enable Selected Injection Mode")
    .description("Enable Selected Injection Mode")
    .commit();

  PATH_ELEMENT(expected).key("importDataFileName")
    .displayedName("Import Data Filename")
    .description("Select file to import DNL or Gain Values")
    .isInputFile()
    .assignmentOptional().defaultValue("./").reconfigurable()
    .commit();

  SLOT_ELEMENT(expected)
    .key("importBinningInformationFile")
    .displayedName("Import Binning File")
    .description("Import DNL and Binnin information form selected file")
    .commit();

  SLOT_ELEMENT(expected)
    .key("importADCGainMapFile")
    .displayedName("Import ADC Gain Map")
    .description("Import ADC Gain Map slopes information from selected file")
    .commit();

  SLOT_ELEMENT(expected)
    .key("importSpektrumFitResultsFile")
    .displayedName("Import spectrum Fit results from File")
    .description("Import SpektrumFitResults from selected file")
    .commit();

  SLOT_ELEMENT(expected)
    .key("computeTargetGainADCConfiguration")
    .displayedName("Compute Target Gain Configuration")
    .description("Requires ADCGainMap and Spectrum fit results laoded in advance")
    .commit();

  BOOL_ELEMENT(expected).key("binningInfoLaoded")
    .displayedName("Binning Information Loaded")
    .readOnly().initialValue(false)
    .commit();

  BOOL_ELEMENT(expected).key("pixelAdcGainMapLoaded")
    .displayedName("Pixel ADC Gain Map Loaded")
    .readOnly().initialValue(false)
    .commit();

  BOOL_ELEMENT(expected).key("spectrumFitResultsLoaded")
    .displayedName("Spectrum Fit Results Loaded")
    .readOnly().initialValue(false)
    .commit();

  BOOL_ELEMENT(expected).key("pixelCalibrationDataSettingsValid")
    .displayedName("Pixel Calibration Data Settings Valid")
    .readOnly().initialValue(false)
    .commit();


  SLOT_ELEMENT(expected)
    .key("setGainConfigurationFromFitResultsFile")
    .displayedName("Set Gain Configuration from FitResults file")
    .description("Import SpektrumFitResults from selected file")
    .commit();

  DOUBLE_ELEMENT(expected).key("calibrateTargetGain")
    .displayedName("Target Gain")
    .description("Select Target Gain value for ADC configuration generation")
    .assignmentOptional().defaultValue(0.7).reconfigurable()
    .commit();

  STRING_ELEMENT(expected).key("pixelsToChange")
    .displayedName("Select Pixels")
    .description("Select Pixels: a/all = sel all avaialable pixels")
    .tags("trimming")
    .assignmentOptional().defaultValue("0-65535").reconfigurable()
    .commit();

  STRING_ELEMENT(expected).key("asicsToChange")
    .displayedName("Select Asics")
    .description("Select ASICs: a/all = sel all avaialable pixels")
    .tags("trimming")
    .assignmentOptional().defaultValue("0-15").reconfigurable()
    .commit();

  STRING_ELEMENT(expected).key("asicsPixelsToChange")
    .displayedName("Select Asic Pixels")
    .description("Select Asic Pixels: a/all = sel all avaialable pixels")
    .tags("trimming")
    .assignmentOptional().defaultValue("0-4095").reconfigurable()
    .commit();


  INT32_ELEMENT(expected).key("valueToAdd")
    .displayedName("Add Value")
    .description("Value diff to change pixel parameter")
    .tags("trimming")
    .assignmentOptional().defaultValue(1).reconfigurable()
    .minExc(-64)
    .maxExc(64)
    .commit();

  STRING_ELEMENT(expected).key("pixelsRegisterSignalName")
    .displayedName("PixelReg Signals")
    .description("Available Signals in Pixel registers")
    .tags("trimming")
    .assignmentOptional().defaultValue("PxsIrampTrm").reconfigurable()
    .commit();

  SLOT_ELEMENT(expected)
    .key("enableInjectionInSelPixels")
    .displayedName("Enable Injection in selected Pixels")
    .description("Enable Injection in selected Pixels")
    .commit();

  SLOT_ELEMENT(expected)
    .key("addValueToPxRegSignal")
    .displayedName("Inc/Dec Px Reg Value")
    .description("Inc/Dec Px Reg Value")
    .commit();

  SLOT_ELEMENT(expected)
    .key("powerUpSelPixels")
    .displayedName("Power Down Selected Pixels")
    .description("Power Down Selected Pixels")
    .commit();

  STRING_ELEMENT(expected).key("monBusColsSelect")
    .displayedName("Select Mon Bus Columns")
    .description("Select Enabled Mon bus columns, for all ASICs identical")
    .tags("trimming")
    .assignmentOptional().defaultValue("0-63").reconfigurable()
    .commit();

  SLOT_ELEMENT(expected)
    .key("enableMonBusInCols")
    .displayedName("Enable Mon Bus in Sel Cols")
    .description("Enable Monbus in selected Columns")
    .commit();

  SLOT_ELEMENT(expected)
    .key("setBufferMode")
    .displayedName("Enable Sequencer Buffer Mode")
    .commit();

  UINT16_ELEMENT(expected).key("sramPattern")
    .displayedName("Sram Pattern")
    .description("Pattern to clock into ASIC via Jtag")
    .assignmentOptional().defaultValue(120).reconfigurable()
    .maxExc(512)
    .commit();

  SLOT_ELEMENT(expected)
    .key("fillSramAndReadoutPattern")
    .displayedName("FillSramAndReadoutPattern")
    .description("Fill Sram And Readout Pattern")
    .commit();

  SLOT_ELEMENT(expected)
    .key("matrixSRAMTest")
    .displayedName("MatrixSRAMTest")
    .description("Fill Sram And Readout Pattern like in the reticle test for the ladder, also generates bad SRAM cell information which can be imported in SramBlacklist")
    .commit();

  SLOT_ELEMENT(expected)
    .key("measureBurstData").displayedName("Measure Burst Data")
    .description("Test function to trigger MeasureBurstData")
    .commit();

  SLOT_ELEMENT(expected)
    .key("doSingleCycle2").displayedName("Do Single Cycle")
    .description("Test function to trigger doSingleCycle")
    .commit();

  BOOL_ELEMENT(expected).key("baselineAvailable")
    .displayedName("Baseline Available")
    .description("Signals if baseline was acquired")
    .readOnly()
    .commit();

  SLOT_ELEMENT(expected)
    .key("setBaseline").displayedName("Set Baseline")
    .description("Function to trigger setBaseline")
    .commit();

 SLOT_ELEMENT(expected)
    .key("clearBaseline").displayedName("Clear Baseline")
    .description("Clear Baseline Values")
    .commit();

  SLOT_ELEMENT(expected)
    .key("measureMeanSramContent").displayedName("Mean SRAM Content")
    .description("Measure Mean SRAM Content")
    .commit();

  SLOT_ELEMENT(expected)
    .key("measureMeanSramContentAllPix").displayedName("Mean SRAM Content for Sram Correction")
    .description("Measure Mean SRAM Content for whole ladder and store sram correction file")
    .commit();

  SLOT_ELEMENT(expected)
    .key("runPixelDelayTrimming").displayedName("Pixel Delay Trimming")
    .description("Starts a trimming run of the pixel delay setting: valid parameters: Min Sram,Max Sram,Iterations")
    .commit();

  DOUBLE_ELEMENT(expected)
    .key("trimTargetSlope")
    .displayedName("Target Slope")
    .description("Target Slope for GainTrimming")
    .assignmentOptional().defaultValue(0.0362).reconfigurable()
    .commit();

  UINT16_ELEMENT(expected)
    .key("trimMaxRelAbsDiff")
    .displayedName("Trimming Goal[%]")
    .description("Maximum relative difference goad for trimming in percent")
    .assignmentOptional().defaultValue(2).reconfigurable()
    .commit();

  PATH_ELEMENT(expected).key("injectionCalibrationFileName")
    .displayedName("Injection Calibration Data Filename")
    .description("Select file the keeps injection calibration data .h5 and .txt files compatible")
    .isInputFile()
    .assignmentOptional().defaultValue("./").reconfigurable()
    .commit();

  SLOT_ELEMENT(expected)
    .key("runGainTrimming").displayedName("Run Gain Trimming")
    .description("Starts a trimming run of the iramp current setting: valid parameters: Min Sram,Max Sram,Iterations, targetSlope, maxDiff")
    .commit();

   UINT32_ELEMENT(expected).key("displayPixel")
    .displayedName("Display Image Pixel")
    .description("")
    .assignmentOptional().defaultValue(12345).reconfigurable()
    .commit();

  UINT32_ELEMENT(expected).key("burstWaitOffsetPixel")
    .displayedName("Image Pixel for Burst SRAM Sweep")
    .description("Specify image pixel number for burst wait offset sweep. "
    "Only one single pixel can be measured. "
    "Take care that this pixel is hit by the beam.")
    .assignmentOptional().defaultValue(12345).reconfigurable()
    .commit();

  UINT32_ELEMENT(expected).key("numFramesToReceive")
    .displayedName("Num Frames to Receive")
    .description("Number of frame to process for all asics")
    .assignmentOptional().defaultValue(800).reconfigurable()
    .tags("trimming")
    .maxExc(801)
    .commit();

  STRING_ELEMENT(expected).key("burstWaitOffsetRange")
    .displayedName("Burst Wait Offset Range")
    .description("Define the range for the burst wait offset sweep. "
    "Twice the cycle length is in general sufficient. "
    "During single shot operation, the hit image number has to be specified.")
    .assignmentOptional().defaultValue("10-60").reconfigurable()
    .commit();

  SLOT_ELEMENT(expected)
    .key("measureBurstOffsetSweep").displayedName("Measure Burst Offset Sweep")
    .description("Starts a parameter sweep of the burst_wait_offset parameter in 10 ns steps")
    .commit();

  SLOT_ELEMENT(expected)
    .key("measureADCGainMap").displayedName("Measure ADC Gain Map")
    .description("Measure ADC Gain Map for all selected Iramp Settings")
    .commit();

  SLOT_ELEMENT(expected)
    .key("displayBinValuesOfPixel").displayedName("Display BinValues OfPixel")
    .description("Displays InjectionSweepValues if measured")
    .commit();



  STRING_ELEMENT(expected).key("selChipParts")
    .displayedName("Select Chip Parts")
    .description("advanced selection patterns possible")
    .assignmentOptional().defaultValue("col0-63:8").reconfigurable()
    .commit();

  STRING_ELEMENT(expected).key("injectionSweepRange")
    .displayedName("Injection Sweep Range")
    .description("Injection Sweep Range for measuring injection sweeps and compute slopes")
    .assignmentOptional().defaultValue("3000-7000;1000").reconfigurable()
    .commit();

  STRING_ELEMENT(expected).key("irampSettingsRange")
    .displayedName("Select IrampSettings Range")
    .description("Select Range to determine ADC Gain Map, 0-63, or 'all' for all settings")
    .assignmentOptional().defaultValue("all").reconfigurable()
    .commit();

  SLOT_ELEMENT(expected)
    .key("measureInjectionSweepSlopes").displayedName("Measure Injection Sweep Slopes")
    .description("Starts a parameter sweep of the signal injection and extracts slope Values")
    .commit();

  SLOT_ELEMENT(expected)
    .key("measureBinningInformation").displayedName("Measure Binning Information")
    .description("Starts a fine parameter sweep of the ADJ_INJ in order to acquire binning information")
    .commit();

  SLOT_ELEMENT(expected)
    .key("generateSramBlacklist").displayedName("Generate SRAM Blacklist")
    .description("Measure Mean SRAM Content for all pixels and find outliers")
    .commit();

  SLOT_ELEMENT(expected)
    .key("stopTrimming").displayedName("Abort")
    .description("Abort")
    .commit();

  UINT32_ELEMENT(expected).key("testImagePixel")
    .displayedName("Image Pixel Number")
    .description("Helper field to compute image pixel from ASIC pixel and vice versa.")
    .tags("helper")
    .assignmentOptional().defaultValue(12345).reconfigurable()
    .maxExc(65536)
    .commit();

  UINT32_ELEMENT(expected).key("testAsicNumber")
    .displayedName("ASIC number")
    .description("Helper field to compute image pixel from ASIC pixel and vice versa.")
    .tags("helper")
    .assignmentOptional().defaultValue(12).reconfigurable()
    .maxExc(16)
    .commit();

  UINT32_ELEMENT(expected).key("testAsicPixel")
    .displayedName("ASIC Pixel number ")
    .description("Helper field to compute image pixel from ASIC pixel and vice versa.")
    .tags("helper")
    .assignmentOptional().defaultValue(1000).reconfigurable()
    .maxExc(4096)
    .commit();

  BOOL_ELEMENT(expected).key("saveTrimmingRawData")
    .displayedName("Save Image Data During Trimming")
    .description("Save Raw Image Data during Trimming. Only available for DsscDataReceiver")
    .tags("trimming")
    .assignmentOptional().defaultValue(false).reconfigurable()
    .commit();

  BOOL_ELEMENT(expected).key("saveTrimmingHistoData")
    .displayedName("Save Histo Data During Trimming")
    .description("Save Histograms during Trimming.")
    .tags("trimming")
    .assignmentOptional().defaultValue(false).reconfigurable()
    .commit();

  BOOL_ELEMENT(expected).key("subtractBaseline")
    .displayedName("Subtract Baseline")
    .description("Subtract Baseline")
    .tags("acquisition")
    .assignmentOptional().defaultValue(false).reconfigurable()
    .commit();

  BOOL_ELEMENT(expected).key("acquireNewData")
    .displayedName("Acquire New Data")
    .description("Acquire new Data or show data with other options")
    .tags("acquisition")
    .assignmentOptional().defaultValue(false).reconfigurable()
    .commit();

  BOOL_ELEMENT(expected).key("displayRms")
    .displayedName("Display RMS Value")
    .description("Display RMS Values")
    .tags("acquisition")
    .assignmentOptional().defaultValue(false).reconfigurable()
    .commit();

  STRING_ELEMENT(expected).key("displayMode")
    .displayedName("Display Mode")
    .description("Select different modes to display data in Histogram or Image.")
    .tags("acquisition")
    .assignmentOptional().defaultValue("AsicImages").reconfigurable()
    .options("AsicImages,FinalSlopes,SramDrift,SramSlopes,Curvature", ",")
    .commit();

  SLOT_ELEMENT(expected)
    .key("acquireHistogram").displayedName("Acquire Histogram")
    .description("Acquire Histogram with given options")
    .commit();

  SLOT_ELEMENT(expected)
    .key("acquireImage").displayedName("Acquire Image")
    .description("Acquire Image with given options")
    .commit();


  NODE_ELEMENT(expected).key("meanBurstData")
          .description("Vectors For Injection Sweep Outputs")
          .displayedName("Injection Sweeps Outputs")
          .commit();

  VECTOR_UINT32_ELEMENT(expected).key("meanBurstData.burstDataXValues")
            .displayedName("Burst Data X Values")
            .readOnly().initialValue(std::vector<unsigned int>(20,0))
            .commit();

  VECTOR_DOUBLE_ELEMENT(expected).key("meanBurstData.burstDataYValues")
            .displayedName("Burst Data Y Values")
            .readOnly().initialValue(std::vector<double>(20,0.0))
            .commit();

  BOOL_ELEMENT(expected).key("meanBurstData.showMeanBurstData")
            .displayedName("Show Mean burst Data Graph")
            .description("Prints a graph for selected pixel after all settings are done")
            .assignmentOptional().defaultValue(false)
            .reconfigurable()
            .commit();

  BOOL_ELEMENT(expected).key("meanBurstData.userSelPixel")
            .displayedName("Usersel Pixel")
            .description("Allows to define the displayed pixel by the user, if false the last active pixel is displayed")
            .assignmentOptional().defaultValue(false)
            .reconfigurable()
            .commit();

  UINT32_ELEMENT(expected).key("meanBurstData.displayPixel")
            .displayedName("Display Image Pixel")
            .readOnly().initialValue(0)
            .commit();

  Schema outSchema;
  OUTPUT_CHANNEL(expected).key("registerConfigOutput")
    .displayedName("Register Config Output")
    .dataSchema(outSchema)
    .commit();

  INPUT_CHANNEL(expected).key("pixelDataInput")
    .displayedName("Pixel Data Input")
    .commit();

  INPUT_CHANNEL(expected).key("meanDataInput")
    .displayedName("Mean Data Input")
    .commit();


  Schema pixelSchema;
  VECTOR_DOUBLE_ELEMENT(pixelSchema).key("pixelData")
    .displayedName("Pixel Data")
    .readOnly().initialValue(std::vector<double>(800, 0.0))
    .commit();

  OUTPUT_CHANNEL(expected).key("pixelValuesOutput")
    .displayedName("Single Pixel Data Output")
    .dataSchema(pixelSchema)
    .commit();

  NODE_ELEMENT(expected).key("ladderHisto")
    .description("Ladder Histogram Generation")
    .displayedName("Histogram Generation")
    .commit();

  VECTOR_UINT32_ELEMENT(expected).key("ladderHisto.binValues")
    .displayedName("Ladder BinValues")
    .readOnly().initialValue(std::vector<unsigned int>(20, 0))
    .commit();

  VECTOR_UINT16_ELEMENT(expected).key("ladderHisto.bins")
    .displayedName("Ladder Bins")
    .readOnly().initialValue(std::vector<unsigned short>(20, 0))
    .commit();

  DOUBLE_ELEMENT(expected).key("ladderHisto.xScaleFactor")
    .displayedName("XAxis Scalefactor")
    .readOnly().initialValue(1)
    .commit();

  UINT32_ELEMENT(expected).key("ladderHisto.displayPixel")
    .displayedName("Shown Pixel")
    .readOnly().initialValue(0)
    .commit();

  BOOL_ELEMENT(expected).key("ladderHisto.enLogScale")
    .displayedName("Enable LogScale")
    .assignmentOptional().defaultValue(false)
    .reconfigurable()
    .commit();

  BOOL_ELEMENT(expected).key("ladderHisto.userSelPixel")
    .displayedName("Usersel Pixel")
    .description("Allows to define the displayed pixel by the user, if false the last active pixel is displayed")
    .assignmentOptional().defaultValue(false)
    .reconfigurable()
    .commit();

  Schema ladderSchema;
  IMAGEDATA(ladderSchema).key("ladderImage")
    .setDimensions("128,512")
    .commit();

  OUTPUT_CHANNEL(expected).key("ladderImageOutput")
    .displayedName("Ladder Image Output")
    .dataSchema(ladderSchema)
    .commit();

}

DsscLadderParameterTrimming::DsscLadderParameterTrimming(const karabo::util::Hash& config)
: Device<>(config),
  SuS::MultiModuleInterface(new SuS::PPTFullConfig(INITIALCONF)),
  m_asicMeanValues(utils::s_totalNumPxs),
  m_pixelData(utils::s_totalNumPxs*utils::s_numSram),
  m_currentTrimmer(nullptr)
{
  KARABO_INITIAL_FUNCTION(initialization)

  KARABO_SLOT(measureBurstData);
  KARABO_SLOT(doSingleCycle2);
  KARABO_SLOT(setBaseline);
  KARABO_SLOT(clearBaseline);
  KARABO_SLOT(fillSramAndReadoutPattern);
  KARABO_SLOT(matrixSRAMTest);
  KARABO_SLOT(runPixelDelayTrimming);
  KARABO_SLOT(runGainTrimming);
  KARABO_SLOT(enableInjectionInSelPixels);
  KARABO_SLOT(setSelectedInjectionMode);
  KARABO_SLOT(setSelectedGainConfig);
  KARABO_SLOT(setCoarseGainSettings);
  KARABO_SLOT(powerUpSelPixels);
  KARABO_SLOT(addValueToPxRegSignal);
  KARABO_SLOT(enableMonBusInCols);
  KARABO_SLOT(measureBurstOffsetSweep);
  KARABO_SLOT(measureInjectionSweepSlopes);
  KARABO_SLOT(measureADCGainMap);
  KARABO_SLOT(measureBinningInformation);
  KARABO_SLOT(measureMeanSramContent);
  KARABO_SLOT(measureMeanSramContentAllPix);
  KARABO_SLOT(acquireHistogram);
  KARABO_SLOT(acquireImage);
  KARABO_SLOT(stopTrimming);
  KARABO_SLOT(setBufferMode);
  KARABO_SLOT(importBinningInformationFile);
  KARABO_SLOT(importADCGainMapFile);
  KARABO_SLOT(importSpektrumFitResultsFile);
  KARABO_SLOT(setGainConfigurationFromFitResultsFile);
  KARABO_SLOT(computeTargetGainADCConfiguration);
  KARABO_SLOT(displayBinValuesOfPixel);
  KARABO_SLOT(generateSramBlacklist);
}

DsscLadderParameterTrimming::~DsscLadderParameterTrimming()
{
  m_asicChannelDataValid = true;
}

void DsscLadderParameterTrimming::stopTrimming()
{
  KARABO_LOG_WARN << "Abort Trimming by user abort";

  runTrimming = false;

  if(m_currentTrimmer){
    m_currentTrimmer->abortTrimming();
  }

  changeDeviceState(State::STOPPING);
}


void DsscLadderParameterTrimming::changeDeviceState(const util::State & newState)
{
  try{
    updateState(newState);
  }
  catch(...)
  {
    KARABO_LOG_WARN << "DsscLadderParameterTrimming: changeDeviceState : WARNING STATE COULD NOT BE UPDATED!!!!";
  }
}


void DsscLadderParameterTrimming::initialization()
{
  cout << "Initializing LadderTrimmer " << endl;

  KARABO_ON_DATA("pixelDataInput", onPixelData);
  KARABO_ON_DATA("meanDataInput", onMeanData);


  cout << "Initialized data channels " << endl;

  changeDeviceState(State::STARTING);

  m_quadrantId = get<string>("quadrantId");

  m_quadrantServerId = get<string>("pptDeviceServerId"); //"pptDeviceServer1"; // "Dssc" + m_quadrantId + "DeviceServer";

  m_pptDeviceId = "DsscPpt_" + m_quadrantId;

  m_recvServerId = get<string>("recvDeviceServerId");

  m_mainProcessorId = "DsscMainProcessor_" + m_quadrantId + "_Mod" + to_string(get<int>("activeModule"));

  m_testDataGeneratorId = "DsscDummyTrainDataGenerator_" + m_quadrantId;

  m_dsscDataReceiverId = "DsscDataReceiver_" + m_quadrantId + "_Mod" + to_string(get<int>("activeModule"));

  m_lastTrainId = 0;
  m_startTrimming = true;
  m_runFastAcquisition = false;
  m_saveRawData = false;

  m_iterations = get<unsigned int>("numIterations");
  m_trimStartAddr = get<unsigned short>("minSram");
  m_trimEndAddr = get<unsigned short>("maxSram");
  m_numRuns = get<unsigned int>("numRuns");

  m_recvMode = RecvMode::MEAN;
  m_recvStatus = RecvStatus::OK;

  m_deviceConfigState = ConfigState::CHANGED;
  m_asicChannelDataValid = false;
  setD0Mode(true);

  setActiveAsics(0xFFFF);
  cout << "helloxxxx1" << endl;
  setLadderReadout(true);

  // pixel sort map is always in ladder mode filled
  initPixelSortMap();

  setSendingAsics(utils::bitEnableStringToValue(get<string>("sendingASICs")));

  if (allDevicesAvailable()) {
    KARABO_LOG_INFO << "All Devices started, ready for trimming routines";
  } else {
    changeDeviceState(State::ERROR);
  }
  changeDeviceState(State::OFF);

  injectionMode = NORM;

  loadCoarseGainParamsIntoGui();

  m_calibGenerator.setCurrentPixels(utils::positionListToVector<int>("0-65535"));
  m_calibGenerator.setOutputDir(get<string>("outputDir"));

  updateActiveModule(get<int>("activeModule"));
  updateBaselineValid();
}


SuS::CHIPTrimmer DsscLadderParameterTrimming::getNewChipTrimmer(bool &ok)
{
  runTrimming = true;

  //enable data sending to dsscProcessor if disabled from somewhere else
  if (isDsscData()) {
    remote().set<bool>(m_dsscDataReceiverId, "enableDAQOutput",true);
  }

  initDataWriter();

  SuS::CHIPTrimmer trimmer(this);
  trimmer.setAsicWise(false);

  auto chipParts = get<string>("selChipParts");
  utils::replace(chipParts, ";", ":");
  utils::replace(chipParts, ".", ":");
  set<string>("selChipParts", chipParts);

  std::vector<uint32_t> paramValues;
  ok = utils::getSimpleSweepVector(get<string>("injectionSweepRange"), paramValues);
  if (!ok) {
    KARABO_LOG_ERROR << "injectionSweepRange has a bad range";
    return trimmer;
  }

  cout << "ParamValues " << paramValues.size() << endl;
  trimmer.setChipParts(chipParts);
  trimmer.setDacRange(paramValues.front(), paramValues.back());
  trimmer.setNumDacVals(paramValues.size());
  trimmer.setSramRange(m_trimStartAddr, m_trimEndAddr);
  trimmer.setOutputDir(get<string>("outputDir"));

  loadPxInjCalibData(&trimmer);
  m_currentTrimmer = &trimmer;
  return trimmer;
}

bool DsscLadderParameterTrimming::isTestData()
{
  return get<string>("dataSource") == "DummyData";
}

bool DsscLadderParameterTrimming::isDsscData()
{
  return get<string>("dataSource") == "DsscDataReceiver";
}

bool DsscLadderParameterTrimming::isXFELData()
{
  return get<string>("dataSource") == "XFELDAQSystem";
}

void DsscLadderParameterTrimming::setBufferMode()
{
  sequencer->setOpMode(SuS::Sequencer::BUFFER);
  programSequencer();

  KARABO_LOG_INFO << "Sequencer Mode activated";
}

void DsscLadderParameterTrimming::initPixelSortMap()
{
  KARABO_LOG_INFO << "Init Pixel Sort Map";
  m_pixelSortMap.assign(totalNumPxs, 0);

#pragma omp parallel for
  for (int px = 0; px < totalNumPxs; px++) {
    m_pixelSortMap[px] = utils::s_dataPixelMap[px]*sramSize;
  }
}

const uint16_t * DsscLadderParameterTrimming::getPixelSramData(int pixel)
{
  return m_pixelData.data() + m_pixelSortMap[pixel];
}

void DsscLadderParameterTrimming::initDataAcquisition()
{

  while(m_deviceConfigState == ConfigState::CHANGING){
    usleep(200000);
  }

  if(m_deviceConfigState == ConfigState::CHANGED){
    resetDataReceiver();
    m_deviceConfigState = ConfigState::VALID;
  }

  m_asicChannelDataValid = false;

  m_recvStatus = RecvStatus::OK;

  errorCodePixels.clear();
}

bool DsscLadderParameterTrimming::waitDataReceived()
{
  //KARABO_LOG_INFO << "Wait Data Received";
  initDataAcquisition();

  remote().execute(m_mainProcessorId, "accumulate");

  int cnt = 0;
  int timeout = m_iterations * 3000;

  utils::Timer timer;
  while (!m_asicChannelDataValid && cnt++ < timeout) {
    usleep(10000);
  }

  remote().execute(m_mainProcessorId, "stop");

  // disable raw data recording m_saveRawData is set only in dssc mode
  if (m_saveRawData) {

    remote().set(m_dsscDataReceiverId, "saveToHDF5", false);

    m_runSettingsVec.push_back(m_runSettingIdx);
    m_runDirectoryVec.push_back(m_runDirectory);
    m_runSettingIdx++;
  }

  bool ok = (cnt < timeout);
  if (!ok) {
    m_recvStatus = RecvStatus::TIMEOUT;
    DSSCSTATUS("DataReceiver Timeout");
    return false;
  }

  m_runFastAcquisition = false;

  //KARABO_LOG_DEBUG << "DataReceiver all data received. TrainID = " << get<unsigned long long>("currentTrainId");
  return true;
}

void DsscLadderParameterTrimming::acquireDisplayData()
{
  StateChangeKeeper keeper(this);

  const string displayMode = get<string>("displayMode");
  const bool subtract = get<bool>("subtractBaseline");
  const bool showRms = get<bool>("displayRms");

  if (displayMode == "AsicImages") {
    enableMeanValueAcquisition(showRms);

    errorCodePixels.clear();

    waitDataReceived();

    if (!showRms && subtract) {
      const size_t numPixels = m_asicMeanValues.size();
#pragma omp parallel for
      for (size_t idx = 0; idx < numPixels; idx++) {
        m_asicMeanValues[idx] = m_asicMeanValues[idx] - baselineValues[idx];
      }
    }
  } else if (displayMode == "FinalSlopes") {
    KARABO_LOG_WARN << " Final Slopes values are not implemented yet";
  } else if (displayMode == "SramDrift") {
    measureSramDriftMap(m_trimStartAddr, m_trimEndAddr);
  } else if (displayMode == "SramSlopes") {
    measureSramSlopesMap(m_trimStartAddr, m_trimEndAddr);
  } else if (displayMode == "Curvature") {
    KARABO_LOG_WARN << "Curvature values are not implemented yet";
  }
}

void DsscLadderParameterTrimming::acquireImage()
{
  const string displayMode = get<string>("displayMode");
  const bool acquire = get<bool>("acquireNewData");

  if (isDsscData()) {
    remote().set<bool>(m_dsscDataReceiverId, "enableDAQOutput",true);
    remote().execute(m_dsscDataReceiverId,"start",300);
  }

  if (acquire) {
    acquireDisplayData();
  }

  if (displayMode == "AsicImages") {
    showLadderImage(m_asicMeanValues);
  } else if (displayMode == "FinalSlopes") {
    showLadderImage(getFinalSlopes());
  } else if (displayMode == "SramDrift" || displayMode == "SramSlopes") {
    showLadderImage(getSramDriftValues());
  } else if (displayMode == "Curvature") {
    showLadderImage(getCurvatureValues());
  }
}

void DsscLadderParameterTrimming::acquireHistogram()
{
  const string displayMode = get<string>("displayMode");
  const bool acquire = get<bool>("acquireNewData");

  if (acquire) {
    acquireDisplayData();
  }

  if (displayMode == "AsicImages") {
    showLadderHisto(m_asicMeanValues);
  } else if (displayMode == "FinalSlopes") {
    showLadderHisto(getFinalSlopes());
  } else if (displayMode == "SramDrift" || displayMode == "SramSlopes") {
    showLadderHisto(getSramDriftValues());
  } else if (displayMode == "Curvature") {
    showLadderHisto(getCurvatureValues());
  }
}

std::vector<uint32_t> DsscLadderParameterTrimming::getPixelsToChange()
{
  std::vector<uint32_t> pixels;
  std::string pxStr = get<string>("pixelsToChange");
  if(pxStr.substr(0,3) == "col"){
    std::string chipPartsStr = pxStr;
    // select number of elem with #
    size_t elem = 0;
    size_t hashPos = pxStr.find('#');
    if(hashPos != std::string::npos){
      chipPartsStr = pxStr.substr(0,hashPos);
      std::vector<int> elems;
      utils::split(pxStr,'#',elems,1);
      elem = elems.front();
    }
    const auto chipParts = utils::getChipParts(chipPartsStr);
    if(chipParts.size()>elem){
      pixels = getSendingColumnPixels<uint32_t>(chipParts.at(elem));
      if(m_setColumns){
        set<string>("monBusColsSelect",chipParts.at(elem).substr(3));
        enableMonBusInCols();
        m_setColumns = false;
      }
    }
  }
  else
  {
    if(pxStr[0] == 'a'){
      pxStr = "0-" + std::to_string(totalNumPxs-1);
      set<string>("pixelsToChange",pxStr);
    }
    pixels = utils::positionListToVector(pxStr);
  }
  return pixels;
}

void DsscLadderParameterTrimming::showLadderHisto(const std::vector<double> & values)
{
  size_t numPixels = values.size();

  utils::DataHisto dataHisto;
  vector<unsigned short> corrValues(numPixels);

  double scaleFactor = utils::convertToUINT16(corrValues, values);

  dataHisto.add(corrValues.data(), numPixels);

  vector<unsigned short> bins;
  vector<unsigned int> binValues;

  dataHisto.getDrawValues(bins, binValues);

  if(get<bool>("ladderHisto.enLogScale")){
    std::transform(binValues.begin(),binValues.end(),binValues.begin(),[](int x){if(x==0) return 1; return x;});
  }

  set<vector<unsigned short>>("ladderHisto.bins", std::move(bins));
  set<vector<unsigned int>>("ladderHisto.binValues", std::move(binValues));

  set<double>("ladderHisto.xScaleFactor", scaleFactor);
  set<unsigned int>("ladderHisto.displayPixel", 65536);

  if (get<bool>("saveTrimmingData")) {
    string dataName = get<string>("displayMode");
    if (get<bool>("displayRms")) {
      dataName += "_RMS";
    }
    saveDataHisto(dataName, dataHisto, scaleFactor);
  }
}

void DsscLadderParameterTrimming::showLadderImage(const std::vector<double> & values)
{
  size_t numPixels = values.size();
  size_t numRows = 128;
  size_t numCols = 512;

  if (numPixels == 4096) {
    numRows = 64;
    numCols = 64;
  }

  NDArray imageArray(values.data(),
    numPixels,
    NDArray::NullDeleter(),
    Dims(numRows, numCols));
  util::Hash dataHash;
  dataHash.set("ladderImage", ImageData(imageArray));
  writeChannel("ladderImageOutput", dataHash);

  if (get<bool>("saveTrimmingData")) {
    string dataName = get<string>("displayMode");
    if (get<bool>("displayRms")) {
      dataName += "_RMS";
    }
    saveDataVector(dataName, values);
  }
}

void DsscLadderParameterTrimming::enableMeanValueAcquisition(bool showRms)
{
  m_recvMode = (showRms) ? RMS : MEAN;
  if (!isDeviceExisting(m_mainProcessorId)) return;

  if(isDsscData()){
    remote().set<bool>(m_dsscDataReceiverId, "enableDAQOutput",true);
  }

  remote().set<bool>(m_mainProcessorId, "measureMean", true);
  remote().set<bool>(m_mainProcessorId, "measureRMS", showRms);
}

void DsscLadderParameterTrimming::enablePixelValueAcquisition()
{
  m_recvMode = PIXEL;
  if (!isDeviceExisting(m_mainProcessorId)) return;

  if(isDsscData()){
    remote().set<bool>(m_dsscDataReceiverId, "enableDAQOutput",true);
  }

  remote().set<bool>(m_mainProcessorId, "measureMean", false);
  remote().set<bool>(m_mainProcessorId, "measureRMS", false);
}

void DsscLadderParameterTrimming::initSaveRawDataParams()
{
  if (!isDsscData()) {
    m_saveRawData = false;
  } else {
    m_saveRawData = get<bool>("saveTrimmingRawData");
    m_runSettingIdx = 0;
    m_runBaseDirectory = get<string>("outputDir");
    m_runDirectory = m_runBaseDirectory + "/Setting0";
    m_runDirectoryVec.clear();
    m_runSettingsVec.clear();

    remote().set<string>(m_dsscDataReceiverId, "outputDir", m_runBaseDirectory);
    remote().set<unsigned int>(m_dsscDataReceiverId, "numTrainsToStore", m_iterations);


    m_currentMeasurementConfig.numIterations = m_iterations;
    m_currentMeasurementConfig.numPreBurstVetos = remote().get<unsigned int>(m_pptDeviceId, "numPreBurstVetos");
    m_currentMeasurementConfig.ladderMode = (remote().get<bool>(m_dsscDataReceiverId, "ladderMode") ? 1 : 0);

    string asicsStr = remote().get<string>(m_dsscDataReceiverId, "asicsToRecord");
    m_currentMeasurementConfig.activeASICs = utils::bitEnableStringToVector(asicsStr);
  }
}

void DsscLadderParameterTrimming::finishSaveRawDataParams()
{
  m_saveRawData = false;

  string fileName = m_runBaseDirectory + "/MeasurementInfo.h5";
  DsscHDF5MeasurementInfoWriter measInfoWriter(fileName);

  m_currentMeasurementConfig.measurementDirectories = m_runDirectoryVec;
  m_currentMeasurementConfig.measurementSettings = m_runSettingsVec;

  measInfoWriter.addMeasurementConfig(m_currentMeasurementConfig);
}

std::vector<double> DsscLadderParameterTrimming::measureBurstData(const std::vector<uint32_t> & measurePixels,
  int STARTADDR, int ENDADDR, bool subtract)
{
  if (m_saveRawData) {
    m_runDirectory = m_runBaseDirectory + "/Setting" + to_string(m_runSettingIdx);
    // WARNING: this works only if DataReceiver runs on same system like LadderTrimming device
    utils::makePath(m_runDirectory);
    remote().set(m_dsscDataReceiverId, "outputDir", m_runDirectory);
    remote().set(m_dsscDataReceiverId, "saveToHDF5", true);
  }

  const int numPixels = measurePixels.size();

  vector<double> binValues(numPixels);

  enableMeanValueAcquisition(false);

  errorCodePixels.clear();

  if (!waitDataReceived()) { // disables also raw data recording
    return binValues;
  }

#pragma omp parallel for
  for (int idx = 0; idx < numPixels; idx++) {
    binValues[idx] = getPixelMeanValue(measurePixels[idx]);
  }

  if (subtract) {
#pragma omp parallel for
    for (int idx = 0; idx < numPixels; idx++) {
      binValues[idx] -= baselineValues[measurePixels[idx]];
    }
  }
  return binValues;
}

std::vector<double> DsscLadderParameterTrimming::measureRMSData(const vector<uint32_t> & measurePixels, const int STARTADDR, const int ENDADDR)
{
  static vector<double> pixelRMSValues;

  const int numPxs = measurePixels.size();
  pixelRMSValues.resize(numPxs);

  m_recvMode = RMS;

  errorCodePixels.clear();

  if (!waitDataReceived()) {
    return pixelRMSValues;
  }

  // getPixelMeanValue can also return rms values
#pragma omp parallel for
  for (int idx = 0; idx < numPxs; idx++) {
    pixelRMSValues[idx] = getPixelMeanValue(measurePixels[idx]);
  }

  return pixelRMSValues;
}

void DsscLadderParameterTrimming::onPixelData(const util::Hash& data,
  const xms::InputChannel::MetaData& meta)
{
  //cout << "Got Pixel Data: " << m_lastTrainId << endl;

  if (m_recvMode != PIXEL) return;

  m_lastTrainId = data.get<unsigned long long>("trainId");
  if (m_lastTrainId <= m_lastPptTrainId) {
    cout << "Got invalid Train Data: " << m_lastTrainId << " wait for train " << m_lastPptTrainId << endl;
    //m_runFastAcquisition = true;
    return;
  }

  try {
    set<unsigned long long>("currentTrainId", m_lastTrainId);
  } catch (...) {
    // set next time
  }

  const auto asicData = data.get<util::NDArray>("asicData");
  const auto *data_ptr = (uint16_t*)asicData.getData<unsigned short>();
  const auto numFrames = asicData.size() / 64 / 64 / 16;
  if(numFrames != utils::s_numSram){
    KARABO_LOG_ERROR << "Wrong number of data received:" << numFrames;
  }

  // TODO:
  // inefficient copy operation: maybe asicData Pointer can be stored somewhere
  // cout << "+++++++++DataSize = " << data_size << " frames = " << data_size / 64 / 64 << endl;

  // format is irrelevant since data is copied 1:1 and always 16x4096x800;
#pragma omp parallel for
  for(uint frame=0; frame<numFrames;frame++){
    const auto * inFrameData  =  data_ptr + frame * utils::s_totalNumPxs;
    auto * outFrameData       =  m_pixelData.data() + frame * utils::s_totalNumPxs;
    for(uint pixel=0; pixel<utils::s_totalNumPxs; pixel++){
      outFrameData[pixel] = inFrameData[pixel];
    }
  }

  m_asicChannelDataValid = true;
}

void DsscLadderParameterTrimming::onMeanData(const util::Hash& data,
  const xms::InputChannel::MetaData& meta)
{
  cout << "Got Mean Data: " << m_lastTrainId << endl;

  if (m_recvMode != MEAN && m_recvMode != RMS) return;

  auto trainIds = data.get<vector<unsigned long long>>("trainIds");
  if (trainIds.size() == 0) {
    cout << "No train id contained: " << m_lastTrainId << endl;
    return;
  }

  m_lastTrainId = trainIds[0];
  if (m_lastTrainId <= m_lastPptTrainId) {
    cout << "Got invalid Train Data: " << m_lastTrainId << " wait for train " << m_lastPptTrainId << endl;
    return;
  }

  set<unsigned long long>("currentTrainId", trainIds.back());

  m_asicMeanValues = data.get<vector<double>>("asicMeanData");

  m_asicChannelDataValid = true;

  cout << "Mean Data Received" << endl;
}

// test function triggered from GUI

void DsscLadderParameterTrimming::measureBurstData()
{
  StateChangeKeeper keeper(this);

  static const auto measurePixels = utils::positionListToVector("0-32767");

  const auto binValues = measureBurstData(measurePixels, m_trimStartAddr, m_trimEndAddr, baselineValuesValid);

  for (int idx = 0; idx < 10; idx++) {
    cout << idx << " : " << binValues[idx] << endl;
  }

  DSSCSTATUS("Measure Burst Data done: " + to_string(m_iterations) + " acquired!");
}


// measurement function triggered from GUI
void DsscLadderParameterTrimming::measureMeanSramContent()
{
  StateChangeKeeper keeper(this);

  uint32_t pixelToMeasure = get<unsigned int>("displayPixel");

  if (!isPixelSending(pixelToMeasure)) {
    KARABO_LOG_WARN << "Can not measure not sending pixel";
    auto sendingPixels = getSendingPixels();
    if (!sendingPixels.empty()) {
      set<unsigned int>("displayPixel", sendingPixels.front());
    }
    return;
  }

  KARABO_LOG_INFO << "Measure Mean SRAM Content in pixel " << pixelToMeasure << " from " << m_trimStartAddr << " to " << m_trimEndAddr;

  m_runFastAcquisition = true;

  SuS::CHIPInterface::measureMeanSramContent(pixelToMeasure, m_trimStartAddr, m_trimEndAddr, false);

  m_runFastAcquisition = false;

  std::vector<double> pixelSramData;
  size_t pxOffs = sramSize*pixelToMeasure;
  pixelSramData.assign(meanSramContent.begin() + pxOffs + m_trimStartAddr, meanSramContent.begin() + pxOffs + m_trimEndAddr + 1);

  util::Hash dataHash;
  dataHash.set("pixelData", pixelSramData);

  writeChannel("pixelValuesOutput", dataHash);

  DSSCSTATUS("Mean SRAM Measurement Done");
}


// measurement function triggered from GUI
void DsscLadderParameterTrimming::measureMeanSramContentAllPix()
{
  StateChangeKeeper keeper(this);

  KARABO_LOG_INFO << "Measure Mean SRAM Content for whole Ladder " << " from " << m_trimStartAddr << " to " << m_trimEndAddr;

  m_runFastAcquisition = true;

  SuS::CHIPInterface::measureMeanSramContent(utils::getUpCountingVector(utils::s_totalNumPxs), m_trimStartAddr, m_trimEndAddr, false);

  m_runFastAcquisition = false;

  const string outputDir = get<string>("outputDir");
  const string fileName =  outputDir + "/" + utils::getLocalTimeStr() + "_LadderSramCorrectionMap.h5";
  DsscHDF5Writer::saveBaselineAndSramCorrection(fileName,meanSramAccs,getSendingAsicsVec(),m_iterations);

  DSSCSTATUS("Mean SRAM Measurement Done");
}


void DsscLadderParameterTrimming::generateSramBlacklist()
{
  measureMeanSramContentAllPix();

  utils::DsscSramBlacklist sramBlacklist(meanSramContent);

  const string outputDir = get<string>("outputDir");
  const string fileName =  outputDir + "/" + utils::getLocalTimeStr() + "_PxSramOutliers.txt";
  sramBlacklist.saveToFile(fileName);

  if(isDeviceExisting(m_dsscDataReceiverId)){
    remote().set<string>(m_dsscDataReceiverId,"sramBlacklistFileName",fileName);
  }

  if(isDeviceExisting(m_mainProcessorId)){
    remote().set<string>(m_mainProcessorId,"sramBlacklistFileName",fileName);
  }

  KARABO_LOG_INFO << "Saved SramBlacklist to " << fileName;
}


// measure function triggered from GUI
void DsscLadderParameterTrimming::setBaseline()
{
  static const auto measurePixels = utils::positionListToVector("0-65535");

  SuS::CHIPInterface::setBaseline(measurePixels, m_trimStartAddr, m_trimEndAddr);

  updateBaselineValid();

  DSSCSTATUS("Baseline with " + to_string(m_iterations) + " acquired!");
}


void DsscLadderParameterTrimming::clearBaseline()
{
  SuS::CHIPInterface::clearBaseLine();

  updateBaselineValid();

  DSSCSTATUS("Baseline cleared");
}

void DsscLadderParameterTrimming::updateBaselineValid()
{
  set<bool>("baselineAvailable",baselineValuesValid);
  set<bool>("subtractBaseline",baselineValuesValid);
}

int DsscLadderParameterTrimming::initSystem()
{
  //triggerPptInitFunction(State::OFF, "initSystem", 1200);

  //triggerPptInitFunction(State::ON, "runStandAlone", 3, 3);

  //triggerPptInitFunction(State::STARTED, "startAcquisition", 3, 3);

  KARABO_LOG_INFO << "Send Configuration to PPT";

  updateAllCounters();

  initChip();

  return 0;
}


bool DsscLadderParameterTrimming::updateAllCounters()
{
  KARABO_LOG_INFO << "update all counters";

  sendBurstParams();


  remote().execute(m_pptDeviceId, "updateSequenceCounters", 300);
  waitJTAGEngineDone();

  DSSCSTATUS("Update Counters done!");

  return true;
}


void DsscLadderParameterTrimming::updateStartWaitOffset(int value)
{
  if (isPPTDeviceAvailable()) {
    remote().set<string>(m_pptDeviceId, "burstParameterName", "start_wait_offs");
    remote().set<int>(m_pptDeviceId, "burstParameterValue", value);
    remote().execute(m_pptDeviceId, "setBurstParameter");
    remote().execute(m_pptDeviceId, "updateStartWaitOffset");
  } else {
    KARABO_LOG_ERROR << "PPT DEvice is not available";
  }
}


void DsscLadderParameterTrimming::runPixelDelayTrimming()
{
  StateChangeKeeper keeper(this);

  m_recvMode = RecvMode::RMS;

  if (!allDevicesAvailable()) {
    KARABO_LOG_ERROR << "Required Karabo Infrastructure not complete - will not run Pixel Delay Trimming";
    return;
  }
  initTrimming();

  calibratePixelDelay();

  const std::string fileName = get<string>("outputDir") + "/" +utils::getLocalTimeStr() +"_DelayTrimmedConfig.conf";
  storeFullConfigFile(fileName,true);

  DSSCSTATUS("Pixel Delays Trimming done");
}


void DsscLadderParameterTrimming::displayInjectionSweep(const std::vector<std::vector<double>> & binValues, const std::vector<unsigned int> & xValues, const std::vector<uint32_t> & measurePixels)
{
  uint32_t selPixel;
  if(get<bool>("meanBurstData.userSelPixel")){
    selPixel = get<unsigned int>("displayPixel");
  }else{
    selPixel = measurePixels.back();
  }
  const std::vector<double> & yValues = binValues[selPixel];
  set<vector<unsigned int>>("meanBurstData.burstDataXValues",xValues);
  set<vector<double>>("meanBurstData.burstDataYValues",yValues);
  set<unsigned int>("meanBurstData.displayPixel", selPixel);
}


// from CHIPInterface updated during Binning Measurement
// dataHistoVec contains only measurePixelsSize entries
void DsscLadderParameterTrimming::displayDataHistos(const utils::DataHistoVec & dataHistoVec, const std::vector<uint32_t> & measurePixels)
{
  KARABO_LOG_INFO << "DISPLAY DATAHISTOS---------------------";

  if(dataHistoVec.empty()) return;

  uint32_t selPixel = measurePixels.back();
  uint32_t pixelIdx = dataHistoVec.size()-1;

  if(get<bool>("ladderHisto.userSelPixel")){
    uint32_t userSelPixel = get<unsigned int>("displayPixel");
    const auto it = std::find(measurePixels.begin(),measurePixels.end(),selPixel);
    if(it != measurePixels.end()){
      pixelIdx = it - measurePixels.begin();
      selPixel = userSelPixel;
    }
  }

  const auto & pixelHisto = dataHistoVec[pixelIdx];

  vector<unsigned short> bins;
  vector<unsigned int> binValues;

  pixelHisto.getDrawValues(bins, binValues);

  if(get<bool>("ladderHisto.enLogScale")){
    std::transform(binValues.begin(),binValues.end(),binValues.begin(),[](int x){if(x==0) return 1; return x;});
  }

  set<double>("ladderHisto.xScaleFactor", 1.0);
  set<vector<unsigned short>>("ladderHisto.bins", std::move(bins));
  set<vector<unsigned int>>("ladderHisto.binValues", std::move(binValues));
  set<unsigned int>("ladderHisto.displayPixel", selPixel);
}


void DsscLadderParameterTrimming::preReconfigure(karabo::util::Hash& incomingReconfiguration)
{
  preReconfigureGeneral(incomingReconfiguration);

  preReconfigureMeanReceiver(incomingReconfiguration);

  preReconfigureTrimming(incomingReconfiguration);

  preReconfigureHelper(incomingReconfiguration);
}


void DsscLadderParameterTrimming::preReconfigureGeneral(karabo::util::Hash & incomingReconfiguration)
{
  Hash filtered = this->filterByTags(incomingReconfiguration, "general");
  vector<string> paths;
  filtered.getPaths(paths);

  if (!paths.empty()) {

    BOOST_FOREACH(string path, paths)
    {
      if (path.compare("sendingASICs") == 0) {
        auto selAsicsStr = filtered.getAs<string>(path);
        if (selAsicsStr.length() != 17) {
          KARABO_LOG_ERROR << "Can not convert sendingASICs string, wrong number of entries";
          selAsicsStr = utils::bitEnableValueToString(getSendingAsics());
          set<string>("sendingASICs", selAsicsStr);
        }
        setSendingAsics(utils::bitEnableStringToValue(selAsicsStr));
      } else if (path.compare("activeModule") == 0) {
        int newActiveModule = filtered.getAs<int>(path);
        updateActiveModule(newActiveModule);
      } else if (path.compare("fullConfigFileName") == 0) {
        string fullConfigFileName = filtered.getAs<string>(path);
        if (utils::checkFileExists(fullConfigFileName)) {
          bool isCalibData = get<bool>("isCalibrationDataConfig");
          bool program = isDeviceExisting(m_pptDeviceId) && !isCalibData;
          loadFullConfig(fullConfigFileName, program);
          loadCoarseGainParamsIntoGui();
          if(isCalibData){
            m_calibGenerator.setPixelRegisters(pixelRegisters);
            set<bool>("pixelCalibrationDataSettingsValid",m_calibGenerator.isCalibrationDataConfigLoaded());
          }
        } else {
          KARABO_LOG_ERROR << "Can not open full config File: " << fullConfigFileName;
          set<string>("fullConfigFileName", pptFullConfig->getFullConfigFileName());
        }
      }
    }
  }
}


void DsscLadderParameterTrimming::updateActiveModule(int newActiveModule)
{
  if (newActiveModule < 1 || newActiveModule > 4) {
    //revert changes
    set<int>("activeModule", currentModule);
  } else {
    setActiveModule(newActiveModule);
  }
}


void DsscLadderParameterTrimming::loadCoarseGainParamsIntoGui()
{
  set<unsigned int>("gain.fcfEnCap",getPixelRegisterValue("0","FCF_EnCap"));
  set<unsigned int>("gain.csaFbCap",getPixelRegisterValue("0","CSA_FbCap"));
  set<unsigned int>("gain.csaResistor",getPixelRegisterValue("0","CSA_Resistor"));
  set<unsigned int>("gain.csaInjCap",getPixelRegisterValue("0","QInjEn10fF"));
  set<bool>("gain.csaInjCap200",getPixelRegisterValue("0","CSA_Cin_200fF")!= 0);
  set<unsigned int>("gain.integrationTime",sequencer->getIntegrationTime(true));

  string value;
  if(getPixelRegisters()->signalIsVarious("Control register","RmpFineTrm","all")){
    value = "Various";
  }else{
    value = to_string(getPixelRegisterValue("0","RmpFineTrm"));
  }
  set<string>("gain.irampFineTrm",value);

  if(getPixelRegisters()->signalIsVarious("Control register","RmpDelayCntrl","all")){
    value = "Various";
  }else{
    value = to_string(getPixelRegisterValue("0","RmpDelayCntrl"));
  }
  set<string>("gain.pixelDelay",value);
}


void DsscLadderParameterTrimming::preReconfigureMeanReceiver(karabo::util::Hash & incomingReconfiguration)
{
  Hash filtered = this->filterByTags(incomingReconfiguration, "meanReceiver");
  vector<string> paths;
  filtered.getPaths(paths);

  if (!paths.empty()) {

    BOOST_FOREACH(string path, paths)
    {
      if (path.compare("minSram") == 0) {
        m_trimStartAddr = filtered.getAs<unsigned short>(path);
        if (isDeviceExisting(m_mainProcessorId)) {
          remote().set<unsigned short>(m_mainProcessorId, "minSram", m_trimStartAddr);
        }
      } else if (path.compare("maxSram") == 0) {
        m_trimEndAddr = filtered.getAs<unsigned short>(path);
        if (isDeviceExisting(m_mainProcessorId)) {
          remote().set<unsigned short>(m_mainProcessorId, "maxSram", m_trimEndAddr);
        }
      } else if (path.compare("numIterations") == 0) {
        const uint iterations = filtered.getAs<unsigned int>(path);
        setNumIterations(iterations);
      }
    }
  }
}


void DsscLadderParameterTrimming::preReconfigureTrimming(karabo::util::Hash & incomingReconfiguration)
{
  Hash filtered = this->filterByTags(incomingReconfiguration, "trimming");
  vector<string> paths;
  filtered.getPaths(paths);

  if (!paths.empty()) {

    BOOST_FOREACH(string path, paths)
    {
      if (path.compare("injectionMode") == 0) {
        KARABO_LOG_INFO << "Press SetInjectionMode to change mode";
      } else if (path.compare("asicsToChange") == 0) {
        auto asics = filtered.getAs<string>(path);
        auto asicPixels = get<string>("asicsPixelsToChange");
        set<string>("pixelsToChange", utils::positionVectorToList(getASICPixels(asics, asicPixels)));
      } else if (path.compare("asicsPixelsToChange") == 0) {
        auto asics = get<string>("asicsToChange");
        auto asicPixels = filtered.getAs<string>(path);
        set<string>("pixelsToChange", utils::positionVectorToList(getASICPixels(asics, asicPixels)));
      } else if (path.compare("numFramesToReceive") == 0) {
        auto numFrames = filtered.getAs<unsigned int>(path);
        setNumFramesToSend(numFrames);
      } else if (path.compare("numRuns") == 0) {
        m_numRuns = filtered.getAs<unsigned int>(path);
      }
    }
  }
}


void DsscLadderParameterTrimming::preReconfigureHelper(karabo::util::Hash & incomingReconfiguration)
{
  Hash filtered = this->filterByTags(incomingReconfiguration, "helper");
  vector<string> paths;
  filtered.getPaths(paths);

  if (!paths.empty()) {

    BOOST_FOREACH(string path, paths)
    {
      if (path.compare("testAsicPixel") == 0) {
        auto asicPixel = filtered.getAs<unsigned int>(path);
        auto asicNumber = get<unsigned int>("testAsicNumber");

        set<unsigned int>("testImagePixel", utils::calcImagePixel(asicPixel, asicNumber));
      } else if (path.compare("testAsicNumber") == 0) {
        auto asicPixel = get<unsigned int>("testAsicPixel");
        auto asicNumber = filtered.getAs<unsigned int>(path);

        set<unsigned int>("testImagePixel", utils::calcImagePixel(asicPixel, asicNumber));
      } else if (path.compare("testImagePixel") == 0) {
        auto imagePixel = filtered.getAs<unsigned int>(path);

        unsigned int asicNumber = utils::getPixelASIC(imagePixel);
        unsigned int asicPixel = utils::calcASICPixel(imagePixel);

        set<unsigned int>("testAsicPixel", asicPixel);
        set<unsigned int>("testAsicNumber", asicNumber);
      }
    }
  }
}


void DsscLadderParameterTrimming::postReconfigure()
{
}


void DsscLadderParameterTrimming::setNumIterations(uint iterations)
{
  m_iterations = iterations;
  if (isDeviceExisting(m_mainProcessorId)) {
    remote().set<unsigned short>(m_mainProcessorId, "numIterations", m_iterations);
  }
}

bool DsscLadderParameterTrimming::isHardwareReady()
{
  auto processorState = dsscPptState();

  cout << m_pptDeviceId << " State is " << processorState.name() << endl;

  if (processorState == State::OFF || processorState == State::UNKNOWN) {
    return false;
  }

  return true;
}


void DsscLadderParameterTrimming::initTrimming()
{
  DSSCSTATUS("Trimming running...");

  setSelectedInjectionMode();

  setActiveModule(get<int>("activeModule"));

  m_startTrimming = true;
}


bool DsscLadderParameterTrimming::allDevicesAvailable()
{
  if(!isPPTDeviceAvailable()) return false;

  if(!checkPPTInputConnected()) return false;
  if(!allReceiverDevicesAvailable()) return false;

  return true;
}


bool DsscLadderParameterTrimming::allReceiverDevicesAvailable()
{
  enablePixelValueAcquisition();

  if (isTestData()) {
    KARABO_LOG_INFO << "Initialize TestDataGenerator";
    if (!isDeviceExisting(m_testDataGeneratorId)) {
      if (!startTestDataGeneratorInstance()) {
        return false;
      }
    }
  }

  if (isDsscData()) {
    KARABO_LOG_INFO << "Initialize DsscDataReceiver";
    if (!isDeviceExisting(m_dsscDataReceiverId)) {
      if (!startDsscDataGeneratorInstance()) {
        return false;
      }
    }
  }

  if (!isDeviceExisting(m_mainProcessorId)) {
    if (!startMainProcessorInstance()) {
      return false;
    } else {
      remote().set<string>(m_mainProcessorId, "sendingASICs", get<string>("sendingASICs"));
    }
  }

  //connectAsicDataInputChannel();

  if (isTestData()) {
    KARABO_LOG_INFO << "Start Dummy Data";
    if (deviceState(m_testDataGeneratorId) != State::ON) {
      remote().executeNoWait(m_testDataGeneratorId, "start_cont_mode");
    }
  }

  if (isDsscData()) {
    KARABO_LOG_INFO << "Start Dssc DataReceiver";
    if (deviceState(m_dsscDataReceiverId) != State::ACQUIRING) {
      remote().executeNoWait(m_dsscDataReceiverId, "start");
    }
  }

  KARABO_LOG_INFO << "DataHandler Devices successfully started!!";
  return true;
}


bool DsscLadderParameterTrimming::isPPTDeviceAvailable()
{
  if (!isDeviceExisting(m_pptDeviceId)) {
    if (!startDsscPptInstance()) {
      return false;
    }
  }

  startPptDevice();

  auto pptState = dsscPptState();

  cout << m_pptDeviceId << " State is " << pptState.name() << endl;

  if (pptState != State::ACQUIRING) {
    KARABO_LOG_ERROR << "PPT Data Acquisition could not be started";
    return false;
  }

  return true;
}


util::State DsscLadderParameterTrimming::dsscPptState()
{
  return deviceState(m_pptDeviceId);
}


util::State DsscLadderParameterTrimming::deviceState(const string & deviceId)
{
  try{
    return remote().get<util::State>(deviceId, "state");
  }
  catch (...)
  {
    //KARABO_LOG_WARN << "Get Device State Exception, state does not exist: " << deviceId;
    return util::State::CHANGING;
  }
}


bool DsscLadderParameterTrimming::startDsscPptInstance()
{
  //auto initialConfig = remote().getClassSchema(m_quadrantServerId,"DsscPpt").getParameterHash();

  const auto env = get<string>("environment");

  util::Hash initialConfig;
  if (env == "MANNHEIM") {
    initialConfig.set<string>("selEnvironment", "MANNHEIM");
    initialConfig.set<string>("pptHost", "192.168.0.120");
    initialConfig.set<unsigned int>("numActiveASICs", 1);
  } else if (env == "FENICE") {
    initialConfig.set<string>("selEnvironment", "HAMBURG");
    initialConfig.set<string>("pptHost", "192.168.0.125");
    initialConfig.set<unsigned int>("numActiveASICs", 16);
  }


  initialConfig.set<unsigned int>("pptPort", 2384);

  initialConfig.set<string>("sendingASICs", get<string>("sendingASICs"));
  initialConfig.set<string>("deviceId", m_pptDeviceId);

  vector<string> outputChannels{ (get<string>("deviceId") + "@registerConfigOutput")};

  util::Hash inputConfig = createInputChannelConfig(outputChannels, "wait");

  initialConfig.set("registerConfigInput", inputConfig);

  auto pair = remote().instantiate(m_quadrantServerId, "DsscPpt", initialConfig);

  bool ok = pair.first;

  if (!ok) {
    KARABO_LOG_ERROR << "DsscPpt initialisation failed: " << pair.second;
    return false;
  }

  KARABO_LOG_INFO << "DsscPpt initialized successfully: " << pair.second;
  return true;
}


bool DsscLadderParameterTrimming::startTestDataGeneratorInstance()
{
  util::Hash initialConfig;
  initialConfig.set<unsigned int>("send_pause", 95);
  initialConfig.set<string>("deviceId", m_testDataGeneratorId);

  auto pair = remote().instantiate(m_recvServerId, "DsscDummyTrainGenerator", initialConfig);

  bool ok = pair.first;
  if (!ok) {
    KARABO_LOG_ERROR << "DsscDummyTrainGenerator initialisation failed: " << pair.second;
    return false;
  }

  KARABO_LOG_INFO << "DsscDummyTrainGenerator initialized successfully: " << pair.second;
  return true;
}


bool DsscLadderParameterTrimming::startDsscDataGeneratorInstance()
{
  util::Hash initialConfig;
  initialConfig.set<string>("asicsToRecord", get<string>("sendingASICs"));
  initialConfig.set<string>("deviceId", m_dsscDataReceiverId);
  initialConfig.set<unsigned int>("testPattern", getExpectedTestPattern());
  initialConfig.set<string>("quadrantId", get<string>("quadrantId"));
  initialConfig.set<bool>("enableDAQOutput", false);
  initialConfig.set<unsigned int>("udpPort", get<unsigned int>("recvDeviceUDPPort"));

  auto pair = remote().instantiate(m_recvServerId, "DsscDataReceiver", initialConfig);

  bool ok = pair.first;
  if (!ok) {
    KARABO_LOG_ERROR << "DsscDataReceiver initialisation failed: " << pair.second;
    return false;
  }

  KARABO_LOG_INFO << "DsscDataReceiver initialized successfully: " << pair.second;
  return true;
}


bool DsscLadderParameterTrimming::startMainProcessorInstance()
{
  util::Hash initialConfig;
  initialConfig.set<string>("deviceId", m_mainProcessorId);
  initialConfig.set<string>("sendingASICs", get<string>("sendingASICs"));

  if (isTestData()) {
    vector<string> outputChannels{ (m_testDataGeneratorId + "@dummyTrainOutput")};

    util::Hash inputConfig = createInputChannelConfig(outputChannels);

    initialConfig.set("input", inputConfig);
  } else if (isDsscData()) {
    vector<string> outputChannels{ (m_dsscDataReceiverId + "@monitorOutput")};

    util::Hash inputConfig = createInputChannelConfig(outputChannels);

    initialConfig.set("input", inputConfig);
  } else if (isXFELData()) {
    //connect a special pcLayer output by name
      vector<string> outputChannels{("DETLAB_DSSC_DAQ_DATA_DET_" + to_string(get<int>("activeModule")) + "CH" + m_quadrantId + "@monitorOutput")};
      util::Hash inputConfig = createInputChannelConfig(outputChannels);
      initialConfig.set("input", inputConfig);      
  }

  auto pair = remote().instantiate(m_recvServerId, "DsscProcessor", initialConfig);
  bool ok = pair.first;

  if (!ok) {
    KARABO_LOG_ERROR << "DsscProcessor initialisation failed: " << pair.second;
    return false;
  }

  KARABO_LOG_INFO << "DsscProcessor initialized successfully: " << pair.second;
  return true;
}


void DsscLadderParameterTrimming::triggerPptInitFunction(util::State originState, const string & function, int timeout, int TRY_CNT)
{
  int cnt = 0;
  while ((dsscPptState() == originState) || (dsscPptState() == util::State::CHANGING)) {
    remote().execute(m_pptDeviceId, function, timeout);

    if (++cnt == TRY_CNT) {
      return;
    }
  }
}


void DsscLadderParameterTrimming::startPptDevice()
{
  triggerPptInitFunction(State::ERROR, "open", 10, 1);

  triggerPptInitFunction(State::UNKNOWN, "open", 10, 1);

  triggerPptInitFunction(State::OFF, "initSystem", 1200);

  triggerPptInitFunction(State::ON, "runStandAlone", 3, 3);

  triggerPptInitFunction(State::STARTED, "startAcquisition", 3, 3);
}


util::Hash DsscLadderParameterTrimming::getModuleSetHash(SuS::ConfigReg * reg, const std::string & moduleSetName)
{
  util::Hash moduleSetHash;

  const auto signalNames = reg->getSignalNames(moduleSetName);
  const auto moduleNumbers = reg->getModuleNumberList(moduleSetName);
  const auto numModules = reg->getNumModules(moduleSetName);

  moduleSetHash.set<string>("moduleNumbers", moduleNumbers);
  moduleSetHash.set<int>("numModules", numModules);

  string sentSignals;
  for (auto && signalName : signalNames) {
    if (reg->isSignalReadOnly(moduleSetName, signalName)) continue;

    if (reg->signalIsVarious(moduleSetName, signalName, "all")) {

      const auto signalValues = (std::vector<unsigned int>)reg->getSignalValues(moduleSetName, "all", signalName);

      util::NDArray arr(signalValues.data(), signalValues.size(), util::Dims(signalValues.size()));
      moduleSetHash.set<util::NDArray>(signalName, arr);
    } else {
      unsigned int value = reg->getSignalValue(moduleSetName, "all", signalName);
      std::vector<unsigned int> signalValues(1, value);
      util::NDArray arr(signalValues.data(), signalValues.size(), util::Dims(signalValues.size()));
      moduleSetHash.set<util::NDArray>(signalName, arr);
    }

    sentSignals += signalName + ";";
  }
  sentSignals.pop_back();

  moduleSetHash.set<string>("signalNames", sentSignals);
  return moduleSetHash;
}


util::Hash DsscLadderParameterTrimming::getSequencerHash(SuS::Sequencer * seq)
{
  util::Hash sequencerHash;

  const auto paramMap = seq->getSequencerParameterMap();
  for (auto && entry : paramMap) {
    sequencerHash.set<unsigned int>(entry.first, entry.second);
  }

  sequencerHash.set<unsigned int>("opMode", (unsigned int) seq->mode);
  return sequencerHash;
}


bool DsscLadderParameterTrimming::getContentFromDevice(uint32_t bitStreamLength, std::vector<bool> &data_vec)
{
  KARABO_LOG_ERROR << "The readback function for configuration registers is currently not implemented in the trimming device";
  return true;
}


bool DsscLadderParameterTrimming::programJtag(bool readBack, bool setJtagEngineBusy, bool recalcXors)
{
  ConfigStateKeeper keeper(m_deviceConfigState);

  util::Hash outData;

  outData.set<string>("regType", "jtag");
  outData.set<string>("progType", "new");
  outData.set<int>("currentModule", currentModule);

  const auto moduleSetNames = jtagRegisters->getModuleSetNames();

  string sentModuleSets;
  for (auto && moduleSet : moduleSetNames) {
    if (jtagRegisters->isModuleSetReadOnly(moduleSet)) continue;

    auto moduleSetHash = getModuleSetHash(jtagRegisters, moduleSet);
    outData.set(moduleSet, moduleSetHash);
    sentModuleSets += moduleSet + ";";
  }

  sentModuleSets.pop_back();

  outData.set<string>("moduleSets", sentModuleSets);


  writeChannel("registerConfigOutput", outData);
  waitJTAGEngineDone();

  return true;
}


bool DsscLadderParameterTrimming::programJtagSingle(const std::string & moduleSetName, bool readBack, bool setJtagEngineBusy, bool recalcXors, bool overwrite)
{
  ConfigStateKeeper keeper(m_deviceConfigState);

  util::Hash outData;
  outData.set<string>("regType", "jtag");
  outData.set<string>("progType", "new");
  outData.set<int>("currentModule", currentModule);
  outData.set<string>("moduleSets", moduleSetName);

  auto moduleSetHash = getModuleSetHash(jtagRegisters, moduleSetName);
  outData.set(moduleSetName, moduleSetHash);


  writeChannel("registerConfigOutput", outData);
  waitJTAGEngineDone();

  return true;
}


bool DsscLadderParameterTrimming::programPixelRegs(bool readBack, bool setJtagEngineBusy)
{
  ConfigStateKeeper keeper(m_deviceConfigState);

  KARABO_LOG_INFO << "Send Pixel Register Content";

  util::Hash outData;

  const string moduleSetName = "Control register";

  outData.set<string>("regType", "pixel");
  outData.set<string>("progType", (m_startTrimming ? "new" : "update")); // startTrimming has no function yet
  outData.set<int>("currentModule", currentModule);
  outData.set<string>("moduleSets", "Control register");

  auto moduleSetHash = getModuleSetHash(pixelRegisters, moduleSetName);
  outData.set(moduleSetName, moduleSetHash);


  writeChannel("registerConfigOutput", outData);
  waitJTAGEngineDone();

  m_startTrimming = false;

  return true;
}

void DsscLadderParameterTrimming::programPixelRegDirectly(int px, bool setJtagEngineBusy)
{
  ConfigStateKeeper keeper(m_deviceConfigState);

  KARABO_LOG_ERROR << "This function should not be called from the Karabo framework";
}

bool DsscLadderParameterTrimming::programSequencer(bool readBack, bool setJtagEngineBusy, bool program)
{
  ConfigStateKeeper keeper(m_deviceConfigState);

  util::Hash outData;
  outData.set<string>("regType", "sequencer");
  outData.set<string>("progType", "new");

  auto sequencerHash = getSequencerHash(getSequencer());
  outData.set("sequencerParams", sequencerHash);


  writeChannel("registerConfigOutput", outData);
  waitJTAGEngineDone();

  if (false/*program*/) {
    programJtag();
  }

  return true;
}

void DsscLadderParameterTrimming::setSendingAsics(uint16_t asics)
{
  SuS::MultiModuleInterface::setSendingAsics(asics);

  if (isDeviceExisting(m_pptDeviceId)) {
    remote().set<string>(m_pptDeviceId, "sendingASICs", utils::bitEnableValueToString(asics));
    remote().execute(m_pptDeviceId, "setSendingASICs", 20);
  }
}

void DsscLadderParameterTrimming::setActiveModule(int modNumber)
{
  SuS::MultiModuleInterface::setActiveModule(modNumber);

  updateSendingAsics(modNumber);

  if (isDeviceExisting(m_pptDeviceId)) {
    programPixelRegs();
    programJtag();
  }
}

void DsscLadderParameterTrimming::updateSendingAsics(int moduleNr)
{
  std::string quadrantId = get<string>("quadrantId");
  uint16_t value = utils::DsscModuleInfo::getSendingAsics(quadrantId,moduleNr);

  set<string>("sendingASICs",utils::bitEnableValueToString(value));

  setSendingAsics(value);
}


void DsscLadderParameterTrimming::sendBurstParams()
{
  ConfigStateKeeper keeper(m_deviceConfigState);

  util::Hash outData;
  outData.set<string>("regType", "burstParams");
  util::Hash burstParamData;
  auto paramNames = getBurstParamNames();
  for (auto && name : paramNames) {
    burstParamData.set<int>(name, getBurstParam(name));
  }
  outData.set("paramValues", burstParamData);


  writeChannel("registerConfigOutput", outData);
  waitJTAGEngineDone();
}


void DsscLadderParameterTrimming::waitJTAGEngineDone()
{
  signalEndOfStream("registerConfigOutput");

  try{
    remote().execute(m_pptDeviceId,"waitJTAGEngineDone",30);
  }catch(...){
    KARABO_LOG_ERROR << "EXCEPETION: COULD NOT WAIT FOR JTAG ENGINE DONE";
  }
}


bool DsscLadderParameterTrimming::checkPPTInputConnected()
{
  if (!isPPTDeviceAvailable()) return false;

  auto inputConnection = remote().get<vector < string >> (m_pptDeviceId, "registerConfigInput.connectedOutputChannels");

  auto elem = find(inputConnection.begin(), inputConnection.end(), get<string>("deviceId") + "@registerConfigOutput");
  return elem != inputConnection.end();
}

int DsscLadderParameterTrimming::getNumberOfActiveAsics() const
{
  return utils::countOnesInInt(activeAsics);
}

int DsscLadderParameterTrimming::getNumberOfSendingAsics() const
{
  return utils::countOnesInInt(sendingAsics);
}

int DsscLadderParameterTrimming::getActiveASICToReadout() const
{
  return utils::getFirstOneInValue(activeAsics);
}

bool DsscLadderParameterTrimming::fastInitChip()
{
  KARABO_LOG_ERROR << "This function should not be called from the Karabo framework";
  return true;
}

void DsscLadderParameterTrimming::initChip()
{
  programJtag();
  programSequencer();
  programPixelRegs();
}

void DsscLadderParameterTrimming::setBurstVetoOffset(int val)
{
  remote().set<unsigned int>(m_pptDeviceId, "numPreBurstVetos", val);
}

int DsscLadderParameterTrimming::getBurstVetoOffset()
{
  return remote().get<unsigned int>(m_pptDeviceId, "numPreBurstVetos");
}

void DsscLadderParameterTrimming::setNumFramesToSend(int val, bool saveOldVal)
{
  if (isDeviceExisting(m_pptDeviceId)) {
    // stop device running
    runContinuousMode(false);

    remote().set<unsigned int>(m_pptDeviceId, "numFramesToSendOut", val);

    runContinuousMode(true);
    KARABO_LOG_INFO << "Change NumFramesToReceive of PPTDevice to" << val;
  }


  if (isDeviceExisting(m_dsscDataReceiverId)) {
    remote().execute(m_dsscDataReceiverId, "stop");
    remote().execute(m_dsscDataReceiverId, "close");
    remote().set<unsigned int>(m_dsscDataReceiverId, "numFramesToReceive", val);
    remote().execute(m_dsscDataReceiverId, "start");
  }

  auto maxSram = get<unsigned short>("maxSram");
  if (maxSram >= val) {
    setMaxSram(val - 1);
  }
  auto minSram = get<unsigned short>("minSram");
  if (minSram > maxSram) {
    setMinSram(0);
  }
}

void DsscLadderParameterTrimming::setMaxSram(unsigned int value)
{
  m_trimEndAddr = value;
  set<unsigned short>("maxSram", m_trimEndAddr);
}

void DsscLadderParameterTrimming::setMinSram(unsigned int value)
{
  m_trimStartAddr = value;
  set<unsigned short>("minSram", m_trimStartAddr);
}

void DsscLadderParameterTrimming::setNumWordsToReceive(int val, bool saveOldVal)
{
  setNumFramesToSend(val / 4096 / 16);
}

void DsscLadderParameterTrimming::runContinuousMode(bool run)
{
  if (run) {
    startPptDevice();
  } else {
    if (dsscPptState() == State::ACQUIRING) {
      remote().execute(m_pptDeviceId, "stopAcquisition");
    }
    if (dsscPptState() == State::STARTED) {
      remote().execute(m_pptDeviceId, "stopStandalone");
    }
  }
}

void DsscLadderParameterTrimming::setSendRawData(bool enable, bool reordered, bool converted)
{
  remote().set<bool>(m_pptDeviceId, "send_raw_data", enable);
}

void DsscLadderParameterTrimming::setRoSerIn(bool bit)
{
  remote().set<bool>(m_pptDeviceId, "jtag_ro_serin", bit, 5);
}

bool DsscLadderParameterTrimming::fillSramAndReadout(uint16_t pattern, bool init, bool jtagMode)
{
  if (dsscPptState() != State::ON) {
    KARABO_LOG_INFO << "Start DsscPpt and initialize before programming pattern via Jtag";
    return false;
  }

  if(init)
  {
    remote().set<unsigned short>(m_pptDeviceId, "sramPattern", 0);
    runContinuousMode(false);
    // takes a while to program the whole ladder
    remote().execute(m_pptDeviceId, "fillSramAndReadout", 600);
  }

  remote().set<unsigned short>(m_pptDeviceId, "sramPattern", pattern);

  runContinuousMode(false);

  // takes a while to program the whole ladder
  remote().execute(m_pptDeviceId, "fillSramAndReadout", 600);

  if (!waitDataReceived()) return false;

  return true;
}


void DsscLadderParameterTrimming::fillSramAndReadoutPattern()
{
  const auto pattern = get<unsigned short>("sramPattern");
  fillSramAndReadout(pattern, true);
}


void DsscLadderParameterTrimming::powerUpSelPixels()
{
  powerDownPixels("all");
  const auto pixels = getPixelsToChange();
  if (pixels.empty()) {
    return;
  }
  powerUpPixels(utils::positionVectorToList(pixels), true/*program*/);
}


void DsscLadderParameterTrimming::enableInjectionInSelPixels()
{
  enableInjection(false,"all");

  const auto selStr = get<string>("pixelsToChange");
  if(selStr.substr(0,3) == "col"){
    m_setColumns = true;
  }

  const auto pixels = getPixelsToChange();
  if (pixels.empty()) {
    return;
  }
  enableInjection(true,utils::positionVectorToList(pixels));
}


void DsscLadderParameterTrimming::setSelectedGainConfig()
{
  StateChangeKeeper keeper(this);

  string gainConfigStr = get<string>("gainSelection");

  SuS::CHIPGainConfigurator configurator(this);
  configurator.setGainMode(gainConfigStr);
  configurator.activateGainMode(gainConfigStr);

  loadCoarseGainParamsIntoGui();
}


void DsscLadderParameterTrimming::setCoarseGainSettings()
{
  setPixelRegisterValue("all","FCF_EnCap",get<unsigned int>("gain.fcfEnCap"));
  setPixelRegisterValue("all","CSA_FbCap",get<unsigned int>("gain.csaFbCap"));
  setPixelRegisterValue("all","CSA_Resistor",get<unsigned int>("gain.csaResistor"));
  setPixelRegisterValue("all","QInjEn10fF",get<unsigned int>("gain.csaInjCap"));
  setPixelRegisterValue("all","CSA_Cin_200fF",get<bool>("gain.csaInjCap200")?1:0);

  set<unsigned int>("gain.integrationTime",sequencer->getIntegrationTime());

  string value;
  if(getPixelRegisters()->signalIsVarious("Control register","RmpFineTrm","all")){
    value = "Various";
  }else{
    value = to_string(getPixelRegisterValue("0","RmpFineTrm"));
  }
  set<string>("gain.irampFineTrm",value);

  if(getPixelRegisters()->signalIsVarious("Control register","RmpDelayCntrl","all")){
    value = "Various";
  }else{
    value = to_string(getPixelRegisterValue("0","RmpDelayCntrl"));
  }
  set<string>("gain.pixelDelay",value);

  programPixelRegs();

  m_calibGenerator.setPixelRegistersChanged();
  set<bool>("pixelCalibrationDataSettingsValid",false);
}


void DsscLadderParameterTrimming::setSelectedInjectionMode()
{
  StateChangeKeeper keeper(this);

  setActiveModule(get<int>("activeModule"));

  string injectionModeName = get<string>("injectionMode");
  auto newMode = getInjectionMode(injectionModeName);
  if (injectionMode == newMode) {
    KARABO_LOG_WARN << "InjectionMode already set, nothing to do";
    return;
  }

  if (isDeviceExisting(m_pptDeviceId)) {
    remote().set<string>(m_pptDeviceId, "injectionMode", injectionModeName);
    remote().execute(m_pptDeviceId, "setInjectionMode");
  }

  setInjectionMode(newMode);
}


void DsscLadderParameterTrimming::addValueToPxRegSignal()
{
  auto value  = get<int>("valueToAdd");
  auto sigStr = get<string>("pixelsRegisterSignalName");

  const auto pixels = getPixelsToChange();
  for (auto p : pixels) {
    uint32_t val = getPixelRegisterValue(std::to_string(p), sigStr);
    val += value;
    setPixelRegisterValue(std::to_string(p), sigStr, val);
  }
  programPixelRegs();
}

void DsscLadderParameterTrimming::enableMonBusInCols()
{
  auto selCols = get<string>("monBusColsSelect");
  enableMonBusCols(selCols);
}

bool DsscLadderParameterTrimming::checkIOBDataFailed()
{
  remote().execute(m_pptDeviceId, "checkIOBDataFailed", 6);
  string channelKey = "activeChannelReadoutFailure";
  const auto failed = remote().get<unsigned short>(m_pptDeviceId, channelKey);
  return failed != 0;
}

unsigned int DsscLadderParameterTrimming::getLastValidTrainId()
{
  try {
    remote().execute(m_pptDeviceId, "readLastPPTTrainID");
    if (m_runFastAcquisition) {
      m_lastPptTrainId = 1;
    } else if (isTestData()) {
      m_lastPptTrainId = remote().get<unsigned long long>(m_testDataGeneratorId, "trainId");
    } else if (isDsscData()) {
      m_lastPptTrainId = remote().get<unsigned int>(m_pptDeviceId,"lastTrainId") + 1;
      //m_lastPptTrainId = remote().get<unsigned long long>(m_dsscDataReceiverId, "currentTrainId")+11;
    } else {
      KARABO_LOG_WARN << "PCLAYER no train id implemented, or not known yet";
    }
  } catch (...) {
    KARABO_LOG_WARN << "Could not read train id from receiver device";
  }
  return m_lastPptTrainId;
}


void DsscLadderParameterTrimming::resetDataReceiver()
{
  if(isDsscData()){
    //remote().execute(m_dsscDataReceiverId, "flushTrainStorage");
  }

  getLastValidTrainId();

  //KARABO_LOG_DEBUG << "Set Min Train ID "<< m_lastPptTrainId << " at " << m_mainProcessorId;

  remote().set<unsigned long long>(m_mainProcessorId, "minValidTrainId", m_lastPptTrainId);
  remote().set<bool>(m_mainProcessorId, "measureMean", m_recvMode == RecvMode::MEAN);
  remote().set<bool>(m_mainProcessorId, "measureRMS", m_recvMode == RecvMode::RMS);

  if (m_recvMode == RecvMode::PIXEL) {
    // nothing to to
  }

  if (m_recvMode == RecvMode::MEAN || m_recvMode == RecvMode::RMS) {
    remote().set<unsigned int>(m_mainProcessorId, "numIterations", m_iterations);
    remote().set<unsigned short>(m_mainProcessorId, "minSram", m_trimStartAddr);
    remote().set<unsigned short>(m_mainProcessorId, "maxSram", m_trimEndAddr);
    remote().execute(m_mainProcessorId, "accumulate");
  }
}

bool DsscLadderParameterTrimming::doSingleCycle(DataPacker* packer, bool testPattern)
{
  return doSingleCycle(10, packer, testPattern);
}

bool DsscLadderParameterTrimming::doSingleCycle(int numTries, DataPacker* packer, bool testPattern)
{
  const int TO = numTries;

  enablePixelValueAcquisition();

  bool allOk = true;
  int try_cnt = 0;
  do {
    allOk = waitDataReceived();
    if (!allOk) {
      if (m_recvStatus > 0) {
        checkIOBDataFailed();
      }
      cout << "Retry receiving cnt " << try_cnt << endl;
    }

    try_cnt++;
  } while (!allOk && try_cnt < TO);

  if (try_cnt == TO) {
    KARABO_LOG_ERROR << "DataReceiver Error";
    return false;
  }
  return true;
}

void DsscLadderParameterTrimming::measureBurstOffsetSweep()
{
  StateChangeKeeper keeper(this);

  const auto measurePixel = get<unsigned int>("burstWaitOffsetPixel");

  std::vector<int> paramValues;
  bool ok = utils::getSimpleSweepVector<int>(get<string>("burstWaitOffsetRange"), paramValues);
  if (!ok) {
    KARABO_LOG_ERROR << "burstWaitOffsetRange has a bad range";
    return;
  }

  auto binValues = sweepBurstWaitOffset(measurePixel, paramValues, m_trimStartAddr, m_trimEndAddr);
  // save data
  saveDataVector("BurstWaitOffset", binValues);

  // display data in PixelData vector output
  std::vector<double> pixelBurstOffsetData(paramValues.back()+1,0);

  const uint numParams = std::min(paramValues.size(),binValues.size());
  for(uint idx=0; idx<numParams; idx++){
    pixelBurstOffsetData[paramValues[idx]] = binValues[idx];
  }

  util::Hash dataHash;
  dataHash.set("pixelData", pixelBurstOffsetData);
  writeChannel("pixelValuesOutput", dataHash);

  //show max value
  auto maxElement = std::max_element(binValues.begin(), binValues.end());
  int maxIdx = maxElement - binValues.begin();
  KARABO_LOG_INFO << "Max Value = " << *maxElement << " at setting " << paramValues[maxIdx];
}


void DsscLadderParameterTrimming::measureADCGainMap()
{
  KARABO_LOG_INFO << "Measure Pixel Slopes for all adc gain settings";

  StateChangeKeeper keeper(this);

  if (get<bool>("saveTrimmingRawData")) {
    initSaveRawDataParams();
  }

  bool ok;
  auto trimmer = getNewChipTrimmer(ok);
  if (!ok) return;

  string irampSettingsRange = get<string>("irampSettingsRange");
  if (irampSettingsRange[0] == 'a') {
    irampSettingsRange = "0-63";
  }

  std::vector<uint32_t> irampSettings;
  utils::getSimpleSweepVector(irampSettingsRange, irampSettings);

  // exectue the measurement for all iramp settings and all chip parts
  const auto pixelADCGainSlopes = trimmer.generateADCGainMap(irampSettings); // numSettings x numPixels
  const size_t numSettings = irampSettings.size();

  // transpose gain map and fill to single Vector numPixels x numSettings
  std::vector<double> adcGainMap(numSettings * totalNumPxs,0.0);
  int set = 0;
  for(auto && gainSlopes : pixelADCGainSlopes){
    for(int px=0; px<totalNumPxs; px++){
      const size_t offs = px*numSettings+set;
      adcGainMap[offs] = gainSlopes[px];
    }
    set++;
  }

  const string outputDir = get<string>("outputDir");
  const string fileName =  utils::getLocalTimeStr() + "_ADCGainMap_InjectionSweepSlopesSummary.h5";
  DsscHDF5TrimmingDataWriter::savePixelADCGainValues(outputDir,fileName,adcGainMap,irampSettings);

  if (get<bool>("saveTrimmingRawData")) {
    finishSaveRawDataParams();
  }
  m_currentTrimmer = nullptr;
  KARABO_LOG_INFO << "All ADC Slopes Measured";
}


void DsscLadderParameterTrimming::measureInjectionSweepSlopes()
{
  KARABO_LOG_INFO << "Measure Pixel Slopes changing only incjection dac settings";

  StateChangeKeeper keeper(this);

  if (get<string>("injectionMode").find("NORM") != string::npos) {
    KARABO_LOG_ERROR << "No Injection mode selected";
    return;
  }

  setSelectedInjectionMode();

  bool ok;
  auto trimmer = getNewChipTrimmer(ok);
  if (!ok) return;

  m_binValues = trimmer.generateSlopeInformationForChipParts();

  showLadderImage(getFinalSlopes());

  saveDataVector("InjectionSweepSlopes", getFinalSlopes());

  m_currentTrimmer = nullptr;

  KARABO_LOG_INFO << "Pixel Slopes Measured";
}


void DsscLadderParameterTrimming::initDataWriter()
{
  std::string quadrantId = get<string>("quadrantId");
  uint moduleNr   = 1;
  uint iobSerial  = 0;
  if(isDsscData())
  {
    moduleNr = remote().get<unsigned short>(m_dsscDataReceiverId,"specificData.moduleNr");
    iobSerial = remote().get<unsigned int>(m_dsscDataReceiverId,"specificData.iobSerial");
  }

  if(!DsscHDF5Writer::checkModuleInfo(quadrantId,moduleNr,iobSerial)){
    KARABO_LOG_WARN << "HDF5 File Writer already has different Module Information, will be overridden, maybe a wrong file was loaded";
  }
  DsscHDF5Writer::updateModuleInfo(quadrantId,moduleNr,iobSerial);

  updateModuleInfo();
}


void DsscLadderParameterTrimming::updateModuleInfo()
{
  set<string>("moduleInfo.quadrantId",DsscHDF5Writer::s_writerModuleInfo.quadrantId);
  set<unsigned int>("moduleInfo.moduleNr",DsscHDF5Writer::s_writerModuleInfo.moduleNr);
  std::stringstream iss;
  iss << "0x" << hex << setw(8) << setfill('0') << DsscHDF5Writer::s_writerModuleInfo.iobSerial;
  set<string>("moduleInfo.iobSerial",iss.str());
}


void DsscLadderParameterTrimming::saveDataVector(const std::string & outputName, const std::vector<double> & dataVector)
{
  initDataWriter();

  const string outputDir = get<string>("outputDir");
  utils::makePath(outputDir);

  const string fileName = outputDir + "/" + utils::getLocalTimeStr() + "_" + outputName + ".h5";
  DsscHDF5TrimmingDataWriter dataWriter(fileName);
  dataWriter.setMeasurementName("TrimmingData");
  dataWriter.addImageData(outputName, 512, dataVector);
}


void DsscLadderParameterTrimming::saveDataHisto(const std::string & outputName, const utils::DataHisto & dataHisto, double scaleFactor)
{
  initDataWriter();

  const string outputDir = get<string>("outputDir");
  utils::makePath(outputDir);

  const string fileName = outputDir + "/" + utils::getLocalTimeStr() + "_" + outputName + ".h5";
  DsscHDF5TrimmingDataWriter dataWriter(fileName);
  dataWriter.setMeasurementName("TrimmingDataHisto");
  dataWriter.addHistoData(outputName, dataHisto, scaleFactor);
}

void DsscLadderParameterTrimming::measureBinningInformation()
{
  StateChangeKeeper keeper(this);

  changeDeviceState(State::ACQUIRING);

  bool ok;
  auto trimmer = getNewChipTrimmer(ok);
  if (!ok) return;

  trimmer.generateBinningInformationForChipParts();

  disableMonBusCols();
  programPixelRegs();

  m_currentTrimmer = nullptr;

  KARABO_LOG_INFO << "DNL Values Measured";
}

util::Hash DsscLadderParameterTrimming::createInputChannelConfig(const std::vector<std::string> & outputChannels, const std::string & onSlowness)
{
  util::Hash inputConfig;
  inputConfig.set<vector < string >> ("connectedOutputChannels", outputChannels);
  inputConfig.set<string>("dataDistribution", "shared");
  inputConfig.set<string>("onSlowness", onSlowness);
  return inputConfig;
}


void DsscLadderParameterTrimming::importBinningInformationFile()
{
  const auto outputDir = get<string>("outputDir");
  m_calibGenerator.setOutputDir(outputDir);

  m_calibGenerator.importBinningInformationFile(get<string>("importDataFileName"));

  set<bool>("binningInfoLaoded",m_calibGenerator.isDNLMapLoaded());

  updateModuleInfo();
}


void DsscLadderParameterTrimming::importADCGainMapFile()
{
  const auto outputDir = get<string>("outputDir");
  m_calibGenerator.setOutputDir(outputDir);

  m_calibGenerator.importADCGainMapFile(get<string>("importDataFileName"));

  set<bool>("pixelAdcGainMapLoaded",m_calibGenerator.isADCGainMapLoaded());

  updateModuleInfo();
}


void DsscLadderParameterTrimming::importSpektrumFitResultsFile()
{
  const auto outputDir = get<string>("outputDir");
  m_calibGenerator.setOutputDir(outputDir);

  m_calibGenerator.importSpektrumFitResultsFile(get<string>("importDataFileName"));

  set<bool>("spectrumFitResultsLoaded",m_calibGenerator.isSpectrumFitResultsLoaded());

  updateModuleInfo();
}


void DsscLadderParameterTrimming::computeTargetGainADCConfiguration()
{
  const auto outputDir = get<string>("outputDir");
  m_calibGenerator.setOutputDir(outputDir);

  bool ok = m_calibGenerator.computeTargetGainADCConfiguration(get<string>("gainSelection"));

  if(ok){
    const std::string fileName = outputDir + "/CalibratedConfig_" + utils::stringReplace(get<string>("gainSelection"),'/','_') + ".conf";
    storeFullConfigFile(fileName,true);

    KARABO_LOG_INFO << "Calibrated configuration saved to:\n-->" << fileName;

    set<bool>("pixelCalibrationDataSettingsValid",false);
  }
}


void DsscLadderParameterTrimming::runGainTrimming()
{
  StateChangeKeeper keeper(this);

  changeDeviceState(State::ACQUIRING);

  enableMeanValueAcquisition(false);

  double targetSlope = get<double>("trimTargetSlope");
  double relDiff     = get<unsigned short>("trimMaxRelAbsDiff");
  double maxDiffAbs  = targetSlope * relDiff / 100.0;

  if (!allDevicesAvailable()) {
    KARABO_LOG_ERROR << "Required Karabo Infrastructure not complete - will not run Pixel Delay Trimming";
    return;
  }

  initTrimming();

  KARABO_LOG_INFO << "Start GainTrimming in Injection Mode " << getInjectionModeName(injectionMode);

  bool ok;
  auto trimmer = getNewChipTrimmer(ok);
  if (!ok) return;

  trimmer.calibratePxsIrampSetting(targetSlope, maxDiffAbs, false);

  trimmer.showTrimmingStats();

  // create some outputs
  std::vector<double> trimmedSlopes;
  std::string range;
  int goodSlopesRel;

  saveGainTrimmingOutputs();

  KARABO_LOG_INFO << trimmer.getGoodTrimmedPixelsStr(trimmedSlopes,range,targetSlope,maxDiffAbs,goodSlopesRel);

  DSSCSTATUS("Trimmed Slopes Stats: " + utils::getVectorStatsStr(trimmedSlopes));

  m_currentTrimmer = nullptr;

  const std::string fileName = get<string>("outputDir") + "/" +utils::getLocalTimeStr() +"_GainTrimmedConfig.conf";
  storeFullConfigFile(fileName,true);

  KARABO_LOG_INFO << "Gain trimming done";
}


void DsscLadderParameterTrimming::saveGainTrimmingOutputs()
{
  string fileName = get<string>("outputDir") + "/GainTrimmingResults.h5";

  const auto finalSlopes = getFinalSlopes();
  const auto irampSettings = utils::convertVectorType<uint32_t,double>(getPixelRegisters()->getSignalValues("Control register","all","RmpFineTrm"));
  // display one dimensional vector as 2 dimensional image
  DsscHDF5TrimmingDataWriter dataWriter(fileName);
  dataWriter.setMeasurementName("LadderTrimming");
  dataWriter.addImageData("FinalSlopes", 512, finalSlopes);
  dataWriter.addImageData("RmpFineTrm", 512, irampSettings);

  {
    int numPixels = finalSlopes.size();
    utils::DataHisto dataHisto;
    vector<unsigned short> corrValues(numPixels);
    double scaleFactor = utils::convertToUINT16(corrValues, finalSlopes);
    dataHisto.add(corrValues.data(), numPixels);
    dataWriter.addHistoData("FinalSlopesHisto", dataHisto, scaleFactor);
  }

  {
    const auto irampSettingsShort = utils::convertVectorType<double,uint16_t>(irampSettings);
    utils::DataHisto dataHisto;
    dataHisto.add(irampSettingsShort.data(), irampSettingsShort.size());
    dataWriter.addHistoData("RmpFineTrmHisto", dataHisto, 1);
  }
}


void DsscLadderParameterTrimming::displayBinValuesOfPixel()
{
    if(!m_binValues.empty()){
      std::vector<uint32_t> xValues;
      bool ok = utils::getSimpleSweepVector(get<string>("injectionSweepRange"), xValues);
      if(!ok)
      {
        KARABO_LOG_ERROR << "Injection Sweep Range Does not fit";
        return;
      }
      std::vector<uint32_t> emptyVec;
      displayInjectionSweep(m_binValues,xValues,emptyVec);
    }
}

void DsscLadderParameterTrimming::setGainConfigurationFromFitResultsFile()
{

  string fileName = get<string>("importDataFileName");
  if(!utils::checkFileExists(fileName)){
    KARABO_LOG_ERROR << "File does not exist:" << fileName;
    return;
  }

  SuS::CHIPGainConfigurator configurator(this);
  configurator.loadGainConfiguration("RmpFineTrm",fileName,false);

  updateModuleInfo();

  KARABO_LOG_INFO << "Calibrated Gain Configuration Programmed from " << fileName;
}

void DsscLadderParameterTrimming::loadPxInjCalibData(SuS::CHIPTrimmer * trimmer)
{
  string fileName = get<string>("injectionCalibrationFileName");
  if(fileName.empty()) return;

  if(!utils::checkFileExists(fileName)){
    KARABO_LOG_ERROR << "PixelInjectionCalibrationFile does not exist: " << fileName;
    return;
  }

  if(utils::getFileExtension(fileName) == ".h5"){
    const auto resultPair = DsscHDF5CalibrationDataGenerator::loadPxInjCalibrationFactors(fileName);
    trimmer->setPixelInjectionCalibrationFactors(resultPair.second,resultPair.first);
    KARABO_LOG_ERROR << "PixelInjectionCalibration factors loaded from: " << fileName;
  }
  else
  {
    const auto resultPair = utils::importInjectionCalibrationFactors(fileName);
    trimmer->setPixelInjectionCalibrationFactors(resultPair.second,resultPair.first);
    KARABO_LOG_INFO <<  "PixelInjectionCalibration factors loaded from: " << fileName;
  }

  updateModuleInfo();
}


void DsscLadderParameterTrimming::sramTest(int iterations, bool init)
{
  const std::string baseDir = get<string>("outputDir");

  for(int i=0; i<iterations; i++)
  {
    const std::string runDir = baseDir + "/RUN"+ to_string(i);
    utils::makePath(runDir);
    set<string>("outputDir",runDir);

    if(init){
      remote().set<unsigned short>(m_pptDeviceId, "sramPattern", 0);
      runContinuousMode(false);
      // takes a while to program the whole ladder
      remote().execute(m_pptDeviceId, "fillSramAndReadout", 600);
    }
    matrixSRAMTest();
  }
}


bool DsscLadderParameterTrimming::matrixSRAMTest()
{
  // disable contiunous operation
  runContinuousMode(false);

  int errCnt = 0;
  bool ok = true;

  for (int i=0; i<5; ++i){
    ok &= matrixSRAMTest(i,errCnt);
  }

  if(ok){
    KARABO_LOG_INFO  << "MatrixSRAMTest Successful";
  }else{
    KARABO_LOG_ERROR << errCnt <<  " errors during MatrixSRAMTest";
  }

  return ok;
}


bool DsscLadderParameterTrimming::matrixSRAMTest(int patternID, int &errCnt)
{
  const string testversion = "0.2";

  /*Changelog:
   * v0.1 = initial version
   * testpattern_A = 0b101010101;
   * testpattern_B = 0b010101010;
   *
   * v0.2 New version for F2B tests
   * New test order:
   * - Write/Read all 0s ("initialization")
   * - Write/Read all 1s (Check transition to 1)
   * - Write/Read all 0s (Check transition to 0)
   * - Write/Read testpattern_A (Check alternating patterns)
   * - Write/Read testpattern_B
   */
  string testName;
  uint16_t testpattern;
  switch(patternID) {
    case 0: testpattern = 0b000000000;
            testName = "DIG5A:MatrixSRAMTest";
            break;
    case 1: testpattern = 0b111111111;
            testName = "DIG5B:MatrixSRAMTest";
            break;
    case 2: testpattern = 0b000000000;
            testName = "DIG5C:MatrixSRAMTest";
            break;
    case 3: testpattern = 0b101010101;
            testName = "DIG5D:MatrixSRAMTest";
            break;
    case 4: testpattern = 0b010101010;
            testName = "DIG5E:MatrixSRAMTest";
            break;
    default: KARABO_LOG_ERROR << "Bad pattern ID selected.";
             return false;
  }

  KARABO_LOG_INFO << "Starting test "<< testName;

  if(!fillSramAndReadout(testpattern,true)){
    KARABO_LOG_ERROR << "Error during fillSramAndReadout, SRAM Test aborted";
    return false;
  }
  const string testPath = get<string>("outputDir") + "/" + testName;
  const auto moduleInfo = utils::DsscModuleInfo::getModuleInfoStr(get<string>("moduleInfo.quadrantID"),
                                                                  get<unsigned int>("moduleInfo.moduleNr"),
                                                                  std::stoul(get<string>("moduleInfo.iobSerial"),0,16));

  int errCnt_thisPattern = saveSramTestResult(testPath,moduleInfo,testpattern);
  errCnt += errCnt_thisPattern;

  bool ok = (errCnt_thisPattern==0);
  if(!ok){
    KARABO_LOG_ERROR <<  testName << " - " << errCnt_thisPattern << " errors found";
  }

  return ok;
}

} // namespace karabo
