#include "Sequencer.h"
#include "SequencerTrack.h"
#include "utils.h"

#include <assert.h>
#include <sstream>
#include <iostream>
#include <fstream>

using namespace SuS;
using namespace std;

#define AIR 5   //after integration min time to rmp
#define AIF 2   //after integration min time to flip
#define MINFT 21 //minimum Flattop

//#define DEBUG

#ifdef CNTRL1IO
//  const int Sequencer::c_numOfTracks  = 6;
//  const int Sequencer::c_holdCntWidth = 10;
//  const int Sequencer::c_holdCntsRegDepth = 5;

//  const char* Sequencer::c_trackNames[] = {
//    "ADC_RMP",
//    "Inject",
//    "FCF_ResAmp_B",
//    "FCF_Res_B",
//    "FCF_Sw1",
//    "FCF_SwIn"
//  };
#endif

#if defined(MM3IO)
//  const int Sequencer::c_numOfTracks  = 4;
//  const int Sequencer::c_holdCntWidth = 14;
//  const int Sequencer::c_holdCntsRegDepth = 5;

//  const char* Sequencer::c_trackNames[] = {
//    "ADC_RMP",
//    "FCF_SwIn",
//    "FCF_Res_B",
//    "FCF_Flip_Inject"
//  };
#endif




bool Sequencer::debugMode = false;
Sequencer::HoldCntsRegister Sequencer::holdCnts;
int Sequencer::cycleLength;
bool Sequencer::holdEnabled;

//namespace{ SuS::logfile::subsystem_registrator log_id( "Sequencer" ); }

#define log_id() "Sequencer"

#define SuS_LOG_STREAM(type,id,output)\
          std::cout << "++++"#type": Sequencer: " << output << std::endl;

std::map<std::string, Sequencer::SeqParam> Sequencer::paramNamesMap;

int Sequencer::c_numOfTracks      = 5;
int Sequencer::c_holdCntWidth     = 14;
int Sequencer::c_holdCntsRegDepth = 5;
int Sequencer::c_numOfParams      = 21;

Sequencer::TrackNum Sequencer::ADC_RMP   = 0;
Sequencer::TrackNum Sequencer::FCF_SwIn  = 1;
Sequencer::TrackNum Sequencer::FCF_Res_B = 2;
Sequencer::TrackNum Sequencer::FCF_Flip  = 3;  // F1: flip and inject shared
Sequencer::TrackNum Sequencer::ISubPulse = 4;  // F1, no functionality
Sequencer::TrackNum Sequencer::Inject    = 4;  // F2: flip and inject split

//std::vector<std::string> Sequencer::c_trackNames = {
//  "ADC_RMP",
//  "FCF_SwIn",
//  "FCF_Res_B",
//  "FCF_Flip_Inject",
//  "ISubPulse"
//};
//
////must be changed only in F2
//std::vector<Sequencer::TrackNum> Sequencer::seqTrackIndexForPhysAddrVec { Sequencer::ADC_RMP,
//                                                                          Sequencer::FCF_SwIn,
//                                                                          Sequencer::FCF_Flip,
//                                                                          Sequencer::FCF_Res_B,
//                                                                          Sequencer::ISubPulse};

Sequencer::Sequencer(std::string filename, bool initTracks) :
   tracks(c_numOfTracks),
   configGood(false),
   ftFlipOffset(7),
   holdLength(0),
   holdPos(0),
   backFlipAtReset(0),
   backFlipToResetOffset(0),
   injectRisingEdgeOffset(0),
   ftInjectOffset(7),
   emptyInjectCycles(3),
   rightShift(0),
   singleSHCapMode(false),
   singleCapLoadLength(11),
   lastIntPhase(21),
   enableGui(true),
   enableIntTimeExtension(false),
   mode(NORM)
{
  if (paramNamesMap.empty()) {
    for(int i=0; i<c_numOfParams; i++){
      paramNamesMap[getParameterName((SeqParam)i)] = (SeqParam)i;
      //SuS_LOG_STREAM(info, log_id(), getParameterName((SeqParam)i) << " = " << i);
    }
  }

  if (initTracks) {
    //std::cout << "init tracks" << std::endl;
    for (unsigned int i=0; i<tracks.size(); ++i) {
      tracks[i] = new SequencerTrack;
      tracks[i]->setTrackNum(i);
//       SuS_LOG_STREAM(info, log_id(), "TrackNum = " << tracks[i]->trackNum <<
//           ", TrackName = " << Sequencer::c_trackNames[tracks[i]->trackNum]);
    }

    loadFile(filename);
  }

  // F1 config
                                    /*jtagSubAddress*/
  setJtagSubAddressAndName(ADC_RMP,   0, "ADC_RMP");
  setJtagSubAddressAndName(FCF_SwIn,  1, "FCF_SwIn");
  setJtagSubAddressAndName(FCF_Flip,  3, "FCF_Flip");
  setJtagSubAddressAndName(FCF_Res_B, 2, "FCF_Res_B");
  setJtagSubAddressAndName(ISubPulse, 4, "ISubPulse");
}


Sequencer::~Sequencer()
{
  for (unsigned int i=0; i<tracks.size(); ++i) {
    delete tracks[i];
  }
}


bool Sequencer::setIntegrationTime(int intTime)
{
  if(intTime<getMaxIntegrationTime()){
    setSequencerParameter(IntegrationLength,intTime);
    return true;
  }
  return false;
}

int Sequencer::getResetToRmpDiff()
{
  int cycleLengthFastClk = cycleLength*7;
  int rmpPos = getRmpPos();
  int resTime, resOffset;
  getResetTimeAndOffset(resTime,resOffset);
  int resRmpDiff = resOffset - rmpPos;
  if(resRmpDiff<0){
    if(abs(resRmpDiff)<resTime && mode!=BUFFER){
      SuS_LOG_STREAM(error, log_id(), "Rmp rises during reset, this is not a valid operation mode");
    }
  }

  return (resRmpDiff + cycleLengthFastClk)%cycleLengthFastClk;
}


void Sequencer::adaptFlipSignalToSingleSHCapMode()
{
  int resetToRmpDiff = getResetToRmpDiff();

  if(backFlipAtReset!=2){
    backFlipAtReset = 2;
  }
  if(backFlipToResetOffset >= resetToRmpDiff){
    backFlipToResetOffset = resetToRmpDiff-1;
  }
}


void Sequencer::setFlipPhases()
{
  int resTime;
  int resetOffset;
  getResetTimeAndOffset(resTime,resetOffset);

  int flipPos = resetOffset;
  //int flipLength = getResetIntegOffset() + resTime + getIntegrationTime() + flattopLength/2 /*flipOffset*/;
  int flipLength = getResetIntegOffset() + resTime + getIntegrationTime() + ftFlipOffset;
  setFlipPhases(flipPos,flipLength);
}

int Sequencer::getMaxBackFlipToResetOffset()
{
  int resTime;
  int resetOffset;
  getResetTimeAndOffset(resTime,resetOffset);

  int resToRmpDiff = getResetToRmpDiff();
  return std::max(0,std::min(resToRmpDiff-1,resetOffset-1));
}


void Sequencer::setFlipPhases(int flipPos, int flipHighLength)
{
  int cycleLengthFastClk = cycleLength*7;
  SequencerTrack::SequencePhases flipSeqPhases;
  if (mode==MANUAL) {
    int flipEnd = flipPos + flipHighLength;
    int lowLength = cycleLengthFastClk-flipHighLength-flipPos;
    bool firstFlipPhaseHigh = (flipEnd > cycleLengthFastClk);

    //SuS_LOG_STREAM(error, log_id(), "flipPos = " << flipPos);
    //SuS_LOG_STREAM(error, log_id(), "cycleLengthFastClk = " << cycleLengthFastClk);
    //SuS_LOG_STREAM(error, log_id(), "flipEnd = " << flipEnd);
    //SuS_LOG_STREAM(error, log_id(), "lowLength = " << lowLength);
    //SuS_LOG_STREAM(error, log_id(), "firstFlipPhaseHigh = " << firstFlipPhaseHigh);
    //SuS_LOG_STREAM(error, log_id(), "backFlipAtReset = " << backFlipAtReset);
    //SuS_LOG_STREAM(error, log_id(), "flipHighLength = " << backFlipAtReset);
    //SuS_LOG_STREAM(error, log_id(), "backFlipToResetOffset = " << backFlipToResetOffset);

    if(firstFlipPhaseHigh){
      int firstFlipHighPhase = flipEnd%cycleLengthFastClk;
      int secFlipHighPhase   = flipHighLength - firstFlipHighPhase;
      flipSeqPhases.push_back(SequencerTrack::SequencePhase(1,firstFlipHighPhase));
      flipSeqPhases.push_back(SequencerTrack::SequencePhase(0,lowLength));
      flipSeqPhases.push_back(SequencerTrack::SequencePhase(1,secFlipHighPhase));
    }else{
      if (backFlipAtReset == 0) { // flip at beginning of phase (best for sampling VHOLD)
        flipSeqPhases.push_back(SequencerTrack::SequencePhase(1,flipPos));
      } else if (backFlipAtReset == 1) {
        flipSeqPhases.push_back(SequencerTrack::SequencePhase(0,flipPos));
      } else {
        if(flipPos<backFlipToResetOffset){
          backFlipToResetOffset = flipPos-1;
        }
        flipSeqPhases.push_back(SequencerTrack::SequencePhase(0,flipPos - backFlipToResetOffset));
        flipSeqPhases.push_back(SequencerTrack::SequencePhase(1,backFlipToResetOffset));
      }
      flipSeqPhases.push_back(SequencerTrack::SequencePhase(1,flipHighLength));
      flipSeqPhases.push_back(SequencerTrack::SequencePhase(0,lowLength));
    }
  } else {
    int backFlipOffset = cycleLengthFastClk-lastIntPhase-2*integrationLength-flattopLength-resetIntegOffset-resetLength;
    /*int*/ flipHighLength = resetLength+resetIntegOffset+integrationLength+ftFlipOffset;
    flipSeqPhases.push_back(SequencerTrack::SequencePhase(0,backFlipOffset));
    flipSeqPhases.push_back(SequencerTrack::SequencePhase(1,flipHighLength));
    flipSeqPhases.push_back(SequencerTrack::SequencePhase(0,cycleLengthFastClk-backFlipOffset-flipHighLength));
    //SuS_LOG_STREAM(error, log_id(), "backFlipOffset= " << backFlipOffset);
    //SuS_LOG_STREAM(error, log_id(), "flipHighLength= " << flipHighLength);
    //SuS_LOG_STREAM(error, log_id(), "cycleLengthFastClk= " << cycleLengthFastClk);
    //int flipEnd = backFlipOffset+flipHighLength;
  }

  if(!getTrack(Sequencer::FCF_Flip)->setPhasesAndCompile(flipSeqPhases)) {
    SuS_LOG_STREAM(error, log_id(), "Error while updating sequencer flipInj.");
  }

  SequencerTrack::SequencePhases injectSeqPhases;
  if (injectRisingEdgeOffset>0) {
    SuS_LOG_STREAM(error, log_id(), "");
    injectSeqPhases.push_back(SequencerTrack::SequencePhase(0,injectRisingEdgeOffset));
  }
  if (emptyInjectCycles>0) {
    injectSeqPhases.push_back(SequencerTrack::SequencePhase(1,cycleLengthFastClk*emptyInjectCycles));
  }
  int injHighLength = cycleLengthFastClk-lastIntPhase-integrationLength-flattopLength-injectRisingEdgeOffset+ftInjectOffset;
  injectSeqPhases.push_back(SequencerTrack::SequencePhase(1,injHighLength));
  injectSeqPhases.push_back(SequencerTrack::SequencePhase(0,cycleLengthFastClk-injHighLength));

  if(!getTrack(Sequencer::Inject)->setPhasesAndCompile(injectSeqPhases)) {
    SuS_LOG_STREAM(error, log_id(), "Error while updating sequencer Inject track.");
  }
}


void Sequencer::setInjectPhases(int injectPos, int injHighLength)
{
  int cycleLengthFastClk = cycleLength*7;
  int injEnd = injectPos + injHighLength;
  int lowLength = cycleLengthFastClk-injHighLength-injectPos;

  //Set Inject
  SequencerTrack::SequencePhases injSeqPhases;
  bool firstInjPhaseHigh = (injEnd > cycleLengthFastClk);
  if(firstInjPhaseHigh){
    int firstInjHighPhase = injEnd%cycleLengthFastClk;
    int secInjHighPhase   = injHighLength - firstInjHighPhase;
    injSeqPhases.push_back(SequencerTrack::SequencePhase(1,firstInjHighPhase));
    injSeqPhases.push_back(SequencerTrack::SequencePhase(0,lowLength));
    injSeqPhases.push_back(SequencerTrack::SequencePhase(1,secInjHighPhase));
  }else{
    injSeqPhases.push_back(SequencerTrack::SequencePhase(0,injectPos));
    injSeqPhases.push_back(SequencerTrack::SequencePhase(1,injHighLength));
    injSeqPhases.push_back(SequencerTrack::SequencePhase(0,lowLength));
  }

  if(!getTrack(Sequencer::ISubPulse)->setPhasesAndCompile(injSeqPhases)) {
    SuS_LOG_STREAM(error, log_id(), "Error while updating sequencer Inj.");
  }
}


void Sequencer::setFlattopLength(int flattop)
{
  //SwIn Investigation
  const SequencerTrack::SequencePhases& swInTrackPhases = tracks.at(Sequencer::FCF_SwIn)->getPhases();
  if (swInTrackPhases.size()<4 && !(mode==SINGLEINT)) {
    SuS_LOG_STREAM(warning, log_id(), "SwIn track should have at least 4 phases but has " << swInTrackPhases.size());
    SuS_LOG_STREAM(warning, log_id(), "Using standard value of " << 35 << " for flattop set first integration 7 after reset.");
    return;
  }
  int firstIntegOffset = swInTrackPhases[0].clockCycles;
  int time = swInTrackPhases[1].clockCycles;

  bool depFetAvailable = (holdEnabled && mode==DEPFET);
  if(depFetAvailable){
    int newflattop=flattop;
    if(flattop>41){
      newflattop = 35 + flattop%7;
      int firstPos=0,secondPos=0,firstLength=0,secondLength=0;
      findHoldPositions(firstPos,firstLength,secondPos,secondLength);
      secondPos = (firstIntegOffset+time+AIF+1)/7+2;
      secondLength = (flattop-newflattop)/7;
      setHoldPositions(firstPos,firstLength,secondPos,secondLength);
    }else{
      disableSecondHold();
    }
    flattop = newflattop;
  }

  setSwInPhases(firstIntegOffset,time,flattop);
  int rmpPos = firstIntegOffset + 2*time + flattop + AIR;
  setRMPPos(rmpPos);

  compileAndCheckAllTracks();
}


void Sequencer::setRMPPos(int rmpPos)
{
  int cycleLengthFastClk = cycleLength*7;
  rmpPos = rmpPos%cycleLengthFastClk;

  setRMPPhases(rmpPos,rampLength);
}

