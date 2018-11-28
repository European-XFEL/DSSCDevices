/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   DsscDependencies.h
 * Author: kirchgessner
 *
 * Created on 4. Juni 2018, 14:36
 */

#ifndef KARABO_DSSCDEPENDENCIES_H
#define KARABO_DSSCDEPENDENCIES_H

#include <karabo/karabo.hpp>

/**
 * The main Karabo namespace
 */

namespace karabo {
namespace DSSC {

    class StateChangeKeeper{
          public:
            StateChangeKeeper(core::Device<>  *device) : dev(device),lastState(dev->getState()){
              dev->updateState(util::State::CHANGING);
              dev->set<std::string>("status","Measuring");
            }

            StateChangeKeeper(core::Device<>  *device, const util::State & afterState) : dev(device),lastState(afterState){
              dev->updateState(util::State::CHANGING);
            }

            ~StateChangeKeeper(){
              dev->updateState(lastState);
            }

            void change(const util::State & newState){lastState = newState;}

          private:
            core::Device<> *dev;
            util::State lastState;
        };
}
}
#endif /* KARABO_DSSCDEPENDENCIES_H */

