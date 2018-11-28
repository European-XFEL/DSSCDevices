/*
 * Author: <kirchgessner>
 *
 * Created on February, 2018, 01:04 PM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_DSSCASICPROCESSOR_HH
#define KARABO_DSSCASICPROCESSOR_HH

#include <karabo/karabo.hpp>
#include "DataHisto.h"

/**
 * The main Karabo namespace
 */
namespace karabo {

    class DsscAsicProcessor : public karabo::core::Device<> {

    public:

        // Add reflection information and Karabo framework compatibility to this class
        KARABO_CLASSINFO(DsscAsicProcessor, "DsscAsicProcessor", "2.0")
        using DataHistos = std::vector<utils::DataHisto>;
        static const size_t s_NUMPX = 4096;

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
        DsscAsicProcessor(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed
         */
        virtual ~DsscAsicProcessor();

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
    
    private:// slots
        void stop();
        void accumulate();
        void resend();

    private:
        void initialization();
        void resetCounters();
        
        void onData(const karabo::util::Hash& data,
            const karabo::xms::InputChannel::MetaData& meta);
        
        util::Hash fillOutputSchema(int asic_idx);

        void fillPixelHistos(const karabo::util::NDArray& data);
        
        void clearHistos();
        
        DataHistos m_pixelHistos;
        std::vector<unsigned long long> m_trainIds;
        int m_iterationCnt;
        int m_numIterations;
        bool m_run;
        
        unsigned short m_minSram,m_maxSram;
        util::Hash m_lastSendHash;
    };
}

#endif
