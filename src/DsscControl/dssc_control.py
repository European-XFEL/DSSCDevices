"""
Author: kirchgessner
Creation date: April 1, 2017, 01:41 PM
Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
"""
import asyncio
import datetime
import sys

from asyncio import (
    CancelledError,
    gather,
    TimeoutError,
    wait_for,
)
from collections import ChainMap
from typing import List

import numpy as np
from karabo.middlelayer import (
    AccessMode,
    Assignment,
    Bool,
    Configurable,
    Device,
    Hash,
    KaraboError,
    Node,
    OutputChannel,
    Overwrite,
    Slot,
    State,
    String,
    UInt16,
    UInt32,
    VectorHash,
    VectorString,
    allCompleted,
    background,
    connectDevice,
    lock,
    setWait,
    sleep,
    waitUntil,
    waitUntilNew,
)

from ._version import version as deviceVersion
from .schemata import (
    BaseDeviceRowSchema,
    ConfigPptRowSchema,
    MeasurementInfo,
    PptRowSchema,
)
from .utils import MeasurementConfig, get_sweep_vector, sanitize_da, strToByteArray

NUM_QUADRANTS = 4
NUM_MODULES = 16

POWER_PROCEDURE_ALLOWED_STATES = [
    State.UNKNOWN,  # Detector not connected to. (PPT)
    State.OFF,  # Detector powered off
    State.ON,  # Detector Initialized
]

DEVICE_STATES = [
    State.UNKNOWN,  # Devices not connected to (proxies or PPT to HW)
    State.OFF,  # Mapping the power procedure State.PASSIVE to OFF
    State.CHANGING,  # Used when creating proxies or inherited from PPT or power procedure
    State.ON,  # Detector initialized (inherited from PPT)
    State.ACQUIRING,  # Detector acquiring (fuses PPT ACQUIRING and STARTED)
    State.ERROR,  # Any exceptions on this device, PPT, or power procedure
    State.INIT,  # Creating proxies
    State.ACTIVE,  # Internal, used when doing procedures (eg. darks or trim)
]


