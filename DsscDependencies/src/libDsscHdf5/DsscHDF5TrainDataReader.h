#ifndef DSSCHDF5TRAINDATAREADER_H
#define DSSCHDF5TRAINDATAREADER_H

#include "DsscHDF5Reader.h"
#include "DataHisto.h"
#include "DsscTrainData.hh"

class DsscHDF5TrainDataReader : public DsscHDF5Reader
{
  public:
    DsscHDF5TrainDataReader();
    DsscHDF5TrainDataReader(const std::string & readFileName);

    using DATAFORMAT = utils::DsscTrainData::DATAFORMAT;

    virtual ~DsscHDF5TrainDataReader();

    void fillTrainData(utils::DsscTrainData * trainDataToFill);
    const uint16_t * getData(const std::vector<int> &pixelsToExport) throw (std::string);
    const uint16_t * loadData() throw (std::string);
    const utils::DsscTrainData * getTrainData();

    uint64_t getTrainID() const {return trainID;}
    std::string getAcquisitionTime() const {return acTime;}
    int getNumFrames() const { return numFrames;}
    int getNumASICs() const { return numASICs;}
    int getNumPreBurstVetos(){return (pptData.empty()? 0 : pptData[0]);}
    bool isLadderMode();
    inline int getMaxASIC() const {return maxASIC;}
    void readImageDataVersion();
    int getTempADCValue(int ASIC);

    int getDimX() const {return dimX;}
    int getDimY() const {return dimY;}

    uint16_t getASICTrailerWord(int asic, int wordNum);

    std::vector<uint32_t> getAvailableASICs() { return availableASICs;}
    static std::vector<uint16_t> fillDataHistoFromTrainDataFile(const std::string & readFileName, utils::DataHistoVec & pixelHistograms, uint16_t asicsToPack);

  private:
    bool checkMattiaVersion();
    void readTrainID();
    void readTrainIDMattia();
    void readAcquisitionTime();
    void readPPTData();
    void readSIBData();
    void readASICTrailerData();
    void readAvailableASICs();
    void readImageFormat();

    void sortData(const std::vector<int> &pixelsToExport);
    void sortDataPreVer4(const std::vector<int> &pixelsToExport);

    int getHDF5ASICID(uint32_t asic);

    static uint16_t * imageData;
    static uint16_t * pixelData;
    static utils::DsscTrainData * trainData;

    uint64_t trainID;
    std::string acTime;
    std::vector<uint16_t> pptData;
    std::vector<uint16_t> sibData;
    std::vector<uint8_t>  asicTrailerData;
    std::vector<uint32_t> availableASICs;

    int dimX;
    int dimY;
    int numFrames;
    int numASICs;
    int maxASIC;
    uint32_t version;
    bool isMattiaHDF5Format;
    DATAFORMAT m_dataFormat;
};

#endif // DSSCHDF5TRAINDATAREADER_H
