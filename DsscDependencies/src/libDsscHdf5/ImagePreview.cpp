#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QDebug>
#include <QProgressDialog>
#include <QRadioButton>
#include <QGridLayout>
#include <QCoreApplication>
#include <QMutex>

#include <TH1.h>
#include <TH1I.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TH2Poly.h>
#include <TQtWidget.h>
#include <TFile.h>
#include <TStyle.h>
#include <TGraphErrors.h>

#include <stdint.h>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <mutex>

#include "DataAnalyzer.h"

#include "ImagePreview.h"
#include "utils.h"

#define GCCWRAP 5

#define PRCEVENTS QCoreApplication::processEvents(QEventLoop::AllEvents,10)

using namespace std;
using namespace SuS;


IPTrailerLabel::IPTrailerLabel(QWidget* _parent)
 : QWidget(_parent)
{
  generateLayout();
}

void IPTrailerLabel::generateLayout()
{
  QHBoxLayout *mainLayout = new QHBoxLayout;
  setLayout(mainLayout);

  mainLayout->addWidget(new QLabel("Trailer Data"));

  vetoCntLbl     = new QLabel("-");
  temp0Lbl       = new QLabel("-");
  temp1Lbl       = new QLabel("-");
  xorLbl         = new QLabel("-");
  testPatternLbl = new QLabel("-");
  trainIDLbl     = new QLabel("-");

  vetoCntLbl     -> setMinimumWidth(60);
  temp0Lbl       -> setMinimumWidth(60);
  temp1Lbl       -> setMinimumWidth(60);
  xorLbl         -> setMinimumWidth(180);
  testPatternLbl -> setMinimumWidth(60);
  trainIDLbl     -> setMinimumWidth(60);

  temp0Lbl       ->setToolTip("Temperature0<br>"
                               "Blue and beef means trailer packet could not be received.<br>"
                               "Green = expected value.");
  temp1Lbl       ->setToolTip("Temperature1");
  vetoCntLbl     ->setToolTip("VetoCnt");
  xorLbl         ->setToolTip("Xor");
  testPatternLbl ->setToolTip("TestPattern");
  trainIDLbl     ->setToolTip("TrainID");

  mainLayout->addWidget(temp0Lbl);
  mainLayout->addWidget(temp1Lbl);
  mainLayout->addWidget(new QLabel("VC:"));
  mainLayout->addWidget(vetoCntLbl);
  mainLayout->addWidget(xorLbl);
  mainLayout->addWidget(new QLabel("TP:"));
  mainLayout->addWidget(testPatternLbl);
  mainLayout->addWidget(new QLabel("TrainID:"));
  mainLayout->addWidget(trainIDLbl);
  mainLayout->addStretch();
}


void IPTrailerLabel::update( utils::DsscTrainData * trainData, int asic, TRMODE mode)
{

  DsscTrailerData trailerData(trainData->asicTrailerData.data(),asic);

  updateTrainID(trainData->trainId);
  updateTrailer(trailerData,mode);
  updateSpecific(trainData->getSpecificData());
}


void IPTrailerLabel::updateSpecific(const utils::DsscSpecificData & specificData)
{
  QString infoStr = QString::fromStdString(specificData.getInfoStr());
  infoStr.replace("\n","<br>");
  trainIDLbl->setToolTip(infoStr);
}

void IPTrailerLabel::updateTrailer(const DsscTrailerData & trailerData, TRMODE mode)
{
  if(mode==ASIC){
    updateTrailer(trailerData);
  }else if(mode==DRDUMMY){
    updateTrailerDummyDRMode(trailerData);
  }else if(mode==IOBDUMMY){
    updateTrailerDummyIOBMode(trailerData);
  }else if(mode==TESTPATTERN){
   updateTrailerTestPatternMode(trailerData);
  }
}

void IPTrailerLabel::updateTrailer(const DsscTrailerData & trailerData)
{
  setVetoCnt(trailerData.m_vetoCnt);
  setTemperature(temp0Lbl,trailerData.m_temp0);
  setTemperature(temp1Lbl,trailerData.m_temp1);
  setXORValue();
  setTestPattern(trailerData.m_testPattern);
}


void IPTrailerLabel::updateTrainID(uint64_t trainID)
{
  /*
  if(trainID == DSSC_PPT::RECV_ERROR_DROPPED_PACKETS){
    trainIDLbl->setText("Dropped Packets");
  }else if(trainID == DSSC_PPT::RECV_ERROR_TIMEOUT){
    trainIDLbl->setText("Readout Timeout");
  }else if(trainID == DSSC_PPT::RECV_ERROR_TEST_PATTERN){
    trainIDLbl->setText("TestPattern");
  }else{
  */
    trainIDLbl->setText(QString::number(trainID));
 // }
}



void IPTrailerLabel::setVetoCnt(uint16_t veto_cnt)
{
  vetoCntLbl->setText(QString::number(veto_cnt));
  vetoCntLbl->setStyleSheet("background-color: white;");
}


void IPTrailerLabel::setTemperature(QLabel *tempLabel, int temp)
{
  tempLabel->setText(QString::number(temp));
  tempLabel->setStyleSheet("background-color: white;");
}

void IPTrailerLabel::setTestPattern(uint16_t value)
{
  testPatternLbl->setText(QString::number(value));

  if(value == 0xBEEF){
    testPatternLbl->setStyleSheet("background-color: blue;");
  }else if(value == 5 ){
    testPatternLbl->setStyleSheet("background-color: lime;");
  }else{
    testPatternLbl->setStyleSheet("background-color: red;");
  }
}


void IPTrailerLabel::setXORValue()
{
  /*
  static const QVector<QString> xorStr  ={ "20||||||||||||||||||||| FCSR YSel<br>",
                                           "19_|||||||||||||||||||| FCSR PXRegs1<br>",
                                           "18__||||||||||||||||||| FCSR XSel1<br>",
                                           "-------------------------------------<br>",
                                           "17___|||||||||||||||||| FCSR ColFoot1<br>",
                                           "16____||||||||||||||||| FCSR PXRegs0<br>",
                                           "15_____|||||||||||||||| FCSR XSel0<br>",
                                           "14______||||||||||||||| FCSR ColFoot0<br>",
                                           "13_______|||||||||||||| JTAG XorIns<br>",
                                           "12________||||||||||||| JTAG Tmp ADC<br>",
                                           "11_________|||||||||||| JTAG Serializer<br>",
                                           "10__________||||||||||| JTAG Sram Control<br>",
                                           "09___________|||||||||| JTAG Sram JTAG<br>",
                                           "--------------------------------------<br>",
                                           "08____________||||||||| Seq Track ISub<br>",
                                           "07_____________|||||||| Seq Track Flip<br>",
                                           "06______________||||||| Seq Track Res_B<br>",
                                           "05_______________|||||| Seq Track SwIn<br>",
                                           "04________________||||| Seq Track RMP<br>",
                                           "03_________________|||| Seq Rep Cnts<br>",
                                           "02__________________||| JTAG Seq Config<br>",
                                           "01___________________|| JTAG Master FSM<br>",
                                           "00____________________| JTAG Global<br>"};

  QString readStr = m_ppt->getXorStr(true);  readStr = readStr.right(readStr.length()-2);
  QString expStr  = m_ppt->getXorStr(false); expStr  =  expStr.right( expStr.length()-2);

  readStr.insert(12,"|"); readStr.insert( 3,"|");
  expStr.insert( 12,"|"); expStr.insert(  3,"|");

  QString toolTipString = "<font face=TypeWriter>CalcXor:<br>0b"+expStr+"<br>ReadXor:<br>0b"+readStr+"<br>";
  for(int i=0; i<readStr.length(); i++){
    if(readStr[i]!=expStr[i]){
      readStr[i] = 'x';
      toolTipString +=  xorStr[i];
    }
  }

  xorLbl->setText(readStr);
  xorLbl->setToolTip(toolTipString);

  if(m_ppt->isXorCorrect()){
    xorLbl->setStyleSheet("background-color: lime;");
  }else{
    xorLbl->setStyleSheet("background-color: red;");
  }
  */
}


