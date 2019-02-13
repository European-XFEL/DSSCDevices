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

#include "DsscPptAPI.hh"

namespace karabo {

    const std::string s_dsscConfBaseNode = "RunMetaData";

    class DsscH5ConfigToSchema {
    public:

        DsscH5ConfigToSchema() {
            std::cout << "constructor2" << std::endl;
        }

        virtual ~DsscH5ConfigToSchema() {
        }

        bool getFullConfigHash(const std::string& filename, karabo::util::Hash& hash);
        karabo::util::Schema getUpdatedSchema();
        void addMapData(karabo::util::Hash& hash, const std::string& node, const std::map<std::string, uint32_t>& mapData);

    private:

        void HashToSchema(const karabo::util::Hash& hash, karabo::util::Schema& expected, const std::string& path);

        void addConfiguration(karabo::util::Hash& hash, const DsscHDF5ConfigData& configData);
        void addConfiguration(karabo::util::Hash& hash, const DsscHDF5RegisterConfigVec& registerConfigVec);
        void addConfiguration(karabo::util::Hash& hash, const DsscHDF5RegisterConfig & registerConfig);
        void addConfiguration(karabo::util::Hash& hash, const std::string& path, const DsscHDF5SequenceData& sequenceData);

        karabo::util::Hash m_lastHash;

    };

}//namespace karabo

#endif /* DSSCCONFIGHASHWRITER_HH */

