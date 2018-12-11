/*
 * Author: <haufs>
 *
 * Created on January, 2018, 02:49 PM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "DsscProcessor.hh"
#include "DsscDependencies.h"
#include "DsscHDF5CorrectionFileReader.h"

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
            .description("Start operation")
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

        INPUT_CHANNEL(expected).key("input")
                .displayedName("Input")
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

    }


    DsscProcessor::~DsscProcessor() {
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
    }


    void DsscProcessor::stop()
    {
      set<bool>("run",false);
      changeDeviceState(State::STOPPED);
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

        changeDeviceState(State::OFF);
    }

    void DsscProcessor::onData(const karabo::util::Hash& data,
                               const karabo::xms::InputChannel::MetaData& meta)  //find data for data source
    {
      if(get<bool>("run") == false){
        changeDeviceState(State::STOPPED);
        return; // dont send anything during idle+
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

      if (data.has("imageFormat"))
      {
        string format = data.get<string>("imageFormat");
        m_inputFormat = utils::DsscTrainData::getFormat(format);
        set<string>("inputDataFormat",format);

        processTrain(data.get<util::NDArray>("image.data"),
                     data.get<util::NDArray>("image.cellId"),
                     data.get<util::NDArray>("image.trainId"));
      }else{
          m_inputFormat = utils::DsscTrainData::DATAFORMAT::IMAGE;
          set<string>("inputDataFormat","imagewise");

          processTrain(data.get<util::NDArray>("image.data"),
                       data.get<util::NDArray>("image.cellId"),
                       data.get<util::NDArray>("image.trainId"));
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

      m_alsoRMS = get<bool>("measureRMS");
      m_numFrames = cellId_size;
      int numAsics = data_size/utils::s_numAsicPixels/cellId_size;

      m_availableAsics = utils::getUpCountingVector(numAsics);

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
      for(size_t train_idx = 0; train_idx<trainId_size; train_idx++)
      {
        if(trainId_ptr[train_idx] <= minValidTrainId) continue;

        set<unsigned long long>("currentTrainId",trainId_ptr[train_idx]);

        size_t train_offset = train_idx*utils::s_totalNumPxs*m_numFrames;
        const unsigned short* train_data_ptr = data_ptr + train_offset;
        if(get<bool>("measureMean") || get<bool>("measureRMS"))
        {
          m_trainIds.push_back(trainId_ptr[train_idx]);

          processMeanValues(train_data_ptr,m_inputFormat);

          if(m_iterationCnt >= m_numIterations){
            sendMeanValues();
          }

          set<unsigned int>("iterationCnt",m_iterationCnt);

        }else{
          auto processedPixelData = processPixelData(train_data_ptr,m_inputFormat);
          sendPixelData(processedPixelData,trainId_ptr[train_idx]);
        }

        //KARABO_LOG_INFO << "Train Processed: " << trainId_ptr[train_idx] << "/" << minValidTrainId;
      }
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
    }


    void DsscProcessor::clearSramBlacklist()
    {
      clearSramBlacklistData();
    }

    void DsscProcessor::acquireBaselineValues()
    {
      m_pixelBackgroundData.clear();

      m_acquireBaseLine = true;
      set<bool>("measureMean", true);
      set<bool>("measureRMS", false);
      accumulate();
    }

}
