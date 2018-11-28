#ifndef CALIBRATIONDATAGENERATOR_H
#define CALIBRATIONDATAGENERATOR_H

#include <string>
#include <vector>
#include "utils.h"
#include "DataHisto.h"
#include "DsscGainParamMap.h"

using namespace std;

namespace SuS
{
class ConfigReg;
//  class DsscCalibrationDB;

  class CalibrationDataGenerator
  {

    public:
      CalibrationDataGenerator(int numPixels=65536);

      // calc functions
      void calcPxDelaysFromIrmpGrid(int currDouble);
      void calcNoiseFromInvErrFunc();  // runs over currentPixels
      double calcNoiseFromInvErrFunc(const std::vector<double> & delayValues, const std::vector<double> & sCurve);
      double calcNoiseFromDelaySweepHistos(const std::vector<double> & delayValues, const utils::DataHistoVec & pixelHistograms);
      void calcNoiseFromPxDelayHistos(const std::vector<std::vector<utils::DataHisto> > & pxDelayHistos);
      bool isNoiseOutlier(int px);
      void fitSpectra(utils::DataHistoVec & pixelHistograms, const std::string &exportFilename, const utils::DataHisto::DNLEvalMap & dnlEvalMap = utils::DataHisto::DNLEvalMap());
      void fitSpectra(utils::DataHistoMap & pixelHistograms, const std::string &exportFilename, const utils::DataHisto::DNLEvalMap & dnlEvalMap = utils::DataHisto::DNLEvalMap());
      void calcSlopes(const std::vector<double> & settings, const std::vector<std::vector<utils::DataHistoVec> > & pxHistos);

      void setADCGainMap(const utils::ADCGainFitResultsVec & gainMap);
      void setDNLValuesMap(const utils::DataHisto::DNLEvalMap & dnlEvalMap);
      void setSpectrumFitResults(const utils::FitResultsVec & fitResults);
      void setMeasuredSpectrumGains(const std::vector<double> & measuredGains, const std::vector<uint32_t> & irampSettings);
      void setPixelInjectionSlopes(const std::vector<double> & pxInjectionSlopes, const std::vector<uint32_t> & activePixels);
      void setPixelInjectionFactors(const std::vector<double> & pxInjectionFactors);

      bool isDNLMapLoaded() const {return m_dnlValuesLoaded;}
      bool isADCGainMapLoaded() const {return m_adcGainFitParamsLoaded;}
      bool isSpectrumFitResultsLoaded() const {return m_spectrumFitResultsLoaded;}
      bool isCalibrationDataConfigLoaded() const {return m_calibDataConfigLoaded;}
      bool isMeasuredGainMapLoaded() const {return m_measuredGainMapLoaded;}
      bool isPixelInjectionSlopesLoaded() const {return m_pixelInjectionSlopesLoaded;}
      bool isPixelInjectionCalibFactorsValid() const {return m_pxInjectionCalibFactorsValid;}
      bool isPixelInjectionSlopeCompareFactorsValid() const {return !m_pxInjectionSlopeCompareFactors.empty();}

      void updatePixelInjectionSlopeCompareFactors(const std::vector<double> & measurementSlopes,  const std::vector<uint32_t> & activePixels, const SuS::DsscGainParamMap & gainParamsMap);
      void clearPixelInjectionSlopeCompareFactors() { m_pxInjectionSlopeCompareFactors.clear();}

      // with pixelSpectrumFits and adcGainFitParameters
      utils::InjectionCalibrationData computePixelInjectionCalibrationData();

      std::vector<uint32_t> & computeADCGainConfiguration(double targetGain);
      // with adcGainFitParameters
      std::vector<uint32_t> & computeADCGainConfiguration(double targetGain, const std::vector<double> & pixelGainValues);

      std::vector<uint32_t> getADCGainVector(const utils::ADCGainFitResultsVec & gainFitResults);

      // database import / export
      void genNewCalibSetConfig();
      void loadDB(std::string filename);
      void importPxDelaysFromDB();
      void exportPxDelaysToDB();

      static double calcGainForSetting(const utils::ADCGainFitResult & fitResult, double refGainEv, int refSetting, int newSetting);
      static int calcNewADCSetting(const utils::ADCGainFitResult &fitResult, int setting, double relativeGainChange);
      int calcNewADCSettingUsingMeanFitParams(int setting, double relativeGainChange);

      // text file import / export
      //bool importPxDelays(std::string filename);
      void exportPxDelays(std::string filename);
      void importGrid(int currDouble, std::string filename);
      void exportNoiseInvErrFunc(std::string filename);
      void importPxDelays(std::string filename);
      void importPxGains(std::string filename);

      // getters and setters
      void setOutputDir(std::string dir) {m_outputDir = dir;}
      double getGain() {return m_gain;}
      void setGain(double gain) {m_gain = gain;}
      const std::vector<std::vector<double> > & getPxDelays() {return m_pxDelays;}
      void setCurrentPixels(const vector<int> & pixels) {m_currentPixels=pixels; }
      void useAllPixels();

      void setPixelRegisters(ConfigReg* pxRegs);
      void setPixelRegistersChanged() {m_calibDataConfigLoaded=false;}
      void setGlobalIrampSetting(int value) {m_currentIrampSettings.assign(m_numPixels,value); m_calibDataConfigLoaded=true;}
      void setCurrentIrampSettings(const std::vector<uint32_t> & irampSettings, const std::vector<uint32_t> & activePixels);

