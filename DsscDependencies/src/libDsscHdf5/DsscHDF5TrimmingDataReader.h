#ifndef DSSCHDF5TRIMMINGDATAREADER_H
#define DSSCHDF5TRIMMINGDATAREADER_H

#include "DsscHDF5Reader.h"
#include "DataHisto.h"
#include "DsscGainParamMap.h"

class DsscHDF5TrimmingDataReader : public DsscHDF5Reader
{
  public:
    DsscHDF5TrimmingDataReader(const std::string & fileName);
    ~DsscHDF5TrimmingDataReader();

    std::string getMeasurementName() const {return m_measurementName;}

    std::vector<uint32_t> readPixels();
    std::vector<uint32_t> readRmpFineTrmSettings();
    std::vector<double> getDataVector(const std::string & node);
    std::string getStringData(const std::string & node);

    utils::DataHisto readPixelDataHisto(uint32_t pixel);
    utils::DataHisto readPixelDataHisto(const std::string & node, uint32_t pixel);
    utils::DataHistoMap readPixelDataHistos(const std::vector<uint32_t> & pixels);
    utils::DataHistoMap readPixelDataHistos();

    utils::FitResultsMap readPixelFitResultsMap();
    utils::FitResultsVec readPixelFitResultsVec();

    utils::SpectrumGainMapFitResultVec readPixelGainMapFitResultsVec(const std::string & paramName);

    utils::ADCGainFitResultsVec readADCGainFitParameterVec();
    SuS::DsscGainParamMap readGainParamsMap();

    template <typename INT_T>
    INT_T readDataValue(const std::string & node){
      INT_T value;
      DsscHDF5Reader::readValue(m_mainNode+node,value);
      return value;
    }
  private:

    uint32_t m_fileVersion;
    std::string m_measurementName;
    const std::string m_mainNode;

};

#endif // DSSCHDF5TRIMMINGDATAREADER_H
