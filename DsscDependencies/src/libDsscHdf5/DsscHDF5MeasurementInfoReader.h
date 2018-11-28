#ifndef DSSCHDF5MEASUREMENTINFOREADER_H
#define DSSCHDF5MEASUREMENTINFOREADER_H

#include <string>
#include <vector>

#include "DsscHDF5Reader.h"

class DsscHDF5MeasurementInfoReader : public DsscHDF5Reader
{
  public:
    DsscHDF5MeasurementInfoReader();
    DsscHDF5MeasurementInfoReader(const std::string & readFileName);
    ~DsscHDF5MeasurementInfoReader();

    inline bool isLadderMode() const { return ladderMode; }
    std::string getConfigFilePath() const { return baseDirectory + "/MeasurementConfig.conf"; }
    std::string getRootFileName() const;
    std::string getBaseDirectory() const { return baseDirectory;}

    std::string getMeasurementName() const { return measName; }
    std::string getMeasurementParam(int paramNum) const { return paramNames.at(paramNum); }
    std::vector<std::string> getMeasurementParams() const { return paramNames; }
    std::vector<uint32_t> getAvailableASICs() const { return availableASICs; }

    std::vector<int> getMeasurementSettings(int paramNum) const {return measurementSettings.at(paramNum);}

    uint32_t getNumIterations() const { return numIterations; }
    uint32_t getNumMeasurements() const { return numMeasurements; }
    uint32_t getNumPreBurstVetos() const {return numPreBurstVetos;}
    std::string getColumnSelection() const {return columnSelection;}

    std::string getMeasurementPath(int measNum) const { return measurementDirectories.at(measNum);}
    std::vector<std::string> getMeasurementPaths() const { return measurementDirectories;}

    bool isValid() const {return valid;}

  private:

    hid_t h5File,dataset_id;

    std::string baseDirectory;
    std::string measName;
    int numIterations;
    int numMeasurements;
    int numPreBurstVetos;
    int fileVersion;
    std::string columnSelection;
    std::vector<std::vector<int>> measurementSettings;
    std::vector<std::string> paramNames;
    std::vector<uint32_t> availableASICs;
    std::vector<std::string> measurementDirectories;
    bool ladderMode;
    bool valid;


};

#endif // DSSCHDF5MEASUREMENTINFOREADER_H