void IPTrailerLabel::updateTrailerTestPatternMode(const DsscTrailerData & trailerData)
{
  int expectedPattern = 5;

  uint16_t value = trailerData.m_testPattern;

  vetoCntLbl     -> setText(QString::number(value));
  temp0Lbl       -> setText(QString::number(value));
  temp1Lbl       -> setText(QString::number(value));
  xorLbl         -> setText(QString::number(value));
  testPatternLbl -> setText(QString::number(value));

  if(value == expectedPattern){
    vetoCntLbl     -> setStyleSheet("background-color: lime;");
    temp0Lbl       -> setStyleSheet("background-color: lime;");
    temp1Lbl       -> setStyleSheet("background-color: lime;");
    xorLbl         -> setStyleSheet("background-color: lime;");
    testPatternLbl -> setStyleSheet("background-color: lime;");
  }else{
    vetoCntLbl     -> setStyleSheet("background-color: red;");
    temp0Lbl       -> setStyleSheet("background-color: red;");
    temp1Lbl       -> setStyleSheet("background-color: red;");
    xorLbl         -> setStyleSheet("background-color: red;");
    testPatternLbl -> setStyleSheet("background-color: red;");
  }
}

void IPTrailerLabel::updateTrailerDummyIOBMode(const DsscTrailerData & trailerData)
{
  int expectedPattern = 1;

  temp0Lbl -> setText(QString::number(trailerData.m_temp0));
  if(trailerData.m_temp0 == 1){
    temp0Lbl->setStyleSheet("background-color: lime;");
  }else{
    temp0Lbl->setStyleSheet("background-color: red;");
  }

  expectedPattern++;

  temp1Lbl -> setText(QString::number(trailerData.m_temp1));
  if(trailerData.m_temp1 == 2){
    temp1Lbl->setStyleSheet("background-color: lime;");
  }else{
    temp1Lbl->setStyleSheet("background-color: red;");
  }

  expectedPattern++;

  testPatternLbl -> setText(QString::number(trailerData.m_testPattern));
  if(trailerData.m_testPattern == 0){
    testPatternLbl->setStyleSheet("background-color: lime;");
  }else{
    testPatternLbl->setStyleSheet("background-color: red;");
  }
}

void IPTrailerLabel::updateTrailerDummyDRMode(const DsscTrailerData & trailerData)
{
  temp0Lbl -> setText(QString::number(trailerData.m_temp0));
  if(trailerData.m_temp0 == 2){
    temp0Lbl->setStyleSheet("background-color: lime;");
  }else{
    temp0Lbl->setStyleSheet("background-color: red;");
  }

  temp1Lbl -> setText(QString::number(trailerData.m_temp1));
  if(trailerData.m_temp1 == 3){
    temp1Lbl->setStyleSheet("background-color: lime;");
  }else{
    temp1Lbl->setStyleSheet("background-color: red;");
  }

  testPatternLbl -> setText(QString::number(trailerData.m_testPattern));
  if(trailerData.m_testPattern == 1){
    testPatternLbl->setStyleSheet("background-color: lime;");
  }else{
    testPatternLbl->setStyleSheet("background-color: red;");
  }
}


ImagePreview::ImagePreview(QWidget* _parent) :
  QWidget(_parent),
  showHisto(true),
  sramImage(NULL),
  fileName("pixelMap.png"),
  numPxsToShow(1),
  numOfPxs(4096),
  sramSize(800),
  enGCCWrap(true),
  ladderMode(false)
{
  generateLayout();

  initRootStyle();

  initMembers();
}


void ImagePreview::initMembers()
{
  for(int asic = 0; asic <= 16; asic++)
  {
    int asicRow = (asic>7)? 64 : 0;
    int asicCol = (asic%8) * 64;

    for(int pixel =0; pixel <4096; pixel++)
    {
      int col = asicCol + pixel%64 + 1;
      int row = asicRow + pixel/64 + 1;
      cols.push_back(col);
      rows.push_back(row);
    }
  }
}


ImagePreview::~ImagePreview()
{
  stop();

  delete PixelMap;
  delete PixelMap_Hexagons;
  delete pixelMapHisto;
  delete pixelMapRMSHisto;

  for(auto & histo : histo_px){
    delete histo;
  }
}


void ImagePreview::initRootStyle()
{
  gStyle->SetPalette(55);
  gStyle->SetNumberContours(255);
  gStyle->SetCanvasColor(0);
  gStyle->SetAxisColor(1);
  gStyle->SetLineWidth(2);
  gStyle->SetFrameLineWidth(2);
  gStyle->SetStatBorderSize(2);
}

void ImagePreview::savePlot()
{
  if (PixelMapWidget->isVisible()) {
    PixelMapRootWidget->GetCanvas()->SaveAs(fileName.c_str());
  }
  else if (histoWidget->isVisible())
  {
    int selPx = pixelSelectSB->value();
    for (int i=0; i<numPxsToShow; ++i)
    {
      histo_px.at(i)->SaveAs(("histo_px" + to_string((i+selPx)%numOfPxs)+".C").c_str());
      TFile histoRootfile(("histo_px" + to_string((i+selPx)%numOfPxs)+".root").c_str(),"RECREATE");
      histo_px.at(i)->Write();
      histoRootfile.Write();
      histoRootfile.Close();
      string file = "histo_px" + to_string((i+selPx)%numOfPxs) + ".png";
      HistoRootWidget.at(i)->GetCanvas()->SaveAs(file.c_str());
    }
  }
}


uint32_t ImagePreview::getSelectedPixel()
{
  uint32_t pixel = pixelSelectSB -> value();
  return convertPixelToShow(pixel);
}


