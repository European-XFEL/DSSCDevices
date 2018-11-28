#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <stdint.h>
#include <vector>
#include <string>
#include <fstream>
#include <map>

#include "SequencerTrack.h"

namespace SuS
{

#ifdef CNTRL1IO
//      enum TrackNum {RMP=0, Inject=1, FCF_ResAmp_B=2, FCF_Res_B=3, FCF_Sw1=4, FCF_SwIn=5};
#endif

#if defined(MM3IO) || defined(F1IO)
//      enum TrackNum {RMP=0, FCF_SwIn=1, FCF_Res_B=2, FCF_Flip_Inject=3, ISubPulse=4
// not used in MM3     ADC_RMP=0, FCF_Sw1=3, Inject=3, FCF_ResAmp_B=2};
#endif


#if defined(F2IO) || defined(F2BIO)
//      enum TrackNum {RMP=0, FCF_Res_B=3, FCF_SwIn=2, FCF_Flip=1, Inject=4, ADC_RMP=0, FCF_Sw1=1, FCF_ResAmp_B=3};
#endif


  class Sequencer
  {
    friend class SequencerTrack;

    public :

      Sequencer(std::string filename = "", bool initTracks=true);
      virtual ~Sequencer();

      struct HoldCntsRegisterEntry {
        HoldCntsRegisterEntry(uint16_t _length, bool _hold) :
          length(_length), hold(_hold) {}
        uint16_t length;  // c_holdCntWidth bits
        bool hold;        // flags a hold phase
      };

      typedef SequencerTrack::SequencePhase SequencePhase;

      typedef std::vector<HoldCntsRegisterEntry> HoldCntsRegister;

      using TrackNum = int;
      static TrackNum ADC_RMP;
      static TrackNum FCF_SwIn;
      static TrackNum FCF_Res_B;
      static TrackNum FCF_Flip;
      static TrackNum ISubPulse;
      static TrackNum Inject;

      // The track enum does not need to be the sequencer address anymore
      // for MM7 the allocation has been changed.
      // The following vector stores at location i, which is also the physical
      // address in the chip, the index (enum) of the track in the software
      // sequencer class.
      //static std::vector<TrackNum> seqTrackIndexForPhysAddrVec;

      //static TrackNum getSeqTrackIndex(int seqTrackAddr) {
      //  return seqTrackIndexForPhysAddrVec[seqTrackAddr];
      //}
      
      TrackNum getSeqTrackIndex(int seqTrackAddr);
      inline int getTrackJtagSubAddress(TrackNum n) {return getTrack(n)->jtagSubAddress;}
      std::string getTrackName(TrackNum n) {return getTrack(n)->name;}

      enum OpMode {NORM=0, SINGLEINT=1, BUFFER=2, RESET=3, MANUAL=4, EXTLATCH=5, DEPFET=6};
      enum SeqParam {
        RampLength=0, ResetLength=1, ResetHoldLength=2, ResetIntegOffset=3,
        IntegrationLength=4, FlattopLength=5, FlattopHoldLength=6,
        HoldPos=7, HoldLength=8, BackFlipAtReset=9, BackFlipToResetOffset=10, 
        RampOffset=11 , SingleCapLoadLength=12, SingleSHCapMode=13, 
        CycleLength=14,             // cycle length of the sequencer, must be the same as in the master FSM config reg
        InjectRisingEdgeOffset=15,  // duration from beginning of cycle until inject switches high
        LastIntPhase=16,            // phase from falling edge of 2nd integration to start of next cycle
        EmptyInjectCycles=17,       // number of cylces which are empty between two injections, and at the start of the burst
        RightShift=18,              // after generation of the sequencer, it is shifted to the right, works across the cycle boundary
        FtInjectOffset=19,          // clock cycles between end of first integration and inject falling edge (polarity can be set in pixel register)
        FtFlipOffset=20,            // clock cycles between end of first integration and flip falling edge (first phase must be high for reset)
        Invalid=999};

    bool adaptTracks();
    bool adaptTracks(int intTime, int time);
    void setAfterResetWait(int waitCycles);
    void setRMPPos(int rmpPos);
    void setSingleCapModeRmpPhases();
    void setFlattopLength(int flattop);
    void adaptFlipSignalToSingleSHCapMode();
    void setFlipPhases(int flipPos, int flipHighLength);
    void setFlipPhases();
    bool setIntegrationTime(int intTime);
    void setSwInPhases(int firstIntegOffset, int time1, int flattop, int time2);
    void setSequencerParameter(SeqParam _p, int _setting, bool genSignals=true);
    void setSequencerParameter(std::string _paramName, int _setting, bool genSignals=true);
    void setTracksNotCompiled();
    SeqParam paramNameToSeqParam(const std::string &paramName);

