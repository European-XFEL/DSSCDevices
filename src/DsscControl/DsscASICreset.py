import asyncio
import datetime
import os
import time

from asyncio import CancelledError
from collections import ChainMap

from karabo.middlelayer import (
    AccessMode,
    allCompleted,
    Bool,
    Configurable,
    connectDevice,
    DaqPolicy,
    Device,
    gather,
    Hash,
    Node,
    OutputChannel,
    setWait,
    sleep,
    Slot,
    slot,
    State,
    String,
    UInt32,
    VectorHash,
    VectorString,
    waitUntil,
)

from ._version import version as deviceVersion
from .schemata import PptRowSchema2
from .scenes.asic_reset_scene import get_scene


class DsscASICreset(Device):
    # provide version for classVersion property
    __version__ = deviceVersion

    activeQuadrant = UInt32(displayedName="Active Quadrant",
                            defaultValue=1,
                            accessMode=AccessMode.READONLY)

    activeModule = UInt32(displayedName="Active Module",
                          defaultValue=1,
                          accessMode=AccessMode.READONLY)

    pptConfig = VectorHash(
        rows=PptRowSchema2,
        displayedName="Ppt devices to connect",
        defaultValue=[Hash([('deviceId', f"SCS_CDIDET_DSSC/FPGA/PPT_Q{q}"),
                            ('quadrantId', f"Q{q}")]) for q in range(1, 5)])

    ppt_dev = []  # short list of active ppt devices
    ppt_dev_indx = {}  # index map of connected ppt devices
    ppt_by_indx = {}

    connectedPptDev = String(displayedName="Connected PPT",
                             defaultValue="",
                             accessMode=AccessMode.READONLY)
    lastProgASIC = 0
    lastModule0Indx = 0

    def __init__(self, configuration):
        super(DsscASICreset, self).__init__(configuration)

    async def onInitialization(self):
        """ This method will be called when the device starts.

            Define your actions to be executed after instantiation.
        """
        await self.injectCtrlSlots()
        await self.connectDevices()

    @Slot(displayedName="Connect",
          description="Connect to devices",
          allowedStates={State.UNKNOWN, State.ON})
    async def connectDevices(self):

        self.state = State.CHANGING

        to_connect = {}
        for i, row in enumerate(self.pptConfig.value):
            pptdevnamestr = row[0]
            to_connect[str(i)] = connectDevice(pptdevnamestr)

        done, pending, error = await allCompleted(**to_connect, timeout=10)
        error = ChainMap(pending, error)

        if len(error):
            failmsg = [r[0] for i, r in enumerate(self.pptConfig.value)
                       if str(i) in error]
            self.status = "Failed connecting PPT: {}".format(failmsg)
            self.state = State.UNKNOWN
            return

        self.ppt_devices = [None if str(i) in error else done[str(i)]
                            for i, r in enumerate(self.pptConfig.value)]
        self.ppt_dev = list(done.values())
        self.ppt_dev_indx = {v: int(k) for k, v in enumerate(self.ppt_devices) if v}
        self.ppt_by_indx = {int(k): v for k, v in enumerate(self.ppt_devices) if v}

        self.connectedPptDev = ", ".join(["Q{}".format(str(i + 1)) for i, d
                                          in enumerate(self.ppt_devices) if d])

        self.state = State.ON

    @Slot(displayedName="Reset all",
          description="Reset ppt devices and settings",
          allowedStates={State.ON, State.UNKNOWN})
    async def resetAll(self):
        self.ppt_dev = []  # short list of active ppt devices
        self.ppt_dev_indx = {}  # index map of connected ppt devices
        self.ppt_by_indx = {}
        self.activeModule = 1
        self.activeQuadrant = 1
        self.state = State.UNKNOWN

    @Slot(displayedName="Reset",
          description="Reset",
          allowedStates={State.ERROR})
    async def reset(self):
        if self.ppt_dev != []:
            self.state = State.ON
            return
        self.state = State.UNKNOWN

    def getQuadrantModuleFrom0index(self, indx):
        return int(indx) // 4 + 1, int(indx) % 4 + 1

    async def setBoolModuleOn(self, indx):
        if (self.lastModule0Indx == indx):
            return
        q, m = self.getQuadrantModuleFrom0index(self.lastModule0Indx)
        print("q,m: ", q, " ", m)
        boolName = "boolQ{}M{}".format(q, m)
        if getattr(self, boolName):
            setattr(self, boolName, False)
        q, m = self.getQuadrantModuleFrom0index(indx)
        boolName = "boolQ{}M{}".format(q, m)
        setattr(self, boolName, True);
        self.lastModule0Indx = indx
        self.state = State.ON

    def getMappedASIC(self, indx):
        if self.activeQuadrant < 3:
            return (indx + 8) % 16
        else:
            return (7 - indx) % 16

    async def programASIC(self, indx):
        self.state = State.CHANGING
        if self.lastProgASIC != indx:
            setattr(self, "boolASIC{}".format(self.lastProgASIC), False);
            setattr(self, "boolASIC{}".format(indx), True);
            self.lastProgASIC = indx
        pptdevice = None
        mappedIndx = self.getMappedASIC(indx)
        quad0Indx = int(self.activeQuadrant) - 1
        if quad0Indx in self.ppt_by_indx.keys():
            pptdevice = self.ppt_by_indx[quad0Indx]
        else:
            self.state = State.ON
            out_str = "PPT Q{} is not used".format(self.activeQuadrant)
            self.status = out_str
            self.log.INFO(out_str)
            return mappedIndx

        await setWait(pptdevice, activeModule=self.activeModule)
        await setWait(pptdevice, lmkOutputToProgram=mappedIndx)
        await pptdevice.programLMKOutput()
        self.state = State.ON
        return mappedIndx

    async def injectCtrlSlots(self):

        def makeActivate(q, m):
            @Slot(displayedName=f"Q{q}M{m}",
                  description=f"Q{q} module {m}",
                  allowedStates={State.ON})
            async def activate(self):
                self.status = f"Set Q{q} module {m}"
                self.activeQuadrant = q
                self.activeModule = m
                await self.setBoolModuleOn((self.activeQuadrant - 1) * 4 + self.activeModule - 1)
                self.log.INFO(self.status)

            return activate

        for i in range(16):
            q, m = i // 4 + 1, i % 4 + 1
            setattr(self.__class__, f"activateQ{q}M{m}", makeActivate(q, m))

        def makeASICslot(asicnum):
            @Slot(displayedName="_",
                  description="ASIC{}".format(asicnum),
                  allowedStates={State.ON})
            async def asicFun(self):
                self.status = "asic{}".format(await self.programASIC(asicnum))

            return asicFun

        for i in range(16):
            setattr(self.__class__, f"asic{i}", makeASICslot(i))

        for i in range(16):
            setattr(self.__class__, f"boolASIC{i}", Bool(displayedName="_",
                                                         defaultValue=True if i == 0 else False,
                                                         accessMode=AccessMode.READONLY))

        for i in range(16):
            q, m = i // 4 + 1, i % 4 + 1
            setattr(self.__class__, f"boolQ{q}M{m}", Bool(displayedName=f"Q{q}M{m}",
                                                          defaultValue=True if i == 0 else False,
                                                          accessMode=AccessMode.READONLY))

        await self.publishInjectedParameters()

        for i in range(16):
            setattr(self.__class__, f"asic{i}", makeASICslot(i))

        for i in range(16):
            setattr(self.__class__, f"boolASIC{i}", Bool(displayedName="_",
                                                         defaultValue=True if i == 0 else False,
                                                         accessMode=AccessMode.READONLY))

        for i in range(16):
            q, m = i // 4 + 1, i % 4 + 1
            setattr(self.__class__, f"boolQ{q}M{m}", Bool(displayedName=f"Q{q}M{m}",
                                                          defaultValue=True if i == 0 else False,
                                                          accessMode=AccessMode.READONLY))

        await self.publishInjectedParameters()

    availableScenes = VectorString(
        displayedName="Available Scenes",
        displayType="Scenes",
        accessMode=AccessMode.READONLY,
        defaultValue=["overview"],
        daqPolicy=DaqPolicy.OMIT
    )

    @slot
    def requestScene(self, params):
        ppt_ids = dict()
        for idx in range(4):
            key = f'ppt_q{idx+1}_id'
            try:
                ppt_ids[key] = self.ppt_by_indx[idx].deviceId
            except KeyError:  # We're connected to fewer than 4 quadrants
                ppt_ids[key] = "disconnected"

        payload = Hash(
            'success', True,
            'name', 'overview',
            'data' , get_scene(self.deviceId, **ppt_ids)
        )

        return Hash(
            'type', 'deviceScene',
            'origin', self.deviceId,
            'payload', payload
        )