void Sequencer::setSingleCapModeRmpPhases()
{
  int cycleLengthFastClk = cycleLength*7;

  SequencerTrack::SequencePhases rmpSeqPhases;

  int firstRMPHighPhase   = 26;
  int rmpLowLength        = cycleLengthFastClk - singleCapLoadLength - rampLength;
  int capSwitchPickleHigh = 10;
  int capSwitchPickleLow  = singleCapLoadLength - capSwitchPickleHigh;
  int secRMPHighPhase     = rampLength - firstRMPHighPhase;

  rmpSeqPhases.push_back(SequencerTrack::SequencePhase(1,firstRMPHighPhase));
  rmpSeqPhases.push_back(SequencerTrack::SequencePhase(0,rmpLowLength));
  rmpSeqPhases.push_back(SequencerTrack::SequencePhase(1,capSwitchPickleHigh));
  rmpSeqPhases.push_back(SequencerTrack::SequencePhase(0,capSwitchPickleLow));
  rmpSeqPhases.push_back(SequencerTrack::SequencePhase(1,secRMPHighPhase));

  if(!getTrack(Sequencer::ADC_RMP)->setPhasesAndCompile(rmpSeqPhases)) {
    SuS_LOG_STREAM(error, log_id(), "Error while updating sequencer ramp.");
  }
}


void Sequencer::setRMPPhases(int rmpPos, int rmpLength)
{
  int rmpEnd = rmpPos + rmpLength;

  int cycleLengthFastClk = cycleLength*7;

  //if(rmpLength>255){
  //  SuS_LOG_STREAM(warning, log_id(), "Ramp high phase longer than 255 - 9 bit mode disabled, solve by increase of first Res_N or SwIn phase");
  //}

/* MM3 Sequencer bugfix
  if(holdEnabled){
    int firstPos=0,secondPos=0,firstLength=0,secondLength=0;
    findHoldPositions(firstPos,firstLength,secondPos,secondLength);

    if( (rmpPos >= (firstPos*7)) && (rmpPos <= (firstPos+1)*7) ){
      rmpPos = firstPos*7;
      rmpLength = rmpEnd - rmpPos;
    }
    if( (rmpPos >= (secondPos-1)*7) && (rmpPos <= (secondPos*7)) ){
      rmpPos = (secondPos-1)*7;
      rmpLength = rmpEnd - rmpPos;
    }

    if( ((rmpEnd%cycleLengthFastClk) >= (firstPos*7)) && ((rmpEnd%cycleLengthFastClk) <= (firstPos+1)*7) ){
      rmpLength += 8;
    }
    if( ((rmpEnd%cycleLengthFastClk) >= (secondPos-1)*7) && ((rmpEnd%cycleLengthFastClk) <= (secondPos*7)) ){
      rmpLength += 8;
    }

    rmpEnd = rmpPos + rmpLength;
  }
*/

  SequencerTrack::SequencePhases rmpSeqPhases;
  bool firstRMPPhaseHigh = (rmpEnd > cycleLengthFastClk);
  if(firstRMPPhaseHigh){
    int firstRMPHighPhase = rmpEnd%cycleLengthFastClk;
    int rmpLowLength      = rmpPos - firstRMPHighPhase;
    int secRMPHighPhase   = rmpLength - firstRMPHighPhase;
    rmpSeqPhases.push_back(SequencerTrack::SequencePhase(1,firstRMPHighPhase));
    rmpSeqPhases.push_back(SequencerTrack::SequencePhase(0,rmpLowLength));
    rmpSeqPhases.push_back(SequencerTrack::SequencePhase(1,secRMPHighPhase));
  }else{
    rmpSeqPhases.push_back(SequencerTrack::SequencePhase(0,rmpPos));
    rmpSeqPhases.push_back(SequencerTrack::SequencePhase(1,rmpLength));
    rmpSeqPhases.push_back(SequencerTrack::SequencePhase(0,cycleLengthFastClk-rmpPos-rmpLength));
  }

  if(!getTrack(Sequencer::ADC_RMP)->setPhasesAndCompile(rmpSeqPhases)) {
    SuS_LOG_STREAM(error, log_id(), "Error while updating sequencer rmp.");
  }

  if(getRmpPos()!= rmpPos){
    SuS_LOG_STREAM(error, log_id(), "RmpPos changed during phase correction. Pos " << rmpPos << "/" << getRmpPos());
  }
  if(getRampLength() != rmpLength){
    SuS_LOG_STREAM(error, log_id(), "RmpLength changed during phase correction. Length " << rmpLength << "/" << getRampLength());
  }
  //rampLength = rmpLength;
}

void Sequencer::setSwInPhases(int firstIntegOffset, int time, int flattop)
{
  int cycleLengthFastClk = cycleLength*7;

  int lastSwInPhaseLength;
  if(mode==SINGLEINT){
    lastSwInPhaseLength = cycleLengthFastClk-firstIntegOffset-time;
  }else{
    lastSwInPhaseLength = cycleLengthFastClk-firstIntegOffset-flattop-2*time;
  }

  bool err = (lastSwInPhaseLength < 14 && !(mode==BUFFER || mode==RESET || mode==EXTLATCH));
  if(err){
    SuS_LOG_STREAM(error, log_id(), "Integration time of " << time << " not possible with this cycle length.");
    SuS_LOG_STREAM(error, log_id(), "Try integration time of " << time+((lastSwInPhaseLength-14)/2)-1 << " or shorter");
    return;
  }

  //Set SwIn
  SequencerTrack::SequencePhases swInSeqPhases;
  if(time>0){
    swInSeqPhases.push_back(SequencerTrack::SequencePhase(0,firstIntegOffset));
    swInSeqPhases.push_back(SequencerTrack::SequencePhase(1,time));
    if(mode==SINGLEINT){
      swInSeqPhases.push_back(SequencerTrack::SequencePhase(0,lastSwInPhaseLength));
    }else{
      swInSeqPhases.push_back(SequencerTrack::SequencePhase(0,flattop));
      swInSeqPhases.push_back(SequencerTrack::SequencePhase(1,time));
      swInSeqPhases.push_back(SequencerTrack::SequencePhase(0,lastSwInPhaseLength));
    }
  }else{
    swInSeqPhases.push_back(SequencerTrack::SequencePhase(0,cycleLengthFastClk));
  }
  if(!getTrack(Sequencer::FCF_SwIn)->setPhasesAndCompile(swInSeqPhases)) {
    SuS_LOG_STREAM(error, log_id(), "Error while updating sequencer swIn.");
  }
}

void Sequencer::setSwInPhases(int firstIntegOffset, int time1, int flattop, int time2)
{
  int cycleLengthFastClk = cycleLength*7;

  int lastSwInPhaseLength;
  if(mode==SINGLEINT){
    lastSwInPhaseLength = cycleLengthFastClk-firstIntegOffset-time1;
  }else{
    lastSwInPhaseLength = cycleLengthFastClk-firstIntegOffset-flattop-time1-time2;
  }

  //Set SwIn
  SequencerTrack::SequencePhases swInSeqPhases;
  if(time1>0){
    swInSeqPhases.push_back(SequencerTrack::SequencePhase(0,firstIntegOffset));
    swInSeqPhases.push_back(SequencerTrack::SequencePhase(1,time1));
    if(mode==SINGLEINT){
      swInSeqPhases.push_back(SequencerTrack::SequencePhase(0,lastSwInPhaseLength));
    }else{
      swInSeqPhases.push_back(SequencerTrack::SequencePhase(0,flattop));
      if(time2>0){
        swInSeqPhases.push_back(SequencerTrack::SequencePhase(1,time2));
      }
      swInSeqPhases.push_back(SequencerTrack::SequencePhase(0,lastSwInPhaseLength));
    }
  }else{
    swInSeqPhases.push_back(SequencerTrack::SequencePhase(0,cycleLengthFastClk));
  }
  if(!getTrack(Sequencer::FCF_SwIn)->setPhasesAndCompile(swInSeqPhases)) {
    SuS_LOG_STREAM(error, log_id(), "Error while updating sequencer swIn.");
  }
}

void Sequencer::setResetPhases(int resPos, int resLength)
{
  int cycleLengthFastClk = cycleLength*7;

  SequencerTrack::SequencePhases resnSeqPhases;
  resnSeqPhases.push_back(SequencerTrack::SequencePhase(1,resPos));
  resnSeqPhases.push_back(SequencerTrack::SequencePhase(0,resLength));
  resnSeqPhases.push_back(SequencerTrack::SequencePhase(1,cycleLengthFastClk-resLength-resPos));
  if(!getTrack(Sequencer::FCF_Res_B)->setPhasesAndCompile(resnSeqPhases)) {
    SuS_LOG_STREAM(error, log_id(), "Error while updating sequencer resn.");
  }
}

void Sequencer::setSingleSHCapMode(bool en)
{
  singleSHCapMode = en;
  if(singleSHCapMode){
    adaptFlipSignalToSingleSHCapMode();
  }
}



void Sequencer::setAfterResetWait(int waitCycles)
{
  //Reset Investigation
  int resTime;
  int resetOffset;
  getResetTimeAndOffset(resTime,resetOffset);

   //SwIn Investigation
  int flattop = getFlattopLength();
  int time    = getIntegrationTime(true);

  int firstIntegOffset = resetOffset + resTime + 1 + waitCycles;

  setSwInPhases(firstIntegOffset,time,flattop);
  int rmpPos = firstIntegOffset + 2 * time + flattop + AIR;
  setRMPPos(rmpPos);

  int flipPos = resetOffset;
  int flipLength = getResetIntegOffset() + resTime + getIntegrationTime() + 5;

  if(hasOneFlipPhase()){
    disableFlip();
  }else{
    setFlipPhases(flipPos,flipLength);
  }
}


bool Sequencer::adaptTracks()
{
  if(mode==MANUAL)
    return false;
  /* Sets the integration time with a maximum flattop length. For noise performance it should be
    * better to minimize the flattop, but the effect is probably negligible...
    */
  int intTime = getIntegrationTime(); //full integration Time with holds

  int time = (mode==BUFFER || mode==RESET || mode==EXTLATCH)? 0 : getIntegrationTime(true); // integration time without holds

  if( (mode==NORM||mode==SINGLEINT) && time == 0){
    time = 20;
  }

  return adaptTracks(intTime,time);
}


bool Sequencer::adaptTracks(int intTime, int time)
{

  if(mode==MANUAL)
    return false;
  //Reset Investigation
  int resTime;
  int resetOffset;
  getResetTimeAndOffset(resTime,resetOffset);
  //-------------------------------------------

  bool holdExtension = (enableIntTimeExtension && holdEnabled && (intTime >= 35));
  bool depFetAvailable = (!holdExtension) && (holdEnabled && mode==DEPFET);


  int cycleLengthFastClk = cycleLength*7;


  //SwIn Investigation
  int flattop = (mode == BUFFER || mode==RESET || mode==EXTLATCH)? 0 : getFlattopLength();
  if(depFetAvailable && flattop<35 && !(mode == BUFFER || mode==RESET||mode==EXTLATCH) && !hasOneFlipPhase()){
    SuS_LOG_STREAM(error, log_id(), "Flattop of " << flattop << " too short. Difficult to insert hold correctly. Minimum 35 using holds");
  }

  int firstIntegOffset = getFirstIntegOffset();


  if(firstIntegOffset <= resetOffset + resTime){
    //sets first integration in middle of available space (max 80 after reset)
    firstIntegOffset = cycleLengthFastClk-flattop-2*time-resetOffset-resTime - 14;
    firstIntegOffset /= 2;
    if(firstIntegOffset>57){
      firstIntegOffset = 57;
    }
    firstIntegOffset += resetOffset + resTime;
  }

  int lastSwInPhaseLength;
  if(mode==SINGLEINT){
    lastSwInPhaseLength = cycleLengthFastClk-firstIntegOffset-time;
  }else{
    lastSwInPhaseLength = cycleLengthFastClk-firstIntegOffset-flattop-2*time;
  }

  bool err = false;
  err |= (lastSwInPhaseLength < 14 && !(mode == BUFFER || mode==RESET||mode==EXTLATCH));
  err |= (depFetAvailable && flattop<21 && !(mode==SINGLEINT) && !(mode==BUFFER || mode==RESET||mode==EXTLATCH));
  if(err){
    SuS_LOG_STREAM(error, log_id(), "Integration time of " << time << " not possible with this cycle length.");
    SuS_LOG_STREAM(error, log_id(), "Try Integration time of " << time+((lastSwInPhaseLength-14)/2)-1 << " or shorter");
    return false;
  }

  //-------------------------------------------

  //RMP Investigation
  int MIN_RMP_LOW_LENGTH = 14;
  int firstRmpLength = cycleLength / 2;
  int rmpLowLength = MIN_RMP_LOW_LENGTH;
  int secRmpLength = cycleLengthFastClk-firstRmpLength-rmpLowLength;
  const SequencerTrack::SequencePhases& rmpTrackPhases = tracks.at(Sequencer::ADC_RMP)->getPhases();
  if (rmpTrackPhases.size()<3) {
    SuS_LOG_STREAM(warning, log_id(), "Rmp track should have at least 3 phases but has " << rmpTrackPhases.size());
    SuS_LOG_STREAM(warning, log_id(), "Using standard value of " << 7 << " for offset of first and last rmp phase.");
  } else {
    firstRmpLength = rmpTrackPhases[0].clockCycles;
    rmpLowLength   = rmpTrackPhases[1].clockCycles;
    secRmpLength   = rmpTrackPhases[2].clockCycles;
  }

  if(firstRmpLength < 100 && mode==NORM){
    firstRmpLength = 100;
    //SuS_LOG_STREAM(warning, log_id(), "Small first RMPHIgh length, can only convert VHold up to value " << firstRmpLength*2 << " Set first phase to 50 to remove warning");
  }

  if(firstRmpLength + rmpLowLength + secRmpLength != cycleLengthFastClk){
    secRmpLength = cycleLengthFastClk - firstRmpLength - rmpLowLength;
  }

  if(mode==BUFFER){
    //keep as defined
  }else if(mode==RESET){
    if((firstRmpLength < resetOffset + resTime + 2)){
      firstRmpLength = resetOffset + resTime + 30;
      secRmpLength = cycleLengthFastClk/2;
      rmpLowLength = cycleLengthFastClk - firstRmpLength - secRmpLength;
    }
  }else{
    if(firstRmpLength + rmpLowLength != (cycleLengthFastClk - lastSwInPhaseLength + AIR)){
      secRmpLength = lastSwInPhaseLength - AIR;
      rmpLowLength = cycleLengthFastClk - firstRmpLength - secRmpLength;
    }
  }

  if(rmpLowLength<=0){
    rmpLowLength = MIN_RMP_LOW_LENGTH;
    firstRmpLength = cycleLengthFastClk - rmpLowLength - secRmpLength;
  }


  int rmpPos    = firstRmpLength + rmpLowLength;
  int rmpLength = firstRmpLength + secRmpLength;

  if(mode==RESET){
    rmpLength = rmpLowLength;
    rmpPos    = firstRmpLength;
  }else if(mode==EXTLATCH){
    rmpLength = (cycleLengthFastClk/3 > 250)? 250 : cycleLengthFastClk/3;
    rmpPos    = cycleLength;
  }else if(mode==BUFFER){
    rmpLength = (cycleLengthFastClk-10 > 250)? 250 : cycleLengthFastClk-10;
    rmpPos    = cycleLengthFastClk-20;
  }
  //-------------------------------------------

  //Flip Inject Investigation
  bool oneFlipPhase = hasOneFlipPhase() && !mode==NORM;

  int flipPos = resetOffset;
  int flipHighLength;
  if(mode==SINGLEINT){
    flipHighLength = resetOffset + resetLength;

    if(!oneFlipPhase){
      flipHighLength += 30;
    }

    if(flipHighLength + 15 > firstIntegOffset){
      SuS_LOG_STREAM(error, log_id(), "SingleIntegration Mode: FlipHighLength can not be adapted correctly. "
          "Increase FirstIntegration Offset to min" << 46 + resetOffset + resetLength << " (min 46 after reset)" );
    }
  }else{
    flipHighLength = getResetIntegOffset() + time + resTime + AIF;
  }
  //-------------------------------------------


  //Set Holds - must be set before tracks are composed
  if(holdExtension){

    int firstPos  = firstIntegOffset/7 + 1;
    int secondPos =  cycleLength - (lastSwInPhaseLength/7) - 1;

    if(mode==SINGLEINT){
      firstPos = secondPos;
      secondPos = 0;
    }

    int length = (intTime - time) / 7;
    if(debugMode){
      SuS_LOG_STREAM(info, log_id(), "first hold " << firstPos << " second hold " << secondPos << " length " << length);
      SuS_LOG_STREAM(info, log_id(), "lastSwInPhaseLength " << lastSwInPhaseLength << " time " << time << " flattop "
          << flattop << " sum " << firstIntegOffset+time+flattop);
    }
    setHoldPositions(firstPos,length,secondPos,length);
    //printHoldCounts();
  }else if(enableIntTimeExtension){
    setZeroHolds();
  }else if(depFetAvailable){
    if(mode==SINGLEINT && !oneFlipPhase){
      setDEPFETHolds(resetOffset,resetOffset+resetLength);
    }else{
      setDEPFETHolds(resetOffset,flipHighLength+flipPos);
    }
  }
  //-------------------------------------------

  //Set Rmp
  setRMPPhases(rmpPos,rmpLength);

  //-------------------------------------------

  //Set SwIn
  if(mode == BUFFER || mode==RESET || mode==EXTLATCH){
    disableSwIn();
  }else{
    setSwInPhases(firstIntegOffset,time,flattop);
  }
  //-------------------------------------------

  //Set Res_N
  if(mode==BUFFER || mode==EXTLATCH){
    disableRes_N();
  }else{
    setResetPhases(resetOffset,resTime);
  }
  //-------------------------------------------

  //Set FlipInject
  if(mode == BUFFER || mode == RESET){
    disableFlip();
  }else if(mode != EXTLATCH){
    setFlipPhases(flipPos,flipHighLength);
  }
  //-------------------------------------------

  //Set ISubPulse
  SequencerTrack::SequencePhases isubPulsePhases;
  isubPulsePhases.push_back(SequencerTrack::SequencePhase(0,cycleLengthFastClk));
  if(!getTrack(Sequencer::ISubPulse)->setPhasesAndCompile(isubPulsePhases)) {
    SuS_LOG_STREAM(error, log_id(), "Error while updating sequencer isubPulse.");
  }
  //-------------------------------------------

  compileAndCheckAllTracks();


  //update sequencer variables
  if(mode==NORM){
    resetLength = resTime;
    integrationLength = intTime;
    rampLength = rmpLength;
  }
  //-------------------------------------------

  //done
  if(debugMode)
  {
    SuS_LOG_STREAM(info, log_id(), "IntegrationTime set to " << intTime);
  }

  return true;
}