class DsscControl(Device):

    # provide version for classVersion property
    __version__ = deviceVersion

    class ChannelNode(Configurable):
        measurementInfo = Node(MeasurementInfo)

    state = Overwrite(
        defaultValue=State.UNKNOWN,
        displayedName="State",
        options=DEVICE_STATES)

    pptConfig = VectorHash(
        rows=PptRowSchema,
        displayedName="Ppt devices to connect",
        defaultValue=[Hash([('deviceId', "SCS_CDIDET_DSSC/FPGA/PPT_Q{}".format(i)),
                            ('quadrantId', "Q{}".format(i)), ('connectIt', True)]) for i in range(1, 5)])

    aggregatorConfig = VectorHash(
        rows=BaseDeviceRowSchema,
        displayedName="DAQ aggregators",
        defaultValue=[Hash([('deviceId', "SCS_DET_DSSC1M-1/DET/{}CH0".format(i)),
                            ('connectIt', True)]) for i in range(16)])

    processorConfig = VectorHash(
        rows=BaseDeviceRowSchema,
        displayedName="Processor devices",
        defaultValue=[Hash([('deviceId', "DET_LAB_DSSC1M-1/CAL/PROC_Q{}M{}".format(q, m)),
                            ('connectIt', True)]) for q in range(1, 5) for m in range(1, 5)])

    detectorConfig1 = VectorHash(
        rows=ConfigPptRowSchema,
        displayedName="DSSC Config",
        defaultValue=[])

    connectedPptDev = String(displayedName="Connected PPT",
                             defaultValue="",
                             accessMode=AccessMode.READONLY)

    status = String(displayedName="Status",
                    accessMode=AccessMode.READONLY)

    abortMeasurement = Bool(displayedName="Abort Measurement",
                            defaultValue=False,
                            accessMode=AccessMode.RECONFIGURABLE,
                            allowedStates={State.ACTIVE, State.ACQUIRING})

    ladderMode = UInt32(displayedName="Ladder Mode", defaultValue=1)

    activeModule = UInt32(displayedName="active Module", defaultValue=1)

    runConfigDir = String(defaultValue="RunConfigFiles")
    initConfigFile = VectorString()
    measurementOutDir = String()
    measurementConfigData = Hash()

    changeSweepParamOnly = Bool(displayedName="Change Sweep Param Only",
                                defaultValue=True)

    selPixels = String(displayedName="Sel Pixels",
                       defaultValue="all")

    saveConfigBurstAcquisition = Bool(displayedName="Store burst measurement Configuration in Rundirectory",
                                      defaultValue=True)

    singleRunMeasurement = Bool(displayedName="Sweep measurement in single Run",
                                defaultValue=True,
                                accessMode=AccessMode.RECONFIGURABLE,
                                allowedStates={State.ACTIVE, State.ON})

    xfelMode = Bool(displayedName="Run XFEL Mode",
                    defaultValue=True,
                    accessMode=AccessMode.RECONFIGURABLE)

    @UInt16(displayedName="Number of Iterations", defaultValue=20,
            allowedStates={State.ON})
    async def numIterations(self, value):
        self.numIterations = value
        await self.set_many_remote(self.ppt_dev, numBurstTrains=self.numIterations)

    processorsConnect = Bool(displayedName="Use processor devices",
                             defaultValue=True,
                             accessMode=AccessMode.READONLY)

    acquireHistograms = Bool(displayedName="Acquire Histograms",
                             defaultValue=False,
                             accessMode=AccessMode.RECONFIGURABLE,
                             allowedStates={State.ON})

    histoOutputDir = String(displayedName="Histogram output directory",
                            defaultValue="./")

    activeModule = UInt32(displayedName="Active Module",
                          defaultValue=1,
                          accessMode=AccessMode.RECONFIGURABLE)


    @UInt16(displayedName="Frames to send", defaultValue=400,
            allowedStates={State.ON})
    async def framesToSend(self, value):
        self.framesToSend = value
        await self.set_many_remote(self.ppt_dev, numFramesToSendOut=self.framesToSend)

    @UInt16(displayedName="Preburst Vetos", defaultValue=10,
            allowedStates={State.ON})
    async def numPreBurstVetos(self, value):
        self.numPreBurstVetos = value
        await self.set_many_remote(self.ppt_dev, numPreBurstVetos=self.numPreBurstVetos)

    measurementNumber = UInt32(displayedName="Current measurement number",
                               defaultValue=0,
                               accessMode=AccessMode.READONLY)

    totalInjSweepSections = UInt32(displayedName="Injection sweep sections",
                                   defaultValue=8,
                                   accessMode=AccessMode.READONLY)

    numParallelColumns = UInt32(displayedName="Number of parallel columns",
                                defaultValue=8,
                                accessMode=AccessMode.RECONFIGURABLE)

    sweepSecondPixelParameter = Bool(displayedName="En Second Parameter")

    firstPixelParameterName = String(displayedName="First PixelParam Name",
                                     defaultValue="RmpFineTrm",
                                     options=["RmpFineTrm",
                                              "RmpCurrDouble",
                                              "RmpEnFineDelayCntrl",
                                              "RmpDelayCntrl",
                                              "CSA_Resistor",
                                              "CSA_FbCap",
                                              "FCF_EnCap",
                                              "D0_EnMimCap",
                                              "D0_BypComprResistor"])

    firstPixelParameterRange = String(displayedName="First PixelParam Range")

    secondPixelParameterName = String(displayedName="Second PixelParam Name",
                                      defaultValue="RmpFineTrm",
                                      options=["RmpFineTrm",
                                               "RmpCurrDouble",
                                               "RmpEnFineDelayCntrl",
                                               "RmpDelayCntrl",
                                               "CSA_Resistor",
                                               "CSA_FbCap",
                                               "FCF_EnCap",
                                               "D0_EnMimCap",
                                               "D0_BypComprResistor"])
    secondPixelParameterRange = String(displayedName="Second PixelParam Range")

    injectionSettingRange = String(displayedName="Injection Setting Range")

    injectionMode = String(displayedName="injection Mode",
                           defaultValue="CHARGE_BUSINJ",
                           options=["CURRENT_BGDAC",
                                    "CURRENT_SUSDAC",
                                    "CHARGE_PXINJ_BGDAC",
                                    "CHARGE_PXINJ_SUSDAC",
                                    "CHARGE_BUSINJ",
                                    "ADC_INJ",
                                    "ADC_INJ_LR",
                                    "EXT_LATCH",
                                    "NORM"])

    injColumnSelectionMode = String(displayedName="injection column selection mode",
                                    defaultValue="SKIP",
                                    options=["BLOCK",
                                             "SKIP",
                                             "SKIPSPLIT"])

    burstParameterName = String(displayedName="BurstParameter Name",
                                defaultValue="start_wait_time",
                                options=["start_wait_time",
                                         "start_wait_offs",
                                         "gdps_on_time",
                                         "clr_on_time",
                                         "clr_cycle",
                                         "iprog_clr_duty",
                                         "iprog_clr_offset",
                                         "SW_PWR_ON",
                                         "fet_on_time"])
    burstParameterRange = String(displayedName="BurstParameter Range")

    timingScanRange = String(displayedName="Timing Scan Range")

    runController = String(displayedName="DAQ RunController",
                           description="Used for taking darks",
                           assignment=Assignment.MANDATORY)

    powerProcedure = String(displayedName="Power Procedure",
                            description="Used for soft interlock",
                            assignment=Assignment.MANDATORY)

    daqOutput = OutputChannel(ChannelNode,
                              displayedName="daqOutput",
                              description="Pipeline Output channel")

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        # DSSC control proxies
        self.ppt_devices = [None for i in range(NUM_QUADRANTS)]
        self.aggregators = [None for i in range(NUM_MODULES)]
        self.processors  = [None for i in range(NUM_MODULES)]
        self.ppt_dev = []  # short list of active ppt devices

        # DAQ proxies for darks taking
        self.run_controller = None
        self.aggr_dev = []
        self.proc_dev = []
        self.ppt_dev_QuadMap = {}  # index map of connected ppt devices
        self.proc_dev_indx = {}

        # Proxy for power procedure
        self.power_procedure = None

        # Reference to background tasks, used to cancel them
        self.task = None

        # Keeps track of state updates in state fusion
        self._last_state_update: str = None

        # Background tasks initialized in connectDevices
        self.state_fusion_task = None
        self.lock_watchdog_task = None

    async def onInitialization(self):
        self.status = "Ready"
        self.abortMeasurement = False
        self.selPixels = "all"
        await self.connectDevices()

    def setMeasurementData(self, measConfig, startDirectory, totalNumOfMeasurements,
                           numOfSweeps, settings1, name1, enSecondParam=False,
                           settings2=[1], name2=""):

        measstartDir = strToByteArray(startDirectory)
        self.measurementConfigData[
            "measurementInfo.measurementStartRunDirectory"] = measstartDir

        tmeasnum = np.uint32(totalNumOfMeasurements)
        self.measurementConfigData["measurementInfo.totalMeasurements"] =  tmeasnum

        setbase = "measurementInfo.measurementSettings."
        self.measurementConfigData[setbase + "setting1.settings"] = settings1
        setlen = np.uint32(len(settings1))
        self.measurementConfigData[setbase + "setting1.numSettings"] = setlen
        setname = strToByteArray(name1)
        self.measurementConfigData[setbase + "setting1.settingName"] = setname

        self.measurementConfigData[setbase + "setting2.settings"] = settings2
        setlen = np.uint32(len(settings2))
        self.measurementConfigData[setbase + "setting2.numSettings"] = setlen
        setname = strToByteArray(name2)
        self.measurementConfigData[setbase + "setting2.settingName"] = setname

        measName = str(name1)
        if measName != "BurstMeasurement":
            measName += " Sweep"

        minfbase = "measurementInfo."
        bmname = strToByteArray(measName)
        self.measurementConfigData[minfbase + "measurementName"] = bmname

        lmode = np.uint32(measConfig.ladderMode)
        self.measurementConfigData[minfbase + "ladderMode"] = lmode

        numSettings = 1
        numMeasurements = len(settings1)
        if enSecondParam:
            numMeasurements = len(settings1) * len(settings2)
            numSettings = 2

        nit = np.uint32(measConfig.numIterations)
        self.measurementConfigData[minfbase + "numIterations"] = nit


        nmeas = np.uint32(numMeasurements)
        self.measurementConfigData[minfbase + "numMeasurementsInSweep"] = nmeas

        nsweeps = np.uint32(numOfSweeps)
        self.measurementConfigData[minfbase + "numOfSweeps"] = nsweeps

        nveto = np.uint32(measConfig.numPreBurstVetos)
        self.measurementConfigData[minfbase + "numPreBurstVetos"] = nveto

        asics = int()
        for pptdevice in self.ppt_dev:
            offset = int((self.ppt_dev_QuadMap[pptdevice] - 1) * 16)
            sendingasics = int(pptdevice.sendingASICs, 2)
            sendingasics = sendingasics << offset
            asics += sendingasics
        self.measurementConfigData[minfbase + "availableASICs"] = np.uint64(asics)

        conPPT = strToByteArray(self.connectedPptDev)
        self.measurementConfigData[minfbase + "connectedPPTs"] = conPPT

        nset = np.uint32(numSettings)
        self.measurementConfigData[minfbase + "numMeasurementParams"] = nset

        timestamp = strToByteArray(str(datetime.datetime.utcnow()))
        self.measurementConfigData[minfbase + "timeStamp"] = timestamp

        if enSecondParam:
            cmbname = strToByteArray(str(name1) + "," + str(name2))
            self.measurementConfigData[minfbase + "measParamNames"] = cmbname
        else:
            sname = strToByteArray(str(name1))
            self.measurementConfigData[minfbase + "measParamNames"] = sname

        pixColSel = np.uint32(0)
        self.measurementConfigData[minfbase + "pixColumnSelect"] = pixColSel
        numParalCols = np.uint32(measConfig.numOfParallelColumns)
        self.measurementConfigData[minfbase + "numOfParallelColumns"] = numParalCols
        activeModNumpyUINT = np.uint32(measConfig.activeModule)
        self.measurementConfigData[minfbase + "activeModule"] = activeModNumpyUINT

        s1 = settings1[0]  # must be updated within sweep loops
        self.measurementConfigData[minfbase + "currentSetting1Val"] = s1
        s2 = settings2[0]  # must be updated within sweep loops
        self.measurementConfigData[minfbase + "currentSetting2Val"] = s2

        cm = np.uint32(0)  # must be updated within sweep loops
        self.measurementConfigData[minfbase + "currentMeasurementNumber"] = cm

        for i in range(1, 5):
            setattr(self.measurementConfigData, "measurementInfo.trainIdDataQ{}.startTrainId".format(i), np.uint64(0))
            setattr(self.measurementConfigData, "measurementInfo.trainIdDataQ{}.endTrainId".format(i), np.uint64(0))
            #should i initialize configFilePath?

        self.measurementConfigData[minfbase + "singleRunMeasurementType"] = True

    async def getPPTsStateString(self):
        states = []
        for ppt_device in self.ppt_dev:
            states.append([self.ppt_dev_QuadMap[ppt_device], ppt_device.state])
        return "States are: {}".format(["{} : {}".format(*s) for s in states])

    async def setBurstParameter(self, paramName, value):
        to_update = []
        await self.set_many_remote(self.ppt_dev,
                                 burstParameterName=paramName,
                                 burstParameterValue=value)

        await self.call_many_remote(self.ppt_dev, "setBurstParameter",
                                  reraise=True)

        for ppt_device in self.ppt_dev:
            if paramName == "start_wait_offs":
                to_update.append(ppt_device.updateStartWaitOffset())
            else:
                to_update.append(ppt_device.updateSequenceCounters())
        try:
            await gather(*to_update)
        except CancelledError:
            self.status = "Setting burst parameter cancelled"

    async def setInjectionValue(self, setting):
        await gather(*[waitUntil(lambda: dev.state != State.CHANGING) for dev in self.ppt_dev])
        await self.set_many_remote(self.ppt_dev, injectionValue=setting)
        await gather(*[waitUntil(lambda: dev.state != State.CHANGING) for dev in self.ppt_dev])
        await self.call_many_remote(self.ppt_dev, "setInjectionValue")

        self.status = await self.getPPTsStateString()

    async def setPixelRegister(self, module, pixelStr, signalName, value,
                               program):
        await self.set_many_remote(self.ppt_dev, selRegName="pixel",
                                 selModule=module,
                                 selPixels=pixelStr,
                                 selPixelSignal=signalName,
                                 selValue=value)
        to_set = []
        for ppt_device in self.ppt_dev:
            if program == True:
                to_set.append(ppt_device.progSelReg())
            else:
                to_set.append(ppt_device.preProgSelReg())
        try:
            await gather(*to_set)
        except CancelledError:
            self.status = "Setting pixel registers cancelled"

    async def waitAggregators(self, state):
        to_wait_for = []
        for agg_dev in self.aggr_dev:
            to_wait_for.append(waitUntil(lambda agg_dev=agg_dev:
                                         agg_dev.state == state))
        await gather(*to_wait_for)

    async def startAcquisition(self):
        # TODO: Refactor to use karaboDevices/daqController
        if self.run_controller.state != State.MONITORING:
            self.status = "XFEL DAQ not in monitoring state"
            if self.run_controller.state == State.ACQUIRING:
                await self.stopAcquisition()

        await self.waitAggregators(State.MONITORING)
        await self.run_controller.record()
        await waitUntil(lambda: self.run_controller.state == State.ACQUIRING)
        await self.waitAggregators(State.ACQUIRING)

    async def stopAcquisition(self):
        if self.run_controller.state != State.ACQUIRING:
            self.status = "XFEL DAQ not in acquiring state"
            return

        await self.waitAggregators(State.ACQUIRING)
        await self.run_controller.tune()
        await waitUntil(lambda: self.run_controller.state == State.MONITORING)
        await self.waitAggregators(State.MONITORING)

    async def acquireData(self):
        if not self.singleRunMeasurement:
            await self.startAcquisition()

        await self.call_many_remote(self.ppt_dev,
                                  "startBurstAcquisition",
                                  reraise=True)
        to_wait_for = []
        for ppt_device in self.ppt_dev:
            to_wait_for.append(waitUntil(lambda ppt_device=ppt_device:
                                         ppt_device.state == State.ACQUIRING or self.abortMeasurement))  # noqa
        await gather(*to_wait_for)

        to_wait_for = []
        for ppt_device in self.ppt_dev:
            to_wait_for.append(waitUntil(lambda ppt_device=ppt_device:
                                         ppt_device.state == State.ON or self.abortMeasurement))  # noqa

        await gather(*to_wait_for)

        trainIdPrecision = 2  # up to 2 train precise
        for pptdev in self.ppt_dev:
            quadnumber = self.ppt_dev_QuadMap[pptdev]
            startTrainId = pptdev.burstData.startTrainId
            while startTrainId == 0:
                self.log.WARN('Received startTrainId zero!')
                await sleep(0.2)
                startTrainId = pptdev.burstData.startTrainId
       	    endTrainId = pptdev.burstData.endTrainId
       	    while endTrainId == 0:
       	       	self.log.WARN('Received endTrainId zero!')
                await sleep(0.2)
       	       	endTrainId = pptdev.burstData.endTrainId

            self.measurementConfigData[
                "measurementInfo.trainIdDataQ{}.startTrainId".format(quadnumber)] = startTrainId
            self.measurementConfigData[
                "measurementInfo.trainIdDataQ{}.endTrainId".format(quadnumber)] = endTrainId

        await self.sendMeasurementInfoData()

        if not self.singleRunMeasurement:
            await self.stopAcquisition()

    async def update_pptdev_settings(self):
        await self.set_many_remote(self.ppt_dev,
                           numBurstTrains=self.numIterations,
                           numFramesToSendOut=self.framesToSend,
                           numPreBurstVetos=self.numPreBurstVetos)

    async def initMeasurement(self):
        self.status = "Init Measurement"
        self.abortMeasurement = False

        self.state = State.ACTIVE

        await self.stopDataSending()

        if self.processorsConnect and self.acquireHistograms:
            await self.set_many_remote(self.proc_dev,
                                     outputDir=self.histoOutputDir)
            await self.call_many_remote(self.proc_dev,
                                      "runHistogramAcquisition",
                                      reraise=True)

        self.pptInitConfigFile = []
        await self.runControl_PrepareAndMonitor()

    async def runControl_PrepareAndMonitor(self):
        if self.run_controller.state == State.MONITORING:
            return

        if self.run_controller.state == State.IGNORING:
            await self.run_controller.monitor()
            await waitUntil(
                lambda: self.run_controller.state == State.MONITORING)
            return

        if self.run_controller.state == State.ACQUIRING:
            await self.stopAcquisition()
            return

        msg = "DAQ is not ready!"
        raise RuntimeError(msg)

    async def finishMeasurement(self):
        if self.abortMeasurement:
            self.abortMeasurement = False
            self.status = "Measurement Aborted"
        else:
            self.status = "Measurement Finished"

        self.abortMeasurement = False

        if self.processorsConnect and self.acquireHistograms:
            to_stop = []
            for procdev in self.proc_dev:
                if procdev.Run:
                    to_stop.append(procdev.stop())

            await gather(*to_stop)
            await self.call_many_remote(self.proc_dev, "saveHistograms")

        self.log.INFO("Measurement finished")
        self.state = State.ON

    def updateStatus(self, enSecond, set1, name1, max1, set2, name2, max2):
        if enSecond:
            msg = "Measuring {} {} / {}".format(name2, set2, max2)
            self.status = msg
        else:
            msg = "Measuring {} {} / {}".format(name1, set1, max1)
            self.status = msg

    def checkBurstParameter(self):
        if not self.burstParameterRange:
            self.status = "Burst Parameter Range not set"
            self.state = State.ERROR
            return False

        if not self.burstParameterName:
            self.status = "Burst Parameter Name not set"
            self.state = State.ERROR
            return False

        return True

    def getMeasurementConfig(self):
        config = MeasurementConfig()
        config.configFileName = ""
        config.numIterations = int(self.numIterations)
        config.numOfParallelColumns = int(self.numParallelColumns)
        config.activeModule = int(self.activeModule)
        config.numPreBurstVetos = int(self.ppt_dev[0].numPreBurstVetos)
        for ppt_device in self.ppt_dev:
            if config.numPreBurstVetos != ppt_device.numPreBurstVetos:
                msg = "Preburst vetos are not the same in all PPTs"
                raise RuntimeError(msg)

        config.activeASIC = 0
        return config

    @Slot(displayedName="Update devices list",
          description="Connect to devices",
          allowedStates={State.UNKNOWN, State.ON})
    async def connectDevices(self):
        if self.state_fusion_task is not None:
            self.state_fusion_task.cancel()
            self.state_fusion_task = None

        if self.lock_watchdog_task is not None:
            self.lock_watchdog_task.cancel()
            self.lock_watchdog_task = None

        self.state = State.CHANGING
        self.status = "Connecting to devices..."

        allConnected = True
        to_connect = {}
        for i, row in enumerate(self.pptConfig.value):
            pptdevnamestr = row[0]
            if row[2]:
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
                            for i, r in enumerate(self.pptConfig.value) if r[2]]
        self.ppt_dev = list(done.values())


        for pptdev in self.ppt_dev:
            for i, row in enumerate(self.pptConfig.value):
                if pptdev._deviceId_ == row[0]:
                    self.ppt_dev_QuadMap[pptdev] = int(row[1][1])
                    break

        self.connectedPptDev = ", ".join(["Q{}".format(str(i + 1)) for i, d
                                          in enumerate(self.ppt_devices) if d])

        try:
            self.power_procedure = await wait_for(connectDevice(self.powerProcedure), timeout=3)
        except TimeoutError:
            if not self.expertMode:
                self.state = State.ERROR
                self.status = "Power Procedure not available"
                self.log.INFO("DOC personel: this device relies on MDL "
                              f"{self.powerProcedure}, which was not found. "
                              "Instantiate it or contact DET OCD.")
                return

        to_connect = {}
        for i, row in enumerate(self.aggregatorConfig.value):
            aggrnamestr = str(row[0])
            if row[1]:
                await sanitize_da(aggrnamestr)
                to_connect[str(i)] = connectDevice(aggrnamestr)

        if to_connect:
            done, pending, error = await allCompleted(**to_connect, timeout=10)
            error = ChainMap(pending, error)

        if len(error):
            failmsg = [r[0] for i, r in enumerate(self.aggregatorConfig.value)
                       if str(i) in error]
            self.status = "Failed connecting Aggregators: {}".format(failmsg)
            self.state = State.UNKNOWN
            return

        self.aggregators = [None if r[0] in error else done[str(i)]
                            for i, r in enumerate(self.aggregatorConfig.value) if r[1]]

        self.aggr_dev = list(done.values())

        self.run_controller = await connectDevice(self.runController)

        self.processorsConnect = np.any([row[1] for row in self.processorConfig.value])

        if self.processorsConnect:
            to_connect = {}
            for i, row in enumerate(self.processorConfig.value):
                procnamestr = row[0]
                if row[1]:
                    to_connect[str(i)] = connectDevice(procnamestr)

            done, pending, error = await allCompleted(**to_connect, timeout=10)
            error = ChainMap(pending, error)

            if len(error):
                failmsg = [r[0] for i, r in enumerate(self.processorConfig.value)
                           if r[0] in error]
                self.status = "Failed connecting Processors: {}".format(failmsg)
                self.state = State.UNKNOWN
                return

            self.processors = [None if r[0] in error else done[str(i)]
                               for i, r in enumerate(self.processorConfig.value) if r[1]]
            self.proc_dev = list(done.values())

        self.state_fusion_task = background(self.state_fusion())
        self.lock_watchdog_task = background(self.lock_watchdog())
        self.status = "Done connecting to devices"

    @Slot(displayedName="Start PPT devices",
          description="Connect and Initialize PPT devices",
          allowedStates={State.UNKNOWN, State.OFF, State.ON})
    async def startPPTdevices(self):
        ### TODO: Merge this slot and initPPTdevices into a single slot
        self.task = background(self._startPPTdevices)

    async def _startPPTdevices(self):
        ### TODO: this block should not be here because it messes the state machine
        ### and it's expected that proxies to PPT devices should already exists
        self.state = State.INIT
        self.status = "connecting PPT devices..."
        await self.connectDevices()
        ### TODO: end

        self.status = "Starting PPT devices..."
        self.log.INFO(self.status)

        to_open = []
        for ppt_device in self.ppt_dev:
            if (ppt_device.state == State.UNKNOWN):
                to_open.append(ppt_device.open())

        if len(to_open):
            await gather(*to_open)
            await waitUntil(lambda: all(dev.state != State.UNKNOWN  # Could be OFF or ERROR
                                        for dev in self.ppt_dev), timeout=10)

        self.log.INFO("Opened PPT")

        to_init = []
        for ppt_device in self.ppt_dev:
            await setWait(ppt_device, xfelMode=self.xfelMode)
            await setWait(ppt_device, numFramesToSendOut=self.framesToSend)
            to_init.append(ppt_device.initSystem())
        await gather(*to_init)
        self.log.INFO("Init PPT")

        await self._initPPTdevices()
        self.status = "PPT devices started"

    @Slot(displayedName="Init PPT devices",
          description="Initialize PPT devices",
          allowedStates={State.ON, State.OFF})
    async def initPPTdevices(self):
        ### TODO: Merge this slot and startPPTdevices into a single slot
        self.task = background(self._initPPTdevices())
        await self.update_pptdev_settings()

    async def _initPPTdevices(self):
        self.state = State.INIT
        try:
            await self.stopDataSending()
            self.status = "Initializing PPT devices..."
            to_init = []
            for ppt_device in self.ppt_dev:
                if ppt_device.state in [State.OFF, State.ON, State.STOPPED]:
                    to_init.append(ppt_device.initSystem())
            await gather(*to_init)
            self.log.INFO("Init PPT")
            self.status = "PPT devices initialized"
        except:
            self.log.ERROR(f"Exception caught: {sys.exc_info()[0]}")
        self.state = State.ON

    @Slot(displayedName="Check ASICs/Reset on PPT devices",
          description="Check ASICs/Reset on all PPT devices",
          allowedStates={State.ACTIVE, State.ON})
    async def checkASICsReset(self):
        currentState = self.state
        self.state = State.CHANGING
        self.status = "Check ASICs/Reset on all PPTs ..."
        to_checkReset = []
        for ppt_device in self.ppt_dev:
            to_checkReset.append(ppt_device.checkASICReset())
        await gather(*to_checkReset)
        self.log.INFO("Check ASICs/Reset")
        self.state = currentState
        self.status = "ASICs are reset on all PPTs"

    @Slot(displayedName="Start Data sending",
          description="Stop continuous mode to reinitialize PPT",
          allowedStates={State.ON})
    async def startDataSending(self):
        self.state = State.CHANGING
        self.status = "Starting data sending..."
        to_run = []
        for ppt_device in self.ppt_dev:
            if (ppt_device.state == State.ON):
                if self.xfelMode:
                    to_run.append(ppt_device.runXFEL())
                else:
                    to_run.append(ppt_device.runStandAlone())

        gather(*to_run)

        self.log.INFO("Run PPT")

        to_start = []
        for ppt_device in self.ppt_dev:
            if (ppt_device.state == State.STARTED):
                to_start.append(ppt_device.startAcquisition())

        await gather(*to_start)
        await waitUntil(lambda: all(dev.state == State.ACQUIRING
                                    for dev in self.ppt_dev))

        self.state = State.ACTIVE
        self.status = "PPTs send data..."

    @Slot(displayedName="Send Dummy Data",
          allowedStates={State.ON, State.OFF})
    async def startAllChannelsDummyData(self):
        self.state = State.CHANGING
        self.status = "Starting data sending..."
        to_run = []
        for ppt_device in self.ppt_dev:
            if (ppt_device.state == State.ON):
                    to_run.append(ppt_device.startAllChannelsDummyData())

        gather(*to_run)

        self.log.INFO("Run PPT")

        to_start = []
        for ppt_device in self.ppt_dev:
            if (ppt_device.state == State.STARTED):
                to_start.append(ppt_device.startAcquisition())

        await gather(*to_start)
        await waitUntil(lambda: all(dev.state == State.ACQUIRING
                                    for dev in self.ppt_dev))

        self.state = State.ACTIVE
        self.status = "PPTs send dummy data..."



    @Slot(displayedName="Stop Data sending",
          description="Stop continuous mode to reinitialize PPT",
          allowedStates={State.ACQUIRING})
    async def stopDataSending(self):
        self.status = "Stopping data sending..."
        to_stop = []

        # TODO: These two loops should in theory be a single one.
        for device in self.ppt_dev:
            if device.state == State.ACQUIRING:
                to_stop.append(device.stopAcquisition())

        await gather(*to_stop)

        to_stop = []
        for device in self.ppt_dev:
            if device.state == State.STARTED:
                to_stop.append(device.stopStandalone())

        await gather(*to_stop)

        self.log.INFO("Stop PPT acquisition")
        self.status = "PPTs stopped"

    @Slot(displayedName="ABORT",
          description="ABORT current measurement",
          allowedStates={State.ACQUIRING, State.ACTIVE})
    async def abortMeasurementSlot(self):
        self.abortMeasurement = True
        await self.stopAcquisition()
        await self.stopDataSending()
        if self.task:
            self.task.cancel()
        self.state = State.ON

    @Slot(displayedName="Run PixelParameterSweep",
          description="Start Pixel parameter sweep of one or two pixel parameters",
          allowedStates={State.ON})
    async def runPixelParameterSweep(self):
        self.task = background(self._runPixelParameterSweep())

    async def _runPixelParameterSweep(self):
        self.status = "Run Pixel Parameter Sweep"

        await self.initMeasurement()

        enSecond = self.sweepSecondPixelParameter

        name2 = "none"
        settings2 = [1]
        if enSecond:
            ok, settings2 = get_sweep_vector(self.secondPixelParameterRange)
            if not ok or not self.secondPixelParameterName:
                msg = "Enabled second parameter, but range or parameter name not specified correctly"
                self.status = msg
                self.log.ERROR(msg)
                self.state = State.ERROR
                return

            name2 = self.secondPixelParameterName

            msg = f"Measure {name2}, Settings {settings2}"
            self.status = msg

        ok, settings1 = get_sweep_vector(self.firstPixelParameterRange)

        if not ok or not self.firstPixelParameterName:
            msg = "First pixel parameters range or parameter name not specified correctly"
            self.status = msg
            self.log.ERROR(msg)
            self.state = State.ERROR
            return

        name1 = self.firstPixelParameterName

        startRunDir = self.getNextRunPathDir()

        await self.pptDevicesStoreConfiguration(startRunDir)

        measurementsInSweep = len(settings2) * len(settings1)
        totalRuns = measurementsInSweep

        measurementConfig = self.getMeasurementConfig()
        self.setMeasurementData(measurementConfig, startRunDir, totalRuns, 0,
                                settings1, name1, enSecond, settings2, name2)

        msg = "Measure {}, Settings {}".format(name1, settings1)
        self.status = msg
        self.status = "Running Measurement ..."
        minfbase = "measurementInfo."

        self.measurementNumber = np.uint32(0)

        if self.singleRunMeasurement:
            await self.startAcquisition()
            self.measurementConfigData[minfbase + "singleRunMeasurementType"] = True
        else:
            self.measurementConfigData[minfbase + "singleRunMeasurementType"] = False

        for set2 in settings2:
            if enSecond:
                for module in range(1,5):
                    await self.setPixelRegister(module,
                                                self.selPixels,
                                                name2,
                                                set2,
                                                False)
                self.measurementConfigData[
                    minfbase + "currentSetting2Val"] = set2  # noqa

            for set1 in settings1:
                self.updateStatus(enSecond, set1, name1, settings1[-1], set2,
                                  name2, settings2[-1])
                for module in range(1,5):
                    await self.setPixelRegister(module,
                                                self.selPixels,
                                                name1,
                                                set1,
                                                True)

                self.measurementConfigData[
                    minfbase + "currentSetting1Val"] = set1  # noqa

                mnum = np.uint32(self.measurementNumber)
                self.measurementConfigData[
                    minfbase + "currentMeasurementNumber"] = mnum  # noqa
                self.measurementNumber = self.measurementNumber + np.uint32(1)

                mtime = strToByteArray(str(datetime.datetime.utcnow()))
                self.measurementConfigData[minfbase + "timeStamp"] = mtime

                await self.acquireData()

                if self.abortMeasurement:  # TODO: this task gets cancelled on abort
                    break
            if self.abortMeasurement:  # TODO: this task gets cancelled on abort
                break

        if self.singleRunMeasurement:
            await self.stopAcquisition()

        await self.finishMeasurement()

    @Slot(displayedName="Run Injection Sweep",
          description=("Start Injection sweep if second a pixel parameter " +
                       "can be swept above"),
          allowedStates={State.ON})
    async def runInjectionSweep(self):
        self.task = background(self._runInjectionSweep())

    async def _runInjectionSweep(self):
        self.status = "Run Injection Sweep"

        await self.initMeasurement()

        enSecond = self.sweepSecondPixelParameter

        ok, settings1 = get_sweep_vector(self.injectionSettingRange)
        if not ok:
            msg = "Injection parameter range not specified correctly"
            self.status = msg
            self.log.ERROR(msg)
            self.state = State.ERROR
            return

        name1 = self.injectionMode
        await self.set_many_remote(self.ppt_dev,
                                 injectionMode=self.injectionMode)

        await self.set_many_remote(self.ppt_dev,
                                 colSelectMode=self.injColumnSelectionMode,
                                 numParallelColumns=self.numParallelColumns)
        to_set = []
        for ppt_device in self.ppt_dev:
            to_set.append(ppt_device.setInjectionMode())

        await gather(*to_set)
        self.log.INFO("Set PPT injection mode")

        name2 = "none"
        settings2 = [1]

        if enSecond:
            ok, settings2 = get_sweep_vector(self.secondPixelParameterRange)
            if not ok or not self.secondPixelParameterName:
                msg = "Second parameter settings enabled, but range or parameter name not specified correctly"
                self.status = msg
                self.log.ERROR(msg)
                self.state = State.ERROR
                return

            name2 = self.secondPixelParameterName

        self.totalInjSweepSections = int(64 / self.numParallelColumns)
        injSections = list(range(self.totalInjSweepSections))

        measurementsInSweep = len(settings2) * len(settings1)
        totalRuns = len(injSections) * measurementsInSweep

        self.measurementNumber = np.uint32(0)

        minfbase = "measurementInfo."

        startRunDir = self.getNextRunPathDir()
        measurementConfig = self.getMeasurementConfig()
        self.setMeasurementData(measurementConfig, startRunDir, totalRuns, self.totalInjSweepSections,
                                settings1, name1, enSecond, settings2,
                                name2)

        injtype = strToByteArray(self.injectionMode)
        self.measurementConfigData[minfbase + "injType"] = injtype

        injSelType = strToByteArray(self.injColumnSelectionMode)
        self.measurementConfigData[minfbase + "injSkipType"] = injSelType

        if self.singleRunMeasurement:
            await self.startAcquisition()
            self.measurementConfigData[minfbase + "singleRunMeasurementType"] = True
        else:
            self.measurementConfigData[minfbase + "singleRunMeasurementType"] = False        

        self.log.INFO(f"injSections {injSections}")

        for injSection in injSections:
            self.log.INFO("Measure Injection Section Q" + str(injSection))

            to_select = []
            to_set_col = []
            for ppt_device in self.ppt_dev:
                to_select.append(setWait(ppt_device, pixelsColSelect = injSection))
                to_set_col.append(ppt_device.setCurrentColSkipOn())
            try:
                await gather(*to_select)
                await gather(*to_set_col)
            except CancelledError:
                self.status = "Select and Set column skip on cancelled"
                return
            self.log.INFO("Set PPT column skip on")

            await self.pptDevicesStoreConfiguration(startRunDir)

            self.status = "Running Measurement ..."

            colSelect = np.uint32(injSection)
            self.measurementConfigData[minfbase + "pixColumnSelect"] = colSelect

            colskipmodeStr = None
            if self.injColumnSelectionMode == "SKIP":
                colskipmodeStr = "colskip0-63:{}#{}".format(self.totalInjSweepSections, injSection)
            elif self.injColumnSelectionMode == "SKIPSPLIT":
                colskipmodeStr = "colskipsplit0-63:{}#{}".format(self.totalInjSweepSections, injSection)

            self.measurementConfigData[minfbase + "columnSelect"] = strToByteArray(colskipmodeStr)

            for set2 in settings2:
                if enSecond:
                    await self.setPixelRegister(self.activeModule,
                                                self.selPixels,
                                                name2,
                                                set2,
                                                True)
                    self.measurementConfigData[
                        minfbase + "currentSetting2Val"] = set2  # noqa

                for set1 in settings1:
                    self.updateStatus(enSecond, set1, name1, settings1[-1],
                                      set2, name2, settings2[-1])
                    await self.setInjectionValue(set1)


                    self.measurementConfigData[
                        minfbase + "currentSetting1Val"] = set1  # noqa
                    cmn = np.uint32(self.measurementNumber)
                    self.measurementConfigData[
                        minfbase + "currentMeasurementNumber"] = cmn
                    self.measurementNumber = self.measurementNumber + np.uint32(1)

                    tnow = strToByteArray(str(datetime.datetime.utcnow()))
                    self.measurementConfigData[minfbase + "timeStamp"] = tnow

                    await self.acquireData()

                    if self.abortMeasurement:  # TODO: remove
                        break

                if self.abortMeasurement:  # TODO: remove
                    break

            if self.abortMeasurement:  # TODO: remove
                break

        if self.singleRunMeasurement:
            await self.stopAcquisition()

        await self.finishMeasurement()


    @Slot(displayedName="Run BurstParameter Sweep",
          description="Start Burst Parameter Sweep",
          allowedStates={State.ON})
    async def runBurstParameterSweep(self):
        self.task = background(self._runBurstParameterSweep())

    async def _runBurstParameterSweep(self):
        self.status = "Run Burst Parameter Sweep"

        await self.runControl_PrepareAndMonitor()
        await self.initMeasurement()

        enSecond = False

        minfbase = "measurementInfo."

        name2 = "none"
        set2 = "none"
        settings2 = [1]

        if not self.checkBurstParameter():
            return

        ok, settings1 = get_sweep_vector(self.burstParameterRange)
        if not ok or not self.burstParameterName:
            msg = "Burst sweep parameter range but not specified correctly"
            self.status = msg
            self.log.ERROR(msg)
            return

        name1 = self.burstParameterName

        totalRuns = len(settings1)


        startRunDir = self.getNextRunPathDir()
        measurementConfig = self.getMeasurementConfig()
        self.setMeasurementData(measurementConfig, startRunDir, totalRuns, 0,
                                settings1, name1, enSecond, settings2,
                                name2)

        self.measurementNumber = np.uint32(0)

        await self.pptDevicesStoreConfiguration(startRunDir)

        self.status = "Running Measurement ..."

        if self.singleRunMeasurement:
            await self.startAcquisition()
            self.measurementConfigData[minfbase + "singleRunMeasurementType"] = True
        else:
            self.measurementConfigData[minfbase + "singleRunMeasurementType"] = False

        for set1 in settings1:
            minfbase = "measurementInfo."
            self.updateStatus(enSecond, set1, name1, settings1[-1], set2,
                              name2, settings2[-1])

            await self.setBurstParameter(name1, set1)
            if name1 == "start_wait_offs":
                await self.call_many_remote(self.ppt_dev, 'updateStartWaitOffset')
            self.measurementConfigData[minfbase + "currentSetting1Val"] = set1

            mnum = np.uint32(self.measurementNumber)
            self.measurementConfigData[
                minfbase + "currentMeasurementNumber"] = mnum  # noqa
            self.measurementNumber = self.measurementNumber + np.uint32(1)

            tnow = strToByteArray(str(datetime.datetime.utcnow()))
            self.measurementConfigData[minfbase + "timeStamp"] = tnow

            await self.acquireData()

            if self.abortMeasurement:
                break

        if self.singleRunMeasurement:
            await self.stopAcquisition()

        await self.finishMeasurement()

    @Slot(displayedName="Run Burst Acquisition",
          description="Start Burst Acquisition of selected number of trains",
          allowedStates={State.ON, State.ACQUIRING})  # TODO: confirm State.ACQUIRING
    async def acquireBursts(self):
        self.task = background(self._acquireBursts())

    async def _acquireBursts(self):

        await self.initMeasurement()

        name1 = "BurstMeasurement"
        settings1 = [1]
        name2 = "none"
        settings2 = [1]

        self.status = "Running {}...".format(name1)

        startRunDir = self.getNextRunPathDir();

        self.measurementNumber = np.uint32(0)
        if self.saveConfigBurstAcquisition:
            await self.pptDevicesStoreConfiguration(startRunDir)
        self.setMeasurementData(self.getMeasurementConfig(),
                                startRunDir, 1, 0,
                                settings1, name1, False, settings2, name2)

        self.updateStatus(False, 'none', name1, settings1[-1], 'none', name2,
                          settings2[-1])

        minfbase = "measurementInfo."
        self.measurementConfigData[minfbase + "currentSetting1Val"] = 0
        self.measurementConfigData[minfbase + "currentSetting2Val"] = 0

        self.measurementConfigData[
            minfbase + "currentMeasurementNumber"] = np.uint32(0)  # noqa

        tnow = strToByteArray(str(datetime.datetime.utcnow()))
        self.measurementConfigData[minfbase + "timeStamp"] = tnow

        self.measurementConfigData[minfbase + "columnSelect"] = strToByteArray("all")

        if self.singleRunMeasurement:
            await self.startAcquisition()

        self.measurementConfigData[minfbase + "singleRunMeasurementType"] = True

        await self.acquireData()

        if self.singleRunMeasurement:
            await self.stopAcquisition()

        await self.finishMeasurement()

    def getNextRunPathDir(self):
        res = 'p' + str(
            self.run_controller.proposalNumber) + "/r{:04d}/".format(
            np.uint32(self.run_controller.runNumber) + 1)
        res = ''.join(res.split())
        return res

    async def pptDevicesStoreConfiguration(self, runPathDir):
        minfbase = "measurementInfo."
        if self.singleRunMeasurement:
            runDir = self.runConfigDir if self.runConfigDir.endswith(
                '/') else self.runConfigDir + '/'
            for pptdevice in self.ppt_dev:
                rpath = runDir + runPathDir + "ConfigFilesMeas{}/Q{}/run_config.conf".format(int(self.measurementNumber), self.ppt_dev_QuadMap[pptdevice])
                await setWait(pptdevice, saveConfigFileToName=rpath)
                barr_path = strToByteArray(rpath)
                self.measurementConfigData[minfbase + "configFilePath.savedConfigPath_Q{}".format(self.ppt_dev_QuadMap[pptdevice])] = barr_path
            await self.call_many_remote(self.ppt_dev, 'storeFullConfigUnder')
        else:
            runDir = self.runConfigDir if self.runConfigDir.endswith(
                '/') else self.runConfigDir + '/'
            for pptdevice in self.ppt_dev:
                rpath = runDir + runPathDir + "Q{}".format(self.ppt_dev_QuadMap[pptdevice]) + "/run_config.conf"
                await setWait(pptdevice, saveConfigFileToName=rpath)
                barr_path = strToByteArray(rpath)
                self.measurementConfigData[minfbase + "configFilePath.savedConfigPath_Q{}".format(self.ppt_dev_QuadMap[pptdevice])] = barr_path
            await self.call_many_remote(self.ppt_dev, 'storeFullConfigUnder')

    @Slot(displayedName="Send measurementInfo Data")
    async def sendMeasurementInfoData(self):
        await self.daqOutput.writeRawData(self.measurementConfigData)

    # SOFT POWER PROCEDURE INTERLOCK
    # As part of soft interlocks, the power procedure is locked, such that it 
    # can only be operated from here. Its slots are replicated here with added
    # states, such that they cannot be called while data is being sent.

    # This disables the devices locks, which may lead to hardware damage when
    # using DEPFET sensors.
    expertMode = Bool(displayedName="Expert Mode - High Risk of Damage",
                      description="This is a DET EXPERT parameter. Safety "
                                  "features of this device will be disabled. "
                                  "This can lead to substantial hardware damage!"
                                  " Do not do this unless you are instructed so "
                                  "by a member of the DET group!",
                      defaultValue=False,
                      accessMode=AccessMode.INITONLY)

    # Power procedure states to possible slots.
    # Needed as the slots cannot be greyed out in the GUI, so we instead
    # check if they can be called here and display a message if not.
    state_to_active_slots = {
        State.PASSIVE: "Switch ASICs on",
        State.ACTIVE: "Switch ASICs off or Switch HV on",
        State.STARTED: "Switch HV off or Switch PLC on",
        State.ENGAGED: "Switch PLC off or Switch Source on",
        State.ON: "Switch Source off"
    }

    @Slot(displayedName="1. ASICs On",
          allowedStates=POWER_PROCEDURE_ALLOWED_STATES)
    async def switchAsicsOn(self):
        # Power Procedure goes from State.PASSIVE to State.ACTIVE
        if self.power_procedure.state != State.PASSIVE:
            self.status = ('Cannot do "1. ASICs On",  but can '
                           f'{self.state_to_active_slots[self.power_procedure.state]}')
            return
        background(self.power_procedure.switchAsicsOn)

    @Slot(displayedName="2. HV On",
          allowedStates=POWER_PROCEDURE_ALLOWED_STATES)
    async def switchHvOn(self):
        # Power Procedure goes from State.ACTIVE to State.STARTED
        if self.power_procedure.state != State.ACTIVE:
            self.status = ('Cannot do "2. HV On",  but can '
                           f'{self.state_to_active_slots[self.power_procedure.state]}')
            return
        background(self.power_procedure.switchHvOn)

    @Slot(displayedName="3. PLC On",
          allowedStates=POWER_PROCEDURE_ALLOWED_STATES)
    async def switchPlcOn(self):
        # Power Procedure goes from State.STARTED to State.ENGAGED
        if self.power_procedure.state != State.STARTED:
            self.status = ('Cannot do "3. PLC On",  but can '
                           f'{self.state_to_active_slots[self.power_procedure.state]}')
            return
        background(self.power_procedure.switchPlcOn)

    @Slot(displayedName="4. SOURCE On",
          allowedStates=POWER_PROCEDURE_ALLOWED_STATES)
    async def switchSourceOn(self):
        # Power Procedure goes from State.ENGAGED to State.ON
        if self.power_procedure.state !=State.ENGAGED:
            self.status = ('Cannot do "4. SOURCE On", but can '
                           f'{self.state_to_active_slots[self.power_procedure.state]}')
            return
        background(self.power_procedure.switchSourceOn)

    @Slot(displayedName="All Off",
           allowedStates=POWER_PROCEDURE_ALLOWED_STATES)
    async def allOff(self):
        """Ensure that no data is being sent while powering down."""
        self.status = "Stopping acquisition before power down"
        devices = [ppt for ppt in self.ppt_dev if ppt.state == State.ACQUIRING]
        try:
            await wait_for(self.call_many_remote(devices, "stopAcquisition"),
                           timeout=3)
        except TimeoutError:
            self.status = "Not all PPTs stopped sending data"
            self.log.INFO("Not all PPTs stopped sending data within 3 seconds. "
                          "Something's up. Check and try again.")
            return
        await self.power_procedure.switchAllOff()

    @Slot(displayedName="SOURCE Off",
          allowedStates=POWER_PROCEDURE_ALLOWED_STATES)
    async def switchSourceOff(self):
        # Power Procedure goes from State.ON to State.ENGAGED
        if self.power_procedure.state != State.ON:
            self.status = ('Cannot do "SOURCE Off",  but can '
                           f'{self.state_to_active_slots[self.power_procedure.state]}')
            return
        background(self.power_procedure.switchHvOff)

    @Slot(displayedName="PLC Off",
          allowedStates=POWER_PROCEDURE_ALLOWED_STATES)
    async def switchPlcOff(self):
        # Power Procedure goes from State.ENGAGED to State.STARTED
        if self.power_procedure.state != State.ENGAGED:
            self.status = ('Cannot do "Source Off",  but can '
                           f'{self.state_to_active_slots[self.power_procedure.state]}')
            return
        background(self.power_procedure.switchSourceOff)

    @Slot(displayedName="HV Off",
          allowedStates=POWER_PROCEDURE_ALLOWED_STATES)
    async def switchHVOff(self):
        # Power Procedure goes from State.STARTED to State.ACTIVE
        if self.power_procedure.state != State.STARTED:
            self.status = ('Cannot do "HV Off",  but can '
                           f'{self.state_to_active_slots[self.power_procedure.state]}')
            return
        background(self.power_procedure.switchHvOff)

    @Slot(displayedName="ASICS Off",
          allowedStates=POWER_PROCEDURE_ALLOWED_STATES)
    async def switchAsicsOff(self):
        # Power Procedure goes from State.ACTIVE to State.PASSIVE
        if self.power_procedure.state != State.ACTIVE:
            self.status = ('Cannot do "ASICs Off",  but can '
                           f'{self.state_to_active_slots[self.power_procedure.state]}')
            return
        background(self.power_procedure.switchAsicsOff)

    lastLockOverwrite = String(
        displayedName="Last Lock Overwrite",
        description="Tracking of locking; for troubleshooting",
        accessMode=AccessMode.READONLY
    )

    async def set_many_remote(self, devices: List['proxy'], **kwargs):
        coros = [setWait(dev, **kwargs) for dev in devices]
        try:
            await gather(*coros)
        except CancelledError:
            self.status = f"Cancelled setting on PPT: {kwargs}"

    async def call_many_remote(self, devices: List['proxy'], slot: str, reraise: bool = False):
        coros = [getattr(dev, slot)() for dev in devices]

        try:
            await gather(*coros)
        except CancelledError as e:
            self.status = f"Cancelled calling {slot}"
            if reraise:
                raise e

    async def onException(self, slot, exception, traceback):
        self.state = State.ERROR
        self.status = f"{slot}: {exception}"

    async def state_fusion(self):
        """Fuse the states of PPTs and Power Procedure into allowed states."""

        failures_left = 10

        # The order of the `ifs` are important and are in order of increasing
        # priority. That is, it's more important to have the state of the power
        # procedure than the PPTs. `elif` is not used to forcibly check any
        # state.
        while failures_left >= 1:
            try:
                state = self.state
                source = ''
                power_proc_state = self.power_procedure.state

                states = [ppt.state for ppt in self.ppt_dev]
                # Wait for all PPTs to have the same state, typically seen
                # when updating settings
                if not all(state == states[0] for state in states):
                    wait_time = min(len(self.ppt_dev) * 0.1, 1)
                    states = [ppt.state for ppt in self.ppt_dev]
                    if not all(state == states[0] for state in states):
                        state = State.ERROR
                        source = "PPTs have different states"

                if states[0] in (State.UNKNOWN, State.ON):  # powered off or on
                    state = states[0]
                    source = "PPTs"
                if states[0] in (State.OPENING, State.CLOSING):
                    state = State.CHANGING
                    source = "PPTs"
                if any(state == State.CHANGING for state in states):
                    state = State.CHANGING
                    source = "PPTs"
                if power_proc_state == State.CHANGING:
                    state = State.CHANGING
                    source = self.power_procedure.deviceId
                if power_proc_state == State.PASSIVE:
                    state = State.OFF
                    source = self.power_procedure.deviceId
                if power_proc_state == State.ERROR:
                    state = State.ERROR
                    source = self.power_procedure.deviceId
                if states[0] in (State.ACQUIRING, State.STARTED):
                    state = State.ACQUIRING  # Acquiring trumps all
                    source = "PPTs"

                source = f"{state} from {source}"
                if state != self.state or source != self._last_state_status:
                    self.state = State(state.value)
                    self.status = source
                    self._last_state_status = source

                states.append(power_proc_state)
                await waitUntilNew(*states)
            except CancelledError:
                self.log.DEBUG("State fusion cancelled")
                return  # eg. the task gets cancelled
            except Exception as e:
                # Handle issues such as PPTs out of sync
                self.log.WARN(f"Exception in state fusion:\n {e}")
                failures_left -= 1

            failures_left = 10  # Reset the counter on success

        else:
            self.status = str(e)
            self.state = State.ERROR

    async def lock_watchdog(self):
        """Check that devices are locked by us, and acquire them if possible."""
        if self.expertMode:
            self.log.INFO("EXPERT MODE: NOT LOCKING DEVICES")
            self.status = "EXPERT MODE: NOT LOCKING DEVICES"
            return

        failures_left = 10

        while failures_left >= 10:
            try:
                locks = {ppt: ppt.lockedBy for ppt in self.ppt_dev}
                locks[self.power_procedure] = self.power_procedure.lockedBy

                broken = [px for px, locked_by in locks.items() if locked_by != self.deviceId]
                if broken:
                    msg = f"OVERRULING LOCK ON: {[px.deviceId for px in broken]}"
                    self.lastLockOverwrite = msg
                    self.log.INFO(msg)
                    await gather(*(lock(px) for px in broken))

                await waitUntilNew(*locks.values())
            except CancelledError:
                self.log.DEBUG("Lock watchdog cancelled")
                return  # eg. the task gets cancelled
            except Exception as e:
                # Handle issues such as failure to obtain locks
                self.log.WARN(f"Exception in lock watchdog:\n {e}")
                failures_left -= 1

            failures_left = 10  # Reset the counter on success

        else:
            self.status = str(e)
            self.state = State.ERROR

    async def onDestruction(self):
        if not self.expertMode:
            if self.lock_watchdog_task is not None:  # In expert mode
                self.lock_watchdog_task.cancel()

            coros = [ppt.slotClearLock() for ppt in self.ppt_dev]

            if self.power_procedure is not None:  # Failed to create proxy at init
                coros.append(self.power_procedure.slotClearLock())

            try:
                await gather(*coros)
            except KaraboError as e:  # A proxy went down before this device
                self.log.WARN(str(e))
