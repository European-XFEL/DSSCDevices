# Test the DsscControl device.
import pytest

from DsscControl.dssc_control import DsscControl
from karabo.middlelayer import Hash
from karabo.middlelayer_api.eventloop import EventLoop

CONFIG = {
    "takeData.daqController": "mandy",
    "powerProcedure": "harold",
}


@pytest.fixture
def event_loop():
    loop = EventLoop()
    yield loop


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
