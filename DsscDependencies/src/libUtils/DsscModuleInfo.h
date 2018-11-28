#ifndef DSSCMODULEINFO_H
#define DSSCMODULEINFO_H

#pragma once

#include <map>
#include <string>
#include <vector>
#include <array>
#include <ostream>

namespace utils{

class ModuleInfo
{
  public:

    ModuleInfo();
    ModuleInfo(int modNr);
    ModuleInfo(std::istream & in);

    void print(std::ostream & out) const;

    inline std::string name() const {return moduleName;}
    inline bool isAsicSending(int asic) const {return ((notSendingAsics & (1<<asic)) == 0);}
    inline std::string getIOBSerialStr() const;
    inline std::string getNotSendingAsicsStr() const;
    inline std::string getAsicIdList() const;

    std::string moduleName;
    int moduleNr;
    uint32_t iobSerial;
    uint16_t notSendingAsics;
    std::array<std::string,16> asicIds;
};

using ModuleInfoMap = std::map<int,ModuleInfo>;

class QuadrantInfo{
  public:

    QuadrantInfo();
    QuadrantInfo(std::istream &in);
    void print(std::ostream & out) const;
    uint32_t getIpAddressInt() const;
    uint numModules() const {return moduleInfoMap.size();}
    std::string getShortIdStr() const;
    uint16_t getShortId() const;
    std::vector<std::string> getModuleNames() const;
    std::string getModuleName(int moduleNr) const;
    ModuleInfo & getModule(int moduleNr);
    const ModuleInfo & getModule(int moduleNr) const;

    std::string quadrantId;
    int quadrantNr;
    std::string pptIpAddress;
    std::string quadrantInfo;
    ModuleInfoMap moduleInfoMap;
};

using QuadrantInfoMap = std::map<std::string,QuadrantInfo>;


class DsscModuleInfo
{
  public:

    DsscModuleInfo();
    ~DsscModuleInfo();

    static const std::string s_moduleInfoFileName;

    // global pointer to module info
    static DsscModuleInfo * g_DsscModuleInfo;

    static const std::vector<std::string> & getQuadrantIds(){return g_DsscModuleInfo->quadrantIds;}
    static std::string getQuadrantIdList();
    static std::string getQuadrantId(int quadrantNr){return g_DsscModuleInfo->quadrantIds.at(quadrantNr);}
    static int getQuadrantNumber(const std::string & quadrantId);

    static std::vector<std::string> getQuadrantPptIpAddresses();

    static void setIOBSerial(const std::string & quadrantId, int moduleNr, uint32_t iobSerial);
    static std::string getIOBSerialStr(const std::string & quadrantId, int moduleNr);

    static uint32_t getQuadrantPptIpAddress(const std::string & quadrantId);
    static std::string getQuadrantPptIpAddressStr(const std::string & quadrantId);

    static std::vector<std::string> getModuleNames(const std::string & quadrantId);
    static std::string getModuleName(const std::string & quadrantId, int moduleNr);
    static int getModuleNr(const std::string & quadrantId, const std::string & moduleName);

    static std::string getModuleInfoStr(const std::string & quadrantId, int moduleNr, uint32_t iobSerial = 0);
    static uint16_t toShortId(const std::string & quadrantId);

  private:
    void readModuleInfoFile();
    void saveModuleInfoFile();

    QuadrantInfoMap quadrantInfoMap;

    std::vector<std::string> quadrantIds;
};


}
#endif // DSSCMODULEINFO_H
