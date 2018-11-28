#ifndef DSSCTRAINDATAPROCESSOR_H
#define DSSCTRAINDATAPROCESSOR_H

#include "DsscSramBlacklist.h"
#include "DsscTrainData.hh"
#include "DataHisto.h"

namespace utils{

static size_t s_gccWrap = 4;

class DsscTrainDataProcessor
{
  public:

    using DATAFORMAT = ConstPixelDataArray::DATAFORMAT;

    DsscTrainDataProcessor();
    virtual ~DsscTrainDataProcessor(){}

    void setParameters(const std::vector<uint32_t> & availableAsics, int numFrames, int minSram, int maxSram, DATAFORMAT format);
    inline void setInputFormat(DATAFORMAT format) {m_inputFormat = format;}

    inline void setSramBlacklist(const std::string & blacklistFileName){m_sramBlacklist.initFromFile(blacklistFileName);}
    inline bool isSramBlacklistValid() const {return m_sramBlacklist.isValid();}
    inline DsscSramBlacklist * getSramBlacklistPtr() {return &m_sramBlacklist;}
    inline void clearSramBlacklistData() { m_sramBlacklist.clear();}
    inline bool frameNotBlacklisted(int pixel, int frame) const {return m_sramBlacklist.getValidSramAddresses(pixel)[frame];}


    inline bool isSramCorrectionDataValid() const {return !m_sramCorrectionData.empty();}
    inline void setSramCorrectionData(const std::vector<std::vector<float>> & sramCorrection) { m_sramCorrectionData = sramCorrection;}
    inline void setBackgroundData(const std::vector<float> & backgroundData) { m_pixelBackgroundData = backgroundData;}
    inline void clearSramCorrectionData() { m_sramCorrectionData.clear(); m_pixelBackgroundData.clear();}

    const DsscTrainData &processPixelData(DsscTrainData *unsortedTrainData);
    const uint16_t *processPixelData(const uint16_t *unsortedTrainData, DATAFORMAT format);

    void processMeanValues(const DsscTrainData *trainData);
    void processMeanValues(const uint16_t *trainData, DATAFORMAT format);

    void fillMeanAccumulators(const DsscTrainData * trainData, const PixelVector & pixels);
    void fillMeanAccumulators(const uint16_t * trainDataPtr, DATAFORMAT format, const PixelVector & pixels);

    void calcMeanValues();
    void clearAccumulators();

    inline const StatsAccVec & getStatsAccVec() const {return m_statsAccVec;}

    static std::vector<double> computeSramSlopeVectorFromMeanSramMatrix(const std::vector<std::vector<double>> & meanSramMatrix);
    static std::vector<double> computeSramSlopeVectorFromMeanSramMatrix(const std::vector<std::vector<float>> & meanSramMatrixFloat);

    static std::vector<double> computeMeanSramRMSMatrix(const std::vector<std::vector<double>> & meanSramMatrix);
    static std::vector<double> computeMeanSramRMSMatrix(const std::vector<std::vector<float>> & meanSramMatrixFloat);

    void fillDataHistoVec(const DsscTrainData *inData, DataHistoVec & pixelHistos, bool fill);
    static void fillDataHistoVec(const DsscTrainData *inData, DataHistoVec & pixelHistos, const PixelVector & pixels, bool fill);
    static void fillDataHistoVec(const DsscTrainData *inData, DataHistoVec & pixelHistos, const DsscSramBlacklist & sramBlacklist, const PixelVector & pixels, bool fill);
    static void fillDataHistoVec(const DsscTrainData *trainData, DataHistoVec & pixelHistos, const std::vector<std::vector<float>>& sramCorrection, const PixelVector & pixels, bool fill);
    static void fillDataHistoVec(const DsscTrainData *trainData, DataHistoVec & pixelHistos, const DsscSramBlacklist & sramBlacklist, const std::vector<std::vector<float>>& sramCorrection, const PixelVector & pixels, bool fill);

    //pointer functions are used only in Karabo::DsscProcessor
    void fillDataHistoVec(const unsigned short* inDataPtr, DataHistoVec & pixelHistos, bool fill);

    static void sortDataArray(const DsscTrainData *inData,  DsscTrainData *outData, DsscTrainData::DATAFORMAT outFormat, const std::vector<std::vector<float>> & sramCorrectionMap);
    static void sortDataArray(const DsscTrainData *inData,  DsscTrainData *outData, DsscTrainData::DATAFORMAT outFormat);

    //pointer functions are used only in Karabo::DsscProcessor
    static void sortDataArray(const unsigned short * inData, DATAFORMAT inFormat, uint32_t numFrames,  DsscTrainData * outData, DATAFORMAT outFormat, const std::vector<std::vector<float> > &sramCorrectionMap);
    static void sortDataArray(const unsigned short *inData, DATAFORMAT inFormat, uint32_t numFrames,  DsscTrainData * outData, DATAFORMAT outFormat);
    static ConstPixelDataArray getDataSortIterator(const uint16_t *inData, DATAFORMAT inFormat, DATAFORMAT outFormat, uint32_t index1, uint32_t index2);

    // static sort functions used in HDF5TrainDataReader
    static void sortAsicToPixelWise(const unsigned short* inDataPtr,  unsigned short* out_data_ptr, const std::vector<uint32_t> & availableAsics, int numFrames);
    static void sortImageToPixelWise(const unsigned short* inDataPtr, unsigned short* out_data_ptr, const std::vector<uint32_t> & availableAsics, size_t numFrames, bool removeNotSending = false);
    static void sortPixelToPixelWise(const unsigned short* inDataPtr, unsigned short* outDataPtr,   const std::vector<uint32_t> & availableAsics, int numFrames);

    // correction functions to change pixel data during sorting
    static inline uint16_t checkGCCWrap(uint16_t in){ return (in<s_gccWrap? in + 256 : in); }

    static inline void applySramCorrection(uint16_t & out, uint16_t value, double correction){
      const uint16_t gccCorr = checkGCCWrap(value);
      out = std::max(0.0,std::min(round(gccCorr-correction),511.0));
    }

  protected:

    PixelVector getPixelsToProcess() const;

    inline int getNumAsicsToSort() const{
      return m_availableAsics.size();
    }

    inline std::vector<uint32_t> getAvailableAsics() const {
      return m_availableAsics;
    }

    inline bool sramCorrectionValid() const { return !m_sramCorrectionData.empty();}
    inline bool backgroundDataValid() const { return !m_pixelBackgroundData.empty();}

    DsscTrainData m_sortedTrainData;
    StatsAccVec m_statsAccVec;

    std::vector<double> m_pixelMeanData;
    std::vector<double> m_pixelRMSData;

    std::vector<unsigned int> m_availableAsics;
    PixelVector m_pixelsToProcess;

    bool m_alsoRMS;

    uint16_t m_minSram;
    uint16_t m_maxSram;

    unsigned int m_numFrames;
    unsigned int m_iterationCnt;
    unsigned int m_numIterations;

    DATAFORMAT m_inputFormat;

    DsscSramBlacklist m_sramBlacklist;
    std::vector<float> m_pixelBackgroundData;
    std::vector<std::vector<float> >m_sramCorrectionData;

};

}
#endif // DSSCTRAINDATAPROCESSOR_H
