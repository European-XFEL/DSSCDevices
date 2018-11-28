#include "PPTFullConfig.h"

using namespace std;
using namespace SuS;

#define ERROR_OUT(error) \
  cout << "ERROR PPTFullConfig: " << error << endl;

PPTFullConfig::PPTFullConfig(const string & fullConfigFileName)
  : PPTFullConfig()
{
  loadFullConfig(fullConfigFileName);
}


PPTFullConfig::PPTFullConfig()
 : CHIPFullConfig(4),
   epcReg(nullptr),
   iobReg(nullptr)
{
  clear();
}

PPTFullConfig::~PPTFullConfig()
{
  if(epcReg)
    delete epcReg;

  if(iobReg)
    delete iobReg;
}

// this function replaces the function in base class
void PPTFullConfig::readFullConfigFile(const string & fileName)
{
  clear();
  configFileName = fileName;

  ifstream in(fileName);
  if(!in.is_open()){
    cout << "Error could not open full config file " << fileName << endl;
    return;
  }

  string line;
  RegType type = UNKNOWN;
  //reads the file and ignores lines with leading '#' character
  while (in.good())
  {
    getline(in,line);
    if (line.empty())
      continue;
    if (line[0]=='#')
      continue;

    if (line[0]==SEQTAG[0]){
      type = getType(line);
    }

    if(line.empty())
      continue;

    if(type == UNKNOWN){
      std::map<string,string> fileMap = readFullConfigFileOld(fileName);
      sequencerFileName = fileMap["seq"];
      pixelFileNames.push_back(fileMap["pixel"]);
      jtagFileNames.push_back(fileMap["jtag"]);
      epcFileName = fileMap["epc"];
      iobFileName = fileMap["iob"];
      return;
    }

    getline(in,line);

    addFileName(type,line);
  }

  sortFileNames();
}


void PPTFullConfig::storeFullConfigFile()
{
  epcReg->saveToFile(epcFileName);
  iobReg->saveToFile(iobFileName);

  CHIPFullConfig::storeFullConfigFile();
}


void PPTFullConfig::storeFullConfigFile(string fileName, bool keepName)
{
  if(fileName.rfind(".conf") == std::string::npos){
    fileName += ".conf";
  }

  configFileName = fileName;
  const string filePath = utils::getFilePath(configFileName);
  const string baseName = utils::getBaseFileName(configFileName);

  epcFileName = filePath + "/" + baseName +"_epc.xml";
  iobFileName = filePath + "/" + baseName +"_iob.xml";

  CHIPFullConfig::storeFullConfigFile(fileName,keepName);
}


void PPTFullConfig::addFileName(RegType type, string line)
{
  // for saftey
  if (line.empty())
    return;
  if (line[0]=='#')
    return;

  //store path as absolute path
  if(line[0] != '/'){
    const string filePath = utils::getFilePath(configFileName);
    line = filePath + "/" + line;
  }

  switch(type){
    case SEQ:
      sequencerFileName = line;
      break;
    case PXS:
      pixelFileNames.push_back(line);
      break;
    case JTG:
      jtagFileNames.push_back(line);
      break;
    case EPC:
      epcFileName = line;
      break;
    case IOB:
      iobFileName = line;
      break;
    default:
      ;//noop;
  }
}


void PPTFullConfig::loadConfig()
{
  CHIPFullConfig::loadConfig();

  if(!epcReg){
    newEPCReg();
  }else{
    epcReg->initFromFile(epcFileName);
  }

  if(!iobReg){
    newIOBReg();
  }else{
    iobReg->initFromFile(iobFileName);
  }

  // epc and iob registers can change
  checkNewRegisterEntries();

  checkImportantRegisterValues();
}


void PPTFullConfig::clear()
{
  CHIPFullConfig::clear();
  epcFileName = iobFileName = "";
}


void PPTFullConfig::newEPCReg()
{
  epcReg = new ConfigReg(epcFileName);
}


void PPTFullConfig::newIOBReg()
{
  iobReg = new ConfigReg(iobFileName);
}


