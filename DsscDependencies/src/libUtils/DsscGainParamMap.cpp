/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   DsscGainParamMap.cpp
 * Author: dssc
 *
 * Created on May 30, 2018, 11:54 AM
 */
#include <iostream>

#include "DsscGainParamMap.h"

using namespace SuS;

#ifdef F1IO
const std::vector<std::string> SuS::DsscGainParamMap::s_gainModeParamNames = {"D0_EnMimCap","FCF_EnIntRes","FCF_EnCap","IntegrationTime"};
#else
const std::vector<std::string> SuS::DsscGainParamMap::s_gainModeParamNames = {"CSA_FbCap","CSA_Resistor","FCF_EnCap","IntegrationTime"};
#endif

DsscGainParamMap::DsscGainParamMap()
{
  for(auto && paramName : s_gainModeParamNames){
    this->operator[](paramName) = 0;
  }
}

DsscGainParamMap::DsscGainParamMap(const std::vector<uint32_t> & values)
{
  if(values.size() != s_gainModeParamNames.size())
    return;

  int idx = 0;
  for(auto && paramName : s_gainModeParamNames){
    this->operator[](paramName) = values[idx++];
  }
}


DsscGainParamMap::DsscGainParamMap(uint32_t val0, uint32_t val1, uint32_t val2, uint32_t val3)
  : DsscGainParamMap(std::vector<uint32_t>({val0,val1,val2,val3}))
{
}


bool DsscGainParamMap::operator==(const DsscGainParamMap & map2){
  for(auto && paramName : s_gainModeParamNames){
    if(this->operator[](paramName) != map2.at(paramName)){
      return false;
    }
  }
  return true;
}

bool DsscGainParamMap::operator!=(const DsscGainParamMap & map2){
  for(auto && paramName : s_gainModeParamNames){
    if(this->operator[](paramName) != map2.at(paramName)){
      return true;
    }
  }
  return false;
}

void DsscGainParamMap::print() const
{
  for(auto && item : *this){
    std::cout << item.first << " : " << item.second << std::endl;
  }
  std::cout << std::endl;
}


