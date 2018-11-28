#ifndef QDSSCHDF5PACKERWIDGET_H
#define QDSSCHDF5PACKERWIDGET_H

#include <vector>
#include <string>

#include <QWidget>
#include <QString>
#include <QMainWindow>
#include <QLabel>
#include <QCheckBox>

class QLineEdit;
class QGroupBox;
class QListWidget;
class QSpinBox;
class QCloseEvent;



class QDsscHDF5PackerWidget : public QMainWindow
{
    Q_OBJECT

  public:
    QDsscHDF5PackerWidget(QWidget *parent = 0);
    virtual ~QDsscHDF5PackerWidget();

  public slots:

    void loadDirectory();
    void exportToRoot();
    void loadFullConfig();
    void storeHDF5ConfigFile();
    void addFile();
    void removeFile();
    void moveFileDown();
    void moveFileUp();
    void openDataAnalyzer();
    void updateDataPackerPixelOffsetsMap();
    void printHDF5FileStructure();

  private:
    void setConfigDir();
    void initFromFirstFile(const QString &fileName);
    void generateLayout();
    void updateStatusBar();
    int getNumberOfActiveAsics(){return (ladderModeEnCB->isChecked()? 16 : 1);}
    int getDataPixelNumber(int pixel);
    void closeEvent(QCloseEvent * event);

    static constexpr int numOfPxs = 4096;
    static constexpr int sramSize = 800;

    QString outdir;
    QString configDir;
    QCheckBox *ladderModeEnCB;
    QLineEdit *pixelToPackEdit;
    QLineEdit *iterationsEdit;
    QLineEdit *configPathEdit;
    QLineEdit *settingsEdit;
    QLineEdit *sweepParameterNameEdit;
    QLineEdit *exportDirectoryEdit;
    QListWidget *fileSelectListEdit;

    QGroupBox * outFileBox;
    QLabel   *outputFileLbl;
};


#endif // QDSSCHDF5PACKERWIDGET_H