void ImagePreview::showContentGraph()
{
  int pixel = getSelectedPixel();

  int startAddr = sramStartAddressSB->value();
  int endAddr = sramEndAddressSB->value();

  int numSettings = endAddr-startAddr+1;

  TGraphErrors *gI = new TGraphErrors(numSettings);

  int point=0;

  auto currentPixelData = getPixelSramData(pixel);

  for(int i=startAddr; i<=endAddr; i++){
    gI->SetPoint(point++,i,convertValue(currentPixelData[i]));
  }

  updateInfoLabels();

  if(!enFocusCB->isChecked()){
    gI->SetMaximum(512);
    gI->SetMinimum(-1);
  }

  gI->SetLineColor(2);

  showHisto = true;
  sramLabelBox->hide();
  PixelMapWidget->hide();
  histoWidget->show();

  for(int i=HistoRootWidget.size(); i>0 ; i--){
    delete HistoRootWidget.at(i-1);
  }
  HistoRootWidget.clear();

  if(histoGLayout)
    delete histoGLayout;

  histoGLayout = new QGridLayout(histoWidget);

  HistoRootWidget.push_back(new TQtWidget(histoWidget));
  HistoRootWidget.at(0)->GetCanvas()->cd();

  gI->Draw("ALP");

  HistoRootWidget.at(0)->GetCanvas()->Update();
  HistoRootWidget.at(0)->GetCanvas()->SetFillColor(17);

  histoGLayout -> addWidget(HistoRootWidget.at(0),0,0);

  histoAreaLabel->setLayout(histoGLayout);
  histoAreaLabel->resize(histoWidget->size().width()*0.99,histoWidget->size().height()*0.99);
  histoWidget->setWidget(histoAreaLabel);
  histoWidget->update();

}

void ImagePreview::updateHistoLayout()
{
  gStyle->SetOptStat(1111);

  showHisto = true;
  sramLabelBox->hide();
  PixelMapWidget->hide();
  histoWidget->show();

  int selPx = pixelSelectSB->value();

  for (int i=0; i<numPxsToShow; ++i) {

    QString histoName = "Histo_Pixel" + QString::number((i+selPx)%numOfPxs);
    std::string histoChar = histoName.toStdString();
    if(i>=histo_px.size()){
      histo_px.push_back(new TH1I(histoChar.c_str(),histoChar.c_str(),512,-0.5,511.5));
      histo_px_mutex.push_back(new QMutex);
    }
    histo_px.at(i) -> Reset("M");
    histo_px.at(i) -> SetFillColor(kBlue);
    histo_px.at(i) -> SetTitle(histoChar.c_str());

    if(i>=HistoRootWidget.size()){
      HistoRootWidget.push_back(new TQtWidget(histoWidget));
    }
    if(enLogScaleCB->isChecked()){
      HistoRootWidget.at(i)->GetCanvas()->SetLogy(1);
    }else{
      HistoRootWidget.at(i)->GetCanvas()->SetLogy(0);
    }
    HistoRootWidget.at(i)->GetCanvas()->SetFillColor(17);

  }

  int numCols = sqrt(numPxsToShow);
  while(numOfPxs%numCols != 0){
    numCols++;
  }
  int numRows = numPxsToShow/numCols;

  if(histoGLayout)
    delete histoGLayout;

  histoGLayout = new QGridLayout(histoWidget);

  for(int x=0; x<numCols; x++){
    for(int y=0; y<numRows; y++){
      histoGLayout -> addWidget(HistoRootWidget.at(x*numRows+y),y,numCols-x-1);
    }
  }

  histoAreaLabel->setLayout(histoGLayout);
  histoAreaLabel->resize(histoWidget->size().width()*0.99,histoWidget->size().height()*0.99);
  histoWidget->setWidget(histoAreaLabel);
  histoWidget->update();
}


void ImagePreview::updateImagePreview()
{
  updateInfoLabels();
  if(showHisto){
    refreshHistogram();
  } else {
    initPixelMap();
    updatePixelMap();
  }
}


void ImagePreview::refreshPixelMapHisto()
{
  initPixelMapHisto();
  updatePixelMapHisto();
}


void ImagePreview::refreshPixelMap(){
  initPixelMap();
  updatePixelMap();
}


void ImagePreview::updateInfoLabels()
{
  IPTrailerLabel::TRMODE mode = IPTrailerLabel::ASIC;

  if(drDummyDataRB->isChecked()){
    mode = IPTrailerLabel::DRDUMMY;
  }else if(iobDummyDataRB->isChecked()){
    mode = IPTrailerLabel::IOBDUMMY;
  }else if(testPatternRB->isChecked()){
    mode = IPTrailerLabel::TESTPATTERN;
  }

  trailerLbl->update(currentTrainData,asicSelectSB->value(),mode);
}


void ImagePreview::setBaseline()
{
  bool subRem = subtractBaselineCB->isChecked();
  bool errRem = showRMSCB->isChecked();
  subtractBaselineCB->setChecked(false);
  showRMSCB->setChecked(false);

  baselineValues = getPixelData();
  cout << "Baseline updated" << endl;

  subtractBaselineCB->setChecked(subRem);
  showRMSCB->setChecked(errRem);
}


const QVector<double> & ImagePreview::getPixelData()
{
  bool errors   = showRMSCB->isChecked();
  bool subtract = (errors)? false : subtractBaselineCB->isChecked();
  uint32_t endAddr   = std::min((uint32_t)sramEndAddressSB->value(),currentTrainData->pulseCnt-1);
  uint32_t startAddr = std::min((uint32_t)sramStartAddressSB->value(),endAddr);
  uint32_t numValues = endAddr-startAddr+1;
  uint32_t numPixels = (ladderMode? 16 : 1) * 4096;

  static QVector<double> binValues(numPixels,0.0);

  if(errors && numValues == 1) return binValues;

  if(errors){

  }else{
    if(ladderMode){
      const auto sortedImageData = currentTrainData->getImageDataArray(startAddr);
      std::copy(sortedImageData.begin(),sortedImageData.end(),binValues.begin());
    }else{
      const auto sortedAsicData = currentTrainData->getAsicDataArray(0,startAddr);
      std::copy(sortedAsicData.begin(),sortedAsicData.end(),binValues.begin());
    }
  }

  if(subtract)
  {
    if(baselineValues.size() != binValues.size()){
      baselineValues = binValues;
    }

    for(uint i=0; i<numPixels;i++){
      binValues[i] -= baselineValues[i];
    }
  }

  return binValues;
}


uint32_t ImagePreview::getStartASIC()
{
  if(!currentTrainData) return 0;

  if(ladderMode) return currentTrainData->availableASICs.front();

  uint32_t selASIC = asicSelectSB->value();
  int idx = 0;
  for(const auto & a : currentTrainData->availableASICs){
    if(a == selASIC){
      return idx;
    }
    idx++;
  }

  return 0;
}


uint32_t ImagePreview::getEndASIC()
{
  if(!currentTrainData) return 0;

  if(ladderMode) return currentTrainData->availableASICs.back();

  uint32_t selASIC = asicSelectSB->value();
  int idx = 0;
  for(const auto & a : currentTrainData->availableASICs){
    if(a == selASIC){
      return idx;
    }
    idx++;
  }

  return 0;
}


uint16_t ImagePreview::convertValue(uint16_t value) const
{
  if(enGCCWrap){
    if(value < GCCWRAP){
      value += 256;
    }
  }

  if(!en9BitModeCB->isChecked()){
    value = value % 256;
  }
  return value;
}


void ImagePreview::refreshHistogram()
{
  showHisto = true;
  PixelMapWidget->hide();
  histoWidget->show();
  sramLabelBox->hide();

  updateHistoLayout();
  updateHistograms();
}


