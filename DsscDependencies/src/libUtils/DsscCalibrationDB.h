#ifndef DSSCCALIBRATIONDB_H
#define DSSCCALIBRATIONDB_H

#include <thread>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <utility>
#include <fstream>
#include <iostream>
#include <mutex>

#include "DataHisto.h"
#include "DsscGainParamMap.h"

namespace SuS
{

/*
 * Usage Example:
 *
 *

string component = "DSSC_F1_Test";
string version = "Gain_0.5keV_v1.0.0.34";

// initialize Configuration for Calibration Set
SuS::DsscCalibrationSetConfig config;
config.setParamValue("numPixels","4096");
config.setParamValue("active","J");
config.setParamValue("gain_eV","0.7");
config.setParamValue("CSA_CapIn","1");
config.setParamValue("FC_CapEn","5");

// open database
SuS::DsscCalibrationDB calDB("testDB");
// add new Calibration Set with date time version and componentID (ASIC or Sensor...)
calDB.addNewCalibrationSetAndSetActive(SuS::DsscCalibrationDB::generateNewConfigID(version,component),
                                       config.getParameterValues());

auto activeConfig = calDB.getActiveCalibrationSet();

// Add Calibration Parameters as 1 dim vectors
std::vector<double> pixelDelaySteps(4096*16,2.0);
std::vector<double> dnlValues(4096*256,0.3);
std::vector<double> gainCorrectionValues(4096,0.93);
activeConfig->addCalibrationParamSet("PixelDelaySteps",pixelDelaySteps);
activeConfig->addCalibrationParamSet("DNLValues",dnlValues);
activeConfig->addCalibrationParamSet("GainCorrectionValues",gainCorrectionValues);

// db file is saved in destructor
*/
   

class DsscCalibrationSetConfig
{
  public:
    DsscCalibrationSetConfig();

    DsscCalibrationSetConfig(const std::string & line);

    static std::string generateMajorVersionStr();

    static void updateParameterNames(const std::vector<std::string> & newNames);
    
    static std::string getParameterNamesList() {
      return utils::stringVectorToList(s_parameterNames);
    }
    
    static const std::vector<std::string> & getParameterNames() {
      return s_parameterNames;
    }

    inline std::string getComponentId() const { return m_componentID; }
    inline std::string getDateTime() const { return m_dateTime; }
    inline std::string getVersion() const { return m_version; }
    inline std::string getGainInfo() const { return m_gainInfo; }

    inline void setDateTime(const std::string & dateTime) { m_dateTime = dateTime; }
    inline void setVersion(const std::string & newVersion) { m_version = newVersion; }

    std::string getParamValue(const std::string & paramName) const
    {
      size_t pos = std::find(s_parameterNames.begin(),s_parameterNames.end(),paramName) - s_parameterNames.begin();
      if(pos < m_parameterValues.size()){
        return m_parameterValues.at(pos);
      }else{
        utils::CoutColorKeeper keeper(utils::STDRED);
        std::cout << "Parameter " << paramName << " not found\n";
      }
      return "";
    }

    void setParamValue(const std::string & paramName, const std::string &value)
    {
      size_t pos = std::find(s_parameterNames.begin(),s_parameterNames.end(),paramName) - s_parameterNames.begin();
      if(pos < m_parameterValues.size()){
        m_parameterValues[pos] = value;
      }else{
        utils::CoutColorKeeper keeper(utils::STDRED);
        std::cout << "Parameter " << paramName << " not found\n";
      }
    }

    void activate(){setParamValue("active","J");}
    void deactivate(){setParamValue("active","N");}
    bool isActive() const {return getParamValue("active") == "J";}

    inline void save(std::ostream & out) const {
      out << getId() << "&" << getParameterValuesList() << "\n";
    }

    inline std::string getId() const {
      return m_componentID + ";" + m_gainInfo + ";" + m_version + ";" + m_dateTime;
    }


    void setParameters(const std::vector<std::string> & values)
    {
      m_parameterValues = values;
    }

