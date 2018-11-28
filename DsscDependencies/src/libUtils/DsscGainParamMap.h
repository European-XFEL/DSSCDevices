/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   DsscGainParamMap.h
 * Author: dssc
 *
 * Created on May 30, 2018, 11:54 AM
 */

#ifndef DSSCGAINPARAMMAP_H
#define DSSCGAINPARAMMAP_H

#include <map>
#include <string>
#include <vector>

namespace SuS
{

class DsscGainParamMap : public std::map<std::string,uint32_t>
{
    public:
        static const std::vector<std::string> s_gainModeParamNames;

        DsscGainParamMap();

        DsscGainParamMap(uint32_t val0, uint32_t val1, uint32_t val2, uint32_t val3);
        DsscGainParamMap(const std::vector<uint32_t> & values);

        bool operator==(const DsscGainParamMap & map2);
        bool operator!=(const DsscGainParamMap & map2);

        void print() const;
};

}
#endif /* DSSCGAINPARAMMAP_H */