    inline int getNumHoldProgBits() const { return ((c_holdCntWidth+1)*c_holdCntsRegDepth); }
    inline int getNumTrackBits(TrackNum n) const{ return tracks[n]->getNumBits();}

    void initVariablesFromPhases();
    bool isOneFlipPhase();
    int getFirstIntegOffset();
    int getFirstIntegrationLength();
    int getFirstInjectLength();
    int getResetLength();
    int getResetIntegOffset();
    int getResetHoldLength();
    int getResetToRmpDiff();
    bool getResetTimeAndOffset(int &resTime, int &resOffset);
    int getRampLength();
    int getRmpPos();
    int getFlattopLength();
    int getFlattopHoldLength();
    int getIntegrationTime(bool noHold = false);
    int getFlipHighLength();
    int getFirstFlipPhase();
    int getRampIntegOffset();
    int getRmpEnd();
    int getHoldIntegrationTime();
    int getSequencerParameter(SeqParam _p) const;
    int getSequencerParameter(std::string _paramName);
    void setJtagSubAddressAndName(TrackNum n, int addr, std::string name);
    static std::string getParameterName(SeqParam _p);
    static std::vector<std::string> getParameterNames();
    Sequencer::OpMode getOpMode();
    static std::string getOpModeStr(Sequencer::OpMode opMode);

//      static const std::map<SeqParam,std::string> seqParamToNames{{RampLength,"RampLength"},
//                                                                  {ResetLength,"ResetLength"},
//                                                                  {ResetHoldLength,"ResetHoldLength"},
//                                                                  {ResetIntegOffset,"ResetIntegOffset"},
//                                                                  {IntegrationLength,"IntegrationLength"},
//                                                                  {FlattopLength,"FlattopLength"},
//                                                                  {FlattopHoldLength,"FlattopHoldLength"},
//                                                                  {HoldLength,"HoldLength"},
//                                                                  {HoldPos,"HoldPos"},
//                                                                  {BackFlipAtReset,"BackFlipAtReset"},
//                                                                  {BackFlipToResetOffset,"BackFlipToResetOffset"},
//                                                                  {RampOffset,"RampOffset"},
//                                                                  {SingleCapLoadLength,"SingleCapLoadLength"},
//                                                                  {SingleSHCapMode,"SingleSHCapMode"}};

//      std::map<std::string,SeqParam> seqParams;

    int getMaxIntegrationTime();
    int getMaxResetWait();
    int getMaxFlattopLength(bool inclHold = true);
    int getMaxResetIntegOffset();
    int getMaxRampIntOffset();
    int getMaxBackFlipToResetOffset();


    int getTotalClockCycles(TrackNum trackNum = ADC_RMP);
    void setSw1Edge();
    inline void setGuiEnabled(bool enable){enableGui = enable;}

    // methods to obtain bit streams for programming
    void getTrackDdynProgBits(TrackNum n, std::vector<bool>& bit) const;
    void getHoldProgBits(std::vector<bool>& bits) const;
    void getStatVals(std::vector<bool>& bits) const;
    void appendInvHolds(std::vector<bool>& bits);

    bool compareTrackContent(const std::vector<bool> &data_vec, TrackNum track);
    bool compareHoldContent(const std::vector<bool> &data_vec);

    std::map<std::string, uint32_t> getSequencerParameterMap() const;

    int trackNameToNum(std::string name);
    std::string trackNumToName(TrackNum n) {return getTrack(n)->name;}
    bool compileAllTracks();
    virtual bool compileAndCheckAllTracks();
    static bool setHoldCnts(HoldCntsRegister _holdCnts);
    static bool checkHoldCnts(SuS::Sequencer::HoldCntsRegister & _holdCnts);
    static bool correctNumberOfHolds(SuS::Sequencer::HoldCntsRegister & _holdCnts);

    static bool holdCntsInSyncWithCycleLength(const SuS::Sequencer::HoldCntsRegister &_holdCnts);
    static int getCycleLength() {return cycleLength;}
    int getHoldCnts();
    void setZeroHolds();
    int getRealCycleLength();
    void setExtLatchSlot(int slot);
    int getExtLatchSlot();

    void setInjectSlot(int slot);
    int getInjectSlot();
    void setInjectPhases(int injectPos, int injHighLength);

    bool getSignalCompilerMode() {return signalsCompilerMode;}

