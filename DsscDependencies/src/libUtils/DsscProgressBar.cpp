#include <iostream>
#include <sstream>
#include <iomanip>


#include "DsscProgressBar.h"

using namespace std;
using namespace utils;

DsscProgressBar::DsscProgressBar()
 : m_start(chrono::system_clock::now()),
   m_title(" please wait..."),
   m_numSettings(1),
   m_currentSetting(0)
{

}

DsscProgressBar::DsscProgressBar(const string & title, uint64_t numValues)
  : m_start(chrono::system_clock::now()),
    m_title(title + " please wait..."),
    m_numSettings(numValues),
    m_currentSetting(0)
{
}


std::string DsscProgressBar::toTimeStr(const std::chrono::system_clock::time_point & time)
{
  //auto time_c = std::chrono::system_clock::to_time_t(time);
  stringstream ss;
  // can not be compiled on DESY computers
  ss << ""; //std::put_time(std::localtime(&time_c), "%F %T");
  return ss.str();
}


void DsscProgressBar::setValue(uint64_t progress)
{
  if(progress == 0){
    m_start = chrono::system_clock::now();
  }
  m_currentSetting = progress;

  if(m_numSettings>1){
    int fraction = 50.0*m_currentSetting/m_numSettings;

    string duration = calcDuration();
    string elapsed  = calcElapsedTime();
    string endtime  = calcApproxEndTime();

    cout << m_title;
    cout << "\n---------------------------------------------------\n|";
    for(int i=0; i<fraction; i++){
      cout << "=";
    }
    cout <<  ">";
    for(int i=fraction; i<50; i++){
      cout <<  " ";
    }
    cout <<  "|" << 100*m_currentSetting/m_numSettings << "%\n";
    cout <<  "---------------------------------------------------";
    cout <<  "\nStart Time:         " << setw(31) << toTimeStr(m_start);
    cout <<  "\nElapsed Time:       " << setw(31) << elapsed;
    cout <<  "\nTime to finish:     " << setw(31) << duration;
    cout <<  "\nEstimated End Time: " << setw(31) << endtime;
    cout << "\n\n";
  }
}


string DsscProgressBar::calcApproxEndTime()
{
  auto now = chrono::system_clock::now();
  uint64_t milliseconds = chrono::duration_cast<chrono::milliseconds>(now-m_start).count();
  uint64_t milliSecsToEnd = m_numSettings*milliseconds/std::max(m_currentSetting,1ul);
  auto finishTime = m_start+chrono::milliseconds(milliSecsToEnd);
  return toTimeStr(finishTime);
}


string DsscProgressBar::calcDuration()
{
  auto now = chrono::system_clock::now();
  uint64_t milliseconds = chrono::duration_cast<chrono::milliseconds>(now-m_start).count();
  uint64_t toGo = m_numSettings - m_currentSetting;
  uint64_t milliSecsToGo = toGo * milliseconds/std::max(m_currentSetting,1ul);
  return getTimeFromMilliSeconds(milliSecsToGo);
}


string DsscProgressBar::calcElapsedTime()
{
  auto now = chrono::high_resolution_clock::now();
  uint64_t milliseconds = chrono::duration_cast<chrono::milliseconds>(now-m_start).count();
  return getTimeFromMilliSeconds(milliseconds);
}


std::string DsscProgressBar::getTimeFromMilliSeconds(uint64_t milliseconds)
{
  uint64_t seconds = milliseconds/1000;
  uint64_t hours = seconds/3600;
  uint64_t mins  = (seconds%3600)/60;
  uint64_t secs  = (seconds%3600) - (mins*60);

  if(hours == 0)
  {
    if(mins == 0){
      return to_string(secs) + " secs";
    }
    return to_string(mins) + ":" + to_string(secs) + " mins";
  }
  return to_string(hours) + ":" + to_string(mins) + ":" + to_string(secs) + " hours";
}
