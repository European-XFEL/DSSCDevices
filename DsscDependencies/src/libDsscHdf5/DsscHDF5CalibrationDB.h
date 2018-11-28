#ifndef DSSCHDF5CALIBRATIONDB_H
#define DSSCHDF5CALIBRATIONDB_H

#include<string>

#include "DsscCalibrationDB.h"
#include "DsscHDF5Reader.h"

class DsscHDF5CalibrationDB : public SuS::DsscCalibrationDB, public DsscHDF5Reader
{
  public:

    DsscHDF5CalibrationDB(const std::string & fileName);
    ~DsscHDF5CalibrationDB();

    void saveAs(const std::string & fileName) override
    {
      std::string name = fileName;
      if(utils::getFileExtension(fileName) != ".h5"){
        name += ".h5";
      }
      SuS::DsscCalibrationDB::saveAs(name); // calls saveCopyAs
    }

    void saveCopyAs(const std::string & fileName) override;
    void open() override;

  private:

    void addConfigurations(hid_t & fileHDF5);
    void loadConfigFromFile(const std::string & configID);
    SuS::DsscCalibrationSet loadCalibrationSetConfig(const std::string & configID);

    void openHDF5Structure();
    void printLoadInfo() const;

    std::vector<std::string> m_availableComponents;
    std::vector<std::string> m_availableConfigs;
    std::vector<std::string> m_componentConfigs;

    static const std::string dbDir;

};

#endif // DSSCHDF5CALIBRATIONDB_H
