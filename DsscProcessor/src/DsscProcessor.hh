/*
 * Author: <haufs>
 *
 * Created on January, 2018, 02:49 PM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_DSSCPROCESSOR_HH
#define KARABO_DSSCPROCESSOR_HH

#include <string>
#include <vector>
#include <chrono>


#include <karabo/karabo.hpp>

#include "DsscTrainDataProcessor.h"

/**
 * The main Karabo namespace
 */
namespace karabo {

    class DsscProcessor : public karabo::core::Device<>, public utils::DsscTrainDataProcessor {
    public:

        // Add reflection information and Karabo framework compatibility to this class
        KARABO_CLASSINFO(DsscProcessor, "DsscProcessor", "3.0")

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
        DsscProcessor(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed
         */
        virtual ~DsscProcessor();

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


        /**
         * This function acts as a hook and is called after an reconfiguration request was received,
         * and AFTER this reconfiguration request got merged into this device's current state.
         * You may access any (updated or not) parameters using the usual getters and setters.
         * @code
         * int i = get<int>("myParam");
         * @endcode
         */
        virtual void postReconfigure();

        /**
         * starts the receiving of data from the input node
         * several accumulation modes are available:
         * - mean values over the given sram range
         * - rms values over the given sram range
         * - pixel data over the full sram range
         * if sram correction or blacklist values are available they will also be applied before the data is
         * sent out via the p2p output
         * background values are only applied to the pixel data
         */
        void accumulate();

        void saveHistograms();

        void stop();

        /**
         * Resets device counters and member variables
         */
        void resetCounters();

        /**
         * This functions sets the sramBlacklist according to the selected value in the empty inject cycles and the injectOffset parameters
         * If the sramBlacklistFileName is not empty the entries of the file are added on top of the empty inject cycles excludes
         */
        void setEmptyInjectCycles();

        /**
         * Clears the sram blacklist if not further needed
         */
        void clearSramBlacklist();


        /**
         * Clears the sram correction and background values, background values are used only for debugging.
         * Sram correction is applied on the pixel data or the mean values if available
         */
        void clearSramCorrection();

        //void acquireSramCorrectionAndBaselineValues();

        /**
         * This functions updates the exising baseline values, by computing new mean values and overriding the existing values
         * is sram blacklists or sram correction data is available, the data will be applied also the new baseline values
         */
        void acquireBaselineValues();
        //void acquireSramCorrectionValues();

        void runHistogramAcquisition();

    private:
        void changeDeviceState(const util::State & newState);

        void initialization();
        void updateSelPixelSramBlacklist();


        void onData(const karabo::util::Hash& data,
                const karabo::xms::InputChannel::MetaData& meta);

        void processTrain(const karabo::util::NDArray& data, const karabo::util::NDArray& cellId, const karabo::util::NDArray& trainId);

        void sendPixelData(const unsigned short* train_data_ptr, unsigned long long train_id);

        void sendMeanValues();
        void savePixelHistos();
        void saveMeasurementInfo();
        void displayPixelHistogram();

        void clearData();

        void startPreview();
        void stopPreview();
        void updatePreviewData(const karabo::util::Hash& data);
        void previewThreadFunc();


        constexpr static uint m_imageSize = 512 * 128;

        std::string m_sourceId;
        std::vector<unsigned long long> m_trainIds;

        bool m_run;
        bool m_preview;
	uint16_t m_previewCell;
        bool m_previewMaximum;        
        uint32_t m_previewMaxCounter;        
        uint32_t m_previewMaxCounterValue;

        std::thread m_previewThread;
        std::mutex m_previewMutex;

        bool m_acquireHistograms;
        bool m_acquireBaseLine;
        bool m_acquireSramCorrection;
        utils::DataHistoVec m_pixelHistoVec;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_starttime;

        std::vector<uint16_t> m_previewImageData;
    };
}

#endif
