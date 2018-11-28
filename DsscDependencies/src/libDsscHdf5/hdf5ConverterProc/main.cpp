#include "../QDsscHDF5PackerWidget.h"
#include <QStringList>
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  auto mainWin = new QDsscHDF5PackerWidget();
  mainWin->show();     

  return app.exec();
}

