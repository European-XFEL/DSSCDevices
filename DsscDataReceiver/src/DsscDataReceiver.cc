/*
 * $Id$
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

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#define MAINOFFS 10
using namespace std;

USING_KARABO_NAMESPACES
        
namespace karabo {  
  
    std::vector<int> DsscDataReceiver::sortMap;  
          
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
              .assignmentOptional().defaultValue(8000).reconfigurable()
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
                .displayedName("Num Images To Receive")
                .description("Number of images that are expected from the camera")
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

      BOOL_ELEMENT(expected).key("storeHistograms")
                .displayedName("Store Histograms in ASCII")
                .description("Store histogram ascii files per measurement directory")
                .tags("receiver")           
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
                .description("Shows if background and correction data is available")
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
                .displayedName("Subtract Backgroung")
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
                .key("acquireTrains").displayedName("AcquireTrains").description("Store selected number of trains in selected directory")
                .allowedStates(State::STARTED,State::ACQUIRING)  
                .commit();  

      SLOT_ELEMENT(expected)
                .key("flushTrainStorage").displayedName("FlushStorage").description("Flush received packets and sorted trains")
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
       
      BOOL_ELEMENT(expected).key("LadderPreview")
                .displayedName("En Ladder Preview")
                .description("Enable update of ladder preview")
                .assignmentOptional().defaultValue(true)
                .reconfigurable()              
                .commit();
            
      BOOL_ELEMENT(expected).key("ASICPreview")
                .displayedName("En ASCI Preview")
                .description("Enable update of ASIC preview")
                .assignmentOptional().defaultValue(true)
                .reconfigurable()              
                .commit();
      
      BOOL_ELEMENT(expected).key("PixelPreview")
                .displayedName("En Pixel Preview")
                .description("Enable update of pixel preview")
                .assignmentOptional().defaultValue(true)
                .reconfigurable()              
                .commit();
      
      BOOL_ELEMENT(expected).key("saveToHDF5")
                .displayedName("Save Images")
                .description("Save Images as HDF5")
                .assignmentOptional().defaultValue(false)
                .reconfigurable()              
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
      
      //only used for middle layer device
      KARABO_SLOT(acquireTrains);      
      KARABO_SLOT(flushTrainStorage);  
      
      KARABO_SLOT(acquireBackgroundAndCorrection);
      KARABO_SLOT(loadBackgroundAndCorrection);
      KARABO_SLOT(resetHistograms);
      KARABO_SLOT(clearThresholdMap);  
      
      KARABO_SLOT(startDataTransfer);
      
    }


    DsscDataReceiver::~DsscDataReceiver() 
    {
      close();
    }
    
    
    void DsscDataReceiver::initialize() 
    {
      this->updateState(State::OFF);
      m_selAsic = get<unsigned int>("asicToShow");
      m_selFrame = get<unsigned int>("displayFrame");
      m_ladderMode = get<bool>("ladderMode");
     

      KARABO_LOG_INFO << "DsscDataReceiver: LadderMode = " << m_ladderMode;
      m_asicPixelToShow = get<unsigned int>("monitor.asicPixelToShow");
      m_asicToShow = get<unsigned int>("monitor.asicToShow"); 
      set<unsigned int>("monitor.ladderPixelToShow",calcImagePixel(m_asicToShow,m_asicPixelToShow));
      m_keepPolling = false;
      
      set<bool>("correction.bgDataValid",false);
      set<bool>("correction.showRawData",true);
      
      m_correct      = get<bool>("correction.correct");
      m_subtract     = get<bool>("correction.subtract");
      m_enThreshold = get<bool>("correction.enThreshold");
      m_threshold   = get<unsigned int>("correction.threshold");
      m_bgDataValid  = get<bool>("correction.bgDataValid");
      m_showRawData  = get<bool>("correction.showRawData");
      m_gccwrap      = get<unsigned short>("correction.gccwrap");
      
      m_acquireBG = false;

      m_correctionFunction = getCorrectionFunction();      
      
      initSortMap();
      initStatsAcc();
      
      clearHistograms();
      clearThresholdMap();

      DsscHDF5Writer::enHDF5Compression = get<bool>("enHDF5Compression");
    }
    
    
    void DsscDataReceiver::initSortMap()
    {
      const int numASICs = m_ladderMode ? 16 : 1;
      const int numcols  = m_ladderMode ? 512 : 64;
      
      sortMap.resize(numASICs*4096);
      int idx = 0;
      for(int asic = 0; asic < numASICs; asic++)
      {
        int rowOffs = (asic>7)? 64 : 0;
        int colOffs = (asic%8)*64;
        for(int row = 0; row<64; row++)
        {
          for(int col =0; col < 64; col++)
          {
            sortMap[idx++] = (row+rowOffs)*numcols + col + colOffs;
          }
        }
      }
      
      KARABO_LOG_INFO << "Sort map initialized";
    }
    
    
    void DsscDataReceiver::clearHistograms()
    {
      pixelHisto.clear();
      asicHisto.clear();      
    }

    
    void DsscDataReceiver::clearThresholdMap()
    {         
      if(m_ladderMode){
        m_thresholdMap.assign(16*4096,0.0);
      }else{
        m_thresholdMap.assign(4096,0.0);
      } 
    }   
    
    
    void DsscDataReceiver::startPolling()
    {
      m_keepPolling = true;
      m_pollThread.reset(new boost::thread(boost::bind(&DsscDataReceiver::getFillStands, this)));
      KARABO_LOG_INFO << "PollThread started...";
    }
    
    
    void DsscDataReceiver::stopPolling()
    {
      m_keepPolling = false;
      if (m_pollThread) {
          m_pollThread->join();
          m_pollThread.reset();
      }
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
      if(this->getState() == State::OFF || this->getState() == State::UNKNOWN){
        this->open();
      }
      
      if(this->getState() == State::STARTED){
        this->activate();
      } 
      
      initStatsAcc();
    }
    
    
    void DsscDataReceiver::stopDataReceiver()
    {      
      KARABO_LOG_DEBUG << "++++ Stop DataReceiver ++++";
      if(m_trainSorter){
        m_trainSorter->stopThread();
        delete m_trainSorter;        
      }
      m_trainSorter = nullptr;
    } 
    
    
    void DsscDataReceiver::stop()
    {      
      if(m_isStopped){
        this->updateState(State::STARTED);
        return;
      }
      
      this->updateState(State::CHANGING);      
      m_isStopped = true;
      
      if (m_writingThread) {
          KARABO_LOG_DEBUG << "Need to join writing thread in destructor!";
          m_writingThread->join();
          m_writingThread.reset();
      }
      
      KARABO_LOG_DEBUG << "Stop m_writeHDFThreads!";

      for(auto & th : m_writeHDFThreads){
        if(th.joinable()){
          th.join();
        }
      }
      KARABO_LOG_DEBUG << "Stopped m_writeHDFThreads!";
      
      this->updateState(State::STARTED);
    }
    
    
    void DsscDataReceiver::restart()
    {
      restartLadderMode(get<bool>("ladderMode"));
    }
    
    
    void DsscDataReceiver::acquireBackgroundAndCorrection()
    {
      m_acquireBG = true;
      m_bgDataValid = false;
      set<bool>("correction.bgDataValid",m_bgDataValid);         
      
//      const string outputDirRem = get<string>("outputDir");      
//      set<string>("outputDir",".");
      
      acquireTrains();
      
      while(get<bool>("acquiring")){
        boost::this_thread::sleep(boost::posix_time::seconds(1));
      }
//      set<string>("outputDir",outputDirRem);
      m_acquireBG = false;
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
      }
      
      set<bool>("correction.bgDataValid",m_bgDataValid);        
    }

 
    void DsscDataReceiver::restartLadderMode(bool ladderMode)
    {
      State stateRem = this->getState();
      if(stateRem == State::UNKNOWN) return;      
      // stop data acquisition before chaning ladder mode
      if(stateRem == State::ACQUIRING){
        this->stop();
      }
      
      if(this->getState() == State::STARTED){
        this->close();
      }
      
      // change ladder mode and initialize buffers
      m_ladderMode = ladderMode;
      initSortMap();
      initStatsAcc();
      
      if(stateRem == State::OFF) return;      
      this->open(); // will reassign train buffers
      if(stateRem == State::STARTED) return;
      this->activate();      
    }
    
    
    void DsscDataReceiver::restartSelASIC()
    {
      if(!m_trainSorter) return;
    
      State stateRem = this->getState();
      if(m_ladderMode){
        m_trainSorter->setActiveASIC(m_selAsic);
      }else{
        if(stateRem == State::ACQUIRING){
          this->stop();
        }
        m_trainSorter->setActiveASIC(m_selAsic);
         if(stateRem == State::ACQUIRING){
          this->activate();
        }         
      }
    }

    
    void DsscDataReceiver::flushTrainStorage()
    {
      KARABO_LOG_DEBUG << "++++ Dismiss Valid Trains ++++";
      m_trainSorter->reStartSort();
    }

 
    void DsscDataReceiver::getFillStands()
    {
      while(m_keepPolling) {
        auto recvStand = m_trainSorter->getFillStand(DsscTrainSorter::Recv);
        auto sortStand = m_trainSorter->getFillStand(DsscTrainSorter::Sort);
        auto lostStand = m_trainSorter->getFillStand(DsscTrainSorter::Lost);

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
            restartSelASIC();
            initPixelNumbers();
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
            set<unsigned int>("monitor.ladderPixelToShow",calcImagePixel(m_asicToShow,m_asicPixelToShow));
            pixelHisto.clear();
          }else if(path.compare("monitor.asicToShow") == 0){
            m_asicToShow = filtered.getAs<unsigned int>(path);
            set<unsigned int>("monitor.ladderPixelToShow",calcImagePixel(m_asicToShow,m_asicPixelToShow));
          }else if(path.compare("monitor.ladderPixelToShow") == 0){
            auto ladderPixel = filtered.getAs<unsigned int>(path);
            m_asicPixelToShow = calcASICPixel(ladderPixel);
            m_asicToShow = calcPixelASIC(ladderPixel);
            set<unsigned int>("prev_asicPixelToShow",m_asicPixelToShow);
            set<unsigned int>("prev_asicToShow",m_asicToShow);
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
            }            
          }else if(path.compare("correction.subtract") == 0){
            m_subtract = filtered.getAs<bool>(path);
            
            if(!m_bgDataValid){
              KARABO_LOG_WARN << "Background can only be active if background data is available";
            }            
          }else if(path.compare("correction.showRawData") == 0){
            m_showRawData = filtered.getAs<bool>(path);
          }
        }        
        m_correctionFunction = getCorrectionFunction();     
        KARABO_LOG_INFO << "Correction Function Changed";
      }
    }  
    

    
        
    void DsscDataReceiver::open()
    {
      this->updateState(State::STARTING);

      KARABO_LOG_DEBUG << "Open";
      
      set<unsigned long long>("trainCnt",0);      
      const unsigned int numFrames = get<unsigned int>("numFramesToReceive"); 
      
      initStatsAcc();
      
      m_pixelData.resize(numFrames);
      
      KARABO_LOG_INFO << "DsscDataReceiver2: LadderMode = " << m_ladderMode;
      m_trainSorter = new DsscTrainSorter(10,numFrames,m_ladderMode,false);
      m_trainSorter->setActiveASIC(m_selAsic);
      m_trainSorter->startSort();
      
      startPolling();
      
      this->updateState(State::STARTED); 
    }

    
    void DsscDataReceiver::close()
    {
      this->updateState(State::CLOSING);

      KARABO_LOG_DEBUG << "Close Action";
      
      stop();      
      
      stopDataReceiver();  
      
      stopPolling();      
      
      this->updateState(State::OFF);
    }
        
    
    void DsscDataReceiver::reset()
    {            
      KARABO_LOG_DEBUG << "Reset Action";
      set<unsigned long long>("trainCnt",0);
      close();
    }  
    
    
    void DsscDataReceiver::activate()
    {
      this->updateState(State::CHANGING);

      KARABO_LOG_DEBUG << "Activate Action";  
      // There might be a remnant (but finished) thread from previous write
      if (m_writingThread) {
        KARABO_LOG_DEBUG << "Old writing thread to join in write()!";
        m_writingThread->join();
        m_writingThread.reset();
      }        
      
      m_isStopped = false; 
      
      m_trainSorter->startSort();
        
      m_writingThread.reset(new boost::thread(boost::bind(&Self::receiveData, this)));
     
      this->updateState(State::ACQUIRING);
    }  
    
    
    void  DsscDataReceiver::receiveData()
    {
      bool keepReceiving = get<bool>("keepReceiving");      
      
      int errorCnt = 0;
      string errorMsg = "";
      
      try
      {
        do
        {          
          // If user pressed stop, we stop any writing
          if (m_isStopped) {
              keepReceiving = false;
              break;
          }            
          
          if(m_nextHistoReset){
            clearHistograms();
            m_nextHistoReset = false;
          }          
                    
          if(m_nextThresholdReset){   
            clearThresholdMap();
            m_nextThresholdReset = false;
          }          

          currentTrainData = m_trainSorter->getNextValidStorage();
                        
          fillMetaData();          
            
          set<unsigned long long>("currentTrainId",currentTrainData->trainId);      
          auto trainCnt = get<unsigned long long>("trainCnt");
          set<unsigned long long>("trainCnt",trainCnt+1);               
            
          //writeChannel("imageOutput",sendData);
          
          if(get<bool>("LadderPreview") ){
            writeChannel("ladderImageOutput",getImageData(m_selFrame));
          }
          
          if(get<bool>("ASICPreview")){
            if(get<bool>("histoGen.constantReset")){
              asicHisto.clear();
            }
            writeChannel("asicImageOutput",getImageData(m_selFrame,m_selAsic)); 
            vector<unsigned short> bins;
            vector<unsigned int> binValues;
            asicHisto.getDrawValues(bins,binValues);            
            set<vector<unsigned short>>("histoGen.asicHistoBins",std::move(bins));
            set<vector<unsigned int>>("histoGen.asicHistoBinValues",std::move(binValues));            
          }
          
          if(get<bool>("PixelPreview")){
            if(get<bool>("histoGen.constantReset")){
              pixelHisto.clear();
            }            
            writeChannel("pixelImageOutput",getPixelData(m_asicPixelToShow, m_asicToShow));
            
            vector<unsigned short> bins;
            vector<unsigned int> binValues;
            pixelHisto.getDrawValues(bins,binValues);
            set<vector<unsigned short>>("histoGen.pixelHistoBins",std::move(bins));
            set<vector<unsigned int>>("histoGen.pixelHistoBinValues",std::move(binValues));
          }   
              
          if(get<bool>("saveToHDF5")){
            saveToHDF(currentTrainData);            
          }

          updateAcquireCnt();          

          m_trainSorter->addFreeStorage(currentTrainData);

        }while(keepReceiving);

      } catch (const Exception &e) {
        errorMsg =  ":\n" + e.detailedMsg();
      } catch (const std::exception &eStd) {
        errorMsg =  ":\n" + string(eStd.what());
      } catch (...) {
        errorMsg =  " unknown exception";              
      }
      
      
      if(!errorMsg.empty()){
        KARABO_LOG_ERROR << "Stop writing since" << errorMsg;
        this->signalEndOfStream("imageOutput");        
        this->signalEndOfStream("ladderImageOutput");        
        this->signalEndOfStream("asicImageOutput");        
        this->signalEndOfStream("pixelImageOutput");        
        this->updateState(State::ERROR);
      }else{
        KARABO_LOG_INFO << get<unsigned long long>("trainCnt") << " trains correctly received. " << errorCnt << " Errors found.";
        this->updateState(State::STARTED);
      }           
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
        
    
    util::Hash DsscDataReceiver::getImageData(int frameNum)
    { 
      int numCols = m_ladderMode? 512 : 64;
      int numRows = m_ladderMode? 128 : 64;
      int numPixels = numCols * numRows;
 
      static vector<unsigned int> imageData(numPixels,0);
      
      const uint16_t * frameData = currentTrainData->imageData(frameNum);     
      
      util::Hash dataHash;

      
      if(m_showThreshold){
        for(int pxIdx=0; pxIdx < numPixels; pxIdx++){
          m_thresholdMap[sortMap[pxIdx]] += m_correctionFunction(frameNum,pxIdx,frameData[pxIdx]);
        }
        
        NDArray imageArray(m_thresholdMap.data(),
                         numPixels,
                         NDArray::NullDeleter(),
                         Dims(numRows,numCols));      
        dataHash.set("ladderImage",ImageData(imageArray));           
      }else{      
        for(int pxIdx=0; pxIdx < numPixels; pxIdx++){
          imageData[sortMap[pxIdx]] = m_correctionFunction(frameNum,pxIdx,frameData[pxIdx]);
        }
        
        NDArray imageArray(imageData.data(),
                         numPixels,
                         NDArray::NullDeleter(),
                         Dims(numRows,numCols));      
        dataHash.set("ladderImage",ImageData(imageArray));        
      }      

      return dataHash;
    }
    
    
    util::Hash DsscDataReceiver::getImageData(int frameNum, int asic)
    {
      if(!m_ladderMode){
        asic = 0;
      }
      
      static vector<unsigned int> imageData(4096,0);
      
      const uint16_t * asicFrameData = currentTrainData->imageData(frameNum,asic);
      util::Hash dataHash;
      int asicOffset = asic * 4096;

      if(m_showThreshold){
        asicHisto.clear();
        for(int pxIdx=0; pxIdx < 4096; pxIdx++){
          auto & value = imageData[pxIdx];
          value = m_thresholdMap[asicOffset+pxIdx];
          asicHisto.add(value);
        }

        NDArray imageArray(m_thresholdMap.data(),
                           4096,
                           NDArray::NullDeleter(),
                           Dims(64,64));
        dataHash.set("asicImage",ImageData(imageArray));        
        
      }else{
        for(int pxIdx=0; pxIdx < 4096; pxIdx++){
          auto & value = imageData[pxIdx];
          value = m_correctionFunction(frameNum,asicOffset+pxIdx,asicFrameData[pxIdx]);
          asicHisto.add(value);
        }

        NDArray imageArray(imageData.data(),
                           4096,
                           NDArray::NullDeleter(),
                           Dims(64,64));
        dataHash.set("asicImage",ImageData(imageArray));
      }
      return dataHash;
    }
    
    
    util::Hash DsscDataReceiver::getPixelData(int asicPixel, int asic)
    {
      if(!m_ladderMode){
        asic = 0;
      }
            
      int dataIdx = asic*4096+asicPixel;
      const auto numFrames = currentTrainData->pulseCnt;
      m_pixelData.resize(numFrames);   

      if(get<bool>("correction.showCorrect") && m_bgDataValid){
        auto & selPixelcorrData = m_pixelCorrectionData[dataIdx];
        m_pixelData.assign(selPixelcorrData.begin(),selPixelcorrData.end());
      }else{
        for(int frame = 0; frame < numFrames; frame++){
          auto & value = m_pixelData[frame];
          value = m_correctionFunction(frame,dataIdx,currentTrainData->imageData(frame)[dataIdx]);
          pixelHisto.add(value);
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
      
      set<bool>("acquiring",true);
      set<unsigned int>("numStoredTrains",0);
      set<bool>("saveToHDF5",true);
      lastState = this->getState();
      
      start();
      updateState(State::ACTIVE);
    }    

        
    void DsscDataReceiver::updateAcquireCnt()
    {
      if(!m_acquireBG){
        if(!get<bool>("acquiring")) return;
      }
      
      updateStatsAcc();

      if(get<bool>("storeHistograms")){
        updatePixelHistos();
      }
      
      uint32_t numStoredTrains = get<unsigned int>("numStoredTrains");
      set<unsigned int>("numStoredTrains",numStoredTrains+1);
      
      if(get<unsigned int>("numTrainsToStore") == numStoredTrains)
      { 
        auto meanRMSValuesVec = DsscHDF5Writer::saveToFile(get<string>("outputDir") + "/TrainMeanRMSImage.h5",m_dataStatsAcc,currentTrainData->availableASICs,numStoredTrains); 
        
        if(m_acquireBG){
          m_pixelBackgroundData = std::move(meanRMSValuesVec.meanValues);
          m_pixelCorrectionData = DsscHDF5Writer::saveToFile(get<string>("outputDir") + "/SRAMCorrectionImage.h5",m_pixelBackgroundData,m_sramCorrectionAcc,currentTrainData->availableASICs,numStoredTrains); 
          set<bool>("correction.bgDataValid",true);
          m_bgDataValid = true;
        }   

        if(get<bool>("storeHistograms")){
          savePixelHistos();
        }
        
        set<vector<float>>("measOutput.meanValuesVec",std::move(meanRMSValuesVec.meanValues));
        set<vector<float>>("measOutput.rmsValuesVec",std::move(meanRMSValuesVec.rmsValues));
        set<unsigned long long>("measOutput.numValues",numStoredTrains*800);
        
        
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

    
    void DsscDataReceiver::updateCorrectionMap()
    {
      const int numPixels = m_dataStatsAcc.size();
      const int numFrames = currentTrainData->pulseCnt;
      
      for(int frame=0; frame<numFrames; frame++){
        auto frameData = currentTrainData->imageData(frame);
        for(int px = 0; px<numPixels; px++){
          m_sramCorrectionAcc[px][frame] += frameData[px];
        }
      }          
    }

    
    void DsscDataReceiver::updateStatsAcc()
    {
      const int numPixels = m_dataStatsAcc.size();
      const int numFrames = currentTrainData->pulseCnt;
      
      for(int frame=0; frame<numFrames; frame++){
        auto frameData = currentTrainData->imageData(frame);
        for(int px = 0; px<numPixels; px++){
          m_dataStatsAcc[px].addValue(frameData[px]);
        }
      }    
      if(m_acquireBG){
        updateCorrectionMap();
      }
    }


    void DsscDataReceiver::updatePixelHistos()
    {
      const int numPixels = m_pixelHistoVec.size();
      const int numFrames = currentTrainData->pulseCnt;
      
      for(int frame=0; frame<numFrames; frame++){
        auto frameData = currentTrainData->imageData(frame);
        for(int px = 0; px<numPixels; px++){
          m_pixelHistoVec[px].add(frameData[px]);
        }
      }    
    }

    
    void DsscDataReceiver::saveToHDFviaThread()
    {  
      // can only work if saveToHDF returns storage to free storage    
      //m_writeHDFThreads.push_back(boost::thread(boost::bind(&DsscDataReceiver::saveToHDF, this, currentTrainData)));
    }
 
    
    void DsscDataReceiver::saveToHDF(DsscTrainData * trainDataToStore)
    {
      KARABO_LOG_INFO << "Save to HDF5 - TrainID = " << trainDataToStore->trainId;
      
#ifdef HDF5      
      string fileName =  utils::getLocalTimeStr() + "_TrainData_" + std::to_string(trainDataToStore->trainId) +".h5";
      DsscHDF5Writer::saveToFile(get<string>("outputDir") + "/" + fileName,trainDataToStore,64,64); 
#else
      KARABO_LOG_INFO << "HDF5 not installed";
#endif
      //m_trainSorter->addFreeStorage(trainDataToStore);
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
    const auto numFrames = currentTrainData->pulseCnt;
    const auto trainId   = currentTrainData->trainId;
    sendData.set<unsigned long long>(getDir("trainId"), trainId);

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
  
  int DsscDataReceiver::calcASICPixel(int imagePixel)
  {
    int row = imagePixel/512;
    int col = imagePixel%512;

    int asicRow = row/64;
    int asicCol = col/64;

    row -= asicRow*64;
    col -= asicCol*64;

    int asicPixel = row*64+col;
    return asicPixel;
  }


  int DsscDataReceiver::calcImagePixel(int asic, int asicPixel)
  {    
    int rowOffset = (asic>7)?  64 : 0;
    int colOffset = (asic%8) * 64;

    int row = asicPixel/64;
    int col = asicPixel%64;

    return (row + rowOffset)*512 + col + colOffset;
  }


  int DsscDataReceiver::calcPixelASIC(int pixel)
  {
    int row = pixel/512;
    int col = pixel%512;
    int asicRow = row/64;
    int asicCol = col/64;

    return asicRow*8+asicCol;
  }


  void DsscDataReceiver::initPixelNumbers()
  {
    if(m_ladderMode)
    {
      if(m_pixelNumbers.size() == (16*4096) ) return;

      m_pixelNumbers.resize(16*4096);      
      int idx = 0;
      for(auto && px : m_pixelNumbers){
        px = idx++;
      }
    }else{
      m_pixelNumbers.resize(4096);
      for(int px = 0; px < 4096; px++){
        m_pixelNumbers[px] = calcImagePixel(m_selAsic,px);
      }
    }
  }
  
  
  void DsscDataReceiver::initStatsAcc()
  {          
    if(m_ladderMode){
      m_dataStatsAcc.assign(16*4096,utils::StatsAcc());
      m_sramCorrectionAcc.assign(16*4096,std::vector<unsigned long>(800));
      m_pixelHistoVec.assign(16*4096,utils::DataHisto());
    }else{
      m_dataStatsAcc.assign(4096,utils::StatsAcc());
      m_sramCorrectionAcc.assign(4096,std::vector<unsigned long>(800));
      m_pixelHistoVec.assign(4096,utils::DataHisto());
    }        
    initPixelNumbers();
  }


  void DsscDataReceiver::savePixelHistos()
  {
    const string fileName = get<string>("outputDir") + "/PixelHistogramExport.dat"; 
    utils::dumpHistogramsToASCII(fileName,m_pixelNumbers,m_pixelHistoVec);
  }


  void DsscDataReceiver::clearPixelHistos()
  {
    for(auto && pixelHisto : m_pixelHistoVec){
      pixelHisto.clear();
    }
  }
  
}
