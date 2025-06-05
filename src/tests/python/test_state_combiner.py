#############################################################################
# Author: Cyril Danilevski, forked from Parenti
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from contextlib import contextmanager

from karabo.middlelayer import Device, State, sleep
from karabo.middlelayer.testing import DeviceTest, async_tst

from DsscControl.state_combiner import DsscStateCombiner


class TestDsscStateCombiner(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.chiller = Device({'_deviceId_': 'chiller'})
        cls.chiller.classId = 'BeckhoffChiller'
        cls.sib_switch = Device({'_deviceId_': 'sib_switch'})
        cls.sib_switch.classId = 'BeckhoffDigitalInput'
        cls.mpod = Device({'_deviceId_': 'mpod'})
        conf = {
            "classId": "StateCombiner",
            "_deviceId_": "TestStateCombiner",
            "deviceIds": [" chiller", "sib_switch ", "mpod"]
        }
        cls.dev = DsscStateCombiner(conf)
        with cls.deviceManager(cls.chiller, cls.sib_switch, cls.mpod,
                               lead=cls.dev):
            yield

    @async_tst
    async def test_dssc_combiner(self):
        self.assertEqual(self.dev.state, State.UNKNOWN)

        # Base case: some other device goes to error
        self.chiller.state = State.ON
        self.sib_switch.state = State.ON
        self.mpod.state = State.ERROR

        await sleep(0.1)  # allow combiner to evaluate new state
        self.assertEqual(self.dev.state, State.ERROR)

        # Base case: the chiller goes to error during normal operation
        self.chiller.state = State.ERROR
        self.sib_switch.state = State.ON
        self.mpod.state = State.ON

        await sleep(0.1)  # allow combiner to evaluate new state
        self.assertEqual(self.dev.state, State.ERROR)

        # Base case, the safety switch goes to OFF
        self.chiller.state = State.ACTIVE
        self.sib_switch.state = State.OFF
        self.mpod.state = State.ON

        await sleep(0.1)  # allow combiner to evaluate new state
        self.assertEqual(self.dev.state, State.OFF)

        # Chiller corner case: chiller in error and safety switch off
        self.chiller.state = State.ERROR
        self.sib_switch.state = State.OFF
        self.mpod.state = State.ON

        await sleep(0.1)  # allow combiner to evaluate new state
        self.assertEqual(self.dev.state, State.PASSIVE)
