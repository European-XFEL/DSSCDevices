#include <QDir>
#include <QStringList>
#include <QFile>
#include <QtGui>
#include <QFileInfo>
#include <QPushButton>
#include <QAbstractButton>
#include <QTextStream>
#include <QDebug>
#include <QMessageBox>
#include <QCoreApplication>
#include <QEventLoop>

#include <TH1I.h>
#include <TQtWidget.h>
#include <TStyle.h>

#include "utils.h"
#include "DataHisto.h"
#include "DataPacker.h"
#include "DataAnalyzer.h"
#include "QConfigReg.h"
#include "QConfigRegWidget.h"
#include "Sequencer.h"
#include "ChipData.h"

#include "RootTreeHandlerWidget.h"
#include "SelASICPixelsWidget.h"
#include "DsscHDF5Packer.h"
#include "DsscHDF5TrainDataReader.h"
#include "DsscHDF5MeanRMSReader.h"
#include "DsscHDF5TrimmingDataWriter.h"
#include "CalibrationDataGenerator.h"

using namespace std;
using namespace SuS;

bool DsscHDF5Packer::s_batchModeActive    = false;
bool DsscHDF5Packer::s_loadBlacklist      = false;
bool DsscHDF5Packer::s_loadSramCorrection = false;
bool DsscHDF5Packer::s_genSramAndBurst    = false;
bool DsscHDF5Packer::s_regenTrainIds      = false;
bool DsscHDF5Packer::s_genBlacklist       = false;
bool DsscHDF5Packer::s_exportInOne        = false;

uint32_t DsscHDF5Packer::s_blackListTrainIdMinPxCnt = 500;
std::string DsscHDF5Packer::s_columnSelection = "";

DsscHDF5Packer::DsscHDF5Packer(const QString & exportFileName, const QString &pixelsToExport,
                               const QString &fullConfigFilePath, const QString &dirPath)
  : rootFileName(exportFileName),fullConfigPath(fullConfigFilePath), baseDirectory(dirPath),
  packer(nullptr),
  pixelRegisters(nullptr),jtagRegisters(nullptr),epcRegisters(nullptr),iobRegisters(nullptr),
  sequencer(nullptr),
  chipData(nullptr),
  numIterations(0),
  numPreBurstVetos(-1),
  pixels(RootTreeHandlerWidget::PositionListToVector(pixelsToExport)),
  m_uintPixels(utils::convertVectorType<int,uint32_t>(pixels.toStdVector())),
  pixelRegsMaxModule(0),
  configLoaded(false),
  m_trainIdBlacklistValid(false),
  m_sramBlacklist(m_trainDataProcessor.getSramBlacklistPtr()),
  m_sweepExportMode(SweepExportMode::ALL)
{
  progress.hide();
  measurementName = "BurstMeasurement";
  sweepParameterNames.push_back(measurementName);
  measurementSettings.push_back(QVector<int>(1,0));

  saveDsscHDF5ConfigFile(baseDirectory + "/ConfigExport.h5",fullConfigPath);

  readFullConfig();

  DataPacker::updatePixelOffsetsMap(pixels,isLadderMode(baseDirectory));

  if(isValid()){
    qDebug() << "+++HDF5 Packer ready for conversion";
  }
}


DsscHDF5Packer::DsscHDF5Packer(const QString & measurementInfoPath)
: packer(nullptr),
  pixelRegisters(nullptr),jtagRegisters(nullptr),epcRegisters(nullptr),iobRegisters(nullptr),
  sequencer(nullptr),
  chipData(nullptr),
  numIterations(0),
  numPreBurstVetos(-1),
  pixelRegsMaxModule(0),
  configLoaded(false),
  m_trainIdBlacklistValid(false),
  m_sramBlacklist(m_trainDataProcessor.getSramBlacklistPtr()),
  m_sweepExportMode(SweepExportMode::ALL),
  infoReader(measurementInfoPath.toStdString())
{
  if(!infoReader.isValid()) return;

  progress.hide();
  baseDirectory = QString::fromStdString(infoReader.getBaseDirectory());
  fullConfigPath = QString::fromStdString(infoReader.getConfigFilePath());
  rootFileName = QString::fromStdString(infoReader.getRootFileName());
  expRootFileName = rootFileName;
  measurementName = QString::fromStdString(infoReader.getMeasurementName());

  auto measurementParams = infoReader.getMeasurementParams();
  for(const auto & param : measurementParams){
    sweepParameterNames << QString::fromStdString(param);
  }

  for(int i=0; i<sweepParameterNames.size(); i++){
    measurementSettings.push_back(QVector<int>::fromStdVector(infoReader.getMeasurementSettings(i)));
  }

  auto exportDirectories = infoReader.getMeasurementPaths();

  qDebug() << "Measurement Directories found:";
  for(const auto & exportDir : exportDirectories){
    QString nextDir = QString::fromStdString(exportDir);
    directoryList << nextDir;
    qDebug() << nextDir;
  }
  qDebug() << "---";

  numIterations = infoReader.getNumIterations();
  numPreBurstVetos = infoReader.getNumPreBurstVetos();

  readFullConfig();

  m_trainDataProcessor.setParameters(infoReader.getAvailableASICs(),utils::s_numSram,0,utils::s_numSram-1,utils::DsscTrainData::DATAFORMAT::IMAGE);

  if(isValid()){
    qDebug() << "+++HDF5 Packer ready for conversion";
  }
}


DsscHDF5Packer::~DsscHDF5Packer()
{
  if(packer)   delete packer;
  if(chipData) delete chipData;
}

std::vector<uint64_t> DsscHDF5Packer::updateTrainIDBlacklistSummary(const QString & fileName)
{
  std::vector<uint64_t> allTrainIds;
  const auto badPxTrainIdxs = utils::importPxTrainIDBlacklistSummary(fileName.toStdString(),allTrainIds);

  utils::DataHisto badTrainIdsHisto;

  for(auto && pxTrainIdxs : badPxTrainIdxs){
    for(auto && badTrainID : pxTrainIdxs){
      badTrainIdsHisto.add(badTrainID);
    }
  }
  badTrainIdsHisto.print();

  const auto bins = badTrainIdsHisto.getBins();
  const auto binValues = badTrainIdsHisto.getBinValues();
  size_t numBins = bins.size();

  auto rootHisto = DataAnalyzer::toRootHisto<TH1I>(badTrainIdsHisto,"BadTrainIdx Counts");
  TQtWidget plotWidget;

  plotWidget.GetCanvas()->cd();
  plotWidget.GetCanvas()->Clear();

  plotWidget.GetCanvas()->SetLogy(1);
  plotWidget.GetCanvas()->Update();

  int width = std::min(10000.0,bins.size()*1.1);
  plotWidget.resize(width,800);
  plotWidget.show();

  gStyle->SetOptStat(0000);

  rootHisto.SetTitle("Bad Train Idx - PixelCount");
  rootHisto.SetLineWidth(0);

  rootHisto.GetXaxis()->SetTitle("Train Index");
  rootHisto.GetYaxis()->SetTitle("Bad Train Count");
  rootHisto.GetXaxis()->SetTitleOffset(1.2);
  rootHisto.GetYaxis()->SetTitleOffset(1.2);
  rootHisto.SetFillColor(kBlue+1);
  rootHisto.DrawCopy();

  const auto outputDir = utils::getFilePath(fileName.toStdString());
  plotWidget.GetCanvas()->SaveAs((outputDir + "/BadTrainIDsHisto.png").c_str());

  bool ok;
  uint32_t newLimit = QInputDialog::getInt(nullptr, tr("Get PixelCount Limit for Bad Trains"),
                               tr("Value:"),s_blackListTrainIdMinPxCnt,1, 65536, 1, &ok);
  if (ok){
     s_blackListTrainIdMinPxCnt = newLimit;
  }


  std::vector<uint64_t> trainIdBlacklist;
  std::map<uint32_t,uint32_t> outlierCounts;
  for(size_t i=0; i<numBins; i++){
    if(binValues[i]>s_blackListTrainIdMinPxCnt){
      outlierCounts[bins[i]] = binValues[i];
      trainIdBlacklist.push_back(allTrainIds[bins[i]]);
    }
  }

  QString outfileName = fileName;
  outfileName.remove("Overview");
  utils::saveTrainIDBlacklist(outfileName.toStdString(),outlierCounts,allTrainIds);

  return trainIdBlacklist;
}


