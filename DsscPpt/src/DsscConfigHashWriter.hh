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

    const std::string s_dsscConfBaseNode = "DetConfRegisters";

    class DsscH5ConfigToSchema {

    public:

        DsscH5ConfigToSchema();

        virtual ~DsscH5ConfigToSchema();

        //karabo::util::Schema getUpdatedSchema();
        void addMapData(karabo::util::Hash& hash, const std::string& node, const std::map<std::string, uint32_t>& mapData);

        static void HashToSchema(const karabo::util::Hash& hash, karabo::util::Schema& expected, const std::string& path);
        static void HashToSchemaDetConf(const karabo::util::Hash& hash, karabo::util::Schema& expected,\
            const std::string& path, bool _readonly);        

        static void addConfiguration(karabo::util::Hash& hash, DsscHDF5ConfigData& configData);
        static void addConfiguration(karabo::util::Hash& hash, DsscHDF5RegisterConfigVec& registerConfigVec);
        static void addConfiguration(karabo::util::Hash& hash, DsscHDF5RegisterConfig & registerConfig);
        static void addConfiguration(karabo::util::Hash& hash, const std::string& path, const DsscHDF5SequenceData& sequenceData);
        
         static bool compareConfigHashData(karabo::util::Hash& hash_old, karabo::util::Hash& hash_new);
        
    private:

        static std::string & removeSpaces(std::string& p) {
            std::replace(p.begin(), p.end(), ' ', '_');
            return p;
        }
        
        static std::string & restoreSpaces(std::string& p) {
            std::replace(p.begin(), p.end(), '_', ' ');
            return p;
        }

    };

}//namespace karabo

#endif /* DSSCCONFIGHASHWRITER_HH */

