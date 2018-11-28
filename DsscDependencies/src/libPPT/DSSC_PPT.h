
/**
 * @file   DSSC_PPT.h
 * @Author Manfred Kirchgessner (Manfred.Kirchgessner@ziti.uni-heidelberg.de)
 * @date   August 2015
 * @brief  DSSC_PPT main control api.
 *
 * Full control of the DSSC Ladder including the PPT, IOB, Regulators, Clockbuffers,
 * and the ReadoutASICs
 *
 */

#ifndef PPTDEVICEAPI_HH
#define	PPTDEVICEAPI_HH

#include <iostream>
#include <map>
#include <string>
#include <array>
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>

#include "MultiModuleInterface.h"

#ifdef HAVE_HDF5
  #include "DsscHDF5Writer.h"
#endif

namespace SuS {

    struct LMKClockSettings
    {
      LMKClockSettings() : div(1),delay(0),mux(0),clkOut(0),reset(0),lmk_num(0),en(1){}
      uint32_t getRegData() const{
        int res = reset;
        if(clkOut != 0){
          res = 0;
        }
        uint32_t regValue = ((res&0x1)<<31)+
                            ((mux&0x3)<<17)+
                            ((en&0x1)<<16)+
                            ((div&0xFF)<<8)+
                            ((delay&0xF)<<4)+
                            (clkOut&0xF);
        return regValue;
      }

      void print() const { std::cout << "lmk = " << lmk_num << " clkOut = " << clkOut << " div = " << div << " delay = " << delay << " mux = " << mux << " en = " << en << std::endl;}


      void setDefault(){div=1;delay=0;mux=0;en=1;reset=0;}
      void setDelayed(bool en = true){ mux = (en)? 2 : 0;}

      int div,delay,mux,clkOut,reset,lmk_num,en;
    };

    class LMKSettings : public std::array<LMKClockSettings,16>
    {
    public:
        static const std::vector<uint32_t> clk_num;
        static const std::vector<uint32_t> lmk_fast;
        static const std::vector<uint32_t> lmk_num;

      LMKSettings(bool fast){
        for(int i=0; i<16; i++){
          operator[](i).clkOut = clk_num[i];
          operator[](i).lmk_num = (fast)? lmk_fast[i] : lmk_num[i];
          operator[](i).en = 1;
        }
      }

      void setDefault(){ for(int i=0; i<16; i++) operator[](i).setDefault(); }
      void enableAll(){  for(int i=0; i<16; i++) operator[](i).en = 1; }
      void disableAll(){ for(int i=0; i<16; i++) operator[](i).en = 0; }
      void setReset(bool res){operator[](0).reset = (res?1:0);}
    };


    class DSSC_PPT : public SuS::MultiModuleInterface
    {
    public:

        static const int c_totalXSelBits = 32; // register x2
        static const int c_totalYSelBits = 64;
        static const int c_PPS           = 8184; //Packet Payload Size

        static const uint8_t c_SIB_SEND_TEMP_CMD = 21;
        static const uint8_t c_SIB_READ_STAT_CMD = 17;
        static const uint8_t c_SIB_SEND_DONE_CMD = 35;

        static const uint16_t c_IOB_DUMMY_DATA_TESTPATTERN = 0x2;
        static const uint16_t c_DR_DUMMY_DATA_TESTPATTERN  = 0x1;

        //static const uint32_t LMK_RES_0;   // in REG0 bit 31 is the reset bit is programmed automatically by lmk module
        static const uint32_t LMK_REG9;     // 0x00032A09 would be Vboost enabled
        static const uint32_t LMK_REG14N;   // Reg14 in normal Mode Global Clock enable = 1  PowerDown = 0
        static const uint32_t LMK_REG14R;   // Reg14 in reset  Mode Global Clock enable = 0  PowerDown = 1
      //static const uint32_t LMK_REG14D;   // Reg14 in diable Mode Global Clock enable = 0  PowerDown = 0

        enum SETUPCONFIG {HAMBURG,MANNHEIM};

        /** Defines the actual Setup Configuration.
         *  Possible configurations are:
          * MANNHEIM: this setup has a different pin connection to the JTAG Chain of the readout ASIC,
          * and also the possibility to read the IOBoard temperature sensor.
          * HAMBURG: this is the final ladder configuration.
         *
         */
        static SETUPCONFIG actSetup;

        enum PixelChainMode {
            PXMODE_NONE = 0,
            PXMODE_ALLSAME = 1,
            PXMODE_CHAIN = 2,
        };




        typedef boost::shared_ptr<DSSC_PPT> DSSC_PPT_Pointer;

        /* Error codes returned by this class. */
        enum ErrorCode {
            ERROR_OK                   = 0x0000,
            ERROR_PPT_CONNECT_FAILED   = 0x0100,
            ERROR_PPT_CLIENT_EXCEPTION = 0x0101,
            ERROR_FTP_CONNECT_FAILED   = 0x0102,
            ERROR_FTP_SEND_FILE_FAILED = 0x0104,
            ERROR_FTP_READ_FILE_FAILED = 0x0108,
            ERROR_PLL_NOT_LOCKED       = 0x0110,
            ERROR_TCP_SEND_CMD_FAILED  = 0x0120,
            ERROR_FAST_INIT_FAILED     = 0x0140,
            ERROR_PARAM_UNKNOWN        = 0x0200,
            ERROR_PARAM_ILLEGAL_TYPE   = 0x0201,
            ERROR_PARAM_ILLEGAL_VALUE  = 0x0202,
            ERROR_PARAM_UNSET          = 0x0204,
            ERROR_PARAM_VECTOR_LENGTH  = 0x0208,
            ERROR_PARAM_NO_METHOD      = 0x0210,
            ERROR_PARAM_SET_FAILED     = 0x0220,
            ERROR_PARAM_GET_FAILED     = 0x0230,
            ERROR_PARAM_READBACK       = 0x0401,
            ERROR_FILE_NOT_FOUND       = 0x0801,
            ERROR_IOB_NOT_FOUND        = 0x1001,
            ERROR_ASIC_INIT_ERROR      = 0x1002,
            ERROR_ASIC_PLL_NOT_LOCKED  = 0x1004,
            ERROR_AURORA_NOT_LOCKED    = 0x1008,
            ERROR_UDP_BIND_ERROR       = 0x2000,
            ERROR_UDP_CREATE_ERROR     = 0x2001,
            ERROR_UDP_OPT_ERROR        = 0x2002
        };

        DSSC_PPT(PPTFullConfig * fullConfig);

        /** Class constructor.
         *  All class variables are initialized. No interaction with the hardware is triggered.
         *  CurrentModule is set to 1. The connection to the PPT must be opened seperately using the
         *  openConnection() function.
         *  @param epcReg already initialized ConfigReg structure holding the EPC Register information
         *  @param iobReg already initialized ConfigReg structure holding the IOB Register information
         *  @param jtagReg already initialized ConfigReg structure holding the JTAG Register information
         *  @param pixelReg already initialized ConfigReg structure holding the Pixel Register information
         *  @param seq already initialized ConfigReg structure holding the Sequencer information
         */
        DSSC_PPT(ConfigReg* epcReg,  ConfigReg* iobReg,
                 std::vector<ConfigReg*> moduleJtagRegs, std::vector<ConfigReg*> modulePixelRegs,
                 Sequencer* seq);

        /** Class destructor.
         *  Closes the tcp connection to the PPT SlowControlServer.
         *  deletes all ConfigRegs and also the Sequencer.
         */
        ~DSSC_PPT();

        /**
         * Keep the last error inforamtion. Public string that can be read out from outside.
         */
        std::string errorString;

        /**
         * Keeps error inforamtion. Don't forget to read it out sometimes to stay informed.
         * Must be cleared from outside by errorMessages.clear().
         */
        std::vector<std::string> errorMessages;

        /**
         * After checking the connected IOBs once, this vector keeps the numbers of the detected IOBs.
         * (Possible values are 1-4).
         */
        std::vector<int> activeIOBs;


        /**
         * Variable that keeps the number of Pixels of a connected readoutASIC. In an F1 Chip this is 4096.
         */
        inline int getNumPxs() const {return numOfPxs;}

         /**
         * Variable that keeps the number of Images that can be stored in a readout ASIC. In an F1 Chip this is 800.
         */
        inline int getSramSize() const {return sramSize;}

        /**
         * Wrapper function to set the EnBusInj bit in the Pixel register.
         * @param enable boolean value.
         * @param pixel select pixel register to change. Possible values are "1-20", 1;5;8;10, "all", "0-4096".
         */
        inline void enBusInj(bool enable, const std::string & pixel = "all"){CHIPInterface::pixelRegisters->setSignalValue("Control register", pixel, "EnBusInj", enable);}

        /**
         * Wrapper function to set the EnBusInjRes bit in the Pixel register.
         * @param enable boolean value.
         * @param pixel select pixel register to change. Possible values are "1-20", 1;5;8;10, "all", "0-4096".
         */
        inline void enBusInjRes(bool enable, const std::string & pixel = "all"){CHIPInterface::pixelRegisters->setSignalValue("Control register", pixel, "EnBusInjRes", enable); }

        /**
         * Wrapper function to set the EnPxInjDC bit in the Pixel register.
         * @param enable boolean value.
         * @param pixel select pixel register to change. Possible values are "1-20", 1;5;8;10, "all", "0-4096".
         */
        inline void enPxInjDC(bool enable, const std::string & pixel = "all"){CHIPInterface::pixelRegisters->setSignalValue("Control register", pixel, "EnPxInjDC", enable);}

        /**
         * Pure virtual function that is implemented in the QT class.
         * @param pixel select the memory block of specified pixel.
         * @return memory array of readback data
         */
        //* virtual functions, to be implemented in karabo or QT class *//
        virtual const uint16_t *getPixelSramData(int pixel) = 0;

        /**
         * Pure virtual function that is implemented in the QT class.
         * @return memory array of trailer data that is read from the readout ASIC.
         */
        virtual const uint16_t *getTrailerData() = 0;

