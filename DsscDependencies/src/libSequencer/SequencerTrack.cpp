#include "Sequencer.h"
#include "SequencerTrack.h"
#include "utils.h"

#include <assert.h>
#include <sstream>
#include <iostream>

using namespace SuS;

#ifdef CNTRL1IO
const int SequencerTrack::c_partSeqDepth = 14;
const int SequencerTrack::c_partSeqWidth = 7;
const int SequencerTrack::c_maxRepCntRegVal = 15;
const int SequencerTrack::c_repCntWidth  = 4;
const int SequencerTrack::c_maxPhases = 6;
#endif

#if defined(MM3IO) || defined(F1IO) || defined(F2IO) || defined(F2BIO)
const int SequencerTrack::c_partSeqDepth = 14;
const int SequencerTrack::c_partSeqWidth = 7;
const int SequencerTrack::c_maxRepCntRegVal = 31;
const int SequencerTrack::c_repCntWidth  = 5;
const int SequencerTrack::c_maxPhases = 6;
#endif

//namespace{ SuS::logfile::subsystem_registrator log_id( "SeqTrack" ); }

#define log_id "SeqTrack"

#define SuS_LOG_STREAM(type,id,output)\
          std::cout << #type":SeqTrack: " << output << std::endl;

SequencerTrack::SequencerTrack() :
  trackNum(0),
  isCompiled(false),
  statVal(false),
  invHold(false),
  sequenceLength(0),
  maxRepCnt(0),
  jtagSubAddress(-1),
  name("undefined")
{
}


SequencerTrack::~SequencerTrack()
{
  //SuS_LOG_STREAM(debug, log_id, "SequencerTrack (trackNum " << trackNum << " ) destructor called.");
}


void SequencerTrack::clearPhases()
{
  phases.clear();
  isCompiled = false;
}


SequencerTrack& SequencerTrack::operator<<(SequencePhase p)
{
  phases.push_back(p);
  return *this;
}


bool SequencerTrack::compareContent(const std::vector<bool> & data_vec, std::vector<std::string> & compareErrors)
{
  std::vector<bool> bits;
  getTrackDdynProgBits(bits);

  if(bits.size() > data_vec.size()){
    compareErrors.push_back("Sequencer: Data Vector too short:" + std::to_string(data_vec.size())+ "/" + std::to_string(bits.size()));
    return false;
  }

  for(uint i=0; i<bits.size(); i++){
     if(bits[i]!=data_vec[i]){
       std::stringstream ss;
       ss << "Sequencer Track bits error at " << i << std::endl;
       ss << "Sequencer Track rbda:" << utils::boolVecToStdStr(data_vec) << std::endl;
       ss << "Sequencer Track bits:" << utils::boolVecToStdStr(bits)     << std::endl;
       compareErrors.push_back(ss.str());
       return false;
     }
  }
  return true;
}

void SequencerTrack::setStatic(bool high)
{
  clearPhases();
  *this << (SequencePhase(high, Sequencer::cycleLength*c_partSeqWidth));
}


bool SequencerTrack::addPhase(bool type, int length)
{
  SequencePhase newPhase(type, length);
  phases.push_back(newPhase);
  isCompiled = false;
  return true;
}


void SequencerTrack::setPhases(SequencePhases const& _phases)
{
  // combine phases with same signal polarity
  clearPhases();
  int combClockCycles = 0;
  bool high = false;
  for (unsigned int i=0; i<_phases.size(); ++i) {
    SequencePhase const& p = _phases[i];
    if ( (p.high && high) || (!p.high && !high) ) {
      combClockCycles += p.clockCycles;
    }
    else {
      if (combClockCycles > 0) {
        phases.push_back(SequencePhase(high, combClockCycles));
      }
      combClockCycles = p.clockCycles;
      high = p.high;
    }
  }
  phases.push_back(SequencePhase(high, combClockCycles));
  if (phases.size() > (int)c_maxPhases) {
    SuS_LOG_STREAM(warning, log_id, "More than " << c_maxPhases << " detected for track "
        << trackNum << ". This cannot be compiled.");
  }
}


bool SequencerTrack::setPhasesAndCompile(SequencePhases const& _phases)
{
  setPhases(_phases);
  return compileAndCheck();
}