bool Sequencer::hasOneFlipPhase()
{
  const SequencerTrack::SequencePhases& flipTrackPhases = tracks.at(FCF_Flip)->getPhases();
  bool oneFlipPhase;
  if ((flipTrackPhases.size() == 1) || mode==RESET || mode == BUFFER) {
      oneFlipPhase = true;
  } else if (flipTrackPhases.size()!=3 && mode!=EXTLATCH) {
    SuS_LOG_STREAM(warning, log_id(), "FlipInject track should have 3 phases but has " << flipTrackPhases.size());
  }

  return oneFlipPhase;
}

bool Sequencer::compareTrackContent(const std::vector<bool> &data_vec, TrackNum track)
{
  return tracks[track]->compareContent(data_vec,compareErrors);
}

bool Sequencer::compareHoldContent(const std::vector<bool> &data_vec)
{
  std::vector<bool> bits;
  getHoldProgBits(bits);

  if(bits.size() > data_vec.size()){
    compareErrors.push_back("Sequencer: Data Vector too short:" + std::to_string(data_vec.size())+ "/" + std::to_string(bits.size()));
    return false;
  }

  for(uint i=0; i<bits.size(); i++){
     if(bits[i]!=data_vec[i]){
       std::stringstream ss;
       ss << "Sequencer Hold bits error at " << i << std::endl;
       ss << "Sequencer Track rbda:" << utils::boolVecToStdStr(data_vec) << std::endl;
       ss << "Sequencer Track bits:" << utils::boolVecToStdStr(bits)     << std::endl;
       compareErrors.push_back(ss.str());
       return false;
     }
  }

  return true;
}


void Sequencer::getTrackDdynProgBits(TrackNum n, std::vector<bool>& bits) const
{
  tracks[n]->getTrackDdynProgBits(bits);
}


void Sequencer::getStatVals(std::vector<bool>& bits) const
{
  bits.clear();
  bits.resize(c_numOfTracks);
  for (int i=0; i<c_numOfTracks; ++i) {
    bits[i] = tracks[i]->getStatVal();
  }
}

void Sequencer::appendInvHolds(std::vector<bool>& bits)
{
  for (int i=0; i<c_numOfTracks; ++i) {
    bits.push_back(tracks[i]->getInvHold());
  }
}


void Sequencer::getHoldProgBits(std::vector<bool>& bits) const
{
  int totalBits = (c_holdCntWidth+1)*c_holdCntsRegDepth;
  bits.clear();
  bits.resize(totalBits);
  assert((int)holdCnts.size()==c_holdCntsRegDepth);

  if (holdEnabled) {
    for (int i=0; i<(int)holdCnts.size(); ++i) {
      uint16_t holdLength = holdCnts[holdCnts.size()-i-1].length - 1;
      /*
      if (holdLength<2) {
        SuS_LOG_STREAM(warning, log_id(), "holdLength = " << holdLength << " but must be > 1, set to 2.");
        holdLength = 2;
      }
      */
      for (int j=0; j<c_holdCntWidth; ++j) {
        if (holdLength & uint16_t(1 << j)) {
          bits[i*(c_holdCntWidth+1)+j] = true;
        }
      }
      // set the hold flag
      bits[c_holdCntWidth + i*(c_holdCntWidth+1)] = holdCnts[holdCnts.size()-i-1].hold;
    }
  }
}


bool Sequencer::compileAndCheckAllTracks()
{
  bool success = true;
  for (int i=0; i<c_numOfTracks; ++i) {
    success &= tracks[i]->compileAndCheck();
  }
  return success;
}

bool Sequencer::compileAllTracks()
{
  if (debugMode) {
    SuS_LOG_STREAM(debug, log_id(), "Compiling all sequencer tracks.");
  }
  bool success = compileAndCheckAllTracks();

  //if(success){
  //  integrationLength = getIntegrationTime();
  //  resetLength = getResetLength();
  //  rampLength = getRampLength();
  //}
  return success;
}


bool Sequencer::saveToFile()
{
  return saveToFile(filename);
}


bool Sequencer::saveToFile(const std::string & _filename)
{
  std::ofstream ofs (_filename, std::ofstream::out);

  ofs << "<!DOCTYPE Sequencer>" << std::endl;

  ofs << "<Sequencer ";
  if (signalsCompilerMode) ofs << "mode=\"signalsCompiler\"";
  ofs << " cycleLength=\"" << cycleLength << "\"";
  ofs << " holdGenEnabled=\"" << holdEnabled << "\"";
  ofs << " singleSHCapMode=\"" << singleSHCapMode << "\"";
  ofs << " opMode=\"" << (int)mode << "\"";
  ofs << ">" << std::endl;

  writeXMLCycleParameters(ofs);
  for(int i=0; i<c_numOfTracks; i++){
    writeXMLSequencerTrack(ofs,(Sequencer::TrackNum)i);
  }

  writeXMLHoldCounts(ofs);

  ofs << "</Sequencer>" << std::endl;

  ofs.close();

  utils::CoutColorKeeper keeper(utils::STDGREEN);
  cout << "Save Sequencer to File: " << _filename << endl;
  return true;
}


void Sequencer::writeXMLCycleParameters(std::ofstream &ofs)
{
  ofs << "<cycleParameters ";
  ofs << "integrationLength=\""     << integrationLength     << "\" ";
  ofs << "resetHoldLength=\""       << resetHoldLength       << "\" ";
  ofs << "flattopLength=\""         << flattopLength         << "\" ";
  ofs << "rampLength=\""            << rampLength            << "\" ";
  ofs << "singleCapLoadLength=\""   << singleCapLoadLength   << "\" ";
  ofs << "flattopHoldLength=\""     << flattopHoldLength     << "\" ";
  ofs << "resetIntegOffset=\""      << resetIntegOffset      << "\" ";
  ofs << "rampIntegOffset=\""       << rampIntegOffset       << "\" ";
  ofs << "resetLength=\""           << resetLength           << "\" ";
  ofs << "backFlipAtReset=\""       << backFlipAtReset       << "\" ";
  ofs << "backFlipToResetOffset=\"" << backFlipToResetOffset << "\"/>";
  ofs << "injectRisingEdgeOffset=\"" << injectRisingEdgeOffset<< "\"/>";
  ofs << "lastIntPhase=\""          << lastIntPhase          << "\"/>";
  ofs << "emptyInjectCycles=\""     << emptyInjectCycles     << "\"/>";
  ofs << "rightShift=\""            << rightShift            << "\"/>";
  ofs << "ftFlipOffset=\""          << ftFlipOffset          << "\"/>";
  ofs << "ftInjectOffset=\""        << ftInjectOffset        << "\"/>";
  ofs << std::endl;
}


void Sequencer::writeXMLSequencerTrack(std::ofstream &ofs, TrackNum trackNum)
{
  SequencerTrack& seqTrack = *getTrack(trackNum);
  std::string trackName = trackNumToName(trackNum);

  ofs << "<SequencerTrack ";
  ofs << "signalName=\"" << trackName             << "\" ";
  ofs << "invHold=\""    << seqTrack.getInvHold() << "\" ";
  ofs << "statVal=\""    << seqTrack.getStatVal() << "\">";
  ofs << std::endl;

  for(uint i=0; i<seqTrack.getPhases().size(); i++){
    writeXMLPhase(ofs,seqTrack.getPhases().at(i));
  }

  ofs << "</SequencerTrack>" << std::endl;
}


void Sequencer::writeXMLPhase(std::ofstream &ofs, const SequencePhase& phase)
{
  ofs << "<Phase ";
  ofs << "length=\"" << phase.clockCycles << "\" ";
  ofs << "type=\""   << phase.high        << "\"/>";
  ofs << std::endl;
}


void Sequencer::writeXMLHoldCounts(std::ofstream &ofs)
{
  ofs << "<HoldGenerator enabled=\"" << holdEnabled << "\">" << std::endl;

  for(uint i=0; i<holdCnts.size(); i++){
    writeXMLHoldRegister(ofs,holdCnts.at(i));
  }

  ofs << "</HoldGenerator>" << std::endl;
}


void Sequencer::writeXMLHoldRegister(std::ofstream &ofs, const HoldCntsRegisterEntry & regEntry)
{
  ofs << "<HoldCntsRegEntry ";
  ofs << "length=\"" << regEntry.length << "\" ";
  ofs << "hold=\""   << regEntry.hold   << "\"/>";
  ofs << std::endl;
}


bool Sequencer::loadFile(std::string _filename)
{
  rampIntegOffset = 20;
  signalsCompilerMode = false;
  holdCnts.clear();

  fieldNotFound = false;
  bool configOk = true;
  bool manualMode = false;
  int currHoldPhase = 0;

  std::ifstream infile(_filename,std::ifstream::in);
  if(!infile.is_open()){
    filename = "";
    configGood = false;
    return false;
  }

  setFileName(_filename);

  TrackNum currTrack;
  SequencerTrack* t = nullptr;
  std::string line;
  while (std::getline(infile, line))
  {
    if(line.length()>0)
    {

      if(line.find("<Sequencer ") != std::string::npos)
      {
        int length = getValueFromXMLLine(line,"cycleLength");
        setCycleLength(length);

        bool enabled = (getValueFromXMLLine(line,"holdGenEnabled")!=0);
        setHoldEnabled(enabled);

        singleSHCapMode = (getValueFromXMLLine(line,"singleSHCapMode")==1);

        std::string guiMode = getStringFromXMLLine(line,"mode");

        if(guiMode.compare("signalsCompiler")==0){
          signalsCompilerMode = true;
#ifdef DEBUG
          std::cout << "Signals compiler mode set." << std::endl;
#endif
        }else{
          signalsCompilerMode = false;
        }
        if(guiMode.compare("manual")==0){
          manualMode = true;
#ifdef DEBUG
          std::cout << "Manual mode set." << std::endl;
#endif
        }
      }else if(signalsCompilerMode && line.find("<cycleParameters") != std::string::npos)
      {
        resetLength           = getValueFromXMLLine(line,"resetLength");
        resetIntegOffset      = getValueFromXMLLine(line,"resetIntegOffset");
        resetHoldLength       = getValueFromXMLLine(line,"resetHoldLength");
        integrationLength     = getValueFromXMLLine(line,"integrationLength");
        flattopLength         = getValueFromXMLLine(line,"flattopLength");
        flattopHoldLength     = getValueFromXMLLine(line,"flattopHoldLength");
        rampIntegOffset       = getValueFromXMLLine(line,"rampIntegOffset");
        rampLength            = getValueFromXMLLine(line,"rampLength");
        singleCapLoadLength   = getValueFromXMLLine(line,"singleCapLoadLength");
        backFlipAtReset       = getValueFromXMLLine(line,"backFlipAtReset");
        backFlipToResetOffset = getValueFromXMLLine(line,"backFlipToResetOffset");
        injectRisingEdgeOffset = getValueFromXMLLine(line,"injectRisingEdgeOffset");
        lastIntPhase           = getValueFromXMLLine(line,"lastIntPhase");
        emptyInjectCycles      = getValueFromXMLLine(line,"emptyInjectCycles");
        rightShift             = getValueFromXMLLine(line,"rightShift");
        ftFlipOffset           = getValueFromXMLLine(line,"ftFlipOffset");
        ftInjectOffset         = getValueFromXMLLine(line,"ftInjectOffset");

        if (rampIntegOffset       < 0) rampIntegOffset        = 20;
        if (backFlipAtReset       < 0) backFlipAtReset        = 0;
        if (backFlipToResetOffset < 0) backFlipToResetOffset  = 0;
        if (singleCapLoadLength   < 0) singleCapLoadLength    = 11;
        if (injectRisingEdgeOffset< 0) injectRisingEdgeOffset = 0;
        if (lastIntPhase          < 0) lastIntPhase           = 21;
        if (emptyInjectCycles     < 0) emptyInjectCycles      = 0;
        if (rightShift            < 0) rightShift             = 0;
        if (ftFlipOffset          < 0) ftFlipOffset           = 7;
        if (ftInjectOffset        < 0) ftInjectOffset         = 7;

      }else if((signalsCompilerMode==false || manualMode) && line.find("<SequencerTrack") != std::string::npos)
      {
        std::string trackName = getStringFromXMLLine(line,"signalName");
        bool invHold = getValueFromXMLLine(line,"invHold") != 0;
        bool statVal = getValueFromXMLLine(line,"statVal") != 0;

        int x = trackNameToNum(trackName);

        if (x>-1) {
          currTrack = (TrackNum)x;
#ifdef DEBUG
          std::cout << "Track " << trackName << " found - number " << x << "!" << std::endl;
#endif
        }else{
          std::cout << "Track " << trackName << " unknown!" << std::endl;
          break;
        }

        t = tracks[currTrack];
        t->setStatVal(statVal);
        t->setInvHold(invHold);
        t->clearPhases();

      }else if((signalsCompilerMode==false || manualMode) && line.find("<Phase")!=std::string::npos){
        int phaseLength = getValueFromXMLLine(line,"length");
        bool phaseType = getValueFromXMLLine(line,"type") != 0;

        t->addPhase(phaseType, phaseLength);
#ifdef DEBUG
        std::cout << "Track " << currTrack << " added Phase: length " << phaseLength << " type " << phaseType << std::endl;
#endif
      }else if(line.find("<HoldGenerator")!=std::string::npos){
        holdEnabled = getValueFromXMLLine(line,"enabled") != 0;
      }else if(line.find("<HoldCntsRegEntry")!=std::string::npos){
        int holdLength = getValueFromXMLLine(line,"length");
        bool enabled = getValueFromXMLLine(line,"hold") != 0;
        holdCnts.push_back( HoldCntsRegisterEntry(holdLength, enabled));
        currHoldPhase++;
      }
    }
  }

  infile.close();

  if(signalsCompilerMode==true){
    generateSignals();
    setOpMode(getOpMode());
  }else if (!manualMode){
    initVariablesFromPhases();
    compileAndCheckAllTracks();
  }
  else { //manual mode
    compileAndCheckAllTracks();
  }

  if (!checkHoldCnts(holdCnts)) {
    configOk = false;
  }

  configGood = configOk;
  if(configGood) {
    utils::CoutColorKeeper keeper(utils::STDGREEN);
    std::cout << "++++ Sequencer initialized from file " << _filename <<  "!" << std::endl;
  }
  else  {
    utils::CoutColorKeeper keeper(utils::STDRED);
    std::cout << "++++ ERROR: Sequencer could not be read correctly!" << std::endl;
  }

  if(fieldNotFound){
    std::cout << "++++warning: Sequencer:Will update sequencer file with new values!" << std::endl;
    saveToFile();
  }

  return configOk;
}


