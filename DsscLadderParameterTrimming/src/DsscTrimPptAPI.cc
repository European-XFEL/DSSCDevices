/* 
 * File:   DsscTrimPptAPI.cpp
 * Author: samartse
 * 
 * Created on June 11, 2019, 2:35 PM
 */

#include "DsscLadderParameterTrimming.hh"

#include "DsscTrimPptAPI.hh"


namespace SuS{

DsscTrimPptAPI::DsscTrimPptAPI(karabo::DsscLadderParameterTrimming* _karaboDevice, const std::string& configFile) :\
  m_karaboDevice(_karaboDevice), SuS::MultiModuleInterface(new SuS::PPTFullConfig(configFile)) {
}

DsscTrimPptAPI::DsscTrimPptAPI(karabo::DsscLadderParameterTrimming* _karaboDevice, SuS::PPTFullConfig* fullconfig) : \
  m_karaboDevice(_karaboDevice), SuS::MultiModuleInterface(fullconfig) {
}



DsscTrimPptAPI::~DsscTrimPptAPI() {
}


void DsscTrimPptAPI::displayInjectionSweep(const std::vector<std::vector<double>> &binValues, const std::vector<unsigned int> & xValues, const std::vector<uint32_t> & measurePixels){
    m_karaboDevice->displayInjectionSweep(binValues, xValues, measurePixels);
}

void DsscTrimPptAPI::displayDataHistos(const utils::DataHistoVec & dataHistoVec, const std::vector<uint32_t> & measurePixels){
    m_karaboDevice->displayDataHistos(dataHistoVec, measurePixels);
}
    
    
const uint16_t* DsscTrimPptAPI::getPixelSramData(int pixel){
    return m_karaboDevice->getPixelSramData(pixel);
}
   
int DsscTrimPptAPI::initSystem(){
    return m_karaboDevice->initSystem();
}
    
bool DsscTrimPptAPI::updateAllCounters(){
    return m_karaboDevice->updateAllCounters();
}
    
void DsscTrimPptAPI::updateStartWaitOffset(int value){
    m_karaboDevice->updateStartWaitOffset(value);
}
    
bool DsscTrimPptAPI::doSingleCycle(DataPacker* packer, bool testPattern){
    m_karaboDevice->doSingleCycle(packer, testPattern);
}
    
bool DsscTrimPptAPI::doSingleCycle(int numTries, DataPacker* packer, bool testPattern){
    m_karaboDevice->doSingleCycle(numTries, packer, testPattern);
}
   
std::vector<double> DsscTrimPptAPI::measureBurstData(const std::vector<uint32_t> & measurePixels, int STARTADDR, int ENDADDR, bool subtract){
    return m_karaboDevice->measureBurstData(measurePixels, STARTADDR, ENDADDR, subtract);
}

std::vector<double> DsscTrimPptAPI::measureRMSData(const std::vector<uint32_t> & measurePixels, const int STARTADDR, const int ENDADDR){
    return m_karaboDevice->measureRMSData(measurePixels, STARTADDR, ENDADDR);
}
    
    // configuration function with DsscPpt Device
bool DsscTrimPptAPI::isHardwareReady(){
    return m_karaboDevice->isHardwareReady();
}

bool DsscTrimPptAPI::fillSramAndReadout(uint16_t pattern, bool init, bool jtagMode){
    return m_karaboDevice->fillSramAndReadout(pattern, init, jtagMode);
}

bool DsscTrimPptAPI::fastInitChip(){
    return m_karaboDevice->fastInitChip();
}
    
void DsscTrimPptAPI::initChip(){
    m_karaboDevice->initChip();
}
    
    
bool DsscTrimPptAPI::getContentFromDevice(uint32_t bitStreamLength, std::vector<bool> &data_vec){
    return m_karaboDevice->getContentFromDevice(bitStreamLength, data_vec);
}

bool DsscTrimPptAPI::programJtag(bool readBack, bool setJtagEngineBusy, bool recalcXors){
    return m_karaboDevice->programJtag(readBack, setJtagEngineBusy, recalcXors);
}
    
bool DsscTrimPptAPI::programJtagSingle(const std::string & moduleSetName, bool readBack, bool setJtagEngineBusy, bool recalcXors, bool overwrite){
    return m_karaboDevice->programJtagSingle(moduleSetName, readBack, setJtagEngineBusy, recalcXors, overwrite);
}
    
bool DsscTrimPptAPI::programPixelRegs(bool readBack, bool setJtagEngineBusy){
    return m_karaboDevice->programPixelRegs(readBack, setJtagEngineBusy);
}
    
void DsscTrimPptAPI::programPixelRegDirectly(int px, bool setJtagEngineBusy){
    m_karaboDevice->programPixelRegDirectly(px, setJtagEngineBusy);
}
    
bool DsscTrimPptAPI::programSequencer(bool readBack, bool setJtagEngineBusy, bool program){
    m_karaboDevice->programSequencer(readBack, setJtagEngineBusy, program);
}

void DsscTrimPptAPI::setBurstVetoOffset(int val){
    m_karaboDevice->setBurstVetoOffset(val);
}
    
int DsscTrimPptAPI::getBurstVetoOffset(){
    return m_karaboDevice->getBurstVetoOffset();
}

void DsscTrimPptAPI::setNumFramesToSend(int val, bool saveOldVal){
    m_karaboDevice->setNumFramesToSend(val, saveOldVal);
}
    
void DsscTrimPptAPI::setNumWordsToReceive(int val, bool saveOldVal){
    m_karaboDevice->setNumWordsToReceive(val, saveOldVal);
}

void DsscTrimPptAPI::runContinuousMode(bool run){
    m_karaboDevice->runContinuousMode(run);
}

void DsscTrimPptAPI::setRoSerIn(bool bit){
    m_karaboDevice->setRoSerIn(bit);
}

void DsscTrimPptAPI::setSendRawData(bool enable, bool reordered, bool converted){
    m_karaboDevice->setSendRawData(enable, reordered, converted);
}

void DsscTrimPptAPI::waitJTAGEngineDone(){
    m_karaboDevice->waitJTAGEngineDone();
}

int DsscTrimPptAPI::getNumberOfActiveAsics() const{
    return m_karaboDevice->getNumberOfActiveAsics();
}
    
int DsscTrimPptAPI::getNumberOfSendingAsics() const{
    return m_karaboDevice->getNumberOfSendingAsics();
}
    
int DsscTrimPptAPI::getActiveASICToReadout() const{
    return m_karaboDevice->getActiveASICToReadout();
}
    
void DsscTrimPptAPI::setSendingAsics(uint16_t asics){
    m_karaboDevice->setSendingAsics(asics);
}
    
void DsscTrimPptAPI::setActiveModule(int modNumber){
    m_karaboDevice->setActiveModule(modNumber);
}
    
void DsscTrimPptAPI::sramTest(int iterations, bool init){
    m_karaboDevice->sramTest(iterations, init);
}

void DsscTrimPptAPI::resetDataReceiver(){
    m_karaboDevice->resetDataReceiver();
}

bool DsscTrimPptAPI::inContinuousMode(){
    return m_karaboDevice->inContinuousMode();
}

}//SuS