        /**
         * Pure virtual function to fill a data class that is reuqired in the DataPacker.
         * All important control bits and their names are stored.
         */
        virtual void generateInitialChipData(DataPacker *packer) = 0;
        virtual void generateCalibrationInfo(DataPacker *packer) = 0;

        /**
         * Pure virtual function to trigger one burst and readout and store the data in an internal memory array.
         * If a DataPacker is given the new data is also stored in a root file on harddisk.
         */
        virtual bool doSingleCycle(DataPacker* packer=NULL, bool testPattern = false) = 0;
        virtual bool doSingleCycle(int numTries, DataPacker* packer=NULL, bool testPattern = false) = 0;

        void waitSingleCycle();

        virtual bool checkIOBDataFailed();


        /** Change the address of the PPT.
          * Address where the software tries to connect to.
          * @param host New Address to connect (eg "192.168.0.116").
          * @param port Optional port value (default 2384).
          * @see getPort()
          * @see getHostAddress()
          * @see setHostAddress()
          * @see openConnection()
          * @return void
          */
        void setPPTAddress(const std::string& host, uint16_t port=2384);

        /** Get the current address of the PPT.
          * @see getPort()
          * @see setHostAddress()
          * @see openConnection()
          * @return string of the current PPT address.
          */
        inline std::string getHostAddress() const {return pptTcpAddress;}

        /** Get the currenct address of the PPT.
          * @see setHostAddress()
          * @see getHostAddress()
          * @see openConnection()
          * @return returns the port number
          */
        inline uint16_t getPort() const { return tcpPort;}

        /** Connect to the TCP server of the PPT. Uses the pptAddress and port that can be specified by setHostAddress().
          * @see closeConnection()
          * @see isOpen()
          * @return DSSC_PPT error code, ERROR_OK or other error condition
          *
          */
        int openConnection();

        /**
          * Closes the client connection to the PPT. All subsequent transactions to
          * the device will fail until open() is called again.
          *
          * @return DSSC_PPT error code, ERROR_OK or other error condition
          */
        int closeConnection();

        /**
          * Resets the connection. Calls closeConnection() waits one second and reopens the same connection by calling
          * openConnection()          *
          */
        int resetConnection();

        /**
          * Check if a connection to PPT device is open.
          * @return boolean value true = connection is Open.
          */
        bool isOpen() const {
            return m_opened;
        }

        bool isHardwareReady() {return isOpen();}

        std::string getConnectedETHChannels();
        /**
         *  Send a string to the TCP server on the PPT.
          * The string that is sent is defined in the internal variable 'dataString'.
          * In contrast to sendReadPacket() and sendReadULPacket() this function does not wait for the answer of the PPT.
          * @see sendReadPacket()
          * @see sendReadULPacket()
          * @return returns true if all bytes could be send to PPT, false if not.
          */
        bool sendPacket();

        /**
         *  Send a string to the TCP server on the PPT.
          * The string that is sent is defined in the internal variable 'dataString'.
          * In contrast to sendPacket() this function waits for the answer of the PPT.
          * @see sendPacket()
          * @see sendReadULPacket
          * @return returns the integer number received by the PPT.
          */
        int  sendReadPacket(bool errId = false);

        std::vector<uint32_t> sendReadVectorPacket();

        /**
         *  Send a string to the TCP server on the PPT.
          * The string that is sent is defined in the internal variable 'dataString'.
          * In contrast to sendPacket() this function waits for the answer of the PPT.
          * Use this function if you want to read registers with 32 and more bits.
          * @see sendPacket()
          * @see sendReadULPacket
          * @return 64bit number received by the PPT.
          */
        uint64_t sendReadULPacket();

        /**
         *  Send a file over FTP to the PPT.
          * The fileName can be absolut or relative
          * @param fileName path to the file to send
          * @see readFileFtp()
          * @return DSSC_PPT error code, ERROR_OK or other error condition
          */
        int  sendFileFtp(const std::string & fileName);

        /** Read a file over FTP fram the PPT. Read file is stored in the current working directory.
          * The fileName must be relative to /tmp in the PPT.
          * @param fileName path to the file in the PPT to receive
          * @see sendFileFtp()
          * @return DSSC_PPT error code, ERROR_OK or other error condition
          */
        int  readFileFtp(const std::string & fileName);

        /**
          * Configures the cycle counters to fit the default values for a standard burst.
          * This function should be called at least once after the registers are loaded.
          *
          * start_wait_time   = 15.0 ms after start_burst signal
          * start gate driver =  3.0 ms before burst
          * switched pwr on   = 25.0 ??s before iprog
          *
          * the following register signals are set to realize these cycles:
          * EPC Register CMD_PROT_ENGINE:
          * pre_burst_wait_time and iob_powerup_time
          * IOB Registers PRB_power_on_delay and PRB_GDPS_delay.
          *
          * For the gate driver:
          * FET_controller_enable,FET_source_off_delay,FET_gate_off_delay,
          * FET_source_on_delay,FET_gate_on_delay.
          *
          * For the clear driver:
          * CLR_preclear_delay,CLR_on_offset,CLR_off_offset,
          * CLR_gate_on_offset,CLR_gate_off_offset,CLR_period,CLR_duty,
          * CLR_clr_control_en,CLR_clrdis_duty and CLR_clrdis_en_delay.
          */
        bool initCycleWaitCounters();


        void updateStartWaitOffset(int start_wait_offs) override;

        void updateCounterValues(int start_wait_time, int start_wait_offs, int gdps_on_time, int iprogLength, int burstLength, int refpulseLength,
                                 int fetOnTime, int clrOnTime, int clrCycle, int clrDuty, int powerOnTime,int iprog_clr_duty, int iprog_clr_offset,
         int iprog_clr_en = 0, int single_cap_en = 0);

        bool updateAllCounters();

        /**
          * Read Burst Cycle information from JTAG Register.
          * This function reads the register values from the master locations and writes their content into the slave locations in
          * the EPC CMD_PROT_ENGINE register.
          * Historically the JTAG Register is the master location
          * for the Burst parameters Burst Length, Iprog Length, Refpulse Length and Veto Latency.
          * And the sequencer is the master lcoation for the cycle lenght and the hold length.
          * The cycle counter values are stored in the PPT, the IOB and also in the ASIC.
          * At all three locations these values have to be consistent in order to run a working burst and readout.
          * This function synchronizhes the counter values for:
          * burst_length, cycle_length, hold_length, iprog_length, refpulse_length, veto_latency.
          * @see initEPCRegsFromJTAGConfigFile()
          * @see resetLocalRegisters()
          */
        void initIOBRegsFromJTAGConfigFile();

        /**
          * Read Burst Cycle information from JTAG Register and EPC register
          * This function reads the register values from the master locations and writes their content into the slave locations in
          * the IOB SYS_FSM registers.
          * Historically the JTAG Register is the master location
          * for the Burst parameters Burst Length, Iprog Length, Refpulse Length and Veto Latency.
          * The sequencer is the master lcoation for the cycle lenght and the hold length.
          * And the EPC Register is the master location for the pre burst wait cycles.
          * The cycle counter values are stored in the PPT, the IOB and also in the ASIC.
          * At all three locations these values have to be consistent in order to run a working burst and readout.
          * This function synchronizhes the counter values for:
          * SYSFSM_burst_length, SYSFSM_iprog_cyclelength, SYSFSM_iprog_length, SYSFSM_refpulse_length,
          * SYSFSM_burst_cyclelength, PRB_GDPS_delay = 5.
          * @see initEPCRegsFromJTAGConfigFile()
          * @see initCycleWaitCounters()
          * @see resetLocalRegisters()
          */
        void initEPCRegsFromJTAGConfigFile();

        /**
          * Initialize the local IOB register values to their reset value.
          * In order to keep the GUI in a consistant state, certain local register values are set to their
          * reset value.
          * Involved bits are:
          * PRB_en,PRB_power_off,PRB_num_prbs_found,PRB_manual_ctrl_en,
          * LMK_en, LMK_valid, LMK_dev_sel,LMK_data, ASIC_reset, PRB_control_sw_supplies_always_on.
          * @see resetLocalRegisters()
          * @see iobReset()
          */
        void initIOBRegsAfterReset();
        void initModuleIOBRegsAfterReset();
        /**
          * Initialize the local EPC register values to their reset value.
          * In order to keep the GUI in a consistant state, certain local register values are set to their
          * reset value.
          * Involved bits are:
          * EnJTAG1-4,RESET_ASIC1_N
          * @see resetLocalRegisters()
          */
        void initEPCRegsAfterReset();

        /**
          * Sets the link ID of the currentModule.
          * The internal variable currentModule keeps the information about the currently active module.
          * This can be changed by setActiveModule(). Link ID is the 48 bit value that is transmitted in the
          * train header of a module.
          * @param serial this is the IOB serial number of the current module
          * @see setActiveModule()
          */
        void setLinkID(uint32_t serial);


        uint64_t getCurrentTrainID();

        /**
          * Init complete system.
          * Performs a system reset, inits EPC Register, IOB Regsiters and Regulators and performs a fastASICInit,
          * also a full ASIC programming.After initSystem() the full system should be in a ready to measure status.
          * @see programEPCRegisters()
          * @see initIOBs()
          * @see initChip()
          * @see initASICs()
          * @see programLMKs()
          * @return DSSC_PPT error code, ERROR_OK or other error condition.
          */
        int  initSystem();
        int  initSingleModule(int currentModule);

        /**
          * Initialize IOBoards.
          * checks for available IOBoards and checks their regulators.
          * Also performs a full IOB register initialisation.
          * @see checkIOBAvailable()
          * @see programIOBRegisters
          * @return true if one ore more IOB could be successfully be initialized.
          */
        int initIOBs();
        int initCurrentIOB();

        /**
          * Initialize ASICs.
          * Disables power regulators of current module, and calls initASICs()
          * @see initASICs().
          */
        void initChip();

        /**
          * Initialize ASICs.
          * Performs an ASIC reset and a fastASIC initialization. Then initializes all ASIC registers.
          * @see fastASICInitTestSystem()
          * @see setASICReset()
          * @see setActiveModule()
          * @return true if all configuration files could be sent to current module PPT.
          */
        int initASICs();
        int initModuleASICs();

