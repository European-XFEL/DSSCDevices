#include "DsscHDF5CorrectionFileReader.h"

DsscHDF5CorrectionFileReader::DsscHDF5CorrectionFileReader()
  : validCorrectionFile(false)
{
}

DsscHDF5CorrectionFileReader::DsscHDF5CorrectionFileReader(const std::string &readFileName)
  : DsscHDF5Reader(readFileName),
    validCorrectionFile(false)
{
  checkValidCorrectionFile();
  checkModuleInfo();
}


DsscHDF5CorrectionFileReader::~DsscHDF5CorrectionFileReader()
{
}


void DsscHDF5CorrectionFileReader::loadCorrectionData(std::vector<float> & baselineValues,
                                                      std::vector<std::vector<float> > & sramCorrectionValues)
{
  readVectorData("/INSTRUMENT/DSSC/SramCorrection/baselineData",baselineValues,H5T_IEEE_F32LE);
  std::vector<float> dummy;
  readVectorData("/INSTRUMENT/DSSC/SramCorrection/correctionData",dummy,H5T_IEEE_F32LE);

  int numPixels = dummy.size()/800;
  sramCorrectionValues.resize(numPixels,std::vector<float>(800));
  for(int i=0;i<numPixels; i++){
    sramCorrectionValues[i].assign(dummy.begin()+(i*800),dummy.begin()+((i+1)*800));
  }
}


void DsscHDF5CorrectionFileReader::checkValidCorrectionFile()
{
  //not correctly implemented
  validCorrectionFile = true;
}