void DsscHDF5Packer::regenerateTrainIDBlacklistSummary(const QString & settingDir)
{
  m_trainIdBlacklistValid = false;
  QDir exportDir(baseDirectory);
  if(!exportDir.exists()){
    return;
  }

  QStringList selectList;
  if(settingDir.isEmpty()){
    selectList << "*PxOutlierTrainIdsOverview.txt";
  }else{
    selectList << ("*" + settingDir + "*PxOutlierTrainIdsOverview.txt");
  }
  QStringList trainIdSummaryFiles = exportDir.entryList(selectList,QDir::Files,QDir::Name);

  if(!trainIdSummaryFiles.empty())
  {
    if(settingDir.isEmpty()){
      m_trainIdBlacklist = updateTrainIDBlacklistSummary(baseDirectory+"/"+trainIdSummaryFiles.front());
      m_trainIdBlacklistValid = true;
      qDebug() << "TrainId Blacklist generated from " << baseDirectory+"/"+trainIdSummaryFiles.front();
    }else{
      for(auto && trainIdOverviewFile : trainIdSummaryFiles){
        const auto currentBlacklist = updateTrainIDBlacklistSummary(baseDirectory+"/"+trainIdOverviewFile);
        if(trainIdOverviewFile.contains(settingDir)){
          m_trainIdBlacklist = currentBlacklist;
          m_trainIdBlacklistValid = true;
        }
        qDebug() << "TrainId Blacklist generated from " << baseDirectory+"/"+trainIdOverviewFile;
      }
    }
  }
}


void DsscHDF5Packer::initPixelInjectionMode(bool signalNotBaseline, int addrSkipCnt, int offset)
{
  m_sweepExportMode = signalNotBaseline? SweepExportMode::SIG : SweepExportMode::BASE;
  m_sramBlacklist->initForPixelInjection(offset,addrSkipCnt,signalNotBaseline);
}

void DsscHDF5Packer::loadSramCorrection(const QString & settingDir)
{
  if(baseDirectory.isEmpty()) return;

  QStringList selectList;
  if(settingDir.isEmpty()){
    selectList << "*SramCorrectionData.h5";
  }else{
    selectList << ("*" + settingDir + "*SramCorrectionData.h5");
  }

  QDir exportDir(baseDirectory);
  const QStringList sramCorrectionFiles = exportDir.entryList(selectList,QDir::Files,QDir::Name);

  if(!sramCorrectionFiles.empty())
  {
    const std::string sramCorrectionFile = sramCorrectionFiles.front().toStdString();
    DsscHDF5CorrectionFileReader correctionReader(sramCorrectionFile);
    if(correctionReader.isValid()){
       std::vector<float> baselineValues;
       std::vector<std::vector<float> > sramCorrectionValues;
       correctionReader.loadCorrectionData(baselineValues,sramCorrectionValues);
       m_trainDataProcessor.setSramCorrectionData(sramCorrectionValues);
       m_trainDataProcessor.setBackgroundData(baselineValues);
    }
    qDebug() << "DsscHDF5Packer:: SramCorrectionData loaded from: " << sramCorrectionFiles.front();
  }
  else
  {
    qDebug() << "Warning, no SramCorrection file found in :" << baseDirectory;
  }
}


void DsscHDF5Packer::loadBlacklists(const QString & settingDir)
{
  if(baseDirectory.isEmpty()) return;
  {
    QDir exportDir(baseDirectory);
    if(!exportDir.exists()){
      return;
    }

    QStringList selectList;
    if(m_sweepExportMode == SweepExportMode::BASE){
      selectList << ("*base_PxSramOutliers.txt");
    }else if(m_sweepExportMode == SweepExportMode::SIG){
      selectList << ("*sig_PxSramOutliers.txt");
    }else{
      if(settingDir.isEmpty()){
        selectList << "*PxSramOutliers.txt";
      }else{
        selectList << ("*" + settingDir + "*PxSramOutliers.txt");
        selectList << ("*Global*PxSramOutliers.txt");
      }
    }
    QStringList sramOutlierFiles = exportDir.entryList(selectList,QDir::Files,QDir::Name);

    if(!sramOutlierFiles.empty())
    {
      m_sramBlacklist->initFromFile((baseDirectory+"/"+sramOutlierFiles.front()).toStdString());
    }
  }

  {
    m_trainIdBlacklistValid = false;
    QDir exportDir(baseDirectory);
    if(!exportDir.exists()){
      return;
    }

    QStringList selectList;
    if(m_sweepExportMode == SweepExportMode::BASE){
      selectList << ("*base_PxOutlierTrainIds.txt");
    }else if(m_sweepExportMode == SweepExportMode::SIG){
      selectList << ("*base_PxOutlierTrainIds.txt");
    }else{
      if(settingDir.isEmpty()){
        selectList << "*base_PxOutlierTrainIds.txt";
        selectList << "*all_PxOutlierTrainIds.txt";
      }else{
        selectList << ("*" + settingDir + "_base_PxOutlierTrainIds.txt");
        selectList << ("*" + settingDir + "_all_PxOutlierTrainIds.txt");
      }
    }
    QStringList trainIdOutlierFiles = exportDir.entryList(selectList,QDir::Files,QDir::Name);

    if(!trainIdOutlierFiles.empty())
    {
      m_trainIdBlacklist = utils::importTrainIDBlacklist((baseDirectory+"/"+trainIdOutlierFiles.front()).toStdString());
      m_trainIdBlacklistValid = true;
      qDebug() << "TrainId Blacklist Loaded from " << baseDirectory+"/"+trainIdOutlierFiles.front();
    }
  }
}


bool DsscHDF5Packer::isPathRelative(const QString & fileNamePath)
{
  return (fileNamePath.left(1)!="/");
}


void DsscHDF5Packer::saveDsscHDF5ConfigFile(const QString & saveToFile, const QString & fullConfigPath)
{
  const auto h5ConfigData = SuS::MultiModuleInterface::getHDF5ConfigData(fullConfigPath.toStdString());

  DsscHDF5Writer::saveConfiguration(saveToFile.toStdString(),h5ConfigData);
}


void DsscHDF5Packer::updateDataPackerPixelOffsetsMap()
{
  DataPacker::updatePixelOffsetsMap(pixels,infoReader.isLadderMode());
}

//complicated routine to generate pixels to export
// - single ASIC Mode and LAdder Mode supported
// - column selection possible for injection sweeps or other measurments
// - col0-5 or colskip0-63:8#4