      const std::string & getOutputDir() {return m_outputDir;}
      int getNumPixels() {return m_numPixels;}
      const std::vector<std::vector<double> > & getScurves() {return m_sCurves;}
      const std::vector<int> & getCurrentPixels() {return m_currentPixels;}
      std::vector<uint32_t> getCurrentPixelsIrampSettings();

      void setNoiseThresh(double thresh) {m_noiseThresh = m_noiseThresh;}
      std::vector<double> const & getNoiseSigma() const {return m_noiseSigma;}
      std::vector<int> getAllPixels() const;
      std::vector<double> genNoiseEnc() const;

      const utils::ADCGainFitResultsVec & getADCGainMapParameters() const {return m_pixelADCGainFitParameters;}
      const utils::FitResultsVec & getSpectrumFitResults() const {return m_spectrumFitResults;}
      const utils::DataHisto::DNLEvalMap & getDNLInformation() const {return m_dnlEvalsMap;}
      const std::vector<std::vector<double> > & getSlopes() const {return m_slopes;}
      const std::vector<double> & getPxInjectionCalibFactors() const {return m_pxInjectionCalibFactors;}
      const std::string getPxInjCalibMode() const {return m_pxInjCalibMode;}

      const std::vector<double> & getMeasuredTargetGainValues() const {return m_targetGainMeasured;}
      const std::vector<double> & getComputedTargetGainValues() const {return m_targetGainComputed;}

      std::string getLastOutput() const {return m_lastOutput;}
      double getMeasuredGainForSetting(uint32_t pixel, uint32_t setting);

      void setIntegrationTime(int intTime);

      static DsscGainParamMap getGainParamsMap(ConfigReg * pixelReg, int integrationTime);
      DsscGainParamMap getGainParamsMap() const {return m_gainParamsMap;}
      DsscGainParamMap getCompareGainParamsMap() const {return m_pxInjectionSlopeCompareGain;}

    protected:

      void updateGainParamsMap(const SuS::DsscGainParamMap & gainParams);
      void compareToLoadedGainSettings(const SuS::DsscGainParamMap & gainParams,const std::vector<uint32_t> & irampSettings, const std::vector<uint32_t> & activePixels);

      void setPixelRegisterValue(const std::string & module, const std::string & signalName, uint32_t value);
      virtual string saveSpectrumFitResults(const std::string & exportFilename);
      void setLastOutput(const std::string & fileName) {m_lastOutput = fileName;}
      size_t getNumLimitPixels() const {return m_irampLimitPixels.size();}

      uint32_t getCoarseGainSetting(const std::string & signalName);

      ConfigReg* m_pixelRegisters;                       // to look up the ADC gain in case the noise calc is run on the complete grid
      std::string m_pxInjCalibMode;

    protected:

      void initGainMapParams();
      void setGainParamsMap();

      int m_numPixels;                                   // total number of all pixels = length of all vectors
      std::vector<std::vector<double> > m_pxDelays;      //
      //std::vector<double> m_noiseEnc;                  // vector containing the noise in ENC, length = numPixels
      std::vector<double> m_noiseSigma;                  // vector containing the sigma of the predestal
      std::vector<utils::DataHistoMap > m_gridHistoMaps; // grid of pxDelay and irmp for currDouble 0 / 1
      std::vector<std::vector<double> > m_sCurves;       // not really needed to store but handy for straight errorFunc fit in DataAnalyzer
      std::vector<double> m_pxGain;                      // vector with individual pixel gains, determined from separate gain measurement
      std::vector<std::vector<double> > m_slopes;        // nested vector can hold for inst all extracted ADC slopes

      std::vector<double> m_targetGainMeasured;
      std::vector<double> m_targetGainComputed;
      std::vector<uint32_t> m_currentIrampSettings;
      std::vector<double> m_pxInjectionSlopes;
      std::vector<double> m_pxInjectionCalibFactors;
      std::vector<double> m_pxInjectionSlopeCompareFactors;

      SuS::DsscGainParamMap m_pxInjectionSlopeCompareGain;

      utils::FitResultsVec m_spectrumFitResults;
      utils::ADCGainFitResultsVec m_pixelADCGainFitParameters;
      utils::ADCGainFitResult m_meanADCGainFitParameters;
      utils::DataHisto::DNLEvalMap m_dnlEvalsMap;
      std::vector<std::map<int,double> > m_measuredGainMap; // nested vector can hold for inst all extracted spectrum gain values for several iramps

      bool m_adcGainFitParamsLoaded;
      bool m_spectrumFitResultsLoaded;
      bool m_calibDataConfigLoaded;
      bool m_dnlValuesLoaded;
      bool m_measuredGainMapLoaded;
      bool m_pixelInjectionSlopesLoaded;
      bool m_pxInjectionCalibFactorsValid;

      string m_outputDir;                                // output directory for all export functions
      const int c_numPxDelaySteps = 16;
      double m_gain;                                     // gain for ENC calculation
      vector<int> m_currentPixels;                       // all operations are performed on this vector
      //utils::DataHisto m_encHisto;                     // histo of all calculated ENCs
      utils::DataHisto m_sigmaHisto;                     // histo of all calculated ENCs
      //double m_meanENC;                                // mean of the m_encHisto
      double m_meanSigma;                                // mean of the m_sigmaHisto
      double m_noiseThresh;                              // upper threshold for inclusion in the histogram
      std::vector<uint32_t> m_irampLimitPixels;
//      DsscCalibrationDB *m_db;
      std::string m_lastOutput;
      DsscGainParamMap m_gainParamsMap;
  };
}

#endif
