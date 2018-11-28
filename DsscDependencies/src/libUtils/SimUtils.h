#ifndef SIMUTILS_H
#define SIMUTILS_H

#include <vector>

#include "DataHisto.h"

namespace utils {


class SimUtils
{
  public:
    SimUtils();

    std::vector<double> genDNLVector();
    DataHisto::DNLValuesMap genDNLMap(const std::vector<uint32_t> & xBins, const std::vector<double> & dnlVec);

    std::vector<double> genGauss(double mu, double sigma, double I, std::vector<uint32_t> & xBins);
    DataHisto genGaussHisto(double mu, double sigma, double I);

    double calcGauss(double x, double mu, double sigma, double I);
    void printValues(const std::vector<double> & vec) const;
    std::vector<double> correctBinsForBadDNL(const std::vector<double> & dnlVec, const std::vector<double> & gaussIn);

    DataHisto fillDataHisto(const std::vector<double> & gaussIn, const std::vector<uint32_t> & xBins);

};


}
#endif // SIMUTILS_H