void DsscHDF5Packer::loadPixelsToExport()
{
  if(!configLoaded){
    qCritical() << "Can not load pixels, configuration file not loaded";
    pixels.clear();
    return;
  }

  QVector<int> asics;

  QString selAsicPixelsStr = "0-4096";
  QString selAsicColumns = "all";

  int maxConfigPixel = stoi(pixelRegisters->getModules("Control register").back());
  bool ladderMode = maxConfigPixel > 4096;
  const auto availableASICs = infoReader.getAvailableASICs();

  std::string columnSelection = s_columnSelection.empty() ? infoReader.getColumnSelection() : s_columnSelection;

  cout << "ColumnSelection = " << columnSelection << endl;
  QString asicsStr = "0";
  if(!s_batchModeActive){
    if(infoReader.isLadderMode()){
      if(!ladderMode){
        selAsicPixelsStr = QString::fromStdString(pixelRegisters->getModuleNumberList("Control register"));
      }else{
        const auto asicsAndPixels = SelASICPixelsWidget::selectASICsAndPixels(availableASICs,columnSelection);
        asicsStr = asicsAndPixels[0];
        selAsicPixelsStr = asicsAndPixels[1];
        selAsicColumns = asicsAndPixels[2];
      }
    }else{
      if(!ladderMode){
        selAsicPixelsStr = QString::fromStdString(pixelRegisters->getModuleNumberList("Control register"));
      }else{
        const auto asicsAndPixels = SelASICPixelsWidget::selectASICsAndPixels(availableASICs,columnSelection);
        asicsStr = asicsAndPixels[0];
        selAsicPixelsStr = asicsAndPixels[1];
        selAsicColumns = asicsAndPixels[2];
      }
    }
  }else{
    asicsStr = "all";
    selAsicPixelsStr = "all";
    selAsicColumns = QString::fromStdString(columnSelection);
  }


  if(asicsStr.left(1) == "a"){
    asicsStr = "0-15";
  }

  if(selAsicPixelsStr.left(1) == "a"){
    selAsicPixelsStr = "0-4095";
  }

  if(selAsicColumns.left(1) == "a"){
    selAsicColumns = "0-63";
  }

  qDebug() << "AsicStr = " << asicsStr;
  qDebug() << "AsicPixelsStr = " << selAsicPixelsStr;
  qDebug() << "AsicColumns = " << selAsicColumns;

  if(selAsicPixelsStr.isEmpty()){
    configLoaded = false;
    qCritical() << "ERROR: no pixels selected";
    return;
  }

  if(asicsStr == "Manual")
  {
    m_exportRunPixelsMap.clear();
    pixels = QConfigRegWidget::positionListToVector<int>(selAsicPixelsStr);
    qSort(pixels);
    m_uintPixels = utils::convertVectorType<int,uint32_t>(pixels.toStdVector());

    const size_t totalNumPixels = pixels.size();
    const size_t expBatchSize = 8192;
    QString key = "ManualPxSel";
    if(totalNumPixels<expBatchSize){
      m_exportRunPixelsMap[key] = pixels;
    }else{
      size_t pxOffs = 0;
      int run = 0;
      while(pxOffs < totalNumPixels)
      {
        size_t thisRunPxCount = std::min(totalNumPixels-pxOffs,expBatchSize);
        QVector<int> currentRunPixels(thisRunPxCount);
        std::copy(pixels.begin()+pxOffs,pixels.begin()+pxOffs+thisRunPxCount,currentRunPixels.begin());

        QString runKey = key + "_PART" + QString::number(run);
        m_exportRunPixelsMap[runKey] = currentRunPixels;
        pxOffs += 8192;
        run++;
      }
    }
  }
  else
  {
    asics = QConfigRegWidget::positionListToVector<int>(asicsStr);
    pixels = QVector<int>::fromStdVector( utils::convertVectorType<uint32_t,int>(SelASICPixelsWidget::getASICPixels(asicsStr,selAsicPixelsStr).toStdVector()));
    m_uintPixels = utils::convertVectorType<int,uint32_t>(pixels.toStdVector());

    m_exportRunPixelsMap.clear();
    if(selAsicColumns.left(3) == "col"){
      auto uintPixels = utils::getSendingChipPartPixels(asics.toStdVector(),selAsicColumns.toStdString(),ladderMode);
      pixels = QVector<int>::fromStdVector(utils::convertVectorType<uint32_t,int>(uintPixels));
      m_uintPixels = utils::convertVectorType<int,uint32_t>(pixels.toStdVector());

      const size_t totalNumPixels = pixels.size();
      const size_t expBatchSize = 8192;
      QString key = selAsicColumns;
      if(totalNumPixels<expBatchSize){
        m_exportRunPixelsMap[key] = pixels;
      }else{
        size_t pxOffs = 0;
        int run = 0;
        while(pxOffs < totalNumPixels)
        {
          size_t thisRunPxCount = std::min(totalNumPixels-pxOffs,expBatchSize);
          QVector<int> currentRunPixels(thisRunPxCount);
          std::copy(pixels.begin()+pxOffs,pixels.begin()+pxOffs+thisRunPxCount,currentRunPixels.begin());

          QString runKey = key + "_PART" + QString::number(run);
          m_exportRunPixelsMap[runKey] = currentRunPixels;
          pxOffs += 8192;
          run++;
        }
      }
    }else if(asics.size() < 2){
      QString key = "ASICS" + asicsStr;
      m_exportRunPixelsMap[key] = pixels;
    }else{
      for(auto && asic : asics){
        QString key = "ASIC" + QString::number(asic);
        auto runPixels = QVector<int>::fromStdVector( utils::convertVectorType<uint32_t,int>(SelASICPixelsWidget::getASICPixels(QString::number(asic),selAsicPixelsStr).toStdVector()));
        m_exportRunPixelsMap[key] = runPixels;
      }
    }
  }
}


bool DsscHDF5Packer::isLadderMode(const QString & dirPath)
{
  if(dirPath.isEmpty()) {
    cout << "Dirpath is Empty: " << dirPath.toStdString() << endl;
    return false;
  }

  QDir exportDir(dirPath);
  const auto filesToPack = exportDir.entryList({"*.h5"},QDir::Files,QDir::Name);

  if(filesToPack.isEmpty()){
    cout << "files to Pack is empty: "  << dirPath.toStdString() << endl;
    return false;
  }
  QString fileName = "";
  for(const auto & name : filesToPack){
    if(name.indexOf("Config")<0){
      fileName = name;
      break;
    }
  }

  DsscHDF5TrainDataReader reader((dirPath + "/" +fileName).toStdString());
  return reader.isLadderMode();
}


void DsscHDF5Packer::loadGainParametersFromDir(const QString & exportDirPath)
{
  const QString configFileName = (exportDirPath + "/MeasurementConfig.conf");
  if(QFile::exists(configFileName)){
    updateFullConfig(configFileName);
  }else{
    std::string paramName = utils::getFileName(exportDirPath.toStdString());
    int paramValue = utils::extractNumberFromString(paramName);
    if(paramName == SuS::DsscGainParamMap::s_gainModeParamNames[0] ||
       paramName == SuS::DsscGainParamMap::s_gainModeParamNames[1] ||
       paramName == SuS::DsscGainParamMap::s_gainModeParamNames[2] ||
       paramName == "RmpFineTrm"){
      pixelRegisters->setSignalValue("Control register","all",paramName,paramValue);
    }else if(paramName == "IntegrationTime"){
      sequencer->setIntegrationTime(paramValue);
    }
  }
}


void DsscHDF5Packer::updateFullConfig(const QString & fileName)
{
  m_fullConfig.loadFullConfig(fileName.toStdString());

  pixelRegisters = m_fullConfig.getPixelReg(0);
  jtagRegisters  = m_fullConfig.getJtagReg(0);
  //epcRegisters   = new ConfigReg(epcRegsFileName);
  //iobRegisters   = new ConfigReg(iobRegsFileName);

  sequencer = m_fullConfig.getSequencer();
  configLoaded = true;
}

bool DsscHDF5Packer::readFullConfig()
{
  configLoaded = false;
  emitUpdateStatus("Read Configuration");

  updateFullConfig(fullConfigPath);

  if(pixelRegisters->isLoaded())
  {
    cout << "Loaded" << m_fullConfig.getSequencerFileName()<< endl;

    loadPixelsToExport();

    const auto pixelRegPixels = pixelRegisters->getModuleNumbers("Control register");
    int pixelRegsMaxModule = *std::max_element(pixelRegPixels.begin(),pixelRegPixels.end());
    int maxPixelToExport   = *std::max_element(pixels.begin(),pixels.end());
    if(maxPixelToExport > pixelRegsMaxModule){
      cout << "#### ERROR pixel register file: " << m_fullConfig.getPixelRegsFileName() << " pixels don't fit to export pixel range";
      cout << "Max Pixel to export = " << maxPixelToExport << " pixelRegsMaxModule = " << pixelRegsMaxModule;
      return configLoaded;
    }

    totalNumPxs = (pixelRegsMaxModule+1);
  }else{
    configLoaded = false;
    cout <<  "#### ERROR Could not load pixel register file: " << m_fullConfig.getPixelRegsFileName()<< endl;
    exit(0);
  }

  if(jtagRegisters->isLoaded())
    cout <<  "Loaded" << m_fullConfig.getJtagRegsFileName();
  else{
    configLoaded = false;
    cout << "#### ERROR Could not load jtag register file: " << m_fullConfig.getJtagRegsFileName() << endl;
  }
/*
  if(epcRegisters->isLoaded())
    cout << "Loaded" << epcRegsFileName << endl;
  else{
    //configLoaded = false;
    cout << "#### ERROR Could not load epc register file: " <<   m_fullConfig.getEPCRegsFileName() << endl;
  }
  if(iobRegisters->isLoaded())
    cout << "Loaded" << iobRegsFileName << endl;
  else{
    //configLoaded = false;
    cout << "#### ERROR Could not load iob register file: " <<   m_fullConfig.getIOBRegsFileName() << endl;
  }
*/
  if(sequencer->configGood)
    cout << "Loaded" << m_fullConfig.getSequencerFileName() << endl;
  else{
    configLoaded = false;
    cout << "#### ERROR Could not load sequencer file: " << m_fullConfig.getSequencerFileName() << endl;
  }
  return configLoaded;
}

void DsscHDF5Packer::updateRootFileNameForMeans()
{
  if(rootFileName.indexOf("means.root") >=0) return;
  rootFileName.insert(rootFileName.length()-5,"_means");
}


void DsscHDF5Packer::initProgressDialog(int numFilesToPack)
{
  progress.setLabelText("Exporting Data");
  progress.setRange(0,numFilesToPack);
  progress.setWindowTitle("HDF5 To Root Converter");
  progress.setWindowModality(Qt::WindowModal);
  progress.setValue(0);
  progress.show();
}



