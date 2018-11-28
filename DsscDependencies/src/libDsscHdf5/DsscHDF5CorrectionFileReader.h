#ifndef DSSCHDF5CORRECTIONFILEREADER_H
#define DSSCHDF5CORRECTIONFILEREADER_H

#include <string>
#include <vector>

#include "DsscHDF5Reader.h"

class DsscHDF5CorrectionFileReader : public DsscHDF5Reader
{
  public:
    DsscHDF5CorrectionFileReader();
    DsscHDF5CorrectionFileReader(const std::string & readFileName);
    ~DsscHDF5CorrectionFileReader();

    bool isValid() {return validCorrectionFile;}

    void loadCorrectionData(std::vector<float> & baselineValues, std::vector<std::vector<float> > & sramCorrectionValues);
  private:

    void checkValidCorrectionFile();


    bool validCorrectionFile;
};

#endif // DSSCHDF5CORRECTIONFILEREADER_H