void ImagePreview::updateHistogram(uint32_t pixel, TH1* histo)
{
  int sramStartAddress = sramStartAddressSB->value();
  int sramEndAddress = sramEndAddressSB->value();

  uint32_t currentASIC = asicSelectSB->value();

  auto pixelData = currentTrainData->getPixelDataPixelWise(currentASIC,pixel);
  for (int j=sramStartAddress; j<=sramEndAddress; j++) {
    histo->Fill(convertValue(pixelData[j]));
  }

  if(enFocusCB->isChecked()){
    DataAnalyzer::setXAxisRange(histo);
  }
}


void ImagePreview::histoUpdated(int px)
{
  px = convertPixelToShow(px); // convert it back to get correct index of histogram
  int pxOffset = pixelSelectSB->value();
  int histoIdx = (px-pxOffset+numOfPxs)%numOfPxs;

  assert(histoIdx<HistoRootWidget.size());

  HistoRootWidget.at(histoIdx)->GetCanvas()->cd();
  double meanValue = histo_px.at(histoIdx)->GetMean();
  if(meanValue == 511.0){
    HistoRootWidget.at(histoIdx)->GetCanvas()->SetFillColor(47);
  }else if(meanValue == 0.0){
    HistoRootWidget.at(histoIdx)->GetCanvas()->SetFillColor(19);
  }
  histo_px.at(histoIdx)->Draw();

  HistoRootWidget.at(histoIdx)->GetCanvas()->Update();
  HistoRootWidget.at(histoIdx)->show();
}


uint32_t ImagePreview::convertPixelToShow(uint32_t pixel)
{
  const bool fullLadder    = ladderMode;
  const int  selAsic       = asicSelectSB->value();
  if(selAsic>7 && fullLadder){
    return numOfPxs - (pixel%numOfPxs) - 1;
  }

  return pixel%numOfPxs;
}


void ImagePreview::updateHistograms()
{
  updateInfoLabels();

  int pxOffset = pixelSelectSB->value();

  for (int i=0; i<numPxsToShow; ++i) {
    updateHistogram(convertPixelToShow(i+pxOffset),histo_px.at(i));
  }
}


void ImagePreview::initHistogram()
{
  showHisto = true;
  PixelMapWidget->hide();
  histoWidget->show();
  sramLabelBox->hide();
  updateHistoLayout();
}


TH1I* ImagePreview::getHisto(int _px)
{
  int pxOffset = pixelSelectSB->value();
  int pxIndex  = _px-pxOffset;
  if (_px >= pxOffset && _px < pxOffset + numPxsToShow) {
    histo_px_mutex.at(pxIndex)->lock();   // to check if it is still being updated
    histo_px_mutex.at(pxIndex)->unlock();
    return histo_px.at(pxIndex);
  }
  else
    return NULL;
}


void ImagePreview::setASICDataToShow()
{
  int selAsic = asicSelectSB->value();
  setASICDataToShow(selAsic);
}


void ImagePreview::setASICDataToShow(int asic)
{
  asicSelectSB->blockSignals(true);
  asicSelectSB->setValue(asic);
  asicSelectSB->blockSignals(false);
}

void ImagePreview::setPixel(int _px)
{
  pixelSelectSB->setValue(_px);
  selNumPxToShowCB->setCurrentIndex(0);
  updateHistoLayout();
}

void ImagePreview::startLiveData()
{
  bool runLive = enableLiveImageBtn->isChecked();

  if(runLive){
    if(!m_trainStorage){
      enableLiveImageBtn->setChecked(false);
      return;
    }
    enableLiveImageBtn->setText("Stop Live Data");
  }else{
    enableLiveImageBtn->setText("Start Live Data");
    return;
  }

  while(enableLiveImageBtn->isChecked())
  {
    currentTrainData = m_trainStorage->getNextValidStorage();
    if(currentTrainData->isValid()){
      updateWidgetNoSort();
      m_trainStorage->addFreeStorage(currentTrainData);
    }else{
      m_trainStorage->addFreeStorage(currentTrainData);
      usleep(300);
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents,10);
  }
}


void ImagePreview::initWidget()
{
  if(showpixelMapHistoBtn->isChecked()){
    setLadderReadout(enLadderReadoutCB->isChecked());
    initPixelMapHisto();
  }else if(showPixelMapBtn->isChecked()){
    setLadderReadout(enLadderReadoutCB->isChecked());
    initPixelMap();
  }else if(acquireDataBtn->isChecked()){
    setLadderReadout(false);
    updateHistoLayout();
  }else if(showContentGraphBtn->isChecked()){
    setLadderReadout(false);
  }
}


void ImagePreview::start(TrainDataRingStorage *trainStorage)
{
  setDataReceiver(trainStorage);
  enableLiveImageBtn->setChecked(true);

  startLiveData();
}

void ImagePreview::stop()
{
  enableLiveImageBtn->setChecked(false);
}


void ImagePreview::setFileName()
{
  const bool histo         = showpixelMapHistoBtn->isChecked();
  const bool hexagons      = showHoneycombCB->isChecked();
  const bool errors        = showRMSCB->isChecked();
  const bool subtract      = subtractBaselineCB->isChecked();

  fileName = (hexagons)? "hexelMap" : "pixelMap";
  if(subtract) fileName += "_baseline";
  if(errors) fileName += "_rms";
  if(histo) fileName += "_histo";
  fileName += ".png";
}


void ImagePreview::updateWidget()
{
  setFileName();

  if(showPixelMapBtn->isChecked()){
    updatePixelMap();
  }else if(showpixelMapHistoBtn->isChecked()){
    updatePixelMapHisto();
  }else if(acquireDataBtn->isChecked()){
    updateHistograms();
  }else if(showContentGraphBtn->isChecked()){
    showContentGraph();
  }
}


void ImagePreview::updateWidgetNoSort()
{
  setFileName();

  if(showPixelMapBtn->isChecked()){
    updatePixelMapNoSort();
  }
}


bool ImagePreview::isLadderReadout()
{
  return ladderMode;
}


void ImagePreview::setLadderMode(bool enable)
{
  enLadderReadoutCB->setChecked(enable);
}


void ImagePreview::setLadderReadout(bool enable)
{
  if(enable != ladderMode)
  {
    if(enable)
      cout << "Enableing Ladder Readout ... please wait" << endl;

    GuiEnableKeeper keeper(this);

    PRCEVENTS;

    delete PixelMap;
    delete PixelMap_Hexagons;

    int xPixelNum = (enable)? 512 : 64;
    int yPixelNum =( (enable)? 16*4096 : 4096 ) / xPixelNum;

    PixelMap = new TH2D("Pixel Map","Pixel Map", xPixelNum, 0, xPixelNum, yPixelNum, 0, yPixelNum);
    PixelMap_Hexagons = DataAnalyzer::createHexagonPixelMap(xPixelNum,yPixelNum);
    PixelMap_Hexagons->SetTitle("HexPixel Map");

    PRCEVENTS;

    ladderMode = enable;
  }
  if(ladderMode){
    DsscPacketReceiverSimple::setLadderReadout(true);
    DsscPacketReceiverSimple::setSendingAsics(0xFFFF);
  }else{
    DsscPacketReceiverSimple::setLadderReadout(false);
    DsscPacketReceiverSimple::setSendingAsics(1<<11);
  }
}