void SequencerTrack::compile()
{
//   SuS_LOG_STREAM(debug, log_id, "pol is " << phases[0].high);
//   SuS_LOG_STREAM(debug, log_id, "length is " << phases[0].clockCycles);
//   SuS_LOG_STREAM(info, log_id, "Compiling sequence for track " << name

  if (isCompiled && Sequencer::debugMode) {
    SuS_LOG_STREAM(warning, log_id, "Track " << name << " was already compiled.");
    return;
  }

  sequenceLength = getSequenceLength();
  if (!sequenceFeasible()) {
    SuS_LOG_STREAM(error, log_id, "Error while compiling track. Nothing done...");
    //return false;
  }

  compiledPartSeqs.clear();

  // maxRepCnt holds the maximum number of repetitions allowed for a partial sequence
  // this is determined by the depth of the partial sequences register and changed throughout
  // the compilation process
  maxRepCnt = sequenceLength - c_partSeqDepth;
  int clockCyclesLeftInCurrent = 0;
  uint8_t partSeq = 0;

  for (unsigned int i=0; i<phases.size(); ++i) {
    SequencePhase const& p = phases[i];
    int clockCycles = p.clockCycles;
    uint8_t newBitVal = p.high ? 1 : 0;

    if (clockCyclesLeftInCurrent > clockCycles) { // partSeq not compete, dont add
      clockCyclesLeftInCurrent -= clockCycles;
      if (newBitVal) {
        partSeq |= ((1 << clockCycles) - 1) << clockCyclesLeftInCurrent;
      }
    }
    else { // partSeq complete, add it
      if (clockCyclesLeftInCurrent > 0) {
        if (newBitVal) {
          partSeq |= (1 << clockCyclesLeftInCurrent) - 1;
        }
        addAlignedPhase(partSeq, 0);
        clockCycles -= clockCyclesLeftInCurrent;
      }

      partSeq = newBitVal ? ((1<<c_partSeqWidth)-1) : 0;
      int repCnt = clockCycles / c_partSeqWidth - 1;
      if (repCnt >= 0) {
        addAlignedPhase(partSeq, repCnt);
      }

      // calculate the number of slots left over for the next partial sequence
      clockCycles %= c_partSeqWidth;
      if (clockCycles > 0) {
        clockCyclesLeftInCurrent = c_partSeqWidth - clockCycles;
        partSeq = newBitVal ? ((1 << clockCycles) - 1) << clockCyclesLeftInCurrent : 0;
      }
      else {
        clockCyclesLeftInCurrent = 0;
      }
    }
  }

  if (Sequencer::debugMode) {
    log();
  }
}


bool SequencerTrack::compileAndCheck()
{
  compile();
  return checkCompiledPartSeqs();
}


bool SequencerTrack::checkCompiledPartSeqs()
{
  bool retVal = true;
  int repCntSum = 0;

  for (unsigned int i=0; i<compiledPartSeqs.size(); ++i) {
    repCntSum += compiledPartSeqs[i].repCnt + 1;
  }

  sequenceLength = getSequenceLength();
  if (repCntSum != sequenceLength) { // double check repCnts
    SuS_LOG_STREAM(error, log_id, "Track " << trackNum << ": Sum of rep cnts is " << repCntSum
        << " but should be " << sequenceLength << ".");
    SuS_LOG_STREAM(error, log_id, "Track " << trackNum << ": Try different signal phase length or position.");
    retVal = false;
  }
  if ((int)compiledPartSeqs.size() != c_partSeqDepth) {
    SuS_LOG_STREAM(error, log_id, name << ", Number of added partial sequences is "
        << compiledPartSeqs.size() << " but should be " << c_partSeqDepth << ".");
    retVal = false;
  }


//   SuS_LOG_STREAM(info, log_id, "Sequencer track "
//       << Sequencer::trackNumToName(Sequencer::TrackNum(trackNum))
//       << " (TrackNum " << trackNum << ") compiled.");
  isCompiled = retVal;
  return retVal;
}


bool SequencerTrack::checkCompiledPartSeqs(int firstHoldPos, int secondHoldPos)
{
   SuS_LOG_STREAM(error, log_id, "Sequencer track can not do this function+++++++++++++++++++++" );
   return true;
}

bool SequencerTrack::compileAndCheck(int firstHoldPos, int secondHoldPos)
{
  SuS_LOG_STREAM(error, log_id, "Sequencer track can not do this function++++++++++++++++++++++" );
  return true;
}

void SequencerTrack::addAlignedPhase(uint8_t _partSeq, int _repCnt)
{
  if(_repCnt <= c_maxRepCntRegVal && _repCnt < maxRepCnt) {
    addPartSeq(_partSeq, _repCnt);
  }
  else {
    while (_repCnt >= c_maxRepCntRegVal && maxRepCnt >= c_maxRepCntRegVal) {
      addPartSeq(_partSeq, c_maxRepCntRegVal);
      _repCnt -= c_maxRepCntRegVal + 1;
    }
    while (_repCnt >= 0) {
      int repsToAdd = (_repCnt > maxRepCnt) ? maxRepCnt : _repCnt;
      addPartSeq(_partSeq, repsToAdd);
      _repCnt -= repsToAdd + 1;
    }
  }
}


void SequencerTrack::addPartSeq(uint8_t _partSeq, int _repCnt)
{
  compiledPartSeqs.push_back(PartialSequence(_partSeq, _repCnt));
  maxRepCnt         -= _repCnt;
}


