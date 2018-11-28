#ifndef SPECTRUMFITRESULT_H
#define SPECTRUMFITRESULT_H

#include <vector>
#include <string>
#include <map>
#include <ostream>

namespace utils
{

class DataHisto;

class SpectrumFitResult
{
  public:


  struct TargetType {
    void print() const;
    inline std::string getName() const {return std::string(name);}
    static TargetType getCurrentTarget(double targetGain);
    double photonEnergy;
    double kab_Eratio;
    double kab_Iratio;
    char name[8];
  };

  //Silver Optimized Values
  //Eka =   22103.1 eV (includes Ka1, Ka2)
  //Ekb =  25003.7 eV (includes Kb1, Kb2, Kb3)

  //E_ratio =  1.13123023769033

  //I_ratio =  0.18954248366013 (nominal)
  //I_ratio _eff=0.13871963562565 (includes also attenuation effect of 450um of sensor thickness, USE THIS RATIO)

                                                                   //photon energy, Kab Eratio, Kab IRatio

  static const TargetType g_FE55;
  static const TargetType g_NBTarget;
  static const TargetType g_MoTarget;
  static const TargetType g_TcTarget;
  static const TargetType g_RuTarget;
  static const TargetType g_RhTarget;
  static const TargetType g_PdTarget;
  static const TargetType g_AgTarget;
  static const TargetType g_AgAdvTarget;
  static const TargetType g_CdTarget;
  static const TargetType g_InTarget;
  static const TargetType g_SnTarget;
  static const TargetType g_SbTarget;
  static const TargetType g_TeTarget;
  static const TargetType g_ITarget;
  static const TargetType g_XeTarget;
  static const TargetType g_CsTarget;

  using TargetMap = std::map<std::string,TargetType>;
  static TargetMap s_targetMap;

  static TargetType s_currentTarget;

  static void setCurrentTarget(const std::string & targetName);
  static bool checkCurrentTarget(const std::string & targetName);

  SpectrumFitResult()
    : roiNoiseStart(0),roiNoiseEnd(0),roiSignalStart(0),roiSignalEnd(0),
      noiseAmp(0),noiseAmpErr(0),noiseMean(0),noiseMeanErr(0),noiseSigma(0),noiseSigmaErr(0),
      kaPeakAmp(0),kaPeakAmpErr(0),kaPeakMean(0),kaPeakMeanErr(0),
      troughrat(0),troughratErr(0)
  {}

  std::vector<double> getDataVec() const;
  static std::vector<std::string> getDataNames();

  void fill(const std::vector<double> & values);

  double getKaDistance() const {return kaPeakMean-noiseMean;}
  double getGain() const; // gain in eV
  double getGainError() const;
  double getENC() const {return noiseSigma*getGain()/3.6;}
  void print(std::ostream &out) const;
  static void printHeader(std::ofstream & out, const std::string & pixels);

  bool isValid() const;

  double roiNoiseStart;    // double to be able to insert lower binBoundary
  double roiNoiseEnd;
  double roiSignalStart;
  double roiSignalEnd;
  double noiseAmp;
  double noiseAmpErr;
  double noiseMean;
  double noiseMeanErr;
  double noiseSigma;
  double noiseSigmaErr;
  double kaPeakAmp;
  double kaPeakAmpErr;
  double kaPeakMean;
  double kaPeakMeanErr;
  double troughrat;
  double troughratErr;
};


class SpectrumGainMapFitResult : public SpectrumFitResult
{
  public:
    SpectrumGainMapFitResult()
      : SpectrumFitResult(), pixel(0),gainSetting(0)
    {}

    SpectrumGainMapFitResult(const SpectrumFitResult & fitResult);
    void operator=(const SpectrumFitResult& fitResult);
    void print(std::ostream &out) const;
    void printHeader(std::ofstream & out, const std::string & pixelsStr) const;

    std::vector<double> getDataVec() const;
    static std::vector<std::string> getDataNames();
    void fill(const std::vector<double> & values);

    //for sortring and gain setting selection
    uint32_t pixel;
    uint32_t gainSetting;
};

using FitResultsMap = std::map<uint32_t,SpectrumFitResult>;
using FitResultsVec = std::vector<SpectrumFitResult>;
using SpectrumGainMapFitResultVec = std::vector<SpectrumGainMapFitResult>;

FitResultsMap importFitResultsFromASCII(const std::string & fileName);
void dumpFitResultsToASCII(const std::string & fileName, const std::vector<uint32_t> & pixels, const FitResultsVec & fitResults);
void dumpFitResultsToASCII(const std::string & fileName, const FitResultsMap & fitResults);

void dumpSpectrumGainMapFitResultsToASCII(const std::string & fileName, const SpectrumGainMapFitResultVec &fitResults);
SpectrumGainMapFitResultVec importSpectrumGainMapFitResultsFromASCII(const std::string & fileName);

}

#endif // SPECTRUMFITRESULT_H
