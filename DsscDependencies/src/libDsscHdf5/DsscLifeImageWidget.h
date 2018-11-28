
#ifndef DSSCLIVEIMAGEWIDGET
#define DSSCLIVEIMAGEWIDGET

#include

class DsscLifeImageWidget : QWidget
{
public:
  DsscLifeImageWidget();
  
  void update();
  
  
private:
  
  TQtWidget * plotwidget;
  
};

#endif 
