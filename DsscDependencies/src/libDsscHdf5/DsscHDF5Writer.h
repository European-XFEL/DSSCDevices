#ifndef DSSCHDF5WRITER_HPP
#define DSSCHDF5WRITER_HPP


#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <utility>

#include "utils.h"
#include "hdf5.h"
#include "DsscTrainData.hh"
#include "DsscHDF5RegisterConfig.hh"

template <class DATA_TYPE>
class VectorQueue
{
  public:
    VectorQueue(int numContainers, size_t vectorSize)
      : vectorArray(std::vector<std::vector<DATA_TYPE>>(numContainers,std::vector<DATA_TYPE>(vectorSize,0)))
    {
      if(numContainers == 0){
        vectorArray.push_back(std::vector<DATA_TYPE>(std::max(vectorSize,(size_t)1),0));
      }
      for(auto && vec : vectorArray){
        m_dataQueue.push_back(vec.data());
      }
    }

    void resize(size_t newVecSize){
      if(vectorArray.front().size() == newVecSize) return;

      for(auto && vec : vectorArray){
        vec.resize(newVecSize);
      }
      m_dataQueue.clear();
      for(auto && vec : vectorArray){
        m_dataQueue.push_back(vec.data());
      }
    }

    DATA_TYPE * pop_front(){
      return m_dataQueue.pop_front();
    }

    void push_back(DATA_TYPE * ptr){
      m_dataQueue.push_back(ptr);
    }

  private:
    std::vector<std::vector<DATA_TYPE>> vectorArray;
    utils::SimpleThreadSafeQueue<DATA_TYPE*> m_dataQueue;
};


class DsscHDF5Writer{

    friend class DsscHDF5CalibrationDB;

  public:
    DsscHDF5Writer(){}
    virtual ~DsscHDF5Writer(){}
    // trainData is passed as lvalue argument, this requires a std::move operation at argument passing.
    // this implementation prevents from copying the trainData object but moves the data, therefore the trainData object
    // is invalid after function call. This could also be implemented with pointers, but this way it is memory save in any case
    static void saveConfiguration(std::string fileName);
    static void saveConfiguration(std::string fileName, const DsscHDF5ConfigData & h5ConfigData);
    static utils::MeanRMSVectors saveToFile(std::string fileName, const std::vector<utils::StatsAcc> & dataStatsAcc, const std::vector<uint32_t> & activeASICs, uint32_t numTrains);
    static void saveBaselineAndSramCorrection(const std::string &fileName, const std::vector<double> & meanSramValues, const std::vector<uint32_t> & activeASICs, uint32_t numTrains);
    static void saveBaselineAndSramCorrection(std::string fileName, const std::vector<float> & backGroundValues, const std::vector<std::vector<float>> & sramCorrectionValues, const std::vector<uint32_t> & activeASICs, uint32_t numTrains);
    static void saveToFile(std::string fileName, utils::DsscTrainData * trainData, int numRows = 64, int numColumns = 64 );

    static void updateModuleInfo(const std::string & quadrantId, uint moduleNr, uint iobSerial);
    static bool checkModuleInfo(const std::string & quadrantId, uint moduleNr, uint iobSerial);
    static void addModuleInfo(hid_t fileHDF5, const std::string & node = "/INSTRUMENT/DSSC/");

    static bool en8BitMode;
    static bool enHDF5Compression;
    static int  hdf5CompressionLevel;

    struct WriterModuleInfo
    {
        std::string quadrantId;
        uint moduleNr;
        uint iobSerial;

        bool operator==(const WriterModuleInfo & rhs) const{
          if(quadrantId != rhs.quadrantId) return false;
          if(moduleNr   != rhs.moduleNr) return false;
          if(iobSerial  != rhs.iobSerial) return false;
          return true;
        }

        bool operator!=(const WriterModuleInfo & rhs) const{
          if(quadrantId != rhs.quadrantId) return true;
          if(moduleNr   != rhs.moduleNr) return true;
          if(iobSerial  != rhs.iobSerial) return true;
          return false;
        }

        std::string getInfoStr() const {
          std::string info;
          info += "QuadrantId: " + quadrantId;
          info += " ModuleNr: " + moduleNr;
          info += " IOBSerial: " + utils::getIOBSerialStr(iobSerial);
          return info;
        }
    };

    static WriterModuleInfo s_writerModuleInfo;

    static const uint64_t m_numBytesInTrain;

  protected:
    static void addImages(hid_t & fileHDF5, utils::DsscTrainData * trainData, hsize_t numRows, hsize_t numColumns);
    static void addConfiguration(hid_t & fileHDF5); // Maybe ConfigReg added and store complete config
    static void addConfiguration(hid_t & fileHDF5, const DsscHDF5ConfigData & configData);
    static void addConfiguration(hid_t & fileHDF5, const DsscHDF5RegisterConfig & registerConfig);
    static void addConfiguration(hid_t & fileHDF5, const DsscHDF5RegisterConfigVec & registerConfigVec);
    static void addConfiguration(hid_t & fileHDF5, const std::string & node, const DsscHDF5SequenceData & sequenceData);