    static int c_numOfTracks;
    static int c_numOfParams;
    static int c_holdCntWidth;
    static int c_holdCntsRegDepth;

    bool isGood(){return configGood;}
    void printHoldCounts();
    void printTracks();
    void findHoldPositions(int& firstPos, int& firstLength, int& secondPos, int& secondLength);
    void findIntegrationPositions(int& firstPos, int& firstLength, int& secondPos, int& secondLength);
    void setFilterInBufferMode();

    SequencerTrack* getTrack(TrackNum n); // {return tracks[(int)n];};

    typedef std::vector<SequencerTrack*> SequencerTracks;
    SequencerTracks tracks;
    static bool debugMode;
    std::string filename;
    // holds the measurement cycle length which must be written to the MasterFSM config reg
    static int cycleLength;
    static HoldCntsRegister holdCnts;
    static bool holdEnabled;
    bool configGood;

    // signal compiler variables
    int resetLength;
    int resetIntegOffset;
    int resetHoldLength;
    int integrationLength;
    int flattopLength;
    int ftFlipOffset;
    int flattopHoldLength;
    //int flipOffset;
    int rampLength;
    int rampIntegOffset;
    //int scndInt2ToBackFlipLength;
    int holdLength; // single arbitrary hold
    int holdPos; // single arbitrary hold
    int backFlipAtReset; // bool needed, but set and get by setSequencerParameter, which needs an int
    int backFlipToResetOffset;
    int injectRisingEdgeOffset;
    int ftInjectOffset;
    int emptyInjectCycles;
    int rightShift;
    
    bool singleSHCapMode;
    int singleCapLoadLength;
    
    int lastIntPhase;
    bool signalsCompilerMode;
    bool enableGui;
    bool enableIntTimeExtension;
    OpMode mode;
    void setResetHoldLength(int _resetHoldLength);

    //void disableFrontendSignals();
    bool saveToFile();
    bool saveToFile(const std::string &_filename);
    bool loadFile(std::string _filename);
    void setFileName(std::string _filename);
    const std::string& getFilename() {return filename;}

    virtual void setCycleLength(int _cycleLength);
    static void setHoldEnabled(bool _holdEnabled);
    static bool getHoldEnabled() {return holdEnabled;}
    static void setDebugMode(bool _debugMode) {debugMode = _debugMode;}
    void disableSecondHold();
    bool setHoldPositions(int firstPos, int firstLength, int secondPos=0, int secondLength=0);
    void setResetLength(int _resetLength) {resetLength = _resetLength;}
    void setIntegrationLength(int _integrationLength) {integrationLength = _integrationLength;}

    void setOpMode(OpMode _mode, bool generate = true);
    void setOpMode(const std::string & _modeStr, bool generate = true);
    //void setDEPFETMode(bool mode);
    //void setManualMode(bool mode);
    void setExtLatchMode();

    void setRampLength(int _rampLength) {rampLength = _rampLength;}
    bool generateSignals();
    bool generateSignals(int _itegrationTime, int _flattop, int _flattopHoldLength,
        int _resetLength,  int _resetIntegOffset, int _resetHoldLength,
        int _rampLength, int _rampIntegOffset, int backFlipAtReset=0, int backFlipToResetOffset=0, int _singleCapLoadLength = 20);
    void setSignalCompilerMode(bool _signalsCompilerMode) {signalsCompilerMode = _signalsCompilerMode;}
    void setRMPPhases(int rmpPos, int rmpLength);
    void setSwInPhases(int firstIntegOffset, int time , int flattop);
    void setResetPhases(int resPos, int resLength);
    void setSingleSHCapMode(bool en);
    inline bool isSingleSHCapMode() const {return singleSHCapMode;}
    
    bool hasOneFlipPhase();

    void disableSwIn();
    void disableFlip();
    void disableRes_N();

    void setDEPFETHolds(int resetOffset, int flipHighLength);

    void generateSimPhases(int t);
    void setIntTimeExtension(bool extend);
//      void emitContentChanged();
    
    void shiftHolds(int shift);
    void shiftHoldsRight(int shift);
    void shiftTrack(SequencerTrack * track, int shift);
    void shiftTrackRight(SequencerTrack * track, uint32_t shift);
    void shiftAllTracksRight(uint32_t shift);

  private:
    int getValueFromXMLLine(const std::string &line, std::string valueName);
    std::string getStringFromXMLLine(const std::string &line, std::string valueName);
    OpMode getOpModeFromXMLLine(const std::string &line);

