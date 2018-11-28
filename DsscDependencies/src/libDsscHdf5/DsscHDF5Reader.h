#ifndef DSSCHDF5READER_H
#define DSSCHDF5READER_H

#include <string>
#include <vector>

#include "hdf5.h"
#include "DataHisto.h"

class DsscHDF5Reader
{
  public:
    DsscHDF5Reader();
    DsscHDF5Reader(const std::string & readFileName);
    ~DsscHDF5Reader();

    void openHDF5File(const std::string & readFileName);
    static herr_t printStructure(const std::string & fileName);
    static herr_t printStructure(hid_t file);

    static int getSramSize() {return 800;}

    bool isValid(){ return loaded;}

  protected:

    static std::vector<std::string> s_directories;
    static herr_t op_func (hid_t loc_id, const char *name, const H5O_info_t *info, void *operator_data);

    /*
     * Operator function.
     */
    static herr_t printHDF5Info(hid_t loc_id, const char *name, void *opdata);

    herr_t loadStructure(const std::string & fromDir, std::vector<std::string> & containedGroups);

    void loadDirectoryName(hid_t loc_id, const char *name, const H5O_info_t *info, void *operator_data);

    std::vector<std::string> m_directories;

    template <typename UINT_T>
    void readVectorData(const std::string & nodeName, std::vector<UINT_T> & dataVector, hid_t hdf5_data_type)
    {
      hid_t dataset_id = H5Dopen2(h5File, nodeName.c_str(), H5P_DEFAULT);
      H5D_space_status_t status;
      H5Dget_space_status(dataset_id,&status);

      hid_t dataspace = H5Dget_space(dataset_id);
      int nDims = H5Sget_simple_extent_ndims(dataspace);
      hsize_t dims[nDims];
      hsize_t maxDims[nDims];
      H5Sget_simple_extent_dims(dataspace,dims,maxDims);

      if(nDims>0){
        int numValues = 1;
        for(int dim = 0; dim < nDims; dim++){
            numValues*=dims[dim];
        }
        dataVector.resize(numValues);

        H5Dread(dataset_id, hdf5_data_type, H5S_ALL, H5S_ALL, H5P_DEFAULT,dataVector.data());
      }
      H5Sclose(dataspace);
      H5Dclose(dataset_id);
    }

    template <typename UINT_T>
    void readVectorData2(const std::string & nodeName, std::vector<std::vector<UINT_T>> & dataVector, hid_t hdf5_data_type, bool transpose = false)
    {
      hid_t dataset_id = H5Dopen2(h5File, nodeName.c_str(), H5P_DEFAULT);
      H5D_space_status_t status;
      H5Dget_space_status(dataset_id,&status);

      hid_t dataspace = H5Dget_space(dataset_id);
      int nDims = H5Sget_simple_extent_ndims(dataspace);
      hsize_t dims[nDims];
      hsize_t maxDims[nDims];
      H5Sget_simple_extent_dims(dataspace,dims,maxDims);

      if(nDims>1){
        size_t numValues = 1;
        for(int dim = 1; dim < nDims; dim++){
            numValues*=dims[dim];
        }

        UINT_T * values = new UINT_T[dims[0]*numValues];
        H5Dread(dataset_id, hdf5_data_type, H5S_ALL, H5S_ALL, H5P_DEFAULT,values);

        if(transpose){
          dataVector.resize(numValues);
          for(auto && vec : dataVector){
              vec.resize(dims[0]);
          }
          for(hsize_t x=0; x <dims[0]; x++){
            size_t offs = x*numValues;
            for(size_t y=0; y<numValues; y++){
              dataVector[y][x] = values[offs+y];
            }
          }
        }else{
          dataVector.resize(dims[0]);
          for(auto && vec : dataVector){
              vec.resize(numValues);
          }
          for(hsize_t x=0; x <dims[0]; x++){
            size_t offs = x*numValues;
            for(size_t y=0; y<numValues; y++){
              dataVector[x][y] = values[offs+y];
            }
          }
        }
        delete [] values;
      }
      H5Sclose(dataspace);
      H5Dclose(dataset_id);
    }

    bool isGroupExisting(const std::string &group);

    void readString(const std::string & node, std::string &value);
    void readDataString(const std::string & node, std::string &value, int MAXLEN = 400);

    void readStringVector(const std::string & node, std::vector<std::string> &valuesVector);
    void readValue(const std::string & node, int &value);
    void readValue(const std::string & node, uint8_t &value);
    void readValue(const std::string & node, uint16_t &value);
    void readValue(const std::string & node, uint32_t &value);
    void readValue(const std::string & node, uint64_t &value);
    void readValue(const std::string & node, double &value);

    void readDataHisto(const std::string & node, utils::DataHisto &histo);

    int getAsicOfPixel(int pixel, bool invertUpperRow, int maxASIC);
    int getAsicPixelNumber(int pixel, bool rotate, int maxASIC);

    void readModuleInfo(const std::string & node = "/INSTRUMENT/DSSC/");
    bool checkModuleInfo(const std::string & node = "/INSTRUMENT/DSSC/");

    std::string fileName;
    hid_t h5File,dataset_id;

    bool loaded;
};

#endif // DSSCHDF5READER_H