bool DsscHDF5Packer::exportToDataHistos(bool startFitting)
{
  if(!configLoaded){
    qDebug() << "Error: no config loaded";
    return false;
  }
  qDebug() << "Exporting" << baseDirectory << "into" << rootFileName;

  if(s_regenTrainIds){
    regenerateTrainIDBlacklistSummary();
  }

  m_exportInOne = s_exportInOne;
  m_fitSpectra  = startFitting;

  int numMeasurements = 1;
  if(sweepParameterNames.size() > 1){
    numMeasurements = measurementSettings[1].size();
  }

  m_pixelHistograms.resize(pixels.size());

  int numSettings = measurementSettings[0].size();
  int numFilesToPack = numIterations*numMeasurements*numSettings;
  initProgressDialog(numFilesToPack);

  if((numMeasurements * numSettings )> 1){
    if(!s_batchModeActive){
      QMessageBox msgBox;
      msgBox.setWindowTitle("HDF5 To DataHisto Export");
      msgBox.setText("Mulit Directory Measurement Found.\nShould all data be filled into the same histogram?");
      /*QPushButton *rootButton  =*/ msgBox.addButton(tr("Export in one Spectrum"), QMessageBox::ActionRole);
      auto multiButton = msgBox.addButton(tr("Export each Directory in one Spectrum"), QMessageBox::ActionRole);
      auto abortButton = msgBox.addButton(QMessageBox::Abort);
      msgBox.exec();

      if (msgBox.clickedButton() == abortButton) {
        return false;
      }
      m_exportInOne = msgBox.clickedButton() != multiButton;
    }
  }else{
    m_exportInOne = true;
  }

  if(m_exportInOne){
    cout << "+++Export all setting measurements into one common histogram" << endl;
  }else{
    cout << "+++Export each setting measurement into one separate histogram" << endl;
  }

  if(s_genSramAndBurst || s_genBlacklist){
    initBlacklistVariables();
  }

  if(numMeasurements == 1)
  {
    m_exportInOne = false;
    exportMeasurementToDataHisto(0);
  }
  else
  {
    int dirNum = 0;
    for(int set=0; set<measurementSettings[1].size(); set++)
    {
      exportMeasurementToDataHisto(dirNum);

      dirNum+=numSettings;
      if(progress.wasCanceled()) break;
    }
  }

  if(m_exportInOne){
    saveBlacklistsAndSummary(baseDirectory + "/Combined");

    saveAndClearPixelHistograms(measurementName);
  }

  progress.hide();
  return true;
}


void DsscHDF5Packer::initBlacklistVariables()
{
  const size_t numPixels = pixels.size();
  m_burstMeans = QVector<QVector<double>>(numPixels);
  m_burstMeansRMS = QVector<QVector<double>>(numPixels);
  m_meanSrams = std::move(QVector<QVector<double>>(numPixels,QVector<double>(800,0.0)));
  m_totalExpFiles = 0;
}

QString DsscHDF5Packer::modeStr()
{
  QString fill = "";
  if(m_sramBlacklist->isValid() || m_trainIdBlacklistValid){
    fill += "_corr";
  }
  if(m_sweepExportMode == SweepExportMode::BASE){
    fill += "_base";
  }else if(m_sweepExportMode == SweepExportMode::SIG){
    fill += "_sig";
  }else{
    fill += "_all";
  }
  return fill;
}


void DsscHDF5Packer::saveAndClearPixelHistograms(const QString & settingStr, int setting)
{
  const size_t numPixels = m_uintPixels.size();

  const int selASIC = utils::getPixelASIC(pixels.toStdVector().front());
  const auto & trainIDs = m_trainIDVector[selASIC];

#pragma omp parallel for
  for(size_t pxIdx=0; pxIdx<numPixels; pxIdx++){
    m_pixelHistograms[pxIdx].fillBufferToHistoMap();
  }

  outputDirectory = baseDirectory + "/HistogramAnalysis";
  utils::makePath(outputDirectory.toStdString());

  QString fill = modeStr();

  std::string exportFileName = (outputDirectory+ "/HistogramExport"+fill+"_"+settingStr+".txt").toStdString();
  qDebug() << "Filename" << QString::fromStdString(exportFileName);
  utils::DataHisto::dumpHistogramsToASCII(exportFileName,m_uintPixels,m_pixelHistograms,setting);

  m_lastExpFileName = outputDirectory + "/SpektrumExport"+fill+"_"+settingStr+".h5";
  DsscHDF5TrimmingDataWriter dataWriter(m_lastExpFileName.toStdString());
  dataWriter.setMeasurementName(measurementName.toStdString());
  dataWriter.addCurrentTargetInfo();

  dataWriter.addGainParamsMap(SuS::CalibrationDataGenerator::getGainParamsMap(pixelRegisters,sequencer->getIntegrationTime()));
  dataWriter.addVectorData("RmpFineTrmSettings",pixelRegisters->getSignalValues("Control register","all","RmpFineTrm"));

  dataWriter.addValueData("NumberOfTrains",trainIDs.size());
  dataWriter.addVectorData("ContainedTrainIds",utils::convertVectorType<uint64_t,double>(trainIDs));

  if(m_fitSpectra){
    const std::string fitResultsFilePath = (outputDirectory+"/SpectrumFitResults"+fill+"_"+settingStr).toStdString();
    const auto fitResults = utils::fitSpectra(m_pixelHistograms);

    DsscHDF5TrimmingDataWriter fitResultsWriter(fitResultsFilePath);
    fitResultsWriter.addCurrentTargetInfo();
    fitResultsWriter.addFitResultData("SpectrumFitResults",fitResults);

    utils::dumpFitResultsToASCII(fitResultsFilePath,m_uintPixels,fitResults);

    DataAnalyzer::createSpectrumFitResultsOutputs(outputDirectory,fitResults,pixels);
  }

  dataWriter.addHistoData("SpectrumHistos",m_pixelHistograms,m_uintPixels);
  m_pixelHistograms.clear();
  m_pixelHistograms.resize(pixels.size());

  for(auto && trainIDVec : m_trainIDVector){
    trainIDVec.clear();
  }
}


bool DsscHDF5Packer::exportToRoot(bool onlyMeanValues)
{
  if(!configLoaded){
    qDebug() << "Error: no config loaded";
    return false;
  }

  if(s_regenTrainIds){
    regenerateTrainIDBlacklistSummary();
  }

  if(s_loadBlacklist){
    loadBlacklists();
  }

  if(onlyMeanValues) updateRootFileNameForMeans();

  qDebug() << "Exporting" << baseDirectory << "into" << rootFileName;

  int numMeasurements = 1;
  if(sweepParameterNames.size() > 1){
    numMeasurements = measurementSettings[1].size();
  }

  for(auto && runElem : m_exportRunPixelsMap) qDebug() << "#### Export ChipPart: " << runElem.first << " pixels = " << QConfigRegWidget::positionVectorToList(runElem.second).left(200);
  for(auto && runElem : m_exportRunPixelsMap){

    if(packer) delete packer;

    pixels = runElem.second;
    m_uintPixels = utils::convertVectorType<int,uint32_t>(pixels.toStdVector());

    updateDataPackerPixelOffsetsMap();

    expRootFileName = rootFileName;
    expRootFileName.insert(rootFileName.size()-5,"_" + runElem.first);
    expRootFileName.replace("#","_");

    qDebug() << "#### Export Measurement to RootFile: " << expRootFileName;

    packer = new DataPacker(expRootFileName,pixels,numMeasurements,"DSSC_HDF5");
    packer->setMeasurementParam(measurementName,numIterations,numPreBurstVetos,sweepParameterNames.front());

    generateInitialChipData();

    packer->setCurrMeasurement(0);

    int numSettings = measurementSettings[0].size();
    int numFilesToPack = numIterations*numMeasurements;
    if(!onlyMeanValues) numFilesToPack*=numSettings;

    initProgressDialog(numFilesToPack);

    if(numMeasurements == 1){
      packer->setCurrMeasurement(0);
      exportMeasurementToRoot(0,onlyMeanValues);
    }else{
      const auto measurementParamName = sweepParameterNames[1].toStdString();
      int meas = 0;
      int dirNum = 0;
      packer->setMeasurementSettingName(QString::fromStdString(measurementParamName));
      for(const auto & setting : measurementSettings[1] )
      {
        packer->setCurrMeasurement(meas++,setting);
        packer->currChipData->setParamValue(measurementParamName,setting);
        exportMeasurementToRoot(dirNum,onlyMeanValues);
        dirNum+=numSettings;
        if(progress.wasCanceled()) break;
      }
    }
    packer->write();
    if(progress.wasCanceled()) break;
  }

  progress.hide();
  return true;
}