Sequencer::OpMode Sequencer::getOpModeFromXMLLine(const std::string &line)
{
  int pos = line.find("opMode");
  if(pos < 0){
    std::cerr << "Sequencer Load File: OpMode not found set to manual mode " << std::endl;
    return MANUAL;
  }

  pos += 8;

  std::string linePart = line.substr(pos);

  pos = linePart.find("\"");

  linePart = linePart.substr(0,pos);

  //convert string to int;
  std::istringstream iss(linePart);
  int a;
  iss >> a; // error

#ifdef DEBUG
  std::cout << "getOpModeFromXMLLine: " << line << " is " << a << std::endl;
#endif

  return (Sequencer::OpMode)a;

}


int Sequencer::getValueFromXMLLine(const std::string &line, std::string valueName)
{
  int pos = line.find(valueName);
  if(pos < 0){
    utils::CoutColorKeeper keeper(utils::STDBROWN);
    std::cerr << "WARNING: getStringFromXMLFile: signalName " << valueName << " not found in " << line << std::endl;
    std::cerr << valueName << " not found." << std::endl;
    keeper.change(utils::STDGREEN);
    std::cerr << "Will store standard value to file" << std::endl;
    fieldNotFound = true;
    return -1;
  }

  pos += valueName.length() + 2;

  std::string linePart = line.substr(pos);

  pos = linePart.find("\"");

  linePart = linePart.substr(0,pos);

  //convert string to int;
  std::istringstream iss(linePart);
  int a;
  iss >> a; // error

#ifdef DEBUG
  std::cout << "getValueFromXMLFile: " << valueName << " is " << a << std::endl;
#endif

  return a;
}

std::string Sequencer::getStringFromXMLLine(const std::string &line, std::string valueName)
{
  int pos = line.find(valueName);
  if(pos < 0){
    std::cerr << "getStringFromXMLFile: signalName " << valueName << " not found in " << line << std::endl;
    return "0";
  }

  pos += valueName.length() + 2;

  std::string linePart = line.substr(pos);

  pos = linePart.find("\"");

  linePart = linePart.substr(0,pos);

#ifdef DEBUG
  std::cout << "getStringFromXMLFile: " << valueName << " is " << linePart << std::endl;
#endif

  return linePart;
}


/*

bool Sequencer::saveToFile(std::string _filename)
{
  SuS_LOG_STREAM(info, log_id(), "Saving to file " << _filename.toStdString());

  QDomDocument doc("Sequencer");
  QDomElement rootEl = doc.createElement("Sequencer");
  doc.appendChild(rootEl);
  rootEl.setAttribute("cycleLength", QString::number(Sequencer::cycleLength));
  rootEl.setAttribute("holdGenEnabled", QString::number(Sequencer::getHoldEnabled()));

  if (signalsCompilerMode) {
    rootEl.setAttribute("mode", "signalCompiler");
    QDomElement cycleParametersEl = doc.createElement("cycleParameters");
    cycleParametersEl.setAttribute("resetLength", QString::number(resetLength));
    cycleParametersEl.setAttribute("resetIntegOffset", QString::number(resetIntegOffset));
    cycleParametersEl.setAttribute("resetHoldLength", QString::number(resetHoldLength));
    cycleParametersEl.setAttribute("integrationLength", QString::number(integrationLength));
    cycleParametersEl.setAttribute("flattopLength", QString::number(flattopLength));
    cycleParametersEl.setAttribute("flattopHoldLength", QString::number(flattopHoldLength));
    cycleParametersEl.setAttribute("rampLength", QString::number(rampLength));
    rootEl.appendChild(cycleParametersEl);
  }
  for (unsigned int t=0; t<tracks.size(); ++t) {
    QDomElement trackEl = doc.createElement("SequencerTrack");
    SequencerTrack& track = *tracks[t];
    //trackEl.setAttribute("statVal", QString::number(track.getStatVal()));
    trackEl.setAttribute("signalName", c_trackNames[t]);
    trackEl.setAttribute("statVal", QString::number((int)track.getStatVal()));
    trackEl.setAttribute("invHold", QString::number((int)track.getInvHold()));
    for (unsigned int i=0; i<track.phases.size(); ++i) {
      QDomElement phaseEl = doc.createElement("Phase");
      SequencerTrack::SequencePhase& phase = track.phases[i];
      phaseEl.setAttribute("type", QString::number((int)phase.high));
      phaseEl.setAttribute("length", QString::number(phase.clockCycles));
      trackEl.appendChild(phaseEl);
    }
    rootEl.appendChild(trackEl);
  }

  QDomElement holdGenEl = doc.createElement("HoldGenerator");
  holdGenEl.setAttribute("enabled", QString::number(holdEnabled));
  rootEl.appendChild(holdGenEl);
  for (unsigned int i=0; i<holdCnts.size(); ++i) {
    QDomElement holdCntsEl = doc.createElement("HoldCntsRegEntry");
    HoldCntsRegisterEntry& h = holdCnts[i];
    holdCntsEl.setAttribute("hold", QString::number(h.hold));
    holdCntsEl.setAttribute("length", QString::number(h.length));
    holdGenEl.appendChild(holdCntsEl);
  }

  QFile file(_filename);
  if (!file.open( QIODevice::WriteOnly )) {
    SuS_LOG_STREAM(error, log_id(), "Saving to file failed");
    return false;
  }

  QTextStream out(&file);
  doc.save(out, 0);
  file.close();
  setFileName(_filename);
  return true;
}


bool Sequencer::loadFile(std::string _filename)
{
  QDomDocument doc;

  QFile file(_filename);
  if (!file.open( QIODevice::ReadOnly ) ) {
    SuS_LOG_STREAM(error, log_id(), "Error opening file.");
    return false;
  }
  if (!doc.setContent(&file)) {
    SuS_LOG_STREAM(error, log_id(), "Failed to parse XML file");
  }
  setFileName(_filename);

  QDomElement rootEl = doc.documentElement();
  setCycleLength(rootEl.attribute("cycleLength").toInt());
  setHoldEnabled(rootEl.attribute("holdGenEnabled").toInt());

  bool retVal = true;

  QDomElement holdGenEl = doc.nextSiblingElement("HoldGenerator");
  QDomNodeList holdCntsNodeList = rootEl.elementsByTagName("HoldCntsRegEntry");
  holdCnts.clear();
  for (int i=0; i<holdCntsNodeList.size(); ++i) {
    QDomElement holdCntEl = holdCntsNodeList.at(i).toElement();
    uint16_t holdLength =  holdCntEl.attribute("length").toInt();
    holdCnts.push_back(HoldCntsRegisterEntry(holdLength, holdCntEl.attribute("hold").toInt()));
  }
  if (!checkHoldCnts(holdCnts)) {
    retVal = false;
  }

  if (rootEl.hasAttribute("mode")) {          // USE SIGNAL COMPILER
    signalsCompilerMode = true;
    QDomElement cycleParametersEl = rootEl.firstChildElement("cycleParameters");
    resetLength       = cycleParametersEl.attribute("resetLength").toInt();
    resetIntegOffset  = cycleParametersEl.attribute("resetIntegOffset").toInt();
    resetHoldLength   = cycleParametersEl.attribute("resetHoldLength").toInt();
    integrationLength = cycleParametersEl.attribute("integrationLength").toInt();
    flattopLength     = cycleParametersEl.attribute("flattopLength").toInt();
    flattopHoldLength = cycleParametersEl.attribute("flattopHoldLength").toInt();
    rampLength        = cycleParametersEl.attribute("rampLength").toInt();
    //rampIntegOffset   = cycleParametersEl.attribute("rampIntegOffset").toInt();
    generateSignals();
  } else {                                    // USE TRACK COMPILER (PHASES)
    signalsCompilerMode = false;
    QDomNodeList trackNodeList = rootEl.elementsByTagName("SequencerTrack");
    for (int i=0; i<trackNodeList.size(); ++i) {
      QDomElement trackEl = trackNodeList.at(i).toElement();
      QDomNodeList phaseNodeList = trackEl.elementsByTagName("Phase");
      int x = trackNameToNum(trackEl.attribute("signalName"));
      TrackNum n;
      if (x>-1) {
        n = (TrackNum)x;
      }else{
        break;
      }
      SequencerTrack& t = *tracks[n];
      t.setStatVal(trackEl.attribute("statVal").toInt());
      t.setInvHold(trackEl.attribute("invHold").toInt());
      t.clearPhases();
      for (int j=0; j<phaseNodeList.size(); ++j) {
        QDomElement phaseEl = phaseNodeList.at(j).toElement();
        bool type = (bool)phaseEl.attribute("type").toInt();
        int length = phaseEl.attribute("length").toInt();
        t.addPhase(type, length);
      }
    }

    //SuS_LOG_STREAM(info, log_id(), "New holdCnts set.");

    // compilation of the tracks needs to be done after the holdCnts are set because
    // the hold positions need to be known for compilation because of a bug in the chip...
    //

//  for (int i=0; i<c_numOfTracks; ++i) {
//    SequencerTrack& t = *tracks[i];
//    if (!t.compileAndCheck()) {
//      retVal = false;
//      SuS_LOG_STREAM(error, log_id(), "Error while loading bits for track " << c_trackNames[i]);
//    }
//  }


    compileAllTracks(); // removed because it calls a virtual function
  }

  setOpMode(getOpMode());

  file.close();
  emit contentChanged();
  emit cycleLengthChanged(cycleLength);

  configGood = retVal;
  return retVal;
}
*/


void Sequencer::setCycleLength(int _cycleLength)
{
  {
    utils::CoutColorKeeper keeper(utils::STDGRAY);
    SuS_LOG_STREAM(info,log_id(),"Setting cycleLength to " << _cycleLength);
  }
  cycleLength = _cycleLength;
  setTracksNotCompiled();
}


int Sequencer::trackNameToNum(std::string name)
{
  for (int i=0; i<c_numOfTracks; ++i) {
    if (name == getTrack(i)->name) return i;
  }
  SuS_LOG_STREAM(error, log_id(), "Trackname unknown. Check Sequencer File!");
  return -1;
}


void Sequencer::setFileName(std::string _filename)
{
  filename = _filename;
}

int Sequencer::getHoldCnts()
{
  if (!checkHoldCnts(holdCnts)) {
    return -1;
  }

  int holdCntsLength = 0;
  for (unsigned int i=0; i<holdCnts.size(); ++i) {
    if (holdCnts[i].hold) {
      holdCntsLength += holdCnts[i].length;
    }
  }
  return holdCntsLength;
}


int Sequencer::getRealCycleLength()
{
  int realCycleLength = cycleLength;
  if (getHoldEnabled()){
    realCycleLength += getHoldCnts();
  }

#ifdef DEBUG
  SuS_LOG_STREAM(debug, log_id(), "Real Cycle Length = " << realCycleLength);
#endif

  return realCycleLength;
}


bool Sequencer::setHoldCnts(HoldCntsRegister _holdCnts)
{
  if(!checkHoldCnts(_holdCnts)) {
    return false;
  }
  //reorderPartSeqsForHold();
  holdCnts = _holdCnts;

  return true;
}


void Sequencer::setExtLatchMode()
{
  setOpMode(EXTLATCH);

  compileAndCheckAllTracks();
}


void Sequencer::setFilterInBufferMode()
{
  setOpMode(BUFFER);

  compileAndCheckAllTracks();
}


bool Sequencer::correctNumberOfHolds(HoldCntsRegister &_holdCnts)
{
  int smallerCnt = 0;
  while((int)_holdCnts.size() < c_holdCntsRegDepth && smallerCnt<=c_holdCntsRegDepth)
  {
    int idx = 0;
    for(auto & holdCnt : _holdCnts){
      if(holdCnt.length>4){
        holdCnt.length -= 2;
        _holdCnts.insert(_holdCnts.begin()+idx,HoldCntsRegisterEntry(2,holdCnt.hold));
        break;
      }
      idx++;
    }
    smallerCnt++;
  }

  uint32_t idx = 0;
  while((int)_holdCnts.size() > c_holdCntsRegDepth && idx < (_holdCnts.size()-1))
  {
    if(_holdCnts[idx].hold == _holdCnts[idx+1].hold){
      _holdCnts[idx].length += _holdCnts[idx+1].length;
      _holdCnts.erase(_holdCnts.begin()+idx+1);
    }
    idx++;
  }


  return (int)_holdCnts.size() == c_holdCntsRegDepth;
}


bool Sequencer::checkHoldCnts(HoldCntsRegister &_holdCnts)
{
  if ((int)_holdCnts.size() != c_holdCntsRegDepth) {
    if(!correctNumberOfHolds(_holdCnts)){
      return false;
    }
  }
  holdCntsInSyncWithCycleLength(_holdCnts); // just check...
  return true;
}


