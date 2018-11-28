#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include "DsscHDF5Reader.h"
#include "DsscHDF5Writer.h"
#include "utils.h"

//#define DEBUG
using namespace std;

std::vector<std::string> DsscHDF5Reader::s_directories;

DsscHDF5Reader::DsscHDF5Reader()
  : fileName(""),
    h5File(0),dataset_id(0),
    loaded(false)
{
}


DsscHDF5Reader::DsscHDF5Reader(const string & readFileName)
  : fileName(readFileName),
    h5File(0),dataset_id(0),
    loaded(false)
{
  openHDF5File(fileName);
  cout << "DsscHDF5Reader: File opened: " << fileName << endl;
}


DsscHDF5Reader::~DsscHDF5Reader()
{
  if(h5File!=0){
    H5Fclose(h5File);
    h5File = 0;
  }
}


void DsscHDF5Reader::openHDF5File(const std::string & readFileName)
{
  if(!utils::checkFileExists(readFileName)){
    utils::CoutColorKeeper keeper(utils::STDRED);
    std::cout << "Error HDF5 file not found: " << readFileName;
    h5File = 0;
    loaded = false;
    return;
  }

  h5File = H5Fopen(readFileName.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);
  loaded = (h5File >= 0);
}


herr_t DsscHDF5Reader::op_func (hid_t loc_id, const char *name, const H5O_info_t *info, void *operator_data)
{
    cout << "/";               /* Print root group in object path */
    if (name[0] == '.')        /* Root group, do not print '.' */
        cout << "  (Group)";
    else
      switch (info->type) {
        case H5O_TYPE_GROUP:          cout << name << "  (Group)";    break;
        case H5O_TYPE_DATASET:        cout << name << "  (Dataset)";   s_directories.emplace_back(name);  break;
        case H5O_TYPE_NAMED_DATATYPE: cout << name << "  (Datatype)" << H5Tget_member_name(loc_id,0); break;
        default: cout << name << "  (Unknown)\n";
      }

    cout << "\n";
    return 0;
}

void DsscHDF5Reader::loadDirectoryName(hid_t loc_id, const char *name, const H5O_info_t *info, void *operator_data)
{
  if (name[0] != '.') {
    if (info->type == H5O_TYPE_DATASET){
      m_directories.emplace_back(name);
    }
  }
}


herr_t DsscHDF5Reader::printStructure(const std::string & fileName)
{
  cout << endl;
  cout << "++++ Reading Structure of File : "<< fileName << "\n";

  hid_t file = H5Fopen(fileName.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);
  return printStructure(file);
}


herr_t DsscHDF5Reader::printHDF5Info(hid_t loc_id, const char *name, void *opdata)
{
  ((std::vector<std::string>*)opdata)->push_back(std::string(name));
  return 0;
}


herr_t DsscHDF5Reader::printStructure(hid_t file)
{
  s_directories.clear();

  unsigned majnum;
  unsigned minnum;
  unsigned release;
  H5get_libversion(&majnum,&minnum,&release);

  cout << "LibVersion: " << majnum << " " << minnum << " : " << release << endl;
  cout << "++++ List of all Objects in the File:\n";
  herr_t status = H5Ovisit (file, H5_INDEX_NAME, H5_ITER_NATIVE, op_func, NULL);
  cout << endl;
  cout << endl;

  return status;
}


herr_t DsscHDF5Reader::loadStructure(const std::string & fromDir, std::vector<std::string> & containedGroups)
{
  H5Giterate(h5File, fromDir.c_str(), NULL, printHDF5Info, &containedGroups);
  return 0;
}


bool DsscHDF5Reader::isGroupExisting(const std::string & group)
{
  auto res = H5Lexists(h5File,group.c_str(),H5P_DEFAULT);
  return res>0;
}

//used for strings written by python code
void DsscHDF5Reader::readString(const std::string & node, std::string &value)
{
  auto dataset_id = H5Dopen2(h5File, node.c_str(), H5P_DEFAULT);

  char  *rdata[1];
  hid_t tid1 = H5Dget_type( dataset_id );
  hid_t xfer_pid = H5Pcreate (H5P_DATASET_XFER);
  H5Dread (dataset_id, tid1, H5S_ALL, H5S_ALL, xfer_pid, rdata);

  H5Tclose(tid1);
  H5Pclose(xfer_pid);
  H5Dclose(dataset_id);
  value = string(rdata[0]);
}

// used for strings written from c++ code
void DsscHDF5Reader::readDataString(const std::string & node, std::string &value, int MAXLEN)
{
  dataset_id = H5Dopen2(h5File, node.c_str(), H5P_DEFAULT);

  hid_t tid1 = H5Dget_type( dataset_id );
  std::vector<char> rdata(MAXLEN,0);
  H5Dread (dataset_id, tid1, H5S_ALL, H5S_ALL, H5P_DEFAULT, rdata.data());

  H5Tclose(tid1);
  H5Dclose(dataset_id);
  value = string(rdata.data());

#ifdef DEBUG
  std::cout << value << std::endl;
#endif
}


