/*
 * File:   DSSC_PPT_API.h
 * Author: kirchgessner
 *
 * Created on 26. Juni 2015, 10:46
 */

#ifndef DSSC_PPT_API_H
#define	DSSC_PPT_API_H

#include "DSSC_PPT.h"


namespace SuS{

class DSSC_PPT_API : public DSSC_PPT{
public:
    DSSC_PPT_API( PPTFullConfig * fullConfig);

    virtual ~DSSC_PPT_API();

    const uint16_t *getPixelSramData(int pixel);
    const uint16_t *getTrailerData();

    bool calibrateCurrCompDAC(bool log=true, int singlePx=-1, int startSetting=0, int usDelay=10);
    bool calibrateCurrCompDAC(std::vector<std::pair<int, double> >& vholdVals, bool log=true, int singlePx=-1, int startSetting=0, int finalIterations=1);
    bool calibratePxsIrampSetting(const double aimSlope);

    int checkCurrCompDACCalibration(){return 0;}

    void enBusInj(bool enable, std::string pixel = "all");
    void enBusInjRes(bool enable, std::string pixel = "all");
    void enPxInjDC(bool enable, std::string pixel = "all");

    void generateInitialChipData(DataPacker *packer);
    void generateCalibrationInfo(DataPacker *packer);

    bool doSingleCycle(DataPacker* packer=NULL, bool testPattern = false);
    bool doSingleCycle(int numTries, DataPacker* packer=NULL, bool testPattern = false);

    void updateDataPackerPixelOffsetsMap(){}

    bool fillSramAndReadout(uint16_t pattern, bool init, bool jtagMode = false) override;


    bool isLadderReadout(){return false;}
    void setLadderReadout(bool enable){}

    inline int setEPCParam(const std::string& moduleSet, const std::string& moduleStr, const std::string& signalName, uint32_t value)
    {
        DSSC_PPT::setEPCParam(moduleSet,moduleStr,signalName,value);
        if(errorMessages.size() != 0){
            return ERROR_PARAM_SET_FAILED;
        }

        return ERROR_OK;
    }

    inline int setIOBParam(const std::string& moduleSet, const std::string& moduleStr, const std::string& signalName, uint32_t value)
    {
        DSSC_PPT::setIOBParam(moduleSet,moduleStr,signalName,value);
        if(errorMessages.size() != 0){
            return ERROR_PARAM_SET_FAILED;
        }

        return ERROR_OK;
    }

    inline int setPixelParam(const std::string& moduleSet, const std::string& moduleStr, const std::string& signalName, uint32_t value)
    {
        DSSC_PPT::setPixelParam(moduleSet,moduleStr,signalName,value);
        if(errorMessages.size() != 0){
            return ERROR_PARAM_SET_FAILED;
        }

        return ERROR_OK;
    }

    inline int setJTAGParam(const std::string& moduleSet, const std::string& moduleStr, const std::string& signalName, uint32_t value)
    {
        DSSC_PPT::setJTAGParam(moduleSet,moduleStr,signalName,value);
        if(errorMessages.size() != 0){
            return ERROR_PARAM_SET_FAILED;
        }

        return ERROR_OK;
    }

    void sramTest(int iterations, bool init = false){
      std::cout << "SramTest can not be implemented in DSSC_PPT_API" << std::endl;
    }

private:

};

}

#endif	/* DSSC_PPT_API_H */

