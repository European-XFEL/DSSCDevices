/*
 * Author: <haufs>
 *
 * Created on January, 2018, 02:49 PM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <thread>

#include "DsscProcessor.hh"
#include "DsscDependencies.h"
#include "DsscHDF5CorrectionFileReader.h"
#include "DsscHDF5TrimmingDataWriter.h"
#include "DsscHDF5MeasurementInfoWriter.h"
using namespace std;

USING_KARABO_NAMESPACES;


namespace karabo {

    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, DsscProcessor)

    void DsscProcessor::expectedParameters(Schema& expected) {

        STRING_ELEMENT(expected).key("inputDataFormat")
          .displayedName("Input Data Format")
          .readOnly().initialValue("unknown")
          .commit();

        SLOT_ELEMENT(expected).key("accumulate")
            .displayedName("Accumulate")
            .description("Start operation")
            .commit();

        SLOT_ELEMENT(expected).key("stop")
            .displayedName("Stop")
            .description("Stop operation")
            .commit();

        SLOT_ELEMENT(expected).key("resetCounters")
            .displayedName("Reset")
            .description("Reset Device, also removes sram blacklist")
            .commit();

        UINT16_ELEMENT(expected).key("emptyInjectCycles")
            .displayedName("EmptyInjectCycles")
            .description("Empty Inject Cycles in Pixel Injection")
            .assignmentOptional().defaultValue(3).reconfigurable()
            .commit();

        UINT16_ELEMENT(expected).key("injectOffset")
            .displayedName("Injection Offset")
            .description("Offset of Injection when empty Inject Cycles are set, can change by number of vetos")
            .assignmentOptional().defaultValue(0).reconfigurable()
            .commit();

        SLOT_ELEMENT(expected).key("setEmptyInjectCycles")
            .displayedName("Set EmptyInjectCycles")
            .description("Set Empty Inject cycles including sram blacklist for injection sweep mean value generation")
            .commit();

        PATH_ELEMENT(expected).key("sramBlacklistFileName")
          .description("Sram Blacklist File Path")
          .displayedName("Sram Blacklist File")
          .isInputFile()
          .assignmentOptional().defaultValue("Default_PxSramOutliers.txt").reconfigurable()
          .commit();

        BOOL_ELEMENT(expected).key("sramBlacklistValid")
            .displayedName("SRAM Blacklist Valid")
            .readOnly().initialValue(false)
            .commit();

        SLOT_ELEMENT(expected).key("clearSramBlacklist")
            .displayedName("Clear Sram Blacklist Data")
            .description("remove sram blacklist data, wont be applied anymore")
            .commit();

        UINT16_ELEMENT(expected).key("blacklistPixel")
            .displayedName("SRAM Blacklist Pixel")
            .description("show sram blacklist of selected pixel")
            .assignmentOptional().defaultValue(12345).reconfigurable()
            .commit();

        VECTOR_BOOL_ELEMENT(expected).key("pixelSramBlacklistValues")
            .displayedName("Pixel Valid Srams")
            .readOnly().initialValue(std::vector<bool>(utils::s_numSram,true))
            .commit();

        PATH_ELEMENT(expected).key("sramCorrectionFileName")
          .displayedName("SRAM Correction  Filename")
          .description("Input file name for the SRAM Correction File, can be generated by DsscDataReceiver or TrimmingDevice")
          .assignmentOptional().defaultValue("./").reconfigurable()
          .commit();

        BOOL_ELEMENT(expected).key("sramCorrectionValid")
            .displayedName("SRAM Correction Valid")
            .readOnly().initialValue(false)
            .commit();

        BOOL_ELEMENT(expected).key("baselineValuesValid")
            .displayedName("Baseline Valid")
            .readOnly().initialValue(false)
            .commit();

        BOOL_ELEMENT(expected).key("subtractBaseline")
            .displayedName("Subtract Baseline")
            .description("Debug feature only, baseline for trimming funtions is applied by trimming device. If enabled in both devices, baseline is subtracted twice")
            .assignmentOptional().defaultValue(false).reconfigurable()
            .commit();

        SLOT_ELEMENT(expected).key("clearSramCorrection")
            .displayedName("Clear Sram Correction Data")
            .description("remove sram correction data, wont be applied anymore")
            .commit();


        SLOT_ELEMENT(expected).key("acquireBaselineValues")
            .displayedName("Acquire Baseline Values")
            .description("start a train acquisition and update baseline values")
            .commit();


        STRING_ELEMENT(expected).key("sourceId")
            .displayedName("Source id")
            .description("DAQ source id, e.g. /SCS_DET_DSSC/DET/{}CH0."
                         "Use {} to denote the position of the infix for each channel.")
            .assignmentOptional().defaultValue("/SCS_DET_DSSC/DET/{}CH0")
            .commit();

        STRING_ELEMENT(expected).key("sendingASICs")
            .displayedName("SendingASICs")
            .description("Indicates which of the 16 ladder ASICs are sending data: 11110000_11110000")
            .tags("general")
            .assignmentOptional().defaultValue("00010000_00000000").reconfigurable()
            .commit();

        UINT8_ELEMENT(expected).key("sourceInfix")
            .displayedName("Source infix")
            .description("Source id infix. Only used if {} denote its position in the source id field")
            .assignmentOptional().defaultValue(0)
            .commit();

        UINT64_ELEMENT(expected).key("minValidTrainId")
            .displayedName("Min Train ID")
            .description("Device processes data only with a train id higher than specified")
            .assignmentOptional().defaultValue(0).reconfigurable()
            .commit();

        UINT64_ELEMENT(expected).key("currentTrainId")
            .displayedName("CurrentTrainID")
            .description("Current TrainID")
            .readOnly()
            .commit();

        BOOL_ELEMENT(expected).key("run")
            .displayedName("Run")
            .description("Enable device where data should be received")
            .assignmentOptional().defaultValue(false).reconfigurable()
            .commit();

        BOOL_ELEMENT(expected).key("measureMean")
            .displayedName("Measure Mean Values")
            .description("Enabled by device where data should be received")
            .assignmentOptional().defaultValue(false).reconfigurable()
            .commit();

        BOOL_ELEMENT(expected).key("measureRMS")
            .displayedName("Measure RMS Values")
            .description("Enabled by device where data should be received")
            .assignmentOptional().defaultValue(false).reconfigurable()
            .commit();

        UINT32_ELEMENT(expected).key("numIterations")
            .displayedName("Iterations")
            .description("Number of iterations to accumulate")
            .assignmentOptional().defaultValue(20).reconfigurable()
            .commit();


        UINT16_ELEMENT(expected).key("minSram")
            .displayedName("Min Sram")
            .description("Minimum SRAM address for mean value computation")
            .assignmentOptional().defaultValue(0).reconfigurable()
            .commit();

        UINT16_ELEMENT(expected).key("maxSram")
            .displayedName("Max Sram")
            .description("Maximum (included) SRAM address for mean value computation")
            .assignmentOptional().defaultValue(799).reconfigurable()
            .commit();

        UINT32_ELEMENT(expected).key("iterationCnt")
            .displayedName("IterationCnt")
            .readOnly()
            .commit();
        
        FLOAT_ELEMENT(expected).key("dataRate")
            .displayedName("Data rate")
            .readOnly()
            .commit();     
        
        SLOT_ELEMENT(expected).key("runHistogramAcquisition")
            .displayedName("Start Histogram Acquisition")
            .description("remove sram correction data, wont be applied anymore")
            .commit();

        SLOT_ELEMENT(expected).key("saveHistograms")
            .displayedName("Save Histograms")
            .description("save acquired histograms")
            .commit();
        
        PATH_ELEMENT(expected).key("outputDir")
            .displayedName("Output Directory")
            .description("Output directory for Histograms")
            .isDirectory()
            .assignmentOptional().defaultValue("./").reconfigurable()
            .commit();
        
        BOOL_ELEMENT(expected).key("acquireHistograms")
            .displayedName("Acquire Online Histograms")
            .readOnly()
            .commit();
      
        NODE_ELEMENT(expected).key("histoGen")
              .description("Vectors for Histogram Generation")
              .displayedName("Online Histogram View")
              .commit();
        
        UINT32_ELEMENT(expected).key("histoGen.ladderPixelToShow")
                .displayedName("Pixel To Show")
                .assignmentOptional().defaultValue(true).reconfigurable()
                .commit();
      
        UINT64_ELEMENT(expected).key("histoGen.pixelhistoCnt")
            .displayedName("Pixel Histo Entry County")
            .readOnly().initialValue(0)
            .commit();

        VECTOR_UINT16_ELEMENT(expected).key("histoGen.pixelHistoBins")
                  .displayedName("Pixel Histo Bins")
                  .readOnly().initialValue(std::vector<unsigned short>(20,0))
                  .commit();

        VECTOR_UINT32_ELEMENT(expected).key("histoGen.pixelHistoBinValues")
                  .displayedName("Pixel Histo Bin Values")
                  .readOnly().initialValue(std::vector<unsigned int>(20,0))
                  .commit();
        
      BOOL_ELEMENT(expected).key("histoGen.displayHistoLogscale")
                .displayedName("Display Histogram Logscale")
                .description("Switch display of Histograms to Logscale")
                .assignmentOptional().defaultValue(false)
                .reconfigurable()
                .commit();


        INPUT_CHANNEL(expected).key("input")
                .displayedName("Input")
                .commit();
        
        SLOT_ELEMENT(expected).key("startPreview")
            .displayedName("Start preview")
            .description("Start sending pixel data to output visualization channel")
            .commit();
        
        SLOT_ELEMENT(expected).key("stopPreview")
            .displayedName("Stop preview")
            .description("Stop sending pixel data to output visualization channel")
            .commit();
        
        BOOL_ELEMENT(expected).key("preview")
            .displayedName("Preview")
            .description("Enabled preview of pixeldata on output channel")
            .readOnly()
            .commit();

        Schema asicOutSchema;

        NDARRAY_ELEMENT(asicOutSchema).key("asicData")
            .shape("16,4096,800")
            .readOnly()
            .commit();

        UINT64_ELEMENT(asicOutSchema).key("trainId")
            .readOnly()
            .commit();

        UINT32_ELEMENT(asicOutSchema).key("pulseCnt")
            .readOnly()
            .commit();

      OUTPUT_CHANNEL(expected).key("pixelDataOutput")
            .displayedName("Pixel Data Output")
            .dataSchema(asicOutSchema)
            .commit();



        Schema meanOutSchema;
        VECTOR_DOUBLE_ELEMENT(meanOutSchema).key("asicMeanData")
                .readOnly()
                .commit();

        STRING_ELEMENT(meanOutSchema).key("dataType")
            .readOnly()
            .commit();

        VECTOR_UINT64_ELEMENT(meanOutSchema).key("trainIds")
            .readOnly()
            .commit();

        UINT64_ELEMENT(meanOutSchema).key("numIterations")
            .readOnly()
            .commit();

        OUTPUT_CHANNEL(expected).key("meanDataOutput")
            .displayedName("Mean and RMS Data Output")
            .dataSchema(meanOutSchema)
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


    DsscProcessor::DsscProcessor(const karabo::util::Hash& config)
      : Device<>(config),
        DsscTrainDataProcessor()
    {
      KARABO_INITIAL_FUNCTION(initialization);

      KARABO_SLOT(accumulate)
      KARABO_SLOT(stop)
      KARABO_SLOT(resetCounters)
      KARABO_SLOT(setEmptyInjectCycles)

      KARABO_SLOT(clearSramCorrection)
      KARABO_SLOT(clearSramBlacklist)

      KARABO_SLOT(acquireBaselineValues)
      KARABO_SLOT(runHistogramAcquisition)

      KARABO_SLOT(saveHistograms)
              
      KARABO_SLOT(startPreview)
      KARABO_SLOT(stopPreview)              

      m_starttime = std::chrono::high_resolution_clock::now();

    }


    DsscProcessor::~DsscProcessor() {
        m_run = false;
      set<bool>("run",false);
    }

    void DsscProcessor::preReconfigure(karabo::util::Hash& incomingReconfiguration)
    {
      vector<string> paths;
      incomingReconfiguration.getPaths(paths);

      if(!paths.empty()){
        BOOST_FOREACH(string path, paths) {
          if(path.compare("sendingASICs") == 0){
            m_availableAsics = utils::bitEnableStringToVector(incomingReconfiguration.getAs<string>(path));
          }else if(path.compare("sramBlacklistFileName") == 0){
            std::string fileName = incomingReconfiguration.getAs<string>(path);
            setSramBlacklist(fileName);
            if(isSramBlacklistValid()){
              KARABO_LOG_INFO << "Loaded SRAM Blacklist from " << fileName;
            }else{
              KARABO_LOG_ERROR << "Could not load SRAM Blacklist from " << fileName;
            }
            set<bool>("sramBlacklistValid",isSramBlacklistValid());

            updateSelPixelSramBlacklist();

          }
          else if(path.compare("blacklistPixel") == 0)
          {
            uint16_t pixel = incomingReconfiguration.getAs<unsigned short>(path);
            const auto pixelValidAddresses = m_sramBlacklist.getValidSramAddresses(pixel);
            set<vector<bool>>("pixelSramBlacklistValues",pixelValidAddresses);
            KARABO_LOG_INFO << "Vector updated ";
          }
          else if(path.compare("sramCorrectionFileName") == 0)
          {
            std::string fileName = incomingReconfiguration.getAs<string>(path);
            if(utils::checkFileExists(fileName)){
              DsscHDF5CorrectionFileReader corrFileReader(fileName);
              bool valid = corrFileReader.isValid();

              if(valid){
                DSSC::StateChangeKeeper keeper(this);
                corrFileReader.loadCorrectionData(m_pixelBackgroundData,m_sramCorrectionData);
              }else{
                KARABO_LOG_ERROR << "Correction file found, but structure invalid" << fileName;
              }
              set<bool>("sramCorrectionValid",valid);
              set<bool>("baselineValuesValid",valid);
            }else{
              KARABO_LOG_ERROR << "File not found. Could not load SRAM Correction from " << fileName;
            }
          }
          else if(path.compare("run") == 0)
          {
            m_run = incomingReconfiguration.getAs<bool>(path);                
          }
        }
      }
    }


    void DsscProcessor::postReconfigure()
    {
      set<bool>("sramBlacklistValid",isSramBlacklistValid());
    }


    void DsscProcessor::updateSelPixelSramBlacklist()
    {
      uint16_t pixel = get<unsigned short>("blacklistPixel");
      const auto pixelValidAddresses = m_sramBlacklist.getValidSramAddresses(pixel);
      set<vector<bool>>("pixelSramBlacklistValues",pixelValidAddresses);
    }


    void DsscProcessor::clearData()
    {
      m_trainIds.clear();
      DsscTrainDataProcessor::clearAccumulators();
    }


    void DsscProcessor::accumulate()
    {
      resetCounters();
      changeDeviceState(State::ACQUIRING);
      set<bool>("run",true);
      m_run = true;
    }


    void DsscProcessor::stop()
    {
      set<bool>("run",false);
      m_run = false;
      changeDeviceState(State::STOPPED);
    }

    void DsscProcessor::saveHistograms()
    {
      savePixelHistos();
    }

    void DsscProcessor::changeDeviceState(const util::State & newState)
    {
      try{
        updateState(newState);
      }
      catch(...)
      {
        KARABO_LOG_WARN << "DsscProcessor: changeDeviceState : WARNING STATE COULD NOT BE UPDATED!!!!";
      }
    }


    void DsscProcessor::resetCounters()
    {
      m_trainIds.clear();
      m_iterationCnt = 0;
      m_numIterations = get<unsigned int>("numIterations");
      set<bool>("run",false);
      m_run = false;

      set<unsigned int>("iterationCnt",m_iterationCnt);

      m_minSram = get<unsigned short>("minSram");
      m_maxSram = get<unsigned short>("maxSram");

      clearData();
    }

    void DsscProcessor::initialization()
    {
        KARABO_ON_DATA("input", onData)

        m_acquireBaseLine = false;
        m_acquireSramCorrection = false;

        m_inputFormat = DATAFORMAT::IMAGE;

        m_sourceId = get<std::string>("sourceId");
        unsigned char sourceInfix = get<unsigned char>("sourceInfix");
        boost::replace_all(m_sourceId, "{}", karabo::util::toString(sourceInfix));

        m_availableAsics = utils::bitEnableStringToVector(get<string>("sendingASICs"));

        m_acquireHistograms = false;          
        set<bool>("acquireHistograms",m_acquireHistograms);
        
        changeDeviceState(State::OFF);
        
        m_run = get<bool>("run");
        m_preview = false;
        set<bool>("preview", false);
        m_previewImageData = std::vector<uint16_t>(m_imageSize, 0);
    }

    void DsscProcessor::onData(const karabo::util::Hash& data,
                               const karabo::xms::InputChannel::MetaData& meta)  //find data for data source
    {
  
    
      auto timenow = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> diff = timenow - m_starttime;
      m_starttime = timenow;
      set<float>("dataRate", 1.0/diff.count());
      
      
                    // verify data is sane
      if (!data.has("image.data")){
        cout << "No Image Data Found in input data " << m_sourceId << endl;
        return;
      }

      if (!data.has("image.cellId")){
        cout << "No cellId Data Found in input data " << m_sourceId << endl;
        return;
      }

      if (!data.has("image.trainId")){
        cout << "No trainId Data Found in input data " << m_sourceId << endl;
        return;
      } 

      if(m_preview) UpdatePreviewData(data);
      
      auto image_data = data.get<util::NDArray>("image.data");
      auto image_cellId = data.get<util::NDArray>("image.data");
      auto image_trainId = data.get<util::NDArray>("image.trainId");          

      if(m_run == false){
        changeDeviceState(State::STOPPED);
        return; // don't send anything during idle+
      }
      changeDeviceState(State::ACQUIRING);

      // resend until stopped from remote
      if(get<bool>("measureMean") || get<bool>("measureRMS")){
        if(m_numIterations == m_iterationCnt){
          sendMeanValues();
          KARABO_LOG_INFO << "RESENT DATA ";
          return;
        }
      }
      
      if (data.has("imageFormat"))
      {
        string format = data.get<string>("imageFormat");
        m_inputFormat = utils::DsscTrainData::getFormat(format);
        set<string>("inputDataFormat",format);

        processTrain(image_data,
                     image_cellId,
                     image_trainId);
      }else{
          m_inputFormat = utils::DsscTrainData::DATAFORMAT::IMAGE;
          set<string>("inputDataFormat","imagewise");

          processTrain(image_data,
                       image_cellId,
                       image_trainId);
      }

    }


    void DsscProcessor::processTrain(const karabo::util::NDArray& data, const karabo::util::NDArray& cellId, const karabo::util::NDArray& trainId)
    {
      // standard c++ data structures to use for access in data

      const uint16_t* data_ptr = data.getData<uint16_t>();
      size_t data_size = data.size();

      const uint16_t* cellId_ptr = cellId.getData<uint16_t>();
      size_t cellId_size = cellId.size();

      const unsigned long long* trainId_ptr = trainId.getData<unsigned long long>();
      size_t trainId_size = trainId.size();

      m_numFrames = cellId_size;
      m_alsoRMS = get<bool>("measureRMS");
          
      for(uint idx = 0; idx < 10 ; idx++){
            cout << idx << " : " << data_ptr[idx] << std::endl;
        }

      int numAsics = data_size/utils::s_numAsicPixels/cellId_size;

      m_availableAsics = utils::getUpCountingVector(numAsics);
      m_sortedTrainData.availableASICs = m_availableAsics;

      if(m_maxSram >= m_numFrames){
        m_maxSram = m_numFrames-1;
        m_minSram = 0;
        set<unsigned short>("maxSram",m_maxSram);
        set<unsigned short>("minSram",m_minSram);
        KARABO_LOG_WARN << "Sram Range does not fit to number of frames, is corrected";
      }


      //cout << "DataSize = " << data_size << endl;
      //cout << "cellId_size = " << cellId_size << endl;
      //cout << "DataSize = " << data_size << endl;

      const auto minValidTrainId = get<unsigned long long>("minValidTrainId");

      // business logic starts here
      
      set<unsigned long long>("currentTrainId",trainId_ptr[0]);

  

      if(get<bool>("measureMean") || get<bool>("measureRMS"))
      {
        m_trainIds.push_back(trainId_ptr[0]);

        processMeanValues(data_ptr,m_inputFormat);

        if(m_iterationCnt >= m_numIterations){
          sendMeanValues();
        }

        set<unsigned int>("iterationCnt",m_iterationCnt);

      }
      else if(m_acquireHistograms)
      {          
        fillDataHistoVec(data_ptr,m_pixelHistoVec,false);
        m_iterationCnt++;
          
        KARABO_LOG_DEBUG << "DataProcessor: filled histogram " << m_iterationCnt << "/" << m_numIterations;
          
        if(m_iterationCnt == m_numIterations)
        {
          //savePixelHistos();    
          displayPixelHistogram();  
          stop();
        }
      }
      else
      {
        auto processedPixelData = processPixelData(data_ptr,m_inputFormat);
        sendPixelData(processedPixelData,trainId_ptr[0]);
      }

        //KARABO_LOG_INFO << "Train Processed: " << this_train->first << "/" << minValidTrainId;
    }


    void DsscProcessor::sendPixelData(const unsigned short * pixel_wise_data_ptr, unsigned long long train_id)
    {
      static const uint32_t numSram = utils::s_numSram;
      //do not copy data
      NDArray asicNDArray(pixel_wise_data_ptr,utils::s_totalNumWords, util::NDArray::NullDeleter(), Dims(utils::s_numAsics,utils::s_numAsicPixels,utils::s_numSram));

      util::Hash pixelDataHash;
      pixelDataHash.set("asicData",asicNDArray);
      pixelDataHash.set("trainId",train_id);
      pixelDataHash.set("pulseCnt",numSram);

      writeChannel("pixelDataOutput",pixelDataHash);
    }


    void DsscProcessor::sendMeanValues()
    {
      //cout << "num Train Ids " << m_trainIds.size() << endl;
      if(m_acquireBaseLine){
        setBackgroundData(utils::convertVectorType<double,float>(m_pixelMeanData));
        set<bool>("baselineValuesValid",backgroundDataValid());
      }

      util::Hash meanDataHash;
      if(get<bool>("measureRMS")){
        meanDataHash.set("asicMeanData",m_pixelRMSData);
        meanDataHash.set("dataType","rmsValues");
      }
      else
      {
        if(get<bool>("baselineValuesValid") && get<bool>("subtractBaseline"))
        {
      #pragma omp parallel for
          for(size_t pixelIdx=0; pixelIdx<utils::s_totalNumPxs; pixelIdx++){
            m_pixelMeanData[pixelIdx] -= m_pixelBackgroundData[pixelIdx];
          }
        }

        meanDataHash.set("asicMeanData",m_pixelMeanData);
        meanDataHash.set("dataType","meanValues");
      }
      meanDataHash.set("trainIds",m_trainIds);
      meanDataHash.set("numIterations",m_numIterations);

      cout << "DataProcessor: send mean data" << endl;
      writeChannel("meanDataOutput",meanDataHash);
    }


    void DsscProcessor::setEmptyInjectCycles()
    {
      uint16_t offset      = get<unsigned short>("injectOffset");
      uint16_t addrSkipCnt = get<unsigned short>("emptyInjectCycles")+1;
      m_sramBlacklist.initForPixelInjection(offset,addrSkipCnt,true);

      string sramBlacklistFileName = get<string>("sramBlacklistFileName");
      if(utils::checkFileExists(sramBlacklistFileName)){
        m_sramBlacklist.initFromFile(sramBlacklistFileName,true);
      }

      KARABO_LOG_INFO << "Sram Blacklist updated for injection sweep";
    }


    void DsscProcessor::clearSramCorrection()
    {
      clearSramCorrectionData();
      set<bool>("sramCorrectionValid",false); 
      set<bool>("baselineValuesValid",false);
    }


    void DsscProcessor::clearSramBlacklist()
    {
      clearSramBlacklistData();
      set<bool>("sramBlacklistValid",false); 
    }

    void DsscProcessor::acquireBaselineValues()
    {
      m_pixelBackgroundData.clear();

      m_acquireBaseLine = true;
      set<bool>("measureMean", true);
      set<bool>("measureRMS", false);
      accumulate();
    }
    
    void DsscProcessor::runHistogramAcquisition()
    {      
      m_pixelHistoVec.assign(m_pixelsToProcess.size(),utils::DataHisto());
      m_acquireHistograms = true;      
      
      set<bool>("acquireHistograms",m_acquireHistograms);
      set<bool>("measureMean",false);
      set<bool>("measureRMS",false);

      accumulate();   
      KARABO_LOG_INFO << "Histogram Generation started";
    }
    
    void DsscProcessor::savePixelHistos()
    {
      utils::fillBufferToDataHistoVec(m_pixelHistoVec);

      const string path = get<string>("outputDir");
      const string outDir = path + "/SpectrumHisto";
      utils::makePath(outDir);
      const string fileName   = outDir + "/" + utils::getLocalTimeStr() + "_PixelHistogramExport.dat";
      utils::DataHisto::dumpHistogramsToASCII(fileName,m_pixelsToProcess,m_pixelHistoVec);

      const string h5FileName = outDir + "/" + utils::getLocalTimeStr() + "_PixelHistogramExport.h5";
      DsscHDF5TrimmingDataWriter dataWriter(h5FileName);
      dataWriter.setMeasurementName("LadderSpectrum");
      dataWriter.addHistoData("SpektrumData",m_pixelHistoVec,m_pixelsToProcess);

      const auto imageValues = utils::calcMeanImageFromHistograms(m_pixelHistoVec,m_pixelsToProcess);
      dataWriter.addImageData("SpectrumPedestalImage",512,imageValues);

      saveMeasurementInfo();
      
      KARABO_LOG_INFO << "Stored Pixel Histograms to " << h5FileName;
      
      m_acquireHistograms = false;
      set<bool>("acquireHistograms",m_acquireHistograms);
    }
    
    void DsscProcessor::saveMeasurementInfo()
    {
      const std::string infoFileName = get<string>("outputDir") + "/MeasurementInfo.h5";      
      
      DsscHDF5MeasurementInfoWriter::MeasurementConfig config;
      config.configFileName = infoFileName;
      config.measurementName = "BurstMeasurement";
      config.measurementSettingName = "Spectrum";
      config.numIterations = m_numIterations;
      config.numPreBurstVetos = 0xFFFF;
      config.ladderMode = 1;
      config.columnSelection = "all";
      config.activeASICs = m_availableAsics;
      config.measurementDirectories = {"SpectrumHisto"};
      config.measurementSettings = {1};
           
      DsscHDF5MeasurementInfoWriter infoWriter(infoFileName);
      infoWriter.addMeasurementConfig(config);
    }
    
    void DsscProcessor::displayPixelHistogram()
    {
      static vector<unsigned short> bins;
      static vector<unsigned int> binValues;

      const auto imagePixel = get<unsigned int>("histoGen.ladderPixelToShow");      
      m_pixelHistoVec[imagePixel].fillBufferToHistoMap();
      m_pixelHistoVec[imagePixel].getDrawValues(bins,binValues);
      
      size_t histoValueCount = m_pixelHistoVec[imagePixel].getCount();
 
      if(!bins.empty()){
        set<unsigned long long>("histoGen.pixelhistoCnt",histoValueCount);
        set<vector<unsigned short>>("histoGen.pixelHistoBins",std::move(bins));
        if(get<bool>("histoGen.displayHistoLogscale")){
          std::transform(binValues.begin(),binValues.end(),binValues.begin(),[](int x){if(x==0) return 1; return x;});
        }
        set<vector<unsigned int>>("histoGen.pixelHistoBinValues",std::move(binValues));
      }

    }
    
    void DsscProcessor::startPreview()
    {
      //
      {
        std::lock_guard< std::mutex > lock( m_previewMutex );
        if(m_preview) return;
        m_preview = true; 
        m_previewThread = std::thread(&DsscProcessor::previewThreadFunc, this);
      }
      set<bool>("preview", true);  
    }

    void DsscProcessor::stopPreview()
    {
      //
      {
        std::lock_guard< std::mutex > lock( m_previewMutex );  
        if(!m_preview) return;
        m_preview = false;
        if(m_previewThread.joinable()) m_previewThread.join();
      }
      set<bool>("preview", false);     
    }
    
    void DsscProcessor::UpdatePreviewData(const karabo::util::Hash& data)
    {
      //
        
        auto imageData = data.get<util::NDArray>("image.data");
        auto imageDataShape = data.get<vector<unsigned long long> >("image.data.shape");
        //copy data for output ladder image channel
        unsigned int frameIndxViz = imageDataShape[0] - 1;
        //ToDo add update of value in GUI
            
        uint32_t indx = frameIndxViz*m_imageSize;
        auto dataPtr = imageData.getData<uint16_t>();
        
        for(int i=0; i<m_imageSize; i++)
        {
            m_previewImageData[i] = dataPtr[indx];
            indx++;
        }
    }
    
    void DsscProcessor::previewThreadFunc()
    {
      //
        NDArray imageArray(m_previewImageData.data(),
                m_imageSize,
                NDArray::NullDeleter(),
                Dims(128,512));
        util::Hash dataHash;
        dataHash.set("ladderImage",ImageData(imageArray));
        while(m_preview)
        {
            //
            writeChannel("ladderImageOutput", dataHash);
        }
        
    }
    
    /*void DsscProcessor::fillImageData(,vector<uint16_t> & imageData, int frameNum)
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
    }//*/

    
}


