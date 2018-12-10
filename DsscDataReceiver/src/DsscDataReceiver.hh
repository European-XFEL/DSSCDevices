/*
 * $Id: DsscDataReceiver.hh 9210 2017-10-26 15:59:42Z manfred.kirchgessner@ziti.uni-heidelberg.de $
 *
 * Author: <manfred.kirchgessner@ziti.uni-heidelberg.de>
 *
 * Created on April, 2016, 03:34 PM
 *
 * Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_DSSCDATARECEIVER_HH
#define KARABO_DSSCDATARECEIVER_HH
#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <functional>

#define HDF5

#include <karabo/karabo.hpp>
#ifdef HDF5
#include "DsscHDF5Writer.h"
#endif
#include "DsscTrainData.hh"
#include "DsscTrainDataProcessor.h"
#include "DsscTrainSorter.h"
#include "DataHisto.h"

/**
 * The main Karabo namespace
 * States:
 *      OFF -> open() -> STARTED -> activate() -> ACQUIRING
 *      ACQUIRING -> stop() -> STARTED -> close() -> OFF
 */
namespace karabo {

    class DsscDataReceiver : public karabo::core::Device<> {

    public:

        // Add reflection information and Karabo framework compatibility to this class
        KARABO_CLASSINFO(DsscDataReceiver, "DsscDataReceiver", "3.1")

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
        DsscDataReceiver(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed (i.e. the event-loop returns)
         */
        virtual ~DsscDataReceiver();


        static util::Schema createDataSchema(karabo::util::DetectorGeometry geometry);

        void updateNumFramesToReceive();

        void stopFileMode();
        void stopDataReceiver();
        void stopDataSorting();

        bool waitDataReceived();

        void startDataTransfer();

        void start();
        void updateStatus(const std::string & text);

        void updateSpecificData(const utils::DsscTrainData *trainData);
        void updateTempADCValues(utils::DsscTrainData * trainData);
        void saveToHDF(utils::DsscTrainData * trainDataToStore);
        void saveToHDFviaThread();
        /**
         * This function acts as a hook and is called after an reconfiguration request was received,
         * but BEFORE this reconfiguration request is actually merged into this device's state.
         *
         * The reconfiguration information is contained in the Hash object provided as an argument.
         * You have a chance to change the content of this Hash before it is merged into the device's current state.
         *
         * NOTE: (a) The incomingReconfiguration was validated before
         *       (b) If you do not need to handle the reconfigured data, there is no need to implement this function.
         *           The reconfiguration will automatically be applied to the current state.
         * @param incomingReconfiguration The reconfiguration information as was triggered externally
         */
        virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration);
        virtual void preReconfigurePreview(karabo::util::Hash& incomingReconfiguration);
        virtual void preReconfigureReceiver(karabo::util::Hash& incomingReconfiguration);
        virtual void preReconfigureCorrection(karabo::util::Hash& incomingReconfiguration);


        void open();
        void close();
        void reset();
        void activate();
        void stop();
        void restart(); // call from external
        void stopWriteThreads();

        void restartLadderMode(bool ladderMode); // call from preReconfigure
        void setActiveAsic();

        void flushTrainStorage();
        void receiveData();
        void displayData();

        void processCurrentTrainData();
        void displayTrainDataToShow();

        void updateCurrentTrainIdAndCnt(uint64_t currTrainId);

        void setASICsToRecord();
        void getASICsToRecord();

        void acquireTrains();
        void acquireBackgroundAndCorrection();
        void loadBackgroundAndCorrection();


        void startPolling();
        void stopPolling();
        void getFillStands();


        bool allASICTestPatternsValid(utils::DsscTrainData * trainDataToCheck);
        bool allASICTempValuesValid(utils::DsscTrainData * trainDataToCheck);
        void checkTestPattern(utils::DsscTrainData * trainDataToCheck);
        void checkTestPatternData(utils::DsscTrainData * trainDataToCheck);

        util::Hash getMonitorData();
        util::Hash getImageData(int frameNum);
        util::Hash getImageData(int frameNum, int asic);
        util::Hash getPixelData(int asicPixel, int asic);

        void fillImageData(std::vector<unsigned int> & imageData, int frameNum);
        std::vector<uint32_t> accumulateImageData(int minSram, int maxSram);
        static std::string getDir(const std::string & name);

        void fillMetaData();
        void fillTrailer();
        void fillHeader();
        void fillSpecificData();
        void initialize();
        void initSortMap();

        void startFileReading();

        void savePixelHistos();
        void clearPixelHistos();
        void updatePixelHistos();

        void finishAcquisition();
        bool checkOutputDirExists();
        bool checkDirExists(const std::string & path);

        void resetHistograms(){m_nextHistoReset = true;}
        void resetThresholdMap(){m_nextThresholdReset = true;}
        void clearHistograms();
        void clearThresholdMap();
        void saveCurrentPixelHisto();

