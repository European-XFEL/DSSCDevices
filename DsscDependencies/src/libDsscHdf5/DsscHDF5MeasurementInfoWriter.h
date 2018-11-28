/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   DsscHDF5MeasurementInfoWriter.h
 * Author: dssc
 *
 * Created on May 31, 2018, 9:18 AM
 */

#ifndef DSSCHDF5MEASUREMENTINFOWRITER_H
#define DSSCHDF5MEASUREMENTINFOWRITER_H


#include "DsscHDF5Writer.h"

class DsscHDF5MeasurementInfoWriter : public DsscHDF5Writer {
public:

    struct MeasurementConfig
    {
        static const unsigned int fileVersion;

        std::string configFileName;
        std::string measurementName;
        std::string measurementSettingName;
        unsigned int numIterations;
        unsigned int numPreBurstVetos;
        unsigned int ladderMode;
        std::string columnSelection;
        std::vector<uint32_t> activeASICs;
        std::vector<std::string> measurementDirectories;
        std::vector<unsigned int> measurementSettings;
    };

    DsscHDF5MeasurementInfoWriter(std::string fileName);

    void addMeasurementConfig(const MeasurementConfig & measConfig);

    virtual ~DsscHDF5MeasurementInfoWriter();

  private:

    hid_t m_fileHDF5;
    std::string m_mainGroupPath;
};

#endif /* DSSCHDF5MEASUREMENTINFOWRITER_H */