void ImagePreview::initPixelMap()
{
  sramLabelBox->hide();
  histoWidget->hide();
  PixelMapWidget->show();
  showHisto = false;

  PixelMap->Reset();
  PixelMap_Hexagons->Reset("");

  gStyle->SetPalette(colorMapStyleSB->value());
  gStyle->SetOptStat(0);
  PixelMapRootWidget->GetCanvas()->SetLogy(0);
}


void ImagePreview::initPixelMapHisto()
{
  const bool errors   = showRMSCB->isChecked();
  const bool subtract = subtractBaselineCB->isChecked();
  const bool logscale = enLogScaleCB->isChecked();

  sramLabelBox->hide();
  histoWidget->hide();
  PixelMapWidget->show();
  showHisto = false;

  gStyle->SetOptStat(1111);

  histoToShow = (errors || subtract)? pixelMapRMSHisto : pixelMapHisto;

  histoToShow -> Reset("M");
  histoToShow -> SetFillColor(kBlue);

  if(errors){
    histoToShow -> SetName("Values");
    histoToShow -> SetTitle("PixelMapHisto;RMS;CNT");
  }else if(subtract){
    histoToShow -> SetName("Baseline Subtracted");
    histoToShow -> SetTitle("PixelMapHisto;ADU;CNT");
  }else{
    histoToShow -> SetName("Mean ADU");
    histoToShow -> SetTitle("PixelMapHisto;Mean ADU;CNT");
  }

  if(logscale){
    PixelMapRootWidget->GetCanvas()->SetLogy(1);
  }else{
    PixelMapRootWidget->GetCanvas()->SetLogy(0);
  }
}


void ImagePreview::updatePixelMapNoSort()
{
  const bool hexagons = showHoneycombCB->isChecked();

  updateInfoLabels();

  uint numAsics = ladderMode? 16 : 1;

  double minValue = 200;
  double maxValue = 0;

  for(uint asic=0; asic<numAsics; asic++)
  {
    auto asicDataArray = currentTrainData->getAsicDataArray(asic,0);
    uint asicColOffs = asic%8 * 64 + 1;
    uint asicRowOffs = asic/8 * 64 + 1;
    for(uint idx = 0; idx<utils::s_numAsicPixels; idx++){
      int value = asicDataArray[idx];
      minValue = (minValue > value)? value : minValue;
      maxValue = (maxValue < value)? value : maxValue;
      int col = idx%64;
      int row = idx/64;
      PixelMap->SetBinContent(asicColOffs+col,asicRowOffs+row,value);
    }
  }

  if(hexagons){
    updateMap(PixelMap_Hexagons,minValue,maxValue);
  }else{
    updateMap(PixelMap,minValue,maxValue);
  }
}



void ImagePreview::updatePixelMap()
{
  const bool hexagons = showHoneycombCB->isChecked();

  updateInfoLabels();

  const auto pixelData = getPixelData();
  auto minMaxIt = std::minmax_element(pixelData.begin(),pixelData.end());
  double minValue = *minMaxIt.first;
  double maxValue = *minMaxIt.second;

  int endASIC   = (pixelData.size() > 4096)? 15 : 0;
  for(int asic = 0; asic <= endASIC; asic++)
  {
    int startIdx = asic * 4096;
    int asicRow = (asic>7)? 64 : 0;
    int asicCol = (asic%8) * 64;

    for(int pixel =0; pixel <4096; pixel++)
    {
      auto dataToPlot = pixelData[startIdx + pixel];
      int col = asicCol + pixel%64 + 1;
      int row = asicRow + pixel/64 + 1;

      if(hexagons){
        const double hex_x = ((row % 2) ? hex_b+col*hex_b : hex_b/2+col*hex_b);
        const double hex_y = (hex_a+row*hex_c);
        PixelMap_Hexagons->SetBinContent(hex_x,hex_y,dataToPlot);
      }else{
        PixelMap->SetBinContent(col,row,dataToPlot);
      }
    }
  }

  if(hexagons){
    updateMap(PixelMap_Hexagons,minValue,maxValue);
  }else{
    updateMap(PixelMap,minValue,maxValue);
  }
}


void ImagePreview::updatePixelMapHisto()
{
  const bool resetHisto    = contResetCB->isChecked();
  if(resetHisto){
    histoToShow->Reset("M");
  }

  updateInfoLabels();

  const auto pixelData = getPixelData();

  for(const auto & dataToPlot : pixelData){
    histoToShow->Fill(dataToPlot);
  }

  updatePlotMapHisto();
}


void ImagePreview::updateMap(TH2 * map, double minValue, double maxValue)
{
  PixelMapRootWidget->GetCanvas()->cd();

  if(userScaleCB->isChecked()){
    map->SetAxisRange(scaleMinSB->value(),scaleMaxSB->value(),"Z");
  }else{
    if(minValue == 0){
      minValue -= maxValue*0.01;
    }
    map->SetAxisRange(minValue,maxValue,"Z");
  }

  map->Draw("COLZ"); // draw the "color palette without text"

  PixelMapRootWidget->GetCanvas()->Update();
  PixelMapWidget->update();
}


void ImagePreview::updatePlotMapHisto()
{
  PixelMapRootWidget->GetCanvas()->cd();

  if(userScaleCB->isChecked()){
    histoToShow->SetAxisRange(scaleMinSB->value(),scaleMaxSB->value());
  }else if(enFocusCB->isChecked()){
    DataAnalyzer::setXAxisRange(histoToShow);
  }
  histoToShow->Draw();

  PixelMapRootWidget->GetCanvas()->Update();
  PixelMapWidget->update();
}


const uint16_t * ImagePreview::getPixelSramData(int pixel)
{
  uint32_t asic      = (ladderMode? getDataAsicNumber(pixel) : asicSelectSB->value());
  uint32_t asicPixel = getDataPixelNumber(pixel);

  return currentTrainData->getPixelData(asic,asicPixel);
}


const uint16_t *ImagePreview::getPixelSramData(int asic, int pixel)
{
  return currentTrainData->getPixelData(asic,pixel);
}


void ImagePreview::calcPixelNumFromCoordinates()
{
  pixelSelectSB->setValue(selStartRowSB->value()*64 + selStartColSB->value());
  imagePixelSB->setValue(getImagePixelNumber(asicSelectSB->value(),pixelSelectSB->value()));
}


void ImagePreview::calcASICPixelFromImagePixel()
{
  uint32_t dataPixel = getDataPixelNumber(imagePixelSB->value());
  uint32_t ASIC  = dataPixel / 4096;
  uint32_t pixel = dataPixel % 4096;
  uint32_t row = pixel/64;
  uint64_t col = pixel%64;

  pixelSelectSB->setValue(pixel);
  asicSelectSB->setValue(ASIC);
  selStartRowSB->setValue(row);
  selStartColSB->setValue(col);
}


uint32_t ImagePreview::getImagePixelNumber(int asic, int asicPixel)
{
  if(!isLadderReadout()) return asicPixel;

  int rowOffset = (asic>7)?  64 : 0;
  int colOffset = (asic%8) * 64;

  int row = asicPixel/64;
  int col = asicPixel%64;

  return (row + rowOffset)*512 + col + colOffset;
}