        /**
          * Initialize ASIC Readout.
          * Calls programLMK(), programJtag(), enableDataReadout().
          * Don't use this function. initSystem() does everything you need.
          * @see initSystem()
          * @see programLMK()
          * @see programJtag()
          * @see enableDataReadout()
          */
        void initAsicReadout();

         /**
          * Fast initialization routine to resolve DECCAP Bug.
          * this routine tries to bring the ASIC in a low power consumtion state as fast as possible.
          * This is realized by programming all pixel registers to default config and some JTAG regisers directly
          * after all powers have been switched on. If the low power state is reached the swichable voltages are switched off.
          * Now the ASIC can completely be initialized and all pixels can be programmed individually.
          * @see programmFullASICConfig()
          * @see doFastInit()
          * @see programFastInitConfig()
          * @return true if all configuration files could be sent to current module PPT.
          */
        int fastASICInitTestSystem();

        void setFastInitConfigSpeed(int setting){fastInitconfigSpeed = setting;}
        inline int  getFastInitConfigSpeed(){return fastInitconfigSpeed;}

        bool checkCorrectASICReset();
        bool checkCorrectASICResetCurrentModule();
        uint16_t getActiveChannelFailure();
        void resetDataFailRegister();
         /**
          * Generation and programming of the ASIC JTAG programming file.
          * One file (cmdsFromSoftware) is generated, containing all required bytes and is sent to the PPT.
          * The JTAG programming is triggered and all bytes are written into the ASIC registers.
          * Programmed registers are: JTAGRegisters,PixelRegistersInChain,Sequencer.
          * @param readBack enables readback and checking of programmed configuration.
          * @see fastASICInitTestSystem()
          * @see programmFullASICConfig()
          * @see programJtag()
          * @see programSequencer()
          * @see programPixelRegs()
          * @return true if all configuration files could be sent to current module PPT.
          */
        int  programmASICConfig(bool readBack);

        /**
          * Runs programmASICConfig() for all available IOBs (modules).
          * @param readBack enables readback and checking of programmed configuration.
          * @see programmASICConfig()
          * @return true if all configuration files could be sent to current module PPT.
          */
        int  programmFullASICConfig(bool readBack);

        /**
          * Generation and programming of the ASIC JTAG programming file.
          * One file (cmdsFromSoftware) is generated, containing all required bytes and is sent to the PPT.
          * The fast ASIC init is triggered and all bytes are written into the ASIC registers.
          * Programmed registers are: some JTAGRegisters,PixelRegistersAllSame.
          * @see fastASICInitTestSystem()
          * @see doFastInit()
          * @see programJtag()
          * @see programSequencer()
          * @see programPixelRegs()
          * @return true if all configuration files could be sent to current module PPT.
          */
        int programFastInitConfig();

        bool fastInitChip(){initChip(); return true;}

        /**
          * Checks if currentModule IOBoard is available and ready to read/write.
          * Reads a special register from the IOB. If the correct value is returned the
          * IOB is available.
          * @see setActiveModule()
          * @see isIOBAvailable()
          * @return int, ioboard revision number
          */
        int checkIOBAvailable(); //checks IOB register

        /**
          * Runs checkIOBAvailable() but remembers if it was already checked.
          * If yes it returns the previous value. Also sets the currentModule value to iobNumber.
          * @param iobNumber
          * @see setActiveModule()
          * @see checkIOBAvailable()
          * @see numAvailableIOBs()
          * @see getAvailableIOBoards()
          * @return int , 1 = IOB number is available
          */
        bool isIOBAvailable(int iobNumber);    //checks local register


        /** Returns the size of return vector of getAvailableIOBoards().
          * @see setActiveModule()
          * @see checkIOBAvailable()
          * @see isIOBAvailable()
          * @see getAvailableIOBoards()
          * @return int , number of found IOBs
          */
        uint numAvailableIOBs();

        /** Returns the numbers of the IOBoards in the system.
          * If already checked returns the previously found numbers.
          * If not checked, all modules are checked.
          * Counting starts at 1
          * @see setActiveModule()
          * @see checkIOBAvailable()
          * @see isIOBAvailable()
          * @see numAvailableIOBs()
          * @return vector of found IOBnumbers.Possible values are 1 - 4.
          */
        std::vector<int> getAvailableIOBoards();


        inline uint32_t getNumWordsToReceive() override {
          return getNumFramesToSend() * 16 * 4096;
        }

        void setNumWordsToReceive(int val, bool saveOldVal = true);

        /** Change number of frames to readout.
         *  Each frame contains 4096 16 bit words.
          * @param numFrames possible values 1-800
          */
        virtual void setNumFramesToSend(int val, bool saveOldVal = true);

        /** Change number of frames to readout.
         *
          * @return numFramesToSend possible values 1-800
          */
        inline uint32_t getNumFramesToSend() override {
          return epcRegisters->getSignalValue("AuroraRX_Control","0","num_frames_to_send");
        }


        /** Enable Burst Veto Offset. If disabled no vetos are sent at the beginning of a burst.
          * @param enable boolean value
          */
        void enableBurstVetoOffset(bool enable);

        /** Define number of Vetos at the beginning of a burst. Sets the number of vetoes that should be send
         * at the beginning of each burst. With this function the first pulses can be discarded in order to
         * acquire images of later events.
          * @param value number of events to discard. this number plus numFramesToSend
          *              should not be larger than the burst length value.
          */
        void setBurstVetoOffset(int value);

        bool isBurstVetoOffsetActive();

        /** Get the specified number of Vetos at the beginning of a burst.
          * return  number of events to discard.
          */
        int  getBurstVetoOffset();


        void setBurstLength(int burstLength);
        void setCycleLength(int cylceLength);
        void setIProgLength(int iprogLength);
        void enableHolds(bool enable);


        /** Get the number of UDP packets that are sent for the specified number of frames to send.
         * The number of frames to send is read out of the EPC Register AuroraRX_Control signal num_frames_to_send.
         * The UDP packet count includes the trailer packets, containing the image descriptors and detector data.
         * @see setNumFramesToSend()
         * @return number of UDP packets that are sent for the currently specified number of frames to send.
          */
        uint32_t  getNumPacketsToReceive();

        static uint64_t getCellIdsStartByte(int numFramesToSend);

        static uint64_t getPulseIdsStartByte(int numFramesToSend);
        static uint64_t getStatusStartByte(int numFramesToSend);
        static uint64_t getSpecificStartByte(int numFramesToSend);
        static uint64_t getASICTrailerStartByte(int numFramesToSend);
        /** This function has no functionality. It is needed by the F1 test setup.
          */
        void setMultiCyclesNum(int num, bool program = false){/*do nothing*/}

        void runContinuousMode(bool run);
        bool inContinuousMode();
        void disableSending(bool disable = true);

        void flushFtdi(){ /*do nothing*/}
        /** This function has no functionality. It is needed by the F1 test setup.
          */
        void setCHIPReadout(ChipReadout mode){}

        /** This function has no functionality. It is needed by the F1 test setup.
          */
        int getCHIPReadout(){ return 0;}

        /** This function has no functionality. It is needed by the F1 test setup.
          */

    /** This function has no functionality. It is needed by the F1 test setup.
          */
        void setWindowReadoutMode(bool en, bool program = false){}

        bool calibrateCurrCompDACForReticleTest(bool log=true, int singlePx=-1, int startSetting=0, int defaultValue = 3){return true;}

        bool writeFpgaRegisters(){return true;}

        uint32_t getIprogCycleLength();
        uint32_t getBurstCycleLength();


        ConfigReg * getRegisters(const std::string & regName)
        {
          if (regName.compare("epc") == 0){
            return epcRegisters;
          }else if (regName.compare("iob") == 0){
            return iobRegisters;
          }else if (regName.compare("jtag") == 0){
            return jtagRegisters;
          }else if (regName.compare("pixel") == 0){
            return pixelRegisters;
          }
          return nullptr;
        }

        /** Get the epcRegisters Object
          * @see epcRegisters
          * @return ConfigReg holding the EPC configuration
          */
        ConfigReg * getEPCRegisters(){return epcRegisters;}

        /** Get the iobRegisters Object
          * @see iobRegisters
          * @return ConfigReg holding the IOB configuration
          */
        ConfigReg * getIOBRegisters(){return iobRegisters;}

        /** Get the jtagRegisters Object
          * @see jtagRegisters
          * @return ConfigReg holding the ASIC JTAG configuration
          */
        ConfigReg * getJTAGRegisters(){return jtagRegisters;}

        /** Get the pixelRegisters Object
          * @see pixelRegisters
          * @return ConfigReg holding the ASIC Pixel configuration
          */
        ConfigReg * getPixelRegisters(){return pixelRegisters;}

        /** Get the sequencer Object
          * @see sequencer
          * @return Sequencer holding the ASIC Sequencer configuration
          */
        Sequencer * getSequencer(){return sequencer;}

        /** Read the PPT FPGA Temperature from the device.
          * @return Converted temperature in degree celsius.
          */
        int readFPGATemperature();

        /** Read the IOB Temperature from the external temperature sensor.
         *  Only valid for the MANNHEIM Test Setup.
          * @return Converted temperature in degree celsius, if Mannheim TestSetup, else 0.0.
          */
        double readIOBTemperature_TestSystem();

        /** Program the PPT PLL. Several configuration values are possible. Default PLL is programmed to
          * @param standAlone (defualt true) switch between XFEL C&C clock and standalone clock.
          * @param externalPll (defualt true) switch between external PLL or FPGA internal PLL
          * @param multiplier (defualt 28)
          * @param intValue (defualt 280)
          * @param fracValue (defualt 0)
          * @param modValue (defualt 25)
          */
        std::string programClocking(bool standAlone, bool externalPll, int multiplier, int intValue, int fracValue, int modValue);

        /** Set the PLL to generate the fast ASIC clock. The external PLL CHIP produces less jitter,
         *  the internal FPGA PLL allows different clock speeds.
          * @param intNotExt boolean value true = intern FPGA Pll, false = external PLL CHIP
          */
        void clockPLLSelect(bool intNotExt);