void SequencerTrack::SequencerTrack::log()
{
  SuS_LOG_STREAM(info, log_id, "Logging track " << name << "."
      << "(TrackNum " << trackNum << ").");
  SuS_LOG_STREAM(info, log_id, "Static value is " << statVal);
  for (unsigned int i=0; i<compiledPartSeqs.size(); i++) {
    SuS_LOG_STREAM(debug, log_id, "PartSeq[" << i << "] : "
        << utils::intToBitString(compiledPartSeqs[i].partSeq,c_partSeqWidth)
        << ", repCnt: " << (int)compiledPartSeqs[i].repCnt);
  }
}


bool SequencerTrack::sequenceFeasible() {
  int cycleLength = Sequencer::getCycleLength();
  if ((int)phases.size() > c_maxPhases) {
    SuS_LOG_STREAM(warning, log_id, "The sequences cannot have more than 6 different phases. "
        << "Only for special cases...");
  }
  if (cycleLength < c_partSeqDepth) {
    SuS_LOG_STREAM(error, log_id, "The cycleLength (" << cycleLength
        << ") must be greater than or equal to " << c_partSeqDepth);
    return false;
  }
  if (sequenceLength%cycleLength!=0) {
    SuS_LOG_STREAM(error, log_id, "Track " << name << ": sequenceLength (" << sequenceLength
       << ") and cycleLength (" << cycleLength << ") do not have an integer relation.");
    return false;
  }
  if (Sequencer::debugMode) {
    SuS_LOG_STREAM(debug, log_id,"sequenceLength/cycleLength is " << sequenceLength/cycleLength);
  }
  return true;
}

void SequencerTrack::getTrackDdynProgBits(std::vector<bool>& bits)
{
  if (!isCompiled) {
    SuS_LOG_STREAM(warning, log_id, "Sequencer track " << name << " is not compiled.");
  }
  if (jtagSubAddress < 0) {
    SuS_LOG_STREAM(error, log_id, "Sequencer track " << name << ": jtagSubAddress is not defined.");
  }
  if((int)compiledPartSeqs.size()!=c_partSeqDepth){
    SuS_LOG_STREAM(error, log_id, "Sequencer Error: "
                      <<"PartSeqs:" << (int)compiledPartSeqs.size()
                      <<"/"<<c_partSeqDepth);
                      return;
  }
//   assert((int)compiledPartSeqs.size()==c_partSeqDepth);
  int totalBits = c_partSeqDepth * (c_partSeqWidth + c_repCntWidth);
  bits.clear();
  bits.resize(totalBits);

   // attention: partial sequences need to be written to the chip in reverse order!
  for (int i=0; i<c_partSeqDepth; ++i) {
    // TODO optimize by filling all ones / zeroes
    for (int j=0; j<c_repCntWidth; ++j) {
      if ((compiledPartSeqs[i].repCnt & (uint8_t)(1 << j)) != 0) {
        bits[(c_partSeqDepth-1-i) * (c_partSeqWidth + c_repCntWidth) + j] = true;
      }
    }
    for (int j=0; j<c_partSeqWidth; ++j) {
      if ((compiledPartSeqs[i].partSeq & (uint8_t)(1 << j)) != 0)
        bits[(c_partSeqDepth-1-i) * (c_partSeqWidth + c_repCntWidth) + c_repCntWidth + j] = true;
    }
  }
}


int SequencerTrack::getSequenceLength()
{
  int seqLength = 0;
  for (unsigned int i=0; i<phases.size(); ++i) {
    seqLength += phases[i].clockCycles;
  }
  if (seqLength%c_partSeqWidth!=0) {
    SuS_LOG_STREAM(warning, log_id, "Sequence length does not have an integer relation to the slow clock.");
  }
  return seqLength/c_partSeqWidth;
}


/*
 * To be able to set the compiled sequences directly through the manual GUI.
 */
void SequencerTrack::setCompiledPartSeqs(PartialSequences const& _compiledPartSeqs)
{
  compiledPartSeqs = _compiledPartSeqs;
}

/*
 * Reverses the compilation process for the GUI (for the case when the registers are directly set
 * through the GUI and the compiler is not used...).
 */
void SequencerTrack::reverseCompile()
{
  SequencePhases newPhases;
  for (unsigned int i=0; i<compiledPartSeqs.size(); ++i) {
    PartialSequence& ps = compiledPartSeqs[i];
    if ((ps.partSeq == 0) || (ps.partSeq == (1<<c_partSeqWidth)-1)) {
      newPhases.push_back(SequencePhase(!(ps.partSeq==0), (ps.repCnt+1)*c_partSeqWidth));
    }
    else {
      for (int r=0; r<ps.repCnt+1; ++r) {
        for (int j=c_partSeqWidth-1; j>=0; --j) {
          newPhases.push_back(SequencePhase((bool)(ps.partSeq & (1 << j)),1));
        }
      }
    }
  }
  phases = newPhases;
  setPhases(newPhases); // sets and cleans up the phases
}
