#ifndef SELASICPIXELSWIDGET_H
#define SELASICPIXELSWIDGET_H

#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QVector>
#include <QPair>

#include <QWidget>


#include "utils.h"

class SelASICPixelsWidget : public QWidget
{
  Q_OBJECT

  public:
    SelASICPixelsWidget();

    static QString selectASICPixels(const std::vector<uint32_t> & availableASICs);
    static QString selectASICPixels(const QVector<uint32_t> &availableASICs);
    static QString selectASICPixels(const QString & availableASICs);
    static QStringList selectASICsAndPixels(const std::vector<uint32_t> & availableASICs, const std::string & selCols = "0-63");
    static QStringList selectASICsAndPixels(const QString & availableASICs, const QString &selectedColumns = "0-63");

    template <typename TYPE>
    static QVector<uint32_t> getPixelASICs(const QVector<TYPE> & selPixels)
    {
      QVector<uint32_t> selAsics;
      QVector<bool> foundAsics(16,false);
      const auto maxPixel = *std::max_element(selPixels.begin(),selPixels.end());
      if(maxPixel < 4096){
        selAsics.fill(0,1);
      }else{
        for(const auto & pix : selPixels){
          foundAsics[getPixelASIC(pix)] = true;
        }
      }

      int asic = 0;
      for(const auto & found : foundAsics){
        if(found){
          selAsics.push_back(asic);
        }
        asic++;
      }
      return selAsics;
    }

    static uint32_t getPixelASIC(uint32_t pixel);
    static QVector<uint32_t> getASICPixels(const QString &asics, const QString &pixels);

  public slots:
    QString calcASICPixels();
    QString calcWindowPixels();
    QString setManualPixels();

    void setLadderMode(bool en);

  signals:
    void pixelsCalculated(QString);

  private:
    void generateLayout();

    static int getImagePixelNumber(int asic, int asicPixel);

    template <typename T=int>
    static std::vector<T> getPixels(const std::string & chipPart)
    {
      std::cout << "chipPart.substr(3) = " << chipPart.substr(3) << std::endl;
      return getColumnPixels<T>(chipPart.substr(3));
    }

    template <typename T=int>
    static std::vector<T> getColumnPixels(int col)
    {
      int numAsicCols = 1;
      int numColPixels = 64;
      int numRowPixels = 64;

      if(ladderMode){
        numAsicCols = 8;
        numColPixels = 128;
        numRowPixels = 512;
      }

      std::vector<T> pixels;
      for(int asicCol = 0; asicCol < numAsicCols; asicCol++){
        for (int i=0; i<numColPixels; ++i) {
          pixels.push_back((T)(col + i*numRowPixels));
        }
        col += 64;
      }
      return pixels;
    }

    template <typename T=int>
    static std::vector<T> getColumnPixels(const std::string & colsStr)
    {
      std::string remColsStr;
      auto colonPos = colsStr.find(":");
      int offset = 0;
      int count  = 0;
      if (colonPos!=std::string::npos) {
        remColsStr = colsStr.substr(0,colonPos);
        int offsetPos = colsStr.find("o");
        count  = stoi(colsStr.substr(colonPos+1,colonPos-offsetPos));
        offset = stoi(colsStr.substr(offsetPos+1));
      } else {
        remColsStr = colsStr;
        if(remColsStr.find("col") != std::string::npos){
          remColsStr = remColsStr.substr(3);
        }
      }
      std::vector<int> cols = utils::positionListToVector<int>(remColsStr);
      std::vector<T> pixels;
      for (auto c: cols) {
        std::vector<T> colPxs = getColumnPixels<T>(c);
        pixels.insert(pixels.end(),colPxs.begin()+offset,
                      //(count==0 || count+offset>colPxs.size() ? colPxs.end()
                      (count==0 ? colPxs.end() : colPxs.begin()+offset+count)
                      );
      }
      return pixels;
    }


    QLineEdit *selASICEdit;
    QLineEdit *selASICPixelsEdit;
    QLineEdit *showPixelsEdit;
    QLineEdit *selColumnsEdit;

    QSpinBox *selStartRowSB;
    QSpinBox *selStartColSB;
    QSpinBox *selSizeSB;

    QLabel   *outputLbl;

    static bool ladderMode;

    static constexpr int numOfPxs = 4096;
};

#endif // SELASICPIXELSWIDGET_H
