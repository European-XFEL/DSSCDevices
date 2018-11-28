#ifndef F2CHIPINTERFACE_H
#define F2CHIPINTERFACE_H

#include "CHIPInterface.h"

namespace SuS {

class F2CHIPInterface : public CHIPInterface
{
  public:
    F2CHIPInterface();
    F2CHIPInterface(CHIPFullConfig *fullConfig);


    bool checkF2Registers();

    void disableInjectionBits() override;
    void setInjectionMode(InjectionMode mode) override;
    void enableInjection(bool en, const std::string & pixelStr, bool program = true) override;
    void setInjectionDAC(int setting) override;

    int getInjectionDACsetting() override;

    void setPowerDownBits(const std::string & pixelStr, bool powerDown) override;
    void enableMonBusCols(const std::vector<int> & cols) override;
    void enableMonBusCols(std::string colsStr) override;
    void enableMonBusForPixels(const std::vector<int> & pixels) override;
    void disableMonBusCols() override;

    void setPixelInjectionGain(const std::string & pixelStr,PXINJGAIN injGain, bool program) override;

    bool calibrateCurrCompDAC(bool log, int singlePx, int startSetting, int defaultValue);

    const std::vector<uint32_t> getPoweredPixelsFromConfig() override;

    void setGlobalDecCapSetting(DECCAPSETTING newSetting) override;

    bool checkD0Mode() override {return true;}
    bool checkBypCompression() override {return true;}

    void setExtLatchMode() override;
    void setIntDACMode() override;
    void setNormalMode() override;
    void setPixelInjectionMode()override;
    
    virtual std::vector<InjectionMode> getActiveModes() const override {
        return {CURRENT_BGDAC,CURRENT_SUSDAC,
                CHARGE_PXINJ_BGDAC,CHARGE_PXINJ_SUSDAC,CHARGE_BUSINJ,
                ADC_INJ,EXT_LATCH,NORM};
      }

    virtual std::vector<std::string> getInjectionModes() override {
      std::vector<InjectionMode> activeModes {CURRENT_BGDAC,CURRENT_SUSDAC,
                                              CHARGE_PXINJ_BGDAC,CHARGE_PXINJ_SUSDAC,CHARGE_BUSINJ,
                                              ADC_INJ,EXT_LATCH,NORM};
      return getActiveNames(activeModes);
    }

    virtual bool hasFixedDecoupling() const {return true;}

    bool checkRegisterTypes() override;


  private:
    std::string currPixels;
    std::vector<bool> isCurrentRunPixel;
    bool m_injModeNeedsMonBus;
};

}

#endif // F2CHIPINTERFACE_H
