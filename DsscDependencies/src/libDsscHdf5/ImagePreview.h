#ifndef IMAGEPREVIEW_H
#define IMAGEPREVIEW_H

#include <QLabel>
#include <QCheckBox>
#include <QWidget>
#include <QSpinBox>

#include <stdint.h>
#include <thread>
#include <string>

#include "DsscTrainSorter.h"

class QPushButton;
class QLineEdit;
class QImage;
class QComboBox;
class QGroupBox;
class TH1;
class TH2;
class TH1I;
class TH1D;
class TH2D;
class TH2Poly;
class TQtWidget;
class QGridLayout;
class QScrollBar;
class QScrollArea;
class QRadioButton;
class QCheckBox;
class QMutex;


namespace SuS
{
  class IPTrailerLabel : public QWidget
  {
    Q_OBJECT

    public:

#ifdef NO_SORT
      using DsscTrailerData = utils::DsscAsicTrailerData;
#endif

      IPTrailerLabel(QWidget* _parent = NULL);
      virtual ~IPTrailerLabel(){}

      enum TRMODE{ASIC,DRDUMMY,IOBDUMMY,TESTPATTERN};

    public slots:
      void update(utils::DsscTrainData *trainData, int asic, TRMODE mode);

      void updateTrailer(const DsscTrailerData & trailerData, TRMODE mode);
      void updateTrainID(uint64_t trainID);
      void updateSpecific(const utils::DsscSpecificData & specificData);

    private:
      void generateLayout();

      void setXORValue();
      void setTestPattern(uint16_t value);
      void setTemperature(QLabel *tempLabel, int temp);
      void setVetoCnt(uint16_t veto_cnt);

      void updateTrailer(const DsscTrailerData & trailerData);
      void updateTrailerTestPatternMode(const DsscTrailerData & trailerData);
      void updateTrailerDummyDRMode(const DsscTrailerData & trailerData);
      void updateTrailerDummyIOBMode(const DsscTrailerData & trailerData);

      QLabel * vetoCntLbl;
      QLabel * temp0Lbl;
      QLabel * temp1Lbl;
      QLabel * xorLbl;
      QLabel * testPatternLbl;
      QLabel * trainIDLbl;
  };


  class ImagePreview : public QWidget
  {
    Q_OBJECT

    public:
      ImagePreview(QWidget* _parent = NULL);
      virtual ~ImagePreview();

      void setLadderMode(bool enable);
      void setDataReceiver(TrainDataRingStorage * trainStorage){m_trainStorage = trainStorage;}
      void updateWidget();
      void updateWidgetNoSort();
      void start(TrainDataRingStorage * trainStorage);
      void stop();

    signals:
      void drawPixelMapSignal();

    public slots:

      void initWidget();
      void updateImagePreview();
      void initHistogram();
      void setPixel(int _px);
      void setNumFramesToSend(int numFrames){sramEndAddressSB->setValue(numFrames-1);}

      void calcPixelNumFromCoordinates();
      void calcASICPixelFromImagePixel();
      uint32_t getImagePixelNumber(int asic, int dataPixel);
      uint32_t getDataPixelNumber(int pixel);
      uint32_t getDataAsicNumber(int pixel);

      void refreshPixelMap();
      void refreshPixelMapHisto();

      void setASICDataToShow();
      void setASICDataToShow(int asic);


    protected slots:
      TH1I* getHisto(int _px);
      void updateHistograms();

      void setBaseline();

      void refreshHistogram();
      void acquireVHoldMap();

      void zoomIn() { scaleArea(0.9); }
      void zoomOut(){ scaleArea(1.1); }

      void scaleArea(double factor);
      void adjustScrollBar(QScrollBar *scrollBar, double factor);
      bool eventFilter(QObject *object, QEvent *event);

      void showContentGraph();

      void histoUpdated(int px);
      void savePlot();

      const uint16_t *getPixelSramData(int pixel);
      const uint16_t *getPixelSramData(int asic, int asicPixel);

      const QVector<double> & getPixelData();

      void startLiveData();

      void setLadderReadout(bool enable);
      bool isLadderReadout();
      void setGCCWrapMode(){ enGCCWrap = enGCCWrapCB->isChecked(); }

      void changeMoreBtn();
      void resetLiveMapHisto();

    private :
      void generateLayout();
      void updateMap(TH2 * map, double minValue, double maxValue);
      void updatePlotMapHisto();

      void setFileName();

      uint32_t getStartASIC();
      uint32_t getEndASIC();

      void initRootStyle();
      void initPixelMap();
      void initPixelMapHisto();
      void initMembers();

