/*
 * Author: <kirchgessner>
 *
 * Created on January, 2018, 12:44 PM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_DSSCDUMMYTRAINGENERATOR_HH
#define KARABO_DSSCDUMMYTRAINGENERATOR_HH

#include <karabo/karabo.hpp>

/**
 * The main Karabo namespace
 */
namespace karabo {

    class DsscDummyTrainGenerator : public karabo::core::Device<> {

    public:

        // Add reflection information and Karabo framework compatibility to this class
        KARABO_CLASSINFO(DsscDummyTrainGenerator, "DsscDummyTrainGenerator", "2.0")

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
        DsscDummyTrainGenerator(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed
         */
        virtual ~DsscDummyTrainGenerator();
    
        void closeRunThreads();

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

        
        
        private:
            
        void initialization(); 
        
        void start_cont_mode();
        void stop();
        void run();
        void start_n_trains();
        void send_train();

        static std::vector<unsigned short> m_train_data;      
        static std::vector<unsigned short> m_cell_id; 
        unsigned short m_train_id;
        std::vector<std::thread*> m_runthreads;
       
       
    };
}

#endif
