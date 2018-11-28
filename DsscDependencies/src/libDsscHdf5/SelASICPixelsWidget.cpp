#include <QGridLayout>
#include <QPushButton>
#include <QFrame>
#include <QDebug>
#include <QEventLoop>

#include "SelASICPixelsWidget.h"
#include "utils.h"

bool SelASICPixelsWidget::ladderMode = true;

SelASICPixelsWidget::SelASICPixelsWidget()
{
  generateLayout();
}

void SelASICPixelsWidget::generateLayout(){
  // ========================================================================== Sel ASIC Pixels Widget ===

  QGridLayout *selAsicPixelsLayout = new QGridLayout;
  this->hide();
  this->setLayout(selAsicPixelsLayout);
  this->setWindowTitle(tr("Select ASIC Pixels"));

  selASICEdit       = new QLineEdit("0-15");
  selASICPixelsEdit = new QLineEdit("0-4095");
  selColumnsEdit    = new QLineEdit("all");
  showPixelsEdit    = new QLineEdit("all");

  selStartRowSB = new QSpinBox();
  selStartColSB = new QSpinBox();
  selSizeSB     = new QSpinBox();
  selStartRowSB->setRange(0,63);
  selStartColSB->setRange(0,63);
  selSizeSB    ->setRange(0,63);
  selStartRowSB->setValue(10);
  selStartColSB->setValue(10);
  selSizeSB->setValue(10);

  auto calcASICPixelsBtn = new QPushButton("Calc");
  auto calcWindowPixelsBtn = new QPushButton("Window");
  auto setManualPixelsBtn = new QPushButton("Set");

  outputLbl = new QLabel("-");

  int row = 0;
  int col = 0;
  selAsicPixelsLayout->addWidget(new QLabel("SelectAsic:"),row,col++);
  selAsicPixelsLayout->addWidget(selASICEdit,row,col++);

  row++;col=0;
  selAsicPixelsLayout->addWidget(new QLabel("SelectPixels:"),row,col++);
  selAsicPixelsLayout->addWidget(selASICPixelsEdit,row,col++);

  row++;col=0;
  selAsicPixelsLayout->addWidget(new QLabel("Seclect Columns:"),row,col++);
  selAsicPixelsLayout->addWidget(selColumnsEdit,row,col++);
  selAsicPixelsLayout->addWidget(calcASICPixelsBtn,row,col++);


  row++;col=0;
  QFrame * f = new QFrame();f->setFrameStyle( QFrame::HLine | QFrame::Sunken );f->setMinimumHeight( 12 );
  selAsicPixelsLayout->addWidget(f,row,col++,1,3);

  row++;col=0;
  selAsicPixelsLayout->addWidget(new QLabel("Window selection:"),row,col++);

  row++;col=0;
  selAsicPixelsLayout->addWidget(new QLabel("Start Row/Col:"),row,col++);
  selAsicPixelsLayout->addWidget(selStartRowSB,row,col++);
  selAsicPixelsLayout->addWidget(selStartColSB,row,col++);

  row++;col=0;
  selAsicPixelsLayout->addWidget(new QLabel("Rectangle size"),row,col++);
  selAsicPixelsLayout->addWidget(selSizeSB,row,col++);
  selAsicPixelsLayout->addWidget(calcWindowPixelsBtn,row,col++);

  row++;col=0;
  f = new QFrame();f->setFrameStyle( QFrame::HLine | QFrame::Sunken );f->setMinimumHeight( 12 );
  selAsicPixelsLayout->addWidget(f,row,col++,1,3);

  row++;col=0;
  selAsicPixelsLayout->addWidget(outputLbl,row,col++);

  row++;col=0;
  selAsicPixelsLayout->addWidget(showPixelsEdit,row,0,1,3);
  selAsicPixelsLayout->addWidget(setManualPixelsBtn,row,4,1,1);

  connect(calcASICPixelsBtn,SIGNAL(clicked()),this,SLOT(calcASICPixels()));
  connect(calcWindowPixelsBtn,SIGNAL(clicked()),this,SLOT(calcWindowPixels()));
  connect(setManualPixelsBtn,SIGNAL(clicked()),this,SLOT(setManualPixels()));

  // ========================================================================== General Layout ===
}



QString SelASICPixelsWidget::calcWindowPixels()
{
  QString selASICs = selASICEdit->text();
  if(selASICs.isEmpty()) return "";

  int windowSize = selSizeSB->value();
  int startCol = selStartColSB->value();
  int startRow = selStartRowSB->value();

  QVector<uint32_t> windowPixels;
  for(int i=0; i<windowSize; i++){
    int row = startRow + i;
    for(int j=0; j<windowSize; j++){
      int col = startCol + j;
      windowPixels.push_back(row*64+col);
    }
  }

  const auto pixelsStr     = QString::fromStdString(utils::positionVectorToList(windowPixels.toStdVector()));
  const auto asicPixels    = getASICPixels(selASICs, pixelsStr);
  const auto selPixelsText = QString::fromStdString(utils::positionVectorToList<uint32_t>(asicPixels.toStdVector()));

  showPixelsEdit->setText(selPixelsText);
  emit pixelsCalculated(selPixelsText);
  return selPixelsText;
}


QString SelASICPixelsWidget::setManualPixels()
{
  selASICEdit->setText("Manual");
  selASICPixelsEdit->setText("Manual");
  selColumnsEdit->setText("Manual");

  QString selPixelsText = showPixelsEdit->text();
  emit pixelsCalculated(selPixelsText);
  return selPixelsText;
}


