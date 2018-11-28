#include <iostream>
#include <math.h>

#include "utils.h"

#include "SimUtils.h"


using namespace std;

namespace utils{

SimUtils::SimUtils()
{
  constexpr int NUM = 1000;
  const double sigma = 1.2;

  vector<double> noDNLAcc(2,0.0);
  vector<double> dnlAcc(2,0.0);

  for(int i=0; i<NUM; i++){
    double mu = utils::getRandomNumber<double>(3000.0,6000.0)/100.0;
    double I  = utils::getRandomNumber<double>(300.0,600.0);
    std::vector<uint32_t> xBins;
    const auto gauss = genGauss(mu,sigma,I,xBins);
    const auto dnlVec = genDNLVector();
    const auto gaussCorr = correctBinsForBadDNL(dnlVec,gauss);

    auto histo = fillDataHisto(gaussCorr,xBins);

    auto resNoDNL = histo.fitGauss();

    auto dnlMap = genDNLMap(xBins,dnlVec);
    auto resDNL   = histo.fitGauss(dnlMap);

    cout << "Mean = " << mu << endl;
    cout << "Sigma = " << sigma << endl;
    cout << "I = " << I << endl;
    auto histoOrig = fillDataHisto(gauss,xBins);
    histoOrig.print();
    histo.print();

    cout <<"No DNL: Mean = " << resNoDNL.noiseMean << " +- " << resNoDNL.noiseSigma << " : " << resNoDNL.noiseAmp << endl;
    cout <<"   DNL: Mean = " << resDNL.noiseMean << " +- " << resDNL.noiseSigma << " : " << resDNL.noiseAmp << endl;

    noDNLAcc[0] += pow((resNoDNL.noiseMean-mu),2);
    noDNLAcc[1] += pow((resNoDNL.noiseSigma-sigma),2);
    dnlAcc[0] += pow((resDNL.noiseMean-mu),2);
    dnlAcc[1] += pow((resDNL.noiseSigma-sigma),2);

    cout << "Iteration " << i << "/" << NUM << endl;
  }

  cout << "Simulation Summary: after " << NUM << " iterations:" << endl;
  cout << "No DNL Mean RMS  = " << sqrt(noDNLAcc[0]/(NUM-1)) << endl;
  cout << "No DNL Sigma RMS = " << sqrt(noDNLAcc[1]/(NUM-1)) << endl;
  cout << "DNL Mean RMS  = " << sqrt(dnlAcc[0]/(NUM-1)) << endl;
  cout << "DNL Sigma RMS = " << sqrt(dnlAcc[1]/(NUM-1)) << endl;
}

void SimUtils::printValues(const std::vector<double> & vec) const
{
  for(auto && val : vec){
    cout << val << "\t";
  }
  cout << endl;
}

std::vector<double> SimUtils::genDNLVector()
{
  static constexpr int NUM = 100;

  std::vector<double> dnlValues(NUM);

  double lastDNL = 0.0;
  for(int i=0; i<NUM; i++){
    double dnl = utils::getRandomNumber<double>(0.0,40.0)/100.0 - 0.2 - 0.5*lastDNL;
    lastDNL = dnl;
    dnlValues[i] = dnl;
  }
  return dnlValues;
}


DataHisto::DNLValuesMap SimUtils::genDNLMap(const std::vector<uint32_t> & xBins, const std::vector<double> & dnlVec)
{
  DataHisto::DNLValuesMap dnlMap;

  const size_t numValues = xBins.size();

  for(size_t i=0; i<numValues; i++){
    dnlMap[xBins[i]] = dnlVec[i];
  }
  return dnlMap;
}

std::vector<double> SimUtils::genGauss(double mu, double sigma, double I, std::vector<uint32_t> & xBins)
{
  static constexpr int NUM = 10;

  std::vector<double> gaussValues(NUM);
  xBins.resize(NUM);

  for(int i = 0; i< NUM; i++){
    double x = mu-NUM/2.0+i;
    xBins[i] = floor(x);
    gaussValues[i] = calcGauss(xBins[i],mu,sigma,I);
  }

  return gaussValues;
}

DataHisto SimUtils::genGaussHisto(double mu, double sigma, double I)
{
  static constexpr int NUM = 10;

  DataHisto histo;

  for(int i = 0; i< NUM; i++){
    double x = mu-NUM/2.0+i;
    double y = calcGauss(x,mu,sigma,I);
    histo.addN(x,y);
  }

  return histo;
}

double SimUtils::calcGauss(double x,double mu, double sigma, double I)
{
  constexpr double A0 = 2.0 * sqrt(M_PI);
  return I/A0*exp(-0.5*pow((x-mu)/sigma,2.0));
}

std::vector<double> SimUtils::correctBinsForBadDNL(const std::vector<double> & dnlVec, const std::vector<double> & gaussIn)
{
  const size_t numBins = gaussIn.size();
  std::vector<double> gaussCorr(numBins,0);
  double remain = 0.0;
  int currentBin = 0;
  double realBin = 0.0;
  for(size_t i = 0; i<numBins; i++)
  {
    double dnl = 1.0+dnlVec[i];
    realBin += dnl;
    currentBin++;

    if(realBin < currentBin){
      gaussCorr[i]  = remain;
      remain        = (currentBin-realBin) * gaussIn[i];
      gaussCorr[i] +=  gaussIn[i] - remain;
    }else{
      gaussCorr[i] = remain;
      gaussCorr[i] += gaussIn[i];
      if(i<numBins-1){
        remain = -(realBin-currentBin) * gaussIn[i+1];
        gaussCorr[i] -= remain;
      }
    }
  }
  return gaussCorr;
}


DataHisto SimUtils::fillDataHisto(const std::vector<double> & gaussIn, const std::vector<uint32_t> & xValues)
{
  DataHisto histo;
  const size_t numValues = std::min(gaussIn.size(),xValues.size());
  for(size_t idx = 0; idx <numValues; idx++){
    histo.addN(xValues[idx],gaussIn[idx]);
  }
  return histo;
}

}
