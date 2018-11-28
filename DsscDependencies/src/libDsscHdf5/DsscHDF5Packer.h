#ifndef DSSCHDF5PACKER_H
#define DSSCHDF5PACKER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QPair>
#include <QProgressDialog>
#include "QAdvancedProgressBar.h"

#include <map>

#include "DsscTrainDataProcessor.h"
#include "DsscHDF5MeasurementInfoReader.h"
#include "DsscHDF5Writer.h"
#include "DataHisto.h"
#include "PPTFullConfig.h"
#include "ConfigReg.h"
#include "MultiModuleInterface.h"

namespace SuS {
  class QConfigReg;
  class Sequencer;
  class ChipData;
}
class DataPacker;


class DsscHDF5Packer : public QObject
{
  Q_OBJECT

  public:
    DsscHDF5Packer(const QString &exportFileName,
                   const QString & pixelsToExport,
                   const QString & fullConfigFilePath,
                   const QString & dirPath);

    DsscHDF5Packer(const QString & measurementInfoPath);

    using MeasurementSummary = QVector<QPair<QVector<double>,QVector<double> >>;

    enum SumDataIdx{PIXEL=0,SRAMMIN=1,SRAMMAX=2,SRAMMEAN=3,SRAMRMS=4,SRAMSLOPE=5,BURSTMIN=6,BURSTMAX=7,BURSTMEAN=8,BURSTRMS=9,BURSTSLOPE=10};

    enum SweepExportMode{ALL=0,SIG=1,BASE=2};

    ~DsscHDF5Packer();

    static bool s_batchModeActive;
    static bool s_loadBlacklist;
    static bool s_regenTrainIds;
    static bool s_genBlacklist;
    static bool s_genSramAndBurst;
    static bool s_loadSramCorrection;
    static bool s_exportInOne;

    static uint32_t s_blackListTrainIdMinPxCnt;

    static std::string s_columnSelection;

    static std::vector<uint64_t> updateTrainIDBlacklistSummary(const QString & fileName);

    QString modeStr();

    void openFile();
    void initFromFirstH5File(const std::string & fileName);

    bool exportToRoot(bool onlyMeanValues = false);
    void exportMeasurementToRoot(int idx, bool onlyMeanValues);
    void exportDirectoryToRoot(const QString  & exportDirPath);
    void exportMeansToRoot(const QString  & exportDirPath);

    void updateRootFileNameForMeans();

    bool exportToASCII();
    void exportDirectoryToASCII(const QString & exportDirPath, int setting);

    bool readFullConfig();
    QStringList loadAllH5FileNamesFromDirectory(const QString & exportDirPath);
    void generateCalibrationInfo();

    void initChipDataParams(std::vector<std::string> &paramNames, std::vector<int> & params);
    void generateInitialChipData();

    int getIntDACValue();
    static bool isPathRelative(const QString & fileNamePath);

    void emitUpdateStatus(const QString & status);
    void setSweepParameterName(const QString & name){ sweepParameterNames.clear();
                                                      sweepParameterNames.push_back(name);}
    void setNumPreBurstVetos(int numVetos){ numPreBurstVetos = numVetos;}

    QString getExportFileName() const {return expRootFileName;}

    static bool isLadderMode(const QString & dirPath);
    static void saveDsscHDF5ConfigFile(const QString &saveToFile, const QString & fullConfigPath);

    bool isValid() const {return configLoaded && !pixels.empty();}

    void exportMeasurementToDataHisto(int idx);
    QPair<QVector<double>, QVector<double> > exportDirectoryToDataHisto(const QString & exportDirPath);
    bool exportToDataHistos(bool startFitting);

    static std::vector<std::string> QListToVector(const QStringList & list);

    void loadBlacklists(const QString &settingDir = "");
    void loadSramCorrection(const QString &settingDir = "");

    void regenerateTrainIDBlacklistSummary(const QString &settingDir = "");
    void initPixelInjectionMode(bool signalNotBaseline, int addrSkipCnt, int offset);

    QString getLastExpFileName() const {return m_lastExpFileName;}

  signals:
    void updateStatus(QString,int);

  private:
    void initProgressDialog(int numFilesToPack);
    void removeBlacklistFiles(QStringList & filesToPack);

    void saveBlacklistsAndSummary(const QString & exportDirPath);
    void saveMeasurementSummary(const MeasurementSummary & measurementSummary);
    void saveDirectorySummaryData(const QString & baseDir);
    void generateBurstAndSramBlacklist(const QString & outputDir);
    void initBlacklistVariables();

    void loadPixelsToExport();
    void updateDataPackerPixelOffsetsMap();
    void saveAndClearPixelHistograms(const QString &settingStr, int setting = 0);
    void updateFullConfig(const QString & fileName);
    void loadGainParametersFromDir(const QString & exportDirPath);


    QString rootFileName;
    QString expRootFileName;
    QString fullConfigPath;
    QString baseDirectory;
    QString outputDirectory;
    QString measurementName;
    DataPacker * packer;
    SuS::PPTFullConfig m_fullConfig;
    SuS::ConfigReg *pixelRegisters;
    SuS::ConfigReg *jtagRegisters;
    SuS::ConfigReg *epcRegisters;
    SuS::ConfigReg *iobRegisters;

    SuS::Sequencer *sequencer;
    SuS::ChipData *chipData;

    QAdvancedProgressBar progress;

    QStringList directoryList;
    QStringList sweepParameterNames;
    QVector<QVector<int>> measurementSettings;
    int numIterations;
    int numPreBurstVetos;
    QVector<int> pixels;
    utils::PixelVector m_uintPixels;

    uint32_t totalNumPxs;
    int pixelRegsMaxModule;
    bool configLoaded;

    int m_numFrames;
    bool m_exportInOne;
    bool m_fitSpectra;
    utils::DataHistoVec m_pixelHistograms;
    std::array<std::vector<int>,16> m_tempADCVector;
    std::array<std::vector<uint64_t>,16> m_trainIDVector;

    bool m_trainIdBlacklistValid;
    std::vector<uint64_t> m_trainIdBlacklist;
    utils::DsscTrainDataProcessor m_trainDataProcessor;
    utils::DsscSramBlacklist * m_sramBlacklist;

    std::array<std::vector<double>,11> m_exportSummaryDataMap;
    std::map<QString,QVector<int>> m_exportRunPixelsMap;

    QString m_lastExpFileName;
    SweepExportMode m_sweepExportMode;

    QVector<QVector<double>> m_burstMeans;
    QVector<QVector<double>> m_burstMeansRMS;
    QVector<QVector<double>> m_meanSrams;
    size_t m_totalExpFiles;
    DsscHDF5MeasurementInfoReader infoReader;
};

#endif // DSSCHDF5PACKER_H