QString SelASICPixelsWidget::calcASICPixels()
{
  const auto selASICs = selASICEdit->text();
  const auto pixelsStr = selASICPixelsEdit->text();
  const auto columnsStr = selColumnsEdit->text();

  if(selASICs.isEmpty()) return "";
  if(pixelsStr.isEmpty()) return "";

  std::vector<uint32_t> asicPixels;

  if(columnsStr.left(3) == "col"){
    const auto asicVec = utils::positionListToVector(selASICs.toStdString());
    asicPixels = utils::getSendingChipPartPixels(asicVec,columnsStr.toStdString(),ladderMode);
  }else{
    asicPixels = getASICPixels(selASICs, pixelsStr).toStdVector();
    if(columnsStr != "all" && columnsStr != "0-63")
    {
      const auto columnPixels = getColumnPixels(columnsStr.toStdString());
      for(int idx = asicPixels.size() -1; idx >= 0 ; --idx){
        if(std::find(columnPixels.begin(),columnPixels.end(),asicPixels[idx]) == columnPixels.end()){
          asicPixels.erase(asicPixels.begin()+idx);
        }
      }
    }
  }

  QString selPixelsText = QString::fromStdString(utils::positionVectorToList<uint32_t>(asicPixels));
  qDebug() << selPixelsText;

  showPixelsEdit->setText(selPixelsText);
  emit pixelsCalculated(selPixelsText);
  return selPixelsText;
}


QVector<uint32_t> SelASICPixelsWidget::getASICPixels(const QString &asics, const QString &pixels)
{
  auto selASICs = asics;
  auto pixelsStr = pixels;

  if(asics.left(1)  == "a"){
    selASICs = "0-15";
  }

  if(pixels.left(1)  == "a"){ pixelsStr = "0-4095";}

  const auto asicsVec  = utils::positionListToVector(selASICs.toStdString());
  const auto pixelsVec = utils::positionListToVector(pixelsStr.toStdString());

  QVector<uint32_t> asicPixels;

  for(const auto & asic : asicsVec){
    for(const auto & pixel : pixelsVec){
      asicPixels.push_back(getImagePixelNumber(asic,pixel));
    }
  }

  qSort(asicPixels.begin(),asicPixels.end());

  return asicPixels;
}


int SelASICPixelsWidget::getImagePixelNumber(int asic, int asicPixel)
{
  if(!ladderMode) return asic * numOfPxs + asicPixel;

  int rowOffset = (asic>7)?  64 : 0;
  int colOffset = (asic%8) * 64;

  int row = asicPixel/64;
  int col = asicPixel%64;

  return (row + rowOffset)*512 + col + colOffset;
}


void SelASICPixelsWidget::setLadderMode(bool en)
{
  ladderMode = en;
  if(ladderMode){
    outputLbl->setText("Output LadderMode:");
  }else{
    selASICEdit->setText("0");
    outputLbl->setText("Output SingleASIC Mode:");
  }
}


uint32_t SelASICPixelsWidget::getPixelASIC(uint32_t pixel)
{
  uint32_t row = pixel/512;
  uint32_t col = pixel%512;
  uint32_t asicRow = row/64;
  uint32_t asicCol = col/64;

  return asicRow*8+asicCol;
}


QString SelASICPixelsWidget::selectASICPixels(const std::vector<uint32_t> & availableASICs)
{
  QString selASICs = QString::fromStdString(utils::positionVectorToList(availableASICs));
  return selectASICPixels(selASICs);
}


QString SelASICPixelsWidget::selectASICPixels(const QVector<uint32_t> & availableASICs)
{
  QString selASICs = QString::fromStdString(utils::positionVectorToList(availableASICs.toStdVector()));
  return selectASICPixels(selASICs);
}


QString SelASICPixelsWidget::selectASICPixels(const QString & availableASICs)
{
  ladderMode = true;
  SelASICPixelsWidget selWidget;
  selWidget.selASICEdit->setText(availableASICs);

  selWidget.show();

  QEventLoop waitLoop;
  connect(&selWidget, SIGNAL(pixelsCalculated(QString)), &waitLoop, SLOT(quit()));
  waitLoop.exec();

  return selWidget.showPixelsEdit->text();
}


QStringList SelASICPixelsWidget::selectASICsAndPixels(const std::vector<uint32_t> & availableASICs, const std::string & selCols)
{
  QString selASICs = QString::fromStdString(utils::positionVectorToList(availableASICs));
  return selectASICsAndPixels(selASICs,QString::fromStdString(selCols));
}


QStringList SelASICPixelsWidget::selectASICsAndPixels(const QString & availableASICs, const QString & selectedColumns)
{
  ladderMode = true;

  SelASICPixelsWidget selWidget;
  selWidget.selASICEdit->setText(availableASICs);
  selWidget.selColumnsEdit->setText(selectedColumns);

  selWidget.show();

  QEventLoop waitLoop;
  connect(&selWidget, SIGNAL(pixelsCalculated(QString)), &waitLoop, SLOT(quit()));
  waitLoop.exec();

  if(selWidget.selASICEdit->text() == "Manual"){
    return {"Manual",selWidget.showPixelsEdit->text(),"Manual"};
  }

  return {selWidget.selASICEdit->text(),selWidget.selASICPixelsEdit->text(),selWidget.selColumnsEdit->text()};
}