        /** Select the source for the fast clock. XFEL is the clock received by the C&C interfacte. Standalone means
         * the on board oscillator.
          * @param xfelNotStandalone boolean value: true = xfel C&C control, Standalone is the default configuration.
          */
        void clockSourceSelect(bool xfelNotStandalone);


        /** Check if PPT PLLis locked.
          * @return true if PLL is locked.
          */
        bool isPPTPllLocked();

        bool checkPPTPllLocked();

        void restartPPT();

        void moveRBFileOnPPT();

         /** Execute direct shell command from remote.
          *  It is not recommended to use this function if not sure what to do.
          * @param command string of the command to execute.
          * @param wait if true the function waits for the answer from the PPT SlowControlServer.
          */
        void executeShellCommand(const std::string & command, bool wait = false);

        /** Send the FLFI command to the PPT SlowControlServer.
          * If the firmare binary is found on the PPT this takes about 25 minutes.
          * @return true if file was found and binary was updated
          */
        bool sendFlashFirmware();

        /** Send the FLLI command to the PPT SlowControlServer.
          * If the linux binary is found on the PPT this takes about 20 minutes.
          * @return true if file was found and binary was updated
          */
        bool sendFlashLinux();

        /** Send the UPIF command to the PPT SlowControlServer.
          * If the IOB xsvf is found on the PPT the existing xsvf file is overwritten with the new one.
          * Run programIOBFPGA() to load the new firmware into the IOB.
          * @see uploadIOBFirmwareFile()
          * @return DSSC_PPT error code, ERROR_OK or other error condition
          */
        int  sendUpdateIOBFirmware();

        /** Send the UPIF command to the PPT SlowControlServer.
          * If the IOB xsvf is found on the PPT the existing xsvf file is overwritten with the new one.
          * Run programIOBFPGA() to load the new firmware into the IOB.
          * @param fileName path to new xsvf file.
          * @see sendUpdateIOBFirmware()
          * @return DSSC_PPT error code, ERROR_OK or other error condition
          */
        int  uploadIOBFirmwareFile(const std::string & fileName);

        /** Send the UPDS command to the PPT SlowControlServer.
          * If all required files are found on the PPT the existing binaries are overwritten.
          * If the SlowControlServer is updated connect via telnet to the PPT and restart the SlowControlServer to apply changes.
          * @return DSSC_PPT error code, ERROR_OK or other error condition
          */
        int  sendUpdateSoftware();


        bool isXFELMode();
        bool isSendingDisabled();

    public:
       /** Enable XFEL control of the PPT. This enables the C&C interface.
         * All VETOS,Start or Stop Signals can be received over the
         * connection. Manual Burst and Readout commands are disabled.
         * Also the fast clock is received and sampled from C&C.
         * This changes only the value of the Multi_purpose_Register CmdProtocolEngine_enable
         * @param enable boolean value.
         * @see enableManualControl()
        */
        void enableXFELControl(bool enable);


        uint8_t readSystemStatus();

       /** Manually triggers one single readout cycle.
         * This changes the value of the EPC Register CMD_PROT_ENGINE  start_readout_from_reg to 1 and then back to 0.
         * This is the slower version of the startReadout() function, which uses the special READ command.
         * @see startReadout()
        */
        void startSingleReadout();

       /** Manually triggers one full burst cycle by sending the special BURS command.
         * The full burst cycle contains all wait cycles specified in the IOB. The PRBs are switched accordingly
         * and after the burst one readout cycle is started, that sends out the data through the whole readout chain.
         * @see startReadout()
         * @see startSingleCycle()
         * @see initCycleWaitCounters()
        */
        void startBurst();

       /** Manually triggers one test pattern readout cycle by sending the special TEST command.
         * The serializer of the ASIC is configured to send after a trailing one test pattern data out.
         * The whole readout chain sends the testpattern out of the system
         * @see startReadout()
         * @see startSingleCycle()
         * @see initCycleWaitCounters()
        */
        void startTestPattern();


        void sendSingleTrain();


        uint32_t getExpectedTestPattern();
        void updateExpectedTestPattern(uint16_t testPattern);

       /** Manually triggers one single readout cycle.
         * This sends the special READ commant to the SlowControlServer.
         * This is the faster version of the startSingleReadout() function, which toggles the EPC register step by step.
         * @see startReadout()
        */
        void startReadout();

       /** Works only in inContinuousMode sends out one train and then continues without sending.
         * This changes the value of the EPC Register CMD_PROT_ENGINE  doSingleCycle to 0 and then to 1.
         * This function also sets the number of cycles to 1
        */
        void startSingleCycle();

        /** Enables the data readout. To be shure that data comes out of the system this function
         *  disables the ASIC_reset in the IOB (currentModule) and also sets the ASIC_readout_enable to one.
         * @see setActiveModule()
        */
        void enableDataReadout();

        void setStartBurstOffset(int offset);


        void setSendRawData(bool enable, bool reordered = false, bool converted = false);

       /** Enables the Ethernet engine to generate continously dummy packets until dummy packets are disabled.
         * the dummy packets do not contain the TrainHeader but the TTP trailer. The dummy data is the same 16 bit word
         * for all channels, counting upwards for each pixelword starting at 0.
         * @param enable true = start dummy packets, false = stop dummy packets.
         * @see enableDummyDRData()
         * @see enableDummyAsicData()
        */
        void enableDummyPackets(bool enable);

       /** Enables the PPT datareceiver to fill the readout chain with dummy data at the output of the aurora core.
        * This means the full datareceiver and the word reassembling is included. The dummy data is the same for every datareceiver.
        * The dummy words are just upwards counting 9 bit words, starting at 2.
         * @param enable true = send DR dummy data in readout cycles, false = stop sending DR dummy data in readout cycles
         * @see enableDummyPackets()
         * @see enableDummyAsicData()
        */
        void enableDummyDRData(bool enable);

       /** Enables the IOB datareceiver to fill the readout chain with dummy data at the output of the deserializer at the input of the IOB.
         * This means the full fifo stages and the aurora core are included. The dummy data is the same for every datareceiver.
         * The dummy words are just upwards counting 9 bit words, starting at 1.
         * @param enable true = send IOB dummy data in readout cycles, false = stop sending IOB dummy data in readout cycles
         * @see enableDummyDRData()
         * @see enableDummyPackets()
        */
        void enableDummyAsicData(bool enable);

        int getDummyDataMode();

        void disableAllDummyData();

        /** Checks if at least one datapath is enabled.
          * If no DP is enabled the readout chain is disabled and no data will be send out of the PPT.
          * DP = 1 - 4.
          * @see isDPEnabled().
          * @return true if at least one Datapath is enabled
          */
        bool singleDPEnabled();

        /** Checks if all 4 datapaths are enabled.
         *  @see isDPEnabled().
          * @return true if all 4 datapaths are enabled
          */
        bool allDPEnabled();

        /** Checks if a datapath is enabled.
          * Datapath enabling is required to receive data from the PPT.
          * Reads back EPC register Multi_purpose_Register from the PPT.
          * DP = 1 - 4
          * @param channel datapath channel 1-4 to
          * @see setDPEnabled()
          * @return true if specified datapath is enabled
          */
        bool isDPEnabled(int channel);

        /** Enables a datapath channel in the PPT
          * Datapath enabling is required to receive data from the PPT.
          * This function sets the EPC register Multi_purpose_Register signal datapath_channel_enable on the PPT.
          * DP = 1 - 4
          * @param channel datapath channel 1-4 to
          * @see isDPEnabled()
          */
        void setDPEnabled(int channel, bool enable);


        /** Programm the clock buffer on the mainboard.
          * This function programs all clock buffers on all available IOBs to default values.
          * @see programLMK()
          */
        void programLMKs();
        void programLMKsDefault();

        /** Programm the clock buffer on the mainboard.
          * This function programs the clock buffers on the currentModule to default values.
          * @see programLMK()
          */
        void programLMK();
        void programLMK(bool fast);
        void programLMKOut(const LMKClockSettings & lmkSettings);
        void programLMKGlobal(bool disable = false);
        void programLMKDefault();
        void disableLMKs();
        void disableLMK();
        void enableLMKs();
        void enableLMK();
        void setDefaultLMKConfig();
        void programLMKPowerDown(int lmk_number);


        /** Programm the clock buffer on the mainboard.
          * Programming the LMK is required to enable the fast ASIC clock.
          * It is always the LMK on the currentModule programmed. Per module there are 4 lmks.
          *
          * @param lmk_number values 1-4 valid
          * @param disableLMK disables the clock output of selected clock buffer
          * @param enSecondOut doues not have any function. All outputs are always set active
          * @param lmk_mux_sel (default 0) lmk mux control
          * @param lmk_div (default 1) lmk divider control
          * @param lmk_delay (default 0) lmk delay value
          * @see setActiveModule()
          * @see programLMK()
          * @see programLMKs()
          */
        void programLMKs(int lmk_number, bool disable);

        void programLMK(int lmk_number, const std::vector<uint32_t> & data_vec);
        void programLMKASIC(int asic, bool fast, bool disable);

        void setLMKConfig(int asic, bool fast, int div, int delay, int mux, bool disable);
        LMKClockSettings getLMKConfig(int asic, bool fast) {return (fast? lmkFastSettings[asic] : lmkSlowSettings[asic]);}


        void toggleLMKGlobalEn();

        /** Checks if the regulator board self detection is active.
         *  This is only possible in the MANNHEIM test setup. In the ladder setup the self detection doues not work yet.
         *  Checks if the signal PRB_self_detect is set in the PRB_control register.
         *  Does not read a value from the hardware.
         *  @return true if bit is set.
          */
        bool isPRBSelfDetected();

        /** Check the status of the power regulator boards for all available IOboards.
         * If self detect is set, the self detection cycle is started.
         * If self detect is off, nothing is done.
         *
         * @param resetPRBAnyWay if true the prb self detection cylce is started. If false the self detection cycle is only started if no PRB is found.
         * @see checkCurrentIOBPRBStatus()
         * @see readNumPRBs()
         * @see isPRBSelfDetected
         */
        void checkPRBStatus(bool resetPRBAnyWay=false);

