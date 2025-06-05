#############################################################################
# Author: Cyril Danilevski, forked from Andrea Parenti
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from asyncio import gather

from karabo.middlelayer import (AccessMode, Device, State, StateSignifier,
                                VectorString, background, connectDevice,
                                waitUntilNew)

from ._version import version as dver


class DsscStateCombiner(Device):
    """Combine the states of various devices, with a twist to handle chiller.

    This device leverages the StateSignifier, but adds a twise for a corner
    case of the DSSC chiller being off.

    When powered off, the BeckhoffChiller device goes to error, as it fails to
    communicate with the hardware, which is the correct thing to do during
    normal operation.
    There's no way to know that the chiller is off, only that communication
    fails, resulting in a number of alarms being triggered.

    However, we can later know, in this device, whether it's fine or not, as
    the detector's interlock has an input of type BeckhoffDigitalInput, also
    monitored here.
    If that input, SIB_1_SIB_ENABLE, is off, it's fine for the chiller to be in
    error.

    Thus, if the most significant state is ERROR, check if it comes from the
    chiller is disabled (the SIB_ENABLE switch is OFF).
    """
    __version__ = dver

    deviceIds = VectorString(
        displayedName="Devices to Monitor",
        accessMode=AccessMode.INITONLY
    )

    def __init__(self, configuration):
        super(DsscStateCombiner, self).__init__(configuration)

        self.devices = []
        self.trump_state = StateSignifier()

    async def onInitialization(self):
        """ This method will be called when the device starts.

            Define your actions to be executed after instantiation.
        """
        self.devices = await gather(
            *(connectDevice(id.strip()) for id in self.deviceIds))
        self.status = "Connected to all remote devices"

        background(self.monitor_states())

    async def monitor_states(self):
        while True:
            all_states = {dev.classId: dev.state for dev in self.devices}
            state = self.trump_state.returnMostSignificant(all_states.values())

            if state == State.ERROR:
                # Hey, could we be off, when chiller in error and sib is off?
                if (all_states['BeckhoffChiller'] == State.ERROR and
                        all_states['BeckhoffDigitalInput'] == State.OFF):
                    state = State.PASSIVE
            if state != self.state:
                # Use current timestamp for the combined state, otherwise on
                # initialization it would have a timestamp in the past.
                self.state = State(state.value)
            await waitUntilNew(*all_states.values())
