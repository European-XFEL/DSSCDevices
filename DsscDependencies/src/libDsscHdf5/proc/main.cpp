#include <QStringList>
#include <QApplication>
#include <QDebug>

#include "../DsscHDF5ReceiveWidget.h"
#include "../DsscHDF5Writer.h"
#include "../DsscHDF5CalibrationDB.h"
#include "../DsscHDF5TrainDataReader.h"
#include "../DsscHDF5Writer.h"
#include "../DsscHDF5TrimmingDataReader.h"
#include "../DsscHDF5TrimmingDataWriter.h"
#include "../../libCHIPInterface/CHIPTrimmer.h"

using namespace std;

void printInfo()
{
  QTextStream out(stdout);
  out << "Usage is no argument\n";
  out << "DsscHDF5Receiver";
  exit(0);
}

int main(int argc, char **argv)
{
  QApplication app(argc, argv);

  QStringList args = QCoreApplication::arguments();
  if (args.contains("-h") ) {
    printInfo();
  }

  auto mainWin = new DsscHDF5ReceiveWidget(nullptr);
  mainWin->show();

  return app.exec();

/*
  string component = "DSSC_F1_Test";
  string gainInfo = "Gain_5keV";
  string version = "v1.0.0";


  // open database
  // initialize Configuration for Calibration Set
  SuS::DsscCalibrationSetConfig config;
  config.setParamValue("numPixels","4096");
  config.setParamValue("active","J");
  config.setParamValue("D0_EnMimCap","1");
  config.setParamValue("FCF_EnIntRes","2");
  config.setParamValue("FCF_EnCap","3");
  config.setParamValue("IntegrationTime","30");

  // add new Calibration Set with date time version and componentID (ASIC or Sensor...)
  SuS::CHIPTrimmer::s_calibrationDB.addNewCalibrationSetAndSetActive(SuS::DsscCalibrationDB::generateNewConfigID(component,gainInfo,version),
                                                                     config.getParameterValues());


  auto activeConfig = SuS::CHIPTrimmer::s_calibrationDB.getActiveCalibrationSet();

//  // Add Calibration Parameters as 1 dim vectors
  std::vector<double> pixelDelaySteps(4096*16,2.0);
  std::vector<double> dnlValues(4096*256,0.3);
  std::vector<double> gainCorrectionValues(4096,0.93);
  activeConfig->addCalibrationParamSet("PixelDelaySteps",pixelDelaySteps);
  activeConfig->addCalibrationParamSet("DNLValues",dnlValues);
  activeConfig->addCalibrationParamSet("GainCorrectionValues",gainCorrectionValues);
  activeConfig->addCalibrationParamSet("GainCorrectionValues",gainCorrectionValues);
  activeConfig->addCalibrationParamSet("GainCorrectionValues",gainCorrectionValues);
  activeConfig->addCalibrationParamSet("GainCorrectionValues",gainCorrectionValues);
*/

}

