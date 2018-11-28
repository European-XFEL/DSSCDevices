#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QGroupBox>
#include <QDir>
#include <QString>
#include <QSpinBox>
#include <QLabel>
#include <QFrame>
#include <QStatusBar>
#include <QDebug>
#include <QMessageBox>

#include <string>

#include "utils.h"
#include "QConfigRegWidget.h"
#include "DataPacker.h"
#include "DsscHDF5Packer.h"
#include "DsscHDF5Reader.h"
#include "QDsscHDF5PackerWidget.h"
#include "SelASICPixelsWidget.h"


QDsscHDF5PackerWidget::QDsscHDF5PackerWidget(QWidget *parent)
  : QMainWindow(parent),outdir(QDir::currentPath())
{
  generateLayout();
  updateStatusBar();
  updateDataPackerPixelOffsetsMap();

  SuS::QConfigRegWidget::initFromSettings(this,"QDsscHDF5PackerWidget");
  outdir = exportDirectoryEdit->text();
  setConfigDir();
}


QDsscHDF5PackerWidget::~QDsscHDF5PackerWidget()
{
}

void QDsscHDF5PackerWidget::setConfigDir()
{
  const auto configFileName = configPathEdit->text().trimmed();
  QFileInfo file(configFileName);
  if(file.exists()){
    configDir = file.absolutePath();
  }else{
    configDir = QDir::currentPath();
  }
}

void QDsscHDF5PackerWidget::closeEvent(QCloseEvent * event)
{
  SuS::QConfigRegWidget::saveSettings(this,"QDsscHDF5PackerWidget");
}


void QDsscHDF5PackerWidget::loadDirectory()
{  
  QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                  outdir,
                                                  QFileDialog::ShowDirsOnly
                                                  | QFileDialog::DontResolveSymlinks);
  if(dir.isEmpty()) return;

  outdir = dir;
  exportDirectoryEdit->setText(dir);
  fileSelectListEdit->addItem(dir);
  updateStatusBar();
}


void QDsscHDF5PackerWidget::updateStatusBar()
{
  if(pixelToPackEdit->text().isEmpty()){
    statusBar()->showMessage("Define pixels to export");
  }else if(!QFile::exists(configPathEdit->text().trimmed())){
    statusBar()->showMessage("Select full config file before export");
  }else if(!QFile::exists(exportDirectoryEdit->text().trimmed())){
    statusBar()->showMessage("Select HDF5 Directory before export");
  }else{
    statusBar()->showMessage("Ready to export");
  }
}


void QDsscHDF5PackerWidget::exportToRoot()
{
  outputFileLbl->setText("exporting ... ");

  QString fileName =  QFileDialog::getSaveFileName(this, tr("Save Root File"),
                                                   outdir,
                                                   tr("root files (*.root)"));
  if(fileName.isEmpty()) return;
  if(fileName.right(5) != ".root") fileName+=".root";

  statusBar()->showMessage("Export to " + fileName);

  if(DsscHDF5Packer::isLadderMode(exportDirectoryEdit->text().trimmed()) != ladderModeEnCB->isChecked()){
    QMessageBox::warning(this,tr("Ladder mode not correct"),
                         tr("Data in directory has different format, change Ladder mode"),
                          QMessageBox::Cancel, QMessageBox::Cancel);
    return;
  }

  DsscHDF5Packer hdf5Packer(fileName,
                            pixelToPackEdit->text().trimmed(),
                            configPathEdit->text().trimmed(),
                            exportDirectoryEdit->text().trimmed());

  if(!hdf5Packer.isValid()){
    QMessageBox::critical(this,tr("configuration error"),
                         tr("Could not load configuration file"),
                          QMessageBox::Cancel, QMessageBox::Cancel);
    return;
  }
  connect(&hdf5Packer,SIGNAL(updateStatus(QString,int)),statusBar(),SLOT(showMessage(QString,int)));
  if(hdf5Packer.exportToRoot()){
    statusBar()->showMessage("Export successful");
  }else{
    statusBar()->showMessage("Errors occured during export");
  }

  outputFileLbl->setText(fileName);
  outFileBox->setVisible(true);
}

