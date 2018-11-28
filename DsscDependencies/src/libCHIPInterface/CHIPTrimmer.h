#ifndef CHIPTRIMMER_H
#define CHIPTRIMMER_H

#include <vector>
#include <string>
#include <map>
#include <thread>
#include <utility>

#include "CHIPGainConfigurator.h"
#include "DataHisto.h"

//#ifdef HAVE_HDF5
//  #include "DsscHDF5CalibrationDB.h"
//#else
//  #include "DsscCalibrationDB.h"
//#endif

namespace SuS {

  class CHIPInterface;

  class CHIPTrimmer
  {
    public :


//#ifdef HAVE_HDF5
//      static DsscHDF5CalibrationDB s_calibrationDB;
//#else
//      static SuS::DsscCalibrationDB s_calibrationDB;
//#endif

      static bool s_saveSpektrumData;
      static bool s_saveImageData;
      static std::string s_saveOutDir;
      static std::vector<std::thread> s_saveThreads;

      CHIPTrimmer():m_chip(nullptr){}

      CHIPTrimmer(CHIPInterface *chip);
      ~CHIPTrimmer(){}

      void abortTrimming(){m_runTrimming = false;}

      void setChipInterface(CHIPInterface *chip);

      bool calibratePxsIrampSettingForDouble(std::vector<uint32_t>  unsettledPxs, // must be copied because it is varied in function
                                             std::vector<double> & finalAbsDiffs,
                                             std::vector<uint32_t> & irampSettings,
                                             const double targetSlope, const double maxAbsDiff,
                                             int currentDouble);

      bool calibratePxsIrampSettingForDoubleUsingInjection(std::vector<uint32_t>  unsettledPxs, // must be copied because it is varied in function
                                                           std::vector<double> & finalAbsDiffs,
                                                           std::vector<uint32_t> & irampSettings,
                                                           const double targetSlope, const double maxAbsDiff,
                                                           int currentDouble);

      bool calibratePxsIrampSettingForDoubleUsingSpektrum(std::vector<uint32_t> unsettledPxs,
                                                          std::vector<double> & finalAbsDiffs,
                                                          std::vector<uint32_t> & irampSettings,
                                                          const double targetSlope,
                                                          const double maxAbsDiff,
                                                          int currentDouble);

      virtual bool calibratePxsIrampSetting(const double targetSlope, const double maxAbsDiff, bool checkOnly = false); // top
      bool calibrateAndCheckPxsIrampSetting(const std::string & runInfo, double targetSlope, const double maxAbsDiff);  // 2nd
      bool calibratePxsIrampSetting(const std::vector<uint32_t> & unsettledPxs, const double targetSlope, const double maxAbsDiff);

      int checkIrampCalibration(std::vector<uint32_t> & unsettledPxs, std::string & status, double targetSlope, double maxAbsDiff);

      int checkIrampCalibrationUsingInjection(std::string & status, double targetSlope, double maxAbsDiff);
      int checkIrampCalibrationUsingSpektrum(std::string & status, double targetSlope, double maxAbsDiff);

      void showTrimmingStats();

      void saveHistograms(const std::vector<uint32_t> & activePixels, const utils::DataHistoVec & pixelHistograms);

      void clearSaveThreads();


      static std::vector<double> calcSlopesFromIntDacSweep(const std::vector<uint32_t> & unsettledPxs,
                                                           uint32_t LINSUM_X, uint32_t LINSUM_XX,
                                                           const std::vector<uint32_t> & dacValues,
                                                           const std::vector<std::vector<double> > &binValues);


      enum TCDM {SLOW=0,DOUBLE=1,BOTH=2};

