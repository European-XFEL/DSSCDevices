/* 
 * File:   FemFsm.hh
 * Author: esenov
 *
 * Created on March 4, 2014, 4:10 PM
 */

#ifndef KARABO_CORE_DSSCRECVFSM_HH
#define	KARABO_CORE_DSSCRECVFSM_HH

#include <karabo/xms/SlotElement.hh>
#include <karabo/core/Device.hh>
#include <karabo/core/BaseFsm.hh>
namespace karabo {
    namespace dsscrecv {

        class DsscRecvFsm : public core::BaseFsm {
        public:

            KARABO_CLASSINFO(DsscRecvFsm, "DsscRecvFsm", "1.0")

            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::xms;

                SLOT_ELEMENT(expected)
                        .key("open").displayedName("Connect UDP").description("Open connection to UDP Socket")
                        .allowedStates("AllOk.Disconnected")
                        .commit();

                SLOT_ELEMENT(expected)
                        .key("close").displayedName("Disconnect UDP").description("Close connection to UDP Socket")
                        .allowedStates("AllOk.Idle")
                        .commit();       
                
                SLOT_ELEMENT(expected)
                        .key("activate").displayedName("Activate").description("Activate Data Taking for 5 seconds")
                        .allowedStates("AllOk.Idle")
                        .commit();  
                
                SLOT_ELEMENT(expected)
                        .key("reset").displayedName("Reset").description("Resets the device in case of an error")
                        .allowedStates("Error")
                        .commit();
               
            }

            void initFsmSlots() {
                SLOT0(open);
                SLOT0(close);
                SLOT0(reset);
                SLOT0(activate);
            }

        public:

            /**************************************************************/
            /*                        Events                              */

            /**************************************************************/

            KARABO_FSM_EVENT2(m_fsm, ErrorFoundEvent, errorFound, std::string, std::string)
            KARABO_FSM_EVENT0(m_fsm, ResetEvent, reset)
            KARABO_FSM_EVENT0(m_fsm, OpenEvent, open)
            KARABO_FSM_EVENT0(m_fsm, CloseEvent, close)
            KARABO_FSM_EVENT0(m_fsm, ActivateEvent, activate)

            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            KARABO_FSM_STATE_VE_EE(Error, errorStateOnEntry, errorStateOnExit)
            KARABO_FSM_STATE_VE_E(Disconnected, disconnectedStateOnEntry)
            KARABO_FSM_STATE_VE_E(Idle, idleStateOnEntry)

            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/
            //ErrorFoundAction required in karabo 1.2.1 copied from BaseFsm.hh
            KARABO_FSM_PV_ACTION0(ErrorFoundAction, errorFoundAction)
            
            KARABO_FSM_VE_ACTION0(ResetAction, resetAction)
            KARABO_FSM_VE_ACTION0(OpenAction, openAction)
            KARABO_FSM_VE_ACTION0(CloseAction, closeAction)
            KARABO_FSM_VE_ACTION0(ActivateAction, activateAction)
            

            /**************************************************************/
            /*                      OkState Machine                       */
            /**************************************************************/

            KARABO_FSM_TABLE_BEGIN(AllOkStt)
            //   Source-State,   Event,        Target-State, Action,          Guard
            Row< Disconnected, OpenEvent, Idle, OpenAction, none >,
            Row< Idle, CloseEvent, Disconnected, CloseAction, none >,   
            Row< Idle, ActivateEvent, Idle, ActivateAction, none >            

            
            KARABO_FSM_TABLE_END

            KARABO_FSM_STATE_MACHINE(AllOk, AllOkStt, Disconnected, Self)

            /**************************************************************/
            /*                      Top Machine                         */
            /**************************************************************/

            // Source-State, Event, Target-State, Action, Guard
            KARABO_FSM_TABLE_BEGIN(TopFemStt)
            Row< AllOk, ErrorFoundEvent, Error, ErrorFoundAction, none >,
            Row< Error, ResetEvent, AllOk, ResetAction, none >
            KARABO_FSM_TABLE_END

            // Name, Transition-Table, Initial-State, Context
            KARABO_FSM_STATE_MACHINE(FemMachine, TopFemStt, AllOk, Self)

            void startFsm() {
                // Define context first
                KARABO_FSM_CREATE_MACHINE(FemMachine, m_fsm);
                KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm)
                KARABO_FSM_SET_CONTEXT_SUB(this, m_fsm, AllOk)
                KARABO_FSM_START_MACHINE(m_fsm)
            }

        private:

            KARABO_FSM_DECLARE_MACHINE(FemMachine, m_fsm);
        };
    }
}

#endif	/* KARABO_CORE_DSSCRECVFSM_HH */