bool Sequencer::holdCntsInSyncWithCycleLength(const HoldCntsRegister & _holdCnts)
{
  int holdCntsCycleLength = 0;
  for (unsigned int i=0; i<_holdCnts.size(); ++i) {
    if (!_holdCnts[i].hold) {
      holdCntsCycleLength += _holdCnts[i].length;
    }
  }
  if ( (holdCntsCycleLength % cycleLength) != 0) {
    SuS_LOG_STREAM(warning, log_id(), "holdCnts not in sync with cycleLength");
    SuS_LOG_STREAM(warning, log_id(), "holdCntsCycleLength = " << holdCntsCycleLength);
    SuS_LOG_STREAM(warning, log_id(), "cycleLength = " << cycleLength);
    return false;
  }

  if(debugMode)
  {
    SuS_LOG_STREAM(info, log_id(), "holdCntsCyleLength / sequenceCycleLength is "
                                    << holdCntsCycleLength / cycleLength);
  }

  int lastNoHoldPhaseLength = 0;
  for (unsigned int i=_holdCnts.size(); i>0; i--) {
    if (!_holdCnts[i].hold) {
      lastNoHoldPhaseLength += _holdCnts[i-1].length;
    }else{
      break;
    }
  }

  if(lastNoHoldPhaseLength < 3){
    SuS_LOG_STREAM(error, log_id(),"Last Hold 0 Phase is too short. Must be larger than 2!");
  }

  return true;
}


void Sequencer::setInjectSlot(int slot)
{
  int totalCycleLength = cycleLength * 7;
  int lowInjLength = getFirstFlipPhase() + slot;

  setFlipPhases(lowInjLength,totalCycleLength-lowInjLength);
}


int Sequencer::getInjectSlot()
{
  int flipLowLength = getFirstFlipPhase();
  int injLowLength  = getFirstInjectLength();

  return injLowLength - flipLowLength;
}


void Sequencer::setExtLatchSlot(int slot)
{
  if(mode != OpMode::EXTLATCH){
    std::cout << "Sequencer: setExtLatchSlot error: not in EXTLATCH mode" << std::endl;
    return;
  }

  int totalCycleLength = cycleLength * 7;
  int lowFlipLength = getRmpPos() + slot;

  setFlipPhases(lowFlipLength,totalCycleLength-lowFlipLength);
}


int Sequencer::getExtLatchSlot()
{
  if(mode != OpMode::EXTLATCH){
    std::cout << "Sequencer: setExtLatchSlot error: not in EXTLATCH mode" << std::endl;
    return 0;
  }

  int rmpPos = getRmpPos();
  int lowFlipLength = getFirstFlipPhase();

  if(cycleLength*7 == lowFlipLength) {
    return 0;
  }

  return lowFlipLength - rmpPos;
}


void Sequencer::setIntTimeExtension(bool extend)
{
  //depFetMode = !extend;
  enableIntTimeExtension = extend;
}


void Sequencer::setOpMode(OpMode _mode, bool generate)
{
  mode = _mode;
  switch(mode) {
    case (int)NORM :
      rampIntegOffset = 20;
      if(resetLength == cycleLength*7){
        resetLength = 14;
      }
      SuS_LOG_STREAM(info, log_id(), "Normal mode enabled.");
      break;
    case (int)SINGLEINT :
      rampIntegOffset = 20;
      if(resetLength == cycleLength*7){
        resetLength = 14;
      }
      SuS_LOG_STREAM(info, log_id(), "Single integration mode enabled.");
      break;
    case (int)RESET :
      rampIntegOffset = cycleLength*7-(getFlattopLength()+2*getIntegrationTime());
      SuS_LOG_STREAM(info, log_id(), "Reset mode enabled.");
      break;
    case (int)BUFFER :
      SuS_LOG_STREAM(info, log_id(), "Buffer mode enabled.");
      break;
    case (int)EXTLATCH:
      setRMPPos(cycleLength);
      setExtLatchSlot(10);
      SuS_LOG_STREAM(info, log_id(), "ExtLatch mode enabled.");
      break;
    case (int)MANUAL :
      SuS_LOG_STREAM(info, log_id(), "Manual mode enabled.");
      break;
    case (int)DEPFET :
      SuS_LOG_STREAM(info, log_id(), "DEPFET mode enabled.");
      break;
  }

  if(signalsCompilerMode){
    generateSignals();
  }else{
    adaptTracks();
  }
}

//void Sequencer::setDEPFETMode(bool mode)
//{
//  mode==DEPFET;
//  enableIntTimeExtension = !mode;
//}
//
//void Sequencer::setManualMode(bool mode)
//{
//  depFetMode = !mode;
//  enableIntTimeExtension = !mode;
//}

//void Sequencer::disableFrontendSignals()
//{
//  int cycleLengthFastClk = cycleLength*7;
//
//  SequencerTrack::SequencePhases swInSeqPhases;
//  swInSeqPhases.push_back(SequencerTrack::SequencePhase(0,cycleLengthFastClk));
//  if(!getTrack(Sequencer::FCF_SwIn)->setPhasesAndCompile(swInSeqPhases)) {
//    SuS_LOG_STREAM(error, log_id(), "Error while updating sequencer swIn.");
//  }
//
//  if( mode==RESET || mode==BUFFER || mode==EXTLATCH){
//    SequencerTrack::SequencePhases resnSeqPhases;
//    resnSeqPhases.push_back(SequencerTrack::SequencePhase(0,cycleLengthFastClk));
//    if(!getTrack(Sequencer::FCF_Res_B)->setPhasesAndCompile(resnSeqPhases)) {
//      SuS_LOG_STREAM(error, log_id(), "Error while updating sequencer resn.");
//    }
//  }
//
//  if(!extLatchMode){
//    SequencerTrack::SequencePhases flipSeqPhases;
//    flipSeqPhases.push_back(SequencerTrack::SequencePhase(1,cycleLengthFastClk));
//    if(!getTrack(Sequencer::FCF_Flip_Inject)->setPhasesAndCompile(flipSeqPhases)) {
//      SuS_LOG_STREAM(error, log_id(), "Error while updating sequencer flipInj.");
//    }
//  }
//}

// Inserts Holds one hold phase before first Integration and one hold phase after Flip
void Sequencer::setDEPFETHolds(int resetOffset, int flipHighLength)
{
  int firstPos=0,secondPos=0,firstLength=0,secondLength=0;
  findHoldPositions(firstPos,firstLength,secondPos,secondLength);
  if(resetOffset<14){
    SuS_LOG_STREAM(error, log_id(), "First Integration offset too small. Minimum 21");
    return;
  }
  int firstPosVal = (resetOffset-1)/7-1;
  int secondPosVal = (flipHighLength+1)/7+2;

  if(debugMode){
    SuS_LOG_STREAM(info, log_id(), "first Pos " << firstPos << " firstPosVal " << firstPosVal << " FirstLength " << firstLength);
  }
  if(firstPos != firstPosVal){
    firstPos    = firstPosVal;
    firstLength = 150;
  }

  if(debugMode){
    SuS_LOG_STREAM(info, log_id(), "Second Pos " << secondPos << " SecondPosVal " << secondPosVal << " SecondLength " << secondLength);
  }
  if(secondPos != secondPosVal ){
    secondPos    = secondPosVal;
    secondLength = 3000;
  }

  setHoldPositions(firstPos,firstLength,secondPos,secondLength);
}


void Sequencer::setZeroHolds()
{
  int remCycleLength = cycleLength;
  int meanCycleLength = remCycleLength/c_holdCntsRegDepth;

  holdCnts.clear();
  for (int i=0; i<(c_holdCntsRegDepth-1); ++i) {
    holdCnts.push_back(HoldCntsRegisterEntry(meanCycleLength, false));
    remCycleLength -= meanCycleLength;
  }
  holdCnts.push_back(HoldCntsRegisterEntry(remCycleLength, false));

  setTracksNotCompiled();

}

int Sequencer::getFirstInjectLength()
{
  int firstInjPhase = cycleLength*7;
  const SequencerTrack::SequencePhases& injTrackPhases = tracks.at(ISubPulse)->getPhases();
  if(injTrackPhases.size()>1){
    firstInjPhase = injTrackPhases[0].clockCycles;
  }

  return firstInjPhase;
}


int Sequencer::getFirstFlipPhase()
{
  int firstFlipPhase = cycleLength*7;
  const SequencerTrack::SequencePhases& flipTrackPhases = tracks.at(FCF_Flip)->getPhases();
  if(flipTrackPhases.size()>1){
    firstFlipPhase = flipTrackPhases[0].clockCycles;
  }

  return firstFlipPhase;
}


int Sequencer::getFlipHighLength()
{
  const SequencerTrack::SequencePhases& flipTrackPhases = tracks.at(FCF_Flip)->getPhases();

  int flipHighLength = 0;
  for(const auto & phase : flipTrackPhases){
    if(phase.high){
      flipHighLength += phase.clockCycles;
    }
  }
  return flipHighLength;
}


int Sequencer::getFirstIntegOffset()
{
  //SwIn Investigation
  int resTime,resetOffset;
  getResetTimeAndOffset(resTime,resetOffset);
  int firstIntegOffset = resTime + resetOffset + 20;

  const SequencerTrack::SequencePhases& swInTrackPhases = tracks.at(Sequencer::FCF_SwIn)->getPhases();
  if(swInTrackPhases.size()>1){
    firstIntegOffset = swInTrackPhases[0].clockCycles;
  }

  return firstIntegOffset;

}

int Sequencer::getResetIntegOffset()
{
  if (mode==MANUAL) {
    int resTime,resetOffset;
    getResetTimeAndOffset(resTime,resetOffset);

    int firstIntegOffset = getFirstIntegOffset();

    return firstIntegOffset-resTime-resetOffset;
  } else {
    return resetIntegOffset;
  }
}


//Flattop is given by phase length between first and second integration.
// In SinlgeIntegrationMode flattop is zero if oneFlipPhase,
// or distance batween reset and firstIntegration if flip has two phases
int Sequencer::getFlattopLength()
{
  if (mode==MANUAL) {
    //SwIn Investigation
    int flattop = 35;

    if(mode==SINGLEINT){
      if(hasOneFlipPhase()){
        flattop = 0;
      }else{
        int resTime,resetOffset;
        getResetTimeAndOffset(resTime,resetOffset);
        int firstIntegOffset = getFirstIntegOffset();

        flattop = firstIntegOffset - resetOffset - resTime;
      }
    }else{
      const SequencerTrack::SequencePhases& swInTrackPhases = tracks.at(Sequencer::FCF_SwIn)->getPhases();
      if (swInTrackPhases.size()>=4) {
        flattop = swInTrackPhases[2].clockCycles;
      }
    }
    return flattop;
  } else {
    return flattopLength;
  }
}


int Sequencer::getFlattopHoldLength()
{
  if (mode==MANUAL) {
    //SwIn Investigation
    int flattop = getFlattopLength();

    if(flattop == 0){
      return 0;
    }

    //findFlatTop Plus Hold Length
    int firstHoldLength = 0;
    int secondHoldLength = 0;
    int firstHoldPos = 0;
    int secondHoldPos = 0;

    findHoldPositions(firstHoldPos,firstHoldLength,secondHoldPos,secondHoldLength);

    int firstIntegOffset = getFirstIntegOffset();
    int integrationLenght = getIntegrationTime(true);

    int flattopPos = firstIntegOffset + integrationLenght;

    if(firstHoldPos>flattopPos && firstHoldPos < flattopPos+flattop){
      return firstHoldLength;
    }

    if(secondHoldPos>flattopPos && secondHoldPos < flattopPos+flattop){
      return secondHoldLength;
    }

    return 0;
  } else {
    return flattopHoldLength;
  }
}


int Sequencer::getRampIntegOffset()
{
  int cycleLengthFastClk = cycleLength*7;
  int firstIntegOffset = getFirstIntegOffset();
  int time = getIntegrationTime();
  int flattop = getFlattopLength();

  int rmpPos = getRmpPos();
  int lastSwInPhaseLength;
  if(mode==SINGLEINT){
    lastSwInPhaseLength = firstIntegOffset+time;
  }else{
    lastSwInPhaseLength = firstIntegOffset+flattop+2*time;
  }

  int rampOffset = rmpPos-lastSwInPhaseLength;
  if(rampOffset<0){
    rampOffset += cycleLengthFastClk;
  }

  return rampOffset;
}

int Sequencer::getRmpPos()
{
  SequencerTrack& track = *getTrack(Sequencer::ADC_RMP);
  const SequencerTrack::SequencePhases& seqPhases = track.getPhases();

  if(seqPhases.size() > 4){
    int cnt = 0;
    for(int i=0; i<4; i++){
      cnt += seqPhases[i].clockCycles;
    }
    return cnt;
  }if(seqPhases.size()==3){
    if(seqPhases.at(0).high){
      return seqPhases.at(0).clockCycles + seqPhases.at(1).clockCycles;
    }else{
      return seqPhases.at(0).clockCycles;
    }
  }else if(seqPhases.size()==2){
    return seqPhases.at(0).clockCycles;
  }else{
    return 0;
  }
}


int Sequencer::getRmpEnd()
{
  int cycleLenghtFastClk = cycleLength * 7;

  int rmpPos    = getRmpPos();
  int rmpLength = getRampLength();

  int rmpEnd = (rmpPos + rmpLength) % cycleLenghtFastClk;

  return rmpEnd;
}

int Sequencer::getRampLength()
{
  if (mode==MANUAL) {
    SequencerTrack& track = *getTrack(Sequencer::ADC_RMP);
    const SequencerTrack::SequencePhases& seqPhases = track.getPhases();

    std::vector<int> highPhases;
    for(uint i=0; i< seqPhases.size(); i++){
      if(seqPhases.at(i).high){
        highPhases.push_back(i);
      }
    }

    int numHighPhases = highPhases.size();

    int rmpLength = 0;
    for(int i=0; i<numHighPhases; i++){
      rmpLength += seqPhases.at(highPhases.at(i)).clockCycles;
    }

    return rmpLength;
  } else {
    return rampLength;
  }
}

int Sequencer::getResetLength()
{
  if (mode==MANUAL) {
    SequencerTrack& track = *getTrack(Sequencer::FCF_Res_B);
    const SequencerTrack::SequencePhases& seqPhases = track.getPhases();

    std::vector<int> lowPhases;
    for(uint i=0; i< seqPhases.size(); i++){
      if(!seqPhases.at(i).high){
        lowPhases.push_back(i);
      }
    }

    int numLowPhases = lowPhases.size();
    if(numLowPhases>1){
      SuS_LOG_STREAM(warning, log_id(), "More than one reset phase found");
    }

    if(numLowPhases == 0){
      return 0;
    }
    return seqPhases.at(lowPhases.front()).clockCycles;
  } else {
    return resetLength;
  }
}


int Sequencer::getResetHoldLength()
{
  if (mode==MANUAL) {
    int resTime;
    int resetOffset;
    getResetTimeAndOffset(resTime,resetOffset);

    if(resTime == 0){
      return 0;
    }

    int firstHoldLength = 0;
    int secondHoldLength = 0;
    int firstHoldPos = 0;
    int secondHoldPos = 0;

    findHoldPositions(firstHoldPos,firstHoldLength,secondHoldPos,secondHoldLength);

    if(firstHoldPos>resetOffset && firstHoldPos < resetOffset+resTime){
      return firstHoldLength;
    }

    if(secondHoldPos>resetOffset && secondHoldPos < resetOffset+resTime){
      return secondHoldLength;
    }
  }
  //else
  return resetHoldLength;
}