void QDsscHDF5PackerWidget::openDataAnalyzer()
{
  auto outfileName = outputFileLbl->text();
  if(outfileName.isEmpty()) return;

  QString cmdStr = "./bin/DataAnalyzer -o " + outfileName + " &";
  system(cmdStr.toStdString().c_str());
}


void QDsscHDF5PackerWidget::loadFullConfig()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open Full Config File"),
                                                  configDir,
                                                  tr("config (*.conf)"));
  if(fileName.isEmpty()) return;
  if(fileName.right(5) != ".conf") fileName+=".conf";

  configPathEdit->setText(fileName);
  updateStatusBar();
}

void QDsscHDF5PackerWidget::storeHDF5ConfigFile()
{
  QString saveToFile =  QFileDialog::getSaveFileName(this, tr("Save HDF5 Run File"),
                                                           outdir,
                                                           tr("hdf5 files (*.h5)"));
  const QString fullConfigPath = configPathEdit->text();

  if(saveToFile.isEmpty()) return;

  if(saveToFile.right(3) != ".h5") saveToFile+=".h5";

  if(fullConfigPath.isEmpty()){
    statusBar()->showMessage("Select full config file before export");
    return;
  }

  DsscHDF5Packer::saveDsscHDF5ConfigFile(saveToFile,fullConfigPath);

  statusBar()->showMessage("Runfile Export successful");

}

void QDsscHDF5PackerWidget::addFile()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                  QDir::currentPath(),
                                                  tr("Images (*_pxRegs.xml *.txt)"));
  if(fileName.isEmpty()) return;
  fileSelectListEdit->addItem(fileName);
}

void QDsscHDF5PackerWidget::removeFile()
{
  QList<QListWidgetItem*> selItems = fileSelectListEdit->selectedItems();
  for(int i=selItems.size(); i>0; --i){
    fileSelectListEdit->removeItemWidget(selItems.at(i-1));
    delete fileSelectListEdit->takeItem(fileSelectListEdit->row(selItems.at(i-1)));
  }
}

void QDsscHDF5PackerWidget::moveFileDown()
{
  int currentRow = fileSelectListEdit->currentRow();
  if (currentRow > fileSelectListEdit->count()-1) return;
  QListWidgetItem * currentItem = fileSelectListEdit->takeItem(currentRow);
  fileSelectListEdit->insertItem(currentRow + 1, currentItem);
  fileSelectListEdit->setCurrentRow(currentRow + 1);
}

void QDsscHDF5PackerWidget::moveFileUp()
{
  int currentRow = fileSelectListEdit->currentRow();
  if (currentRow == 0) return;
  QListWidgetItem * currentItem = fileSelectListEdit->takeItem(currentRow);
  fileSelectListEdit->insertItem(currentRow - 1, currentItem);
  fileSelectListEdit->setCurrentRow(currentRow - 1);
}


void QDsscHDF5PackerWidget::updateDataPackerPixelOffsetsMap()
{
  qDebug() << "++++ Update data packer pixels offset map";
  DataPacker::dataPixelsOffsetMap.clear();
  for(int px = 0; px < getNumberOfActiveAsics() * numOfPxs; px++){
    DataPacker::dataPixelsOffsetMap[px] = getDataPixelNumber(px)*sramSize;
  }
}