        /** Check the status of the power regulator boards for the currentModule.
         * If self detection is on and number of PRBs is zero the self detection cycle is started.
         * If self detection is off, the self detection cycle is only started if resetPRBAnyWay is set to true.
         *
         * @param resetPRBAnyWay if true the prb self detection cylce is started. If false the self detection cycle is only started if no PRB is found.
         * @see checkPRBStatus()
         * @see readNumPRBs()
         * @see isPRBSelfDetected()
         * @return number of found prbs. If Self detection is off this function returns the number of specified PRBs.
         */
        int  checkCurrentIOBPRBStatus(bool resetPRBAnyWay=false);

        /** Reads the number of regulator boards of the current module.
         * The value is read from the currentModule IOB
         * @see checkPRBStatus()
         * @see readNumPRBs()
         * @see isPRBSelfDetected()
         * @see setActiveModule()
         * @return number of found prbs. If Self detection is off this function returns the number of specified PRBs.
         */
        int  readNumPRBs();

        /** Enables the digital static voltage in all available PRBs of all available IOBs.
         * Calling enablePRBStaticVoltages() for all available IOBs.
         * @param enable true = static voltage enabled.
         */
        void enablePRBStaticVoltages(bool enable);

        /** Enables the digital static voltage in all available PRBs of the current module.
         * Enabling is done by setting the PRB_control signal PRB_en to 1 and PRB_power_off to zero.
         * @param enable true = static voltage enabled.
         * @see enablePRBStaticVoltages().
         */
        void enablePRBStaticVoltage(bool enable);

        void setPRBPowerSelect(const std::string & asicsStr, bool program);
        std::string getPRBPowerSelect();

        /** Enables all 3 voltages of all available IOBs and all PRBs.
         * Calling setCurrentAllVoltagesOn() for all available IOBs.
         * @param enable true = all PRB voltages enabled.
         * @see setAllVoltagesOn().
         */
        void setAllVoltagesOn(bool enable);

        /** Enables all 3 voltages of the PRBs.
         * Enabling is done by setting the PRB_control_sw_supplies_always_on signal PRB_control_sw_supplies_always_on to 1.
         * @param enable true = all PRB voltages enabled.
         * @see setAllVoltagesOn().
         */
        void setCurrentAllVoltagesOn(bool enable);

         /** Function of the F1 Test System, does nothing
         */
        void setStaticVDDA(bool enVDDA){std::cout << "setStaticVDDA is not implemented" << std::endl;}
        /** Function of the F1 Test System, does nothing
         */
        void setStaticVDDDADC(bool enVDDDADC){std::cout << "setStaticVDDDADC is not implemented" << std::endl;}
        /** Function of the F1 Test System, does nothing
         */
        void setStaticVDDD(bool enVDDD){std::cout << "setStaticVDDD is not implemented" << std::endl;}

        int getStaticVDDA(){return 0;}
        int getStaticVDDDADC(){return 0;}
        int getStaticVDDD(){return 0;}

        /** Enables manual Voltage Control.
         * If enabled the values in the PRB_manual_ctrl registers become valid.
         * Manual Controlled PRB Voltages are valid only for the currentModule.
         * Enabling is done by setting the PRB_control signal PRB_manual_ctrl_en to 1.
         * @param enable true = manual control enabled.
         * @see setAllVoltagesOn().
         */
        void enableManualVoltageControl(bool enable);

        /** Manual Voltage Control.
         * If manual controll is enabled all 3 voltages of all 4 regulators on all 4 connected regulator boards can be
         * switched on and off. If allNotStatic is true all 3 voltages are switched on. If false only the static voltage is switched on.
         * If all voltages should be switched off, one can set the PRB_control signal PRB_power_off to 1.
         * @param prb PRB in chain 1 - 4. 4 is the last PRB in the chain.
         * @param supply 1 - 4 on selected PRB.
         * @param allNotStatic true = all, false = only static
         * @see enableManualVoltageControl().
         */
        void setManualVoltage(int prb, int supply, bool allNotStatic);

        /** Reset Manual Voltage Control Values.
         * Sets all PRB_manual_ctrl registers to 0.
         * @see enableManualVoltageControl().
         */
        void resetManualVoltages();

        /** Enable JTAG Engines. If a JTAG Engine is switched from zero to one, all bits in the input fifo are sent to the
         * ASICs in the chain at full speed. Speed can be selected by the EPCRegister JTAG_Control_Register signal ASIC_JTAG_Clock_Divider (default 13).
         * The Clock Divider divides the 100 MHz input clock of the JTAG Engine.
         * This function can be used to minimize the ASIC JTAG programming time, because the JTAG Engine is normally faster than the microblace filling the input fifo.
         *
         */
        void enableJTAGEngines(bool enable);

        /** Wait for the currentModule JTAG Engine done.
         *  This function triggers the SlowControlServer to test the JTAG Engine input fifo to be empty.
         *  If the fifo is empty the Server sends an answer, on which this function waits.
         * @see setActiveModule()
         */
        void waitJTAGEngineDone();


        void checkJtagEnabled();

        /** Sends a reset command to all available Modules JTAG Controllers.
          * @see resetASICJTAGController()
         */
        void resetASICJTAGControllers();


         /** Sends a command to the ASIC JTAG Engine in the PPT to perform a JTAG Reset in the ASICs.
          * the reset sequence is initiated by a "ff" command, sent to the JTAG Engine of the currentModule.
          * @see setActiveModule()
         */
        void resetASICJTAGController();
        void addResetToConfigVector();


        void jtagBypassTest(uint8_t pattern);

        /** Manual ASIC JTAG programming. Using this function, it is possible to send byte per byte to the currentModule
         *  ASIC Engine. This function can be used for debugging. The normal way to program the ASIC JTAG is by sending a
         *  comfiguration file cmdsFromSoftware, that includes all required JTAG Bytes.
         *  @param chainNumber same as currentModule, selects the ASIC JTAG chain.
         *  @param data 8 bits word. Next Byte to send to the ASIC JTAG chain.
         */
        void ASICJtagWrite(int chainNumber, std::vector<uint8_t> data);

        /** Set the ASIC reset. This minimized the power consumption of the readout ASICs.
         *  If set, the serializer, the state machine and the sequencer are resetted. The ASIC reset is realized by a
         *  low voltage level at a certain pin.
         *  If actSetup is set to MANNHEIM the setASICReset_TestSystem is also called.
         * @param reset true = reset, false = no reset.
         * @see setASICReset_TestSystem()
         */
        void setASICReset(bool reset);

        // toggle reset
        void resetChip(){
          setASICReset(true);
          setASICReset(false);
        }

        /** Set the ASIC reset. This minimized the power consumption of the readout ASICs.
         *  The serializer, the state machine and the sequencer are resetted. The ASIC reset is realized by a
         *  low voltage level at a certain pin. This function is only valid for the Mannheim TestSetup.
         * @param reset true = reset, false = no reset.
         */
        void setASICReset_TestSystem(bool reset);

        /** Resets the whole system.
         *  This function brings the system in a startup state. Several blocks are resetted:
         *  The IOBoard, the ddr3 controller, the ethernet engine, the aurora engine, the datareceiver,
         *  all epc registers are set to their reset value. To keep the GUI in a consistent state resetLocalRegisters()
         *  is called if reset is true.
         *  @param reset true = resetAll, false = release reset.
         *  @see iobReset()
         *  @see epcReset()
         *  @see datapathReset()
         *  @see resetLocalRegisters()
         */
        void resetAll(bool reset);
        void resetCurrentModule(bool reset);

         /** Sets the local register values to their rest value.
          * This function is called if resetAll is called with reset=true.
          * This function calls
          * initEPCRegsAfterReset() ,initIOBRegsAfterReset()
          * initEPCRegsFromJTAGConfigFile(), initIOBRegsFromJTAGConfigFile()
          * @see initEPCRegsAfterReset()
          * @see initIOBRegsAfterReset()
          * @see initEPCRegsFromJTAGConfigFile()
          * @see initIOBRegsFromJTAGConfigFile()
         */
        void resetLocalRegisters();

        /** Resets the datapath.
         *  This function resets the Aurora link. If it is not locked after resetAll, use this function to
         *  lock the aurora link.
         *  @param reset true = reset datapath, false = release reset.
         *  @see iobInitAurora()
         *  @see iobReset()
         *  @see epcReset()
         *  @see resetAll()
         *  @see resetLocalRegisters()
         */
        void datapathReset(bool reset);

        int checkAllASICPllLocked();
        int checkASICPllLocked();
        void iobAsicPllReset(bool reset);

        /** Resets the iob.
         *  This function enables the reset Pin to the IOB. Also the power regulators
         *  are switched off if IOB is resetted.
         * To keep the software in a consistent state the loacal IOB registers
         *  are updated to their reset values by calling initIOBRegsAfterReset().
         *  @param reset true = reset IOB, false = release reset.
         *  @see datapathReset()
         *  @see epcReset()
         *  @see resetAll()
         *  @see resetLocalRegisters()
         */
        void iobReset(bool reset);

        /** Resets the EPC registers.
         *  This function sets the EPC register in the PPT FPGA to their reset values.
         * Effected registers:
         *  Certain registers are not effected: JTAG_Engines, DataPath, All config Registers
         *  PLL Controller, JTAG_Engines_Control_Register,Multi_purpose_Register,IOB_Control_Engines that are resetted by the IOB Reset
         *  and the SerialNumberReader.
         *  @param reset true = reset IOB, false = release reset.
         *  @see datapathReset()
         *  @see epcReset()
         *  @see resetAll()
         *  @see resetLocalRegisters()
         */
        void epcReset(bool reset);

        /** Read the reset state.
         * @return boolean value.
         */
        bool readIOBReset();

        /** Read the reset state from hardware.
         * @return boolean value.
         */
        bool readAsicReset();

        /** Read the reset state from hardware.
         * @return boolean value.
         */
        bool readEPCReset();

        /** Read the reset state from hardware.
         * @return boolean value.
         */
        bool readDPReset();

        /** Read the reset state from hardware.
         *
         * @return Returns the PRB_power_off value of the currentModule prb
         */
        bool readPRBReset();

