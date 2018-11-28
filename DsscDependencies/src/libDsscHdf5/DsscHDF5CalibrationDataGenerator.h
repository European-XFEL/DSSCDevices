#ifndef DSSCHDF5CALIBRATIONDATAGENERATOR_H
#define DSSCHDF5CALIBRATIONDATAGENERATOR_H

#include <utility>

#include "CalibrationDataGenerator.h"
#include "ConfigReg.h"
#include "DsscHDF5TrimmingDataWriter.h"
#include "DsscHDF5TrimmingDataReader.h"

class DsscHDF5CalibrationDataGenerator : public SuS::CalibrationDataGenerator
{
  public:
    DsscHDF5CalibrationDataGenerator();

    bool computeTargetGainADCConfiguration(const std::string & gainString);
    std::vector<double> computePixelInjectionCalibrationFactors();
    std::string generateTargetGainConfiguration(double targetGain, const utils::SpectrumGainMapFitResultVec & spectrumGainFitResultsVec, const std::vector<uint32_t> & irampSettings);

    //bool computeTargetGainADCConfiguration(double targetGain, std::vector<uint32_t> & irampSettingsVec);
    std::string computeTargetGainADCStatistics(double targetGain, const utils::SpectrumGainMapFitResultVec & spectrumGainFitResultsVec);

    bool loadHistoDataAndFit(const std::string & histogramFileName);
    bool loadHistoDataAndFit(utils::DataHistoMap &dataHistoMap);

    bool importSpektrumFitResultsFile(const std::string & sprectrumFitResultsFileName);
    bool importADCGainMapFile(const std::string & adcGainMapFileName);
    bool importBinningInformationFile(const std::string & binningInfoFileName);
    bool importMeasuredGainMap(const std::string & gainMapFileName);
    bool importPixelInjectionSlopes(const std::string & slopeFileName);
    bool importPxInjectionCalibFactors(const std::string & calibFileName);

    void compareInjectionSlopes(const std::string & slopeFileName);

    static std::pair<std::string,std::vector<double>> loadPxInjCalibrationFactors(const std::string & calibFileName);

  protected:

    std::string saveSpectrumFitResults(const std::string & exportFilename) override;
    std::string saveCalibratedIrampSettings(const std::string & gainStr);

};

#endif // DSSCHDF5CALIBRATIONDATAGENERATOR_H
