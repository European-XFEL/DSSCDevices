############################################################################
# Author: parenti
# Created on April 30, 2019, 04:41 PM
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import math
import socket
import threading
import time
from queue import Full, Queue

import parse

from karabo.bound import (
    FLOAT_ELEMENT, INT32_ELEMENT, KARABO_CLASSINFO, NODE_ELEMENT,
    OVERWRITE_ELEMENT, SLOT_ELEMENT, STRING_ELEMENT, UINT64_ELEMENT, Hash,
    MetricPrefix, PythonDevice, State, Unit, Worker)

from ._version import version as deviceVersion


@KARABO_CLASSINFO("DsscSIB", deviceVersion)
class DsscSIB(PythonDevice):
    ASICS = 64  # the number of ASICs
    cmnd_terminator = "\r\n"  # the command terminator

    status_str = {
        0x00: "Start",
        0x03: "Setting-up",
        0x13: "Warning (setting-up)",
        0x05: "Ready",
        0x15: "Warning (ready)",
        0x06: "Operation",
        0x04: "Warning",
        0x02: "Error",
        0x14: "SIB interCOM Warning"
    }

    log_parser = parse.compile(
        "LOG[{logCounter:d}];SIB_MASTER:{sibMaster:d}!"
        "LocalQloop:{localQloop:d}!GlobalQloop:{globalQloop:d}!"
        "QloopAddress:{qloopAddress:x}!SIBs:{sibs:d}"
    )
    log_counter_v2_parser = parse.compile(
        "LOG[{logCounter:d}][{decision}]"
    )
    log_v2_parser = parse.compile(
        "SIB_MASTER:{sibMaster:d}!"
        "LocalQloop:{localQloop:d}!GlobalQloop:{globalQloop:d}!"
        "QloopAddress:{qloopAddress:x}!SIBs:{sibs:d}"
    )
    last_trainid_parser = parse.compile(
        "SIB[0]!LastTrainID:{lastTrainId:d}!SIB STATE:{sibState:x}!"
        "SIB Operation Mode:{som:d}"
    )
    tstatus_parser = parse.compile(
        "SIB[0]!T_STATUS:{tStatus:g}[PPFC:{ppfc:d}!IOB:{iob:d}!ASIC:{asic:d}]!"
        "P_STATUS:{pStatus:d}!H_STATUS:{hStatus:d}!"
        "CABLE_STATUS:{cableStatus:d}!COOL_STATUS:{coolStatus:d}!"
        "EXP_STATUS:{expStatus:d}"
    )
    tstatus_v2_parser = parse.compile(
        "SIB[0]!T_STATUS:{tStatus:g}[PPFC:{ppfc:d}!IOB:{iob:d}!Therm:{therm:d}!ASIC:{asic:d}]!"
        "P_STATUS:{pStatus:d}!H_STATUS:{hStatus:d}!"
        "CABLE_STATUS:{cableStatus:d}!COOL_STATUS:{coolStatus:d}!"
        "EXP_STATUS:{expStatus:d}"
    )
    vccsum_parser = parse.compile(
        "SIB[0]!VCCSUM:{vccSum:g}[{vccSumStatus:d}]!LV:{lv:d}!HV1:{hv1:d}!"
        "HV2:{hv2:d}!PICO:{pico:d}"
    )
    ppfc_parser = parse.compile(
        "SIB[0]!PPFC[{:d}]!PPFC_T1:{ppfcT1:g}[{ppfcT1Status:d}]!"
        "PPFC_T2:{ppfcT2:g}[{ppfcT2Status:d}]!PPFC_T3:{ppfcT3:g}[{ppfcT3Status:d}]!"
        "PPFC_T4:{ppfcT4:g}[{ppfcT4Status:d}]"
    )
    iob_parser = parse.compile(
        "SIB[0]!IOB[{iob:d}]!IOB_T1:{iobT1:g}[{iobT1Status:d}]!"
        "IOB_T2:{iobT2:g}[{iobT2Status:d}]!"
        "IOB_T3:{iobT3:g}[{iobT3Status:d}]!"
        "IOB_T4:{iobT4:g}[{iobT4Status:d}]"
    )
    mg_parser = parse.compile(
        "SIB[0]!MG:[{mg:d}]!P1:{p1:g}[{p1Status:d}]!P2:{p2:g}[{p2Status:d}]!"
        "P3:{p3:g}[{p3Status:d}]!P4:{p4:g}[{p4Status:d}]"
    )
    t1_parser = parse.compile(
        "SIB[0]!H1:{h1:g}[{h1Status:d}]!H2:{h2:g}[{h2Status:d}]!T1:{t1:g}!T2:{t2:g}"
    )
    t1_v2_parser = parse.compile(
        "SIB[0]!"
        "DP1:{dp1:g}[{dp1Status:d}]!"
        "DP2:{dp2:g}[{dp2Status:d}]!"
        "H1:{h1:g}!H2:{h2:g}!"
        "T1:{t1:g}!T2:{t2:g}"
    )
    asic_trainid_parser = parse.compile("ASIC!TrainID:{trainId:d}")
    asic_parser = parse.compile(
        "({asic_nr:d})[{ASICXX.status:d}]{ASICXX.t0:g}:{ASICXX.t1:g}"
    )
    ntc_parser = parse.compile(
        "SIB[0]!NTC[{:d}]!"
        "NTC1:{ntc1:g}[{ntc1Status:d}]!"
        "NTC2:{ntc2:g}[{ntc2Status:d}]!"
        "NTC3:{ntc3:g}[{ntc3Status:d}]!"
        "NTC4:{ntc4:g}[{ntc4Status:d}]"
    )

    @staticmethod
    def expectedParameters(expected):
        (
            OVERWRITE_ELEMENT(expected).key("state")
            .setNewOptions(State.UNKNOWN, State.NORMAL, State.ERROR)
            .commit(),

            STRING_ELEMENT(expected).key("hostname")
            .displayedName("Hostname")
            .description("The SIB hostname or IP.")
            .assignmentMandatory()
            .init()
            .commit(),

            INT32_ELEMENT(expected).key("port")
            .displayedName("Port")
            .description("The port for TCP/IP communication with the SIB.")
            .assignmentOptional().defaultValue(1000)
            .minInc(0)
            .init()
            .commit(),

            STRING_ELEMENT(expected).key("version")
            .displayedName("F/W Version")
            .readOnly()
            .commit(),

            UINT64_ELEMENT(expected).key("lastUpdated")
            .displayedName("Last Updated")
            .description("The time elapsed since the last update received "
                         "from the SIB.")
            .unit(Unit.SECOND)
            .readOnly().defaultValue(0)
            .commit(),

            FLOAT_ELEMENT(expected).key("epsilon")
            .displayedName("Epsilon")
            .description("The minimum relative change for publishing a "
                         "parameter update.")
            .assignmentOptional().defaultValue(0.01)
            .minInc(0.01)
            .reconfigurable()
            .commit(),

            SLOT_ELEMENT(expected).key("restart")
            .displayedName("SIB Restart")
            .description("Restart the SIB. The SIB will reset by forcing a "
                         "watchdog timeout. It is needed if a HARD FAILURE "
                         "triggered the ERROR state in the SIB. NOTE: At "
                         "startup the CrateEn signal is automatically "
                         "disabled. This command should never be used during "
                         "detector operation as will automatically switch off "
                         "the MPOD crate..")
            .allowedStates(State.ERROR, State.NORMAL)
            .commit(),

            INT32_ELEMENT(expected).key("som")
            .displayedName("SIB Operation Mode")
            .description("Sets the SIB operation mode. 0: Safe mode (The SIB "
                         "will work in safe mode.) 1: SuperUser Mode (The SIB "
                         "will work normally but a WARNING condition will not "
                         "trigger the CrateEn and Cooling INTERLOCKS.) "
                         "2: Manual unsafe mode (All interlocks are disabled. "
                         "The SIB status can be ignored.)")
            .assignmentOptional().defaultValue(0)
            .minInc(0).maxInc(2)
            .allowedStates(State.NORMAL)
            .reconfigurable()
            .commit(),

            INT32_ELEMENT(expected).key("logLevel")
            .displayedName("Log Level")
            .description("Logging mode. 0: readings logging is disabled; 1: "
                         "logs only SIB-related sensor data; 2: sends only "
                         "ASIC calibrated data; 3: sends SIB sensor and ASIC "
                         "data).")
            .assignmentOptional().defaultValue(3)
            .minInc(0).maxInc(3)
            .allowedStates(State.NORMAL)
            .reconfigurable()
            .commit(),

            UINT64_ELEMENT(expected).key("trainId")
            .displayedName("TrainID")
            .readOnly()
            .commit(),
        )

        for i in range(DsscSIB.ASICS):
            node = f"asic{i:02}"
            (
                NODE_ELEMENT(expected).key(node)
                .displayedName(f"ASIC {i:2}")
                .commit(),

                INT32_ELEMENT(expected).key(f"{node}.status")
                .displayedName("Status")
                .readOnly()
                .commit(),

                FLOAT_ELEMENT(expected).key(f"{node}.t0")
                .displayedName("T0")
                .readOnly()
                .commit(),

                FLOAT_ELEMENT(expected).key(f"{node}.t1")
                .displayedName("T1")
                .readOnly()
                .commit(),
            )

        (
            INT32_ELEMENT(expected).key("logCounter")
            .displayedName("LOG Counter")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("sibMaster")
            .displayedName("SIB_MASTER")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("localQloop")
            .displayedName("LocalQloop")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("globalQloop")
            .displayedName("GloballQloop")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("qloopAddress")
            .displayedName("QloopAddress")
            .hex()
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("sibs")
            .displayedName("SIBs")
            .readOnly()
            .commit(),

            UINT64_ELEMENT(expected).key("lastTrainId")
            .displayedName("LastTrainID")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("sibState")
            .displayedName("SIB_STATE")
            .readOnly()
            .commit(),

            STRING_ELEMENT(expected).key("decision")
            .displayedName("Decision")
            .description("Previous state -> current state: qloop global status"
                         " : qloop status : cable status : experiment ok "
                         "status : temperature status : humidity status : "
                         "pressure status. 0 means ok; 1 is error; 2 is "
                         "warning; 3 is undefined")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("tStatus")
            .displayedName("T_STATUS")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("ppfc")
            .displayedName("PPFC")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("iob")
            .displayedName("IOB")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("therm")
            .displayedName("THERM")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("asic")
            .displayedName("ASIC")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("pStatus")
            .displayedName("P_STATUS")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("hStatus")
            .displayedName("H_STATUS")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("cableStatus")
            .displayedName("CABLE_STATUS")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("coolStatus")
            .displayedName("COOL_STATUS")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("expStatus")
            .displayedName("EXP_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("vccSum")
            .displayedName("VCCSUM")
            .unit(Unit.VOLT)
            .metricPrefix(MetricPrefix.MILLI)
            .description("In mV, 3V when fully powered")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("vccSumStatus")
            .displayedName("VCCSUM_STATUS")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("lv")
            .displayedName("LV")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("hv1")
            .displayedName("HV1")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("hv2")
            .displayedName("HV2")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("pico")
            .displayedName("PICO")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("ppfcT1")
            .displayedName("PPFC_T1")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("ppfcT1Status")
            .displayedName("PPFC_T1_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("ppfcT2")
            .displayedName("PPFC_T2")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("ppfcT2Status")
            .displayedName("PPFC_T2_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("ppfcT3")
            .displayedName("PPFC_T3")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("ppfcT3Status")
            .displayedName("PPFC_T3_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("ppfcT4")
            .displayedName("PPFC_T4")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("ppfcT4Status")
            .displayedName("PPFC_T4_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("iobT1")
            .displayedName("IOB_T1")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("iobT1Status")
            .displayedName("IOB_T1_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("iobT2")
            .displayedName("IOB_T2")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("iobT2Status")
            .displayedName("IOB_T2_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("iobT3")
            .displayedName("IOB_T3")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("iobT3Status")
            .displayedName("IOB_T3_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("iobT4")
            .displayedName("IOB_T4")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("iobT4Status")
            .displayedName("IOB_T4_STATUS")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("mg")
            .displayedName("MG")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("p1")
            .displayedName("P1")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("p1Status")
            .displayedName("P1_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("p2")
            .displayedName("P2")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("p2Status")
            .displayedName("P2_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("p3")
            .displayedName("P3")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("p3Status")
            .displayedName("P3_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("p4")
            .displayedName("P4")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("p4Status")
            .displayedName("P4_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("dp1")
            .displayedName("DP1")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("dp1Status")
            .displayedName("DP1_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("dp2")
            .displayedName("DP2")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("dp2Status")
            .displayedName("DP2_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("h1")
            .displayedName("H1")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("h1Status")
            .displayedName("H1_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("h2")
            .displayedName("H2")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("h2Status")
            .displayedName("H2_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("t1")
            .displayedName("T1")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("t2")
            .displayedName("T2")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("ntc1")
            .displayedName("NTC1")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("ntc1Status")
            .displayedName("NTC1_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("ntc2")
            .displayedName("NTC2")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("ntc2Status")
            .displayedName("NTC2_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("ntc3")
            .displayedName("NTC3")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("ntc3Status")
            .displayedName("NTC3_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("ntc4")
            .displayedName("NTC4")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("ntc4Status")
            .displayedName("NTC4_STATUS")
            .readOnly()
            .commit(),
        )

    def __init__(self, configuration):
        # always call PythonDevice constructor first!
        super(DsscSIB, self).__init__(configuration)

        # Define the first function to be called after the constructor has
        # finished
        self.registerInitialFunction(self.initialization)

        # Initialize your member variables here...
        self.socket = None

        self.BUFFER_SIZE = 1024

        self.connectWorker = Worker(self.connect_to_sib, 5000)

        self.last_update = time.time()
        self.last_update_interval = 1
        self.last_update_worker = Worker(self.last_update_counter, 1000)

        # Process data in a separate thread
        self.data_queue = Queue(maxsize=20)
        self.processThread = threading.Thread(target=self.consumer)
        self.processThread.daemon = True
        self.processThread.start()  # start processor

        self.listenThread = threading.Thread(target=self.listener)
        self.listenThread.daemon = True
        self.listenThread.start()

    def initialization(self):
        """ This method will be called after the constructor.

        If you need methods that can be callable from another device or GUI
        you may register them here:
        self.KARABO_SLOT(self.myslot1)
        self.KARABO_SLOT(self.myslot2)
        ...
        Corresponding methods (myslot1, myslot2, ...) should be defined in this
        class
        """
        # Define your slots here
        self.KARABO_SLOT(self.restart)

        # Connect to SIB in a Worker
        self.connectWorker.start()

        # Count elapsed time since last update
        self.last_update_worker.start()

    def preReconfigure(self, configuration):
        self.configure_sib(configuration)

    def connect_to_sib(self):
        if self['state'] != State.UNKNOWN:
            # Already connected
            return

        try:
            hostname = self['hostname']
            port = self['port']
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(5)
            self.socket.connect((hostname, port))  # connect
            reply = self.socket.recv(self.BUFFER_SIZE)
            self.logger.info(f"Connected to SIB. Reply: {reply}")

            self.authenticate()  # authenticate

            configuration = Hash('som', self['som'], 'log', self['logLevel'])
            self.configure_sib(configuration)  # send initial configuration

            self.socket.settimeout(1)
            self.updateState(State.NORMAL)
        except Exception as e:
            self.logger.error(f"Cannot connect to SIB: {e}")
            return

    def authenticate(self):
        try:
            # Stop logging
            self.socket.send(f"LOG 0;{DsscSIB.cmnd_terminator}".encode())

            # Flush socket
            while True:
                try:
                    self.socket.recv(self.BUFFER_SIZE)
                except Exception:
                    break

            # Finally authenticate
            self.socket.send(f"AUTH;{DsscSIB.cmnd_terminator}".encode())
            self.socket.recv(self.BUFFER_SIZE)  # SIB prompts for password
            password = "DSSC2018"
            self.socket.send(f"{password}{DsscSIB.cmnd_terminator}".encode())
            on_connect = self.socket.recv(self.BUFFER_SIZE)

            self.socket.send(f"VER;{DsscSIB.cmnd_terminator}".encode())
            fw_version = self.socket.recv(self.BUFFER_SIZE).decode().strip()
            self.set("version", fw_version)

            self.logger.info(f"Authentication successful. Reply: {on_connect} - {fw_version}")
        except Exception as e:
            self.logger.error(f"Cannot authenticate: {e}")
            self.updateState(State.ERROR)

    def last_update_counter(self):
        if self['state'] == State.UNKNOWN:
            # Not connected
            return

        elapsed_time = time.time() - self.last_update
        if elapsed_time > self.last_update_interval:
            self['lastUpdated'] = elapsed_time
            if self.last_update_interval < 10:
                # more frequent updates in the beginning
                self.last_update_interval += 1
            else:
                self.last_update_interval += 10

    def reset_last_updated(self):
        self.last_update = time.time()
        self.last_update_interval = 1
        if self['lastUpdated'] > 0:
            self['lastUpdated'] = 0

    def restart(self):
        self.logger.info("Restarting SIB")
        self.socket.send(f"RESET;{DsscSIB.cmnd_terminator}".encode())

    def consumer(self):
        while True:
            data_row, ts = self.data_queue.get()
            data = self.process_data(data_row)
            self.set_properties(data, ts)

    def listener(self):
        data = ""
        counter = 10
        while True:
            if self.socket is None or self['state'] == State.UNKNOWN:
                # not connected
                time.sleep(1)
                continue

            try:
                new_data = self.socket.recv(self.BUFFER_SIZE).decode('ascii')
                self.reset_last_updated()  # reset "last updated" counters
            except socket.timeout as e:
                if counter > 0:
                    if self['logLevel'] > 0:
                        # if LOG > 0 updates are expected all the time
                        counter -= 1
                        self.logger.debug(f"Listener caught this: {e}")
                else:
                    self.logger.info("Lost connection with SIB")
                    self.socket = None
                    self.updateState(State.UNKNOWN)
                    self.reset_last_updated()  # reset "last updated" counters
                continue

            ts = self.getActualTimestamp()
            data += new_data  # concatenate to old data
            self.logger.debug(f"Data lenght: tot={len(data)} "
                           f"new={len(new_data)} Bytes")
            data_split = data.split(DsscSIB.cmnd_terminator)
            if data.endswith(DsscSIB.cmnd_terminator):
                # complete data -> process all
                data = ""
            else:
                # incomplete data -> keep last chunk
                data = data_split[-1]
                data_split = data_split[:-1]

            for data_row in data_split:
                try:
                    # Queue data for processing
                    self.data_queue.put((data_row, ts), block=False)
                except Full:
                    self.logger.error("'data_queue' full. Discarding new data!")

            counter = 10  # reset counter

    def set_properties(self, result, ts):
        h = Hash()
        epsilon = self['epsilon']
        try:
            for key, value in result.items():
                if self.is_update_required(key, value, epsilon):
                    # valid update
                    h[key] = value

            if not h.empty():
                if 'sibState' in h:
                    sib_state = h['sibState']
                    status = DsscSIB.status_str.get(
                        sib_state, f"Unknown SIB state ({sib_state:#x})"
                    )
                    h['status'] = status

                self.logger.debug(f"Setting {h}")
                self.set(h, ts)  # Bulk set

        except Exception as e:
            self.logger.warning(f"Could not set {h}. {e}")

    def is_update_required(self, key, value, epsilon):
        """Return True if 'value' requires an update."""
        value_on_device = self[key]
        if isinstance(value_on_device, float):
            if math.isclose(value, value_on_device, rel_tol=epsilon):
                # float: skip update if change smaller than tolerance
                return False
        elif value is math.nan and value_on_device is math.nan:
            return False
        elif value == value_on_device:
            # default: skip update if there was no value change
            return False

        return True

    @staticmethod
    def process_data(data: str) -> dict:
        data = data.replace('|', '!')  # '|' cannot be parsed correctly
        result = dict()

        if 'SIB_MASTER' in data:  # LOG data
            if data.startswith('LOG['):
                result = DsscSIB.log_parser.parse(data).named
            elif data.startswith('SIB_MASTER'):
                result = DsscSIB.log_v2_parser.parse(data).named

        elif data.startswith('LOG['):
            result = DsscSIB.log_counter_v2_parser.parse(data).named

        elif data.startswith('SIB['):
            if 'LastTrainID:' in data:
                result = DsscSIB.last_trainid_parser.parse(data).named
            elif 'T_STATUS:' in data:
                if 'Therm:' in data:
                    result = DsscSIB.tstatus_v2_parser.parse(data).named
                else:
                    result = DsscSIB.tstatus_parser.parse(data).named
            elif 'VCCSUM:' in data:
                result = DsscSIB.vccsum_parser.parse(data).named
            elif 'PPFC_T1:' in data:
                result = DsscSIB.ppfc_parser.parse(data).named
            elif 'IOB_T1:' in data:
                result = DsscSIB.iob_parser.parse(data).named
            elif 'MG:' in data:
                result = DsscSIB.mg_parser.parse(data).named
            elif 'DP1:' in data:
                result = DsscSIB.t1_v2_parser.parse(data).named
            elif 'H1:' in data:
                result = DsscSIB.t1_parser.parse(data).named
            elif 'NTC' in data:
                result = DsscSIB.ntc_parser.parse(data).named

        elif data.startswith('ASIC'):
            result = DsscSIB.asic_trainid_parser.parse(data).named

        elif data.startswith('(0)'):
            for row in data.rstrip(';').split(';'):
                res = DsscSIB.asic_parser.parse(row)

                if res is None or 'asic_nr' not in res.named:
                    continue

                asic_nr = res.named['asic_nr']
                if not (0 <= asic_nr < DsscSIB.ASICS):
                    continue

                for k, value in res.named.items():
                    if k == 'asic_nr':
                        continue  # not a Karabo parameter
                    key = k.replace('ASICXX', f'asic{asic_nr:02d}')

                    result[key] = value

        # Sanitize unitialized values to NaN
        # The SIB sets unitialized values to either + or - 65535
        # Convert this to NaN to highlight to operators that these are unset
        for key in result:
            if result[key] in (65535, -65535):
                result[key] = math.nan

        return result

    def configure_sib(self, configuration):
        if configuration.has('som'):
            som = configuration['som']
            self.socket.send(f"SOM {som};{DsscSIB.cmnd_terminator}".encode())

        if configuration.has('logLevel'):
            log_level = configuration['logLevel']
            self.socket.send(f"LOG {log_level};{DsscSIB.cmnd_terminator}".encode())