    static void addCorrectionData(hid_t & fileHDF5, const std::string & node, const std::string & name, const std::vector<std::vector<float>>& correctionMap);
    static void addDoubleVectorData(hid_t & fileHDF5, const std::string & node, const std::string & name, const std::vector<std::vector<double>>& doubleVector);

    static void addDescriptors(hid_t & fileHDF5, utils::DsscTrainData * trainData);
    static void addSpecific(hid_t & fileHDF5, utils::DsscTrainData * trainData);

    static void addTrain(hid_t & fileHDF5, utils::DsscTrainData * trainData);
    static void addMapData(hid_t & fileHDF5, const std::string & node, const std::map<std::string,uint32_t> & mapData);

    static void createNewGroup(hid_t & fileHDF5, const std::string & groupName);
    static void createGroupStructure(hid_t & fileHDF5);
    static void createGroupStructureConfig(hid_t & fileHDF5);
    static void createGroupStructureConfig(hid_t & fileHDF5, const DsscHDF5ConfigData & configData);
    static void createGroupStructureRegister(hid_t & fileHDF5,const DsscHDF5RegisterConfig & registerConfig);
    static void createGroupStructureRegister(hid_t &fileHDF5, const std::vector<DsscHDF5RegisterConfig> &registerConfig);
    static void createGroupStructureSequence(hid_t & fileHDF5, const std::string &node, const DsscHDF5SequenceData & sequenceData);

    static std::array<hsize_t,4> getDims(utils::DsscTrainData * trainData);

    template <typename UINT_T>
    static void writeData(hid_t & fileHDF5, const std::string & node, const std::string & name, hid_t dtype_id, hsize_t *dims,const UINT_T * data, uint32_t arraySize = 1)
    {// properties = access property list
      const std::string nodeName = (node.back() == '/')? node+name : node + "/" + name;

      hid_t datatype = H5Tcopy(dtype_id);
      H5Tset_size (datatype, sizeof(UINT_T)*arraySize);
      if(arraySize==1){
        H5Tset_order(datatype, H5T_ORDER_LE);
      }

      hid_t dataspace = H5Screate_simple(1,dims,NULL);

      hid_t dataset = H5Dcreate2(fileHDF5,nodeName.c_str(),datatype,dataspace,
                                 H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);

      herr_t status = H5Dwrite(dataset,datatype,H5S_ALL,H5S_ALL,H5P_DEFAULT,data);
      if(status<0) std::cout << "HDF5Writer: Could not write "<< nodeName << " to HDF5 file" <<  std::endl;

      H5Sclose(dataspace);
      H5Tclose(datatype);
      H5Dclose(dataset);
    }

    static void writeDataString(hid_t & fileHDF5, const std::string & node, const std::string & name, const std::string & data)
    {
      const uint32_t STRSIZE = data.length();

      hsize_t dimStr[1] = {1};

      if(STRSIZE < 1){
        hsize_t dimStr[1] = {1};
        writeData<char>(fileHDF5,node,name,H5T_C_S1,dimStr,"Empty",5u);
        std::cout << "WARNING HDF5 Writer: Empty String written" << std::endl;
        return;
      }else{
        writeData<char>(fileHDF5,node,name,H5T_C_S1,dimStr,data.c_str(),STRSIZE);
      }
    }

    static void writeStringVector(hid_t & fileHDF5, const std::string & node, const std::string & name, const std::vector<std::string> & dataVec)
    {
      std::vector<const char*> arr_c_str;
      for (size_t ii = 0; ii < dataVec.size(); ++ii){
        arr_c_str.push_back(dataVec[ii].c_str());
      }

      const std::string nodeName = (node.back() == '/')? node+name : node + "/" + name;

      hid_t datatype = H5Tcopy(H5T_C_S1);
      H5Tset_size (datatype, H5T_VARIABLE);

      hsize_t dims[1] {arr_c_str.size()};

      hid_t dataspace = H5Screate_simple(1,dims,NULL);

      hid_t dataset = H5Dcreate2(fileHDF5,nodeName.c_str(),datatype,dataspace,
                                 H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);

      herr_t status = H5Dwrite(dataset,datatype,H5S_ALL,H5S_ALL,H5P_DEFAULT,arr_c_str.data());
      if(status<0) std::cout << "HDF5Writer: Could not write "<< nodeName << " to HDF5 file" <<  std::endl;

      H5Sclose(dataspace);
      H5Tclose(datatype);
      H5Dclose(dataset);
    }

    static bool isGroupExisting(hid_t fileHDF5, const std::string &group);


    static VectorQueue<uint8_t> s_8BitStorages;

};


#endif
