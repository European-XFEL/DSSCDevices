/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   DsscConfigHashWriter.hh
 * Author: samartse
 *
 * Created on February 7, 2019, 9:24 AM *
 *
 */



#ifndef DSSCCONFIGHASHWRITER_HH
#define DSSCCONFIGHASHWRITER_HH

#include <karabo/karabo.hpp>

#include "DsscPpt.hh"

using namespace karabo::util;
using namespace karabo::log;
using namespace karabo::io;
using namespace karabo::net;
using namespace karabo::xms;
using namespace karabo::core;

namespace karabo {


    void getFullConfigHash(const std::string&, Hash&);

    void addMapData(Hash&, const std::string&, const std::map<std::string, uint32_t>&);


} //namespace karabo

#endif /* DSSCCONFIGHASHWRITER_HH */