        void loadSramBlacklist();
        void clearSramBlacklist();
        void clearSramCorrection();

        util::Dims getDims();

        //module Info initialization for HDF5 file writing
        void initDataWriter();
        void updateModuleInfo();
    private:

        inline float rawData(int frame, int pixel, const  uint16_t & value) const;
        inline float rawDataSramBL(int frame, int pixel, const  uint16_t & value) const;
        inline float gccWrapCorrect(int frame, int pixel, const  uint16_t & value) const;
        inline float sramCorrectData(int frame, int pixel, const uint16_t & value) const;
        inline float sramCorrectSramBLData(int frame, int pixel, const uint16_t & value) const;
        inline float offsetCorrectData(int frame, int pixel, const uint16_t & value) const ;
        inline float offsetCorrectSramBLData(int frame, int pixel, const uint16_t & value) const ;
        inline float offsetCorrectTHData(int frame, int pixel, const uint16_t & value) const ;
        inline float offsetCorrectTHSramBLData(int frame, int pixel, const uint16_t & value) const ;
        inline float fullCorrectData(int frame, int pixel, const uint16_t & value) const ;
        inline float fullCorrectSramBLData(int frame, int pixel, const uint16_t & value) const ;
        inline float fullCorrectTHData(int frame, int pixel, const uint16_t & value) const ;
        inline float fullCorrectTHSramBLData(int frame, int pixel, const uint16_t & value) const ;

        std::function<float(int,int,const uint16_t & )> getGccCorrectionFunction();
        std::function<float(int,int,const uint16_t & )> getSimpleCorrectionFunction();
        std::function<float(int,int,const uint16_t & )> getThresholdFunction();
        std::function<float(int,int,const uint16_t & )> getThresholdSramBLFunction();
        std::function<float(int,int,const uint16_t & )> getSimpleSramBLCorrectionFunction();
        std::function<float(int,int,const uint16_t & )> getSramBlacklistCorrectionFunction();
        std::function<float(int,int,const uint16_t & )> getNoBlacklistCorrectionFunction();

        std::function<float(int,int,const uint16_t & )> getCorrectionFunction();

        void updateProcessorParams();
        void initStatsAcc();
        void updateStatsAcc();
        void computeMeanBursts();
        void updateCorrectionMap();
        void computeSramCorrectionData();
        void storeMeanAndRMSImage();
        void stopDisplay();
        void startDisplay();
        void displayPixelHistogram();
        void displayAsicHistogram();

        DsscTrainSorter m_trainSorter;

        /// thread for actual work
        boost::shared_ptr<boost::thread> m_writingThread;
        boost::shared_ptr<boost::thread> m_displayThread;
        boost::shared_ptr<boost::thread> m_pollThread;

        std::vector<boost::thread> m_writeHDFThreads;

        bool m_isStopped;
        bool m_runDisplay;
        bool m_acquireBG;
        bool m_genTrainStats;
        bool m_keepPolling;

        bool m_correct;
        bool m_subtract;
        bool m_enThreshold;
        unsigned int m_threshold;
        bool m_bgDataValid;
        bool m_showThreshold;
        bool m_showRawData;
        uint16_t m_gccwrap;


        util::Hash sendData;


        static const std::unordered_map<std::string,std::string> directoryStructure;
        utils::DataHistoVec m_pixelHistoVec;

        std::vector<float> m_pixelData;
        std::vector<float> m_pixelBackgroundData;
        std::vector<unsigned int> m_thresholdMap;
        std::vector<utils::StatsAcc> m_dataStatsAcc;
        std::vector<std::vector<utils::StatsAcc>> m_sramCorrectionAcc;
        std::vector<std::vector<float>> m_pixelCorrectionData;

        utils::DataHisto pixelHisto;
        utils::DataHisto asicHisto;

        utils::DsscTrainData * currentTrainData;
        utils::DsscTrainData * m_trainDataToShow;

        bool m_ladderMode;
        int m_asicPixelToShow;
        int m_asicToShow;
        int m_selAsic;
        int m_selFrame;
        unsigned long long m_currentTrainID;

        util::State lastState;

        bool m_nextHistoReset;
        bool m_nextThresholdReset;

        std::function<float(int,int,const uint16_t & )> m_correctionFunction;
        std::function<float(int,uint16_t *,uint16_t num)> m_correctionFunctionPixelWise;

        uint64_t m_numCheckedData;
        uint64_t m_errorCnt;

        std::vector<uint64_t> m_notSendingASICs;
        std::vector<uint32_t> m_availableASICs;
        std::vector<int> m_sendingASICs;
        uint16_t m_actASICs;

        utils::DsscTrainDataProcessor m_processor;

    };
}

#endif
