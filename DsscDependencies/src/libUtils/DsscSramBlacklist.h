#ifndef DSSCSRAMBLACKLIST_H
#define DSSCSRAMBLACKLIST_H

#include "utils.h"

namespace utils{
class DsscSramBlacklist
{
  public:
    DsscSramBlacklist();
    DsscSramBlacklist(const std::vector<double> &meanSramContent);
    DsscSramBlacklist(const std::string & fileName);

    void saveToFile(const std::string & fileName) const;
    bool isValid() const {return m_sramBlacklistValid;}
    void initFromFile(const std::string & fileName, bool add = false);

    bool isBadSramCellsFile(const std::string & fileName) const;


    void initForPixelInjection(uint32_t offset, uint32_t addrSkipCnt, bool signalNotBaseline);

    inline const std::vector<bool> & getValidSramAddresses(int pixel) const  {return m_validPxSramAddresses[pixel];}
    inline const std::vector<uint32_t> & getSramBlacklist(int pixel) const  {return m_sramAddrBlacklist[pixel];}

    void clear();

  protected:
    void importBadSramCellsMap(const std::string & fileName);
    void importFromBlacklistFile(const std::string & fileName, bool add);

  private:

    void fillValidSramAddressesFromSramBlacklist();
    void generateSramBlacklist(const std::vector<double> &meanSramContent);
    void initVectors();

    bool m_sramBlacklistValid;
    std::vector<std::vector<uint32_t>> m_sramAddrBlacklist;
    std::vector<std::vector<bool>> m_validPxSramAddresses;
};

}

#endif // DSSCSRAMBLACKLIST_H
