/* 
 * File:   DSSC_PPT_API.cpp
 * Author: kirchgessner
 * 
 * Created on 26. Juni 2015, 10:46
 */
#include "DSSC_PPT_API.h"

using namespace SuS;

DSSC_PPT_API::DSSC_PPT_API( PPTFullConfig * fullConfig) :
  DSSC_PPT(fullConfig)
{
  std::cout << "Num Modules in Pixel Register found " << getPixelRegisters()->getNumModules("Control register") << std::endl;
}

DSSC_PPT_API::~DSSC_PPT_API()
{
  
}

bool DSSC_PPT_API::fillSramAndReadout(uint16_t pattern, bool init, bool jtagMode)
{
  bool ok = true;
  
  runContinuousMode(false);
  disableSending(false);
  
  if(init){
    setSendRawData(true);
    jtagRegisters-> setSignalValue("SRAM Controller Config Register", "all", "Send Test Data", 1);
    programJtagSingle("SRAM Controller Config Register");
  }
  
  jtagClockInSramTestPattern(pattern);

  startBurst();
  
  if(init){
    jtagRegisters-> setSignalValue("SRAM Controller Config Register", "all", "Send Test Data", 0);
    programJtagSingle("SRAM Controller Config Register");
    setSendRawData(false,false,true);
  }

  return ok;
}



const uint16_t * DSSC_PPT_API::getPixelSramData(int pixel)
{
  errorMessages.push_back("Function Not Implemented");
  return NULL;
}


const uint16_t * DSSC_PPT_API::getTrailerData()
{
  errorMessages.push_back("Function Not Implemented");
  return NULL;
}


bool DSSC_PPT_API::calibrateCurrCompDAC(bool log, int singlePx, int startSetting, int usDelay)
{
  errorMessages.push_back("Function Not Implemented");
  return false;
}


bool DSSC_PPT_API::calibrateCurrCompDAC(std::vector<std::pair<int, double> >& vholdVals, bool log, int singlePx, int startSetting, int finalIterations)
{
  errorMessages.push_back("Function Not Implemented");
  return false;
}

bool DSSC_PPT_API::calibratePxsIrampSetting(const double aimSlope)
{
  errorMessages.push_back("Function Not Implemented");
  return false;
}

void DSSC_PPT_API::enBusInj(bool enable, std::string pixel)
{
  errorMessages.push_back("Function Not Implemented");
}

void DSSC_PPT_API::enBusInjRes(bool enable, std::string pixel)
{
  errorMessages.push_back("Function Not Implemented");
}

void DSSC_PPT_API::enPxInjDC(bool enable, std::string pixel)
{
  errorMessages.push_back("Function Not Implemented");
}

void DSSC_PPT_API::generateInitialChipData(DataPacker *packer)
{
  errorMessages.push_back("Function Not Implemented");
}

void DSSC_PPT_API::generateCalibrationInfo(DataPacker *packer){
  errorMessages.push_back("Function Not Implemented");
}

bool DSSC_PPT_API::doSingleCycle(DataPacker* packer, bool testPattern)
{
  errorMessages.push_back("Function Not Implemented");
  return false;
}

bool DSSC_PPT_API::doSingleCycle(int numTries, DataPacker* packer, bool testPattern)
{
  errorMessages.push_back("Function Not Implemented");
  return false;
}