      uint32_t getSelectedPixel();
      uint32_t convertPixelToShow(uint32_t pixel);
      inline uint16_t convertValue(uint16_t value) const;

      void updateHistogram(uint32_t pixel, TH1* histo);
      void updatePixelMap();
      void updatePixelMapNoSort();
      void updatePixelMapHisto();


      void updateInfoLabels();
      void setXAxisRange(TH1* histo);
      void updateHistoLayout();

      bool showHisto;
      bool showContentMap;

      QSpinBox * scaleMinSB;
      QSpinBox * scaleMaxSB;
      QCheckBox * userScaleCB;

      QSpinBox *pixelSelectSB;
      QSpinBox *asicSelectSB;

      QSpinBox *sramStartAddressSB;
      QSpinBox *sramEndAddressSB;

      QRadioButton* dontDiscardRB;
      QRadioButton* discardEverySecondRB;
      QCheckBox* en9BitModeCB;
      QCheckBox* enLogScaleCB;
      QCheckBox* maskPowerDnPxsCB;
      QSpinBox* powerDnPxsValueSB;
      QCheckBox *enGCCWrapCB;
      QCheckBox *showRMSCB;

      QPushButton *resetLiveMapHistoBtn;
      QPushButton * enableLiveImageBtn;
      QCheckBox *contResetCB;

      QCheckBox* enFocusCB;
      QCheckBox* showHoneycombCB;
      QCheckBox* subtractBaselineCB;

      QRadioButton* asicDataRB;
      QRadioButton* testPatternRB;
      QRadioButton* iobDummyDataRB;
      QRadioButton* drDummyDataRB;
      QRadioButton* kc705DummyDataRB;

      QVector <TQtWidget*> HistoRootWidget;
      QVector <TH1I* >histo_px;
      QVector <QMutex*> histo_px_mutex;

      QScrollArea *PixelMapWidget;
      QGridLayout *PixelMapGLayout;
      TQtWidget* PixelMapRootWidget;
      TH2D* PixelMap;
      TH2Poly* PixelMap_Hexagons;
      TH1D* pixelMapHisto;
      TH1D* pixelMapRMSHisto;
      TH1D* histoToShow;

      QVector <double> baselineValues;

      QVector<double> cols,rows;

      QImage *sramImage;
      QLabel *sramLabel;
      QGroupBox *sramLabelBox;
      QCheckBox *enLadderReadoutCB;
      QPushButton *showMoreDetailsBtn;

      QSpinBox* selStartRowSB;
      QSpinBox* selStartColSB;

      QSpinBox* colorMapStyleSB;

      void drawGrid(int cols, int rows, int scalefac);

      QRadioButton *acquireDataBtn;
      QRadioButton *showContentGraphBtn;
      QRadioButton *showpixelMapHistoBtn;
      QRadioButton *showPixelMapBtn;

      QSpinBox *sramPatternSB;
      QComboBox *selNumPxToShowCB;
      QScrollArea *histoWidget; // contains all histoWidgets
      QGridLayout * histoGLayout;
      QLabel *histoAreaLabel;
      QSpinBox *imagePixelSB;

      QGroupBox *vholdBox;
      QGridLayout *vholdGridLayout;

      QLabel *vHoldStringLabel;

      std::thread liveMapThread;

      std::string fileName;

      int numPxsToShow;
      int numOfPxs;
      int sramSize;

      int lastXpos;
      int lastYpos;
      int lastXScrolPos;
      int lastYScrolPos;

      //Coordinate system for hexagon pixel map:
      //Pixel dimensions (a,b,c) are taken from the DSSC interface document by P. Lechner
      //lower left pixel center (pixel 0) is at (b, a). Left edge is at b/2, lower edge is at 0
      //pixel 64 (one row above) center is at (b/2, a+c). Left edge is at 0, lower edge is at a
      double hex_a = 0.136;
      double hex_b = 0.236;
      double hex_c = 0.204;

      bool enGCCWrap;
      int activeAsic;

      IPTrailerLabel *trailerLbl;

      class GuiEnableKeeper{
        public:
          GuiEnableKeeper(ImagePreview * sramGui)
           : m_gui(sramGui)
          {
            m_gui->setDisabled(true);
          }

          ~GuiEnableKeeper(){
            m_gui->setDisabled(false);
          }

        private:
          ImagePreview *m_gui;
      };
      bool ladderMode;

      TrainDataRingStorage *m_trainStorage;
      utils::DsscTrainData * currentTrainData;
  };

}

#endif
