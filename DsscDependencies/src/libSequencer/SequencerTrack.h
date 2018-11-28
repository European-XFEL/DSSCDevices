#ifndef SEQUENCERTRACK_H
#define SEQUENCERTRACK_H

#include "Sequencer.h"

namespace SuS
{

  class SequencerTrack
  {
    friend class Sequencer;
    friend class SequencerQt;
    friend class Sequencer_ChipBugFixes;
    friend class Sequencer_GUI;
    friend class CHIP_GUI;
    friend class CNTRL1_GUI;
    friend class MM3_GUI;
    friend class F1_GUI;
    friend class F2_GUI;
    friend class MMX_GUI;
    friend class MeasurementWidget;

    // holds an abstract description of the sequence in the phases variable as well as the
    // values which need to be written to the registers in the chip. calling the compile()
    // function calculates the values which need to be written to the chip registers from
    // the phases vector

    public :

      SequencerTrack();
      virtual ~SequencerTrack();

      void getTrackDdynProgBits(std::vector<bool>& bit);
      bool compareContent(const std::vector<bool>& data_vec, std::vector<std::string> & compareErrors);
      int getSequenceLength();

      void setStatic(bool high);

      //  helper struct to describe the sequences can be low / high and stores the length
      struct SequencePhase {
        SequencePhase(bool _high, int _clockCycles) :
          high(_high), clockCycles(_clockCycles) {}
        bool high;
        int clockCycles;
      };
      typedef std::vector<SequencePhase> SequencePhases;

      // holds the values which need to be written to the registers in the chip
      struct PartialSequence {
        PartialSequence() : partSeq(0), repCnt(0) {}
        PartialSequence(int _partSeq, int _repCnt) : partSeq(_partSeq), repCnt(_repCnt) {}
        uint8_t partSeq;
        int repCnt;
      };

      typedef std::vector<PartialSequence> PartialSequences;

      static const int c_partSeqDepth;
      static const int c_partSeqWidth;
      static const int c_repCntWidth;
      static const int c_maxRepCntRegVal;
      static const int c_maxPhases;

      SequencerTrack& operator<<(SequencePhase p);

    protected :

      void setInvHold( bool _invHold) {invHold = _invHold;}
      bool getInvHold() const  {return invHold;}

      void setStatVal( bool _statVal) {statVal = _statVal;}
      bool getStatVal() const {return statVal;}
      void clearPhases();
      void setPhases(SequencePhases const& _phases);
      bool setPhasesAndCompile(SequencePhases const& _phases);
      bool addPhase(bool type, int length);
      void compile();
      void reverseCompile();
      bool compileAndCheck();


      bool checkCompiledPartSeqs();
      const SequencePhases& getPhases() {return phases;}
      void log();
      bool sequenceFeasible();
      void setTrackNum(int n) {trackNum = n;}
      inline int getTrackNum() const {return trackNum;}
      inline int getNumBits() const {return (c_partSeqDepth * (c_partSeqWidth + c_repCntWidth));}

      void addPartSeq(uint8_t _partSeq, int _repCnt);
      void setCompiledPartSeqs(PartialSequences const& compiledPartSeqs);

    //private :
      virtual bool checkCompiledPartSeqs(int firstHoldPos, int secondHoldPos);
      virtual bool compileAndCheck(int firstHoldPos, int secondHoldPos);
      virtual void addAlignedPhase(uint8_t _partSeq, int _repCnt);

      int trackNum;
      SequencePhases phases;             // software description of the sequence
      bool isCompiled;                   // if false compile needs to be called
      PartialSequences compiledPartSeqs; // as to be written to the chip
      bool statVal;                      // static value for iprog
      bool invHold;                      // invert hold signal (not used in MM3)

      // compilations variables
      //int currentClockCycle;
      int sequenceLength;                // length of the sequence in slow clock cycles derived from
                                         // the phases vector
      int maxRepCnt;                     // current maximum value which can be written to the register
                                         // this value must always hold the difference between the
                                         // current compiledPartSeqs.size() and the sequenceLength
                                         // to still be able to match the cycleLength

      int jtagSubAddress;                // sub address for programming
      std::string name;
  }; // SequencerTrack

} // namespace SuS

#endif
