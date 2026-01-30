"""
Author: kirchgessner
Creation date: April 1, 2017, 01:41 PM
Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
"""
from asyncio import (
    CancelledError,
    gather,
    TimeoutError,
    wait_for,
)
from collections import ChainMap
from typing import List

from karabo.middlelayer import (
    AccessMode,
    Assignment,
    Bool,
    Device,
    Hash,
    KaraboError,
    Overwrite,
    Slot,
    State,
    String,
    UInt16,
    VectorHash,
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
from .schemata import PptRowSchema


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

    state = Overwrite(
        defaultValue=State.UNKNOWN,
        displayedName="State",
        options=DEVICE_STATES)

    pptConfig = VectorHash(
        rows=PptRowSchema,
        displayedName="Ppt devices to connect",
        defaultValue=[Hash([('deviceId', "SCS_CDIDET_DSSC/FPGA/PPT_Q{}".format(i)),
                            ('quadrantId', "Q{}".format(i)), ('connectIt', True)]) for i in range(1, 5)])

    connectedPptDev = String(displayedName="Connected PPT",
                             defaultValue="",
                             accessMode=AccessMode.READONLY)

    @UInt16(displayedName="Quantity of Trains", defaultValue=20,
            allowedStates={State.ON})
    async def numBurstTrains(self, value):
        self.numBurstTrains = value
        await self.set_many_remote(self.ppt_dev, numBurstTrains=value)

    @UInt16(
        displayedName="Frames to send",
        defaultValue=400,
        maxInc=800,
        allowedStates={State.ON, State.OFF})  # State.OFF allowed as when sending dummy data
    async def framesToSend(self, value):
        self.framesToSend = value
        await self.set_many_remote(self.ppt_dev, numFramesToSendOut=self.framesToSend)
        self.status = f"Set {self.framesToSend.value} frames"

    @UInt16(displayedName="Preburst Vetos", defaultValue=10,
            allowedStates={State.ON})
    async def numPreBurstVetos(self, value):
        self.numPreBurstVetos = value
        await self.set_many_remote(self.ppt_dev, numPreBurstVetos=self.numPreBurstVetos)

    runController = String(displayedName="DAQ RunController",
                           description="Used for taking darks",
                           assignment=Assignment.MANDATORY)

    powerProcedure = String(displayedName="Power Procedure",
                            description="Used for soft interlock",
                            assignment=Assignment.MANDATORY)

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        # DSSC control proxies
        self.ppt_devices = [None for i in range(NUM_QUADRANTS)]
        self.ppt_dev = []  # short list of active ppt devices

        # DAQ proxies for darks taking
        self.run_controller = None
        self.ppt_dev_QuadMap = {}  # index map of connected ppt devices
        self.proc_dev_indx = {}

        # Proxy for power procedure
        self.power_procedure = None

        # Reference to background tasks, used to cancel them
        self.task = None

        # Keeps track of state updates in state fusion
        self._last_status_update: str = None

        # Background tasks initialized in connectDevices
        self.state_fusion_task = None
        self.frame_monitoring_task = None

    async def onInitialization(self):
        await self.connectDevices()

    async def startAcquisition(self):
        # TODO: Refactor to use karaboDevices/daqController
        if self.run_controller.state != State.MONITORING:
            self.status = "XFEL DAQ not in monitoring state"
            if self.run_controller.state == State.ACQUIRING:
                await self.stopAcquisition()

        await self.run_controller.record()
        await waitUntil(lambda: self.run_controller.globalState.daqGlobalState == State.ACQUIRING)

    async def stopAcquisition(self):
        # TODO: Refactor to use karaboDevices/daqController
        if self.run_controller.globalState.daqGlobalState != State.ACQUIRING:
            self.status = "XFEL DAQ not in acquiring state"
            return

        await self.run_controller.tune()
        await waitUntil(lambda: self.run_controller.globalState == State.MONITORING)

    @Slot(displayedName="Update devices list",
          description="Connect to devices",
          allowedStates={State.UNKNOWN, State.ON})
    async def connectDevices(self):
        if self.state_fusion_task is not None:
            self.state_fusion_task.cancel()
            self.state_fusion_task = None

        if self.frame_monitoring_task is not None:
            self.frame_monitoring_task.cancel()
            self.frame_monitoring_task = None

        self.state = State.CHANGING
        self.status = "Connecting to devices..."

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

        connected_ppt_devices = [] 
        for pptdev in self.ppt_dev:
            for row in self.pptConfig.value:
                if pptdev.deviceId == row[0]:
                    self.ppt_dev_QuadMap[pptdev] = int(row[1][1])
                    connected_ppt_devices.append(row[1])
                    break

        self.connectedPptDev = ", ".join(sorted(connected_ppt_devices))

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

        self.run_controller = await connectDevice(self.runController)

        self.state_fusion_task = background(self.state_fusion())
        self.frame_monitoring_task = background(self.monitor_frames())
        self.status = "Done connecting to devices"

    @Slot(displayedName="Init PPT devices",
          description="Initialize PPT devices",
          allowedStates={State.ON, State.OFF})
    async def initPPTdevices(self):
        self.task = background(self._initPPTdevices())

    async def _initPPTdevices(self):
        try:
            await self.stopDataSending()
            self.status = "Initializing PPT devices..."
            to_init = []
            for ppt_device in self.ppt_dev:
                if ppt_device.state in [State.OFF, State.ON, State.STOPPED]:
                    to_init.append(ppt_device.initSystem())
            await gather(*to_init)
            self.log.INFO("Init PPT")
            await self.set_many_remote(self.ppt_dev,
                               numBurstTrains=self.numIterations,
                               numFramesToSendOut=self.framesToSend)
            self.status = "PPT devices initialized"
        except Exception as e:
            self.log.ERROR(f"Exception caught: {e}")

    @Slot(displayedName="Check ASICs/Reset on PPT devices",
          description="Check ASICs/Reset on all PPT devices",
          allowedStates={State.ACTIVE, State.ON})
    async def checkASICsReset(self):
        self.status = "Check ASICs/Reset on all PPTs ..."
        to_checkReset = []
        for ppt_device in self.ppt_dev:
            to_checkReset.append(ppt_device.checkASICReset())
        await gather(*to_checkReset)
        self.log.INFO("Check ASICs/Reset")
        self.status = "ASICs are reset on all PPTs"

    @Slot(displayedName="Start Data sending",
          description="Stop continuous mode to reinitialize PPT",
          allowedStates={State.ON})
    async def startDataSending(self):
        self.status = "Starting data sending..."
        await gather(*(ppt.runXFEL() for ppt in self.ppt_dev))
        await waitUntil(lambda: all(dev.state == State.ACQUIRING
                                    for dev in self.ppt_dev))
        self.status = "PPTs send data..."

    @Slot(displayedName="Send Dummy Data",
          allowedStates={State.ON, State.OFF})
    async def startAllChannelsDummyData(self):
        self.status = "Starting data sending..."
        await gather(*(ppt.startAllChannelsDummyData() for ppt in self.ppt_dev))
        self.status = "PPTs send dummy data..."

    @Slot(displayedName="Stop Data sending",
          allowedStates={State.ACQUIRING})
    async def stopDataSending(self):
        self.status = "Stopping data sending..."

        to_stop = []
        for device in self.ppt_dev:
            if device.state == State.ACQUIRING:
                to_stop.append(device.stopAcquisition())
            elif device.state == State.STARTED:
                to_stop.append(device.stopStandalone())
        await gather(*to_stop)

        self.log.INFO("Stop PPT acquisition")
        self.status = "PPTs stopped"

    @Slot(displayedName="Run Burst Acquisition",
          description="Start Burst Acquisition of selected number of trains",
          allowedStates={State.ON})
    async def acquireBursts(self):
        await gather(*(ppt.startBurstAcquisition() for ppt in self.ppt_dev))

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

    async def monitor_frames(self):
        """Monitor frames accross PPTs.

        While frames are expected to be set here, it happens that they're set
        manually on a PPT device.
        This background task monitors frames and update all if any changes.

        The timestamp is checked to ensure that changes coming from remote
        devices are newer than ours, and not due to us changing the value here.
        (This allows smooth update of frames to send from remote devices and
        from here.)
        """
        while True:
            frames = {ppt.deviceId: ppt.numFramesToSendOut
                      for ppt in self.ppt_dev}

            for did, frames_to_send in frames.items():
                if (frames_to_send != self.framesToSend
                    and frames_to_send.timestamp > self.framesToSend.timestamp):
                    self.framesToSend = frames_to_send
                    await self.set_many_remote(
                        self.ppt_dev,
                        numFramesToSendOut=self.framesToSend
                    )
                    self.status = f"Set {self.framesToSend.value} frames from {did}"
                    break

            await waitUntilNew(*frames.values())

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

                if self.power_procedure is not None:
                    power_proc_state = self.power_procedure.state
                else:  # In expert mode
                    power_proc_state = None

                states = [ppt.state for ppt in self.ppt_dev]
                # Wait for all PPTs to have the same state, typically seen
                # when updating settings
                if not all(state == states[0] for state in states):
                    await sleep(1)
                    states = [ppt.state for ppt in self.ppt_dev]
                    if not all(state == states[0] for state in states):
                        state = State.ERROR
                        source = "PPTs have different states"
                else:
                    if states[0] in (State.UNKNOWN, State.OFF, State.ON):  # powered off or on
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

                status = f"{state.value} from {source}"
                if state != self.state or status != self._last_status_update:
                    self.state = State(state.value)
                    self.status = status
                    self._last_status_update = status

                if power_proc_state is not None:
                    states.append(power_proc_state)

                await wait_for(waitUntilNew(*states), timeout=1)
            except TimeoutError:
                pass  # Timeout due to forcing state update monitoring just above
            except CancelledError:
                self.log.DEBUG("State fusion cancelled")
                return  # eg. the task gets cancelled
            except Exception as e:
                # Handle issues such as PPTs out of sync
                self.log.WARN(f"Exception in state fusion:\n {e}")
                failures_left -= 1

            failures_left = 10  # Reset the counter on success

            # Update locks on remote devices
            if self.state in {State.ERROR, State.ACQUIRING, State.CHANGING}:
                # Set the appropriate locks depending on the source of changes.
                if self.power_procedure.deviceId == source:
                    coros = (lock(ppt) for ppt in self.ppt_dev
                             if not ppt.lockedBy)
                    await gather(*coros)
                if "PPTs" in source and not self.power_procedure.lockedBy:
                    await lock(self.power_procedure)

            if self.state in {State.OFF, State.ON}:
                # Idling, free all locks that may exist.
                coros = []
                for px in [*self.ppt_dev, self.power_procedure]:
                    if px.lockedBy == self.deviceId:
                        coros.append(px.slotClearLock())
                await gather(*coros)

        else:  # Loop exited after 10 consecutive failures
            self.status = "State Fusion failed 10 times in a row!"
            self.state = State.ERROR

    async def onDestruction(self):
        # Clear all locks on remote devices if locked by us.
        coros = []
        for px in [*self.ppt_dev, self.power_procedure]:
            if px is not None and px.lockedBy == self.deviceId:
                coros.append(px.slotClearLock())
        try:
            await gather(*coros)
        except KaraboError as e:  # A proxy went down before this device
            self.log.WARN(str(e))
