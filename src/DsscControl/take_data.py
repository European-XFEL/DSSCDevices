# This mixin provides a node for taking burst of data, with the DAQ optionally in the loop
#
# The detector should start sending data BEFORE changing the DAQ state, ideally waiting
# an extra ~5-10 seconds when using DEPFET (temperature stabilization).
#
# If the detector is already acquiring, do NOT stop acquisition before or after the run.
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
)

from .scenes.take_data_scene import take_data_scene


class TakeDataNode(Configurable):

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
        background(self._take_data)

    async def _take_data(self):
        # Calculate run time, in seconds
        run_time = self.quantity.value

        if self.unit == "Trains":
            run_time = run_time / 10
        if self.unit == "Minutes":
            run_time *= 60
        # It's already seconds otherwise.

        msg = f"Will take {run_time} seconds."
        if self.daqInTheLoop:
            msg += " With run."
        self.status = msg

        # Get detector acquiring
        was_acquiring = self.parent.state == State.ACQUIRING
        if not was_acquiring:
            self.status = "Make detector acquire."
            coros = [ppt.runXFEL() for ppt in self.parent.ppt_dev]
            await gather(*coros)
            await waitUntil(lambda: self.parent.state == State.ACQUIRING)
            if self.parent.isDEPFET:
                self.status = "Waiting for DEPFET to warm up."
                await sleep(6)  # Let DEPFET sensor warm up.

        if self.daqInTheLoop:
            async with getDevice(self.daqController) as controller:
                controller.start()
                run_control_id = controller.runController
            async with getDevice(run_control_id) as run_control:
                self.status = f"Started run #{int(run_control.runNumber)}."

        await sleep(run_time)

        if self.daqInTheLoop:
            async with getDevice(self.daqController) as controller:
                controller.stop()

        if not was_acquiring:
            coros = [ppt.stopAcquisition() for ppt in self.parent.ppt_dev]
            await gather(*coros)
            while self.parent.state == State.ACQUIRING:
                continue

        self.status = "Acquisition done."


class TakeData(Configurable):
    takeData = Node(
        TakeDataNode,
        displayedName="Data Taking",
    )

    def scene(device_id: str):
        return take_data_scene(device_id)
