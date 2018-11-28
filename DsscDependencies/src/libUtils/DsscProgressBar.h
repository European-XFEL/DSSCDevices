#ifndef DSSCPROGRESSBAR_H
#define DSSCPROGRESSBAR_H

#include <string>
#include <chrono>

namespace utils{


class DsscProgressBar
{
  public:
    DsscProgressBar();
    DsscProgressBar(const std::string & title, uint64_t numValues);

    void setTitle(const std::string & title){m_title = title;}
    void setRange(uint64_t numValues){m_numSettings = std::max(numValues,1ul);}
    void setValue(uint64_t progress);
    void addValue(){m_currentSetting++;}

    inline uint64_t value() const {return m_currentSetting;}

  private:
    std::string calcDuration();
    std::string calcElapsedTime();
    std::string calcApproxEndTime();

    std::string getTimeFromMilliSeconds(uint64_t milliseconds);

    std::string toTimeStr(const std::chrono::system_clock::time_point & time);

    std::chrono::system_clock::time_point m_start;
    std::string m_title;
    uint64_t m_numSettings;
    uint64_t m_currentSetting;
};

}
#endif // DSSCPROGRESSBAR_H
