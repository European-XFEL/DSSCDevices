#include <iostream>
#include <fstream>
#include "DsscHDF5MeasurementInfoReader.h"
#include "utils.h"

using namespace std;

DsscHDF5MeasurementInfoReader::DsscHDF5MeasurementInfoReader()
 : baseDirectory("."),
   measName("unknown"),
   numIterations(0),
   numMeasurements(0),
   fileVersion(0),
   ladderMode(true),
   valid(false)
{
}


DsscHDF5MeasurementInfoReader::DsscHDF5MeasurementInfoReader(const string &readFileName)
  : DsscHDF5Reader(readFileName),
    baseDirectory(utils::getFilePath(readFileName)),
    measName(""),
    numIterations(0),
    numMeasurements(0),
    fileVersion(0),
    valid(false)
{

  readModuleInfo("");

  cout << "Load Measurement Info file: " << readFileName << endl;
  readString("MeasurementName",measName);

  readStringVector("Measurements",measurementDirectories);
  readStringVector("measParamNames",paramNames);

  readValue("numIterations",numIterations);
  readValue("numMeasurements",numMeasurements);
  readValue("numPreBurstVetos",numPreBurstVetos);

  if(isGroupExisting("fileVersion")){
    readValue("fileVersion",fileVersion);
  }

  columnSelection = "0-63";

  if(fileVersion>0){
    if(isGroupExisting("columnSelection")){
      std::cout << "HDF5 MeasurementInfo ColumnSelection : "  ;
      //readString("columnSelection",columnSelection);
      readDataString("columnSelection",columnSelection);
      std::cout << columnSelection << std::endl;
    }
    if(fileVersion>1){
      readModuleInfo();
    }
  }

  int ladder;
  readValue("ladderMode",ladder);
  ladderMode = (ladder != 0);

  readVectorData("availableASICs",availableASICs,H5T_STD_U32LE);

  valid = true;
  measurementSettings.clear();
  for(uint i=0; i<paramNames.size(); i++){
    std::vector<int> settings;
    readVectorData("MeasurementSettings/settings" + to_string(i+1),settings,H5T_STD_I32LE);
    measurementSettings.push_back(settings);
  }

  if(numMeasurements != (int)measurementDirectories.size()){
    std::cout << "ERROR: numMeasurements does not fit to number of directories: " << numMeasurements << "/" << measurementDirectories.size() << std::endl;
    measurementDirectories.clear();
    valid = false;
  }

  int numMeasSettings = 1;
  for(const auto measSettings : measurementSettings){
    numMeasSettings*=measSettings.size();
  }

  if(numMeasurements != numMeasSettings){
    std::cout << "ERROR: numMeasurements does not fit to number of measurement settings: " << numMeasurements << "/" << numMeasSettings << std::endl;
    measurementDirectories.clear();
    valid = false;
  }

  bool missingDir = false;
  for(const auto & directory : measurementDirectories){
    if(!utils::dirExists(baseDirectory + "/" + directory)){
      missingDir = true;
      cout << "ERROR: Directory not found: " << baseDirectory + "/" + directory << endl;
    }
  }

  if(missingDir){
    measurementDirectories.clear();
    valid = false;
  }
}


DsscHDF5MeasurementInfoReader::~DsscHDF5MeasurementInfoReader()
{
  H5Fclose(h5File);
}


std::string DsscHDF5MeasurementInfoReader::getRootFileName() const
{
  std::string outFileName = measName + ".root";

  utils::stringReplace(outFileName,' ','_');
  utils::stringReplace(outFileName,'#','N');

  return (baseDirectory + "/" + outFileName);
}

