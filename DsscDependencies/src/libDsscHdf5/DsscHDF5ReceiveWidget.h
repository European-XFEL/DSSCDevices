

#ifndef DSSCHDF5RECEIVEWIDGET_H
#define DSSCHDF5RECEIVEWIDGET_H

#include <QTimer>
#include <QProgressBar>
#include <QWidget>
#include "DsscHDF5Receiver.h"
#include "DsscTrainSorter.h"
#include "ImagePreview.h"
/**
  \brief This is the GUI for the dataAnalyzer

  \author Manfred Kirchgessner
  */
class QLineEdit;

class DsscHDF5ReceiveWidget : public QWidget
{
  Q_OBJECT

  public:

    DsscHDF5ReceiveWidget(QWidget* parent = nullptr);
    ~DsscHDF5ReceiveWidget();

  public slots:

    void setPort(int port){ m_udpPort = port; m_trainSorter.setUDPPort(m_udpPort);}
    void setOutputDirectory();

    void writeToHDF5(bool write);
    void fillHistos(bool fill);
    void start();
    void end();

    void setASICToShow(int asic);
    void updateFillstands();

  private:
    void fillHistograms();
    void closeEvent(QCloseEvent * event);

    QLineEdit * outputDirEdit;
    QCheckBox *fillHistosCB;
    SuS::ImagePreview * previewWidget;

    void generateLayout();

    int m_udpPort;
    bool m_writeHDF5;
    bool m_fillHistos;

    QTimer timer;
    SuS::DsscHDF5Receiver * hdf5Receiver;

    DsscTrainSorter m_trainSorter;
    QProgressBar * recvBar;
    QProgressBar * sortBar;
    std::thread *m_fillHistoThread;

};

#endif // DSSCHDF5RECEIVEWIDGET_H

