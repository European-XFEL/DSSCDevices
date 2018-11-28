/*
 * Author: <kirchgessner>
 *
 * Created on January, 2018, 12:44 PM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <thread>

#include "DsscDummyTrainGenerator.hh"

using namespace std;

USING_KARABO_NAMESPACES;

namespace karabo {

  
    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, DsscDummyTrainGenerator)

    void DsscDummyTrainGenerator::expectedParameters(Schema& expected) {
        
        STRING_ELEMENT(expected).key("channelId")
            .displayedName("Channel id")
            .description("DAQ channel id, e.g. /SCS_DET_DSSC/DET/{}CH0."
                         "Use {} to denote the position of the infix for each channel.")
            .assignmentOptional().defaultValue("/SCS_DET_DSSC/DET/0CH0")
            .commit();  
      
        SLOT_ELEMENT(expected)
                .key("start_cont_mode").displayedName("Start ContMode").description("Send Continuously @10Hz")
                .allowedStates(State::STOPPED)
                .commit();
        
        SLOT_ELEMENT(expected)
                .key("stop").displayedName("Stop Sending").description("Stop sending dummy trains")
                .allowedStates(State::ON)
                .commit();
        
        SLOT_ELEMENT(expected)
                .key("start_n_trains").displayedName("Send N Trains").description("Send defined number of trains.")
                .allowedStates(State::STOPPED)
                .commit();
        
        UINT64_ELEMENT(expected)
                .key("num_trains_to_send").displayedName("Number of Trains to Send")
                .description("Select number of dummy trains to send")
                .assignmentOptional().defaultValue(10).reconfigurable()
                .allowedStates(State::STOPPED)                                    
                .commit();
        
        UINT32_ELEMENT(expected)
                .key("send_pause").displayedName("Send Pause [ms]")
                .description("Time to wait between two trains, adjust value to 10 Hz train sending, in ms")
                .assignmentOptional().defaultValue(100).reconfigurable()
                .commit();
        
        UINT64_ELEMENT(expected)
                .key("trainId").displayedName("Current train Id")
                .description("Current Train ID")
                .readOnly()
                .commit();
        
        Schema data;
        
         NODE_ELEMENT(data).key("image")
                .displayedName("Image")                                                                           
                .commit();
         
        NDARRAY_ELEMENT(data).key("image.data")
                .shape("800,16,64,64")
                .readOnly()
                .commit();                
        
        NDARRAY_ELEMENT(data).key("image.cellId")
                .shape("800")
                .readOnly()
                .commit();  
        
        NDARRAY_ELEMENT(data).key("image.trainId")
                .shape("800")                
                .readOnly()
                .commit();  
        
        OUTPUT_CHANNEL(expected).key("dummyTrainOutput")
        .displayedName("Dummy Train Data")
        .dataSchema(data)
        .commit();
        

                 
    }


    std::vector<unsigned short> DsscDummyTrainGenerator::m_train_data(16*4096*800);      
    std::vector<unsigned short> DsscDummyTrainGenerator::m_cell_id(800); 
    
    DsscDummyTrainGenerator::DsscDummyTrainGenerator(const karabo::util::Hash& config) : Device<>(config),
            m_train_id(0)
    {        
      KARABO_INITIAL_FUNCTION(initialization)
      
      KARABO_SLOT(start_cont_mode);
      KARABO_SLOT(stop);
      KARABO_SLOT(start_n_trains);

    }


    DsscDummyTrainGenerator::~DsscDummyTrainGenerator() {
      stop();
    }
    
    void DsscDummyTrainGenerator::closeRunThreads()
    {
      for(int i=0; i<m_runthreads.size(); i++){
        if(m_runthreads[i]->joinable()){
          m_runthreads[i]->join();
        }
        delete m_runthreads[i];
      }
      m_runthreads.clear();
    }


    void DsscDummyTrainGenerator::preReconfigure(karabo::util::Hash& incomingReconfiguration) {
    }


    void DsscDummyTrainGenerator::postReconfigure() {
    }
    
    void DsscDummyTrainGenerator::initialization() { 
      
      updateState(State::STOPPED);

      size_t idx=0;
      for(int frame=0; frame<800; frame++){
        m_cell_id[frame] = frame;
        uint16_t frameMask = frame << 12;
        for(int asic=0; asic<16; asic++){
          uint16_t asicMask = frameMask + (asic<<8);
          /*
          for(int px=0; px<4096; px++){
            m_train_data[idx++] = asicMask + px%256;
          }*/
          for(int y = 0; y < 64; y++){
            for(int x = 0; x < 64; x++){
               m_train_data[idx++] = ((x & y) ? frame : x);
            }
          }
        }
      }
    }
    
    void DsscDummyTrainGenerator::start_cont_mode()
    {
      updateState(State::ON);
      m_runthreads.push_back(new std::thread(&DsscDummyTrainGenerator::run,this));
    }
        
    void DsscDummyTrainGenerator::stop()
    {
      updateState(State::CLOSING);
      
      closeRunThreads();
    }
    
    void DsscDummyTrainGenerator::run()
    {
      while(getState() == State::ON){
        send_train();
      }
      updateState(State::STOPPED);
    }
    
        
    void DsscDummyTrainGenerator::start_n_trains()
    {
      updateState(State::ON);
      unsigned long long train_cnt = 0;
      unsigned long long num_trains_to_send = get<unsigned long long>("num_trains_to_send");
      while(getState() == State::ON && train_cnt < num_trains_to_send){
        send_train();
        train_cnt++;
      }
      updateState(State::STOPPED);
      
      signalEndOfStream("dummyTrainOutput");
    }
    
    void DsscDummyTrainGenerator::send_train()
    {      
      NDArray imageData(m_train_data.data(),m_train_data.size(), Dims(800,16,64,64));
      NDArray cellIdData(m_cell_id.data(),m_cell_id.size(), Dims(m_cell_id.size()));
      NDArray trainIdData(&m_train_id,1, Dims(1));

      util::Hash node;      
      node.set("data",imageData);
      node.set("cellId",cellIdData);    
      node.set("trainId",trainIdData);  
      
      util::Hash dummy_train_out;
      dummy_train_out.set("image",node);
      
      writeChannel("dummyTrainOutput",dummy_train_out); 
      
      set<unsigned long long>("trainId",m_train_id);
      
      m_train_id++;
      usleep(get<unsigned int>("send_pause")*1000); // 100ms
    }
}