bool PPTFullConfig::checkConfig()
{
  CHIPFullConfig::checkConfig();

  cout << "DEBUG::PPTFullConfig checkConfig()" << endl;

  if(epcFileName.empty()){
    cout << "EPC Register file not set, full config not usable" << endl;
//    throw "EPC Register file not set";
    good = false;
  }

  if(!utils::checkFileExists(epcFileName)){
    cout << "EPC Register file does not exist " << epcFileName <<" full config not usable" << endl;
//    throw "EPC Register file does not exist";
    good = false;
  }

  if(iobFileName.empty()){
    cout << "IOB Register file not set, full config not usable" << endl;
//    throw "IOB Register file not set";
    good = false;
  }

  if(!utils::checkFileExists(iobFileName)){
    cout << "IOB Register file does not exist " << iobFileName <<" full config not usable" << endl;
//    throw "IOB Register file does not exist";
    good = false;
  }

  return good;
}


void PPTFullConfig::generateFullConfigFile()
{
  CHIPFullConfig::generateFullConfigFile();

  ofstream stream(configFileName,ios_base::app);
  if (!stream.is_open()){
    cout << "Error could not open full config file to save " << configFileName << endl;
    return;
  }

  stream << EPCTAG << ":\n";
  stream << utils::getFileName(epcFileName) <<"\n";

  stream << IOBTAG << ":\n";
  stream << utils::getFileName(iobFileName) <<"\n";
}


void PPTFullConfig::checkNewRegisterEntries()
{
  //EPC Registers
  bool newReg;

  newReg  = checkNewEntry(epcReg,"UART_Control_Register","SIB_timeout_value","22-31",152,152);

  newReg |= checkNewEntry(epcReg,"SIB_Receive_Register","rd_data","0-15",176,0,true);
  newReg |= checkNewEntry(epcReg,"SIB_Receive_Register","byte_number","24-31",176,0,true);
  newReg |= checkNewEntry(epcReg,"DataRecv_to_Eth0_Register","expected_test_pattern","20-28",104,5);
  newReg |= checkNewEntry(epcReg,"DataRecv_to_Eth0_Register","reset_fail_data","19",104,0);
  newReg |= checkNewEntry(epcReg,"Data_Receive_Status_0","Failed_Asics_Module_1","0-15",160,0,true);
  newReg |= checkNewEntry(epcReg,"Data_Receive_Status_0","Failed_Asics_Module_2","16-31",160,0,true);
  newReg |= checkNewEntry(epcReg,"Data_Receive_Status_1","Failed_Asics_Module_3","0-15",164,0,true);
  newReg |= checkNewEntry(epcReg,"Data_Receive_Status_1","Failed_Asics_Module_4","16-31",164,0,true);

  newReg |= checkNewEntry(epcReg,"Data_Receive_Status_0","Failed_Asics_Module_1","0-15",160,0,true);
  newReg |= checkNewEntry(epcReg,"Data_Receive_Status_0","Failed_Asics_Module_2","16-31",160,0,true);
  newReg |= checkNewEntry(epcReg,"Data_Receive_Status_1","Failed_Asics_Module_3","0-15",164,0,true);
  newReg |= checkNewEntry(epcReg,"Data_Receive_Status_1","Failed_Asics_Module_4","16-31",164,0,true);

  //23.01.2018 in GUI v2.6
  newReg |= checkNewEntry(epcReg,"Single_Cycle_Register","rotate_ladder","8",180,0);
  newReg |= checkNewEntry(epcReg,"Single_Cycle_Register","sort_asic_wise","9",180,0);

  newReg |= checkNewEntry(epcReg,"Data_Monitor_Select","channel_select","0;1",188,0);
  newReg |= checkNewEntry(epcReg,"Data_Monitor_Select","frame_select","2-11",188,0);
  newReg |= checkNewEntry(epcReg,"Data_Monitor_Select","asic_select","12-15",188,0);
  newReg |= checkNewEntry(epcReg,"Data_Monitor_Select","pixel_select","16-27",188,0);
  newReg |= checkNewEntry(epcReg,"Data_Monitor_Select","en_image_mode","28",188,1);
  newReg |= checkNewEntry(epcReg,"Data_Monitor_Select","nc","29-31",188,1);

  newReg |= checkNewEntry(epcReg,"Data_Monitor_Status","monitor_train_id","0-15",192,0,true);
  newReg |= checkNewEntry(epcReg,"Data_Monitor_Status","image_available","16",192,0,true);
  newReg |= checkNewEntry(epcReg,"Data_Monitor_Status","nc","17-31",192,0,true);

  //07.09.2018 in GUI v3.0
  newReg |= checkNewEntry(epcReg,"Current_Train_Id","Current_Train_Id","0-31",200,0,true);

  newReg |= checkNewEntry(epcReg,"IOB_Serial_Numbers","IOB1_Serial","0-31",204,0);
  newReg |= checkNewEntry(epcReg,"IOB_Serial_Numbers","IOB2_Serial","32-63",204,0);
  newReg |= checkNewEntry(epcReg,"IOB_Serial_Numbers","IOB3_Serial","64-95",204,0);
  newReg |= checkNewEntry(epcReg,"IOB_Serial_Numbers","IOB4_Serial","96-127",204,0);

  // renamed signals
  newReg |= removeEntry(epcReg,"Det_Specific_Data_Register","nc");
  newReg |= checkNewEntry(epcReg,"Det_Specific_Data_Register","UserSpecificData1","0-15",184,0);
  newReg |= checkNewEntry(epcReg,"Det_Specific_Data_Register","UserSpecificData2","16-31",184,0);
  newReg |= checkNewEntry(epcReg,"Det_Specific_Data_Register","UserSpecificData3","32-47",184,0);
  newReg |= checkNewEntry(epcReg,"Det_Specific_Data_Register","nc","48-63",184,0,true);
  newReg |= removeEntry(epcReg,"Det_Specific_Data_Register","IOB1_Serial");
  newReg |= removeEntry(epcReg,"Det_Specific_Data_Register","IOB2_Serial");
  newReg |= removeEntry(epcReg,"Det_Specific_Data_Register","IOB3_Serial");
  newReg |= removeEntry(epcReg,"Det_Specific_Data_Register","IOB4_Serial");

  newReg |= checkNewEntry(epcReg,"Multi_purpose_Register","sib_control_reset","14",8,0,false);

  newReg |= checkNewEntry(epcReg,"Eth_Output_Data_Rate","Eth_Output_Data_Rate","0-31",208,0,true);

  if(newReg){
    epcReg->save();
  }

  //IOB Registers
  newReg  = checkNewEntry(iobReg,"LMK_control","LMK_clock_divider","8-23",256,511);
  newReg |= checkNewEntry(iobReg,"LMK_control","nc","24-31",256,0,true);
  if(newReg){
    iobReg->save();
  }
}


