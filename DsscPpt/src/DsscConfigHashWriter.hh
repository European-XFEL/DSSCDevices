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

    class DsscConfigHashWriter {
    public:
        DsscConfigHashWriter();
        DsscConfigHashWriter(const DsscConfigHashWriter& orig);
        virtual ~DsscConfigHashWriter();

        void getFullConfigHash(std::string&, Hash&);

        void addMapData(Hash&, const std::string&, const std::map<std::string, uint32_t>&);

        const static std::string m_baseNode;


    private:
        void addConfiguration(Hash& _resHash, const DsscHDF5ConfigData& _h5config);
        void addConfiguration(Hash&, const DsscHDF5RegisterConfigVec&);
        void addConfiguration(Hash&, const DsscHDF5RegisterConfig & registerConfig);
        void addConfiguration(Hash&, std::string, const DsscHDF5SequenceData&);

    };
} //namespace karabo

#endif /* DSSCCONFIGHASHWRITER_HH */

