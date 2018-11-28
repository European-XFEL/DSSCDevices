#ifndef DATAHISTO_H
#define DATAHISTO_H

#include "utils.h"
#include "FitUtils.h"
#include "SpectrumFitResult.h"

namespace utils{

class DataHisto;

using InjectionCalibrationData = std::vector<std::array<double,7>>;

using DataHistoVec = std::vector<DataHisto>;
using DataHistoMap = std::map<int,DataHisto>;

class DataHisto
{
  public:

    static bool enYNormalization;
    using DNLValuesMap = std::map<int,double>;

    static DNLValuesMap DNLVALUESEMPTYMAP;

    struct DNLValue{
        void print(std::ofstream & out) const;
        void read(const std::string & str);
        int bin;
        double dnl;
    };

    struct DNLEval{

        int numValues() const {return 8;} //values + pixel
        void print(std::ofstream & out) const;

        uint32_t pixel;
        double maxDNL;
        double meanDNL;
        double meanDNLRMS;
        double movingMeanDNLRMS;
        double maxINL;
        double meanINL;
        double meanINLRMS;

        DNLValuesMap dnlMap;
        DNLValuesMap inlMap;
        DNLValuesMap dnlMoveMap;
        uint32_t numBinsFound;
    };


    using DNLEvalVec   = std::vector<DNLEval>;
    using DNLEvalMap   = std::map<int,DNLEval>;
    using DNLValuesVec = std::vector<DNLValue>;

    DataHisto();
    DataHisto(const std::vector<uint16_t> & newValues);
    DataHisto(const uint16_t * newValues, int numValues);

    template <typename ITER>
    DataHisto(ITER begin,ITER end)
     : count(0)
    {
      std::fill(binValsBuffer,binValsBuffer+BUFS,0);

      histoValues.reserve(std::distance(begin,end));
      add(begin,end);
    }


    static void dumpHistogramsToASCII(const std::string & fileName, const std::vector<uint32_t> & pixels, const std::vector<DataHisto> & pixelHistos, int setting = 0);
    static DataHistoMap importHistogramsFromASCII(const std::string & fileName, const std::vector<uint32_t> & pixels={}, uint32_t numPixels=65536);
    static DataHistoMap importHistogramsFromASCII(const std::string & fileName, const std::vector<int> & pixels, int numPixels);

    static void exportDNLEvaluationsMap(const DataHisto::DNLEvalMap & evalMap, const std::string & fileName, int deskewSetting = 0);
    static void exportDNLEvaluationsVec(const DataHisto::DNLEvalVec & evalMap, const std::string & fileName, int deskewSetting = 0);
    static DNLEvalMap importDNLEvaluationsMap(const std::string & fileName);
    static DataHisto::DNLEval getDNLEvalFromImportString(const std::string & str);
    static uint32_t getPixelfromStr(const std::string & str);
    static double getValuefromStr(const std::string & str);
    void operator+=(const DataHisto & histo);
    void addN(uint16_t value, uint32_t num);
    void add(uint16_t value);

    inline void addToBuf(uint16_t value){binValsBuffer[value]++;}
    inline void addToBufN(uint16_t value, uint32_t num){binValsBuffer[value]+=num;}

    void addToBuf(const uint16_t *value, uint32_t numValues, const std::vector<bool> &validSram);

    inline void addToBuf(const uint16_t *value, uint32_t numValues){  addToBuf(value,value+numValues);}
    inline void add(const uint16_t *value, uint32_t numValues){add(value,value+numValues);}


    template<typename ITER>
    void addToBuf(ITER begin, ITER end, const std::vector<bool> &validSram){
      auto boolIt = validSram.begin();
      for (ITER it = begin; it != end; it++) {
        if(*boolIt){
          binValsBuffer[(uint)*it]++;
        }
        boolIt++;
      }
    }

    template<typename ITER>
    void addToBuf(ITER begin, ITER end){
      for (ITER it = begin; it != end; it++) {
        binValsBuffer[(uint)*it]++;
      }
    }

    template<typename ITER>
    void add(ITER begin, ITER end){
      for (ITER it = begin; it != end; it++) {
        add(*it);
      }
    }

    static uint64_t binWeight(uint64_t acc, const std::map<uint16_t,uint64_t>::value_type & bin) { return acc + (bin.second*bin.first); }

    std::pair<int, int> getBinRange() const;
    std::vector<uint16_t> getBins() const;
    std::vector<double> getBinsDouble() const;
    std::vector<uint32_t> getBinValues() const;
    std::vector<double> getBinValuesDouble() const;

    std::vector<uint16_t> getBinsFilled() const;
    std::vector<double>   getBinsDoubleFilled() const;
    std::vector<uint32_t> getBinValuesFilled() const;
    std::vector<double>   getBinValuesDoubleFilled() const;

    // get dnl corrected vale vectors: be careful one bin is divided into N values: N = DNLPREC
    static DNLValuesMap toMap(const DNLValuesVec & dnlValues);
    static std::vector<DataHisto::DNLValuesMap> toDnlValuesMapVec(const DNLEvalMap & dnlEvals);

    uint32_t getCount() const {return count;}
    uint64_t getBinValue(uint64_t bin) {return histoValues[bin];}

    static double getDNLValue(const DNLValuesMap & dnlMap, int bin);
    void getDrawValues(std::vector<uint16_t> & xValues, std::vector<uint32_t> &yValues) const;

    inline uint32_t filledBins() const { return histoValues.size();}

    uint32_t getBinContent(uint16_t bin) const;