bool PPTFullConfig::checkNewEntry(ConfigReg * reg, const std::string & moduleSet, const std::string & signalName,const std::string & posList, int address, uint32_t defaultValue, bool readOnly)
{
  bool newSignal = false;
  if(!reg->moduleSetExists(moduleSet)){
    const auto modulesList = reg->getModuleNumberList(reg->getModuleSetNames().front());
    reg->addModuleSet(moduleSet,modulesList,address);
  }

  if(!reg->signalNameExists(moduleSet,signalName)){
    reg->addSignal(moduleSet,signalName,posList,readOnly);
    cout << "New Signal "<< signalName << " added to ModuleSet " << moduleSet << endl;
    newSignal = true;
    reg->setSignalValue(moduleSet,"all",signalName,defaultValue);
  }

  return newSignal;
}


bool PPTFullConfig::removeEntry(ConfigReg * reg, const std::string & moduleSet, const std::string & signalName)
{
  bool removedSignal = false;
  if(!reg->moduleSetExists(moduleSet)){
    return removedSignal;
  }

  if(reg->signalNameExists(moduleSet,signalName)){
    const auto signalNames = reg->getSignalNames(moduleSet);
    int signalIdx = std::distance(signalNames.begin(),std::find(signalNames.begin(),signalNames.end(),signalName));
    reg->removeSignal(moduleSet,signalIdx);
    cout << "Signal "<< signalName << " removed from ModuleSet " << moduleSet << endl;
    removedSignal = true;
  }

  return removedSignal;
}


void PPTFullConfig::checkImportantRegisterValues()
{
  if(epcReg->getSignalValue("JTAG_Control_Register","0","ASIC_JTAG_Clock_Divider")<25)
  {
    epcReg->setSignalValue("JTAG_Control_Register","0","ASIC_JTAG_Clock_Divider",25);

    utils::CoutColorKeeper keeper(utils::STDBROWN);
    cout << "WARNING: Bad Setting detected: Set ASIC_JTAG_Clock_Divider to 25" << endl;
  }

  iobReg->setSignalValue("Aurora_Preemphasys","all","Aurora_Preemphasys",2);
  iobReg->setSignalValue("Aurora_Swing","all","Aurora_Swing",10);
}