        /** Read the channel ready state from hardware.
         *
         * @return Returns the  channel ready state of the currentModule aurora channel.
         */
        bool isAuroraReady();

        bool checkAuroraReady();
        bool checkSingleAuroraReady();
        /** Reset the transmitting aurora channel in the IOB of the current module.
         *  The Aurora_reset bit is toggled to 1 and back to zero.         *  @see datapathReset()
         *  @see epcReset()
         *  @see resetAll()
         *  @see resetLocalRegisters()
         */
        void auroraTXReset();

        /** Initialize the aurora channel.
         *  Therefore the dataPath reset is toggled to 1 and back to 0.
         *  @see epcReset()
         *  @see resetAll()
         *  @see resetLocalRegisters()
         */
        void iobInitAurora();

        /** Reads ASIC PLL Locked status from currentModule IOB.
         * The ASIC PLL is connected to the fast 700 MHz clock.
         * @return boolean value.
         */
        bool isAsicPllLocked();

        /** Reads the statusbits of the Receiver Aurora from the PPT.
         * @param statusBits reference to the status bit vector. the vector keeps the status information
         * of the aurora core. See cpp file for bit information.
         * @return boolean value.
         */
        void readOutAuroraStatus(std::vector<bool> &statusBits);

        /** Reads the PPT serial number from the PPT. The serial number is stored in a file in the linux
         * directory tree: /etc/serial.
         * Serial larger than 0x18000000 belong to PPTv2.
         * @return uint64_t value.
         */
        uint64_t readSerialNumber();

        /** Reads the IOB serial number from the currentModule IOB.
         * The IOB serial number is read out from a ROM on the IOB automatically after IOBReset goes low.
         * The serial is stored in two 32 bit registers.
         * Serial larger than 0x16c00000 belong to IOB Rev2.
         * @return string value representing the hex value of the rserial number.
         */
        std::string getIOBSerial();
        uint64_t getIOBSerialNumber();

        /** Return the expected xors.
         */

        int SIBRead();
        int SIBFlush();
        void SIBWrite(int value);
        void setSIBLoopBackMode(bool enable);

        void SIBSendManualTemperatures();

        /** Base hardware communication function for all read access to the
         * EPC registers.
         * Sends the "EPCC R 0xXX" command.
         * To read an EPC register containing more than 32 bits, this function must be
         * called several times. The number of reads must fit exactly to the size of the register.
         * @param address Address to read.
         * @see EPCWrite()
         * @return integer value of the read data.
         */
        int  EPCRead(int address);

        /** Base hardware communication function for all write access to the
         * EPC registers.
         * Sends the "EPCC W 0xXX 0xXX" command.
         * To write an EPC register containing more than 32 bits, this function must be
         * called several times. The number of writes must fit exactly to the size of the register.
         * @param address Address to write.
         * @param data data word to write.
         * @see EPCRead()
         */
        void EPCWrite(int address, int data);
        void EPCWriteD(int address, int data);

        /** Program and readback all moduleSets of the EPC Registers.
         * @return boolean value, true if readback of all registers was correct.
         */
        bool programEPCRegisters();

        /** Program the EPC register with the given name.
         * @param moduleSetStr Name of the Module Set to program
         */
        void programEPCRegister(const std::string & moduleSetStr);


        /** Readback and check all EPC registers.
         * @return DSSC_PPT error code, ERROR_OK or ERROR_PARAM_READBACK
         */
        int  readBackEPCRegisters();

        /** Readback and check selected EPC register.
         * @param moduleSetName Select the EPC Register by name.
         * @param overwrite it true, all local register values are overwritten with the readback value.
         * @return boolean value, true = all readback data correct.
         */
        bool readBackEPCRegister(const std::string & moduleSetName, bool overwrite = true);

        /** Readback and check selected EPC register.
         * @param moduleSet Select the EPC Register by number.
         * @param overwrite it true, all local register values are overwritten with the readback value.
         * @return boolean value, true = all readback data correct.
         */
        //bool readBackEPCRegister(int moduleSet, bool overwrite = false);

        /** Trigger the programming of the selected IOB FPGA.
         * The programming is initialized by sending the executeShellCommand with the
         * following string "/opt/bin/xsvf_player -n 'X' /opt/IOB_Firmware.xsvf";
         * @param iobNumber Select the IOB Module to program.
         */
        void programIOBFPGA(int iobNumber);

        void setIOBSerialForTrainData(int iobNumber, uint32_t serial);
        void setQuadrantIdForTrainData(uint16_t quadrantId);
        void setQuadrantId(const std::string & quadrantId);

        /** Base hardware communication function for all read access to the
         * IOB registers. The IOB number to control is selected by the currentModule.
         * Sends the "IOBC R  0xXX " command.
         * All IOB registers consist of 32 bytes. So every register can be read/written at once.
         * @param address Address to read.
         * @see iobControl_write()
         * @see setActiveModule()
         * @return integer value of the selected IOB register.
         */
        int  iobControl_read(int address);

        /** Base hardware communication function for all write access to the
         * IOB registers. The IOB number to control is selected by the currentModule.
         * Sends the "IOBC R  0xXX " command.
         * All IOB registers consist of 32 bytes. So every register can be read/written at once.
         * @param address Address to write.
         * @param data data to write.
         * @see iobControl_read()
         * @see setActiveModule()
         */
        void iobControl_write(int address, int data);
        void iobControl_writeD(int address, int data);

        bool sendDirectWrite(std::vector<uint32_t> & cmdVec);

        int programQuadIOBRegisters();
        int programQuadIOBRegisters(const std::string & moduleSetStr); // program all 4 IOB same register
        int programIOBRegisters();
        int programIOBRegister(const std::string & moduleSetStr, bool overwrite = false);
        int programIOBRegisterD(const std::string & moduleSetStr, bool overwrite = false);

        int  readBackIOBRegisters();
        int  readBackIOBRegister();
        int  readBackIOBSingleRegs(const std::string & moduleSet, bool overwrite);
        bool readBackIOBSingleReg(const std::string & moduleSet, bool overwrite = true);

        int  programJtags(bool readBack = false);
        bool programJtag(bool readBack = false, bool setJtagEngineBusy=true, bool recalcXors = true);
        bool programJtagSingle(const std::string & moduleSetName, bool readBack = false, bool setJtagEngineBusy = true, bool recalcXors = true, bool overwrite = false);
        bool readBackJtag(bool overwrite = false);
        bool readBackSingleJtag(const std::string & moduleSetName, bool overwrite = false);

        inline void testJtagInChain(bool inChain){checkPixelRegsInChain(inChain);}
        void checkPixelRegsInChain(bool inChain);
        void setPixelRegsInChain(bool inChain);

        int  programAllPixelRegs(bool readBack = false);
        bool programPixelRegs(bool readBack=false, bool setJtagEngineBusy=true);
        void programPixelRegDirectly(int px, bool setJtagEngineBusy=true);
        bool programPixelRegsInChain(bool readBack=false);
        bool programPixelRegsAllAtOnce(bool readBack=false);
        bool programPixelRegsSingleASIC(int asic, bool readBack = false);

        bool readBackPixelChain();
        bool readBackPixelSingleASIC(int asic);

        /** Programs the ASIC Sequencers of all available modules.
         *  @param readBack boolean value. If true the sequencer is read out and checked against the local configuration.
         *  @return
         */
        bool programSequencers(bool readBack = false);

        /** Programs the ASIC Sequencer JTAG Registers with the values defined in
         *  the Sequencer config register. Reads the sequencer configuration and sends the configuration bytes to the
         *  SlowControlServer that starts the JTAG programming using the commands from the cmdsFromSoftware file.
         *  @param readBack boolean value. If true the sequencer is read out and checked against the local configuration.
         *  @param setJtagEngineBusy obsolete parameter that is required in the F1 environment.
         *  @param reprogramJtag if set to true the JTAG register is reprogrammed in order to stay
         *         in a consistent state if the cycle length or the hold length has changed.
         *  @return true if the configuration file could be sent and the readback was successfully.
         *          If false the reason can be found in the errorString and in the errorMessage vector.
         */
        bool programSequencer(bool readBack = false, bool setJtagEngineBusy=true, bool reprogramJtag=true);

        /** Reads the current configuration of the sequencer registers from the readout ASICs.
         *  The read data are checked against the local configuration. Readback errors can be found
         *  in the rbWarningMessages vector.
         *  @return true if the configuration file could be sent and the readback was successfully.
         *          If false the reason can be found in the errorString and in the errorMessage vector.
         */
        bool readBackSequencer();


        /** No functionality. F1 Setup function */
        bool getSinglePixelReadoutMode(){return false;}
        /** No functionality. F1 Setup function */
        void setSinglePixelReadoutMode(bool en, bool program = true){if(en){std::cout << "Could not enable singlePixelReadoutMode - mode only available in F1 Test Setups!" << std::endl;}}
        /** No functionality. F1 Setup function */
        void setSinglePixelToReadout(int px, bool program = false){std::cout << "Could not enable singlePixelReadoutMode - mode only available in F1 Test Setups!" << std::endl;}
        /** No functionality. F1 Setup function */
        void setFullFReadoutMode(bool en, bool program = false){if(en){std::cout << "Could not enable singlePixelReadoutMode - mode only available in F1 Test Setups!" << std::endl;}}

         /**
         * Set debug signal at test pin 0 = SMA1_val.
         * Only SMA_1_val is implemented
         * Possible values are 0 to 15:
         * 0 =  fsm_ddr_readout
         * 1 =  fsm_burst
         * 2 =  fsm_iprog
         * 3 =  fsm_wait_for_train
         * 4 =  asic_fsm_burst
         * 5 =  fsm_reset_ram
         * 6 =  readout_timeout
         * 7 =  cycle_done
         * 8 =  readout_done
         * 9 =  memory_full
         * 10 = veto
         * 11 = golden
         * 12 = sending_cmd
         * 13 = invalid_cmd
         * 14 = train_info_recvd
         * 15 = xfel_cmd_error_detect
         */
        void setDebugOutputSelect(int SMA_1_val, int SMA_2_val, bool enableSMA_1 = true, bool enableSMA_2 = true);

