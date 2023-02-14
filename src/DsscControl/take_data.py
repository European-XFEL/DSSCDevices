# This mixin provides a node for taking burst of data, with the DAQ optionally in the loop
#
# The detector should start sending data BEFORE changing the DAQ state, ideally waiting
# an extra ~5-10 seconds when using DEPFET (temperature stabilization).
#
# If the detector is already acquiring, do NOT stop acquisition before or after the run.
from asyncio import (
    CancelledError,
    create_task,
    Event,
    gather,
)
from typing import Union

from karabo.middlelayer import (
    Node,
    Slot,
    Configurable,
    getDevice,
    String,
    AccessMode,
    UInt32,
    Bool,
    State,
    Assignment,
    sleep,
    background,
    waitUntil,
)

from .scenes.take_data_scene import take_data_scene


class TakeDataNode(Configurable):

    # This event is used to cancel data taking, whether from a slot
    # or within the device, while respecting the detector state.
    _event = Event()

    async def _sleep(self, sleep_time: Union[int, float]):
        # If self._event is set elsewhere, this coro needs
        # to be cancelled to avoid resetting the event.
        if sleep_time == 0:
            return
        try:
            await sleep(sleep_time)
        except CancelledError:
            pass
        else:
            if not self._event.is_set():
                self._event.set()

    @property
    def status(self):
        return self.parent.status

    @status.setter
    def status(self, msg: str):
        self.parent.status = msg
        self.parent.doNotCompressEvents = not self.parent.doNotCompressEvents

    daqController = String(
        displayedName="DAQ Controller Id",
        accessMode=AccessMode.INITONLY,
        assignment=Assignment.MANDATORY,
    )

    daqInTheLoop = Bool(
        displayedName="Take runs",
        description="Take runs while taking data",
        defaultValue=True,
        accessMode=AccessMode.RECONFIGURABLE,
        allowedStates={State.ON, State.ACQUIRING},
    )
    
    quantity = UInt32(
        displayedName="Quantity",
        description="The quantity of trains/secs/mins to take. 0 means continuous",
        defaultValue=20,
        accessMode=AccessMode.RECONFIGURABLE,
        allowedStates={State.ON, State.ACQUIRING},
    )
    
    unit = String(
        displayedName="Unit",
        options=["Trains", "Minutes", "Seconds"],
        defaultValue="Trains",
        accessMode=AccessMode.RECONFIGURABLE,
        allowedStates={State.ON, State.ACQUIRING},
    )
    
    @Slot(
        displayedName="Take Data",
        description="Take data with the specified conditions",
        allowedStates={State.ON, State.ACQUIRING},
    )
    async def takeData(self):
        if self._event.is_set():
            self.status = "Acquision already ongoing. Cancel first."
            return  # Some acquisition already in progress
        background(self._take_data)

    async def _take_data(self):
        # Start acquisition
        was_acquiring = self.parent.state == State.ACQUIRING
        if not was_acquiring:
            self.status = "Making detector acquire."
            coros = [ppt.runXFEL() for ppt in self.parent.ppt_dev]
            await gather(*coros)

            await waitUntil(lambda: self.parent.state == State.ACQUIRING)

            # if self.parent.isDEPFET:
            #     self.status = "Waiting for DEPFET to warm up."
            #     await sleep(6)  # Let DEPFET sensor warm up.

        if self.daqInTheLoop:
            async with getDevice(self.daqController) as controller:
                await controller.start()
                run_control_id = controller.runController
            async with getDevice(run_control_id) as run_control:
                self.status = f"Started run #{int(run_control.runNumber)}."

        # Calculate run time in seconds
        run_time = self.quantity.value
        if run_time == 0:
            msg = f"Will take continuously until operator stops."
        else:
            if self.unit == "Trains":
                run_time = run_time / 10
            if self.unit == "Minutes":
                run_time *= 60
            # It's already seconds otherwise.
            msg = f"Will take {run_time} seconds."

        if self.daqInTheLoop:
            msg += " With run."
        self.status = msg

        # Take data for the given condition
        sleep = background(self._sleep(run_time))
        await self._event.wait()
        sleep.cancel()  # Should the acquisition be cancelled.

        # Stop acquisition
        if self.daqInTheLoop:
            async with getDevice(self.daqController) as controller:
                await controller.stop()

        if not was_acquiring:
            coros = [ppt.stopAcquisition() for ppt in self.parent.ppt_dev]
            await gather(*coros)
            await waitUntil(lambda: self.parent.state != State.ACQUIRING)

        self.status = "Acquisition done."
        self._event.clear()

    @Slot(
        displayedName="Cancel",
        description="Cancel the data taking",
        allowedStates=[State.ACQUIRING],
    )
    async def cancel(self):
        self._event.set()
        self.status = "Acquisition cancelled."


class TakeData(Configurable):
    takeData = Node(
        TakeDataNode,
        displayedName="Data Taking",
    )

    def scene(device_id: str):
        return take_data_scene(device_id)