    std::string getParameterValuesList() const
    {
      return utils::stringVectorToList(m_parameterValues);
    }

    const std::vector<std::string> & getParameterValues() const
    {
      return m_parameterValues;
    }

    static std::vector<std::string> getCalibrationSetConfigValueNames();
    std::vector<std::string> getCalibrationSetConfigVec();


  private:

    static void checkParam(const std::string & requiredParam){
      auto elem = std::find(s_parameterNames.begin(),s_parameterNames.end(),requiredParam);
      if(elem == s_parameterNames.end()){
        s_parameterNames.push_back(requiredParam);
      }
    }

    void updateIdFromLine(const std::string & line);
    void updateParamsFromLine(const std::string & line);

    static std::vector<std::string> s_parameterNames;

    std::vector<std::string> m_parameterValues;

    std::string m_dateTime;
    std::string m_version;
    std::string m_gainInfo;
    std::string m_componentID;
};


class DsscCalibrationSet : public DsscCalibrationSetConfig
{
  public:
    DsscCalibrationSet() : DsscCalibrationSetConfig() {}

    DsscCalibrationSet(const std::string & configId);
    DsscCalibrationSet(std::istream & in, const std::string & configIdLine);


    void saveCalibrationSet(std::ostream & out) const
    {
      DsscCalibrationSetConfig::save(out);

      for(auto && entry : m_values){
        for(auto && paramVer : entry.second){
          out << "-" << entry.first << ";" << paramVer.first << "\n";
          utils::join(out,paramVer.second,";");
          out << "\n";
        }
      }
      out << ">\n";
    }

    std::vector<std::string> getCalibrationParamSetNames() const
    {
      std::vector<std::string> paramSetNames;

      // Retrieve all keys
      transform(m_values.begin(), m_values.end(), back_inserter(paramSetNames), utils::RetrieveKey());

      sort(paramSetNames.begin(),paramSetNames.end());

      return paramSetNames;
    }

    std::vector<std::string> getCalibrationParamVersions(const std::string & paramName) const
    {
      const auto paramVersions = m_values.at(paramName);
      std::vector<std::string> paramSetVersions;

      // Retrieve all keys
      transform(paramVersions.begin(), paramVersions.end(), back_inserter(paramSetVersions), utils::RetrieveKey());

      sort(paramSetVersions.begin(),paramSetVersions.end());

      return paramSetVersions;
    }

    const std::vector<double> & getCalibrationParamSet(const std::string & paramName) const
    {
      auto elem = m_values.find(paramName);
      if(elem == m_values.end()){
        throw "Parameter Not found: " + paramName;
      }

      //return newest values set
      const auto paramVersions = getCalibrationParamVersions(paramName);
      return elem->second.at(paramVersions.back());
    }

    const std::vector<double> & getCalibrationParamSet(const std::string & paramName, const std::string & version) const
    {
      return m_values.at(paramName).at(version);
    }


    void addCalibrationParamSet(const std::string & paramNameVer, const std::vector<double> & values)
    {
      std::vector<std::string> paramInfos;
      utils::split(paramNameVer,';',paramInfos);
      std::string paramName = "unknown";
      std::string paramVersion = generateMajorVersionStr();
      if(paramInfos.size() >= 1){
        paramName = paramInfos.front();
      }

      if(paramInfos.size() == 2){
        paramVersion = paramInfos.back();
      }else{
        paramVersion = updateMinorVersionNumber(paramName);
      }

      m_values[paramName][paramVersion] = values;
    }

    bool isCalibrationParamSetIncluded(const std::string & paramName) const
    {
      auto elem = m_values.find(paramName);
      return elem != m_values.end();
    }

  private:
    std::string updateMinorVersionNumber(const std::string & paramName);

    // versionaized parameterValues: paramName -> version -> values
    std::unordered_map<std::string, std::map< std::string,std::vector<double> > > m_values;
};


class DsscCalibrationDB
{
  public:
    DsscCalibrationDB();
    DsscCalibrationDB(const std::string & fileName);
    virtual ~DsscCalibrationDB();