int Sequencer::getIntegrationTime(bool noHold)
{
  if (mode==MANUAL) {
    SequencerTrack& track = *getTrack(Sequencer::FCF_SwIn);
    const SequencerTrack::SequencePhases& seqPhases = track.getPhases();

    std::vector<int> highPhases;
    for(uint i=0; i< seqPhases.size(); i++){
      if(seqPhases.at(i).high){
        highPhases.push_back(i);
      }
    }

    int numHighPhases = highPhases.size();

    int integrationTime=0;
    for(int i=0; i<numHighPhases; i++){
      integrationTime+=seqPhases.at(highPhases.at(i)).clockCycles;
    }
    if(debugMode){
      SuS_LOG_STREAM(warning, log_id(), "GetIntegrationTime: NumHighPhases: " << numHighPhases << " IntegrationTime: " << integrationTime);
    }
    integrationTime = (numHighPhases>0)? integrationTime/numHighPhases : 0;

    if(!noHold && holdEnabled && enableIntTimeExtension){
      integrationTime += getHoldIntegrationTime();
    }
    //if(!noHold){
    //  integrationLength = integrationTime;
    //}
    return integrationTime;
  } else {
    return integrationLength;
  }
}

int Sequencer::getHoldIntegrationTime()
{
  if(holdEnabled){

    int firstHoldLength = 0;
    int secondHoldLength = 0;
    int firstHoldPos = 0;
    int secondHoldPos = 0;

    findHoldPositions(firstHoldPos,firstHoldLength,secondHoldPos,secondHoldLength);
//     SuS_LOG_STREAM(info, log_id(), "First Hold: " << firstHoldPos);
//     SuS_LOG_STREAM(info, log_id(), "First Length: " << firstHoldLength);
//     SuS_LOG_STREAM(info, log_id(), "Second Hold: " << secondHoldPos);
//     SuS_LOG_STREAM(info, log_id(), "Second Length: " << secondHoldLength);

    if((firstHoldLength>0) && ( mode==SINGLEINT || (firstHoldLength == secondHoldLength) ) ){

      int firstIntegrationLength = 0;
      int secondIntegrationLength = 0;
      int firstIntegrationPos = 0;
      int secondIntegrationPos = 0;

      findIntegrationPositions(firstIntegrationPos,firstIntegrationLength,secondIntegrationPos,secondIntegrationLength);
      if(secondIntegrationPos>0){
//       SuS_LOG_STREAM(info, log_id(), "First Integration: " << firstIntegrationPos);
//       SuS_LOG_STREAM(info, log_id(), "First Length: " << firstIntegrationLength);
//       SuS_LOG_STREAM(info, log_id(), "Second Integration: " << secondIntegrationPos);
//       SuS_LOG_STREAM(info, log_id(), "Second Length: " << secondIntegrationLength);
        return (firstHoldLength * 7);
      }
    }
  }
  return 0;

}

void Sequencer::findHoldPositions(int &firstPos, int& firstLength, int& secondPos, int& secondLength)
{
  int numPhases = holdCnts.size();
  int phase = 0;
  firstLength  = 0;
  secondLength = 0;
  firstPos     = 0;
  secondPos    = 0;

  if(holdEnabled == 0){
    firstPos     = cycleLength+1;
    firstLength  = 0;
    secondPos    = cycleLength+1;
    secondLength = 0;
    return;
  }

  while(phase < numPhases)
  {
    if(!holdCnts.at(phase).hold){
      firstPos += holdCnts.at(phase).length;
    }else{
      secondPos += firstPos;
      break;
    }
    phase++;
  }

  while(phase < numPhases)
  {
    if(holdCnts.at(phase).hold){
      firstLength += holdCnts.at(phase).length;
    }else
    {
      break;
    }
    secondPos++;
    phase++;
  }

  while(phase < numPhases)
  {
    if(!holdCnts.at(phase).hold){
      secondPos += holdCnts.at(phase).length;
    }else{
      break;
    }
    phase++;
  }

  while(phase < numPhases)
  {
    if(holdCnts.at(phase).hold){
      secondLength += holdCnts.at(phase).length;
    }else
    {
      break;
    }
    phase++;
  }

  if(secondPos==0){
    secondPos = cycleLength+1;
    secondLength = 0;
  }

}

void Sequencer::findIntegrationPositions(int &firstPos, int& firstLength, int& secondPos, int& secondLength)
{
  SequencerTrack& track = *getTrack(Sequencer::FCF_SwIn);
  const SequencerTrack::SequencePhases& seqPhases = track.getPhases();
  int numPhases = seqPhases.size();
  int phase = 0;
  firstLength  = 0;
  secondLength = 0;
  firstPos     = 0;
  secondPos    = 0;

  while(phase < numPhases)
  {
    if(!seqPhases.at(phase).high){
      firstPos += seqPhases.at(phase).clockCycles;
    }else{
      secondPos += firstPos;
      break;
    }
    phase++;
  }

  while(phase < numPhases)
  {
    if(seqPhases.at(phase).high){
      firstLength += seqPhases.at(phase).clockCycles;
    }else
    {
      break;
    }
    secondPos += seqPhases.at(phase).clockCycles;
    phase++;
  }

  while(phase < numPhases)
  {
    if(!seqPhases.at(phase).high){
      secondPos += seqPhases.at(phase).clockCycles;
    }else{
      break;
    }
    phase++;
  }

  while(phase < numPhases)
  {
    if(seqPhases.at(phase).high){
      secondLength += seqPhases.at(phase).clockCycles;
    }else
    {
      break;
    }
    phase++;
  }
}


int Sequencer::getMaxRampIntOffset()
{
  int cycleLengthFastClk = cycleLength*7;

  return cycleLengthFastClk - rampLength - 1;
}

int Sequencer::getMaxFlattopLength(bool inclHold)
{
  int cycleLengthFastClk = getCycleLength()*7;
  return cycleLengthFastClk - 2*integrationLength - resetLength - resetIntegOffset - rampIntegOffset;
}

int Sequencer::getMaxResetIntegOffset()
{
  int firstIntegOffset = getFirstIntegOffset();
  int resLength = getResetLength();

  return firstIntegOffset - resLength - 2;
}


void Sequencer::disableRes_N()
{
  int cycleLengthFastClk = cycleLength*7;
  //Set SwIn
  SequencerTrack::SequencePhases resSeqPhases;
  resSeqPhases.push_back(SequencerTrack::SequencePhase(0,cycleLengthFastClk));
  if(!getTrack(Sequencer::FCF_Res_B)->setPhasesAndCompile(resSeqPhases)) {
    SuS_LOG_STREAM(error, log_id(), "Error while updating sequencer Res_B.");
  }
}


void Sequencer::disableSwIn()
{
  int cycleLengthFastClk = cycleLength*7;
  //Set SwIn
  SequencerTrack::SequencePhases swInSeqPhases;
  swInSeqPhases.push_back(SequencerTrack::SequencePhase(0,cycleLengthFastClk));
  if(!getTrack(Sequencer::FCF_SwIn)->setPhasesAndCompile(swInSeqPhases)) {
    SuS_LOG_STREAM(error, log_id(), "Error while updating sequencer swIn.");
  }
}


void Sequencer::disableFlip()
{
  int cycleLengthFastClk = cycleLength*7;
  //Set FlipInject
  SequencerTrack::SequencePhases flipSeqPhases;
  flipSeqPhases.push_back(SequencerTrack::SequencePhase(1,cycleLengthFastClk));

  if(!getTrack(Sequencer::FCF_Flip)->setPhasesAndCompile(flipSeqPhases)) {
    SuS_LOG_STREAM(error, log_id(), "Error while updating sequencer flipInj.");
  }

  if(!getTrack(Sequencer::ISubPulse)->setPhasesAndCompile(flipSeqPhases)) {
    SuS_LOG_STREAM(error, log_id(), "Error while updating sequencer flipInj.");
  }
}


int Sequencer::getMaxResetWait()
{
  int time = getIntegrationTime(true);
  int flattop = getFlattopLength();

  int resTime;
  int resOffset;

  getResetTimeAndOffset(resTime,resOffset);

  int maxResetWait = 0;

  int cycleLengthFastClk = cycleLength*7;

  maxResetWait = cycleLengthFastClk - 1 - 14 - resTime - resOffset - flattop - 2 * time;

  if(maxResetWait<=0){
    SuS_LOG_STREAM(error, log_id(), "Max reset length < 0. Try shorter resetOffset or integrationTime");
  }

  return maxResetWait;

}

bool Sequencer::getResetTimeAndOffset(int &resTime, int &resOffset)
{
 //Reset Investigation
  resTime = 14;
  resOffset = 35;
  if(mode==BUFFER || mode==EXTLATCH){
    resTime = cycleLength*7;
    resOffset = 0;
    return true;
  }

  const SequencerTrack::SequencePhases& resTrackPhases = tracks.at(Sequencer::FCF_Res_B)->getPhases();
  if (resTrackPhases.size()==2){
    if(resTrackPhases[0].high){
      resOffset = resTrackPhases[0].clockCycles;
      resTime   = resTrackPhases[1].clockCycles;
    }else{
      resOffset = 0;
      resTime   = resTrackPhases[0].clockCycles;
    }
  } else if (resTrackPhases.size()==3){
    if(resTrackPhases[0].high){
      resOffset = resTrackPhases[0].clockCycles;
      resTime   = resTrackPhases[1].clockCycles;
    }else{
      resOffset = resTrackPhases[0].clockCycles + resTrackPhases[1].clockCycles;
      resTime   = resTrackPhases[0].clockCycles + resTrackPhases[2].clockCycles;
    }
  }

  if(resTime<14){
    SuS_LOG_STREAM(warning, log_id(), "Reset time really short, should not be smaller than 14 fast cycles");
  }
  return true;
}

std::string Sequencer::getOpModeStr(Sequencer::OpMode opMode)
{
  switch(opMode){
    case NORM     : return "NORM";
    case SINGLEINT: return "SINGLEINT";
    case BUFFER   : return "BUFFER";
    case RESET    : return "RESET";
    case MANUAL   : return "MANUAL";
    case EXTLATCH : return "EXTLATCH";
    case DEPFET   : return "DEPFET";
    default: return "UNKNOWN";
  }
}


Sequencer::OpMode Sequencer::getOpMode()
{
  if(mode == Sequencer::MANUAL) return mode;

  if (signalsCompilerMode) {
    return mode;
  } else {
    int numFlipPhases = getTrack(Sequencer::FCF_Flip)->getPhases().size();
    int numResPhases  = getTrack(Sequencer::FCF_Res_B)->getPhases().size();
    int numSwInPhases = getTrack(Sequencer::FCF_SwIn)->getPhases().size();
    int numRmpPhases  = getTrack(Sequencer::ADC_RMP)->getPhases().size();

    OpMode investigatedMode = Sequencer::MANUAL;
    if(numFlipPhases == 3){
      investigatedMode = Sequencer::MANUAL;
    }else if(numRmpPhases==1){
      investigatedMode = Sequencer::MANUAL;
    }else if(numResPhases==1){
      if(numFlipPhases == 2){
        investigatedMode = Sequencer::EXTLATCH;
      }else if(numFlipPhases == 1 && numSwInPhases == 1 ){
        investigatedMode = Sequencer::BUFFER;
      }else{
        investigatedMode = Sequencer::MANUAL;
      }
    }else if(numFlipPhases==1){
      if(numSwInPhases==1){
        investigatedMode = Sequencer::RESET;
      }else if(numSwInPhases<=3){
        investigatedMode = Sequencer::SINGLEINT;
      }else{
        investigatedMode = Sequencer::MANUAL;
      }
    }else{
      if(numSwInPhases<=3){
        investigatedMode = Sequencer::SINGLEINT;
      }else{
        investigatedMode = Sequencer::NORM;
      }
    }
    return investigatedMode;
  }
}


int Sequencer::getMaxIntegrationTime()
{
  const SequencerTrack::SequencePhases& resPhases  = getTrack(Sequencer::FCF_Res_B)->getPhases();

  int maxIntegrationTime = (resPhases[0].clockCycles-5)/2 + integrationLength-5;

  if(debugMode){
    SuS_LOG_STREAM(info, log_id(), "Max integration time is " << maxIntegrationTime);
  }

  return maxIntegrationTime;
}


void  Sequencer::setSw1Edge()
{
  setTracksNotCompiled();

  const SequencerTrack::SequencePhases& swInPhases = getTrack(Sequencer::FCF_SwIn)->getPhases();
  const SequencerTrack::SequencePhases& resPhases  = getTrack(Sequencer::FCF_Res_B)->getPhases();

  int numPhases = swInPhases.size();
  //single integration mode? -> return
  int swInHighPhases = 0;
  for(int i=0; i<numPhases; i++){
    if(swInPhases.at(i).high){
      swInHighPhases++;
    }
  }
  if(swInHighPhases==1)
    return;

  numPhases = resPhases.size();

  //find clock cycles until reset finished
  int resBackHighCycles = 0;
  for(int i=0; i<numPhases; i++){
    if(!resPhases.at(i).high){
      while(!resPhases.at(i).high){
        resBackHighCycles +=resPhases.at(i).clockCycles;
        i++;
      }
      break;
    }
    resBackHighCycles +=resPhases.at(i).clockCycles;
  }

  int missingCycles = (7-resBackHighCycles%7)%7;
  resBackHighCycles += missingCycles;

  assert(resBackHighCycles%7==0);

  //find clock cycles until first integration phase
  int highPhase = 0;
  while(!swInPhases.at(highPhase).high){
    highPhase++;
    if(highPhase==numPhases)
      break;
  }

  int sw1High = 0;
  for(int i=0; i<highPhase; i++)
  {
    sw1High += swInPhases.at(i).clockCycles;
  }

  sw1High -= resBackHighCycles;

  sw1High += getMaxIntegrationTime()+1;

  missingCycles = (7-sw1High%7)%7;
  sw1High += missingCycles;

  assert(sw1High%7==0);

  if(enableGui){
    SuS_LOG_STREAM(info, log_id(), "Cycles to reset finished: " << resBackHighCycles );
    SuS_LOG_STREAM(info, log_id(), "Cycles to first integration phase: " << sw1High);
  }

  SequencerTrack::SequencePhases newSw1Phases;
  newSw1Phases.push_back(SequencerTrack::SequencePhase(1,resBackHighCycles));
  newSw1Phases.push_back(SequencerTrack::SequencePhase(0,sw1High));
  int totalCycles = getTotalClockCycles(Sequencer::ADC_RMP);
  newSw1Phases.push_back(SequencerTrack::SequencePhase(1,totalCycles-sw1High-resBackHighCycles));
  SuS_LOG_STREAM(info, log_id(), "TotalCycles: " << totalCycles);
  int numHoldPhases = holdCnts.size();

  std::vector<int> holdHighPhaseIdxs;
  for(int i=0; i<numHoldPhases; i++){
    if(holdCnts.at(i).hold)
      holdHighPhaseIdxs.push_back(i);
  }

  if(holdHighPhaseIdxs.size()>2){
    SuS_LOG_STREAM(error, log_id(), "Only two hold high phases allowed!");
    return;
  }

  int numSlowCycles = 0;
  for(int i=0; i<holdHighPhaseIdxs.back();i++){
    if(!holdCnts.at(i).hold)
      numSlowCycles+=holdCnts.at(i).length;
  }

  int missingSlowCycles = sw1High/7 - numSlowCycles;
  int midLowPhaseIdx = holdHighPhaseIdxs.back() - 1;
  holdCnts.at(midLowPhaseIdx).length+=missingSlowCycles;

  if(missingSlowCycles!=0){
    SuS_LOG_STREAM(warning, log_id(), "Second hold phase position corrected: " + std::to_string(missingSlowCycles));
  }
  if(!holdCnts.back().hold){
    holdCnts.back().length-=missingSlowCycles;
  }else{
    SuS_LOG_STREAM(error, log_id(), "Last hold phase must be low");
    return;
  }

  SequencerTrack& sw1track = *getTrack(Sequencer::FCF_SwIn);
  if(!sw1track.setPhasesAndCompile(newSw1Phases)) {
    SuS_LOG_STREAM(error, log_id(), "Error while updating sw1 track in sequencer.");
    return;
  }

  compileAndCheckAllTracks();
}