void DsscHDF5Packer::exportMeasurementToRoot(int idx, bool onlyMeanValues)
{
  const auto paramName = sweepParameterNames[0].toStdString();
  const auto settings = measurementSettings[0];

  if(settings.size() + idx > directoryList.size()){
    throw "Error: directory list smaller than expected";
  }

  for(const auto & setting : settings){
    packer->currChipData->setParamValue(paramName,setting);
    packer->currChipData->changeSetting(setting);
    const auto measDirectory = baseDirectory + "/" + directoryList[idx++];
    if(onlyMeanValues){
      exportMeansToRoot(measDirectory);
    }else{
      exportDirectoryToRoot(measDirectory);
    }
    if(progress.wasCanceled()){
      qWarning() << "User abort";
      break;
    }
  }

  emitUpdateStatus("Writing data...");
  qDebug() << "---> Data Packer Done - Export successful <---";
}



void DsscHDF5Packer::exportMeansToRoot(const QString & exportDirPath)
{
  emitUpdateStatus("Start Export");

  QString fileName = (exportDirPath + "/TrainMeanRMSImage.h5");
  try{
    DsscHDF5MeanRMSReader reader(fileName.toStdString());
    auto pixelData = reader.getData(pixels.toStdVector());
    if(pixelData){
      packer->updateMetaData(0,reader.getTrainID());
      packer->addCurrentPixelsBurst(pixelData);
    }else{
      qDebug() << "HDF5DataPacker: pixel Data invalid";
    }
  }
  catch (std::exception & e)
  {
    qDebug() << "Error could not convert file" << fileName;
    qDebug() << e.what();
  }

  progress.setValue(progress.value()+1);

  emitUpdateStatus(fileName + "...done");
}


std::vector<std::string> DsscHDF5Packer::QListToVector(const QStringList & list)
{
  std::vector<std::string> vector;
  for(auto && item : list){
    vector.emplace_back(item.toStdString());
  }
  return vector;
}


void DsscHDF5Packer::exportMeasurementToDataHisto(int idx)
{
  const auto settings = measurementSettings[0];

  if(settings.size() + idx > directoryList.size()){
    throw "Error: directory list smaller than expected";
  }

  DsscHDF5Packer::MeasurementSummary measurementSummary;

  for(auto && setting : settings)
  {
    QString settingDirName = directoryList[idx++];

    if(s_loadBlacklist){
      loadBlacklists(settingDirName);
    }

    if(s_loadSramCorrection){
      loadSramCorrection(settingDirName);
    }

    const auto measDirectory = baseDirectory + "/" + settingDirName;
    qDebug() << "Export Measurement Directory : " << measDirectory;

    const auto meanRmsVectors = exportDirectoryToDataHisto(measDirectory);

    if(!m_exportInOne){
      measurementSummary.push_back(meanRmsVectors);
      saveAndClearPixelHistograms(settingDirName,setting);
    }

    if(progress.wasCanceled()){
      qWarning() << "User abort";
      break;
    }
  }

  if(!m_exportInOne){
    saveMeasurementSummary(measurementSummary);
  }

  emitUpdateStatus("Writing data...");
  qDebug() << "---> Data Histo Packer Done - Export successful <---";
}


void DsscHDF5Packer::saveMeasurementSummary(const MeasurementSummary & measurementSummary)
{
  qDebug() << "Saving Measurement Summary";
  QFile file(baseDirectory + "/MeasurementSummaryMeanRMS.txt");
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
    qDebug() << "ERROR: Could not open MeasurmentSummaryMeanRMS file";
    return;
  }

  int numSettings = measurementSummary.size();

  QTextStream out(&file);
  out << "#Measurement Summary\n";
  out << "#Measurement Name:\t"<< measurementName <<"\t\n";
  out << "#Num Settings:\t" << numSettings <<"\t\n";
  int idx = 0;
  for(auto && paramName : sweepParameterNames){
    out << "#Measurement Param" << idx << " Name :\t" << paramName <<"\t\n";
    out << "#Measurement Param" << idx << " NumSettings :\t" << measurementSettings[idx].size() <<"\t\n";
    out << "#Measurement Param" << idx << " Settings:\t";
    for(auto && setting : measurementSettings[idx]){
      out << setting << ";";
    }
    out << "\t\n";
    idx++;
  }
  out << "#NumIterations:\t"<< numIterations << "\t\n";
  out << "#NumPreBurstVetos:\t"<< numPreBurstVetos << "\t\n";
  out << "#Measurement Summary\n";
  out << "#Measurement Summary\n";
  out << "#Pixel\tSetting\tMean\tRms\t\n";
  idx = 0;
  for(auto && pixel : pixels){
    for(int set=0; set<numSettings; set++){
      double mean = measurementSummary[set].first[idx];
      double rms  = measurementSummary[set].second[idx];
      out << pixel << "\t" << set << "\t" << mean << "\t" << rms << "\t\n";
    }
    idx++;
  }

  qDebug() << "Measurement Summary saved to " << file.fileName();

  file.close();
}

QPair<QVector<double>,QVector<double>> DsscHDF5Packer::exportDirectoryToDataHisto(const QString & exportDirPath)
{
  if(pixels.empty()) return {QVector<double>(),QVector<double>()};

  loadGainParametersFromDir(exportDirPath);

  const auto filesToPack = loadAllH5FileNamesFromDirectory(exportDirPath);
  const int selASIC = utils::getPixelASIC(pixels.toStdVector().front());
  const size_t numPixels = pixels.size();
  const size_t numFiles = filesToPack.size();
  const size_t numSram = 800;

  if(s_genSramAndBurst || s_genBlacklist){
    for(size_t px=0; px<numPixels; px++){
      m_burstMeans[px].resize(m_totalExpFiles+numFiles);
      m_burstMeansRMS[px].resize(m_totalExpFiles+numFiles);
    }
  }

  utils::StatsAccVec measurementStats(numPixels);

  emitUpdateStatus("Start Export");

  if(m_sramBlacklist->isValid()){
    qDebug() << "Export Directory using valid sram blacklist: " << exportDirPath;
  }

  size_t fileIdx = 0;
  for(auto && nextFile : filesToPack)
  {
    try{
      utils::Timer timer("Process one HDF5 file");

      DsscHDF5TrainDataReader reader((exportDirPath + "/" +nextFile).toStdString());
      m_numFrames = reader.getNumFrames();

      const auto * trainData = reader.getTrainData(); // loads and sorts data pixel wise 16/4096/800
      if(trainData)
      {
        m_tempADCVector[selASIC].emplace_back(trainData->getTempADCValue(selASIC));
        m_trainIDVector[selASIC].emplace_back(trainData->trainId);

        m_trainDataProcessor.fillDataHistoVec(trainData,m_pixelHistograms,false);

        // compute stats for current train, required for meanBursts
        // accumulators are added to measurementStats for setting means (sram blacklist included)

        m_trainDataProcessor.clearAccumulators();
        m_trainDataProcessor.fillMeanAccumulators(trainData,m_uintPixels);

        const auto pixelStatsAcc = m_trainDataProcessor.getStatsAccVec();

#pragma omp parallel for
        for(size_t pxIdx=0; pxIdx<numPixels;pxIdx++)
        {
          size_t imagePixel = pixels[pxIdx];
          const auto & pxAccs = pixelStatsAcc[imagePixel];
          measurementStats[pxIdx] += pxAccs;

          const auto pxStats = pxAccs.calcStats();

          if(s_genSramAndBurst || s_genBlacklist)
          {
            m_burstMeans[pxIdx][m_totalExpFiles+fileIdx] = pxStats.mean;
            m_burstMeansRMS[pxIdx][m_totalExpFiles+fileIdx] = pxStats.rms;

            const auto pixelDataArr = trainData->getPixelDataPixelWise(imagePixel);
            auto & pixelMeanSram = m_meanSrams[pxIdx];
            for(size_t sram=0; sram<numSram; sram++){
              pixelMeanSram[sram] += pixelDataArr[sram];
            }
          }
        }
      }else{
        qDebug() << "HDF5DataPacker: pixel Data invalid";
      }
      fileIdx++;
    }
    catch (std::exception & e)
    {
      qDebug() << "Error could not add data to Histograms from file " << nextFile;
      qDebug() << e.what();
    }

    progress.setValue(progress.value()+1);
    if(progress.wasCanceled()){
      qDebug() << "User Abort!";
      break;
    }
    emitUpdateStatus(nextFile + "...done");
  }

  if(s_genSramAndBurst || s_genBlacklist)
  {
    m_totalExpFiles += fileIdx;

    if(fileIdx < numFiles){
#pragma omp parallel for
      for(size_t px=0; px<numPixels; px++){
        m_burstMeans[px].resize(m_totalExpFiles);
        m_burstMeansRMS[px].resize(m_totalExpFiles);
      }
    }
  }

  QVector<double> meanValues(numPixels);
  QVector<double> rmsValues(numPixels);
#pragma omp parallel for
  for(size_t idx=0; idx<numPixels;idx++){
    const auto pxMeasStats = measurementStats[idx].calcStats();
    meanValues[idx] = pxMeasStats.mean;
    rmsValues[idx]  = pxMeasStats.rms;
  }

  for(auto && summaryValueVec : m_exportSummaryDataMap){
    summaryValueVec.resize(numPixels);
  }

  if(!s_exportInOne){
    saveBlacklistsAndSummary(exportDirPath);
  }

  return {meanValues,rmsValues};
}