        /** Sends the "FINT" command to the SlowControlServer.
         *  If the JTAG Engines of the currentModule are preloaded with data the fast init sequence is triggered.
         *  The fastInit sequence is:
         *  +Switch on all 3 ASIC supplys.
         *  +Enable the JTAG Engines that start immediately to programm the fastInit configuration.
         *  +Wait until first JTAG initialization has finished.
         *  +Disable switched supplys.
         * @return false if no data was found in the JTAG Engine FIFO else returns true.
         */
        bool doFastInit();

        /** Start the JTAG Programming. If the cmdsFromSoftware File can be found on the PPT this function
         * trigges the JTAG Engine to start reading the configuration file and to perform the JTAG
         * programming.
         * @return true if a configuration file cmdsFromSoftware can be found in the PPT.
         */
        bool startJTAGProgramming();

        /**
          * Starts an acquisition sequence - prepares the PPT and ASICs to receive
          * triggers and read out.
          *
          * @return DSSC_PPT error code, ERROR_OK or other error condition
          */
        int start();

        /**
          * Stops an acquisition sequence - stops readout of the PPT and ASICS. May be called
          * once triggers have stopped to cleanly terminate acquisition, or during acquisition
          * to abort cleanly
          *
          * @return DSSC_PPT error code, ERROR_OK or other error condition
          */
        int stop();


        /** Reads the full build information of the IOB firmware.
         *  @return The full build information in one printable string.
         */
        std::string readIOBBuildStamp();

        /** Reads the full build information of the linux binary.
         *  @return The full build information in one printable string.
         */
        std::string readLinuxBuildStamp();

        /** Reads the full build information of the PPT firmware.
         *  @return The full build information in one printable string.
         */
        std::string readBuildStamp();

        /** Reads the Build Date information from the current PPT firmware.
         *  @return The Build Date information.
         */
        uint32_t readBuildDate();

        /** Reads the Build Time and Revision information from the current PPT firmware.
         *  @return The Build Time and revision information. 2 MSB contain the Build Time,
         *  2 LSBytes contain the build revision.
         */
        uint32_t readBuildTimeRev();

        /** Change the initDistance during fastInit.
         * The init distance defines the delay after switching on the ASIC power and
         * the beginning of the JTAG programming. Default value is 150.
         * Normally values over 100 work fine.
         * If init distance is too small the ASIC programming does not work and the F1 ASIC
         * still consumes a lot of power because certain decoupling
         * registers could not be programmed correctly.
         * @param _initDist new value to be set.
         */
        inline void setInitDist(uint32_t _initDist){initDist = _initDist;}
        uint32_t getInitDist(){return initDist;}

        bool setEthernetOutputDatarate(double megabit);
        uint getEthernetOutputThrottleDivider();
        void setEthernetOutputThrottleDivider(uint);

        /** Change the DAC output voltage of the F1 Test Setup.
         * @param DACSetting new value (0-65535)
         */
        void setDACSetting(int DACSetting);

        /** Send a command over the RS232 lines.
         * @param command command to send.
         */
        void sendRS232command(const std::string & command);

        /** Manually reset the iobsNotChecked reminder. this can be used to force a recheck
         * of the connected IOB modules.
         */
        void resetIOBChecked(){iobsNotChecked = true;}

        /** Check the PPTRevision of a given binary. to check the revision, a second txt file  with identical name
         *  has to be given additionally to the binary that includes the PPT Revision and the build_rev in
         *  two text lines like 'ppt_rev = 2' and 'build_rev = 5555'.
         * @param fileName const reference with full or relative path to the binary file.
         * @return boolean value true if given information fits to requirements
         */
        bool checkPPTRevision(const std::string &fileName);


        /** Check the Liunx SVNRevision of a given binary. To check the revision, a second txt file  with identical name
         *  has to be given additionally to the binary that includes the Linux build_rev in
         *  a text lines like 'build_rev = 5555'.
         * @param fileName const reference with full or relative path to the binary file.
         * @return boolean value true if given information fits to requirements
         */
        bool checkLinuxRevision(const std::string &fileName);

        /** Check the Liunx SVNRevision of a given binary. To check the revision, a second txt file  with identical name
         *  has to be given additionally to the binary that includes the Linux build_rev in
         *  a text lines like 'build_rev = 5555'.
         * @param fileName const reference with full or relative path to the binary file.
         * @return boolean value true if given information fits to requirements
         */
        bool checkIOBRevision(const std::string &fileName);

        /** Flush the SIB RX fifo. Reads all remaining bytes from the SIB fifo
         */
        void flushSIBFifo();

        /** Enable the UART Test Pins of the PPT.
          * UART Test pins are Test 8 = PPT Tx out Test 6 = PPT Rx in.
          * If enabled the SIB can't receive any commands.
         * @param enable true = the test pins are used, false the SIB can communicate.
         */
        void enUARTTestPins(bool enable);

        /** Read the 65 bytes from the SIB. All bytes are stored into the status vector.
         * @param status reference that is filled with the readback values.
         */
        void getSIBStatus(std::vector<uint8_t> &status);

        std::vector<uint32_t> readSIBReceiveRegister();


        /** Set the UART clock divider to the selected Baudrate.
         * @param baudrate in kbit/s. Typical & tested values are:
         *                   19200,  38400,  57600,  76800
         *                  115200, 230400, 460800
         *                  921600.
         */
        void setUARTBaudRate(const std::string &baudrate);


        /** Triggers one burst to start the send temperatures sequence.
         */
//         void sendTemperaturesToSIB();

        std::vector<uint32_t> getFailedAsicNumbers(uint16_t);

        void getOnlySendingASICPixels(std::vector<int> & sendingPixels) const override
        {
          for(int idx=sendingPixels.size()-1; idx>=0; idx--){
            int asic = getASICNumberFromPixel(sendingPixels[idx]);
            if(!isAsicSending(asic)){
              sendingPixels.erase(sendingPixels.begin()+idx);
            }
          }
        }

        void getOnlySendingASICPixels(std::vector<uint32_t> & sendingPixels) const override
        {
          for(int idx=sendingPixels.size(); idx>=0; idx--){
            int asic = getASICNumberFromPixel(sendingPixels[idx]);
            if(!isAsicSending(asic)){
              sendingPixels.erase(sendingPixels.begin()+idx);
            }
          }
        }


        virtual void setNumberOfActiveAsics(int numAsics) override;

        void checkJTAGEngine(int chain);

        void cloneEth0(bool en);
        bool isCloneEth0Enabled();

    public: //simulationFunctions
        void clearSimulationFile();
        void setSimulationMode(bool enable);
        void setSimFileName(const std::string & fileName);
        inline bool isSimulationMode() const {return simulationMode;}
        inline std::string getSimFileName() const {return simFileName;}

//F2 new functions
    public:




    protected: //simulationFunctions
        void dumpDataString();

    private: //simulationFunctions
        void simWriteIOBAddress(std::ofstream &out, uint32_t address, uint16_t iob_address, uint32_t data);
        void simReadIOBAddress(std::ofstream &out, uint32_t address, uint16_t iob_address);

        void simReadbackFromJTAG(int byteStreamLength);

    private:

        bool writeDataToSocket(const char * data, uint64_t totalLength);

        //stupid debug function
        void printConfigVectorSize() const {std::cout << "ConfigVectorSize = " << configVector.size() << std::endl;}

        bool checkSingleModuleRegRBData(ConfigReg *configReg, const std::string & moduleSet, bool overwrite = false);


        /** Adds the full Pixel Register configuration to the configVector.
         *  Used to add the configuration of all pixels before the programming file cmdsFromSoftware is written.
         *  Also the XY select register is configured in order to program the pixel registers in chain mode.
         *  @param readBack boolean value. If true the configuration is added with the readback flag set.
         */
        void addPixelRegistersToConfigVector(bool readBack = false);
        void addPixelRegistersToConfigVector(int asic, bool readBack = false);

        /** Adds the selected Pixel configuration to the configVector.
         *  Used to add the configuration of one single pixel before the programming file cmdsFromSoftware is written.
         *  Also the XY select register is configured in order to program only the selected pisel.
         *  @param px selectes the pixel to be programmed
         */
        void addPixelRegistersToConfigVector(const std::string & moduleSet, int px);

        /** Adds the full JTAG Register configuration to the configVector.
         *  Used to add a configuration before the programming file cmdsFromSoftware is written.
         *  @param readBack boolean value. If true the configuration is added with the readback flag set.
         */
        void addJTAGRegistersToConfigVector(bool readBack = false);

        /** Adds the configuration of a single ModuleSet to the configVector.
         *  Used to add a configuration before the programming file cmdsFromSoftware is written.
         *  Calls the addSingleJTAGRegistersToConfigVector(int set, ...)
         *  @param moduleSetStr selects the moduleSet of the JTAG Register.
         *  @param readBack boolean value. If true the configuration is added with the readback flag set.
         */
        void addSingleJTAGRegistersToConfigVector(const std::string & moduleSetStr, bool readBack = false);

        /** Adds the default pixel configuration to the configVector.
         * Default Pixel is Pixel 0. The XY select register is also configured.
         *  @param readBack boolean value. If true the configuration is added with the readback flag set.
         */
        void addDefaultPixelRegisterToConfigVector(const std::string &moduleSet, bool readBack = false);

        /** Adds the sequencer configuration to the configVector.
         *  Used to add a configuration before the programming file cmdsFromSoftware is written.
         *  @param readBack boolean value. If true the configuration is added with the readback flag set.
         */
        void addSequencerToConfigVector(bool readBack = false);

        /** Returns a special XY select register configuration.
         * Returns the configuration for the XY select register in the data vector that is
         * required to programm all pixels simultaneously with the same configuration.
         * Used in addDefaultPixelRegisterToConfigVector.
         * @see addDefaultPixelRegisterToConfigVector()
         * @param data reference to the XY select register configuration
         */
        void enableYSelEnableXSel(std::vector<uint32_t> &data);