uint32_t ImagePreview::getDataPixelNumber(int pixel)
{
  if(!isLadderReadout()) return pixel;

  int row = pixel/512;
  int col = pixel%512;

  int asicRow = row/64;
  int asicCol = col/64;

  row -= asicRow*64;
  col -= asicCol*64;

  int asic = asicRow * 8 + asicCol;
  int asicPixel = row*64+col;

  return asic * numOfPxs + asicPixel;
}

uint32_t ImagePreview::getDataAsicNumber(int pixel)
{
  if(!isLadderReadout()) return pixel;

  int row = pixel/512;
  int col = pixel%512;

  int asicRow = row/64;
  int asicCol = col/64;

  row -= asicRow*64;
  col -= asicCol*64;

  int asic = asicRow * 8 + asicCol;

  return asic;
}


void ImagePreview::acquireVHoldMap()
{
  int memSramStartAddress = sramStartAddressSB->value();
  int memSramEndAddress   = sramEndAddressSB->value();

  sramStartAddressSB->setValue(0);
  sramEndAddressSB->setValue(0);

  updatePixelMap();

  sramStartAddressSB->setValue(memSramStartAddress);
  sramEndAddressSB->setValue(memSramEndAddress);
}


bool ImagePreview::eventFilter(QObject *object, QEvent *event)
{
  if(event->type()== QEvent::MouseButtonPress){

    QMouseEvent * mouseEvent = static_cast<QMouseEvent *>(event);

    lastXpos = mouseEvent->x();
    lastYpos = mouseEvent->y();

    lastXScrolPos = histoWidget->horizontalScrollBar()->value();
    lastYScrolPos = histoWidget->verticalScrollBar()->value();
  }
  else if(event->type()== QEvent::MouseMove)
  {
    static int eventCnt=0;
    eventCnt++;
    if(eventCnt%4 == 0){
      QMouseEvent * mouseEvent = static_cast<QMouseEvent *>(event);

      int diffX = lastXpos - mouseEvent->x();
      int diffY = lastYpos - mouseEvent->y();

      histoWidget->horizontalScrollBar()->setValue(lastXScrolPos+diffX);
      histoWidget->verticalScrollBar()->setValue(lastYScrolPos+diffY);
    }
  }else if(event->type() == QEvent::KeyPress){

    QKeyEvent * kevent = static_cast<QKeyEvent *>(event);

    if(kevent->key() == Qt::Key_Y){
      zoomIn();
    }
    if(kevent->key() == Qt::Key_X){
      zoomOut();
    }
  }

  return true;
}


void ImagePreview::scaleArea(double factor)
{
  histoAreaLabel->resize(factor * histoAreaLabel->size());

  adjustScrollBar(histoWidget->horizontalScrollBar(), factor);
  adjustScrollBar(histoWidget->verticalScrollBar(), factor);
  histoAreaLabel->update();
}


void ImagePreview::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
  scrollBar->setValue(int(factor * scrollBar->value()
                      + ((factor - 1) * scrollBar->pageStep()/2)));
}


void ImagePreview::changeMoreBtn()
{
  bool more = showMoreDetailsBtn->isChecked();
  QString moreText = (more)? "Show more" : "Hide params";
  showMoreDetailsBtn->setText(moreText);
}


void ImagePreview::resetLiveMapHisto()
{
  histoToShow -> Reset("M");
}