void DsscHDF5Reader::readStringVector(const std::string & node, std::vector<string> &valuesVector)
{
  dataset_id = H5Dopen2(h5File, node.c_str(), H5P_DEFAULT);

  hsize_t dims, maxdims;
  hid_t dataspace = H5Dget_space(dataset_id);
  H5Sget_simple_extent_dims(dataspace, &dims, &maxdims );

  char *rdata[dims];
  hid_t tid1 = H5Tcopy (H5T_C_S1);
  H5Tset_size (tid1,H5T_VARIABLE);
  hid_t xfer_pid = H5Pcreate (H5P_DATASET_XFER);
  H5Dread (dataset_id, tid1, H5S_ALL, H5S_ALL, xfer_pid, rdata);

  H5Tclose(tid1);
  H5Pclose(xfer_pid);
  H5Sclose(dataspace);
  H5Dclose(dataset_id);
  valuesVector.clear();
  for(size_t i=0; i<dims; i++){
    valuesVector.push_back(string(rdata[i]));
  }
#ifdef DEBUG
  std::cout << "Loaded " << valuesVector.size() << " values from " << node << std::endl;
#endif
}


void DsscHDF5Reader::readValue(const std::string & node, int &value)
{
  dataset_id = H5Dopen2(h5File, node.c_str(), H5P_DEFAULT);
  H5Dread(dataset_id, H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT,&value);
  H5Dclose(dataset_id);

#ifdef DEBUG
  std::cout << value << std::endl;
#endif
}


void DsscHDF5Reader::readValue(const std::string & node, uint8_t &value)
{
  dataset_id = H5Dopen2(h5File, node.c_str(), H5P_DEFAULT);
  H5Dread(dataset_id, H5T_STD_U8LE, H5S_ALL, H5S_ALL, H5P_DEFAULT,&value);
  H5Dclose(dataset_id);

#ifdef DEBUG
  std::cout << value << std::endl;
#endif
}


void DsscHDF5Reader::readValue(const std::string & node, uint16_t &value)
{
  dataset_id = H5Dopen2(h5File, node.c_str(), H5P_DEFAULT);
  H5Dread(dataset_id, H5T_STD_U16LE, H5S_ALL, H5S_ALL, H5P_DEFAULT,&value);
  H5Dclose(dataset_id);

#ifdef DEBUG
  std::cout << value << std::endl;
#endif
}


void DsscHDF5Reader::readValue(const std::string & node, uint32_t &value)
{
  dataset_id = H5Dopen2(h5File, node.c_str(), H5P_DEFAULT);
  H5Dread(dataset_id, H5T_STD_U32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT,&value);
  H5Dclose(dataset_id);

#ifdef DEBUG
  std::cout << value << std::endl;
#endif
}


void DsscHDF5Reader::readValue(const std::string & node, uint64_t &value)
{
  dataset_id = H5Dopen2(h5File, node.c_str(), H5P_DEFAULT);
  H5Dread(dataset_id, H5T_STD_U64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT,&value);
  H5Dclose(dataset_id);

#ifdef DEBUG
  std::cout << value << std::endl;
#endif
}

void DsscHDF5Reader::readValue(const std::string & node, double &value)
{
  dataset_id = H5Dopen2(h5File, node.c_str(), H5P_DEFAULT);
  H5Dread(dataset_id, H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT,&value);
  H5Dclose(dataset_id);
#ifdef DEBUG
  std::cout << value << std::endl;
#endif
}


int DsscHDF5Reader::getAsicPixelNumber(int pixel, bool rotate, int maxASIC)
{
  if(maxASIC==0) return pixel;

  int row = pixel/512;
  int col = pixel%512;


  int asicRow = row/64;
  int asicCol = col/64;

  row -= asicRow*64;
  col -= asicCol*64;


  int asicPixel = row*64+col;

  bool upperRow = (asicRow != 0);
  if(rotate && upperRow){
    asicPixel = 4095 - asicPixel;
  }

  return asicPixel;
}


int DsscHDF5Reader::getAsicOfPixel(int pixel, bool invertUpperRow, int maxASIC)
{
  if(maxASIC==0) return pixel/4096;

  int row = pixel/512;
  int col = pixel%512;

  int asicRow = row/64;
  int asicCol = col/64;

  if(invertUpperRow && asicRow==1){
    asicCol = 7 - asicCol;
  }

  row -= asicRow*64;
  col -= asicCol*64;

  int asic = asicRow * 8 + asicCol;

  return asic;
}


void DsscHDF5Reader::readDataHisto(const std::string & node, utils::DataHisto &histo)
{
  std::vector<std::vector<double>> binValuesVector;
  readVectorData2<double>(node + "/histoBins",binValuesVector,H5T_IEEE_F64LE);

  for(auto && binPair : binValuesVector){
    histo.addN(binPair[0],binPair[1]);
  }
}


bool DsscHDF5Reader::checkModuleInfo(const string &node)
{
  std::string quadId = "Qx";
  uint16_t modNr = 1;
  uint iobSerial = 0;
  if(isGroupExisting(node + "ModuleInfo")){
    readDataString(node+"ModuleInfo/quadrantId",quadId);
    readValue(node+"ModuleInfo/moduleNr",modNr);
    readValue(node+"ModuleInfo/iobSerial",iobSerial);
  }

  return DsscHDF5Writer::checkModuleInfo(quadId,modNr,iobSerial);
}


void DsscHDF5Reader::readModuleInfo(const string &node)
{
  std::string quadId = "Qx";
  uint16_t modNr = 1;
  uint iobSerial = 0xDDDD;

  if(isGroupExisting(node + "ModuleInfo")){
    readDataString(node+"ModuleInfo/quadrantId",quadId);
    readValue(node+"ModuleInfo/moduleNr",modNr);
    readValue(node+"ModuleInfo/iobSerial",iobSerial);
  }
  DsscHDF5Writer::updateModuleInfo(quadId,modNr,iobSerial);
}