    void initializeActiveCalibrationConfig(int numPixels, const std::string & componentId, const std::string &gainStr, const DsscGainParamMap & paramMap);
    DsscCalibrationSetConfig m_activeConfig;

    static const std::vector<std::string> s_dsscCameraComponents;

    static std::string generateNewConfigID(const std::string & oldConfigId, int minorVersion);
    static std::string generateNewConfigID(const std::string & componentID, const std::string & gainInfo, const std::string & version ) {
      return componentID + ";" + gainInfo + ";" + version + ";" + utils::getLocalTimeStr();
    }

    static std::string extractMajorVersion(const std::string & idStr);
    static std::string extractComponentId(const std::string & idStr);
    static std::string extractGainInfo(const std::string & idStr);
    static std::string extractDateTime(const std::string & idStr);

    std::string addCalibrationSet(const std::string & calibParamName, const std::vector<double> & calibrationParameterValues);

    std::string addNewCalibrationSet(const std::string & configId, const std::vector<std::string> & configParameters); // without data
    std::string addNewCalibrationSet(const DsscCalibrationSet &newSetToAdd); // with data

    std::string addNewCalibrationSetAndSetActive(const std::string & configId, const std::vector<std::string> & configParameters);

    bool activateLatestConfig(const std::string & componentId, const std::string & gainInfo);

    DsscCalibrationSet* setConfigActive(const std::string & configId);

    DsscCalibrationSet *getActiveCalibrationSet() {
      std::unique_lock<std::mutex> lock(m_openMutex);
      return m_activeSet;
    }

    void save(){
      saveAs(m_dbFileName);
    }

    virtual void saveAs(const std::string & fileName){
      std::unique_lock<std::mutex> lock(m_openMutex);
      saveCopyAs(fileName);
      m_dbFileName = fileName;
    }

    virtual void saveCopyAs(const std::string & fileName);
    virtual void open();

    void open(const std::string & fileName ){
      m_dbFileName = fileName;
      open();
    }

    const std::vector<double> & getParameterVector(const std::string & parameter) const  {
      return m_activeSet->getCalibrationParamSet(parameter);
    }

    void setParameterVector(const std::string & parameter, const std::vector<double> & values) {
      m_activeSet->addCalibrationParamSet(parameter,values);
    }

    std::vector<std::string> getConfigurationIds() const;
    std::vector<std::string> getConfigurationIdsOfComponent(const std::string & componentId) const;
    std::vector<std::string> getGainInfos(const std::string & componentId) const;

    std::vector<std::string> getComponentIds() const;

    const DsscCalibrationSet & getCalibrationSet(const std::string & configId) const;
    bool isCalibrationSetExisting(const std::string & componentId, const std::string & gainInfo) const;
    std::vector<std::string> getConfigIdsForComponentGain(const std::string & componentId, const std::string & gainInfo) const;
    std::string getMaxVerConfigIdForComponentGain(const std::string & componentId, const std::string & gainInfo) const;
    static std::string getMaxVerConfigId(const std::vector<std::string> & configIds);
    std::string getFilename() {return m_dbFileName;}

    static std::string getBinningParamName(uint32_t pixel, int irampSetting){
       // "_" section 1 = pixel "_" section 3 = Iramp Setting
       const std::string calibParamName = "BinningInfoPx_"+std::to_string(pixel) + "_Iramp_" + std::to_string(irampSetting);
       return calibParamName;
    }


    utils::DataHisto::DNLValuesVec getPixelBinningInformation(uint32_t pixel, int irampSetting);

  private:
     std::string updateMinorVersionNumber(const std::string & configId);
     bool newCalibrationSetRequired(const std::string & calibParamName);

     bool isCalibParamExisting(const std::string & calibParamName);


  protected:
    std::string m_dbFileName;

    std::unordered_map<std::string,DsscCalibrationSet> m_valuesMap;

    DsscCalibrationSet * m_activeSet;

    std::mutex m_openMutex;
    std::thread * m_openThread;
};

}
#endif // DSSCCALIBRATIONDB_H
