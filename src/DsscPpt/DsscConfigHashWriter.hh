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
#include "DsscRegisterConfiguration.hh"

namespace karabo {

    class DsscConfigToSchema {

    public:

        DsscConfigToSchema();

        virtual ~DsscConfigToSchema();

        //karabo::data::Schema getUpdatedSchema();
        void addMapData(karabo::data::Hash& hash, const std::string& node, const std::map<std::string, uint32_t>& mapData);

        static void HashToSchema(const karabo::data::Hash& hash, karabo::data::Schema& expected, const std::string& path);
        static void HashToSchemaDetConf(const karabo::data::Hash& hash, karabo::data::Schema& expected,\
            const std::string& path, bool _readonly);        

        static void addConfiguration(karabo::data::Hash& hash, DsscConfigData& configData);
        static void addConfiguration(karabo::data::Hash& hash, DsscRegisterConfigVec& registerConfigVec);
        static void addConfiguration(karabo::data::Hash& hash, DsscRegisterConfig & registerConfig);
        static void addConfiguration(karabo::data::Hash& hash, const std::string& path, const DsscSequenceData& sequenceData);
        
        std::vector<std::pair<std::string, unsigned int>> compareConfigHashData(karabo::data::Hash& hash_old, karabo::data::Hash& hash_new);
        void compareConfigHashData_rec(karabo::data::Hash& hash_old, karabo::data::Hash& hash_new, std::string path);
        
    private:

        static std::string removeSpaces(std::string& p) {
            std::string res(p);
            std::replace(res.begin(), res.end(), ' ', '_');
            return res;
        }
        
        std::vector<std::pair<std::string, unsigned int>> paths_diffVals;

    };

}//namespace karabo

#endif /* DSSCCONFIGHASHWRITER_HH */