void DsscHDF5Packer::saveBlacklistsAndSummary(const QString & exportDirPath)
{
  if(!s_genBlacklist && !s_genSramAndBurst){
    return;
  }

  const size_t numPixels = pixels.size();
  const size_t numSram = 800;

#pragma omp parallel for
  for(size_t idx=0; idx<numPixels;idx++){
    for(size_t sram=0; sram<numSram;sram++){
      m_meanSrams[idx][sram] /= m_totalExpFiles;
    }
  }

  if(s_genBlacklist){
    generateBurstAndSramBlacklist(exportDirPath);
  }

  if(s_genSramAndBurst){
    saveDirectorySummaryData(exportDirPath);
  }

  initBlacklistVariables();
}


void DsscHDF5Packer::generateBurstAndSramBlacklist(const QString & outputDir)
{
  static constexpr double SIGMA = 3.0;
  qDebug() << "Compute Blacklist Data";

  const size_t numPixels = m_burstMeans.size();
  const size_t numSram   = utils::s_numSram;
  const size_t numTrains = m_burstMeans.front().size();

  if(numPixels==0) return;

  std::vector<std::vector<uint32_t>> outlierBursts(numPixels);

#pragma omp parallel for
  for(size_t px=0; px<numPixels;px++){
    const auto & pixelBurstMeans = m_burstMeans[px];
    auto & pxOutlierBursts = outlierBursts[px];

    const auto stats = utils::getMeandAndRMS(pixelBurstMeans.data(),pixelBurstMeans.size());
    double maxLimit = stats.mean + stats.rms*SIGMA;
    double minLimit = stats.mean - stats.rms*SIGMA;

    int trainIdx=0;
    for(auto && burstMean : pixelBurstMeans){
      if(burstMean > maxLimit || burstMean < minLimit){
        pxOutlierBursts.push_back(trainIdx);
      }
      trainIdx++;
    }
  }

  qDebug() << "Save TrainID Outliers Overview";

  if(m_sweepExportMode != SweepExportMode::SIG)
  {
    const int selASIC = utils::getPixelASIC(pixels.toStdVector().front());
    const auto & trainIDs = m_trainIDVector[selASIC];

    std::string fileName = (outputDir + modeStr() + "_PxOutlierTrainIdsOverview.txt").toStdString();
    utils::saveTrainIDBlacklistSummary(fileName,utils::convertVectorType<int,uint32_t>(pixels.toStdVector()),outlierBursts,trainIDs);
  }


  std::map<uint32_t,uint32_t> outlierCounts;
  for(auto && pxOutliers : outlierBursts){
    for(auto && outlier : pxOutliers){
      outlierCounts[outlier]++;
    }
  }

  {
    // remove values below s_blackListTrainIdMinPxCnt counts
    std::for_each(outlierCounts.begin(),outlierCounts.end(),[](std::pair<const uint32_t, uint32_t> & x){if(x.second<s_blackListTrainIdMinPxCnt) x.second = 0;});
    utils::removeZerosMap(outlierCounts);
  }

  if(m_sweepExportMode != SweepExportMode::SIG)
  {
    qDebug() << "Save TrainID Outliers";
    const int selASIC = utils::getPixelASIC(pixels.toStdVector().front());
    const auto & trainIDs = m_trainIDVector[selASIC];

    std::string fileName = (outputDir + modeStr() + "_PxOutlierTrainIds.txt").toStdString();
    utils::saveTrainIDBlacklist(fileName,outlierCounts,trainIDs);
  }

  //###############################################################################################
  //###############################################################################################

  qDebug() << "Compute Sram Outliers";

  std::vector<double> XVALUES(numSram);
#pragma omp parallel for
  for(size_t i=0; i<numSram; i++){
    XVALUES[i] = i;
  }

  const double S_X  = std::accumulate(XVALUES.begin(), XVALUES.end(), 0.0);
  const double S_XX = std::inner_product(XVALUES.begin(), XVALUES.end(), XVALUES.begin(), 0.0);
#ifdef HAVE_HDF5
  std::vector<double> meanSramsToExport(utils::s_totalNumPxs*utils::s_numSram);
#endif
  std::vector<std::vector<uint32_t>> outlierSrams(numPixels);

#pragma omp parallel for
  for(size_t px=0; px<numPixels;px++)
  {
    const auto pixel = pixels[px];

    auto pixelMeanSrams = m_meanSrams[px].toStdVector();
#ifdef HAVE_HDF5
    // copy all meanSramValues into one large vector for storing sramCorrection file
    std::copy(pixelMeanSrams.begin(),pixelMeanSrams.end(),meanSramsToExport.begin()+pixel*utils::s_numSram);
#endif
    //### always run advanced outlier removal
    //### experimental Sram outliers improvement,
    // if large fraction of srams, bad extreme values are eliminated
    // if rms > 10 sram errors sram errors occured, can be found since they have normally high values
    // find most occuring value and remove all values with more than 5 distance, is valid for reasonable sram slopes

    double slope;
    auto stats = utils::getMeandAndRMS(pixelMeanSrams.data(),pixelMeanSrams.size());
    double maxLimit;
    double minLimit;

    if(stats.rms < 3)
    {
      slope = utils::linearRegression(S_X,S_XX,XVALUES,pixelMeanSrams);
      // correct for sram slope for improved outlier detection
      for(size_t idx = 0; idx<pixelMeanSrams.size(); idx++){
        pixelMeanSrams[idx] -= idx*slope;
      }
      const auto checkStats = utils::getMeandAndRMS(pixelMeanSrams.data(),pixelMeanSrams.size());

      maxLimit = checkStats.mean + checkStats.rms*SIGMA;
      minLimit = checkStats.mean - checkStats.rms*SIGMA;
    }
    else
    {
      // find worst outliers then correct for slope and then use the mean value
      uint16_t checkMeanValue = utils::calcModeOfVector(utils::convertVectorType<double,uint16_t>(pixelMeanSrams));
      double checkMaxLimit = checkMeanValue + 5.0;
      double checkMinLimit = checkMeanValue - 5.0;
      std::vector<double> goodValues;
      std::vector<double> goodAddresses;
      uint16_t sram = 0;
      for(auto && value : pixelMeanSrams){
        if(value > checkMinLimit && value < checkMaxLimit){
          goodValues.push_back(value);
          goodAddresses.push_back(sram);
        }
        sram++;
      }

      // correct for sram slope for improved outlier detection
      slope = utils::linearRegression(goodAddresses,goodValues);

      for(size_t idx = 0; idx<goodValues.size(); idx++){
        goodValues[idx] -= goodAddresses[idx]*slope;
      }

      const auto checkStats = utils::getMeandAndRMS(goodValues.data(),goodValues.size());
      maxLimit = checkStats.mean + checkStats.rms*SIGMA;
      minLimit = checkStats.mean - checkStats.rms*SIGMA;

      for(size_t idx = 0; idx<pixelMeanSrams.size(); idx++){
        pixelMeanSrams[idx] -= idx*slope;
      }
    }

    m_exportSummaryDataMap[PIXEL][px]     = pixel;
    m_exportSummaryDataMap[SRAMMEAN][px]  = stats.mean;
    m_exportSummaryDataMap[SRAMRMS][px]   = stats.rms;
    m_exportSummaryDataMap[SRAMSLOPE][px] = slope;

    { // fill sram outliers
      auto & pxOutlierSrams = outlierSrams[px];

      uint32_t sram = 0;
      for(auto && meanSram : pixelMeanSrams){
        if(meanSram > maxLimit || meanSram < minLimit){
          pxOutlierSrams.push_back(sram);
        }
        sram++;
      }

      //add existing sram blacklist entries:
      if(m_sramBlacklist->isValid()){
        const auto & pxSramBlacklist = m_sramBlacklist->getValidSramAddresses(pixel);
        for(auto && badSram : pxSramBlacklist){
          pxOutlierSrams.push_back(badSram);
        }
        utils::removeDuplicates(pxOutlierSrams);
      }
    }
  }

#ifdef HAVE_HDF5
  const std::string sramCorrFileName = (outputDir + modeStr() + "_SramCorrectionData.h5").toStdString();
  DsscHDF5Writer::saveBaselineAndSramCorrection(sramCorrFileName,meanSramsToExport,utils::getUpCountingVector(16),numTrains);
#endif

  qDebug() << "Save Sram Outliers";
  std::string fileName = (outputDir + modeStr() + "_PxSramOutliers.txt").toStdString();
  utils::savePxSramBlacklist(fileName,utils::convertVectorType<int,uint32_t>(pixels.toStdVector()),outlierSrams);
}


