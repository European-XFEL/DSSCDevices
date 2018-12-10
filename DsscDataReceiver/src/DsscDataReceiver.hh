/*
 * $Id$
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
#include "DsscTrainSorter.h"


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
        KARABO_CLASSINFO(DsscDataReceiver, "DsscDataReceiver", "2.0")

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
        
        void stopDataReceiver();
        
        bool waitDataReceived();
        
        void startDataTransfer();

        void start();
        void updateStatus(const std::string & text);
        
        void saveToHDF(DsscTrainData * trainDataToStore);
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
        
        void restartLadderMode(bool ladderMode); // call from preReconfigure
        void restartSelASIC(); // call from preReconfigure
                
        void flushTrainStorage();
        void receiveData();
        
        void acquireTrains();
        void acquireBackgroundAndCorrection();
        void loadBackgroundAndCorrection();

            
        void startPolling();
        void stopPolling();
        void getFillStands();
 
        
        util::Hash getImageData(int frameNum);
        util::Hash getImageData(int frameNum, int asic);
        util::Hash getPixelData(int asicPixel, int asic);
                
        static std::string getDir(const std::string & name);
        
        void fillMetaData();
        void fillTrailer();
        void fillHeader();
        void fillSpecificData();
        void initialize();
        void initSortMap();
        void initPixelNumbers();
        
        int calcASICPixel(int imagePixel);
        int calcImagePixel(int asic, int asicPixel);
        int calcPixelASIC(int pixel);


        void savePixelHistos();
        void clearPixelHistos();
        void updatePixelHistos();

        void updateAcquireCnt();
        bool checkOutputDirExists();
        bool checkDirExists(const std::string & path);
        
        void resetHistograms(){m_nextHistoReset = true;}
        void resetThresholdMap(){m_nextThresholdReset = true;}
        void clearHistograms();
        void clearThresholdMap();

    private:
        
        inline float rawData(int frame, int pixel, const  uint16_t & value) const;   
        inline float gccWrapCorrect(int frame, int pixel, const  uint16_t & value) const;   
        inline float sramCorrectData(int frame, int pixel, const uint16_t & value) const;
        inline float offsetCorrectData(int frame, int pixel, const uint16_t & value) const ;  
        inline float offsetTHCorrectData(int frame, int pixel, const uint16_t & value) const ;   
        inline float fullCorrectData(int frame, int pixel, const uint16_t & value) const ;  
        inline float fullCorrectTHData(int frame, int pixel, const uint16_t & value) const ;  
        
        std::function<float(int,int,const uint16_t & )> getCorrectionFunction();
        
        void initStatsAcc();
        void updateStatsAcc();
        void updateCorrectionMap();
        void storeMeanAndRMSImage();
        
        DsscTrainSorter* m_trainSorter;
                
        /// thread for actual work
        boost::shared_ptr<boost::thread> m_writingThread;
        
        std::vector<boost::thread> m_writeHDFThreads;
               
        bool m_isStopped;
        bool m_acquireBG;
        
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
        static std::vector<int> sortMap;        
        std::vector<int> m_pixelNumbers;        
        std::vector<unsigned int> m_pixelData;
        std::vector<unsigned int> m_thresholdMap;
        std::vector<utils::StatsAcc> m_dataStatsAcc;
        std::vector<utils::DataHisto> m_pixelHistoVec;
        std::vector<std::vector<unsigned long>> m_sramCorrectionAcc;
        std::vector<float> m_pixelBackgroundData;
        std::vector<std::vector<float>> m_pixelCorrectionData;

        utils::DataHisto pixelHisto;
        utils::DataHisto asicHisto;
        
        DsscTrainData * currentTrainData;
        
        bool m_keepPolling;
        boost::shared_ptr<boost::thread> m_pollThread;
              
        int m_selAsic;
        int m_selFrame;
        
        bool m_ladderMode;
        int m_asicPixelToShow;
        int m_asicToShow;
        util::State lastState;
        bool m_nextHistoReset;
        bool m_nextThresholdReset;
        
        std::function<float(int,int,const uint16_t & )> m_correctionFunction;
        
    };
}

#endif
