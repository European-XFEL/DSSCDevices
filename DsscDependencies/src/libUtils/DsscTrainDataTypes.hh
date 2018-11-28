#ifndef DSSCTRAINDATATYPES_H
#define DSSCTRAINDATATYPES_H

#pragma once

#include "utils.h"

namespace utils
{

struct THeader{
  THeader():magicNumber(0),majorTrainFormatVersion(0),minorTrainFormatVersion(0),
            trainID(0),dataID(0),tbLinkID(0),detLinkID(0),pulseCnt(0),detSpecificLength(0),tbSpecificLength(0){}

  uint64_t magicNumber;
  uint32_t majorTrainFormatVersion;
  uint32_t minorTrainFormatVersion;
  uint64_t trainID;
  uint64_t dataID;
  uint32_t tbLinkID;
  uint32_t detLinkID;
  uint64_t pulseCnt;
  uint32_t detSpecificLength;
  uint32_t tbSpecificLength;

  void print() const {
    std::cout << "+++ Train Header : "<< std::endl;
    std::cout << "Magic Number : "<< magicNumber             << "( 0x" << std::hex << magicNumber             << " )" << std::dec << std::endl;
    std::cout << "Major Version: "<< majorTrainFormatVersion << "( 0x" << std::hex << majorTrainFormatVersion << " )" << std::dec << std::endl;
    std::cout << "Minor Version: "<< minorTrainFormatVersion << "( 0x" << std::hex << minorTrainFormatVersion << " )" << std::dec << std::endl;
    std::cout << "Train ID     : "<< trainID                 << "( 0x" << std::hex << trainID                 << " )" << std::dec << std::endl;
    std::cout << "Data ID      : "<< dataID                  << "( 0x" << std::hex << dataID                  << " )" << std::dec << std::endl;
    std::cout << "Det Link ID  : "<< detLinkID               << "( 0x" << std::hex << detLinkID               << " )" << std::dec << std::endl;
    std::cout << "TB Link ID   : "<< tbLinkID                << "( 0x" << std::hex << detLinkID               << " )" << std::dec << std::endl;
    std::cout << "Pulse Cnt    : "<< pulseCnt                << "( 0x" << std::hex << pulseCnt                << " )" << std::dec << std::endl;
    std::cout << "Det Spec Len : "<< detSpecificLength       << "( 0x" << std::hex << detSpecificLength       << " )" << std::dec << std::endl;
    std::cout << "TB Spec Len  : "<< tbSpecificLength        << "( 0x" << std::hex << tbSpecificLength        << " )" << std::dec << std::endl;
  }
};


struct TTrailer{
  TTrailer():checkSum0(0),checkSum1(0),status(0),magicNumber(0),ttp(0){}

  uint64_t checkSum0;
  uint64_t checkSum1;
  uint64_t status;
  uint64_t magicNumber;
  uint64_t ttp;

  inline bool magicNumberCorrect() const {return magicNumber == 0x58544446DEADABCD;}

  void print() const
  {
    std::cout << "+++ Train Trailer: "<< std::endl;
    std::cout << "Magic Number: "<< magicNumber << "( 0x" << std::hex << magicNumber << " )" << std::dec << std::endl;
    std::cout << "Status      : "<< status      << "( 0x" << std::hex << status      << " )" << std::dec << std::endl;
    std::cout << "CheckSum0   : "<< checkSum0   << "( 0x" << std::hex << checkSum0   << " )" << std::dec << std::endl;
    std::cout << "CheckSum1   : "<< checkSum1   << "( 0x" << std::hex << checkSum1   << " )" << std::dec << std::endl;
    std::cout << "TTP Bytes   : "<< ttp   << "( 0x" << std::hex << ttp   << " )" << std::dec << std::endl;
  }
};

struct DsscSpecificData{

    DsscSpecificData()
     : pptVetoCnt(0),numPreBursVetos(0),userSpecific1(0),userSpecific2(0),userSpecific3(0),moduleNr(0),iobSerial(0),
       rotate_ladder(false),sort_asic_wise(false),
       send_dummy_dr_data(false),send_raw_data(false),send_conv_data(true),send_reord_data(true),
       single_ddr3_block(false),clone_eth0_to_eth1(false)
    {}

    uint16_t pptVetoCnt;
    uint16_t numPreBursVetos;
    uint16_t userSpecific1;
    uint16_t userSpecific2;
    uint16_t userSpecific3;
    uint8_t moduleNr;
    uint32_t iobSerial;

    bool rotate_ladder;
    bool sort_asic_wise;
    bool send_dummy_dr_data;
    bool send_raw_data; // raw_data as received from asics - for dummy data
    bool send_conv_data; // converted decimal from gcc code
    bool send_reord_data;  // bit reordered needed due to different assignment in F2 asic
    bool single_ddr3_block; // debug mode for slwo ddr3 writing
    bool clone_eth0_to_eth1; // copy data from module 1 to module 2, module 2 data will be lost