        /** Returns a special XY select register configuration.
         * Returns the configuration for the XY select register in the data vector that is
         * required to programm all pixel registers individually in chain mode.
         * Used in addPixelRegistersToConfigVector.
         * @see addPixelRegistersToConfigVector()
         * @param data reference to the XY select register configuration
         */
        void disableYSelEnableXSel(std::vector<uint32_t> &data);
        void disableYSelEnableXSel(std::vector<uint32_t> &data, int asic);

        /** Configures the XY select registers of the pixel JTAG chain to a certain pixel.
         * If only one certain pixel is to be programmed this function returns the correct
         * configuration for the XY select registers in the data vector.
         * @param data reference to the configuration bits that are filled according to the
         * specified pixel in px.
         * @param px selects the pixel to be programmed.
         */
        void programXYSelectRegs(std::vector<uint32_t> &data, int px);

        void setAllASICSBypass();


        /** Clears the configVector that is filled with all JTAG configuration bytes.
         */
        void initConfigVector(){ configVector.clear(); }
        void initDirectVector(){ directVector.clear(); }
        /** After sending JTAG configuration with readback enabled the SlowControlServer
         * stores the readback data in the ReadBackJtagCommands file.
         * This function loads this file form the PPT and compares the readback data to the current
         * configuration.
         * @return ERROR_OK
         */
        int  loadReadbackDataFromPPT();


        /** Stores the JTAG Configuration bytes from the given data vector into the cmdsFromSoftware
         *  file.
         *  @param data constant reference to the JTAG configuration bytes to store.
         *  @param fileName file name to store the configuration bytes. Use 'cmdsFromSoftware' for the fileName.
         */
        void printVectorToConfigFile(std::vector<uint32_t> &data, const std::string &fileName);


        /** Sends the JTAG Configuration bytes directly to the SlowControlserver via TCP string.
         *  This is much faster than sending a file via FTP.
         *  @param data constant reference to the JTAG configuration bytes to store.
         *  @return bool if SlowControlserver sends correct acknowledge bytes
         */
        bool sendConfigurationDirectly(std::vector<uint32_t> &data);


        /** Wrapper function to generate and send the JTAG configuration file to the PPT.
         *  In sendConfigurationFile() also the JTAG programming is triggered.
         *  @param data constant reference to the JTAG configuration bytes to store.
         *  @param regs aftger sending a new configuration file the XORs are recalculated. With this param
         *  only certain registers can be chosen to be recalculated.
         */
        bool sendConfigurationVectorAsFile(std::vector<uint32_t> &data);
        bool sendConfigurationVector(std::vector<uint32_t> &data, RegisterType regs = AllRegs);

        /** Send the cmdsFromSoftware JTAG configuration file and trigger the JTAG programming.
         *  Since the configuration bytes keep the information which module JTAG chain is to be programmed
         *  this function needs no module information.
         */
        int sendConfigurationFile(const std::string & fileName);

        /** Get a subset of readback bytes from the readback data vector for comparision.
         * @param bitStreamLength next number of bits to compare with a configuration register.
         * @param data_vec reference that is filled with the readback bits from the read back byte vector.
         */
        bool getContentFromDevice(uint bitStreamLength, std::vector<bool> &data_vec);

    //    uint32_t getIOBModuleSetValue(int set);

        /** Wrapper function to cut out a section out of a bitvector.
         *  This function is used to add required bits at the end of each pixel register chain after 1024 pixels.
         *  This is required due to the two face clocking in the full custom shift register in the pixel jtag chain.
         *  @param pxRegsBits vector of all pixel register configuration bits.
         *  @param start first bit to cut out.
         *  @param numBits length of the cut out section.
         *  @param subBits reference to the shorter section of the original bitvector.
         */
        std::vector<bool> getSubPxRegBits(const std::vector<bool> & pxRegsBits, uint start, uint numBits);

        /** Converts a bitvector to a byte vector and appends it to the given data vector.
         *  This function is used to append the bits if a configuration register to the configVector.
         *  @param data reference to the byte vector to fill.
         *  @param jtagInstr Address of JTAG register to program the in the ASIC. This address is stored in the register configuration.
         *  @param bits Bitvector to add as a byte vector to the data vector reference.
         *  @param readBack if set the JTAG instruction is configured to perform a readback.
         *  @see configVector
         *  @see sendConfigurationVectorAsFile()
         */
        void appendAndSendJtagReg(std::vector<uint32_t> &data, uint8_t jtagInstr,const std::vector<bool> &bits, bool readBack);
        void appendAndSendJtagReg(std::vector<uint32_t> &data, int asic, uint8_t jtagInstr,const std::vector<bool> &bits, bool readBack);

        void appendJtagReset();

        /** Obsolete function. **/
       // static uint getSignalLsb(ConfigReg * registers, int moduleSet, int signal);

       /** Obsolete function. **/
        static int getRegSignalValue(int reg, int lsb, int bits);

        /** Static function to get a section of a space seperated string list.
         * @param line string containing several sections spaced with exp.
         * @param numPos number of the section, starts at 0.
         * @see ipToInt()
         * @return the string section at position numPos
         */
        static std::string getSection(const std::string & line, int numPos);

        int readRevFromFile(const std::string &fileName, int &build);

        void enableASICPower(uint16_t asic);
        void disableASICPower(uint16_t asic);

        void updateStaticASICPowers();

        std::string readSVNRevision();

        void setRoSerIn(bool bit);

        //
        int runTestPatternAcquisition(uint16_t _testPattern){std::cout << "runTestPatternAcquisition not implemented" << std::endl; return 0;}
        void resetDataReceiver(){std::cout << "resetDataReceiver not implemented" << std::endl;}

    private:
        bool m_opened;
        std::string pptTcpAddress;
        uint16_t tcpPort;
        uint16_t ftpPort;
        int socket_nummer;

        std::string dataString;

        /** After a JTAG readback this vector is filled with the readback data in order to compare it
         * to the current configuration.
         */
        std::vector<int> rbDataVec;

        /** This variable remembers if it was already checked for connected IOBoards since the last reset or
         *  IOB FPGA programming. If it set the last found configuration is valid.
         *  @see setActiveModule()
         */
        bool iobsNotChecked;

        /** This variable determines the module that is currently controlles, configured and read
         *  from. Can be set by setActiveModule()
         *  @see setActiveModule()
         */

        bool isSpecialComponentId();
        bool runSpecialPixelProgrammingRoutine();


        /** This vector is filled with all bytes for the next JTAG programming.
         *  The content of the vector is written into the cmdsFromSoftware file
         *  that is sent to the PPT.
         */
        std::vector<uint32_t> directVector;
        std::vector<uint32_t> configVector;
        std::vector<uint32_t> lastConfigVectorForRB;

        uint32_t initDist;

        /** Variable that keeps the lowest PPT firmware revision number
         *  that is compatible to this software
         */
        static int firstFirmware;

        /** Variable that keeps the lowest PPT linux revision number
         *  that is compatible to this software
         */
        static int firstLinux;

        /** Variable that keeps the lowest IOB firmware revision number
         *  that is compatible to this software
         */
        static int firstIOB;

        /** Variable that keeps the current PPT revision.
         *  Revision must be checked during update
         */
        static int pptRev;

    public:
        static constexpr double PPTCYCLELENGTH =  7.0 / 695000.0; // in ms           //1e-5
        static constexpr double MICROSECOND    =  1.0 / PPTCYCLELENGTH / 1000.0;       //99.285
        static constexpr int    PRBPROGTIME    = 14.0 * MICROSECOND;                 //6 us to programm 13.5 until power is up
        static constexpr int    FETOFFSET      = 9 * MICROSECOND + 44;
        static constexpr int    DISOFFSET      = PRBPROGTIME - 14;
        static constexpr int    CLROFFSET      = 1;

      protected:

        bool simulationMode;
        std::string simFileName;
        bool m_standAlone;

        std::array<uint16_t,4> activePowers;

        int fastInitconfigSpeed;

        LMKSettings lmkFastSettings;
        LMKSettings lmkSlowSettings;

        LMKClockSettings getLMKSettings(int asic, bool fast){
          return (fast)? lmkFastSettings[asic] : lmkSlowSettings[asic];
        }

        class JTAGClockDivKeeper
        {
        public:
          JTAGClockDivKeeper(DSSC_PPT *ppt, int newClockDiv)
          : m_ppt(ppt)
          {
            m_clkDiv = m_ppt->getEPCParam("JTAG_Control_Register","all","ASIC_JTAG_Clock_Divider");
            m_ppt->setEPCParam("JTAG_Control_Register","all","ASIC_JTAG_Clock_Divider",newClockDiv);
            m_ppt->setEPCParam("JTAG_Control_Register","all","EnJTAG1",0);
            m_ppt->setEPCParam("JTAG_Control_Register","all","EnJTAG2",0);
            m_ppt->setEPCParam("JTAG_Control_Register","all","EnJTAG3",0);
            m_ppt->setEPCParam("JTAG_Control_Register","all","EnJTAG4",0);

            m_ppt->programEPCRegister("JTAG_Control_Register");
          }
          ~JTAGClockDivKeeper(){
            m_ppt->setEPCParam("JTAG_Control_Register","all","ASIC_JTAG_Clock_Divider",m_clkDiv);
            m_ppt->programEPCRegister("JTAG_Control_Register");
          }
        private:
            DSSC_PPT *m_ppt;
            uint32_t m_clkDiv;
        };


        class CheckSendingKeeper
        {
        public:
            CheckSendingKeeper(DSSC_PPT *ppt)
             : m_ppt(ppt),
               sendDisabled(ppt->isSendingDisabled()),
               contModeRem(ppt->inContinuousMode())
            {
                if(!sendDisabled){
                    m_ppt->disableSending(true);
                }
                if(!contModeRem){
                    m_ppt->runContinuousMode(true);
                }
            }

            ~CheckSendingKeeper(){
                if(!sendDisabled){
                    m_ppt->disableSending(false);
                }
                if(!contModeRem){
                    m_ppt->runContinuousMode(false);
                }
            }
        private:
            DSSC_PPT *m_ppt;
            bool sendDisabled;
            bool contModeRem;
        };

        //ContModeKeeper defined in libCHIPInterface
    };
}


#endif	/* PPTDEVICE_HH */