      void setTrimCurrentDoubleMode(TCDM mode){m_trimCurrentDoubleMode = mode;}
      void setChipParts(std::string chipParts);
      inline const std::vector<std::string> & getChipParts() {return m_chipParts;}
      inline void setDacRange(uint32_t low, uint32_t high) {m_startDacVal=low; m_endDacVal=high;}
      inline void setSramRange(uint32_t low, uint32_t high) {m_startMemAddr=low; m_endMemAddr=high;}
      inline void setAsicWise(bool asicWise) {m_asicWise=asicWise;}
      inline void setNumDacVals(int numDacVals) {m_numDacVals=numDacVals;}
      std::string getGoodTrimmedPixelsStr(std::vector<double> & trimmedSlopes, std::string & rangeStr, const double aimSlope,
      const double maxAbsDiff, int & goodSlopesRel) const;
      const std::vector<double> & getFinalSlopes() const {return m_finalSlopes;}
      const std::vector<double> & getCurvatureValues() const {return m_curvatureValues;}
      void findUntrimmablePixels(std::vector<uint32_t> & pixels);
      void setOutputDir(std::string outputDir);

      void addValuesToCalibrationDB(const std::string & name ,const std::vector<double> & values);
      void updateCalibrationDBGainConfig();

      void generateBinningInformation(const std::vector<uint32_t> &dacSettings, const std::vector<int> & rmpFineTrmSettings);
      const utils::DataHisto::DNLEvalVec &generateBinningInformationForChipParts();

      std::vector<std::vector<double>> generateADCGainMap(const std::vector<uint32_t> irampSettings);
      std::vector<std::vector<double>> generateSlopeInformationForChipParts();

      std::vector<uint32_t> getAllPixels() const;

      void setPixelInjectionCalibrationFactors(const std::vector<double> & pxInjCalibFactors, const std::string & calibGainMode);

      double calcGainValueFromSlope(int pixel,double slope);

    private :

      std::vector<uint32_t> getUntrimmablePixels() const;
      bool itWillBeHardToGetValidValues(const std::vector<uint32_t> & unsettledPxs) const;
      virtual int updateIrampCalibStatus(std::vector<uint32_t> & unsettledPxs, std::string & status, const double targetSlope,const double maxAbsDiff);
      void calibratePxsIrampSettingFromGainMeasurement();
      virtual void writeIrampCalibrateLog(const double targetSlope, const std::string & fileName, bool gainNotSlope = false);
      int calcRampDiff(double slopeDiff, double targetSlope) const;
      int calcRampDiffSpektrum(double gainDiff, double targetGain) const;
      int calcRampDiffUsingRatio(double slopeRat, double targetSlope, int lastRampDiff) const;

      void initMemberVectorSizes();
      void initChipProgressBar(const std::string & title, uint64_t numValues);

    private :
      CHIPInterface *m_chip;
      std::vector<bool>   m_pixelsToTrim;     // inital list with pixels to trim
      std::vector<double> m_finalSlopes;      // full size, all pixels
      std::vector<double> m_curvatureValues;      // full size, all pixels
      std::vector<uint32_t>  m_currentRunPixels; // holds pixels trimmed in current run
      std::vector<std::string> m_chipParts;   // holds parts which are trimmed in together

      //default targetSlope = 0.0632

                              // default vals
      int m_startDacVal;      // 4000
      int m_endDacVal;        // 7000
      int m_numDacVals;       // 5
      int m_startMemAddr;     // 0
      int m_endMemAddr;       // 799
      bool m_asicWise;
      uint32_t m_iterations;
      uint32_t m_numRuns;
      uint32_t m_trimPixelDelayMode;
      std::vector<uint32_t> m_dacValues;
      bool m_errorCode;
      bool m_runTrimming;
      bool m_untrimmablePixelsFound;
      std::vector<bool> m_untrimmablePixels;
      std::vector<std::string> m_trimmingStats;
      TCDM m_trimCurrentDoubleMode;
      std::string m_outputDir;

      std::vector<double> m_pxInjCalibFactors;
      bool m_pxInjCalibfactorsValid;
      std::string m_calibInjMode;

    public:
      CHIPGainConfigurator m_gainConfigurator;

  }; // class CHIPTrimmer

} // namespace SuS

#endif