void DsscHDF5Packer::saveDirectorySummaryData(const QString & baseDir)
{
  QString outputDir = baseDir + "MeanBurstAndSramSummary/";

  utils::makePath(outputDir.toStdString());

  if(pixels.empty()) return;
  if(m_meanSrams.empty()) return;

  const auto stdpixels =pixels.toStdVector();
  const int selASIC = utils::getPixelASIC(stdpixels.front());
  size_t numTrains = m_trainIDVector[selASIC].size();

  qDebug() << "Save Mean Sram Summary";

  {
    QFile file(outputDir + "MeanSramData.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
      qDebug() << "ERROR: Could not open MeanSramData file";
      return;
    }
    int numSram = m_meanSrams.front().size();

    QTextStream out(&file);
    out << "#MeanSram Summary\n";
    out << "#Number of Trains:\t" << numTrains << "\t\n";
    out << "#Pixel\tMeanSram0-799\n";
    int idx = 0;
    for(auto && pixel : pixels){
      const auto & pixelSram = m_meanSrams[idx];
      out << pixel << "\t";
      for(int sram=0; sram<numSram; sram++){
        out << pixelSram[sram] << "\t";
      }
      out << "\n";

      m_exportSummaryDataMap[PIXEL][idx] = pixel;

      auto minmax = std::minmax_element(pixelSram.begin(),pixelSram.end());
      m_exportSummaryDataMap[SRAMMIN][idx] = *minmax.first;
      m_exportSummaryDataMap[SRAMMAX][idx] = *minmax.second;
      idx++;

      // mean and rms is computed with slope
    }

    file.close();
  }

  if(m_burstMeans.empty()) return;

  qDebug() << "Save Bursts Mean Summary";

  {
    QFile file(outputDir + "BurstMeansAndRMS.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
      qDebug() << "ERROR: Could not open BurstMeansAndRMS file";
      return;
    }

    size_t numBursts = m_burstMeans.front().size();

    std::vector<double> XVALUES(numBursts);
    for(size_t i=0; i<numBursts; i++){
      XVALUES[i] = i;
    }

    const double S_X  = std::accumulate(XVALUES.begin(), XVALUES.end(), 0.0);
    const double S_XX = std::inner_product(XVALUES.begin(), XVALUES.end(), XVALUES.begin(), 0.0);

    QTextStream out(&file);
    out << "#BurstMeans Summary\n";
    out << "#Number of Trains:\t" << numTrains << "\t\n";
    out << "#TrainIDs:\t";
    for(auto && trainID : m_trainIDVector[selASIC]){
      out << trainID << "\t";
    }
    out << "\n";
    out << "#Pixel\tType\tValues...\n";
    int idx = 0;
    for(auto && pixel : pixels){
      const auto & pixelBurstMeans = m_burstMeans[idx];
      out << pixel << "\tMean\t";
      for(size_t burst=0; burst<numBursts; burst++){
        out << pixelBurstMeans[burst] << "\t";
      }
      out << "\n";
      const auto & pixelBurstMeanRMS = m_burstMeansRMS[idx];
      out << pixel << "\tRMS\t";
      for(size_t burst=0; burst<numBursts; burst++){
        out << pixelBurstMeanRMS[burst] << "\t";
      }
      out << "\n";

      m_exportSummaryDataMap[PIXEL][idx] = pixel;

      auto minmax = std::minmax_element(pixelBurstMeans.begin(),pixelBurstMeans.end());
      m_exportSummaryDataMap[BURSTMIN][idx] = *minmax.first;
      m_exportSummaryDataMap[BURSTMAX][idx] = *minmax.second;
      const auto stats = utils::getMeandAndRMS(pixelBurstMeans.toStdVector());
      m_exportSummaryDataMap[BURSTMEAN][idx] = stats.mean;
      m_exportSummaryDataMap[BURSTRMS][idx] = stats.rms;
      double slope = utils::linearRegression(S_X,S_XX,XVALUES,pixelBurstMeans.toStdVector());
      m_exportSummaryDataMap[BURSTSLOPE][idx] = slope;

      idx++;
    }

    file.close();
  }


  {
    QVector<QString> paramNames {"Pixel","MinSram","MaxSram","MeanSram","RMSSram","SlopeSram","MinBurst","MaxBurst","MeanBurst","RMSBurst","SlopeBurst"};
    size_t numValues = m_exportSummaryDataMap.size();
    size_t numPixels = pixels.size();

    QFile file(outputDir + "/BurstMeansAndMeanSramSummary.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
      qDebug() << "ERROR: Could not open _BurstMeansAndMeanSramSummary file";
      return;
    }

    QTextStream out(&file);
    out << "#BurstMeansAndMeanSram Summary\n";
    out << "#";
    for(auto && name : paramNames){
      out << name << "\t";
    }
    out << "\n";

    for(size_t px=0; px<numPixels; px++){
      for(size_t idx=0; idx<numValues; idx++){
        out << m_exportSummaryDataMap[idx][px] << "\t";
      }
      out << "\n";
    }

    file.close();

    for(size_t idx=1; idx<numValues; idx++){
      const auto fileName = DataAnalyzer::generateHistoAnd2DMap(outputDir.toStdString(),paramNames[idx].toStdString(),"number",stdpixels,m_exportSummaryDataMap[idx]);
      cout << "Histo and Map stored in " << fileName << endl;
    }
  }
  qDebug() << "Summary Data Stored in " << outputDir;
}



void DsscHDF5Packer::exportDirectoryToRoot(const QString & exportDirPath)
{
  const auto filesToPack = loadAllH5FileNamesFromDirectory(exportDirPath);
  //const int selASIC = DataAnalyzer::getPixelASIC(pixels.front());

  emitUpdateStatus("Start Export");

  for(const auto & nextFile : filesToPack)
  {
    try{
      DsscHDF5TrainDataReader reader((exportDirPath + "/" +nextFile).toStdString());
      const auto * currentTrainData = reader.getTrainData();
      if(currentTrainData){
        packer->addCurrentPixelsBurst(currentTrainData);
      }else{
        qDebug() << "HDF5DataPacker: pixel Data invalid";
      }
    }
    catch (...)
    {
      qDebug() << "Error could not convert file: " << nextFile;
    }

    progress.setValue(progress.value()+1);
    if(progress.wasCanceled()){
      qDebug() << "User Abort!";
      break;
    }
    emitUpdateStatus(nextFile + "...done");
  }
}


bool DsscHDF5Packer::exportToASCII()
{
  if(!configLoaded){
    return false;
  }

  qDebug() << "Exporting" << baseDirectory << "into histograms";

  int numMeasurements = 1;
  if(sweepParameterNames.size() > 1){
    numMeasurements = measurementSettings[1].size();
  }
  int numSettings = measurementSettings[0].size();
  int numFilesToPack = numIterations*numMeasurements*numSettings;

  initProgressDialog(numFilesToPack);

  int setting = 0;
  for(const auto & directory : directoryList)
  {
    exportDirectoryToASCII(baseDirectory + "/" + directory,setting++);
    if(progress.wasCanceled()){
      qWarning() << "User abort";
      break;
    }
  }
  return true;
}


