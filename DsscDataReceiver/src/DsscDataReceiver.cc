/*
 * $Id: DsscDataReceiver.cc 9210 2017-10-26 15:59:42Z manfred.kirchgessner@ziti.uni-heidelberg.de $
 *
 * Author: <manfred.kirchgessner@ziti.uni-heidelberg.de>
 *
 * Created on April, 2016, 03:34 PM
 *
 * Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved.
 */
#include <iostream>
#include "utils.h"

#include "DsscTrainDataSchema.h"
#include "DsscDataReceiver.hh"
#include "DsscHDF5CorrectionFileReader.h"
#include "CHIPInterface.h"
#include "DsscHDF5TrimmingDataWriter.h"
#include "DsscTrainDataProcessor.h"
#include "DsscDependencies.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#define MAINOFFS 10
using namespace std;

USING_KARABO_NAMESPACES


namespace karabo {

    const std::unordered_map<std::string,std::string> DsscDataReceiver::directoryStructure {
          {"imageData"              ,"INSTRUMENT.DSSC.imageData.data"},
          {"pCellId"                ,"INSTRUMENT.DSSC.pulseData.cellId"},
          {"pLength"                ,"INSTRUMENT.DSSC.pulseData.length"},
          {"pPulseId"               ,"INSTRUMENT.DSSC.pulseData.pulseId"},
          {"pStatus"                ,"INSTRUMENT.DSSC.pulseData.status"},
          {"ptrainId"               ,"INSTRUMENT.DSSC.pulseData.trainId"},
          {"asicTrailer"            ,"INSTRUMENT.DSSC.specificData.asicTrailer"},
          {"pptData"                ,"INSTRUMENT.DSSC.specificData.pptData"},
          {"sibData"                ,"INSTRUMENT.DSSC.specificData.sibData"},
          {"dataId"                 ,"INSTRUMENT.DSSC.trainData.dataId"},
          {"imageCount"             ,"INSTRUMENT.DSSC.trainData.imageCount"},
          {"tbLinkId"               ,"INSTRUMENT.DSSC.trainData.tbLinkId"},
          {"femLinkId"              ,"INSTRUMENT.DSSC.trainData.femLinkId"},
          {"trainId"                ,"INSTRUMENT.DSSC.trainData.trainId"},
          {"majorTrainFormatVersion","INSTRUMENT.DSSC.trainData.majorTrainFormatVersion"},
          {"minorTrainFormatVersion","INSTRUMENT.DSSC.trainData.minorTrainFormatVersion"},
          {"magicNumberStart"       ,"INSTRUMENT.DSSC.trainData.magicNumberStart"},
          {"checkSum0"              ,"INSTRUMENT.DSSC.trainData.checkSum0"},
          {"checkSum1"              ,"INSTRUMENT.DSSC.trainData.checkSum1"},
          {"status"                 ,"INSTRUMENT.DSSC.trainData.status"},
          {"magicNumberEnd"         ,"INSTRUMENT.DSSC.trainData.magicNumberEnd"},
          {"detSpecificLength"      ,"INSTRUMENT.DSSC.trainData.detSpecificLength"},
          {"tbSpecificLength"       ,"INSTRUMENT.DSSC.trainData.tbSpecificLength"}};


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, DsscDataReceiver)

    void DsscDataReceiver::expectedParameters(Schema& expected) {

      PATH_ELEMENT(expected).key("outputDir")
                .displayedName("Output Directory")
                .description("Output directory for HDF5 files")
                .isDirectory()
                .tags("receiver")
                .assignmentOptional().defaultValue("./").reconfigurable()
                .commit();

      UINT32_ELEMENT(expected).key("udpPort")
              .displayedName("UDP Port")
              .description("UDP Port number")
              .tags("receiver")
              .assignmentMandatory().reconfigurable()
              .commit();

      BOOL_ELEMENT(expected).key("saveImageWiseSorted")
              .displayedName("SaveImageWise Sorted Data")
              .description("ImageWise sorted data reduced hard disk space if less than 800 frames are stored")
              .assignmentOptional().defaultValue(false).reconfigurable()
              .allowedStates(State::UNKNOWN,State::OFF,State::STARTED)
              .commit();

      PATH_ELEMENT(expected).key("sramBlacklistFileName")
                .displayedName("SRAM Blacklist Filename")
                .description("Input file name for the SRAM Blacklist")
                .tags("dataCorrect")
                .assignmentOptional().defaultValue("./").reconfigurable()
                .commit();

      SLOT_ELEMENT(expected).key("loadSramBlacklist")
                .displayedName("Load SRAM Blacklist")
                .description("Load Sram Blacklist from file, is also loaded after name change")
                .commit();


      BOOL_ELEMENT(expected).key("sramBlacklistValid")
              .displayedName("Sram Blacklist Valid")
              .description("If an SRAM Blacklist was loaded it is indicated by this flag")
              .readOnly().initialValue(false)
              .commit();

      SLOT_ELEMENT(expected).key("clearSramBlacklist")
                .displayedName("Clear SRAM Blacklist")
                .description("clear data, wont be applied anymore")
                .commit();

      SLOT_ELEMENT(expected)
                .key("startFileReading").displayedName("Start File Reading")
                .description("Start reading TrainData files from fileModeInputDir")
                .allowedStates(State::EXTRACTING)
                .commit();

      BOOL_ELEMENT(expected).key("enFileMode")
              .displayedName("En File Mode")
              .description("Load data from a directory and pass it to the TrainData pipeline")
              .assignmentOptional().defaultValue(false).reconfigurable()
              .allowedStates(State::UNKNOWN,State::OFF)
              .commit();

      PATH_ELEMENT(expected).key("fileModeInputDir")
                .displayedName("file Mode Input Directory")
                .description("Input directory for file mode")
                .isDirectory()
                .assignmentOptional().defaultValue("./").reconfigurable()
                .allowedStates(State::UNKNOWN,State::OFF,State::EXTRACTING)
                .commit();

      UINT64_ELEMENT(expected).key("maxFilesToRead")
              .displayedName("Stop After N Files")
              .description("Read only N files, 0 measn read all")
              .assignmentOptional().defaultValue(0).reconfigurable()
              .commit();

      UINT32_ELEMENT(expected).key("asicToShow")
              .displayedName("ASIC Data to show")
              .description("ASIC data to show")
              .tags("receiver")
              .assignmentOptional().defaultValue(11).reconfigurable()
              .allowedStates(State::STARTED, State::ACQUIRING)
              .commit();


      UINT32_ELEMENT(expected).key("numTrainsToStore")
              .displayedName("Num Trains to Store")
              .description("Number of trains to stored during next acquisition")
              .tags("receiver")
              .assignmentOptional().defaultValue(5).reconfigurable()
              .commit();

      UINT32_ELEMENT(expected).key("numStoredTrains")
              .displayedName("Num Stored Trains")
              .description("Number of Trains stored during current acquisition")
              .readOnly()
              .commit();

      BOOL_ELEMENT(expected).key("acquiring")
              .displayedName("Acquiring Trains")
              .description("receiver is currently in acquisition mode")
              .readOnly()
              .commit();

      UINT32_ELEMENT(expected).key("displayFrame")
              .displayedName("Frame to Show")
              .description("Frame To show")
              .tags("receiver")
              .assignmentOptional().defaultValue(0).reconfigurable()
              .commit();


      UINT32_ELEMENT(expected).key("numFramesToReceive")
                .displayedName("Num Images To Receive - no functionality")
                .description("Deprecated - Number of images that are expected from the camera")
                .tags("receiver")
                .assignmentOptional().defaultValue(800).reconfigurable()
                .allowedStates(State::UNKNOWN,State::OFF)
                .commit();

      UINT32_ELEMENT(expected).key("testPattern")
                .displayedName("Expected TestPattern")
                .description("Expected Test Pattern from FEM")
                .tags("receiver")
                .assignmentOptional().defaultValue(5).reconfigurable()
                .commit();

      BOOL_ELEMENT(expected).key("checkTestPatternData")
              .displayedName("Acquire TestPatterns")
              .description("Start Testpattern checking")
              .assignmentOptional().defaultValue(false).reconfigurable()
              .commit();

      BOOL_ELEMENT(expected).key("activateDisplay")
              .displayedName("Display Active")
              .tags("pixelPreview")
              .description("Stop Visualization by disabling, mabe needed during data acquisition")
              .assignmentOptional().defaultValue(true).reconfigurable()
              .commit();
    /*
      STRING_ELEMENT(expected).key("showMeasurementData")
                .displayedName("Show Measurement Data")
                .description("Select Data to be displayed")
                .assignmentOptional().defaultValue("").reconfigurable()
                .options({"Images","MeanValues","RMS Values"});
                .commit();
      */

      UINT32_ELEMENT(expected).key("updateFrequency")
                .displayedName("Display Update Frequency")
                .description("Fraction of trains which should be displayed, reduced computations")
                .assignmentOptional().defaultValue(3).reconfigurable()
                .commit();

      NODE_ELEMENT(expected).key("monitor")
              .description("Monitor Settings")
              .displayedName("Monitor Settings")
              .commit();


      UINT32_ELEMENT(expected).key("monitor.asicPixelToShow")
                .displayedName("ASICPixel")
                .description("Select ASIC Pixel to show in pixel preview")
                .tags("pixelPreview")
                .assignmentOptional().defaultValue(0).reconfigurable()
                .commit();


      UINT32_ELEMENT(expected).key("monitor.asicToShow")
                .displayedName("ASIC")
                .description("Select ASIC of asic pixel to show in preview")
                .tags("pixelPreview")
                .assignmentOptional().defaultValue(12).reconfigurable()
                .commit();

      UINT32_ELEMENT(expected).key("monitor.ladderPixelToShow")
                .displayedName("LadderPixel")
                .description("Select LadderPixel to show in preview")
                .tags("pixelPreview")
                .assignmentOptional().defaultValue(0).reconfigurable()
                .commit();


      BOOL_ELEMENT(expected).key("removeOrig")
                .displayedName("Remove after Transfer")
                .description("Remove data after transfer")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .commit();

      STRING_ELEMENT(expected).key("transferDirectory")
                .displayedName("TransferDirectory")
                .description("Select Source Directory for data transfer")
                .assignmentOptional().defaultValue("").reconfigurable()
                .commit();

      STRING_ELEMENT(expected).key("transferDistantDirectory")
                .displayedName("Distant TransferDirectory")
                .description("Select distant Directory for data transfer")
                .assignmentOptional().defaultValue("").reconfigurable()
                .commit();


      SLOT_ELEMENT(expected)
                .key("startDataTransfer").displayedName("Start Data Transfer")
                .description("Data transfer to max-exfl.desy.de")
                .commit();

      BOOL_ELEMENT(expected).key("ladderMode")
                .displayedName("Ladder Mode")
                .description("Sort valid data of 16 or 1 ASIC")
                .tags("receiver")
                .assignmentOptional().defaultValue(true).reconfigurable()
                .allowedStates(State::STARTED, State::OFF, State::UNKNOWN, State::ACQUIRING)
                .commit();

      BOOL_ELEMENT(expected).key("enHDF5Compression")
                .displayedName("Enable HDF5 Compression")
                .description("Set HDF5 compression on reduces acquisition speed")
                .tags("receiver")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .commit();

      UINT16_ELEMENT(expected).key("minSram")
              .displayedName("Min SRAM For Histograms")
              .description("Minumum SRAM Address to fill into Histograms")
              .assignmentOptional().defaultValue(0).reconfigurable()
              .maxExc(800)
              .commit();

      UINT16_ELEMENT(expected).key("maxSram")
              .displayedName("Max Sram for Histogram")
              .description("Maximum SRAM Address to fill into Histograms")
              .assignmentOptional().defaultValue(799).reconfigurable()
              .maxExc(800)
              .commit();

       BOOL_ELEMENT(expected).key("storeMeanAndRMS")
                .displayedName("Store Mean and RMS Values")
                .description("Store for each pixel a mean and RMS value. Slows down data acquisition")
                .tags("receiver")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::STARTED, State::OFF, State::UNKNOWN, State::ACQUIRING)
                .commit();

      UINT32_ELEMENT(expected).key("storeNthFrame")
                .displayedName("Store only every nth train")
                .description("Dismiss number of frames in order to reduce amount of data during time series")
                .assignmentOptional().defaultValue(1).reconfigurable()
                .allowedStates(State::STARTED, State::OFF, State::UNKNOWN, State::ACQUIRING)
                .commit();

      BOOL_ELEMENT(expected).key("enableDataReduction")
                .displayedName("Enable Data Reduction")
                .description("Enables storage of only every nth frame")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::STARTED, State::OFF, State::UNKNOWN, State::ACQUIRING)
                .commit();


      NODE_ELEMENT(expected).key("correction")
              .description("Enable Correction Modes")
              .displayedName("Correction Modes")
              .commit();

      UINT32_ELEMENT(expected).key("correction.threshold")
                .displayedName("Apply Threshold")
                .description("If value > 0 images show only values larger than threshold")
                .tags("dataCorrect")
                .assignmentOptional().defaultValue(0).reconfigurable()
                .commit();

      UINT16_ELEMENT(expected).key("correction.gccwrap")
              .displayedName("GCC Wrap")
              .description("Select threshold for GCC Wrap")
              .tags("dataCorrect")
              .assignmentOptional().defaultValue(4).reconfigurable()
              .commit();

      BOOL_ELEMENT(expected).key("correction.bgDataValid")
                .displayedName("Background Data Available")
                .description("Shows if background and is available")
                .readOnly()
                .commit();

      BOOL_ELEMENT(expected).key("correction.sramCorrectionValid")
                .displayedName("Sram Correction Data Available")
                .description("Shows if SramCorrection data is available")
                .readOnly()
                .commit();

      BOOL_ELEMENT(expected).key("correction.showRawData")
                .displayedName("Show Raw Data")
                .description("Enable Raw Data and disable all correction")
                .tags("dataCorrect")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .commit();

      BOOL_ELEMENT(expected).key("correction.enThreshold")
                .displayedName("Enable Threshold")
                .description("Enable Threshold for all pixels")
                .tags("dataCorrect")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .commit();

      BOOL_ELEMENT(expected).key("correction.correct")
                .displayedName("Enable SRAM Correction")
                .description("Enable SRAM Correction for all pixels")
                .tags("dataCorrect")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .commit();

      BOOL_ELEMENT(expected).key("correction.showCorrect")
                .displayedName("Show SRAM Correction Data")
                .description("Shows data that is used to correct SRAM")
                .tags("dataCorrect")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .commit();

      BOOL_ELEMENT(expected).key("correction.subtract")
                .displayedName("Subtract Background")
                .description("Subtract Background if background data is available")
                .tags("dataCorrect")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .commit();


      SLOT_ELEMENT(expected)
                .key("open").displayedName("Connect UDP").description("Open connection to UDP Socket")
                .allowedStates(State::UNKNOWN,State::OFF)
                .commit();

       SLOT_ELEMENT(expected)
                .key("close").displayedName("Disconnect UDP").description("Close connection to UDP Socket")
                .allowedStates(State::STARTED)
                .commit();

       SLOT_ELEMENT(expected)
                .key("activate").displayedName("Activate").description("Activate Data Taking for 5 seconds")
                .allowedStates(State::STARTED)
                .commit();

       SLOT_ELEMENT(expected)
                .key("reset").displayedName("Reset").description("Resets the device in case of an error")
                .allowedStates(State::ERROR)
                .commit();

      SLOT_ELEMENT(expected).key("stop")
                .displayedName("Stop")
                .description("Stops receiving")
                .allowedStates(State::ACQUIRING)
                .commit();

      SLOT_ELEMENT(expected).key("start")
                .displayedName("Start")
                .description("Start Device")
                .allowedStates(State::STARTED, State::OFF, State::UNKNOWN, State::ACQUIRING)
                .commit();

      SLOT_ELEMENT(expected).key("restart")
                .displayedName("Restart")
                .description("Stop,Close,Open and initialize receive buffers")
                .allowedStates(State::STARTED, State::OFF, State::UNKNOWN, State::ACQUIRING)
                .commit();

      SLOT_ELEMENT(expected)
                .key("acquireTrains").displayedName("Acquire Trains").description("Store selected number of trains in selected directory")
                .allowedStates(State::STARTED,State::ACQUIRING)
                .commit();

      SLOT_ELEMENT(expected)
                .key("flushTrainStorage").displayedName("FlushStorage").description("Stop,Flush received packets and sorted trains, And Start again")
                .allowedStates(State::STARTED, State::OFF, State::ACQUIRING)
                .commit();

      SLOT_ELEMENT(expected)
                .key("acquireBackgroundAndCorrection")
                .displayedName("Acquire Background and Correction").description("Acquire correction and save to file for every pixel and each SRAM address, same for a background map")
                .allowedStates(State::STARTED, State::ACQUIRING)
                .commit();

      SLOT_ELEMENT(expected)
                .key("loadBackgroundAndCorrection")
                .displayedName("Load Background and Correction").description("Load correction from selected output directory")
                .allowedStates(State::STARTED, State::ACQUIRING)
                .commit();

      SLOT_ELEMENT(expected).key("clearSramCorrection")
                .displayedName("Clear SRAM Correction")
                .description("clear data, wont be applied anymore")
                .commit();

      SLOT_ELEMENT(expected)
                .key("clearThresholdMap")
                .displayedName("Clear Threshold Map").description("Reset threshold map and restart acquisition")
                .allowedStates(State::STARTED, State::ACQUIRING)
                .commit();

      SLOT_ELEMENT(expected)
                .key("resetHistograms")
                .displayedName("Reset Histograms")
                .description("Manual Reset of Histograms")
                .allowedStates(State::STARTED, State::ACQUIRING)
                .commit();

      SLOT_ELEMENT(expected)
                .key("saveCurrentPixelHisto").displayedName("Save Pixel Histogram")
                .description("Saves histogram in OuputDirectory")
                .commit();

      BOOL_ELEMENT(expected).key("showOnlineHistogram")
                .displayedName("Show Online histogram")
                .description("Shows the real online histogram from Measurement, if disabled the pixel histo is shown")
                .assignmentOptional().defaultValue(false)
                .reconfigurable()
                .commit();

      BOOL_ELEMENT(expected).key("displayHistoLogscale")
                .displayedName("Display Histogram Logscale")
                .description("Switch display of Histograms to Logscale")
                .assignmentOptional().defaultValue(false)
                .reconfigurable()
                .commit();

      BOOL_ELEMENT(expected).key("enableLadderPreview")
                .displayedName("En Ladder Preview")
                .description("Enable update of ladder preview")
                .assignmentOptional().defaultValue(true)
                .reconfigurable()
                .commit();

      BOOL_ELEMENT(expected).key("enableDAQOutput")
                .displayedName("En DAQ Monitor Output")
                .description("Enable output to connect trimming devices")
                .assignmentOptional().defaultValue(false)
                .reconfigurable()
                .commit();

      BOOL_ELEMENT(expected).key("enableASICPreview")
                .displayedName("En ASCI Preview")
                .description("Enable update of ASIC preview")
                .assignmentOptional().defaultValue(true)
                .reconfigurable()
                .commit();

      BOOL_ELEMENT(expected).key("enablePixelPreview")
                .displayedName("En Pixel Preview")
                .description("Enable update of pixel preview")
                .assignmentOptional().defaultValue(true)
                .reconfigurable()
                .commit();

      STRING_ELEMENT(expected)
                .key("asicsToRecord").displayedName("ASICs to Record")
                .description("define ASICs which should be recorded")
                .assignmentOptional().defaultValue("11111111_11111111").reconfigurable()
                .commit();

      SLOT_ELEMENT(expected)
              .key("setASICsToRecord").displayedName("Set ASICs to Record")
              .description("Define ASICs which are sending and which should be recorded in the HDF5 file: 11111111_11111111")
              .commit();

      SLOT_ELEMENT(expected)
              .key("getASICsToRecord").displayedName("Update ASICs to Record")
              .description("Check data from Ladder for which ASICs are actually sending data")
              .commit();

      BOOL_ELEMENT(expected).key("saveToHDF5")
                .displayedName("Save Images")
                .description("Save Images as HDF5")
                .assignmentOptional().defaultValue(false)
                .reconfigurable()
                .commit();

      BOOL_ELEMENT(expected).key("saveJustSpectra")
                .displayedName("Only Store Spectrums Only")
                .description("Only Save Spectrum")
                .tags("receiver")
                .assignmentOptional().defaultValue(false)
                .reconfigurable()
                .commit();

      BOOL_ELEMENT(expected).key("storeHistograms")
                .displayedName("Store Histograms in ASCII")
                .description("Store histogram ascii files per measurement directory")
                .tags("receiver")
                .assignmentOptional().defaultValue(false).reconfigurable()
                .allowedStates(State::STARTED, State::OFF, State::UNKNOWN, State::ACQUIRING)
                .commit();

      BOOL_ELEMENT(expected).key("keepReceiving")
                .displayedName("Keep receiving")
                .description("Keeps the activate button pressed")
                .assignmentOptional().defaultValue(true)
                .reconfigurable()
                .commit();

      UINT64_ELEMENT(expected).key("currentTrainId")
                .displayedName("Current Train ID")
                .description("Monitors the currently processed Train ID token")
                .readOnly()
                .commit();

      UINT64_ELEMENT(expected).key("trainCnt")
                .displayedName("Received TrainCount")
                .description("Monitors the currently processed number of correctly received trains")
                .readOnly()
                .commit();

      UINT64_ELEMENT(expected).key("errorCnt")
                .displayedName("Wrong TestPatterns")
                .description("Number of received trains with wrong TestPatterns")
                .readOnly()
                .commit();

      UINT32_ELEMENT(expected).key("recvStand")
                .displayedName("Fillstand Receive")
                .description("fillstand of packet receive buffer")
                .readOnly()
                .commit();

      UINT32_ELEMENT(expected).key("sortStand")
                .displayedName("Fillstand Sort")
                .description("Fillstand of sorted image buffer")
                .assignmentOptional().defaultValue(5)
                .commit();

      UINT32_ELEMENT(expected).key("lostStand")
                .displayedName("Lost Frac %")
                .description("Fraction of lost trains")
                .readOnly()
                .commit();

      NODE_ELEMENT(expected).key("specificData")
              .displayedName("DetSpecificData")
              .commit();

      UINT16_ELEMENT(expected).key("specificData.pulseCnt")
                .displayedName("Pulse Cnt")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("specificData.pptVetoCnt")
                .displayedName("PPT Veto Cnt")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("specificData.numPreBursVetos")
                .displayedName("Num Preburst Vetos")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("specificData.userSpecific1")
                .displayedName("User Specific 1")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("specificData.userSpecific2")
                .displayedName("User Specific 2")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("specificData.userSpecific3")
                .displayedName("User Specific 3")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("specificData.moduleNr")
                .displayedName("Module Nr")
                .readOnly()
                .commit();

      UINT32_ELEMENT(expected).key("specificData.iobSerial")
                .displayedName("IOB Serial")
                .readOnly()
                .commit();

      BOOL_ELEMENT(expected).key("specificData.sort_asic_wise")
                .displayedName("Sort Asic Wise")
                .readOnly()
                .commit();

      BOOL_ELEMENT(expected).key("specificData.rotate_ladder")
                .displayedName("Rotate Ladder")
                .readOnly()
                .commit();

      BOOL_ELEMENT(expected).key("specificData.send_dummy_dr_data")
                .displayedName("Dummy DR Data")
                .readOnly()
                .commit();

      STRING_ELEMENT(expected).key("specificData.dataFormat")
                .displayedName("Data Format")
                .readOnly()
                .commit();


      NODE_ELEMENT(expected).key("asicADCTemperatue")
              .description("Value form the ASIC TempADC")
              .displayedName("ASIC Temperature")
              .commit();

      UINT16_ELEMENT(expected).key("asicADCTemperatue.tempASIC1")
                .displayedName("ASIC 1 Temp")
                .description("Temp ADC Value")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("asicADCTemperatue.tempASIC2")
                .displayedName("ASIC 2 Temp")
                .description("Temp ADC Value")
                .readOnly()
                .commit();


      UINT16_ELEMENT(expected).key("asicADCTemperatue.tempASIC3")
                .displayedName("ASIC 3 Temp")
                .description("Temp ADC Value")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("asicADCTemperatue.tempASIC4")
                .displayedName("ASIC 4 Temp")
                .description("Temp ADC Value")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("asicADCTemperatue.tempASIC5")
                .displayedName("ASIC 5 Temp")
                .description("Temp ADC Value")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("asicADCTemperatue.tempASIC6")
                .displayedName("ASIC 6 Temp")
                .description("Temp ADC Value")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("asicADCTemperatue.tempASIC7")
                .displayedName("ASIC 7 Temp")
                .description("Temp ADC Value")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("asicADCTemperatue.tempASIC8")
                .displayedName("ASIC 8 Temp")
                .description("Temp ADC Value")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("asicADCTemperatue.tempASIC0")
                .displayedName("ASIC 0 Temp")
                .description("Temp ADC Value")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("asicADCTemperatue.tempASIC9")
                .displayedName("ASIC 9 Temp")
                .description("Temp ADC Value")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("asicADCTemperatue.tempASIC10")
                .displayedName("ASIC 10 Temp")
                .description("Temp ADC Value")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("asicADCTemperatue.tempASIC11")
                .displayedName("ASIC 11 Temp")
                .description("Temp ADC Value")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("asicADCTemperatue.tempASIC12")
                .displayedName("ASIC 12 Temp")
                .description("Temp ADC Value")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("asicADCTemperatue.tempASIC13")
                .displayedName("ASIC 13 Temp")
                .description("Temp ADC Value")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("asicADCTemperatue.tempASIC14")
                .displayedName("ASIC 14 Temp")
                .description("Temp ADC Value")
                .readOnly()
                .commit();

      UINT16_ELEMENT(expected).key("asicADCTemperatue.tempASIC15")
                .displayedName("ASIC 15 Temp")
                .description("Temp ADC Value")
                .readOnly()
                .commit();



      NODE_ELEMENT(expected).key("measOutput")
              .description("Output of Measurement Acqusition")
              .displayedName("Measurement Output")
              .commit();

      VECTOR_FLOAT_ELEMENT(expected).key("measOutput.meanValuesVec")
                .displayedName("MeasOutput.Measurement Mean Values")
                .readOnly().initialValue(std::vector<float>(65536,0.0))
                .commit();

      VECTOR_FLOAT_ELEMENT(expected).key("measOutput.rmsValuesVec")
                .displayedName("Measurement RMS Values")
                .readOnly().initialValue(std::vector<float>(65536,0.0))
                .commit();

      UINT64_ELEMENT(expected).key("measOutput.numValues")
                .displayedName("Measurement Total Number of Frames")
                .description("Total Number of Frames used for Mean and RMS computation")
                .readOnly().initialValue(800)
                .commit();


      NODE_ELEMENT(expected).key("histoGen")
              .description("Vectors for Histogram Generation")
              .displayedName("Histogram Generation")
              .commit();

      BOOL_ELEMENT(expected).key("histoGen.constantReset")
                .displayedName("Constant Histo Reset")
                .description("Resets the histograms before each train")
                .assignmentOptional().defaultValue(true)
                .reconfigurable()
                .commit();

      UINT64_ELEMENT(expected).key("histoGen.pixelhistoCnt")
                .displayedName("Pixel Histo Entry County")
                .readOnly().initialValue(0)
                .commit();

      VECTOR_UINT16_ELEMENT(expected).key("histoGen.pixelHistoBins")
                .displayedName("Pixel Histo Bins")
                .readOnly().initialValue(std::vector<unsigned short>(20,0))
                .commit();

      VECTOR_UINT16_ELEMENT(expected).key("histoGen.asicHistoBins")
                .displayedName("ASIC Histo Bins")
                .readOnly().initialValue(std::vector<unsigned short>(20,0))
                .commit();

      VECTOR_UINT32_ELEMENT(expected).key("histoGen.pixelHistoBinValues")
                .displayedName("Pixel Histo Bin Values")
                .readOnly().initialValue(std::vector<unsigned int>(20,0))
                .commit();

      VECTOR_UINT32_ELEMENT(expected).key("histoGen.asicHistoBinValues")
                .displayedName("ASIC Histo Bin Values")
                .readOnly().initialValue(std::vector<unsigned int>(20,0))
                .commit();


      Schema data;
      DSSC_TRAINDATA_SCHEMA_SIMPLE(data);

      OUTPUT_CHANNEL(expected).key("imageOutput")
              .displayedName("ImageOutput")
              .dataSchema(data)
              .commit();

      Schema xfelSchema;
      XFEL_DAQ_SCHEMA(xfelSchema);

      OUTPUT_CHANNEL(expected).key("monitorOutput")
              .displayedName("DAQ Monitor Output")
              .dataSchema(xfelSchema)
              .commit();

      Schema ladderSchema;
      IMAGEDATA(ladderSchema).key("ladderImage")
                .setDimensions("128,512")
                .commit();

      OUTPUT_CHANNEL(expected).key("ladderImageOutput")
              .displayedName("Ladder Image Output")
              .dataSchema(ladderSchema)
              .commit();

      Schema asicSchema;
      IMAGEDATA(asicSchema).key("asicImage")
                .setDimensions("64,64")
                .commit();

      OUTPUT_CHANNEL(expected).key("asicImageOutput")
              .displayedName("ASIC Image Output")
              .dataSchema(asicSchema)
              .commit();

      Schema pixelSchema;
      VECTOR_FLOAT_ELEMENT(pixelSchema).key("pixelData")
                .displayedName("Pixel Data")
                .readOnly().initialValue(std::vector<float>(800,0.0))
                .commit();

      OUTPUT_CHANNEL(expected).key("pixelImageOutput")
              .displayedName("Pixel Data Output")
              .dataSchema(pixelSchema)
              .commit();


    }


    DsscDataReceiver::DsscDataReceiver(const Hash& config)
    : Device<>(config),
      currentTrainData(nullptr),
      lastState(State::STARTED)
    {
      KARABO_INITIAL_FUNCTION(initialize);

      KARABO_SLOT(stop);
      KARABO_SLOT(start);
      KARABO_SLOT(open);
      KARABO_SLOT(close);
      KARABO_SLOT(reset);
      KARABO_SLOT(activate);
      KARABO_SLOT(restart);
      KARABO_SLOT(startFileReading);

      //only used for middle layer device
      KARABO_SLOT(acquireTrains);
      KARABO_SLOT(flushTrainStorage);

      KARABO_SLOT(acquireBackgroundAndCorrection);
      KARABO_SLOT(loadBackgroundAndCorrection);
      KARABO_SLOT(resetHistograms);
      KARABO_SLOT(clearThresholdMap);

      KARABO_SLOT(startDataTransfer);

      KARABO_SLOT(setASICsToRecord);
      KARABO_SLOT(saveCurrentPixelHisto);
      KARABO_SLOT(loadSramBlacklist);
      KARABO_SLOT(clearSramBlacklist);
      KARABO_SLOT(clearSramCorrection);

    }


    DsscDataReceiver::~DsscDataReceiver()
    {
      close();
    }


    void DsscDataReceiver::initialize()
    {
      DSSC::StateChangeKeeper keeper(this,State::OFF);

      m_selAsic    = get<unsigned int>("asicToShow");
      m_selFrame   = get<unsigned int>("displayFrame");
      m_ladderMode = get<bool>("ladderMode");

      m_displayThread = nullptr;
      m_writingThread = nullptr;
      m_pollThread = nullptr;

      setASICsToRecord();

      KARABO_LOG_INFO << "DsscDataReceiver: LadderMode = " << m_ladderMode;
      m_asicPixelToShow = get<unsigned int>("monitor.asicPixelToShow");
      m_asicToShow      = get<unsigned int>("monitor.asicToShow");
      set<unsigned int>("monitor.ladderPixelToShow",utils::calcImagePixel(m_asicToShow,m_asicPixelToShow));

      m_keepPolling = false;
      m_isStopped   = true;
      m_runDisplay  = false;

      set<bool>("correction.bgDataValid",false);
      set<bool>("correction.sramCorrectionValid",false);
      set<bool>("correction.showRawData",true);

      m_correct      = get<bool>("correction.correct");
      m_subtract     = get<bool>("correction.subtract");
      m_enThreshold  = get<bool>("correction.enThreshold");
      m_threshold    = get<unsigned int>("correction.threshold");
      m_bgDataValid  = get<bool>("correction.bgDataValid");
      m_showRawData  = get<bool>("correction.showRawData");
      m_gccwrap      = get<unsigned short>("correction.gccwrap");

      m_acquireBG     = false;
      m_genTrainStats = false;

      m_correctionFunction = getCorrectionFunction();

      clearHistograms();
      clearThresholdMap();

      initStatsAcc();

      m_notSendingASICs.assign(16,0);
      m_pixelData.resize(utils::s_numSram,0);

      DsscHDF5Writer::enHDF5Compression = get<bool>("enHDF5Compression");
    }


    void DsscDataReceiver::clearSramBlacklist()
    {
      m_processor.clearSramBlacklistData();
      set<bool>("sramBlacklistValid",false);
    }


    void DsscDataReceiver::loadSramBlacklist()
    {
      const auto sramBlacklistFileName = get<string>("sramBlacklistFileName");
      if(utils::checkFileExists(sramBlacklistFileName)){
        m_processor.setSramBlacklist(sramBlacklistFileName);
      }else{
        KARABO_LOG_ERROR << "SRAM Blacklist file not found";
      }
      set<bool>("sramBlacklistValid",m_processor.isSramBlacklistValid());
    }


    void DsscDataReceiver::clearSramCorrection()
    {
      m_processor.clearSramCorrectionData();
      m_pixelBackgroundData.clear();
      m_pixelCorrectionData.clear();
      m_bgDataValid = false;
      set<bool>("correction.bgDataValid",false);
      set<bool>("correction.sramCorrectionValid",false);
    }

    void DsscDataReceiver::saveCurrentPixelHisto()
    {
      auto selectedPixel = get<unsigned int>("monitor.ladderPixelToShow");
      std::vector<uint32_t> pixels(1,selectedPixel);
      utils::DataHistoVec dataHistoVec;
      if(get<bool>("showOnlineHistogram")){
        dataHistoVec.push_back(m_pixelHistoVec[selectedPixel]);
      }else{
        dataHistoVec.push_back(pixelHisto);
      }
      string fileName = get<string>("outputDir") + "/OnlineHisto_Px" + to_string(selectedPixel) + ".dat";
      string h5FileName = get<string>("outputDir") + "/OnlineHisto_Px" + to_string(selectedPixel) + ".h5";
      utils::DataHisto::dumpHistogramsToASCII(fileName,pixels, dataHistoVec,0);

      DsscHDF5TrimmingDataWriter dataWriter(h5FileName);
      dataWriter.setMeasurementName("PixelSpectrum");
      dataWriter.addHistoData("SpektrumData",dataHistoVec,pixels);

      KARABO_LOG_INFO << "DataHisto Stored in : " << fileName;
    }


    void DsscDataReceiver::setASICsToRecord()
    {
      string text = get<string>("asicsToRecord");
      if (text.length()==17){
        KARABO_LOG_INFO << "Set ASICs to Record " << text;
        m_actASICs = utils::bitEnableStringToValue(text);
      }else{
        KARABO_LOG_WARN << "String has wrong format " << text;
      }
      DsscDataSorter::setAvailableASICs(m_actASICs);
    }

    void DsscDataReceiver::getASICsToRecord()
    {
      string asicsStr = utils::vectorToBitEnableStringToValue(m_availableASICs);
      set<string>("asicsToRecord",asicsStr);
    }


    void DsscDataReceiver::clearHistograms()
    {
      pixelHisto.clear();
      asicHisto.clear();
    }


    void DsscDataReceiver::clearThresholdMap()
    {
      if(m_ladderMode){
        m_thresholdMap.assign(16*utils::s_numAsicPixels,0.0);
      }else{
        m_thresholdMap.assign(utils::s_numAsicPixels,0.0);
      }
    }


    void DsscDataReceiver::startPolling()
    {
      stopPolling();

      m_keepPolling = true;
      m_pollThread.reset(new boost::thread(boost::bind(&DsscDataReceiver::getFillStands, this)));
      KARABO_LOG_INFO << "PollThread started...";
    }


    void DsscDataReceiver::stopPolling()
    {
      m_keepPolling = false;
      if(m_pollThread){
        m_pollThread->join();
        m_pollThread.reset();
      }
      m_pollThread = nullptr;
    }


    void DsscDataReceiver::startDataTransfer()
    {
        string srcDirName = get<string>("transferDirectory");
        if(!boost::filesystem::exists(srcDirName)){
          KARABO_LOG_WARN << "ERROR: Directory to transfer not found: " << srcDirName;
          return;
        }

        string distName = get<string>("transferDistantDirectory");

        utils::escapeSpaces(distName);
        utils::escapeSpaces(srcDirName);

        string callString  = "rsync -av";
        if(get<bool>("removeOrig")){
            callString += " --remove-source-files ";
        }
        callString += " -e \"ssh -i /home/dssc/.ssh/gweiden-fecdsscdata01-rsa-nokey\" ";
        callString += srcDirName;
        callString += " gweiden@max-exfl.desy.de:/gpfs/exfel/data/scratch/gweiden/Measurements/";
        callString += distName + " && echo \"image data transfer done\" & ";
        system(callString.c_str());
    }


    void DsscDataReceiver::start()
    {
      lastState = getState();

      if(getState() == State::OFF || getState() == State::UNKNOWN){
        open();
      }

      if(getState() == State::STARTED){
        activate();
      }
    }


    void DsscDataReceiver::stopFileMode()
    {
      this->updateState(State::EXTRACTING);
      m_trainSorter.stopSortThread();
    }


    void DsscDataReceiver::stop()
    {
      DSSC::StateChangeKeeper keeper(this,State::STARTED);

      if(get<bool>("enFileMode")){
        stopFileMode();
      }

      m_isStopped = true;

      m_trainSorter.stopSortThread();

      if(m_writingThread){
        m_writingThread->join();
        m_writingThread.reset();
      }
      m_writingThread = nullptr;

      //stopWriteThreads();
    }


    void DsscDataReceiver::stopWriteThreads()
    {
      KARABO_LOG_INFO << "Stop m_writeHDFThreads!";

      for(auto && th : m_writeHDFThreads){
        if(th.joinable()){
          th.join();
        }
      }
      KARABO_LOG_INFO << "Stopped m_writeHDFThreads!";
    }


    void DsscDataReceiver::restart()
    {
      restartLadderMode(get<bool>("ladderMode"));
    }


    void DsscDataReceiver::acquireBackgroundAndCorrection()
    {
      m_genTrainStats = true;
      m_acquireBG     = true;
      m_bgDataValid   = false;

      set<bool>("correction.bgDataValid",m_bgDataValid);
      set<bool>("correction.sramCorrectionValid",m_bgDataValid);

//      const string outputDirRem = get<string>("outputDir");
//      set<string>("outputDir",".");

      acquireTrains();

      while(get<bool>("acquiring")){
        boost::this_thread::sleep(boost::posix_time::seconds(1));
      }
//      set<string>("outputDir",outputDirRem);
    }


    void DsscDataReceiver::loadBackgroundAndCorrection()
    {
      m_bgDataValid = false;
      set<bool>("correction.bgDataValid",m_bgDataValid);

      auto corrPath = get<string>("outputDir");
      corrPath += "/SRAMCorrectionImage.h5";

      if(!boost::filesystem::exists(corrPath)){
        KARABO_LOG_WARN << "No background information file found in directory";
        return;
      }

      DsscHDF5CorrectionFileReader corrFileReader(corrPath);
      if(corrFileReader.isValid()){
        corrFileReader.loadCorrectionData(m_pixelBackgroundData,m_pixelCorrectionData);
        m_bgDataValid = true;
      }else{
        m_processor.clearSramCorrectionData();
      }

      set<bool>("correction.bgDataValid",m_bgDataValid);
      set<bool>("correction.sramCorrectionValid",m_bgDataValid);
    }


    void DsscDataReceiver::restartLadderMode(bool ladderMode)
    {
      DSSC::StateChangeKeeper keeper(this);

      // change ladder mode
      m_ladderMode = ladderMode;
      m_trainSorter.setActiveAsics(m_actASICs);
    }


    void DsscDataReceiver::setActiveAsic()
    {
      m_trainSorter.setActiveAsic(m_selAsic);
    }


    void DsscDataReceiver::flushTrainStorage()
    {
      if(getState() == State::UNKNOWN){
        KARABO_LOG_INFO << "++++ Cannot Flush in UNKNOWN STATE ++++";
        return;
      }

      const auto lastReceivedTrainID = m_trainSorter.getLastReceivedTrainID();
      KARABO_LOG_INFO << "++++ Dismis Invalid Trains in storage up to train ID " << lastReceivedTrainID << "/" << m_currentTrainID <<" ++++";
      m_trainSorter.dismissInvalidTrains(lastReceivedTrainID+1);

      KARABO_LOG_INFO << "++++ Flushed train Storage ++++";
    }


    void DsscDataReceiver::getFillStands()
    {
      while(m_keepPolling) {
        auto recvStand = m_trainSorter.getFillStand(DsscTrainSorter::Recv);
        auto sortStand = m_trainSorter.getFillStand(DsscTrainSorter::Sort);
        auto lostStand = m_trainSorter.getFillStand(DsscTrainSorter::Lost);

        set<unsigned int>("sortStand",sortStand);
        set<unsigned int>("recvStand",recvStand);
        set<unsigned int>("lostStand",lostStand);

        boost::this_thread::sleep(boost::posix_time::seconds(1));
      }
    }


    Schema DsscDataReceiver::createDataSchema(DetectorGeometry geometry)
    {
      /*
      Creates a standardized Krb schema object for streaming data. If a geometry object is passed node elements are created
      accordingly.

      The schema consists of the following elements:

      IMAGEDATA("data")
          The data object. Usually should be created from a numpy or vector object of 1 to 3 dimensions. Will optionally
          also include the detector geometry as a geometry sub-node

      UINT32_ELEMENT("data.stackAxis")
          An addition to the data element schema to hold the axis id over which images are stacked. PyDetLib will expect
          axis 2, i.e. the z-axis, for stacking. The DimensionJuggler device can be used to adjust data with other layouts
          prior to input into PyDetLib devices.

      VECTOR_UINT32_ELEMENT("cellIds")
          A vector containing the memory cell ids which map to the stack axis of "data" element. If supplied it needs to be
          of the same length as the stack-axis dimension.

      VECTOR_UINT32_ELEMENT("pulseIds")
          A vector containing the pulse ids which map to the stack axis of "data" element. If supplied it needs to be
          of the same length as the stack-axis dimension.

      UINT64_ELEMENT("trainId")
          The train id this data token is pertinent to.

      VECTOR_STRING_ELEMENT("passport")
          A passport for the data. Devices which operate on the data should append to this vector with a short description
          of what was done.


      :param geometry: (optional, = None). A geometry object describing the detector from which the data is sent.
      :return: a krb Schema object
      */

      Schema dataSchema;

      UINT64_ELEMENT(dataSchema).key("trainId")
                .readOnly()
                .commit();
    /*
      IMAGEDATA(dataSchema)
            .key("data")
            .setGeometry(geometry)
            .commit();

      UINT32_ELEMENT(dataSchema)
              .key("data.stackAxis")
              .assignmentOptional().noDefaultValue()
              .commit();

      VECTOR_UINT32_ELEMENT(dataSchema)
              .key("cellIds")
              .assignmentOptional().noDefaultValue()
              .commit();

      VECTOR_UINT32_ELEMENT(dataSchema)
              .key("pulseIds")
              .assignmentOptional().noDefaultValue()
              .commit();

      UINT64_ELEMENT(dataSchema)
              .key("trainId")
              .assignmentOptional().noDefaultValue()
              .commit();

      VECTOR_STRING_ELEMENT(dataSchema)
              .key("passport")
              .assignmentOptional().noDefaultValue()
              .commit();
*/
      return dataSchema;
    }


    void DsscDataReceiver::updateStatus(const std::string & text)
    {
      KARABO_LOG_INFO << text;
    }


    void DsscDataReceiver::preReconfigure(Hash& incomingReconfiguration)
    {
      preReconfigureReceiver(incomingReconfiguration);
      preReconfigurePreview(incomingReconfiguration);
      preReconfigureCorrection(incomingReconfiguration);
    }


    void DsscDataReceiver::preReconfigureReceiver(Hash& incomingReconfiguration)
    {
      Hash filtered = this->filterByTags(incomingReconfiguration, "receiver");
      vector<string> paths;
      filtered.getPaths(paths);

      if(!paths.empty()){
        BOOST_FOREACH(string path, paths)
        {
          if(path.compare("asicToShow") == 0){
            m_selAsic = filtered.getAs<unsigned int>(path);
            if(!get<bool>("ladderMode")){
              restartLadderMode(false);
            }
          }else if(path.compare("displayFrame") == 0){
            m_selFrame = filtered.getAs<unsigned int>(path);
          }else if(path.compare("outputDir") == 0){
            const auto outputDir = filtered.getAs<string>(path);
            checkDirExists(outputDir);
          }else if(path.compare("ladderMode") == 0){
            bool ladderMode = filtered.getAs<bool>(path);
            restartLadderMode(ladderMode);
          }else if(path.compare("enHDF5Compression") == 0){
            bool enHDF5Compression = filtered.getAs<bool>(path);
            DsscHDF5Writer::enHDF5Compression = enHDF5Compression;
          }else if(path.compare("storeMeanAndRMS") == 0){
            m_genTrainStats = filtered.getAs<bool>(path);
          }else if(path.compare("saveJustSpectra") == 0){
            bool justSprectra = filtered.getAs<bool>(path);
            if(justSprectra){
              set<bool>("storeHistograms",true);
            }
          }
        }
      }
    }


    void DsscDataReceiver::preReconfigurePreview(Hash& incomingReconfiguration)
    {
      Hash filtered = this->filterByTags(incomingReconfiguration, "pixelPreview");
      vector<string> paths;
      filtered.getPaths(paths);
      if(!paths.empty()){
        BOOST_FOREACH(string path, paths)
        {
          if(path.compare("monitor.asicPixelToShow") == 0){
            m_asicPixelToShow = filtered.getAs<unsigned int>(path);
            set<unsigned int>("monitor.ladderPixelToShow",utils::calcImagePixel(m_asicToShow,m_asicPixelToShow));
            pixelHisto.clear();
          }else if(path.compare("monitor.asicToShow") == 0){
            m_asicToShow = filtered.getAs<unsigned int>(path);
            set<unsigned int>("monitor.ladderPixelToShow",utils::calcImagePixel(m_asicToShow,m_asicPixelToShow));
            pixelHisto.clear();
          }else if(path.compare("monitor.ladderPixelToShow") == 0){
            auto ladderPixel = filtered.getAs<unsigned int>(path);
            m_asicPixelToShow = utils::calcASICPixel(ladderPixel);
            m_asicToShow = utils::getPixelASIC(ladderPixel);
            set<unsigned int>("monitor.asicPixelToShow",m_asicPixelToShow);
            set<unsigned int>("monitor.asicToShow",m_asicToShow);
            pixelHisto.clear();
          }else if(path.compare("activateDisplay") == 0){
            bool enable = filtered.getAs<bool>(path);
            if(enable){
              startDisplay();
            }else{
              stopDisplay();
            }
          }
        }
      }
    }


    void DsscDataReceiver::preReconfigureCorrection(Hash& incomingReconfiguration)
    {
      Hash filtered = this->filterByTags(incomingReconfiguration, "dataCorrect");
      vector<string> paths;
      filtered.getPaths(paths);
      if(!paths.empty()){
        BOOST_FOREACH(string path, paths)
        {
          if(path.compare("correction.gccwrap") == 0){
            m_gccwrap = filtered.getAs<unsigned short>(path);
          }else if(path.compare("correction.threshold") == 0){
            m_threshold = filtered.getAs<unsigned int>(path);
          }else if(path.compare("correction.enThreshold") == 0){
            m_enThreshold = filtered.getAs<bool>(path);
            if(!m_bgDataValid){
              KARABO_LOG_WARN << "Threshold can only be active if background data is available";
            }
          }else if(path.compare("correction.correct") == 0){
            m_correct = filtered.getAs<bool>(path);

            if(!m_bgDataValid){
              KARABO_LOG_WARN << "SRAM Correction can only be active if correction data is available";
              m_processor.clearSramCorrectionData();
            }else{
              if(m_correct){
                m_processor.setSramCorrectionData(m_pixelCorrectionData);
              }else{
                m_processor.clearSramCorrectionData();
              }
            }
          }else if(path.compare("correction.subtract") == 0){
            m_subtract = filtered.getAs<bool>(path);
            if(!m_bgDataValid){
              KARABO_LOG_WARN << "Background can only be active if background data is available";
            }
          }else if(path.compare("correction.showRawData") == 0){
            m_showRawData = filtered.getAs<bool>(path);
          }else if(path.compare("sramBlacklistFileName") == 0){
            const auto sramBlacklistFileName = filtered.getAs<string>(path);
            if(utils::checkFileExists(sramBlacklistFileName)){
              m_processor.setSramBlacklist(sramBlacklistFileName);
            }else{
              KARABO_LOG_ERROR << "SRAM Blacklist file not found";
            }
            set<bool>("sramBlacklistValid",m_processor.isSramBlacklistValid());
          }
        }
        m_correctionFunction = getCorrectionFunction();
        KARABO_LOG_INFO << "Correction Function Changed";
      }
    }


    void DsscDataReceiver::open()
    {
      this->updateState(State::STARTING);

      KARABO_LOG_INFO << "Open";

      set<unsigned long long>("trainCnt",0);

      m_trainSorter.setActiveAsic(m_selAsic);
      m_trainSorter.setActiveAsics(m_actASICs);

      if(get<bool>("enFileMode")){
        this->updateState(State::EXTRACTING);
      }else{
        m_trainSorter.setUDPPort(get<unsigned int>("udpPort"));
        m_trainSorter.start();
        this->updateState(State::STARTED);
      }

      startPolling();

      startDisplay();
    }


    void DsscDataReceiver::close()
    {
      this->updateState(State::CLOSING);

      KARABO_LOG_INFO << "Close Action";

      stopPolling();

      stop();

      stopDataReceiver();

      stopDisplay();

      m_trainSorter.closeConnection();

      this->updateState(State::OFF);
    }


    void DsscDataReceiver::stopDataReceiver()
    {
      m_trainSorter.stopSortThread();
      m_trainSorter.stop();
    }


    void DsscDataReceiver::activate()
    {
      DSSC::StateChangeKeeper keeper(this,State::ACQUIRING);

      KARABO_LOG_INFO << "Activate Action";

      set<bool>("enableDAQOutput",true);

      // There might be a remnant (but finished) thread from previous write
      if (m_writingThread) {
        KARABO_LOG_INFO << "Old writing thread to join in write()!";
        m_writingThread->join();
        m_writingThread.reset();
      }

      m_writingThread.reset(new boost::thread(boost::bind(&Self::receiveData, this)));

      clearPixelHistos();
    }


    void DsscDataReceiver::reset()
    {
      KARABO_LOG_INFO << "Reset Action";
      set<unsigned long long>("trainCnt",0);
      close();
    }


    bool DsscDataReceiver::allASICTestPatternsValid(utils::DsscTrainData * trainDataToCheck)
    {
      uint16_t expectedTestPattern = get<unsigned>("testPattern");

      for(auto && sendingASIC : m_sendingASICs ){
        const auto testPattern = trainDataToCheck->getTestPattern(sendingASIC);
        if(testPattern != expectedTestPattern){
          KARABO_LOG_WARN << "ASIC" << sendingASIC << "did not send data at train : " << trainDataToCheck->trainId;
          return false;
        }
      }
      return true;
    }


    bool DsscDataReceiver::allASICTempValuesValid(utils::DsscTrainData * trainDataToCheck)
    {
      for(auto && sendingASIC : m_sendingASICs ){
        const auto tempValue = trainDataToCheck->getTempADCValue(sendingASIC);
        if(tempValue == 511){
          KARABO_LOG_WARN << "ASIC" << sendingASIC << " temperature Value indicated a problem : " << trainDataToCheck->trainId;
          return false;
        }
      }
      return true;
    }


    void DsscDataReceiver::checkTestPattern(utils::DsscTrainData * trainDataToCheck)
    {
      uint16_t expectedTestPattern = get<unsigned>("testPattern");
      m_sendingASICs.clear();
      for(int asic=0; asic<16; asic++){
        const auto testPattern = trainDataToCheck->getTestPattern(asic);
        if(testPattern == expectedTestPattern){
          m_sendingASICs.push_back(asic);
        }else{
          m_notSendingASICs[asic]++;
          //KARABO_LOG_INFO << "ASIC" << asic << "did not send data at train : " << trainDataToCheck->trainId;
        }
      }
    }


    void DsscDataReceiver::checkTestPatternData(utils::DsscTrainData * trainDataToCheck)
    {
      m_numCheckedData += trainDataToCheck->pulseCnt*utils::s_totalNumPxs;
      m_errorCnt += trainDataToCheck->checkTestPatternData(get<unsigned>("testPattern"));

      KARABO_LOG_INFO << "Errors Found:" << m_errorCnt << " - After "<< m_numCheckedData << " checked words" ;
    }


    void DsscDataReceiver::startFileReading()
    {
      DSSC::StateChangeKeeper keeper(this,State::ACQUIRING);

      set<bool>("acquiring",true);
      set<unsigned int>("numStoredTrains",0);

      m_isStopped = false;

      const string inputDir = get<string>("fileModeInputDir");
      m_trainSorter.setMaxFilesToRead(get<unsigned long long>("maxFilesToRead"));
      m_trainSorter.startReadHDF5FilesDirectory(inputDir);

      m_writingThread.reset(new boost::thread(boost::bind(&Self::receiveData, this)));
    }

    void DsscDataReceiver::fillMetaData()
    {
      fillTrailer();

      fillHeader();

      fillSpecificData();
    }

    std::function<float(int,int,const uint16_t & )> DsscDataReceiver::getCorrectionFunction()
    {
      m_showThreshold = false;

      clearHistograms();
      clearThresholdMap();

      if(m_showRawData){
        return [&](int frame, int pixel, const uint16_t & value ) -> float {
          return this->rawData(frame,pixel,value);
        };
      }

      if(m_bgDataValid)
      {
        if(!m_enThreshold){
          if(!m_correct){
            return [&](int frame, int pixel, const uint16_t & value ) -> float {
              return this->offsetCorrectData(frame,pixel,value);
            };
          }else if(!m_subtract){
            return [&](int frame, int pixel, const uint16_t & value ) -> float {
              return this->sramCorrectData(frame,pixel,value);
            };
          }else{
            return [&](int frame, int pixel, const uint16_t & value ) -> float {
              return this->fullCorrectData(frame,pixel,value);
            };
          }
        }else{
          m_showThreshold = true;
          if(!m_correct){
            return [&](int frame, int pixel, const uint16_t & value ) -> float {
              return this->offsetTHCorrectData(frame,pixel,value);
            };
          }else{
            return [&](int frame, int pixel, const uint16_t & value ) -> float {
              return this->fullCorrectTHData(frame,pixel,value);
            };
          }
        }
      }
      if(m_gccwrap > 0){
        return  [&](int frame, int pixel, const uint16_t & value ) -> float {
         return this->gccWrapCorrect(frame,pixel,value);
        };
      }else{
        return  [&](int frame, int pixel, const uint16_t & value ) -> float {
         return this->rawData(frame,pixel,value);
        };
      }
    }


    float DsscDataReceiver::rawData(int frame, int pixel, const uint16_t & value) const
    {
      return (float)value;
    }


    float DsscDataReceiver::gccWrapCorrect(int frame, int pixel, const  uint16_t & value) const
    {
      return (value < m_gccwrap)? value + 256 : value;
    }


    float DsscDataReceiver::offsetCorrectData(int frame, int pixel, const uint16_t & value) const
    {
      float correctedValue = gccWrapCorrect(frame,pixel,value);

      correctedValue -= m_pixelBackgroundData[pixel];

      return std::max(0.0f,correctedValue + MAINOFFS);
    }


    float DsscDataReceiver::offsetTHCorrectData(int frame, int pixel, const uint16_t & value) const
    {
      float correctedValue = offsetCorrectData(frame,pixel,value)-MAINOFFS;

      return (correctedValue > m_threshold)? 1.0 : 0.0;
    }


    float DsscDataReceiver::sramCorrectData(int frame, int pixel, const uint16_t & value) const
    {
      float correctedValue = gccWrapCorrect(frame,pixel,value);

      correctedValue -= m_pixelCorrectionData[pixel][frame];
      return std::max(0.0f,correctedValue + MAINOFFS);
    }


    float DsscDataReceiver::fullCorrectData(int frame, int pixel, const uint16_t & value) const
    {
      float correctedValue = gccWrapCorrect(frame,pixel,value);
      correctedValue -= (m_pixelBackgroundData[pixel] + m_pixelCorrectionData[pixel][frame]);
      return std::max(0.0f,correctedValue + MAINOFFS);
    }


    float DsscDataReceiver::fullCorrectTHData(int frame, int pixel, const uint16_t & value) const
    {
      float correctedValue = fullCorrectData(frame,pixel,value)-MAINOFFS;

      return (correctedValue > m_threshold)? 1.0 : 0.0;
    }


    // try not to copy data
    util::Hash DsscDataReceiver::getMonitorData()
    {
      const auto & cellIds = currentTrainData->cellIds;

      NDArray imageData(currentTrainData->imageData(),currentTrainData->getNumData(),NDArray::NullDeleter(),getDims());
      NDArray cellIdData(cellIds.data(),cellIds.size(),NDArray::NullDeleter(), Dims(cellIds.size()));
      NDArray trainIdData(&m_currentTrainID,1,Dims(1));

      util::Hash node;
      node.set("data",imageData);
      node.set("cellId",cellIdData);
      node.set("trainId",trainIdData);

      util::Hash monitorData;
      monitorData.set("image",node);
      monitorData.set<string>("imageFormat",currentTrainData->getFormatStr());

      return monitorData;
    }

    util::Dims DsscDataReceiver::getDims()
    {
      util::Dims dims;
      if(currentTrainData->getFormat() == utils::DsscTrainData::DATAFORMAT::PIXEL){
        dims = Dims(currentTrainData->availableASICs.size(),utils::s_numAsicPixels,currentTrainData->pulseCnt);
      }else if(currentTrainData->getFormat() == utils::DsscTrainData::DATAFORMAT::ASIC){
        dims = Dims(currentTrainData->pulseCnt,currentTrainData->availableASICs.size(),utils::s_numAsicPixels);
      }else if(currentTrainData->getFormat() == utils::DsscTrainData::DATAFORMAT::IMAGE){
        dims = Dims(currentTrainData->pulseCnt,128,512);
      }
      return dims;
    }

    // display function
    void DsscDataReceiver::fillImageData(vector<unsigned int> & imageData, int frameNum)
    {
      const size_t numCols = m_ladderMode? 512 : 64;
      const size_t numRows = m_ladderMode? 128 : 64;
      const size_t numImagePixels = numCols * numRows;

      imageData.resize(numImagePixels);

      if(!m_ladderMode){
        auto asicDataArr = m_trainDataToShow->getAsicDataArray(0,frameNum);
        std::copy(asicDataArr.begin(),asicDataArr.end(),imageData.begin());
        for(size_t px=0; px<numImagePixels; px++){
          imageData[px] = m_correctionFunction(frameNum,px,asicDataArr[px]);
        }
      }else{
        auto imageDataArr = m_trainDataToShow->getImageDataArray(frameNum);
        for(size_t px=0; px<numImagePixels; px++){
          imageData[px] = m_correctionFunction(frameNum,px,imageDataArr[px]);
        }
      }
    }

    // display function
    util::Hash DsscDataReceiver::getImageData(int frameNum)
    {
      int numCols = m_ladderMode? 512 : 64;
      int numRows = m_ladderMode? 128 : 64;
      int numPixels = numCols * numRows;

      util::Hash dataHash;

      if(m_showThreshold)
      {
        const auto asicsToSort = m_trainDataToShow->availableASICs;
        const int numAsics = asicsToSort.size();
        const unsigned short minSram = get<unsigned short>("minSram");
        const unsigned short maxSram = get<unsigned short>("maxSram");

        auto accumulateData = accumulateImageData(minSram,maxSram);

        for(int asicIdx = 0; asicIdx < numAsics; asicIdx++){
          const int asicOffs = asicsToSort[asicIdx] * utils::s_numAsicPixels;
          for(uint pxIdx = 0; pxIdx < utils::s_numAsicPixels; pxIdx ++){
            const uint32_t imagePixel = utils::s_imagePixelMap[asicOffs+pxIdx];
            m_thresholdMap[imagePixel] += accumulateData[imagePixel];
          }
        }

        NDArray imageArray(m_thresholdMap.data(),
                         numPixels,
                         NDArray::NullDeleter(),
                         Dims(numRows,numCols));
        dataHash.set("ladderImage",ImageData(imageArray));
      }
      else
      {
        static vector<unsigned int> s_imageData;

        fillImageData(s_imageData,frameNum);

        NDArray imageArray(s_imageData.data(),
                         numPixels,
                         NDArray::NullDeleter(),
                         Dims(numRows,numCols));

        dataHash.set("ladderImage",ImageData(imageArray));
      }

      return dataHash;
    }


    // display function
    util::Hash DsscDataReceiver::getImageData(int frameNum, int asic)
    {
      static vector<unsigned int> s_imageData(utils::s_numAsicPixels,0);

      util::Hash dataHash;

      int asicIdx = (m_ladderMode)? DsscTrainSorter::getAsicIdx(asic) : 0;

      if(asicIdx>=0)
      {
        int asicOffset = asic * utils::s_numAsicPixels;

        if(m_showThreshold)
        {
          const auto * asicSortMap = utils::s_imagePixelMap.data() + asicOffset;
          for(size_t pxIdx=0; pxIdx < utils::s_numAsicPixels; pxIdx++){
            s_imageData[pxIdx] = m_thresholdMap[asicSortMap[pxIdx]];
          }
        }
        else
        {
          auto asicDataArr = m_trainDataToShow->getAsicDataArray(asic,frameNum);
          const auto * asicSortMap = utils::s_imagePixelMap.data() + asicOffset;

          for(size_t pxIdx=0; pxIdx < utils::s_numAsicPixels; pxIdx++){
            s_imageData[pxIdx] = m_correctionFunction(frameNum,asicSortMap[pxIdx],asicDataArr[pxIdx]);
          }
          asicHisto.addToBuf(s_imageData.begin(),s_imageData.end());
        }
      }

      NDArray imageArray(s_imageData.data(),
                         utils::s_numAsicPixels,
                         NDArray::NullDeleter(),
                         Dims(64,64));
      dataHash.set("asicImage",ImageData(imageArray));
      return dataHash;
    }


    // display function
    util::Hash DsscDataReceiver::getPixelData(int asicPixel, int asic)
    {
      int asicIdx = (m_ladderMode)? DsscTrainSorter::getAsicIdx(asic) : 0;

      const size_t numFrames = m_trainDataToShow->pulseCnt;
      m_pixelData.resize(numFrames);

      if(asicIdx < 0){
        m_pixelData.assign(numFrames,0);
      }else{
        asic = (m_ladderMode)? asic : 0;
        int imagePixel = utils::s_imagePixelMap[asic*utils::s_numAsicPixels+asicPixel];

        if(get<bool>("correction.showCorrect") && m_bgDataValid){
          const auto & selPixelcorrData = m_pixelCorrectionData[imagePixel];
          m_pixelData.resize(utils::s_numSram);
          std::copy(selPixelcorrData.begin(),selPixelcorrData.end(),m_pixelData.begin());
        }
        else
        {
          // handles pixel data sort mode
          auto pixelData = m_trainDataToShow->getPixelDataPixelWise(imagePixel);
          for(size_t frame = 0; frame < numFrames; frame++){
            m_pixelData[frame] = m_correctionFunction(frame,imagePixel,pixelData[frame]);
          }
          pixelHisto.addToBuf(m_pixelData.begin(),m_pixelData.end());
        }
      }


      util::Hash dataHash;
      dataHash.set("pixelData",m_pixelData);
      return dataHash;
    }


    bool DsscDataReceiver::checkDirExists(const std::string & outdir)
    {
      boost::filesystem::path data_dir(outdir);
      if (!boost::filesystem::exists( data_dir ))
      {
        if(boost::filesystem::create_directories(data_dir)){
          KARABO_LOG_INFO << "Created new directory: " << outdir;
          return true;
        }else{
          KARABO_LOG_ERROR << "Could not create output directory: " << outdir;
          return false;
        }
      }
      return true;
    }


    bool DsscDataReceiver::checkOutputDirExists()
    {
      return checkDirExists(get<string>("outputDir"));
    }


    void DsscDataReceiver::acquireTrains()
    {
      if(!checkOutputDirExists()){
        KARABO_LOG_ERROR << "Could not create directory...will stop";
        return;
      }

      start();

      set<unsigned int>("numStoredTrains",0);
      set<bool>("enableDAQOutput",false);
      set<bool>("acquiring",true);

      if(!get<bool>("saveJustSpectra") && !m_acquireBG){
        set<bool>("saveToHDF5",true);
      }


      updateState(State::ACTIVE);
    }

    void DsscDataReceiver::updateProcessorParams()
    {
      static const std::vector<uint32_t> availableAsics = utils::getUpCountingVector(16);
      m_processor.setParameters(availableAsics,currentTrainData->pulseCnt,get<unsigned short>("minSram"),get<unsigned short>("maxSram"),currentTrainData->getFormat());
    }


    void DsscDataReceiver::computeSramCorrectionData()
    {
      static const unsigned int nthreads = std::min(4u,std::thread::hardware_concurrency());

      const uint numPixels = m_pixelBackgroundData.size();

#pragma omp parallel for num_threads(nthreads)
      for(size_t pxIdx = 0;pxIdx<numPixels; pxIdx++)
      {
        auto & pixelCorrectionAcc = m_sramCorrectionAcc[pxIdx];
        auto & pixelCorrectionData = m_pixelCorrectionData[pxIdx];
        const uint pixelBackground = m_pixelBackgroundData[pxIdx];
        for(uint sram=0; sram < utils::s_numSram; sram++){
          pixelCorrectionData[sram] = pixelCorrectionAcc[sram].calcStats().mean - pixelBackground;
        }
      }
    }


    void DsscDataReceiver::updateCorrectionMap()
    {
      static const unsigned int nthreads = std::min(4u,std::thread::hardware_concurrency());

      const auto asicsToSort = currentTrainData->availableASICs;
      const int numAsics     = asicsToSort.size();

#pragma omp parallel for num_threads(nthreads)
      for(int asicIdx = 0; asicIdx < numAsics; asicIdx++)
      {
        const auto * asicSortMap = utils::s_imagePixelMap.data() + asicsToSort[asicIdx] * utils::s_numAsicPixels;
        for(uint32_t px = 0; px<utils::s_numAsicPixels; px++){
          const uint32_t imagePixel = asicSortMap[px];
          auto pixelDataArr    = currentTrainData->getPixelDataPixelWise(imagePixel);
          auto & pixelCorrectAcc = m_sramCorrectionAcc[imagePixel];
          for(uint sram=0; sram<utils::s_numSram; sram++){
            const auto value = pixelDataArr[sram];
            pixelCorrectAcc[sram].addValue(value);
          }
        }
      }
    }


    // display function
    std::vector<uint32_t> DsscDataReceiver::accumulateImageData(int minSram, int maxSram)
    {
      static const unsigned int nthreads = std::min(4u,std::thread::hardware_concurrency());

      std::vector<uint32_t> accumulateVector(utils::s_totalNumPxs,0);

      const auto asicsToSort = m_trainDataToShow->availableASICs;
      const uint32_t numAsics = asicsToSort.size();

#pragma omp parallel for num_threads(nthreads)
      for(uint32_t asicIdx=0; asicIdx<numAsics; asicIdx++)
      {
        const uint32_t asicOffs = asicsToSort[asicIdx] * utils::s_numAsicPixels;
        for(uint32_t px = 0; px<utils::s_numAsicPixels; px++)
        {
          const uint32_t imagePixel = utils::s_imagePixelMap[asicOffs+px];
          auto pixelDataArr = m_trainDataToShow->getPixelDataPixelWise(asicsToSort[asicIdx],px);

          for(int frame=minSram;frame<=maxSram; frame++){
            accumulateVector[imagePixel] += m_correctionFunction(frame,imagePixel,pixelDataArr[frame]);
          }
        }
      }
      return accumulateVector;
    }

    void DsscDataReceiver::updateStatsAcc()
    {
      const unsigned short pulseCnt = currentTrainData->pulseCnt;
      const unsigned short minSram = std::min(get<unsigned short>("minSram"),pulseCnt);
      const unsigned short maxSram = std::min(get<unsigned short>("maxSram"),pulseCnt);

      const auto asicsToSort = currentTrainData->availableASICs;
      const int numAsics = asicsToSort.size();

#pragma omp parallel for
      for(int asicIdx = 0; asicIdx < numAsics; asicIdx++)
      {
        const auto * asicOffs = utils::s_imagePixelMap.data() + asicsToSort[asicIdx] * utils::s_numAsicPixels;
        for(uint pxIdx = 0; pxIdx<utils::s_numAsicPixels; pxIdx++){
          const auto imagePixel = asicOffs[pxIdx];
          auto pixelDataArr = currentTrainData->getPixelDataPixelWise(imagePixel);
          for(uint sram=minSram; sram<=maxSram; sram++){
            m_dataStatsAcc[imagePixel].addValue(m_correctionFunction(sram,imagePixel,pixelDataArr[sram]));
          }
        }
      }
    }


    void DsscDataReceiver::computeMeanBursts()
    {
      const unsigned short pulseCnt = currentTrainData->pulseCnt;
      const unsigned short minSram = std::min(get<unsigned short>("minSram"),pulseCnt);
      const unsigned short maxSram = std::min(get<unsigned short>("maxSram"),pulseCnt);

      const auto asicsToSort = currentTrainData->availableASICs;
      const int numAsics = asicsToSort.size();

#pragma omp parallel for
      for(int asicIdx = 0; asicIdx < numAsics; asicIdx++)
      {
        const auto * asicOffs = utils::s_imagePixelMap.data() + asicsToSort[asicIdx] * utils::s_numAsicPixels;
        for(uint pxIdx = 0; pxIdx<utils::s_numAsicPixels; pxIdx++){
          const auto imagePixel = asicOffs[pxIdx];
          const auto & pixelCorrectAcc = m_sramCorrectionAcc[imagePixel];
          auto & pixelStatsAcc = m_dataStatsAcc[imagePixel];
          for(uint sram=minSram; sram<=maxSram; sram++){
            pixelStatsAcc += pixelCorrectAcc[sram];
          }
        }
      }
    }


    void DsscDataReceiver::saveToHDFviaThread()
    {
      // can only work if saveToHDF returns storage to free storage
      //m_writeHDFThreads.push_back(boost::thread(boost::bind(&DsscDataReceiver::saveToHDF, this, currentTrainData)));
    }

    // display function
    void DsscDataReceiver::updateSpecificData(const utils::DsscTrainData * trainData)
    {
      const auto specific = trainData->getSpecificData();
      set<unsigned short>("specificData.pulseCnt",trainData->pulseCnt);
      set<unsigned short>("specificData.pptVetoCnt",specific.pptVetoCnt);
      set<unsigned short>("specificData.numPreBursVetos",specific.numPreBursVetos);
      set<unsigned short>("specificData.userSpecific1",specific.userSpecific1);
      set<unsigned short>("specificData.userSpecific2",specific.userSpecific2);
      set<unsigned short>("specificData.userSpecific3",specific.userSpecific3);
      set<unsigned short>("specificData.moduleNr",specific.moduleNr);
      set<unsigned int  >("specificData.iobSerial",specific.iobSerial);
      set<bool>("specificData.sort_asic_wise",specific.sort_asic_wise);
      set<bool>("specificData.rotate_ladder",specific.rotate_ladder);
      set<bool>("specificData.send_dummy_dr_data",specific.send_dummy_dr_data);
      set<string>("specificData.dataFormat",m_trainSorter.getInputFormat());
    }


    void DsscDataReceiver::updateTempADCValues(utils::DsscTrainData * trainData)
    {
      for(int asic=0; asic<16; asic++){
        set<unsigned short>("asicADCTemperatue.tempASIC"+to_string(asic),trainData->getTempADCValue(asic));
      }
    }

    void DsscDataReceiver::saveToHDF(utils::DsscTrainData * trainDataToStore)
    {
      KARABO_LOG_INFO << "Save to HDF5 - TrainID = " << trainDataToStore->trainId;

#ifdef HDF5
      string fileName =  utils::getLocalTimeStr() + "_TrainData_" + std::to_string(trainDataToStore->trainId) +".h5";
      DsscHDF5Writer::saveToFile(get<string>("outputDir") + "/" + fileName,trainDataToStore,64,64);
#else
      KARABO_LOG_INFO << "HDF5 not installed";
#endif
      //m_trainSorter.addFreeStorage(trainDataToStore);
    }

    std::string DsscDataReceiver::getDir(const std::string & name)
    {
      const auto it = directoryStructure.find(name);
      if(it != directoryStructure.cend()){
        return it->second;
      }

      std::cout << "unknown parameter name " << name << std::endl;
      exit(0);
      return "";
    }




  void DsscDataReceiver::fillSpecificData()
  {
  //  sendData.set(getDir("pCellId"), packetVector.getCellIds());
  //  sendData.set(getDir("pCellId"), packetVector.getCellIds());
  //  sendData.set(getDir("pLength"), std::vector<unsigned int>(numFrames,0x200));
    sendData.set(getDir("pPulseId"), currentTrainData->pulseIds);
  //  sendData.set(getDir("pStatus"), std::vector<unsigned int>(numFrames,0));
  //  sendData.set(getDir("ptrainId"), std::vector<unsigned int>(numFrames,trainId));

  //  sendData.set(getDir("asicTrailer"), currentTrainData->asicTrailerData);
  //  sendData.set(getDir("pptData"),     currentTrainData->pptData);
  //  sendData.set(getDir("sibData"),     currentTrainData->sibData);

  //  fillData(getHeaderData());
  //  fillData(getTrailerData());
  }


  void DsscDataReceiver::fillHeader()
  {
    sendData.set<unsigned long long>(getDir("trainId"),          currentTrainData->trainId);
    sendData.set<unsigned long long>(getDir("dataId"),           currentTrainData->dataId);
    sendData.set<unsigned long long>(getDir("imageCount"),       currentTrainData->pulseCnt);
    sendData.set<unsigned int>(getDir("tbLinkId"),               currentTrainData->tbLinkId);
    sendData.set<unsigned int>(getDir("femLinkId"),              currentTrainData->detLinkId);
    sendData.set<unsigned int>(getDir("detSpecificLength"),      currentTrainData->detSpecificLength);
    sendData.set<unsigned int>(getDir("tbSpecificLength"),       currentTrainData->tbSpecificLength);
  }


  void DsscDataReceiver::fillTrailer()
  {
 //   sendData.set<unsigned long long>(getDir("checkSum0"),      trailer.checkSum0);
 //   sendData.set<unsigned long long>(getDir("checkSum1"),      trailer.checkSum1);
 //   sendData.set<unsigned long long>(getDir("status"),         trailer.status);
 //   sendData.set<unsigned long long>(getDir("magicNumberEnd"), trailer.magicNumber);
  }


  void DsscDataReceiver::initStatsAcc()
  {
    m_dataStatsAcc.assign(utils::s_totalNumPxs,utils::StatsAcc());
    m_sramCorrectionAcc.assign(utils::s_totalNumPxs,utils::StatsAccVec(utils::s_numSram));
    m_pixelCorrectionData.assign(utils::s_totalNumPxs,std::vector<float>(utils::s_numSram));

    m_pixelHistoVec.assign(utils::s_totalNumPxs,utils::DataHisto());
  }


  void DsscDataReceiver::updatePixelHistos()
  {
    //KARABO_LOG_INFO << "Update Pixel Histos";
    m_processor.fillDataHistoVec(currentTrainData,m_pixelHistoVec,false);
    //cout << "Pixel data to histogram vector added" << endl;
  }


  void DsscDataReceiver::savePixelHistos()
  {
    static const auto pixelNumbers = utils::getUpCountingVector<unsigned int>(utils::s_totalNumPxs);

    utils::fillBufferToDataHistoVec(m_pixelHistoVec);

    const string fileName = get<string>("outputDir") + "/PixelHistogramExport.dat";
    const string h5FileName = get<string>("outputDir") + "/PixelHistogramExport.h5";
    utils::DataHisto::dumpHistogramsToASCII(fileName,pixelNumbers,m_pixelHistoVec);

    DsscHDF5TrimmingDataWriter dataWriter(h5FileName);
    dataWriter.setMeasurementName("LadderSpectrum");
    dataWriter.addHistoData("SpektrumData",m_pixelHistoVec,pixelNumbers);

    const auto imageValues = utils::calcMeanImageFromHistograms(m_pixelHistoVec,pixelNumbers);
    dataWriter.addImageData("SpectrumPedestalImage",512,imageValues);

    KARABO_LOG_INFO << "Stored Pixel Histograms to " << h5FileName;
  }

  void DsscDataReceiver::displayPixelHistogram()
  {
    vector<unsigned short> bins;
    vector<unsigned int> binValues;
    size_t histoValueCount = 0;
    if(get<bool>("showOnlineHistogram")){
      auto imagePixel = get<unsigned int>("monitor.ladderPixelToShow");
      m_pixelHistoVec[imagePixel].fillBufferToHistoMap();
      m_pixelHistoVec[imagePixel].getDrawValues(bins,binValues);
      histoValueCount = m_pixelHistoVec[imagePixel].getCount();
    }else{
      pixelHisto.fillBufferToHistoMap();
      pixelHisto.getDrawValues(bins,binValues);
      histoValueCount = pixelHisto.getCount();
    }

    if(!bins.empty()){
      set<unsigned long long>("histoGen.pixelhistoCnt",histoValueCount);
      set<vector<unsigned short>>("histoGen.pixelHistoBins",std::move(bins));
      if(get<bool>("displayHistoLogscale")){
        std::transform(binValues.begin(),binValues.end(),binValues.begin(),[](int x){if(x==0) return 1; return x;});
      }
      set<vector<unsigned int>>("histoGen.pixelHistoBinValues",std::move(binValues));
    }

    if(get<bool>("histoGen.constantReset")){
      pixelHisto.clear();
    }
  }

  void DsscDataReceiver::displayAsicHistogram()
  {
    vector<unsigned short> bins;
    vector<unsigned int> binValues;

    asicHisto.fillBufferToHistoMap();
    asicHisto.getDrawValues(bins,binValues);
    set<vector<unsigned short>>("histoGen.asicHistoBins",std::move(bins));
    set<vector<unsigned int>>("histoGen.asicHistoBinValues",std::move(binValues));

    if(get<bool>("histoGen.constantReset")){
      asicHisto.clear();
    }
  }

  void DsscDataReceiver::clearPixelHistos()
  {
    for(auto && pixelHisto : m_pixelHistoVec){
      pixelHisto.clear();
    }
  }

  void DsscDataReceiver::startDisplay()
  {
    m_runDisplay = true;

    if(m_displayThread){
      KARABO_LOG_INFO << "Old display thread to join in write()!";
      m_displayThread->join();
      m_displayThread.reset();
    }
    m_displayThread.reset(new boost::thread(boost::bind(&Self::displayData, this)));
  }

  void DsscDataReceiver::stopDisplay()
  {
    m_runDisplay = false;

    if(m_displayThread){
      KARABO_LOG_INFO << "Old display thread to join in write()!";
      m_displayThread->join();
      m_displayThread.reset();
    }
    m_displayThread = nullptr;
  }


  void DsscDataReceiver::displayData()
  {
    m_numCheckedData = 0;
    m_errorCnt = 0;

    TrainDataRingStorage * trainDataRingStorage = ((TrainDataRingStorage*)&m_trainSorter);
    while(m_runDisplay)
    {
      m_trainDataToShow = trainDataRingStorage->getNextValidStorage();
      if(m_trainDataToShow == nullptr) return;

      if(m_trainDataToShow->isValid())
      {
        if(m_nextHistoReset){
          clearHistograms();
          m_nextHistoReset = false;
        }

        if(m_nextThresholdReset){
          clearThresholdMap();
          m_nextThresholdReset = false;
        }

        if(m_isStopped){
          set<unsigned long long>("currentTrainId",m_trainDataToShow->trainId);
          auto trainCnt = get<unsigned long long>("trainCnt") + 1;
          set<unsigned long long>("trainCnt",trainCnt);
        }

        updateTempADCValues(m_trainDataToShow);

        uint updateFrequency = get<unsigned int>("updateFrequency");
        if((m_trainDataToShow->trainId%updateFrequency) == 0)
        {
          updateSpecificData(m_trainDataToShow);

          if(get<bool>("enableLadderPreview") ){
            writeChannel("ladderImageOutput",getImageData(m_selFrame));
          }

          if(get<bool>("enableASICPreview"))
          {
            writeChannel("asicImageOutput",getImageData(m_selFrame,m_selAsic));

            displayAsicHistogram();
          }

          if(get<bool>("checkTestPatternData")){
            checkTestPatternData(m_trainDataToShow);
          }else{
            m_numCheckedData = 0;
            m_errorCnt = 0;
          }
        }

        if(get<bool>("enablePixelPreview"))
        {
          writeChannel("pixelImageOutput",getPixelData(m_asicPixelToShow, m_asicToShow));

          displayPixelHistogram();
        }
      }

      if(get<bool>("saveImageWiseSorted")){
        currentTrainData = m_trainDataToShow;
        processCurrentTrainData();
      }

      if(m_isStopped){
        trainDataRingStorage->addFreeStorage(m_trainDataToShow);
      }else{
        // put back that it can also be stored
        trainDataRingStorage->addValidStorage(m_trainDataToShow);
      }
      m_trainDataToShow = nullptr;
    }
  }


  void DsscDataReceiver::receiveData()
  {
    cout << "RUN receiveData" << endl;

    bool keepReceiving = get<bool>("keepReceiving");

    int errorCnt = 0;
    string errorMsg = "";

    m_trainSorter.startSortThread();
    m_isStopped = false;

    try
    {
      do
      {
        // in image wise mode this function should directly return
        // data is hanled in display thread
        if(get<bool>("saveImageWiseSorted")){
          m_isStopped = true;
          m_trainSorter.stopSortThread();
        }

        // If user pressed stop, we stop any writing
        if (m_isStopped) {
          keepReceiving = false;
          break;
        }

        currentTrainData = m_trainSorter.getNextTrainData();
        if(currentTrainData == nullptr) break;

        if(!currentTrainData->isValid()){
          KARABO_LOG_ERROR << "Current TRAIN Data is invalid";
          m_trainSorter.returnTrainData(currentTrainData);
          currentTrainData = nullptr;
          continue;
        }

        processCurrentTrainData();

        m_trainSorter.returnTrainData(currentTrainData);
        currentTrainData = nullptr;
      }while(keepReceiving);

    } catch (const Exception &e) {
      errorMsg =  ":\n" + e.detailedMsg();
    } catch (const std::exception &eStd) {
      errorMsg =  ":\n" + string(eStd.what());
    } catch (...) {
      errorMsg =  " unknown exception";
    }


    if(!errorMsg.empty()){
      KARABO_LOG_ERROR << "Stop writing because:" << errorMsg;
      this->signalEndOfStream("imageOutput");
      this->signalEndOfStream("ladderImageOutput");
      this->signalEndOfStream("asicImageOutput");
      this->signalEndOfStream("pixelImageOutput");
      this->updateState(State::ERROR);
      exit(0);
    }else{
      KARABO_LOG_INFO << get<unsigned long long>("trainCnt") << " trains correctly received. " << errorCnt << " Errors found.";
      this->updateState(State::STARTED);
    }
  }


  void DsscDataReceiver::processCurrentTrainData()
  {
    m_currentTrainID = currentTrainData->trainId;

    checkTestPattern(currentTrainData);

    set<unsigned long long>("currentTrainId",m_currentTrainID);
    auto trainCnt = get<unsigned long long>("trainCnt");
    set<unsigned long long>("trainCnt",trainCnt+1);

    //fillMetaData();
    //writeChannel("imageOutput",sendData);

    // dont send data if not all ASICs have valid test pattern (little protection)
    bool allAsicsDataValid = allASICTestPatternsValid(currentTrainData);

    if(!allASICTempValuesValid(currentTrainData)){
      utils::CoutColorKeeper keeper(utils::STDRED);
      KARABO_LOG_WARN << "ASIC TEMP VALUES PROBLEM... Check if this trains have to be removed";
    }

    updateProcessorParams();

    // save only every nth frame: return data directly
    if(get<bool>("enableDataReduction")){
      if((m_currentTrainID % get<unsigned>("storeNthFrame") )!= 0){
        return;
      }
    }

    if(allAsicsDataValid)
    {
      if(get<bool>("enableDAQOutput")){ // send pixelWise data
        writeChannel("monitorOutput",getMonitorData());
      }

      if(get<bool>("saveToHDF5") && !get<bool>("saveJustSpectra")){
        saveToHDF(currentTrainData);
      }

      if(get<bool>("acquiring"))
      {
        if(m_genTrainStats){
          updateCorrectionMap();
        }

        if(get<bool>("storeHistograms")){
          updatePixelHistos();
        }

        uint32_t numTrainsToStore = get<unsigned int>("numTrainsToStore");
        uint32_t numStoredTrains = get<unsigned int>("numStoredTrains") + 1;
        set<unsigned int>("numStoredTrains",numStoredTrains);

        if(numStoredTrains >= numTrainsToStore){
          finishAcquisition();
        }
      }

      m_availableASICs = currentTrainData->availableASICs;
    }else{
      KARABO_LOG_WARN << "not all ASICs sent correct data, or test pattern is wrong, dod not send any data";
    }
  }

  void DsscDataReceiver::finishAcquisition()
  {
    uint numStoredTrains = get<unsigned int>("numStoredTrains");

    if(m_genTrainStats)
    {
      computeMeanBursts();
      utils::MeanRMSVectors meanRMSValuesVec = DsscHDF5Writer::saveToFile(get<string>("outputDir") + "/TrainMeanRMSImage.h5",m_dataStatsAcc,currentTrainData->availableASICs,numStoredTrains);

      if(m_acquireBG){
        m_pixelBackgroundData = std::move(meanRMSValuesVec.meanValues);
        computeSramCorrectionData();

        DsscHDF5Writer::saveBaselineAndSramCorrection(get<string>("outputDir") + "/SRAMCorrectionImage.h5",m_pixelBackgroundData,m_pixelCorrectionData,currentTrainData->availableASICs,numStoredTrains);

        set<bool>("correction.bgDataValid",true);
        set<bool>("correction.sramCorrectionValid",true);
        m_bgDataValid = true;
        m_acquireBG     = false;
      }

      m_genTrainStats = get<bool>("storeMeanAndRMS");
      set<vector<float>>("measOutput.meanValuesVec",std::move(meanRMSValuesVec.meanValues));
      set<vector<float>>("measOutput.rmsValuesVec",std::move(meanRMSValuesVec.rmsValues));
      set<unsigned long long>("measOutput.numValues",numStoredTrains*800);
    }

    if(get<bool>("storeHistograms")){
      savePixelHistos();
    }

    set<bool>("acquiring",false);
    set<bool>("saveToHDF5",false);
    if(lastState == State::STARTED){
      m_isStopped = true;
    }

    this->updateState(lastState);

    KARABO_LOG_INFO << "All Trains stored: " << numStoredTrains;
    set<unsigned int>("numStoredTrains",0);
  }


}