int Sequencer::getTotalClockCycles(TrackNum trackNum)
{
  int totalClockCycles = 0;

  const SequencerTrack::SequencePhases& seqPhases = getTrack(trackNum)->getPhases();

  for(uint i=0; i< seqPhases.size(); i++){
    totalClockCycles += seqPhases.at(i).clockCycles;
  }

  return totalClockCycles;
}



void Sequencer::disableSecondHold()
{
  int firstPos=0,secondPos=0,firstLength=0,secondLength=0;
  findHoldPositions(firstPos,firstLength,secondPos,secondLength);
  setHoldPositions(firstPos,firstLength);
}


void Sequencer::setTracksNotCompiled()
{
  for (unsigned int i=0; i<tracks.size(); ++i) {
    tracks[i]->isCompiled = false;
  }
}


bool Sequencer::setHoldPositions(int firstPos, int firstLength, int secondPos, int secondLength)
{
  setTracksNotCompiled();

  bool singleHold = (secondPos==0);

  int remCycleLength      = cycleLength;
  int maxFirstHoldPos     = cycleLength-3;
  int maxHoldLengthPerReg = 1 << (c_holdCntWidth - 1);
  int totalMaxHoldLength  = (singleHold) ? 3*maxHoldLengthPerReg : maxHoldLengthPerReg;


  if(firstLength <= 1){
    SuS_LOG_STREAM(info, log_id(), "MM3 Error: First hold length must not be smaller than 2. Set to 2" );
    firstLength=2;
  }
  if(secondLength <= 1){
    SuS_LOG_STREAM(info, log_id(), "MM3 Error: Second hold length must not be smaller than 2. Set to 2" );
    secondLength=2;
  }

  if((firstLength+secondLength) > 1000 && (cycleLength%2)==1){
    SuS_LOG_STREAM(warning, log_id(), "Using long holds, set cycle length to an even number to not run out of sync");
  }


  if (firstPos>maxFirstHoldPos) {
    SuS_LOG_STREAM(error, log_id(), "setHoldPosition(): Hold position cannot be > " << maxFirstHoldPos);
    return false;
  }
  if (firstLength>totalMaxHoldLength) {
    SuS_LOG_STREAM(error, log_id(), "setHoldPosition(): Hold length cannot be > " << totalMaxHoldLength);
    return false;
  }

  if(enableGui && debugMode){
    SuS_LOG_STREAM(info, log_id(),"Setting hold @ " << firstPos << " with length " << firstLength);
    if (!singleHold)
    {
      SuS_LOG_STREAM(info, log_id(),"Setting second hold @ " << secondPos << " with length " << secondLength);
    }
  }

  holdCnts.clear();
  if (firstPos>0) {
    holdCnts.push_back(HoldCntsRegisterEntry(firstPos, false));
    remCycleLength -= firstPos;
  }

  for (int i=0; i<3; ++i) {
    if (firstLength <= maxHoldLengthPerReg) {
      holdCnts.push_back(HoldCntsRegisterEntry(firstLength, true));
      break;
    } else {
      holdCnts.push_back(HoldCntsRegisterEntry(maxHoldLengthPerReg, true));
      firstLength -= maxHoldLengthPerReg;
    }
  }

  if (singleHold) {
    int extraEntriesWithZeroLength = c_holdCntsRegDepth-holdCnts.size()-1;
    holdCnts.push_back(HoldCntsRegisterEntry(remCycleLength-extraEntriesWithZeroLength, false));
    for (int i=0; i<extraEntriesWithZeroLength; ++i) {
      holdCnts.push_back(HoldCntsRegisterEntry(1, false));
    }
  } else {
    holdCnts.push_back(HoldCntsRegisterEntry(secondPos-firstPos-1, false));

    for (int i=0; i<3; ++i) {
      if (secondLength <= maxHoldLengthPerReg) {
        holdCnts.push_back(HoldCntsRegisterEntry(secondLength, true));
        break;
      } else {
        holdCnts.push_back(HoldCntsRegisterEntry(maxHoldLengthPerReg, true));
        secondLength -= maxHoldLengthPerReg;
      }
    }

    holdCnts.push_back(HoldCntsRegisterEntry(cycleLength-secondPos+1, false));
  }
  return true;
}

void Sequencer::printHoldCounts()
{
  for(uint i=0; i< holdCnts.size(); i++){
    SuS_LOG_STREAM(info, log_id(),"holdphase " << i << " Phase " << holdCnts[i].hold << " length " << holdCnts[i].length );
  }
}

void Sequencer::printTracks()
{
  for(int i=0; i< c_numOfTracks; i++){
    SuS_LOG_STREAM(info, log_id(),"+++++");
    for(uint p=0; p< getTrack((TrackNum)i)->getPhases().size(); p++){
      SuS_LOG_STREAM(info, log_id(),"TrackPhase " << i << " Phase " << getTrack((TrackNum)i)->getPhases().at(p).high << " length " << getTrack((TrackNum)i)->getPhases().at(p).clockCycles );
    }
  }
}

void Sequencer::generateSimPhases(int t)
{
//TODO for F2
    //Fix Track 1
    SequencerTrack::SequencePhases injSeqPhases;
    injSeqPhases.push_back(SequencerTrack::SequencePhase(0,1));
    injSeqPhases.push_back(SequencerTrack::SequencePhase(1,21));
    injSeqPhases.push_back(SequencerTrack::SequencePhase(0,((cycleLength*7)-21-1)));
    if(!getTrack(Sequencer::FCF_Flip)->setPhasesAndCompile(injSeqPhases)) {
      SuS_LOG_STREAM(error, log_id(), "Error while updating INJECT track.");
    }


    //Fix Track 2
    SequencerTrack::SequencePhases resSeqPhases;
    resSeqPhases.push_back(SequencerTrack::SequencePhase(0,2));
    resSeqPhases.push_back(SequencerTrack::SequencePhase(1,21));
    resSeqPhases.push_back(SequencerTrack::SequencePhase(0,((cycleLength*7)-21-2)));
    if(!getTrack(Sequencer::FCF_Res_B)->setPhasesAndCompile(resSeqPhases)) {
      SuS_LOG_STREAM(error, log_id(), "Error while updating RES track.");
    }

    //Fix Track 3
    SequencerTrack::SequencePhases swinSeqPhases;
    swinSeqPhases.push_back(SequencerTrack::SequencePhase(0,4+t*6));
    swinSeqPhases.push_back(SequencerTrack::SequencePhase(1,21));
    swinSeqPhases.push_back(SequencerTrack::SequencePhase(0,((cycleLength*7)-21-4)-t*6));
    if(!getTrack(Sequencer::FCF_SwIn)->setPhasesAndCompile(swinSeqPhases)) {
      SuS_LOG_STREAM(error, log_id(), "Error while updating SwIn track.");
    }

    //Dynamic Track
    SequencerTrack::SequencePhases rmpSeqPhases;
    rmpSeqPhases.push_back(SequencerTrack::SequencePhase(0,t*7));
    rmpSeqPhases.push_back(SequencerTrack::SequencePhase(1,7));
    rmpSeqPhases.push_back(SequencerTrack::SequencePhase(0,(cycleLength-t-1)*7));
    if(!getTrack(Sequencer::ADC_RMP)->setPhasesAndCompile(rmpSeqPhases)) {
      SuS_LOG_STREAM(error, log_id(), "Error while updating RMP track.");
    }

    getTrack(Sequencer::ADC_RMP)->setStatVal(0);
    getTrack(Sequencer::FCF_SwIn)->setStatVal(0);
    getTrack(Sequencer::FCF_Res_B)->setStatVal(0);
    getTrack(Sequencer::FCF_Flip)->setStatVal(0);
    getTrack(Sequencer::ISubPulse)->setStatVal(0);

    getTrack(Sequencer::ADC_RMP)->setInvHold(0);
    getTrack(Sequencer::FCF_SwIn)->setInvHold(0);
    getTrack(Sequencer::FCF_Res_B)->setInvHold(0);
    getTrack(Sequencer::FCF_Flip)->setInvHold(0);
    getTrack(Sequencer::ISubPulse)->setInvHold(1);

}


void Sequencer::initVariablesFromPhases()
{
  setOpMode(getOpMode(),false);

  integrationLength = getIntegrationTime(false);
  flattopLength     = getFlattopLength();
  flattopHoldLength = getFlattopHoldLength();
  resetLength       = getResetLength();
  resetIntegOffset  = getResetIntegOffset();
  resetHoldLength   = getResetHoldLength();
  rampLength        = getRampLength();
  rampIntegOffset   = getRampIntegOffset();
}

bool Sequencer::generateSignals(
    int _integrationLength,
    int _flattopLength,
    int _flattopHoldLength,
    int _resetLength,
    int _resetIntegOffset,
    int _resetHoldLength,
    int _rampLength,
    int _rampIntegOffset,
    int _backFlipAtReset,
    int _backFlipToResetOffset,
    int _singleCapLoadLength)
{
  integrationLength = _integrationLength;
  flattopLength     = _flattopLength;
  flattopHoldLength = _flattopHoldLength;
  resetLength       = _resetLength;
  resetIntegOffset  = _resetIntegOffset;
  resetHoldLength   = _resetHoldLength;
  rampLength        = _rampLength;
  rampIntegOffset   = _rampIntegOffset;
  backFlipAtReset   = _backFlipAtReset;
  backFlipToResetOffset = _backFlipToResetOffset;
  singleCapLoadLength  = _singleCapLoadLength;

  adaptFlipSignalToSingleSHCapMode();

  return generateSignals();
}

//void setNormModeParameters(
//    int _integrationLength,
//    int _flattopLength,
//    int _flattopHoldLength,
//    int _resetLength,
//    int _resetHoldLength,
//    int _rampLength)
//{
//  integrationLength = _integrationLength;
//  flattopLength     = _flattopLength;
//  flattopHoldLength = _flattopHoldLength;
//  resetLength       = _resetLength;
//  resetHoldLength   = _resetHoldLength;
//  rampLength         = _rampLength;
//}