void DsscHDF5Packer::exportDirectoryToASCII(const QString & exportDirPath, int setting)
{
  const auto filesToPack = loadAllH5FileNamesFromDirectory(exportDirPath);

  emitUpdateStatus("Start ASCII Export");

  const int numPxs = m_uintPixels.size();

  utils::DataHistoVec pixelHistos(numPxs);

  for(const auto & nextFile : filesToPack)
  {
    DsscHDF5TrainDataReader reader((exportDirPath + "/" +nextFile).toStdString());
    const auto * trainData = reader.getTrainData(); // loads and sorts data pixel wise 16/4096/800
    if(trainData){
      m_trainDataProcessor.fillDataHistoVec(trainData,pixelHistos,true);
    }else{
      qDebug() << "HDF5DataPacker: pixel Data invalid";
    }

    progress.setValue(progress.value()+1);
    if(progress.wasCanceled()){
      qDebug() << "User Abort!";
      break;
    }
    emitUpdateStatus(nextFile + "...done");
  }

  std::string exportFileName = exportDirPath.toStdString() + "/HistogramExport.txt";
  utils::DataHisto::dumpHistogramsToASCII(exportFileName,utils::convertVectorType<int,uint32_t>(pixels.toStdVector()),pixelHistos,setting);
}


void DsscHDF5Packer::emitUpdateStatus(const QString &status)
{
  emit updateStatus(status,0);
  QCoreApplication::processEvents(QEventLoop::AllEvents,10);
}


void DsscHDF5Packer::removeBlacklistFiles(QStringList & filesToPack)
{
  for(auto && badTrainId : m_trainIdBlacklist){
    const auto filtered = filesToPack.filter(QString::number(badTrainId));
    if(!filtered.empty()){
      for(auto && badFileName : filtered){
        filesToPack.removeAll(badFileName);
      }
    }
  }
}


QStringList DsscHDF5Packer::loadAllH5FileNamesFromDirectory(const QString & exportDirPath)
{
  emitUpdateStatus("Load files from directory");

  if(exportDirPath.isEmpty()) return QStringList();

  QDir exportDir(exportDirPath);
  if(!exportDir.exists()){
    throw "Directory not found:  " + exportDirPath;
  }

  QStringList filesToPack = exportDir.entryList({"*_TrainData_*.h5"},QDir::Files,QDir::Name);
  qDebug() << filesToPack.size() << "Files found to export";

  if(filesToPack.empty()){
    return QStringList();
  }

  qSort(filesToPack);

  // remove blacklist files
  if(m_trainIdBlacklistValid){
    removeBlacklistFiles(filesToPack);
  }

  for(auto && file : filesToPack){
    qDebug() << file;
  }

  if(numIterations != filesToPack.size()){
    qDebug() << "Num Iterations does not fit to files to pack: " << numIterations << "/" << filesToPack.size();
    if(m_trainIdBlacklistValid){
      qDebug() << "Removed" << m_trainIdBlacklist.size() << "files from list" << endl;
    }
  }

  // if trainIdBlacklist valid, probably some files have been removed so copy the file names of the last valid file
  // to the end of the vector
  if(m_trainIdBlacklistValid){
    while(filesToPack.size() < numIterations){
      filesToPack.push_back(filesToPack.back());
    }
  }

  numIterations = filesToPack.size();
  return filesToPack;
}


void DsscHDF5Packer::generateInitialChipData()
{
  emitUpdateStatus("Prepare Root File");

  int pixel = packer->getCurrPixel();

  vector<string> paramNames;
  vector<int> params;

  initChipDataParams(paramNames,params);

  int sramSize = DsscHDF5TrainDataReader::getSramSize();
  DataPacker::numChipParams = params.size();
  DataPacker::numChipData = sramSize;

  packer->setChipData(SuS::ChipData(params,paramNames,sramSize,pixel));

  generateCalibrationInfo();
}


int DsscHDF5Packer::getIntDACValue()
{
  uint8_t VDAC_i1a_B = jtagRegisters->getSignalValue("Global Control Register","0","VDAC_En_i1a_B");
  uint8_t VDAC_Bin_B = jtagRegisters->getSignalValue("Global Control Register","0","VDAC_Bin_B");
  uint8_t VDAC_Bin   = jtagRegisters->getSignalValue("Global Control Register","0","VDAC_Bin");

  int val = 0;

  switch(VDAC_i1a_B) {
    case 0b11111111 : val += 0; break;
    case 0b11110111 : val += 1; break;
    case 0b11100111 : val += 2; break;
    case 0b11000111 : val += 3; break;
    case 0b11000011 : val += 4; break;
    case 0b10000011 : val += 5; break;
    case 0b10000001 : val += 6; break;
    case 0b10000000 : val += 7; break;
    default : qWarning() << "Error in internal DAC LSB coding.";
  }

  val += (uint8_t)((7 & ~VDAC_Bin_B) << 3);
  val += VDAC_Bin << 6;

  return val;
}


void DsscHDF5Packer::initChipDataParams(vector<string> &paramNames, vector<int> & params)
{
  int pixel = pixels.front();

  auto pixelSignalNames = pixelRegisters->SuS::ConfigReg::getSignalNames("Control register");
  paramNames.insert(paramNames.end(),pixelSignalNames.begin(),pixelSignalNames.end());
  for(const auto & signalName : pixelSignalNames){
    params.push_back(pixelRegisters->SuS::ConfigReg::getSignalValue("Control register",to_string(pixel),signalName));
  }

  paramNames.push_back("IntDACValue");
  params.push_back(getIntDACValue());

  paramNames.push_back("IntegrationLength");
  params.push_back(sequencer->getSequencerParameter(Sequencer::IntegrationLength));
  paramNames.push_back("FlattopLength");
  params.push_back(sequencer->getSequencerParameter(Sequencer::FlattopLength));
  paramNames.push_back("FlattopHoldLength");
  params.push_back(sequencer->getSequencerParameter(Sequencer::FlattopHoldLength));
  paramNames.push_back("ResetLength");
  params.push_back(sequencer->getSequencerParameter(Sequencer::ResetLength));
  paramNames.push_back("ResetIntegOffset");
  params.push_back(sequencer->getSequencerParameter(Sequencer::ResetIntegOffset));
  paramNames.push_back("ResetHoldLength");
  params.push_back(sequencer->getSequencerParameter(Sequencer::ResetHoldLength));
  paramNames.push_back("RampLength");
  params.push_back(sequencer->getSequencerParameter(Sequencer::RampLength));
  paramNames.push_back("BackFlipAtReset");
  params.push_back(sequencer->getSequencerParameter(Sequencer::BackFlipAtReset));
  paramNames.push_back("BackFlipToResetOffset");
  params.push_back(sequencer->getSequencerParameter(Sequencer::BackFlipToResetOffset));
  paramNames.push_back("HoldPos");
  params.push_back(sequencer->getSequencerParameter(Sequencer::HoldPos));
  paramNames.push_back("HoldLength");
  params.push_back(sequencer->getSequencerParameter(Sequencer::HoldLength));
  paramNames.push_back("RampOffset");
  params.push_back(sequencer->getSequencerParameter(Sequencer::RampOffset));
  paramNames.push_back("SingleSHCapMode");
  params.push_back(sequencer->isSingleSHCapMode());

  paramNames.push_back("Vhold");
  params.push_back(0);

  paramNames.push_back("TempADC");
  params.push_back(0);

  paramNames.push_back("TrainID");
  params.push_back(0);

  paramNames.push_back("BurstTime");
  params.push_back(0);

  auto jtagSignalNames = jtagRegisters->SuS::ConfigReg::getSignalNames("Global Control Register");
  paramNames.insert(paramNames.end(),jtagSignalNames.begin(),jtagSignalNames.end());
  for(const auto & signalName : jtagSignalNames){
    params.push_back(jtagRegisters->SuS::ConfigReg::getSignalValue("Global Control Register","0",signalName));
  }

  for(auto && name : sweepParameterNames){
    if(find(paramNames.begin(),paramNames.end(),"name") == paramNames.end()){
      paramNames.push_back(name.toStdString());
      params.push_back(0);
    }
  }
}


void DsscHDF5Packer::generateCalibrationInfo()
{
  int pixelRem = packer->getCurrPixel();

  std::map<std::string,int> params;
  if(pixelRegisters->signalNameExists("Control register","CompCoarse") ){
    params = {{"CompCoarse",0},
              {"RmpFineTrm",0},
              {"RmpCurrDouble",0},
              {"FCF_EnCap",0}};
  }else{
    params = {{"CSA_FbCap",0},
             {"CSA_Resistor",0},
             {"RmpFineTrm",0},
             {"RmpCurrDouble",0},
             {"FCF_EnCap",0}};
  }

  for(const auto & px : pixels)
  {
    int i=0;
    for(auto && param : params)
    {
      param.second = pixelRegisters->getSignalValue("Control register",to_string(px),param.first);
      i++;
    }
    packer->setCurrPixel(px);
    packer->setCalibrationConfig(params);
  }

  packer->setCurrPixel(pixelRem);
}

