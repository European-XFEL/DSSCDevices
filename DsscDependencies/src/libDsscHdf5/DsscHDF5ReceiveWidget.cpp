#include <QDebug>
#include <QGridLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QDir>
#include <QLineEdit>
#include <QFileDialog>
#include <QProgressBar>

#include "DsscHDF5ReceiveWidget.h"
#include "DsscTrainDataProcessor.h"

DsscHDF5ReceiveWidget::DsscHDF5ReceiveWidget(QWidget *parent)
 : QWidget(parent),
   previewWidget(new SuS::ImagePreview(nullptr)),
   m_udpPort(8000),
   m_writeHDF5(false),
   hdf5Receiver(nullptr),
   m_fillHistoThread(nullptr)
{
  generateLayout();

  previewWidget->show();

  timer.setSingleShot(false);
  connect(&timer,SIGNAL(timeout()),this,SLOT(updateFillstands()));
}


DsscHDF5ReceiveWidget::~DsscHDF5ReceiveWidget()
{
  end();
  previewWidget->close();
  fillHistos(false);
}


void DsscHDF5ReceiveWidget::start()
{
  previewWidget->show();

  timer.start(1000);

  m_trainSorter.start();
  //writeToHDF5(m_writeHDF5);

  previewWidget->start((TrainDataRingStorage*)&m_trainSorter);
}


void DsscHDF5ReceiveWidget::writeToHDF5(bool write)
{
  m_writeHDF5 = write;

  if(!hdf5Receiver){
    hdf5Receiver = new SuS::DsscHDF5Receiver;
  }

  QString outputDir = outputDirEdit->text();
  QDir checkDir;
  if(!checkDir.exists(outputDir)){
    checkDir.mkpath(outputDir);
  }
  hdf5Receiver->setOutputDirectory(outputDir.toStdString());

  m_trainSorter.startSortThread();

  if(write){
    hdf5Receiver->start(&m_trainSorter);
  }else{
    hdf5Receiver->stop();
  }
}


void DsscHDF5ReceiveWidget::end()
{
  if(hdf5Receiver){
    hdf5Receiver->stop();
  }

  previewWidget->stop();

  usleep(2000);

  m_trainSorter.stopSortThread();
  m_trainSorter.stop();

  m_trainSorter.closeConnection();

  timer.stop();
}


void DsscHDF5ReceiveWidget::setOutputDirectory()
{
  QString path = QFileDialog::getExistingDirectory(this, tr("Open Export Directory"),
                                                QDir::currentPath(),
                                                QFileDialog::ShowDirsOnly
                                                | QFileDialog::DontResolveSymlinks);
  if(path.isEmpty()) return;

  QDir checkDir;
  if(!checkDir.exists(path)){
    checkDir.mkpath(path);
  }

  outputDirEdit->setText(path);

  if(hdf5Receiver){
    hdf5Receiver->setOutputDirectory(path.toStdString());
  }
}


void DsscHDF5ReceiveWidget::setASICToShow(int asic)
{
  previewWidget->setASICDataToShow(asic);
}


void DsscHDF5ReceiveWidget::updateFillstands()
{
  sortBar->setValue(m_trainSorter.getFillStand(DsscTrainSorter::Sort));
  recvBar->setValue(m_trainSorter.getFillStand(DsscTrainSorter::Recv));
}

void DsscHDF5ReceiveWidget::fillHistos(bool fill)
{
  m_fillHistos = fill;
  if(m_fillHistos){
    m_fillHistoThread = new std::thread(&DsscHDF5ReceiveWidget::fillHistograms,this);
  }else{
    if(m_fillHistoThread){
      if(m_fillHistoThread->joinable()){
        m_fillHistoThread->join();
      }
      delete m_fillHistoThread;
      m_fillHistoThread = nullptr;
    }
    qDebug() << "Histogram filling stopped";
  }
}

void DsscHDF5ReceiveWidget::fillHistograms()
{
  utils::DataHistoVec pixelHistograms(utils::s_totalNumPxs);
  utils::DsscTrainDataProcessor processor;
  TrainDataRingStorage * trainDataRingStorage = ((TrainDataRingStorage*)&m_trainSorter);

  while(m_fillHistos)
  {
    auto * trainData = trainDataRingStorage->getNextValidStorage();
    if(trainData==nullptr){
      m_fillHistos=false;
      fillHistosCB->setChecked(false);
      return;
    }

    processor.fillDataHistoVec(trainData,pixelHistograms,false);
    trainDataRingStorage->addFreeStorage(trainData);
  }
}


void DsscHDF5ReceiveWidget::closeEvent(QCloseEvent * event)
{
  fillHistos(false);
  end();
  previewWidget->close();
}

void DsscHDF5ReceiveWidget::generateLayout()
{
  setWindowTitle("DsscHDF5ReceiveWidget");

  QGridLayout * mainGridLayout = new QGridLayout;
  setLayout(mainGridLayout);

  QPushButton * connectBtn = new QPushButton("Connect");
  QPushButton * disconnectBtn = new QPushButton("Disconnect");

  QSpinBox * portSB = new QSpinBox;
  portSB->setRange(1,100000);
  portSB->setValue(m_udpPort);

  auto storeToHDF5CB = new QCheckBox("Write to HDF5");
  storeToHDF5CB->setChecked(m_writeHDF5);
  connect(storeToHDF5CB,SIGNAL(toggled(bool)),this,SLOT(writeToHDF5(bool)));

  fillHistosCB = new QCheckBox("FillHistograms");
  fillHistosCB->setChecked(m_writeHDF5);
  connect(fillHistosCB,SIGNAL(toggled(bool)),this,SLOT(fillHistos(bool)));

  QPushButton * setOutputDirBtn = new QPushButton("Select Output Directory");
  {
    QString defaultDirectory = "/media/SUSdata/HDF5";
    const QDir dirInfo1(defaultDirectory);
    if(!dirInfo1.exists()){
      defaultDirectory = "/dataFast";
      const QDir dirInfo2(defaultDirectory);
      if(!dirInfo2.exists()){
        defaultDirectory = QDir::currentPath();
      }
    }
    outputDirEdit = new QLineEdit(defaultDirectory);
  }

  recvBar = new QProgressBar();
  recvBar->setMinimum(0);
  recvBar->setMaximum(10);


  sortBar = new QProgressBar();
  sortBar->setMinimum(0);
  sortBar->setMaximum(3);

  int row = 0;
  mainGridLayout->addWidget(new QLabel("UDP Port"),row,0);
  mainGridLayout->addWidget(portSB,row++,1);

  mainGridLayout->addWidget(outputDirEdit,row,0,1,2);
  mainGridLayout->addWidget(setOutputDirBtn,row++,2);

  mainGridLayout->addWidget(connectBtn,row,0);
  mainGridLayout->addWidget(disconnectBtn,row++,1);

  mainGridLayout->addWidget(storeToHDF5CB,row,1);
  mainGridLayout->addWidget(fillHistosCB,row++,2);

  mainGridLayout->addWidget(new QLabel("Recv:"),row,0);
  mainGridLayout->addWidget(recvBar,row++,1);

  mainGridLayout->addWidget(new QLabel("Sort:"),row,0);
  mainGridLayout->addWidget(sortBar,row++,1);

  connect(connectBtn,SIGNAL(clicked()),this,SLOT(start()));
  connect(disconnectBtn,SIGNAL(clicked()),this,SLOT(end()));
  connect(setOutputDirBtn,SIGNAL(clicked()),this,SLOT(setOutputDirectory()));
  connect(portSB,SIGNAL(valueChanged(int)),this,SLOT(setPort(int)));
}

