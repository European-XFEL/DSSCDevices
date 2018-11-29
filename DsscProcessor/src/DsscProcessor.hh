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
#include <functional>

#include <karabo/karabo.hpp>

#include "DsscTrainDataProcessor.h"

/**
 * The main Karabo namespace
 */
namespace karabo {

   class DsscProcessor : public karabo::core::Device<> , public utils::DsscTrainDataProcessor {

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

        void accumulate();
        void stop();
        void resetCounters();
        void setEmptyInjectCycles();

        void clearSramBlacklist();
        void clearSramCorrection();

    private:
        void changeDeviceState(const util::State & newState);

        void initialization();
        void updateSelPixelSramBlacklist();


        void onData(const karabo::util::Hash& data,
            const karabo::xms::InputChannel::MetaData& meta);

        void processTrain(const karabo::util::NDArray& data, const karabo::util::NDArray& cellId, const karabo::util::NDArray& trainId);

        void processTrainXFELDAQ(const karabo::util::NDArray& data, const karabo::util::NDArray& cellId, const karabo::util::NDArray& trainId);


        void sendPixelData(const unsigned short* train_data_ptr, unsigned long long train_id);

        void sendMeanValues();

        void clearData();
        
        void UpdateVariablesConfig(const karabo::util::Hash&);

        void fillImageData(std::vector<unsigned int> & imageData);
        
        enum class SourceTypeEnum {DUMMY, DATARECEIVER, XFELDAQ};
        
        SourceTypeEnum m_sourceType;
        boost::function<void(const karabo::util::Hash& data,
                             const karabo::xms::InputChannel::MetaData& meta)> m_onDataImpl;
        
        void onDataDummy(const karabo::util::Hash& data,
                         const karabo::xms::InputChannel::MetaData& meta);
        void onDataXFELDAQ(const karabo::util::Hash& data,
                         const karabo::xms::InputChannel::MetaData& meta);
        void onDataDataReceiver(const karabo::util::Hash& data,
                         const karabo::xms::InputChannel::MetaData& meta);

        void vizThreadFunc();
        void startVizThread();
        void stopVizThread();

        static constexpr size_t m_numCols = 512;
        static constexpr size_t m_numRows = 128;
        static constexpr size_t m_imageNumPixs = m_numCols*m_numRows;


        std::string m_sourceId;
        std::vector<unsigned long long> m_trainIds;

        std::vector<uint16_t> m_dataViz; //array for data vizualization
        uint16_t m_frameIndxViz = 0;
        std::thread m_vizThread;
        bool m_vizThreadRun = false;    
        bool m_vizualize = false;        //settings variable
    };
}

#endif