    void writeXMLCycleParameters(std::ofstream &ofs);
    void writeXMLSequencerTrack(std::ofstream &ofs, TrackNum trackNum);
    void writeXMLPhase(std::ofstream &ofs, const SequencePhase& phase);
    void writeXMLHoldRegister(std::ofstream &ofs, const HoldCntsRegisterEntry& regEntry);
    void writeXMLHoldCounts(std::ofstream &ofs);

    static std::map<std::string, SeqParam> paramNamesMap;

//     signals :
//
//       void contentChanged();
//       void filenameChanged(std::string const& filename);
//       void cycleLengthChanged(int cycleLength);
//       void programSequencer();
//       void doSingleBurst();

  //private :

  //  SignalCompilerParameters sigCompParams;

    std::vector<std::string> compareErrors;
    bool fieldNotFound;


  public:
      inline const std::vector<std::string> & getCompareErrors() const {return compareErrors;}
      inline void clearCompareErrors() { compareErrors.clear(); }
    
      class ConfigKeeper{

        public:
          ConfigKeeper(Sequencer * _seq,Sequencer::OpMode newMode = Sequencer::MANUAL)
          : seq(_seq)
          {
            mode = seq->getOpMode();
            holdEnabled           = seq->getHoldEnabled();
            cycleLength           = seq->getCycleLength();
            rampLength            = seq->getSequencerParameter(SeqParam::RampLength);
            resetLength           = seq->getSequencerParameter(SeqParam::ResetLength);
            resetHoldLength       = seq->getSequencerParameter(SeqParam::ResetHoldLength);
            resetIntegOffset      = seq->getSequencerParameter(SeqParam::ResetIntegOffset);
            integrationLength     = seq->getSequencerParameter(SeqParam::IntegrationLength);
            flattopLength         = seq->getSequencerParameter(SeqParam::FlattopLength);
            flattopHoldLength     = seq->getSequencerParameter(SeqParam::FlattopHoldLength);
            backFlipAtReset       = seq->getSequencerParameter(SeqParam::BackFlipAtReset); 
            backFlipToResetOffset = seq->getSequencerParameter(SeqParam::BackFlipToResetOffset);
            singleCapLoadLength   = seq->getSequencerParameter(SeqParam::SingleCapLoadLength);
            singleSHCapMode       = seq->getSequencerParameter(SeqParam::SingleSHCapMode);
            injectRisingEdgeOffset = seq->getSequencerParameter(SeqParam::InjectRisingEdgeOffset);
            seq->setOpMode(newMode);
          }

          ~ConfigKeeper()
          {
            seq->setHoldEnabled(holdEnabled);
            seq->setCycleLength(cycleLength);
            seq->setSequencerParameter(SeqParam::RampLength,rampLength,false);
            seq->setSequencerParameter(SeqParam::ResetLength,resetLength,false);
            seq->setSequencerParameter(SeqParam::ResetHoldLength,resetHoldLength,false);
            seq->setSequencerParameter(SeqParam::ResetIntegOffset,resetIntegOffset,false);
            seq->setSequencerParameter(SeqParam::IntegrationLength,integrationLength,false);
            seq->setSequencerParameter(SeqParam::FlattopLength,flattopLength,false);
            seq->setSequencerParameter(SeqParam::FlattopHoldLength,flattopHoldLength,false);
            seq->setSequencerParameter(SeqParam::BackFlipAtReset,backFlipAtReset,false);
            seq->setSequencerParameter(SeqParam::BackFlipToResetOffset,backFlipToResetOffset,false);
            seq->setSequencerParameter(SeqParam::SingleCapLoadLength,singleCapLoadLength,false);
            seq->setSequencerParameter(SeqParam::SingleSHCapMode,false);
            seq->setSequencerParameter(SeqParam::InjectRisingEdgeOffset,injectRisingEdgeOffset,false);
            seq->setOpMode(mode); // this also calls generateSignals()

            //seq->compileAndCheckAllTracks(); // done by the last setSequencerParameter
          }

        private:
          Sequencer *seq;
          Sequencer::OpMode mode;

          int cycleLength;
          bool holdEnabled;          

          // signal compiler variables
          int resetLength;
          int resetIntegOffset;
          int resetHoldLength;
          int integrationLength;
          int flattopLength;
          int flattopHoldLength;
          int rampLength;
          int backFlipAtReset;
          int backFlipToResetOffset;
          int singleCapLoadLength;
          int singleSHCapMode;
          int injectRisingEdgeOffset;
      };

  }; // Sequencer

} // SuS


#endif