void ImagePreview::generateLayout()
{
  setWindowTitle("ImagePreview");

  QVBoxLayout *vLayout = new QVBoxLayout;
  this->setLayout(vLayout);
  histoGLayout = new QGridLayout();

  histoWidget = new QScrollArea;
  histoWidget->setLayout(histoGLayout);
  histoWidget->installEventFilter(this);
  histoWidget->setWidgetResizable(true);
  histoWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  histoAreaLabel = new QLabel(histoWidget);
  histoAreaLabel->setMinimumSize(800,500);
  histoAreaLabel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

  PixelMapGLayout = new QGridLayout();
  PixelMapWidget = new QScrollArea;
  PixelMapWidget->setLayout(PixelMapGLayout);
  PixelMapWidget->installEventFilter(this);
  PixelMapWidget->setWidgetResizable(true);
  PixelMapWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

  PixelMapRootWidget = new TQtWidget(PixelMapWidget);
  PixelMapRootWidget->GetCanvas()->cd();
  PixelMapRootWidget->GetCanvas()->Clear();

  int xPixelNum = 64;
  int yPixelNum = 4096/xPixelNum;

  pixelMapHisto    = new TH1D("Pixel Map Histo","Pixel Map Histo",512,-0.5,511.5);
  pixelMapRMSHisto = new TH1D("Pixel Map RMS Histo","Pixel Map RMS Histo",10240,-511.5,511.5);
  PixelMap = new TH2D("Pixel Map","Pixel Map", xPixelNum, 0, xPixelNum, yPixelNum, 0, yPixelNum);
  PixelMap_Hexagons = DataAnalyzer::createHexagonPixelMap(xPixelNum,yPixelNum);
  PixelMap_Hexagons->SetTitle("HexPixel Map");

  PixelMapGLayout -> addWidget(PixelMapRootWidget,0,0);

  histoAreaLabel->setLayout(histoGLayout);
  histoAreaLabel->resize(histoWidget->size().width()*0.99,histoWidget->size().height()*0.99);
  histoWidget->setWidget(histoAreaLabel);
  histoWidget->update();

  selNumPxToShowCB = new QComboBox();

  for(int num=1; num <= numOfPxs; num*=2){
    selNumPxToShowCB->addItem(QString::number(num));
  }
  selNumPxToShowCB->setCurrentIndex(0);
  selNumPxToShowCB->setMaximumWidth(60);

  asicSelectSB = new QSpinBox;
  asicSelectSB->setRange(0,15);
  asicSelectSB->setValue(10);

  enableLiveImageBtn = new QPushButton("Start Live Data");
  enableLiveImageBtn->setCheckable(true);
  enableLiveImageBtn->setChecked(false);

  contResetCB = new QCheckBox("Cont Histo Reset");
  contResetCB->setChecked(false);
  contResetCB->setVisible(false);

  resetLiveMapHistoBtn = new QPushButton("Reset LiveHisto");
  resetLiveMapHistoBtn->setVisible(false);
  connect(resetLiveMapHistoBtn,SIGNAL(clicked()),this,SLOT(resetLiveMapHisto()));

  scaleMinSB = new QSpinBox();
  scaleMinSB->setRange(-512,512);
  scaleMinSB->setValue(0);
  scaleMinSB->setToolTip("If > 0 scale is fixed to given max value");
  //scaleMinSB->hide();

  scaleMaxSB = new QSpinBox();
  scaleMaxSB->setRange(-512,512);
  scaleMaxSB->setValue(300);
  //scaleMaxSB->hide();

  userScaleCB = new QCheckBox("En UserScale");
  QLabel * fixScaleLabel = new QLabel("Scale From/To");

  vLayout -> addWidget(histoWidget);
  vLayout -> addWidget(PixelMapWidget);

  QFormLayout *mainLayout = new QFormLayout;
  sramLabelBox = new QGroupBox("SRAM Content");
  QVBoxLayout * sramLabelBoxLayout = new QVBoxLayout;
  sramLabelBox->setLayout(sramLabelBoxLayout);
  sramLabel = new QLabel;
  sramLabelBoxLayout->addWidget(sramLabel);

  sramStartAddressSB = new QSpinBox;
  sramStartAddressSB->setRange(0,sramSize-1);
  sramStartAddressSB->setValue(0);
  sramStartAddressSB->setMaximumWidth(60);
  sramEndAddressSB = new QSpinBox;
  sramEndAddressSB->setRange(0,sramSize-1);
  sramEndAddressSB->setValue(sramSize-1);
  sramEndAddressSB->setMaximumWidth(60);

  pixelSelectSB = new QSpinBox;
  pixelSelectSB->setRange(0,numOfPxs-1);
  pixelSelectSB->setMaximumWidth(60);

  selStartRowSB = new QSpinBox();
  selStartColSB = new QSpinBox();
  selStartRowSB->setRange(0,63);
  selStartColSB->setRange(0,63);
  selStartRowSB->setValue(20);
  selStartColSB->setValue(20);
  QPushButton * calcPixelFromRangeBtn = new QPushButton("Calc");
  connect(calcPixelFromRangeBtn,SIGNAL(clicked()),this,SLOT(calcPixelNumFromCoordinates()));

  imagePixelSB = new QSpinBox();
  imagePixelSB->setRange(0,65535);
  imagePixelSB->setValue(0);

  QPushButton * calcASICPixelFromImagePixelBtn = new QPushButton("Calc");
  connect(calcASICPixelFromImagePixelBtn,SIGNAL(clicked()),this,SLOT(calcASICPixelFromImagePixel()));

  QButtonGroup *plotGroup = new QButtonGroup;
  acquireDataBtn = new QRadioButton("&Acquire Data");
  showContentGraphBtn = new QRadioButton("Show Content Graph");
  showpixelMapHistoBtn = new QRadioButton("Show PixelMap Histo");
  showPixelMapBtn = new QRadioButton("&Show Pixel Map");

  plotGroup->addButton(acquireDataBtn);
  plotGroup->addButton(showContentGraphBtn);
  plotGroup->addButton(showpixelMapHistoBtn);
  plotGroup->addButton(showPixelMapBtn);

  showPixelMapBtn->setChecked(true);


  QPushButton *setBaselineBtn = new QPushButton("Set Baseline");

  showHoneycombCB = new QCheckBox("Hexagon Pixel Shape");
  showHoneycombCB->setChecked(false);

  subtractBaselineCB = new QCheckBox("Subtract Baseline");
  subtractBaselineCB->setChecked(false);

  QHBoxLayout * iprogValueLayout = new QHBoxLayout();
  iprogValueLayout->addStretch();

  //#############################################################

//   QGroupBox* discardGroup = new QGroupBox("");
//   dontDiscardRB = new QRadioButton("Don't discard");
//   discardEverySecondRB = new QRadioButton("Discard every second value");
//   dontDiscardRB->setChecked(true);
//   QHBoxLayout* discardLayout = new QHBoxLayout();
//   discardLayout->addWidget(dontDiscardRB);
//   discardLayout->addWidget(discardEverySecondRB);
//   discardLayout->addStretch();
//   discardGroup->setLayout(discardLayout);

  //#############################################################

  QGroupBox* fillSramBox = new QGroupBox("");
  QHBoxLayout* fillSRAMLayout = new QHBoxLayout();
  fillSramBox->setLayout(fillSRAMLayout);
  sramPatternSB = new QSpinBox;
  sramPatternSB->setRange(0,511);
  sramPatternSB->setValue(0xAA);
  QPushButton *fillSramBtn = new QPushButton("Fill SRAM and readout");
  fillSRAMLayout->addWidget(sramPatternSB);
  fillSRAMLayout->addWidget(fillSramBtn);
  fillSRAMLayout->addStretch();

  //#############################################################


  QGroupBox* powerDownGroup = new QGroupBox("");
  maskPowerDnPxsCB = new QCheckBox("Mask PowerDn Pxs");
  maskPowerDnPxsCB -> setChecked(true);

  powerDnPxsValueSB = new QSpinBox();
  powerDnPxsValueSB->setRange(0,511);
  powerDnPxsValueSB->setValue(40);
  powerDnPxsValueSB->setToolTip("Set PowerDown pixels on this value, to adapt pixel map colors");

  QHBoxLayout* powerDownLayout = new QHBoxLayout();
  powerDownLayout->addWidget(maskPowerDnPxsCB);
  powerDownLayout->addWidget(new QLabel("Power Down Value:"));
  powerDownLayout->addWidget(powerDnPxsValueSB);
  powerDownLayout->addStretch();
  powerDownGroup->setLayout(powerDownLayout);


  en9BitModeCB = new QCheckBox("Enable 9 bit mode");
  en9BitModeCB -> setChecked(true);

  enLogScaleCB = new QCheckBox("Enable Log Scale");
  enLogScaleCB -> setChecked(true);

  enFocusCB = new QCheckBox("Enable Focus");
  enFocusCB -> setChecked(true);

  enGCCWrapCB = new QCheckBox("En GCCWrap");
  enGCCWrapCB -> setChecked(true);
  connect(enGCCWrapCB,SIGNAL(toggled(bool)),this,SLOT(setGCCWrapMode()));

  showRMSCB = new QCheckBox("Display RMS");
  showRMSCB -> setChecked(false);

  QGridLayout *selParamsLayout = new QGridLayout;

  trailerLbl = new IPTrailerLabel(this);

  showMoreDetailsBtn = new QPushButton("Show More");
  showMoreDetailsBtn->setCheckable(true);

  enLadderReadoutCB = new QCheckBox("Ladder Readout");

  QPushButton * acquireVHoldMapBtn = new QPushButton("Acquire VHold");
  colorMapStyleSB = new QSpinBox();
  colorMapStyleSB->setRange(1,112);


  QHBoxLayout *sramLayout = new QHBoxLayout;
  sramLayout->addWidget(new QLabel("SRAM Address Range"));
  sramLayout->addWidget(sramStartAddressSB);
  sramLayout->addWidget(new QLabel(" - "));
  sramLayout->addWidget(sramEndAddressSB);
  sramLayout->addStretch();

#if ROOT_VERSION_CODE > ROOT_VERSION(6,0,0)
  colorMapStyleSB->setValue(kBird); //kBird
  sramLayout->addWidget(new QLabel("MapStyle"));
  sramLayout->addWidget(colorMapStyleSB);
#else
  colorMapStyleSB->setValue(55); //kRainBow
#endif
  sramLayout->addWidget(acquireVHoldMapBtn);
  sramLayout->addWidget(enLadderReadoutCB);
  sramLayout->addWidget(showMoreDetailsBtn);

  //##########################################################################

  QGroupBox * paramsBox = new QGroupBox;
  selParamsLayout->addWidget(new QLabel("Sel Pixel"),3,0);
  selParamsLayout->addWidget(pixelSelectSB,3,1);
  selParamsLayout->addWidget(new QLabel("Col/Row:"),3,2);
  selParamsLayout->addWidget(selStartColSB,3,3);
  selParamsLayout->addWidget(selStartRowSB,3,4);
  selParamsLayout->addWidget(calcPixelFromRangeBtn,3,5);
  selParamsLayout->addWidget(new QLabel("Num Pixels to Show"),3,6);
  selParamsLayout->addWidget(selNumPxToShowCB,3,7);

  selParamsLayout->addWidget(imagePixelSB,4,4);
  selParamsLayout->addWidget(calcASICPixelFromImagePixelBtn,4,5);
  selParamsLayout->addWidget(new QLabel("Sel ASIC to Show"),4,6);
  selParamsLayout->addWidget(asicSelectSB,4,7);
  paramsBox->setLayout(selParamsLayout);

  QHBoxLayout *specialGroupLayout = new QHBoxLayout;
  specialGroupLayout->addWidget(powerDownGroup);
  specialGroupLayout->addWidget(fillSramBox);

  //##########################################################################

  QGroupBox * optionsBox = new QGroupBox;
  QHBoxLayout* optionsLayout = new QHBoxLayout;
  QPushButton* savePlotBtn = new QPushButton("Save Plot");

  optionsLayout->addWidget(en9BitModeCB);
  optionsLayout->addWidget(enLogScaleCB);
  optionsLayout->addWidget(enFocusCB);
  optionsLayout->addWidget(enGCCWrapCB);
  optionsLayout->addWidget(savePlotBtn);
  optionsBox->setLayout(optionsLayout);

  mainLayout->addRow(sramLabelBox);
  mainLayout->addRow(trailerLbl);
  mainLayout->addRow(sramLayout);
  mainLayout->addRow(paramsBox);
  mainLayout->addRow(specialGroupLayout);
  mainLayout->addRow(optionsBox);

  QGridLayout * btnLayout = new QGridLayout;
  asicDataRB = new QRadioButton("ASIC Data");
  asicDataRB->setToolTip("Normal operation mode to receive data form ASIC");
  asicDataRB->setChecked(true);

  testPatternRB = new QRadioButton("TestPattern mode");

  iobDummyDataRB = new QRadioButton("IOB DummyData");
  iobDummyDataRB->setToolTip("Receive & Check Dummy data from IOB");

  drDummyDataRB  = new QRadioButton("DR DummyData");
  drDummyDataRB->setToolTip("Receive & Check Dummy data from DataReceiver");

  kc705DummyDataRB = new QRadioButton("KC705 DummyData");
  kc705DummyDataRB->setToolTip("Receive & Check Dummy data from 16 Dummy ASICs");

  QButtonGroup *dataModeGrp = new QButtonGroup;
  dataModeGrp->addButton(asicDataRB,1);
  dataModeGrp->addButton(testPatternRB,2);
  dataModeGrp->addButton(iobDummyDataRB,3);
  dataModeGrp->addButton(drDummyDataRB,4);
  dataModeGrp->addButton(kc705DummyDataRB,5);

  int row = 0;
  int col = 0;
  btnLayout->addWidget(acquireDataBtn,row,col++);
  btnLayout->addWidget(showContentGraphBtn,row,col++);
  btnLayout->addWidget(asicDataRB,row,col++);
  btnLayout->addWidget(testPatternRB,row,col++);
  btnLayout->addWidget(iobDummyDataRB,row,col++);

  row=1;
  col=0;

  btnLayout->addWidget(showpixelMapHistoBtn,row,col++);
  btnLayout->addWidget(showPixelMapBtn,row,col++);
  btnLayout->addWidget(drDummyDataRB,row,col++);
  btnLayout->addWidget(kc705DummyDataRB,row,col++);

  row=2;
  col=0;
  btnLayout->addWidget(fixScaleLabel,row,col++);
  btnLayout->addWidget(scaleMinSB,row,col++);
  btnLayout->addWidget(scaleMaxSB,row,col++);
  btnLayout->addWidget(userScaleCB,row,col++);

  row=3;
  col=0;
  btnLayout->addWidget(setBaselineBtn,row,col++);
  btnLayout->addWidget(showHoneycombCB,row,col++);
  btnLayout->addWidget(subtractBaselineCB,row,col++);
  btnLayout->addWidget(showRMSCB,row,col++);
  col++;
  btnLayout->addWidget(enableLiveImageBtn,row,col++);

  row=4;
  col=4;
  btnLayout->addWidget(contResetCB,row,col++);
  btnLayout->addWidget(resetLiveMapHistoBtn,row,col++);

  mainLayout->addRow(btnLayout);

  vLayout->addLayout(mainLayout);

  updateHistoLayout();

  //connect(dataModeGrp, SIGNAL(buttonClicked(int)), this, SLOT(setDataMode()));

  connect(enableLiveImageBtn,SIGNAL(clicked()),this,SLOT(startLiveData()));

  connect(showMoreDetailsBtn,SIGNAL(toggled(bool)),optionsBox,         SLOT(setHidden(bool)));
  connect(showMoreDetailsBtn,SIGNAL(toggled(bool)),paramsBox,          SLOT(setHidden(bool)));
  connect(enableLiveImageBtn,SIGNAL(toggled(bool)),asicDataRB,          SLOT(setHidden(bool)));
  connect(enableLiveImageBtn,SIGNAL(toggled(bool)),iobDummyDataRB,      SLOT(setHidden(bool)));
  connect(enableLiveImageBtn,SIGNAL(toggled(bool)),drDummyDataRB,       SLOT(setHidden(bool)));
  connect(enableLiveImageBtn,SIGNAL(toggled(bool)),kc705DummyDataRB,    SLOT(setHidden(bool)));
  connect(enableLiveImageBtn,SIGNAL(toggled(bool)),drDummyDataRB,       SLOT(setHidden(bool)));

  connect(enableLiveImageBtn,SIGNAL(toggled(bool)),contResetCB,         SLOT(setVisible(bool)));
  connect(enableLiveImageBtn,SIGNAL(toggled(bool)),resetLiveMapHistoBtn,SLOT(setVisible(bool)));
  connect(enableLiveImageBtn,SIGNAL(toggled(bool)),showMoreDetailsBtn,  SLOT(setChecked(bool)));
  connect(enableLiveImageBtn,SIGNAL(toggled(bool)),optionsBox,          SLOT(setVisible(bool)));

  connect(enableLiveImageBtn,SIGNAL(toggled(bool)),this,SLOT(initWidget()));
  connect(showMoreDetailsBtn,  SIGNAL(clicked()), this, SLOT(changeMoreBtn()));

  connect(acquireVHoldMapBtn,  SIGNAL(clicked()), this, SLOT(acquireVHoldMap()));
  connect(setBaselineBtn,      SIGNAL(clicked()), this, SLOT(setBaseline()));
  connect(savePlotBtn,         SIGNAL(clicked()), this, SLOT(savePlot()));

  connect(asicSelectSB,        SIGNAL(valueChanged(int)),this, SLOT(setASICDataToShow()));
  connect(enLadderReadoutCB,    SIGNAL(toggled(bool)), this, SLOT(setLadderReadout(bool)));

  connect(plotGroup,SIGNAL(buttonClicked(int)),this,SLOT(initWidget()));

  showMoreDetailsBtn->setChecked(true);
}
