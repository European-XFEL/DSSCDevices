#ifndef DSSCHDF5MEANRMSREADER_H
#define DSSCHDF5MEANRMSREADER_H

#include "DsscHDF5Reader.h"

class DsscHDF5MeanRMSReader : public DsscHDF5Reader
{
  public:
    DsscHDF5MeanRMSReader();
    DsscHDF5MeanRMSReader(const std::string & fileName);

    const uint16_t * getData(const std::vector<int> & pixelsToExport);
    uint64_t getTrainID(){return trainID;}

    virtual ~DsscHDF5MeanRMSReader();

  private:

    void sortData(const std::vector<int> &pixelsToExport);

    void readAcquisitionTime();
    void readAvailableASICs();
    void readNumValues();
    void readNumTrains();
    void readNumASICs();
    void readMeanValues();
    int getHDF5ASICID(uint32_t asic);

    static uint16_t * pixelData;
    static uint64_t trainID;

    std::vector<uint32_t> availableASICs;

    int dimX;
    int dimY;

    std::vector<float> meanValues;
    std::string m_acTime;
    uint64_t m_numValues;
    uint32_t m_numTrains;
    uint16_t m_numASICs;

    int maxASIC;
};

#endif // DSSCHDF5MEANRMSREADER_H