int QDsscHDF5PackerWidget::getDataPixelNumber(int pixel)
{
  if(!ladderModeEnCB->isChecked()) return pixel;

  if(getNumberOfActiveAsics() != 16 ) return pixel;

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

void QDsscHDF5PackerWidget::printHDF5FileStructure()
{
  QString fileName =  QFileDialog::getOpenFileName(this, tr("Pic Existing HDF5 File"),
                                                   outdir,
                                                   tr("root files (*.h5)"));
  if(fileName.isEmpty()) return;
  if(fileName.right(3) != ".h5") fileName+=".h5";

  DsscHDF5Reader::printStructure(fileName.toStdString());
  statusBar()->showMessage("Check Output in Terminal");
}


void QDsscHDF5PackerWidget::generateLayout()
{
  auto mainWidget = new QWidget;
  auto mainFormLayout = new QFormLayout;
  mainWidget->setLayout(mainFormLayout);

  setCentralWidget(mainWidget);

  auto selAsicPixelsWidget = new SelASICPixelsWidget();

  ladderModeEnCB  = new QCheckBox;
  pixelToPackEdit = new QLineEdit("");
  iterationsEdit  = new QLineEdit("");
  settingsEdit    = new QLineEdit("");
  configPathEdit  = new QLineEdit("./session.conf");
  sweepParameterNameEdit = new QLineEdit("*");
  exportDirectoryEdit    = new QLineEdit("*");
  fileSelectListEdit     = new QListWidget;

  ladderModeEnCB         -> setAccessibleName("LadderModeEnCB");
  pixelToPackEdit        -> setAccessibleName("PixelToPackEdit");
  iterationsEdit         -> setAccessibleName("IterationsEdit");
  settingsEdit           -> setAccessibleName("SettingsEdit");
  configPathEdit         -> setAccessibleName("ConfigPathEdit");
  sweepParameterNameEdit -> setAccessibleName("SweepParameterNameEdit");
  exportDirectoryEdit    -> setAccessibleName("ExportDirectoryEdit");

  connect(selAsicPixelsWidget,SIGNAL(pixelsCalculated(QString)),pixelToPackEdit,SLOT(setText(QString)));

  auto remFileBtn = new QPushButton("removeFile");
  auto addFileBtn = new QPushButton("addFile");
  auto moveFileUpBtn = new QPushButton("move up");
  auto moveFileDownBtn = new QPushButton("move down");

  auto fileEditLayout = new QHBoxLayout;

  fileEditLayout->addWidget(addFileBtn);
  fileEditLayout->addWidget(remFileBtn);
  fileEditLayout->addWidget(moveFileDownBtn);
  fileEditLayout->addWidget(moveFileUpBtn);

  connect(addFileBtn,SIGNAL(clicked(bool)),this,SLOT(addFile()));
  connect(remFileBtn,SIGNAL(clicked(bool)),this,SLOT(removeFile()));
  connect(moveFileDownBtn,SIGNAL(clicked(bool)),this,SLOT(moveFileDown()));
  connect(moveFileUpBtn,SIGNAL(clicked(bool)),this,SLOT(moveFileUp()));

  auto loadConfigBtn = new QPushButton("Load Config");
  connect(loadConfigBtn,SIGNAL(clicked(bool)),this,SLOT(loadFullConfig()));


  connect(ladderModeEnCB,SIGNAL(toggled(bool)),selAsicPixelsWidget,SLOT(setLadderMode(bool)));
  connect(ladderModeEnCB,SIGNAL(toggled(bool)),this,SLOT(update()));
  connect(ladderModeEnCB,SIGNAL(toggled(bool)),this,SLOT(updateDataPackerPixelOffsetsMap()));

  auto showStructureBtn = new QPushButton("Print Structure of HDF5 File");
  connect(showStructureBtn,SIGNAL(clicked(bool)),this,SLOT(printHDF5FileStructure()));

  auto ladderModeLayout = new QHBoxLayout;
  ladderModeLayout->addWidget(new QLabel("Ladder Mode"));
  ladderModeLayout->addWidget(ladderModeEnCB);
  ladderModeLayout->addStretch();
  ladderModeLayout->addWidget(showStructureBtn);

  mainFormLayout->addRow(ladderModeLayout);
  selAsicPixelsWidget->setLadderMode(ladderModeEnCB->isChecked());

  // ----------------------------------------------- //
  auto selPixelsBtn = new QPushButton("Pixel Selection Tool v");
  selPixelsBtn->setCheckable(true);
  selPixelsBtn->setChecked(false);
  connect(selPixelsBtn,SIGNAL(toggled(bool)),selAsicPixelsWidget,SLOT(setVisible(bool)));

  auto selPixelsBox = new QGroupBox;
  auto selPixelsBoxLayout = new QFormLayout;
  selPixelsBox ->setLayout(selPixelsBoxLayout);

  selPixelsBoxLayout->addRow(selPixelsBtn);
  selPixelsBoxLayout->addRow(selAsicPixelsWidget);
  selPixelsBoxLayout->addRow("Pixels To Export",pixelToPackEdit);

  mainFormLayout->addRow(selPixelsBox);

  // ----------------------------------------------- //

  auto selConfigBox = new QGroupBox;
  auto selConfigBoxLayout = new QHBoxLayout;
  selConfigBox ->setLayout(selConfigBoxLayout);

  selPixelsBoxLayout->addRow(selPixelsBtn);
  selPixelsBoxLayout->addRow(selAsicPixelsWidget);
  selPixelsBoxLayout->addRow("Pixels To Export",pixelToPackEdit);
  selConfigBoxLayout->addWidget(new QLabel("Configuration"));
  selConfigBoxLayout->addWidget(configPathEdit);
  selConfigBoxLayout->addWidget(loadConfigBtn);

  mainFormLayout->addRow(selConfigBox);

  auto exportRunLayout = new QHBoxLayout;

  auto exportRunFileBtn = new QPushButton("Export HDF5 RunFile");
  exportRunLayout->addWidget(exportRunFileBtn);
  exportRunLayout->addStretch();
  connect(exportRunFileBtn,SIGNAL(clicked()),this,SLOT(storeHDF5ConfigFile()));

  mainFormLayout->addRow(exportRunLayout);

  // ----------------------------------------------- //

 // mainFormLayout->addRow("Iterations",iterationsEdit);
 // mainFormLayout->addRow("Sweep Range",settingsEdit);
 // mainFormLayout->addRow("ParameterName",sweepParameterNameEdit);

  // ----------------------------------------------- //

  auto loadHDF5DirBtn = new QPushButton("Load HDF5 Directory");
  connect(loadHDF5DirBtn,SIGNAL(clicked(bool)),this,SLOT(loadDirectory()));

  auto selDirBox = new QGroupBox;
  auto selDirBoxLayout = new QHBoxLayout;
  selDirBox ->setLayout(selDirBoxLayout);
  selDirBoxLayout->addWidget(new QLabel("H5 Files Directory"));
  selDirBoxLayout->addWidget(exportDirectoryEdit);
  selDirBoxLayout->addWidget(loadHDF5DirBtn);

  mainFormLayout->addRow(selDirBox);

  //mainFormLayout->addRow(fileSelectListEdit);
  //mainFormLayout->addRow(fileEditLayout);
  // ----------------------------------------------- //

  auto packToRootBtn = new QPushButton("Export to root");

  connect(packToRootBtn,SIGNAL(clicked(bool)),this,SLOT(exportToRoot()));
  mainFormLayout->addRow(packToRootBtn);

  // ----------------------------------------------- //

  outFileBox = new QGroupBox("Export done");
  auto outFileBoxLayout = new QHBoxLayout;
  outFileBox ->setLayout(outFileBoxLayout);
  outputFileLbl = new QLabel("waiting for export");
  outFileBox->setVisible(false);

  auto openDataAnalyzerBtn = new QPushButton("Open in DataAnalyzer");
  connect(openDataAnalyzerBtn,SIGNAL(clicked(bool)),this,SLOT(openDataAnalyzer()));

  outFileBoxLayout->addWidget(outputFileLbl);
  outFileBoxLayout->addWidget(openDataAnalyzerBtn);
  mainFormLayout->addRow(outFileBox);

  // ----------------------------------------------- //
  centralWidget()->setMinimumWidth(650);

}
