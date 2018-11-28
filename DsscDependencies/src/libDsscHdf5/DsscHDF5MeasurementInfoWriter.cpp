/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   DsscHDF5MeasurementInfoWriter.cpp
 * Author: dssc
 *
 * Version 2: Module Info added
 *
 * Created on May 31, 2018, 9:18 AM
 */

#include "DsscHDF5MeasurementInfoWriter.h"

using namespace std;

const unsigned int DsscHDF5MeasurementInfoWriter::MeasurementConfig::fileVersion = 2;

DsscHDF5MeasurementInfoWriter::DsscHDF5MeasurementInfoWriter(string fileName)
{
  if(fileName.rfind(".h5") == std::string::npos){
    fileName += ".h5";
  }
  m_fileHDF5 = H5Fcreate(fileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

  addModuleInfo(m_fileHDF5,"");
}


void DsscHDF5MeasurementInfoWriter::addMeasurementConfig(const DsscHDF5MeasurementInfoWriter::MeasurementConfig & measConfig)
{
  m_mainGroupPath = "";

//  createNewGroup(m_fileHDF5, "/INSTRUMENT");
//  createNewGroup(m_fileHDF5, "/INSTRUMENT/DSSC");
//  m_mainName = "/INSTRUMENT/DSSC/TrimmingData/";
//  createNewGroup(m_fileHDF5, m_mainName);

  unsigned int numSettings = 1;
  unsigned int numMeasurements = measConfig.measurementSettings.size();
  hsize_t singleValue = 1;

  writeDataString(m_fileHDF5,m_mainGroupPath,"MeasurementName",measConfig.measurementName);
  writeData<unsigned int> (m_fileHDF5,m_mainGroupPath,"numIterations",H5T_STD_U32LE,&singleValue,&measConfig.numIterations);
  writeData<unsigned int> (m_fileHDF5,m_mainGroupPath,"numMeasurements",H5T_STD_U32LE,&singleValue,&numMeasurements);
  writeData<unsigned int> (m_fileHDF5,m_mainGroupPath,"numPreBurstVetos",H5T_STD_U32LE,&singleValue,&measConfig.numPreBurstVetos);
  writeData<unsigned int> (m_fileHDF5,m_mainGroupPath,"ladderMode",H5T_STD_U32LE,&singleValue,&measConfig.ladderMode);
  writeData<unsigned int> (m_fileHDF5,m_mainGroupPath,"fileVersion",H5T_STD_U32LE,&singleValue,&measConfig.fileVersion);
  writeDataString(m_fileHDF5,m_mainGroupPath,"columnSelection",measConfig.columnSelection);

  hsize_t numAsics = measConfig.activeASICs.size();
  writeData<unsigned int> (m_fileHDF5,m_mainGroupPath,"availableASICs",H5T_STD_U32LE,&numAsics,measConfig.activeASICs.data());
  writeData<unsigned int> (m_fileHDF5,m_mainGroupPath,"numMeasurementParams",H5T_STD_U32LE,&singleValue,&numSettings);

  std::vector<std::string> measParamNames {measConfig.measurementSettingName};
  writeStringVector(m_fileHDF5,m_mainGroupPath,"measParamNames",measParamNames);
  writeDataString(m_fileHDF5,m_mainGroupPath,"timeStamp",utils::getLocalTimeStr());

  string currentGroup = m_mainGroupPath + "/MeasurementSettings";
  hsize_t numMeasSettings = measConfig.measurementSettings.size();
  writeData<unsigned int> (m_fileHDF5,currentGroup,"settings1",H5T_STD_U32LE,&numMeasSettings,measConfig.measurementSettings.data());
  writeData<unsigned int> (m_fileHDF5,currentGroup,"numSettings",H5T_STD_U32LE,&singleValue,&numMeasurements);
  writeDataString(m_fileHDF5,currentGroup,"settingName",measConfig.measurementSettingName);

  writeStringVector(m_fileHDF5,m_mainGroupPath,"Measurements",measConfig.measurementDirectories);
}


DsscHDF5MeasurementInfoWriter::~DsscHDF5MeasurementInfoWriter()
{
  H5Fclose(m_fileHDF5);
}

