# Test the DsscControl device.
import pytest

from asyncio import (
    create_task,
    Event,
    get_running_loop,
    sleep,
)
from DsscControl.dssc_control import DsscControl
from karabo.middlelayer import Hash, State
from karabo.middlelayer_api import device_client
from karabo.middlelayer_api.eventloop import EventLoop

CONFIG = {
    "takeData.daqController": "mandy",
    "powerProcedure": "harold",
}


@pytest.fixture
def event_loop():
    loop = EventLoop()
    yield loop


class MockProxy:
    runNumber = 42
    runController = "Athens"

    async def start(self):
        print("starting run")

    async def stop(self):
        print("stopping run")


class MockGetDevice:
    def __init__(self, *args, **kwargs):
        pass

    async def __aenter__(self):
        return MockProxy()

    async def __aexit__(self, *args):
        pass


@pytest.mark.asyncio
async def test_can_instantiate():
    config = Hash(CONFIG)  # Make a copy of the config as items get poped.
    device = DsscControl(config)

    await device.startInstance()


@pytest.mark.asyncio
async def test_all_mixins_provide_scene():
    mixins = [kls.__name__ for kls in DsscControl.__bases__[:-1]]

    config = Hash(CONFIG)
    device = DsscControl(config)
    await device.startInstance()
    await device.onInitialization()

    assert device.availableScenes.value == mixins

    for mixin in mixins:
        res = device.requestScene({"name": mixin})
    assert res["payload"]["name"] == mixin


@pytest.mark.asyncio
@pytest.mark.parametrize(
    "is_depfet, qty, unit, expected_time, ditl",
    [
        (False, 40, "Trains",  "Will take 4.0 seconds. With run.", True),
        (False, 0, "Seconds", "Will take continuously until operator stops. With run.", True),
    ]
)
async def test_take_data_detector_acquiring(is_depfet, qty, unit, expected_time, ditl):
    device_client._getDevice = MockGetDevice

    config = Hash(CONFIG)
    config["takeData.daqInTheLoop"] = ditl
    config["takeData.quantity"] = qty
    config["takeData.unit"] = unit
    device = DsscControl(config)

    await device.startInstance()
    await device.onInitialization()

    device.ppt_dev = []

    # Force using our loop
    device.takeData._event = Event(loop=get_running_loop())

    assert device.status == "Ready"

    device.state = State.ACQUIRING
    task = create_task(device.takeData.takeData())

    await sleep(0.1)

    assert device.status == expected_time

    if qty == 0:
        device.takeData._event.set()

    await sleep(4)

    assert device.status == "Acquisition done."
    assert task.done()
    assert not device.takeData._event.is_set()


@pytest.mark.asyncio
@pytest.mark.parametrize(
    "is_depfet, qty, unit, expected_time, ditl",
    [
        (False, 40, "Trains",  "Will take 4.0 seconds. With run.", True),
        (False, 0, "Seconds", "Will take continuously until operator stops. With run.", True),
    ]
)
async def test_take_data_detector_standby(is_depfet, qty, unit, expected_time, ditl):
    device_client._getDevice = MockGetDevice

    config = Hash(CONFIG)
    config["isDEPFET"] = is_depfet
    config["takeData.daqInTheLoop"] = ditl
    config["takeData.quantity"] = qty
    config["takeData.unit"] = unit
    device = DsscControl(config)

    await device.startInstance()
    await device.onInitialization()

    device.ppt_dev = []

    # Force using our loop
    device.takeData._event = Event(loop=get_running_loop())

    assert device.status == "Ready"

    create_task(device.takeData._take_data())
    await sleep(0.1)
    assert device.status == "Making detector acquire."

    device.state = State.ACQUIRING
    get_running_loop().something_changed()
    await sleep(0.1)
    assert device.status == expected_time

    await sleep(5)  # XXX: do not provide a total run time greater than 4 secs.
    if qty == 0:
        device.takeData._event.set()
    device.state = State.ON
    get_running_loop().something_changed()

    await sleep(0.1)
    assert device.status == "Acquisition done."
    assert not device.takeData._event.is_set()