bool Sequencer::generateSignals()
{
  if(mode == MANUAL){
    SuS_LOG_STREAM(info, log_id(), "generateSignals(): mode = MANUAL, nothing done.");
    return true;
  }

  int cycleLengthFastClk = cycleLength*7;

//SuS_LOG_STREAM(info, log_id(), "Generating all signals automatically.");
//
//SequencerTrack& t0 = *getTrack(RMP);
//for (unsigned int i=0; i<tracks.size()-1; ++i) {
//  //SequencerTrack& t0 = *getTrack((TrackNum)i);
//  t0 = *getTrack((TrackNum)i);
//  SuS_LOG_STREAM(info, log_id(), "0 i = " << i << ", TrackNum = " << t0.trackNum <<
//      ", TrackName = " << Sequencer::c_trackNames[t0.trackNum]);
//}
//SequencerTrack& t1 = *getTrack(RMP);
//for (unsigned int i=0; i<tracks.size(); ++i) {
//  //SequencerTrack& t1 = *getTrack((TrackNum)i);
//  t1 = *getTrack((TrackNum)i);
//  SuS_LOG_STREAM(info, log_id(), "1 i = " << i << ", TrackNum = " << t1.trackNum <<
//      ", TrackName = " << Sequencer::c_trackNames[t1.trackNum]);
//}
//SequencerTrack* t0 = getTrack(RMP);
//for (unsigned int i=0; i<tracks.size()-1; ++i) {
//  //SequencerTrack& t0 = *getTrack((TrackNum)i);
//  t0 = getTrack((TrackNum)i);
//  SuS_LOG_STREAM(info, log_id(), "0 i = " << i << ", TrackNum = " << t0->trackNum <<
//      ", TrackName = " << Sequencer::c_trackNames[t0->trackNum]);
//}
//SequencerTrack* t1 = getTrack(RMP);
//for (unsigned int i=0; i<tracks.size(); ++i) {
//  //SequencerTrack& t1 = *getTrack((TrackNum)i);
//  t1 = getTrack((TrackNum)i);
//  SuS_LOG_STREAM(info, log_id(), "1 i = " << i << ", TrackNum = " << t1->trackNum <<
//      ", TrackName = " << Sequencer::c_trackNames[t1->trackNum]);
//}
//

  // RMP
  if(singleSHCapMode) {
    setSingleCapModeRmpPhases();
  } else {
    int rampPos = cycleLengthFastClk-lastIntPhase+rampIntegOffset;
    if(mode == EXTLATCH){
      rampPos = cycleLength;
    }
    setRMPPos(rampPos);
  }

  // SWIN
  int firstIntegOffset = cycleLengthFastClk-2*integrationLength-flattopLength-lastIntPhase;
  SequencerTrack& swinTrack = *getTrack(Sequencer::FCF_SwIn);
  swinTrack.clearPhases();

  if (mode==BUFFER || mode==RESET || mode==EXTLATCH) {
    swinTrack << SequencePhase(0,cycleLengthFastClk);
  } else {
    swinTrack << SequencePhase(0,firstIntegOffset);
    swinTrack << SequencePhase(mode==SINGLEINT ? 0 : 1,integrationLength);
    swinTrack << SequencePhase(0,flattopLength);
    swinTrack << SequencePhase(1,integrationLength);
    swinTrack << SequencePhase(0,lastIntPhase);
  }

  // RESET
  SequencerTrack& resTrack = *getTrack(Sequencer::FCF_Res_B);
  resTrack.clearPhases();
  int resetOffset = firstIntegOffset-resetLength-resetIntegOffset;
  if (mode==BUFFER || mode==EXTLATCH) {
    resTrack.setStatic(0);
  } else {
    resTrack << SequencePhase(1,resetOffset) << SequencePhase(0,resetLength)
             << SequencePhase(1,cycleLengthFastClk-resetOffset-resetLength);
  }

  // FLIP
  if (mode==BUFFER || mode==RESET || mode==SINGLEINT) {
    disableFlip();
  } else if(mode!=EXTLATCH) {
    setFlipPhases();
  }

  // MM5 reset 2
#ifndef F2IO
  #ifndef F2BIO
    SequencerTrack& res2Track = *getTrack(Sequencer::ISubPulse);
    res2Track.clearPhases();
    #if defined(F1IO)
      res2Track.setStatic(0);
    #else
    resetOffset += 10;
    if (mode==BUFFER) {
      res2Track.setStatic(0);
    } else {
      res2Track << SequencePhase(0,resetOffset) << SequencePhase(1,resetLength)
                << SequencePhase(0,cycleLengthFastClk-resetOffset-resetLength);
    }
    #endif
  #endif
#endif

  int trackShift = 0;
  if(singleSHCapMode){
    trackShift = (cycleLengthFastClk - singleCapLoadLength - (rampLength - 28) ) - (cycleLengthFastClk - lastIntPhase);
    for(uint32_t i=1; i<tracks.size(); i++){
      shiftTrack(tracks[i],trackShift);
    }
  }

  compileAndCheckAllTracks();

  /*
   * changed hold to occur in the middle of the flattop (@ Andrea's request)
   * --> if flattopHoldLength is used, we need both hold slots to implement this,
   * resetHoldLength must be set to 0
   */

  // HOLD
  if (holdPos > 0) {
    setHoldEnabled(true);
    holdCnts[0] = HoldCntsRegisterEntry(holdPos, false);
    holdCnts[1] = HoldCntsRegisterEntry(holdLength, true);
    holdCnts[2] = HoldCntsRegisterEntry(cycleLength-holdPos-2, false);
    holdCnts[3] = HoldCntsRegisterEntry(1, false);
    holdCnts[4] = HoldCntsRegisterEntry(1, false);
  } else if (flattopHoldLength > 0 || resetHoldLength > 0) {
    int flipWindow = flattopLength/7 - 3;
    setHoldEnabled(true);
    int fastHoldFlattopOffset = firstIntegOffset + integrationLength + 14;
    int holdFlattopOffset = fastHoldFlattopOffset / 7 ;
    if ((fastHoldFlattopOffset+(trackShift/7*7))%7) ++holdFlattopOffset;

    if (resetHoldLength > 0) {
      int holdResetOffset = resetOffset/7 + 1;
      if ((resetOffset + trackShift)%7) ++holdResetOffset;
      if (debugMode) {
        SuS_LOG_STREAM(debug, log_id(), "holdResetOffset is " << holdResetOffset);
        SuS_LOG_STREAM(debug, log_id(), "resetHoldLength is " << resetHoldLength);
      }
      holdCnts[0] = HoldCntsRegisterEntry(holdResetOffset, false);
      holdCnts[1] = HoldCntsRegisterEntry(resetHoldLength, true);
      holdCnts[2] = HoldCntsRegisterEntry(holdFlattopOffset-holdResetOffset+flipWindow, false);
    } else {
      holdCnts[0] = HoldCntsRegisterEntry(holdFlattopOffset, false);
      holdCnts[1] = HoldCntsRegisterEntry(flattopHoldLength/2, true);
      holdCnts[2] = HoldCntsRegisterEntry(flipWindow, false);
    }
    if (debugMode) {
      SuS_LOG_STREAM(debug, log_id(), "holdFlattopOffset is " << holdFlattopOffset);
      SuS_LOG_STREAM(debug, log_id(), "flattopHoldLength is " << flattopHoldLength);
    }
    if (flattopHoldLength>0) {
      holdCnts[3] = HoldCntsRegisterEntry(
          resetHoldLength > 0 ? flattopHoldLength : flattopHoldLength/2, true);
      holdCnts[4] = HoldCntsRegisterEntry(cycleLength-holdFlattopOffset-flipWindow, false);
    } else {
      holdCnts[3] = HoldCntsRegisterEntry(1, false);
      holdCnts[4] = HoldCntsRegisterEntry(cycleLength-holdFlattopOffset-1-flipWindow, false);
    }
  } else {
    setHoldEnabled(false);
  }

  if(singleSHCapMode)
  {
    shiftHolds(trackShift);
  }

  /*
  if (flattopHoldLength > 0 || resetHoldLength > 0) {
    setHoldEnabled(true);
    int holdFlattopOffset = (firstIntegOffset + integrationLength) / 7 + 1;
    if (holdFlattopOffset%7) ++holdFlattopOffset;
    if (resetHoldLength > 0) {
      int holdResetOffset = resetOffset/7 + 1;
      if (resetOffset%7) ++holdResetOffset;
      if (debugMode) {
        SuS_LOG_STREAM(debug, log_id(), "holdResetOffset is " << holdResetOffset);
        SuS_LOG_STREAM(debug, log_id(), "resetHoldLength is " << resetHoldLength);
      }
      holdCnts[0] = HoldCntsRegisterEntry(holdResetOffset, false);
      holdCnts[1] = HoldCntsRegisterEntry(resetHoldLength, true);
      holdCnts[2] = HoldCntsRegisterEntry(holdFlattopOffset-holdResetOffset, false);
    } else {
      holdCnts[0] = HoldCntsRegisterEntry(1, false);
      holdCnts[1] = HoldCntsRegisterEntry(1, false);
      holdCnts[2] = HoldCntsRegisterEntry(holdFlattopOffset-2, false);
    }
    if (debugMode) {
      SuS_LOG_STREAM(debug, log_id(), "holdFlattopOffset is " << holdFlattopOffset);
      SuS_LOG_STREAM(debug, log_id(), "flattopHoldLength is " << flattopHoldLength);
    }
    if (flattopHoldLength>0) {
      holdCnts[3] = HoldCntsRegisterEntry(flattopHoldLength, true);
      holdCnts[4] = HoldCntsRegisterEntry(cycleLength-holdFlattopOffset, false);
    } else {
      holdCnts[3] = HoldCntsRegisterEntry(1, false);
      holdCnts[4] = HoldCntsRegisterEntry(cycleLength-holdFlattopOffset-1, false);
    }
  } else {
    setHoldEnabled(false);
  }
  */

  shiftAllTracksRight(rightShift);
  return true;
}

void Sequencer::shiftTrack(SequencerTrack * track, int shift)
{
  if(shift < 0){
    shift = getTotalClockCycles() + shift;
  }
  shiftTrackRight(track, shift);
}


void Sequencer::shiftTrackRight(SequencerTrack * track, uint32_t shift)
{
  //SuS_LOG_STREAM(debug, log_id(), "trackShift is " << shift);

  auto & phases = track->phases;
  if(phases.size() <= 1) return;

  while(shift > 0)
  {
    int currentShift = shift;
    if((uint32_t)phases.back().clockCycles < shift){
      currentShift = phases.back().clockCycles;
    }

    if(phases.front().high == phases.back().high)
    {
      phases.front().clockCycles += currentShift;
    }else{
      phases.insert(phases.begin(),SequencePhase(phases.back().high,currentShift));
    }
    phases.back().clockCycles -= currentShift;
    shift -= currentShift;
    if(phases.back().clockCycles <= 0){
      phases.erase(phases.end()-1);
    }
  }
}


void Sequencer::shiftAllTracksRight(uint32_t shift)
{
  for (auto i=0; i<c_numOfTracks; ++i) {
    shiftTrackRight(getTrack(i), shift);
  }
}


void Sequencer::shiftHolds(int shiftFastCycles)
{
  int shiftSlowCycles = floor(1.0/7.0*shiftFastCycles);
  if(shiftSlowCycles<0){
    shiftSlowCycles = cycleLength + shiftSlowCycles;
  }
  shiftHoldsRight(shiftSlowCycles);

  checkHoldCnts(holdCnts);
}


void Sequencer::shiftHoldsRight(int shiftSlowCycles)
{
  while(shiftSlowCycles > 0)
  {
    int currentShift = shiftSlowCycles;
    if(holdCnts.back().length <= currentShift){
      currentShift = holdCnts.back().length;
      holdCnts.insert(holdCnts.begin(),HoldCntsRegisterEntry(currentShift,holdCnts.back().hold));
      holdCnts.erase(holdCnts.end()-1);
      if(holdCnts.back().hold){
        holdCnts.insert(holdCnts.begin(),holdCnts.back());
        holdCnts.erase(holdCnts.end()-1);
      }
    }
    else
    {
      if(holdCnts.front().hold == holdCnts.back().hold)
      {
        holdCnts.front().length += currentShift;
      }else{
        holdCnts.insert(holdCnts.begin(),HoldCntsRegisterEntry(currentShift,holdCnts.back().hold));
      }
      holdCnts.back().length -= currentShift;
    }
    shiftSlowCycles -= currentShift;
  }
}


SequencerTrack* Sequencer::getTrack(Sequencer::TrackNum n)
{
  if (n > c_numOfTracks-1) {
    SuS_LOG_STREAM(error, log_id(), "TrackNum " << n << " not available.");
    return nullptr;
  }
  SequencerTrack* track = tracks[n];
  //SuS_LOG_STREAM(info, log_id(), "getTrack(): n = " << n << ", TrackNum = " << track->trackNum << ", trackNumAddr " << &(track->trackNum) <<
  //    ", TrackName = " << Sequencer::c_trackNames[track->trackNum] << ", trackAddr " << tracks[n]);
  return track;
}


void Sequencer::setHoldEnabled(bool _holdEnabled)
{
  holdEnabled = _holdEnabled;
}


void Sequencer::setResetHoldLength(int _resetHoldLength)
{
  resetHoldLength = _resetHoldLength;
  generateSignals();
}


void Sequencer::setSequencerParameter(std::string _paramName, int _setting, bool genSignals)
{
  SuS_LOG_STREAM(info, log_id(), "Setting sequencer parameter " << _paramName << " = " << _setting << ".");
  setSequencerParameter(paramNameToSeqParam(_paramName), _setting, genSignals);
}


void Sequencer::setSequencerParameter(SeqParam _p, int _setting, bool genSignals)
{
  switch(_p) {
    case (int)RampLength :
      rampLength = _setting; break;
    case (int)ResetLength :
      resetLength = _setting; break;
    case (int)ResetHoldLength :
      holdPos = 0;
      resetHoldLength = _setting; break;
    case (int)ResetIntegOffset :
      resetIntegOffset = _setting; break;
    case (int)IntegrationLength :
      integrationLength = _setting; break;
    case (int)FlattopLength :
      flattopLength = _setting; break;
    case (int)FlattopHoldLength :
      holdPos = 0;
      flattopHoldLength = _setting; break;
    case (int)HoldLength :
      holdLength = _setting; break;
    case (int)HoldPos :
      holdPos = _setting; break;
    case (int)BackFlipAtReset:
      backFlipAtReset = _setting; break;
    case (int)BackFlipToResetOffset:
      backFlipToResetOffset = _setting; break;
    case (int)RampOffset:
      rampIntegOffset = _setting; break;
    case (int)SingleCapLoadLength:
      singleCapLoadLength = _setting; break;
    case (int)SingleSHCapMode:
      singleSHCapMode = (_setting!=0); break;
    case (int)CycleLength:
      setCycleLength(_setting);  // SequencerQt takes care of JTAG register
      break;
    case (int)InjectRisingEdgeOffset:
      injectRisingEdgeOffset = _setting;
      break;
    case (int)LastIntPhase:
      lastIntPhase = _setting;
      break;
    case (int)EmptyInjectCycles:
      emptyInjectCycles = _setting;
      break;
    case (int)RightShift:
      rightShift = _setting;
      break;
    case (int)FtInjectOffset:
      ftInjectOffset = _setting;
      break;
    case (int) FtFlipOffset:
      ftFlipOffset = _setting;
      break;
    default :
      SuS_LOG_STREAM(warning, log_id(), "SeqParam " << (int)_p << " not valid.");
  }

  if (genSignals) generateSignals();

}


int Sequencer::getSequencerParameter(std::string _paramName)
{
  return getSequencerParameter(paramNameToSeqParam(_paramName));
}


std::string Sequencer::getParameterName(SeqParam _p)
{
  switch(_p) {
    case (int)RampLength :
      return "RampLength";
    case (int)ResetLength :
      return "ResetLength";
    case (int)ResetHoldLength :
      return "ResetHoldLength";
    case (int)ResetIntegOffset :
      return "ResetIntegOffset";
    case (int)IntegrationLength :
      return "IntegrationLength";
    case (int)FlattopLength :
      return "FlattopLength";
    case (int)FlattopHoldLength :
      return "FlattopHoldLength";
    case (int)HoldLength :
      return "HoldLength";
    case (int)HoldPos :
      return "HoldPos";
    case (int)BackFlipAtReset:
      return "BackFlipAtReset";
    case (int)BackFlipToResetOffset:
      return "BackFlipToResetOffset";
    case (int)RampOffset :
      return "RampOffset";
    case (int)SingleCapLoadLength :
      return "SingleCapLoadLength";
    case (int)SingleSHCapMode :
      return "SingleSHCapMode";
    case (int)CycleLength:
      return "CycleLength";
    case (int)InjectRisingEdgeOffset:
      return "InjectRisingEdgeOffset";
    case (int)LastIntPhase:
      return "LastIntPhase";
    case (int)EmptyInjectCycles:
      return "EmptyInjectCycles";
    case (int)RightShift:
      return "RightShift";
    case (int)FtInjectOffset:
      return "FtInjectOffset";
    case (int)FtFlipOffset:
      return "FtFlipOffset";
    default :
      SuS_LOG_STREAM(warning, log_id(), "SeqParam " << (int)_p << " not valid.");
      return "";
  }
}


int Sequencer::getSequencerParameter(SeqParam _p) const
{
  switch(_p) {
    case (int)RampLength :
      return rampLength;
    case (int)ResetLength :
      return resetLength;
    case (int)ResetHoldLength :
      return resetHoldLength;
    case (int)ResetIntegOffset :
      return resetIntegOffset;
    case (int)IntegrationLength :
      return integrationLength;
    case (int)FlattopLength :
      return flattopLength;
    case (int)FlattopHoldLength :
      return flattopHoldLength;
    case (int)HoldLength :
      return holdLength;
    case (int)HoldPos :
      return holdPos;
    case (int)BackFlipAtReset:
      return backFlipAtReset;
    case (int)BackFlipToResetOffset:
      return backFlipToResetOffset;
    case (int)RampOffset :
      return rampIntegOffset;
    case (int)SingleCapLoadLength :
      return singleCapLoadLength;
    case (int) SingleSHCapMode :
      return singleSHCapMode;
    case (int) CycleLength:
      return cycleLength;
    case (int)InjectRisingEdgeOffset:
      return injectRisingEdgeOffset;
    case (int)LastIntPhase:
      return lastIntPhase;
    case (int)EmptyInjectCycles:
      return emptyInjectCycles;
    case (int)RightShift:
      return rightShift;
    case (int)FtInjectOffset:
      return ftInjectOffset;
    case (int)FtFlipOffset:
      return ftFlipOffset;
    default :
      SuS_LOG_STREAM(warning, log_id(), "SeqParam " << (int)_p << " not valid.");
      return -1;
  }
}


std::vector<std::string> Sequencer::getParameterNames()
{
  std::vector<std::string> paramNames;
  for(int i=0; i<c_numOfParams; i++){
    paramNames.push_back(getParameterName((SeqParam)i));
  }
  return paramNames;
}


Sequencer::SeqParam Sequencer::paramNameToSeqParam(const std::string & paramName)
{
  auto it = paramNamesMap.find(paramName);
  if (it == paramNamesMap.end()) {
    SuS_LOG_STREAM(error, log_id(), paramName << " not found.");
    return Invalid;
  }
  return it->second;
}


std::map<std::string,uint32_t> Sequencer::getSequencerParameterMap() const
{
  std::map<std::string,uint32_t> paraMap;
  for(int i=0; i<c_numOfParams; i++){
    paraMap[getParameterName((SeqParam)i)] = getSequencerParameter((SeqParam)i);
  }
  return paraMap;
}


void Sequencer::setJtagSubAddressAndName(TrackNum n, int addr, std::string name)
{
  getTrack(n)->jtagSubAddress = addr;
  getTrack(n)->name = name;
}


Sequencer::TrackNum Sequencer::getSeqTrackIndex(int seqTrackAddr)
{
  for (auto i=0; i<c_numOfTracks; ++i) {
    if (getTrack(i)->jtagSubAddress == seqTrackAddr)
      return i;
  }
  cout << "ERROR: Sequencer - seqTrackAddr " << seqTrackAddr << " not found" << endl;
  return 0;
}
