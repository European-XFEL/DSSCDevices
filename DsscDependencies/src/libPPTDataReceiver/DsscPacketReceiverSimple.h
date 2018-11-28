#ifndef DSSCPACKETRECEIVERSIMPLE_H
#define DSSCPACKETRECEIVERSIMPLE_H

#include "DsscTrainData.hh"

class DsscPacketReceiverSimple
{

  public:
    DsscPacketReceiverSimple();
    ~DsscPacketReceiverSimple();

    static bool s_ladderMode;
    static std::vector<uint> s_sendingAsics;
    static uint s_activeAsic;
    static bool s_enableDummyDataChecking;

    static int getAsicIdx(int asic){return std::distance(s_sendingAsics.begin(),std::find(s_sendingAsics.begin(),s_sendingAsics.end(),asic));}
    static bool isAsicSending(int asic){ return (std::find(s_sendingAsics.begin(),s_sendingAsics.end(),asic) != s_sendingAsics.end());}

    static void setActiveAsic(int asic) { s_activeAsic = asic; }
    static uint getActiveAsic() { return s_activeAsic; }
    static void setSendingAsics(uint16_t asics) { s_sendingAsics = utils::bitEnableValueToVector(asics); }

    static int numAsicsToSort(){return s_sendingAsics.size();}
    static std::vector<uint32_t> getAsicsToSort() { return s_sendingAsics; }

    static void setLadderReadout(bool enable) { s_ladderMode = enable; }
    static bool isLadderReadout() { return s_ladderMode; }


    static constexpr uint32_t c_pktSize = 8192;
    static constexpr uint8_t  c_eof     = 0x40;
    static constexpr uint8_t  c_sof     = 0x80;
    static constexpr uint64_t c_magic_trailer = 0x58544446DEADABCD;
    static constexpr uint64_t c_magic_header  = 0x58544446BEEFFACE;
    static constexpr uint32_t c_magic_trailer_short = 0xDEADABCD;
    static constexpr uint32_t c_gccWrap = 5;

    static constexpr int s_SOCKET_TIMEOUT = 300;

    int getUDPPort() const {return m_UDPPort;}
    void setUDPPort(int port) {m_UDPPort = port;}
    bool isConnected() const {return m_connected;}
    void setValidTrainId(uint64_t trainId) {m_validTrainId = trainId;}
    std::string getInputFormat() const {return m_inputFormat;}
    utils::DsscSpecificData getSpecificData();

    utils::DsscAsicTrailerData& getAsicTrailer();
    utils::DsscAsicTrailerData& getAsicTrailer(int asic);

    const utils::DsscAsicTrailerData& getAsicTrailer(int asic) const;
    const utils::DsscAsicTrailerData& getAsicTrailer() const;

    utils::THeader getHeaderData();

    uint16_t *getDataBuffer(){ return (m_sortedTrainStorage.data()+utils::DsscTrainData::c_HEADER_OFFS);}

    void receiveAndSortData(uint64_t nextValidTrainId);
    void setNextValidTrainId(uint64_t nextValidTrainId){m_validTrainId = nextValidTrainId;}
    uint64_t getLastReceivedTrainID() const {return m_lastReceivedTrainId;}

    void start(uint16_t numFramesToReceive){}
    void enContinuousReceiving(bool enable){}
    void stopDataReceiver(){}

    inline void updateExpectedTestPattern(uint32_t testPattern){}

    void setTestPatternMode(bool enable, int testPattern = 5){ m_errorCnt = 0; m_totalCnt = 0;}
    void enDummyDRData(bool enable){ m_errorCnt = 0; m_totalCnt = 0;}
    void enDummyIOBData(bool enable){ m_errorCnt = 0; m_totalCnt = 0;}
    void enableKC705DummyData(bool enable){ m_errorCnt = 0; m_totalCnt = 0;}

    void updateStatus(const std::string & text){}

    void fillDummyData(){}

    void checkDummyData();
    void checkIOBDummyData();
    void checkDRDummyData();
    void checkDataArray();

    virtual int openConnection();
    virtual void closeConnection();

  protected:

    void init(){
      m_sof_received = false;
      m_numTrainPackets = 0;
      m_currentOffset = 0;
      m_unfilledTrainStoragePtr->setInvalid();
    }

    void dataInvalid();
    void updateAsicTrailerArray();


    static uint32_t getFrameID(char * data, size_t length);
    static uint16_t getPacketID(char * data, size_t length);
    static uint64_t getTrainID(char * data);

    void receivePackets();
    void sortData();
    uint64_t getLostFraction() const;

    void setSocketRecvBufSize(uint32_t size);
    void setSocketTimeout(uint32_t miliseconds);

    uint16_t calcDummyDrExpectedValue(uint32_t asic, uint32_t pixel);

    // pointers to train storage fields
    // can be exchanged for different multithreaded implementations
    utils::DsscTrainData *m_unfilledTrainStoragePtr;
    utils::DsscTrainData *m_unsortedTrainStoragePtr;
    utils::DsscTrainData *m_sortedTrainStoragePtr;

    // for singe threaded usage the class holds two train data storages
    utils::DsscTrainData m_unsortedTrainStorage;
    utils::DsscTrainData m_sortedTrainStorage;

    int m_udpSocket;
    bool m_connected;
    uint16_t m_UDPPort;

    bool m_sof_received;
    uint32_t m_numTrainPackets;
    size_t m_currentOffset;

    uint64_t m_lastReceivedTrainId;
    uint64_t m_validTrainId;
    std::string m_inputFormat;


    uint64_t m_errorCnt;
    uint64_t m_lostCnt;
    uint64_t m_totalCnt;

    utils::DsscAsicTrailerDataArr m_trailerData;

};


#endif // DSSCPACKETRECEIVERSIMPLE_H
