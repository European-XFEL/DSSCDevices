#include "FitUtils.h"
#include <math.h>

#include <gsl/gsl_integration.h> //GNU Scientific Library numerical integrator
#include "Minuit2/FunctionMinimum.h"
#include "Minuit2/MnMigrad.h"
#include "Minuit2/MnHesse.h"
#include "Minuit2/MnUserParameters.h"
#include "Minuit2/MnPrint.h"

using namespace std;
namespace utils
{

// changed to pointers and length in order to define easier the ROI (no need to copy vectors)
GaussChi2FCN::GaussChi2FCN(
              double * meas,
              double * pos,    // x vector needs to have size length + 1 (integration)
              double * mvar,
              int length,
              double (*fitfunc)(double, void*))
    : fMeasurements(meas),
      fPositions(pos),
      fMVariances(mvar),
      dataLength(length),
      m_fitfunc(fitfunc)
{
  gsl_set_error_handler(&GSLError::Handler);
}


GaussChi2FCN::~GaussChi2FCN()
{
}

double GaussChi2FCN::operator()(const vector<double>& par) const
{
  double chi2 = 0.;

  double result, error;
  gsl_integration_workspace * w = gsl_integration_workspace_alloc (10000);
  gsl_function F;
  F.function = m_fitfunc;

  vector<double> par_copy(par);
  F.params = &par_copy;

  //bool binBoundariesApplied = true;
  //if (fMeasurements.size() == fPositions.size()) binBoundariesApplied = false;

  for(int n = 0; n < dataLength; n++) {
    if (fMVariances[n] != 0){

      //TODO Error handling of integration (maybe increase of available memory, or fallback to evaluation of function at the bin position)
      //if (binBoundariesApplied)
      //  gsl_integration_qags (&F, fPositions[n], fPositions[n+1], 0, 1e-3, 10000, w, &result, &error);
      //else
        gsl_integration_qags (&F, fPositions[n], fPositions[n]+1, 0, 1e-3, 10000, w, &result, &error);

      /*printf ("x               = %d\n", fPositions[n]);
      printf ("result          = % .18f\n", result);
      printf ("estimated error = % .18f\n", error);
      printf ("intervals =  %d\n", w->size);*/
      chi2 += ((result - fMeasurements[n])*(result - fMeasurements[n])/fMVariances[n]);

      //double functionValue = ig->Integral(fPositions[n],fPositions[n]+1);

      /*vector<double> x;
      x.push_back(fPositions[n]);
      double functionValue = DataAnalyzer::fitfunction(x.data(),par.data());
      chi2 += ((functionValue - fMeasurements[n])*(functionValue - fMeasurements[n])/fMVariances[n]);*/

      //cout << "x " << x.at(0) << " functionValue: " << functionValue << endl;
      //cout << "x " << fPositions[n] /*<< " functionValue " << functionValue*/ << " fMeasurements[n] " << fMeasurements[n] << " fMVariances[n] " << fMVariances[n]  << " chi2 " << chi2 << endl;
    }
  }
  gsl_integration_workspace_free (w);
  /*delete tf1;
  delete wf1;
  delete ig;*/
  //cout << "chi2 " << chi2 << endl;
  return chi2;
}

double GaussChi2FCN::Up() const {return 1.;}

/*************  Fit function definition **********************/


double gauss(const double *x, const double *par)
{
  double noise_amp   = par[0];
  double noise_mean  = par[1];
  double noise_sigma = par[2];
  return noise_amp * exp( -(0.5) * pow((x[0]-noise_mean)/noise_sigma,2) );
}


double gauss(double x, void *par)
{
  std::vector<double> xvec {x};
  return gauss(xvec.data(), ((std::vector<double> *)par)->data());
}

double hyperbel(const double *x, const double *par)
{
  double a = par[0];
  double b = par[1];
  double c = par[2];
  return a + b / (x[0]+c);
}

double hyperbel(double x, void *par)
{
  std::vector<double> xvec {x};
  return hyperbel(xvec.data(), ((std::vector<double> *)par)->data());
}


double inverseHyperbel(double y, const double * par)
{
  double a = par[0];
  double b = par[1];
  double c = par[2];
  return b/(y-a) - c;
}


double fitfunction(double x, void *par)
{
  std::vector<double> xvec {x};
  return fitfunction(xvec.data(), ((std::vector<double> *)par)->data());
}

double fitfunction(const double *x, const double *par)
{
  double result = 0.;

  //! declare and initialize constant value for normalization of Gaussian:
  //! integral of normal distribution (Gaussian with amplitude=sigma=1)
  const double sqr1 = sqrt(2*M_PI);

  //! -----------------------------------------------
  //!    declaration of parameters of fit function

  //! Gaussian noise peak:
  //! normalization (number of counts), mean = position, sigma = std. dev.
  double noise_norm, noise_mean, noise_sigma;

  //! Kalpha line Gaussian:
  //! normalization (number of counts), mean = position, sigma = std. dev.,
  //! and energy [keV]
  double peak1_norm, peak1_mean, peak1_sigma;

  //! Kbeta line Gaussian:
  //! normalization (number of counts), mean = position, sigma = std. dev.,
  //! and energy [keV]
  double peak2_norm, peak2_mean, peak2_sigma;

  //! * overall normalization of trough relative to noise peak
  double troughamp;

  double troughasym = 1;

  //! assign Gaussian noise peak parameter values
  noise_norm  = par[0];
  noise_mean  = par[1];
  noise_sigma = par[2];

  //! assign Gaussian Kalpha peak parameter values
  peak1_norm  = par[3];
  peak1_mean  = par[4];
  peak1_sigma = par[5];

  troughamp   = par[6];

  //double peakt_beta_ref = par[7];
  double peakt_beta     = par[7] * (peak1_mean - noise_mean);
  //en_Ka       = par[26];  // [keV]

  //! assign unique high-energy tail of noise peak parameter:
  //! * scale factor of normalization relative to low-energy tail of X-ray line
  //! * scale factor of shape ("decay") parameter relative to shape parameter
  //!   of low-energy tail of X-ray line
  double noise_tails = par[8];

  //! assign Gaussian Kbeta peak parameter values
  peak2_norm  = par[9];
  peak2_mean  = par[10];
  peak2_sigma = par[11];

  double tail_norm = par[12];

  //! on/off switches for specific spectral components (see above)
  double peakn_switch = par[13];
  double peak1_switch = par[14];
  double peak2_switch = par[15];
  double peakt1_switch = par[16];
  double peakt2_switch = par[17];
  double peakp1_switch = par[18];
  double peakp2_switch = par[19];

  //! normalization of low-energy tails of Kalpha and Kbeta peaks
  double peakt1_normn =
    peakt_beta * exp(-0.25*pow(peak1_sigma/peakt_beta,2));
  double peakt2_normn =
    peakt_beta * exp(-0.25*pow(peak2_sigma/peakt_beta,2));

  //! noise peak (Gaussian, normalized to noise_norm counts)
  if (peakn_switch > 0)
    result += noise_norm *
      (1/(noise_sigma*sqr1)) * exp( -(0.5) * pow((x[0]-noise_mean)/noise_sigma,2) );

  //! if required: peak1 (Kalpha) Gaussian, normalized to peak1_norm counts
  if (peak1_switch > 0)
    result += peak1_norm *
      (1/(peak1_sigma*sqr1)) * exp( -(0.5) * pow((x[0]-peak1_mean)/peak1_sigma,2) );

  //! if required: peak1 (Kalpha) low-energy tail,
  //!              relative normalization to Kalpha peak
  if (peakt1_switch > 0)
    result += tail_norm * peak1_norm *
      (1/peakt1_normn) * 0.5 * erfc( (x[0]-peak1_mean)/peak1_sigma + peak1_sigma/(2*peakt_beta) ) * exp( (x[0]-peak1_mean)/peakt_beta );

  //! if required: peak1 (Kalpha) high-energy tail of noise peak,
  //!              relative normalization to Kalpha peak
  if (peakt1_switch > 0)
    result += tail_norm * peak1_norm *
      noise_tails * (1/peakt1_normn) * 0.5 * erfc(-(x[0]-noise_mean)/noise_sigma+noise_sigma/(2*peakt_beta*troughasym)) * exp(-(x[0]-noise_mean)/(peakt_beta*troughasym));

  //! if required: peak1 (Kalpha) plateau1,
  //!              relative normalization to Kalpha peak
  if (peakp1_switch > 0)
    result += peak1_norm * troughamp * 0.5 *
      (erf( (x[0]-noise_mean)/peak1_sigma ) - erf( (x[0]-peak1_mean)/peak1_sigma ));

  //! if required: peak2 (Kbeta) Gaussian, normalized to peak2_norm counts
  if (peak2_switch > 0)
    result += peak2_norm *
      (1/(peak2_sigma*sqr1)) * exp( -(0.5) * pow((x[0]-peak2_mean)/peak2_sigma,2) );

  //! if required: peak2 (Kbeta) low-energy tail,
  //!              relative normalization to Kalpha peak
  if (peakt2_switch > 0)
    result += tail_norm * peak2_norm *
      (1/peakt2_normn) * 0.5 * erfc( (x[0]-peak2_mean)/peak2_sigma + peak2_sigma/(2*peakt_beta) ) * exp( (x[0]-peak2_mean)/peakt_beta );

  //! if required: peak2 (Kbeta) plateau2,
  //!              relative normalization to Kalpha peak
  if (peakp2_switch > 0)
    result += peak2_norm * troughamp * 0.5 *
      (erf( (x[0]-noise_mean)/peak2_sigma ) - erf( (x[0]-peak2_mean)/peak2_sigma ));

  //! if required: peak2 (Kbeta) high-energy tail of noise peak,
  //!              relative normalization to Kbeta peak
  if (peakt2_switch > 0)
    result += tail_norm * peak2_norm *
      noise_tails * (1/peakt2_normn) * 0.5 * erfc(-(x[0]-noise_mean)/noise_sigma+noise_sigma/(2*peakt_beta*troughasym)) * exp(-(x[0]-noise_mean)/(peakt_beta*troughasym));

  return result;
}


ROOT::Minuit2::FunctionMinimum minuitFit(double * bins, double * vals, double * errs, int length, ROOT::Minuit2::MnUserParameters upar, double(*fitfunc)(double, void*))
{
  // if boundaries are applied, bins.size() = vlas.size()+1
  //std::cout << "minuitFit(): Bin boundaries " << (bins.size()==vals.size() ? " NOT APPLIED." : " APPLIED.") << std::endl;

  // create Chi2 FCN function
  GaussChi2FCN fFCN(vals, bins, errs, length, fitfunc);

  // Optimization

  ROOT::Minuit2::MnMigrad migrad(fFCN, upar);
  std::cout<<"start migrad "<<std::endl;
  ROOT::Minuit2::FunctionMinimum funcMin = migrad();
  if(!funcMin.IsValid()) {
    //try with higher strategy
    std::cout<<"FM is invalid, try with strategy = 2."<<std::endl;
    ROOT::Minuit2::MnMigrad migrad(fFCN, upar, 2);
    funcMin = migrad();

    std::cout<<"minimum after migrad: "<< funcMin <<std::endl;

    //ROOT::Minuit2::MnHesse MnHesse(fFCN, upar, 3);
    ROOT::Minuit2::MnHesse hesse;
    hesse( fFCN, funcMin);

    std::cout<<"minimum after hesse: "<< funcMin <<std::endl;
  }
  else
  {
    std::cout<<"minimum after migrad: "<< funcMin <<std::endl;
  }
  return funcMin;
}


double fitfunctionSimplified(double x, void *par)
{
  std::vector<double> xvec {x};
  return fitfunctionSimplified(xvec.data(), ((std::vector<double> *)par)->data());
}


double fitfunctionSimplified(const double *x, const double *par)
{
  double result = 0.0;
  //double sqr1 = sqrt(2*TMath::Pi());

  double noise_mean   = par[0];                   // x0
  double noise_sigma  = par[1];                   // sigma
  double peak1_sigma  = noise_sigma;              // Georg: extra parameter
  double peakp_amp    = par[2];                   // Ns = peakp_amp*troughrat
  double peak1_mean   = par[3];                   // xi
  double troughrat    = par[4];                   // trough ratio ?
  double peakp1_normn = (peak1_mean-noise_mean);  // Andrea: xi-x0

  double kabE_ratio = par[5];  // 55Fe 6.49/5.9;
  double kabI_ratio = par[6];  // 55Fe 0.1168

  // Kalpha gauss
  result += peakp_amp * exp( -(0.5) * pow((x[0]-peak1_mean)/peak1_sigma,2) );

  // Kbeta gaus
    // fitmodel = fittype(
    // '(k1*a1/(b1-xc))*0.5*( erf( -(x-b1)/(c1/sqrt(2)) ) - erf( -(x-xc)/(c1/sqrt(2)) ) )
    // + a1*exp(-((x-b1)/c1)^2)
    // + a1*I_ratio*exp(-((x-((b1-xc)*kabE_ratio+xc))/c1)^2)','problem', {'xc','kabE_ratio','I_ratio'});
  result += peakp_amp * kabI_ratio * exp( -(0.5) * pow((x[0]-(peak1_mean-noise_mean)*kabE_ratio-noise_mean)/peak1_sigma,2) );

  // shelf term
  //result += peak1_norm * peakp_amp * troughamp *
  //result += troughamp * (0.5) * (TMath::Erfc( (x[0]-peak1_mean)/peak1_sigma ) - TMath::Erfc( (x[0]-noise_mean)/peak1_sigma ));
  result += troughrat * peakp_amp * (1/peakp1_normn) * (0.5) * (erfc( (x[0]-peak1_mean)/peak1_sigma ) - erfc( (x[0]-noise_mean)/peak1_sigma ));

  return result;
}


ADCGainFitResultsVec generateADCGainFitResults(const std::vector<double> & dataVector, const std::vector<uint32_t> & irampSettings)
{
  if(dataVector.empty()) return ADCGainFitResultsVec();

  const size_t numSettings = irampSettings.size();
  const size_t numPixels   = dataVector.size()/numSettings;

  ADCGainFitResultsVec fitResultsVector(numPixels);

  std::atomic_int pixelCnt;
  pixelCnt = 0;

  // generate fit results from loaded data and store it in a new file.
  // At the same time fill the adcfitResultsVector for return
#pragma omp parallel for
  for(size_t px = 0; px<numPixels; px++)
  {
    const auto * pxDataVector = dataVector.data()+(px*numSettings);

    std::vector<double>xValues;
    std::vector<double>yValues;
    for(size_t idx=0; idx < numSettings; idx++){
      if(pxDataVector[idx]>0.0){
        xValues.push_back(irampSettings[idx]);
        yValues.push_back(pxDataVector[idx]);
      }
    }

    if(yValues.empty()){
      fitResultsVector[px].fill(0.0);
      continue;
    }

    auto pxFitResult = utils::fitADCGainVector(xValues,yValues);
    pxFitResult[0] = px;
    fitResultsVector[px] = pxFitResult;

#pragma omp critical
    utils::CoutColorKeeper keeper(utils::STDCYAN);
    cout << "++++ Fitted ADCGain Curve " << std::setw(5) <<  pixelCnt++ << "/" << numPixels << endl;
  }

  return fitResultsVector;
}


ADCGainFitResult fitADCGainVector(std::vector<double> & rmpFineTrmVec, std::vector<double> & gainRatioVec )
{
  ADCGainFitResult result;

  if(rmpFineTrmVec.size() != gainRatioVec.size()) return result;

  // since gain value has arbitrary units the value can vary a lot
  // calc start value for gainRatio ~ 1 value is around 40
  double xFactorInit = 50.0 * utils::getMeandAndRMS(gainRatioVec).mean;

  ROOT::Minuit2::MnUserParameters uparNoise;
  uparNoise.Add("yOffset", 0.0, 1.0);
  uparNoise.Add("xFactor", xFactorInit, 1.0);
  uparNoise.Add("xOffset", 30., 1.0);
  //uparNoise.Fix(0);

  std::vector<double> yValsErrors(rmpFineTrmVec.size(),0.2);

  ROOT::Minuit2::FunctionMinimum noiseFit = utils::minuitFit(rmpFineTrmVec.data(),gainRatioVec.data(),yValsErrors.data(),rmpFineTrmVec.size(),uparNoise,&utils::hyperbel);

  // pixel remains undefined
  int idx = 1;
  result[idx++] = noiseFit.UserState().Value(0);
  result[idx++] = noiseFit.UserState().Value(1);
  result[idx++] = noiseFit.UserState().Value(2);
  result[idx++] = noiseFit.UserState().Error(0);
  result[idx++] = noiseFit.UserState().Error(1);
  result[idx++] = noiseFit.UserState().Error(2);

  return result;
}

void ADCGainFitResults::printHeader(std::ofstream & out, const std::string & pixelsStr)
{
  out << "#ADCGainFitResults:\n";
  out << "#TimeStamp :\t" << utils::getLocalTimeStr() << "\n";
  out << "#Contained Pixels :\t" << pixelsStr << "\n";
  if(pixelsStr == "0-4095"){
    out << "#### Columns\t64\t\n";
  }else{
    out << "#### Columns\t512\t\n";
  }
  out << "#Pixel\t";
  out <<  "yOffset\t";
  out <<  "yOffsetErr\t";
  out <<  "xFactor\t";
  out <<  "xFactorErr\t";
  out <<  "xOffset\t";
  out <<  "xOffsetErr\t";
  out << "\n";
}


std::vector<std::string> ADCGainFitResults::getFitParamNames()
{
  return {"yOffs","xFactor","xOffs","yOffsError","xFactorError","xOffsError"};
}


std::vector<utils::Stats> calcMeanRMSADCGainFitParams(const ADCGainFitResultsVec &adcGainFitResultsVec, int &numGoodValues)
{
  const size_t numPixels = adcGainFitResultsVec.size();
  numGoodValues=0;

  double sigma = 4.0;

  std::vector<utils::Stats> output;

  for(int i=0; i<6; i++){
    std::vector<double> paramValues(numPixels);
#pragma omp parallel for
    for(size_t px=0; px<numPixels; px++){
      paramValues[px] = adcGainFitResultsVec[px][i+1];
    }

    // since fitting runs often bad, only best values should be taken for the mean value, sigma 4 or sigma 3.
    // the paramValues are run through and checked for outliers until the number of good values does not change anymore.
    const auto goodParamValues = utils::removeOutliersFromVectorIt(paramValues,sigma);
    output.push_back(utils::getMeandAndRMS(goodParamValues));
    if(i==1){
      numGoodValues = goodParamValues.size();
    }
  }

  return output;
}


}
