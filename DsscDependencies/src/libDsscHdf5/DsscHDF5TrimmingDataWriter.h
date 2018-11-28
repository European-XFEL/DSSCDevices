#ifndef DSSCHDF5TRIMMINGDATAWRITER_H
#define DSSCHDF5TRIMMINGDATAWRITER_H

#include <string>
#include <vector>

#include "DsscHDF5Writer.h"
#include "DataHisto.h"
#include "DsscGainParamMap.h"

class DsscHDF5TrimmingDataWriter : public DsscHDF5Writer
{
  public:
    DsscHDF5TrimmingDataWriter(const std::string &fileName);
    ~DsscHDF5TrimmingDataWriter();

    std::string getFileName() const {return m_fileName;}
    void setMeasurementName(const std::string & measurementName);

    void addValueData(const std::string & paramName, double value);
    void addStringData(const std::string & paramName, const std::string & value);


    void addVectorData(const std::string & paramName, const std::vector<uint32_t> & values);
    void addVectorData(const std::string & paramName, const std::vector<double> & values);

    void addImageData(const std::string & paramName, hsize_t width, const std::vector<double> & values);

    // add single dataHisto
    void addHistoData(const std::string & histoName, const utils::DataHisto & dataHisto, double scalefactor = 1.0);

    // add DataHistoVec
    void addHistoData(const std::string & groupName, const utils::DataHistoVec & dataHistoVec);
    void addHistoData(const std::string & groupName, const utils::DataHistoVec & dataHistoVec, const std::vector<uint32_t> & containedPixels);


    void addDualVectorData(const std::string & paramName, const std::vector<std::vector<double>> & values);
    void addStringVectorData(const std::string & paramName, const std::vector<std::string> & values);

    void addFitResultData(const std::string & paramName, const utils::FitResultsVec & fitResults);
    void addSpectrumGainMapFitResultData(const std::string & paramName, const utils::SpectrumGainMapFitResultVec & trimmingResults);

    void addFitResultsImageMaps(const utils::FitResultsVec & fitResults);
    void addSpectrumGainMapFitResultsImageMaps(const std::string &subDirName, const utils::SpectrumGainMapFitResultVec & fitResults);
    utils::ADCGainFitResult addADCGainMapFitSlopesData(const utils::ADCGainFitResultsVec & resultsVec);

    void addInjectionCalibrationData(const utils::InjectionCalibrationData & calibrationData);

    void addNewGroup(const std::string & groupName);
    void setCurrentGroup(const std::string & groupName);

    void addCurrentTargetInfo();
    void addCurrentModuleInfo();
    void addGainParamsMap(const SuS::DsscGainParamMap & gainParams, const std::string & node = "GainParams");

    static utils::ADCGainFitResultsVec savePixelADCGainValues(const std::string & outputDir, const std::string & fileName, const std::vector<double> &pixelADCGainValues, const std::vector<uint32_t> & irampSettings);
    static std::string saveSpectrumFitResults(const std::string & outputDir, const utils::SpectrumGainMapFitResultVec & spectrumFitResults, const std::vector<uint32_t> & irampSettings, std::vector<double> &measuredPixelGainValues);

  private:

    bool isGroupExisting(const std::string & nodeName) const {
      return DsscHDF5Writer::isGroupExisting(m_fileHDF5,m_trimmingDirName+nodeName);
    }

    hid_t m_fileHDF5;
    std::string m_fileName;
    std::string m_trimmingDirName;
    static const uint32_t fileVersion;
};


#endif // DSSCHDF5TRIMMINGDATAWRITER_H
