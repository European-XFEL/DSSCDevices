#include "DsscHDF5Writer.h"

#include "DsscHDF5CalibrationDB.h"

using namespace std;

const std::string DsscHDF5CalibrationDB::dbDir = "/INSTRUMENT/DSSC/calibrationDB/";

DsscHDF5CalibrationDB::DsscHDF5CalibrationDB(const std::string & fileName)
{
  m_dbFileName = fileName;
  if(utils::checkFileExists(m_dbFileName)){
    m_openMutex.lock(); // unlocked when file is read
    m_openThread = new std::thread((void(DsscHDF5CalibrationDB::*)(void))&DsscHDF5CalibrationDB::open,this);
  }
}


DsscHDF5CalibrationDB::~DsscHDF5CalibrationDB()
{
  saveAs(m_dbFileName);
  //utils::fileBackup(m_dbFileName);
}

void DsscHDF5CalibrationDB::saveCopyAs(const std::string & fileName)
{
  if(loaded && (fileName == m_dbFileName)){
    H5Fclose(h5File);
    utils::fileMove(fileName,fileName+".old");
    h5File = 0;
  }
  // h5file is closed in DsscHDF5Reader
  h5File = H5Fcreate(fileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

  addConfigurations(h5File);
  // h5file is closed in DsscHDF5Reader

  utils::CoutColorKeeper keeper(utils::STDGREEN);
  cout << "HDF5 Calibration DB saved to "  << fileName << endl;
}


void DsscHDF5CalibrationDB::addConfigurations(hid_t & fileHDF5)
{
  DsscHDF5Writer::createNewGroup(fileHDF5, "/INSTRUMENT");
  DsscHDF5Writer::createNewGroup(fileHDF5, "/INSTRUMENT/DSSC");
  DsscHDF5Writer::createNewGroup(fileHDF5, "/INSTRUMENT/DSSC/calibrationDB");

  const  string baseName ="/INSTRUMENT/DSSC/calibrationDB/";
  const auto configParamNames = SuS::DsscCalibrationSetConfig::getParameterNames();
  DsscHDF5Writer::writeDataString(fileHDF5,baseName,"parameterNames",utils::joinToString(configParamNames,";"));

  std::map<std::string,bool> addedDeviceGroups;
  for(auto && item : m_valuesMap)
  {
    const auto calibrationSet = item.second;

    string deviceName = calibrationSet.getComponentId();
    string id         = calibrationSet.getId();
    string idDir = baseName + deviceName + "/" + id;
    string node  = idDir + "/calibrationData";

    if(!addedDeviceGroups[deviceName]){
      DsscHDF5Writer::createNewGroup(fileHDF5, baseName + deviceName);
      addedDeviceGroups[deviceName] = true;
    }
    DsscHDF5Writer::createNewGroup(fileHDF5, idDir);
    DsscHDF5Writer::createNewGroup(fileHDF5, node);

    DsscHDF5Writer::writeDataString(fileHDF5,idDir,"ComponentID",calibrationSet.getComponentId());
    DsscHDF5Writer::writeDataString(fileHDF5,idDir,"DateTime",calibrationSet.getDateTime());
    DsscHDF5Writer::writeDataString(fileHDF5,idDir,"Version",calibrationSet.getVersion());
    DsscHDF5Writer::writeDataString(fileHDF5,idDir,"GainInfo",calibrationSet.getGainInfo());

    const auto parameterNames = calibrationSet.getParameterNames();
    for(auto && param : parameterNames){
      DsscHDF5Writer::writeDataString(fileHDF5,idDir,param,calibrationSet.getParamValue(param));
    }

    const auto calibrationParamNames = calibrationSet.getCalibrationParamSetNames();
    for(auto && paramName : calibrationParamNames)
    {
      std::string paramNode = node + "/" + paramName;
      DsscHDF5Writer::createNewGroup(fileHDF5,paramNode);

      const auto versions = calibrationSet.getCalibrationParamVersions(paramName);
      for(auto && version : versions)
      {
        const auto paramValues = calibrationSet.getCalibrationParamSet(paramName,version);
        hsize_t numValues = paramValues.size();
        DsscHDF5Writer::writeData<double>(fileHDF5,paramNode,version,H5T_IEEE_F64LE,&numValues,paramValues.data());
      }
    }
  }
}


void DsscHDF5CalibrationDB::open()
{
  if(m_dbFileName.rfind(".h5") != string::npos){
    openHDF5Structure();
  }else{
    SuS::DsscCalibrationDB::open();
  }
}

void DsscHDF5CalibrationDB::openHDF5Structure()
{
  openHDF5File(m_dbFileName);
  if(h5File != 0){
    string activeConfigId;
    m_availableComponents.clear();
    m_availableConfigs.clear();
    m_componentConfigs.clear();

    if(isGroupExisting(dbDir)){

      // initialize configuration parameter names
      string parameterNamesList;
      readDataString(dbDir + "parameterNames",parameterNamesList);
      vector<string> parameterNames;
      utils::split(parameterNamesList,';',parameterNames);

      SuS::DsscCalibrationSetConfig::updateParameterNames(parameterNames);

      //load components which have available calibration data
      loadStructure(dbDir,m_availableComponents);

      utils::removeValue(m_availableComponents,"parameterNames");

      // for each component load available calibration data sets
      for(auto && component : m_availableComponents){
        string componentDir = dbDir + component;
        std::vector<std::string> availConfigs;
        loadStructure(componentDir,availConfigs);

        bool componentActive = false;
        for(auto && configId : availConfigs){
          string activeValue;
          string configGroup = componentDir + "/" + configId;
          readDataString(configGroup + "/active",activeValue);
          loadConfigFromFile(configId);
          if(activeValue == "J"){
            componentActive = true;
            activeConfigId = configId;
          }
        }

        m_availableConfigs.insert(m_availableConfigs.end(),availConfigs.begin(),availConfigs.end());
        if(componentActive){
          m_componentConfigs.insert(m_componentConfigs.end(),availConfigs.begin(),availConfigs.end());
        }
      }
      loaded = true;
      setConfigActive(activeConfigId);
    }
    printLoadInfo();
  }
  // unlock mutex if openHDF5 is called from constructor where mutex is locked
  if(!m_openMutex.try_lock()){
    m_openMutex.unlock();
  }
}


void DsscHDF5CalibrationDB::printLoadInfo() const
{
  utils::CoutColorKeeper keeper(utils::STDGREEN);
  cout << "\nLoaded HDF5 Calibration DB File:\n";
  cout << m_availableConfigs.size() << "  Calibration Configurations found\n";
  if(m_availableComponents.size() == 1){
    cout << "For 1 component: " << m_availableComponents.front();
  }else{
    cout << "For " << m_availableComponents.size() << " different components";
  }
  cout << "\n" << endl;
}


void DsscHDF5CalibrationDB::loadConfigFromFile(const std::string & configID)
{
  string baseDir = dbDir + extractComponentId(configID) + "/" + configID + "/calibrationData";
  auto newCalibrationConfigSet = loadCalibrationSetConfig(configID);

  vector<string> calibParameterNames;
  loadStructure(baseDir,calibParameterNames);

  for(auto && paramName : calibParameterNames){
    string paramDir = baseDir + "/" + paramName;
    vector<string> calibParameterVersions;
    loadStructure(paramDir,calibParameterVersions);
    for(auto && version : calibParameterVersions){
      vector<double> paramValues;
      readVectorData<double>(paramDir + "/" + version,paramValues,H5T_IEEE_F64LE);
      newCalibrationConfigSet.addCalibrationParamSet(paramName,paramValues);
    }
  }
  addNewCalibrationSet(newCalibrationConfigSet);
}


SuS::DsscCalibrationSet DsscHDF5CalibrationDB::loadCalibrationSetConfig(const std::string & configID)
{
  string baseDir = dbDir + extractComponentId(configID) + "/" + configID + "/";
  const auto configParamNames = SuS::DsscCalibrationSetConfig::getParameterNames();
  string paramValuesList = configID + "&";
  for(auto && paramName : configParamNames){
    string value;
    string node = baseDir + "/" + paramName;
    readDataString(node,value);
    paramValuesList += value + ";";
  }

  return SuS::DsscCalibrationSet(paramValuesList);
}