    Stats getStats() const;
    Stats getStats(uint32_t minBin, uint32_t maxBin) const;

    double calcMean() const;
    double calcRMS(const double & mean) const;

    double calcMean(uint32_t minBin, uint32_t maxBin) const;
    double calcRMS(const double & mean, uint32_t minBin, uint32_t maxBin) const;

    uint16_t getMaxBin();

    double calcDNLRMS() const;
    DNLValuesVec calcDNLValues() const;
    DNLEval calcDNLEval() const;

    SpectrumFitResult fitSpectrum(const DNLValuesMap & dnlValuesMap = DNLValuesMap());
    SpectrumFitResult fitGauss(const DNLValuesMap & dnlValuesMap = DNLValuesMap());

    double calcStepBoarderFraction(int lowerBin, bool aboveThresh=true) const;

    void dumpContent(std::ostream &outStream, int width) const;
    void dumpContent(std::ostream &outStream, const std::string & infoStr, int width) const;
    void print(std::ostream &outStream = std::cout) const;
    void clear();
    void toZero();

    void calcBinBoundaries(const DNLValuesMap & dnlEvalMap);
    void fillHistoDNLInformation(int minBin, int maxBin, const DNLValuesMap &dnlValuesMap);
    const std::vector<double> & getBinBoundaries() const {return m_binBoundaries;}
    bool hasBinBoundaries() const {return !m_binBoundaries.empty();}

    void fillBufferToHistoMap();

  public:
    static std::vector<double> calcStepBoarderFractions(const DataHistoVec & pixelHistograms, bool aboveThresh=true);

  private:
    uint32_t count;

    std::unordered_map<uint16_t,uint64_t> histoValues; // bin , count
    static const size_t BUFS = 512;// buffer for fast add, bin is the index, value the count of the bin
    uint64_t binValsBuffer[BUFS];
    std::vector<double> m_binBoundaries;
    DNLValuesMap m_dnlValuesMap;
};

// value occuring at most in vector
uint16_t calcModeOfVector(const std::vector<uint16_t> & values);

// value occuring at most in vector
template <typename ITER>
uint16_t calcModeOfVector(ITER *begin, ITER end)
{
  DataHisto histo;
  histo.addToBuf(begin,end);
  histo.fillBufferToHistoMap();
  histo.print();
  return histo.getMaxBin();
}

std::string computeBinningRangeStr(const DataHisto::DNLEvalMap & evalMap);

std::vector<double> calcDNLRMSValuesFromHistograms(const DataHistoVec & pixelHistograms);
std::vector<double> calcMeanImageFromHistograms(const DataHistoVec & pixelHistograms, const std::vector<uint32_t> & pixels);
std::vector<DataHisto::DNLValuesVec> calcDNLValuesFromHistograms(const DataHistoVec & pixelHistograms);
DataHisto::DNLEvalVec calcDNLEvalsFromHistograms(const std::vector<uint32_t> & pixels, const DataHistoVec & pixelHistograms);
void fillBufferToDataHistoVec(DataHistoVec & pixelHistograms);

StatsVec getMeandAndRMSVector(const DataHistoVec & pixelHistograms);
std::vector<double> getMeanVector(const DataHistoVec & pixelHistograms);
std::vector<double> getRMSVector(const DataHistoVec & pixelHistograms);
std::vector<double> calcPxDelaySteps(const std::vector<DataHistoVec> & histos);
std::vector<double> calcPxDelaySteps(int px, const DataHistoMap & histos);
std::vector<double> calcThreshAndGetFracBelowThresh(const DataHistoVec & histos);

FitResultsVec fitSpectra(DataHistoVec & pixelHistograms, const DataHisto::DNLEvalMap &dnlEvalMap = DataHisto::DNLEvalMap());
FitResultsMap fitSpectra(DataHistoMap & pixelHistograms, const DataHisto::DNLEvalMap &dnlEvalMap = DataHisto::DNLEvalMap());

std::vector<double> dnlValuesToSingleVec(const DataHisto::DNLValuesVec & dnlValues);
std::vector<double> dnlValuesToSingleVec(const DataHisto::DNLValuesMap & dnlValues);

DataHisto::DNLValuesVec singleVecToDNLValues(const std::vector<double> & singleVec);

template <typename TYPE>
void dumpFitResultsToASCII(const std::string & fileName, const std::vector<TYPE> & pixels, const FitResultsVec & fitResults)
{
  std::ofstream out(fileName);

  SpectrumFitResult::printHeader(out,positionVectorToList(pixels));

  int idx = 0;
  for(auto && res : fitResults){
    out << pixels[idx++] << "\t";
    res.print(out);
    out << "\n";
  }

  std::cout << "FitResult Export to " << fileName << " successful" << std::endl;
}

ADCGainFitResultsMap importADCGainFitResultsFromASCII(const std::string & fileName);
void dumpFitResultsToASCII(const std::string & fileName, const std::vector<uint32_t> & pixels, const ADCGainFitResultsVec & fitResults);
void dumpFitResultsToASCII(const std::string & fileName, const ADCGainFitResultsMap & fitResults);

void dumpInjectionCalibrationData(const std::string & fileName, uint32_t coarseGainSetting, const std::string & injectionMode, const InjectionCalibrationData & injectionCalibrationFactors);
std::pair<std::string,std::vector<double>> importInjectionCalibrationFactors(const std::string & fileName);
std::pair<std::string, InjectionCalibrationData> importInjectionCalibrationData(const std::string & fileName);

}

#endif // DATAHISTO_H
