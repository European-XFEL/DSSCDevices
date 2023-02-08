#############################################################################
# Author: parenti
# Created on April 30, 2019, 04:41 PM
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import math
from queue import Queue
import socket
import threading
import time

import parse

from karabo.bound import (
    FLOAT_ELEMENT, INT32_ELEMENT, KARABO_CLASSINFO, NODE_ELEMENT,
    OVERWRITE_ELEMENT, SLOT_ELEMENT, STRING_ELEMENT, UINT64_ELEMENT, Hash,
    PythonDevice, State, Worker
)

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
    last_trainid_parser = parse.compile(
        "SIB[0]!LastTrainID:{lastTrainId:d}!SIB STATE:{sibState:x}!"
        "SIB Operation Mode:{som:d}"
    )
    tstatus_parser = parse.compile(
        "SIB[0]!T_STATUS:{tStatus}[PPFC:{ppfc:d}!IOB:{iob:d}!ASIC:{asic:d}]!"
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
        "PPFC_T2:{:g}[{:d}]!PPFC_T3:{ppfcT3:g}[{ppfcT3Status:d}]!"
        "PPFC_T4:{ppfcT4:g}[{ppfcT4Status:d}]"
    )
    iob_parser = parse.compile(
        "SIB[0]!IOB[{iob:d}]!IOB_T1:{iobT1:g}[{iobT1Status:d}]!"
        "IOB_T2:{iobT2:g}[{iobT2Status:d}]!"
        "IOB_T3:{iobT3:g}[{iobT3Status:d}]!"
        "IOB_T4:{iobT4:g}[{iobT4Status:d}]"
    )
    mg_parser = parse.compile(
        "SIB[0]!MG:[{mg:d}]!P1:{p1:g}[{p1Status:d}]!P2:{:g}[{:d}]!"
        "P3:{:g}[{:d}]!P4:{p4:g}[{p4Status:d}]"
    )
    t1_parser = parse.compile(
        "SIB[0]!H1:{:g}[{:d}]!H2:{h2:g}[{h2Status:d}]!T1:{:g}!T2:{t2:g}"
    )
    asic_trainid_parser = parse.compile("ASIC!TrainID:{trainId:d}")
    asic_parser = parse.compile(
        "({asic_nr:d})[{ASICXX.status:d}]{ASICXX.t0:g}:{ASICXX.t1:g}"
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

            INT32_ELEMENT(expected).key("log")
            .displayedName("LOG Mode")
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
            .alarmHigh(0).needsAcknowledging(False)
            .commit(),

            INT32_ELEMENT(expected).key("globalQloop")
            .displayedName("GloballQloop")
            .readOnly()
            .alarmHigh(0).needsAcknowledging(False)
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

            INT32_ELEMENT(expected).key("tStatus")
            .displayedName("T_STATUS")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("ppfc")
            .displayedName("PPFC")
            .readOnly()
            .alarmHigh(0).needsAcknowledging(False)
            .commit(),

            INT32_ELEMENT(expected).key("iob")
            .displayedName("IOB")
            .readOnly()
            .alarmHigh(0).needsAcknowledging(False)
            .commit(),

            INT32_ELEMENT(expected).key("asic")
            .displayedName("ASIC")
            .readOnly()
            .alarmHigh(0).needsAcknowledging(False)
            .commit(),

            INT32_ELEMENT(expected).key("pStatus")
            .displayedName("P_STATUS")
            .readOnly()
            .alarmHigh(0).needsAcknowledging(False)
            .commit(),

            INT32_ELEMENT(expected).key("hStatus")
            .displayedName("H_STATUS")
            .readOnly()
            .alarmHigh(0).needsAcknowledging(False)
            .commit(),

            INT32_ELEMENT(expected).key("cableStatus")
            .displayedName("CABLE_STATUS")
            .readOnly()
            .alarmHigh(0).needsAcknowledging(False)
            .commit(),

            INT32_ELEMENT(expected).key("coolStatus")
            .displayedName("COOL_STATUS")
            .readOnly()
            .alarmHigh(0).needsAcknowledging(False)
            .commit(),

            INT32_ELEMENT(expected).key("expStatus")
            .displayedName("EXP_STATUS")
            .readOnly()
            .alarmHigh(0).needsAcknowledging(False)
            .commit(),

            FLOAT_ELEMENT(expected).key("vccSum")
            .displayedName("VCCSUM")
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
            .alarmHigh(0).needsAcknowledging(False)
            .commit(),

            FLOAT_ELEMENT(expected).key("p1")
            .displayedName("P1")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("p1Status")
            .displayedName("P1_STATUS")
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

            FLOAT_ELEMENT(expected).key("h2")
            .displayedName("H2")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("h2Status")
            .displayedName("H2_STATUS")
            .readOnly()
            .commit(),

            FLOAT_ELEMENT(expected).key("t2")
            .displayedName("T2")
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

        self.connectWorker = Worker(self.connect_to_sib, 5000, -1)
        self.connectWorker.daemon = True

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
            self.log.INFO(f"Connected to SIB. Reply: {reply}")

            self.authenticate()  # authenticate

            configuration = Hash('som', self['som'], 'log', self['log'])
            self.configure_sib(configuration)  # send initial configuration

            self.socket.settimeout(1)
            self.updateState(State.NORMAL)
        except Exception as e:
            self.log.ERROR(f"Cannot connect to SIB: {e}")
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
            reply = self.socket.recv(self.BUFFER_SIZE)
            self.log.INFO(f"Authentication successful. Reply: {reply}")
        except Exception as e:
            self.log.ERROR(f"Cannot authenticate: {e}")
            self.updateState(State.ERROR)

    def restart(self):
        self.log.INFO("Restarting SIB")
        self.socket.send(f"RESET;{DsscSIB.cmnd_terminator}".encode())

    def consumer(self):
        while True:
            data_row, ts = self.data_queue.get()
            self.process_data_row(data_row, ts)

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
                ts = self.getActualTimestamp()
                data += new_data  # concatenate to old data
                self.log.DEBUG(f"Data lenght: tot={len(data)} "
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
                    # Queue data for processing
                    self.data_queue.put((data_row, ts), block=False)

                counter = 10  # reset counter
            except Exception as e:
                if counter > 0:
                    if self['log'] > 0:
                        # if LOG > 0 updates are expected all the time
                        counter -= 1
                    self.log.DEBUG(f"Listener caught this: {e}")
                else:
                    self.log.INFO(f"Lost connection with SIB: {e}")
                    self.socket = None
                    self.updateState(State.UNKNOWN)

    def set_properties(self, result, ts, debug=False):
        h = Hash()
        epsilon = self['epsilon']
        try:
            for key, value in result.named.items():
                if not isinstance(value, str) and abs(value) == 65535:
                    # 65535 means NaN
                    value = math.nan

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

                if debug:
                    self.log.INFO(f"Setting {h}")
                self.set(h, ts)  # Bulk set

        except Exception as e:
            self.log.WARN(f"Could not set {h}. {e}")

    def is_update_required(self, key, value, epsilon):
        """Return True if 'value' requires an update."""
        value_on_device = self[key]
        if isinstance(value_on_device, float):
            if math.isclose(value, value_on_device, rel_tol=epsilon):
                # float: skip update if change smaller then tolerance
                return False
        elif value is math.nan and value_on_device is math.nan:
            return False
        elif value == value_on_device:
            # default: skip update if there was no value change
            return False

        return True

    def process_data_row(self, data, ts):
        data = data.replace('|', '!')  # '|' cannot be parsed correctly

        if data.startswith('LOG['):
            # LOG data
            result = DsscSIB.log_parser.parse(data)
            if result is not None:
                self.set_properties(result, ts)

        elif data.startswith('SIB['):
            if 'LastTrainID:' in data:
                result = DsscSIB.last_trainid_parser.parse(data)
            elif 'T_STATUS:' in data:
                result = DsscSIB.tstatus_parser.parse(data)
            elif 'VCCSUM:' in data:
                result = DsscSIB.vccsum_parser.parse(data)
            elif 'PPFC_T1:' in data:
                result = DsscSIB.ppfc_parser.parse(data)
            elif 'IOB_T1:' in data:
                result = DsscSIB.iob_parser.parse(data)
            elif 'MG:' in data:
                result = DsscSIB.mg_parser.parse(data)
            elif 'H1:' in data:
                result = DsscSIB.t1_parser.parse(data)
            else:
                # no match -> data won't be parsed
                return

            if result is not None:
                self.set_properties(result, ts)

        elif data.startswith('ASIC'):
            # ASIC data
            result = DsscSIB.asic_trainid_parser.parse(data)
            if result is not None:
                self.set_properties(result, ts)

        elif data.startswith('(0)'):
            # ASIC data
            h = Hash()
            epsilon = self['epsilon']
            for row in data.rstrip(';').split(';'):
                result = DsscSIB.asic_parser.parse(row)

                if result is None:
                    continue

                if 'asic_nr' not in result.named:
                    continue

                asic_nr = result.named['asic_nr']
                if not (0 <= asic_nr < DsscSIB.ASICS):
                    continue

                for k, value in result.named.items():
                    if k == 'asic_nr':
                        continue  # not a Karabo parameter
                    key = k.replace('ASICXX', f'asic{asic_nr:02d}')

                    if abs(value) == 65535:
                        value = math.nan  # 65535 means NaN
                    if self.is_update_required(key, value, epsilon):
                        h[key] = value

            try:
                if not h.empty():
                    self.log.DEBUG(f"Setting {h}")
                    self.set(h, ts)  # bulk set
            except Exception as e:
                self.log.WARN(f"Could not set {h}. {e}")

    def configure_sib(self, configuration):
        if configuration.has('som'):
            som = configuration['som']
            self.socket.send(f"SOM {som};{DsscSIB.cmnd_terminator}".encode())

        if configuration.has('log'):
            log = configuration['log']
            self.socket.send(f"LOG {log};{DsscSIB.cmnd_terminator}".encode())