    void print() const
    {
      std::cout << "+++ DsscSpecific Data: "<< std::endl;
      std::cout << std::setw(20) << "PPT Veto Cnt: "<< pptVetoCnt << std::endl;
      std::cout << std::setw(20) << "Num PreburstVetos: "<< numPreBursVetos << std::endl;
      std::cout << std::setw(20) << "User Specific 1: "<< userSpecific1 << std::endl;
      std::cout << std::setw(20) << "User Specific 2: "<< userSpecific2 << std::endl;
      std::cout << std::setw(20) << "User Specific 3: "<< userSpecific3 << std::endl;
      std::cout << std::setw(20) << "Module Nr: " << (int)moduleNr << std::endl;
      std::cout << std::setw(20) << "IOB Serial: 0x"<< std::hex << iobSerial << std::dec << std::endl;
      std::cout << "PPT Data Flags: \n";
      std::cout << std::setw(22) << "rotate_ladder:" << rotate_ladder << std::endl;
      std::cout << std::setw(22) << "sort_asic_wise:" <<sort_asic_wise << std::endl;
      std::cout << std::setw(22) << "send_dummy_dr_data:" << send_dummy_dr_data << std::endl;
      std::cout << std::setw(22) << "send_raw_data:" << send_raw_data << std::endl;
      std::cout << std::setw(22) << "send_conv_data:" << send_conv_data << std::endl;
      std::cout << std::setw(22) << "send_reord_data:"  << send_reord_data<< std::endl;
      std::cout << std::setw(22) << "single_ddr3_block:" << single_ddr3_block << std::endl;
      std::cout << std::setw(22) << "clone_eth0_to_eth1:" << clone_eth0_to_eth1<< std::endl;
      std::cout << "++++++++++++++++++++++++"<< std::endl;
      std::cout << std::endl;
    }

    std::string getInfoStr() const {
      std::stringstream iss;
      iss << "+++ DsscSpecific Data:\n";
      iss << std::setw(20) << "PPT Veto Cnt: "<< pptVetoCnt << "\n";
      iss << std::setw(20) << "Num PreburstVetos: "<< numPreBursVetos << "\n";
      iss << std::setw(20) << "User Specific 1: "<< userSpecific1 << "\n";
      iss << std::setw(20) << "User Specific 2: "<< userSpecific2 << "\n";
      iss << std::setw(20) << "User Specific 3: "<< userSpecific3 << "\n";
      iss << std::setw(20) << "Module Nr: " << moduleNr << "\n";
      iss << std::setw(20) << "IOB Serial: 0x"<< std::hex << iobSerial << std::dec << "\n";
      iss << "PPT Data Flags: \n";
      iss << std::setw(22) << "rotate_ladder:" << rotate_ladder << "\n";
      iss << std::setw(22) << "sort_asic_wise:" <<sort_asic_wise << "\n";
      iss << std::setw(22) << "send_dummy_dr_data:" << send_dummy_dr_data << "\n";
      iss << std::setw(22) << "send_raw_data:" << send_raw_data << "\n";
      iss << std::setw(22) << "send_conv_data:" << send_conv_data << "\n";
      iss << std::setw(22) << "send_reord_data:"  << send_reord_data<< "\n";
      iss << std::setw(22) << "single_ddr3_block:" << single_ddr3_block << "\n";
      iss << std::setw(22) << "clone_eth0_to_eth1:" << clone_eth0_to_eth1<< "\n";
      iss << "++++++++++++++++++++++++\n";
      iss << "\n";
      return iss.str();
    }
};

struct DsscAsicTrailerData
{
    static constexpr uint16_t c_numTrailerWords = 8;
    static constexpr uint16_t c_totalTrailerWords = 2*16*c_numTrailerWords;

    DsscAsicTrailerData() :
      m_vetoCnt(0xDEAD),m_temp0(0xDEAD),m_temp1(0xDEAD),
      m_testPattern(0xDEAD),m_roXor(0xDEAD),m_asic(0){}

    DsscAsicTrailerData(const std::vector<uint8_t> & asicTrailerDataVec, int asic)
     : m_asic(asic)
    {
      if(asicTrailerDataVec.size() < c_totalTrailerWords){
        std::cout << "asicTrailerDataVec.size() is too small: " << asicTrailerDataVec.size() << "/" << c_totalTrailerWords << std::endl;
        m_valid = false;
        return;
      }
      const uint8_t* asicOffs = asicTrailerDataVec.data() + calcOffset();
      update(asicOffs);
    }

    DsscAsicTrailerData(uint8_t * asicTrailerData, int asic)
     : m_asic(asic)
    {
      const uint8_t* asicOffs = asicTrailerData + calcOffset();
      update(asicOffs);
    }

    uint32_t calcOffset(){
      return 2*c_numTrailerWords*m_asic;
    }

    template<typename TYPE>
    TYPE word(const uint8_t * data, uint32_t &num){
      TYPE word = *((TYPE *) &data[num]);
      num += sizeof(TYPE);
      return word;
    }

    bool update(const uint8_t * trailerData)
    {
      uint32_t byteNr = 0;
      m_temp0 = utils::convertGCC(word<uint16_t>(trailerData,byteNr));
      m_temp1 = utils::convertGCC(word<uint16_t>(trailerData,byteNr));

      m_vetoCnt = word<uint32_t>(trailerData,byteNr);

      uint32_t xor_h;
      xor_h  =  word<uint32_t>(trailerData,byteNr);
      m_roXor = std::bitset<21>(xor_h);
      byteNr += 2;
      m_testPattern = word<uint16_t>(trailerData,byteNr);

      m_valid = true;
      return m_valid;
    }

    uint16_t m_vetoCnt;
    uint16_t m_temp0;
    uint16_t m_temp1;
    uint16_t m_testPattern;
    std::bitset<21> m_roXor;
    bool m_valid;
    int m_asic;
};

using DsscAsicTrailerDataArr = std::array<DsscAsicTrailerData,16>;

}


#endif // DSSCTRAINDATATYPES_H
