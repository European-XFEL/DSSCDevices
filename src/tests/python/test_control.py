import uuid

import pytest
from karabo.middlelayer import (
    connectDevice,
    Device,
    isSet,
    Overwrite,
    sleep,
    Slot,
    State,
    String,
    UInt32,
    waitUntil,
)
from karabo.middlelayer.testing import (
    AsyncDeviceContext,
    create_device_server,
    event_loop,
)


from DsscControl.dssc_control import DsscControl


def create_instanceId():
    return f"test-mdl-{uuid.uuid4()}"


class MockDevice(Device):
    fullConfigFileName = String(description="Mocking PPT property")

    # This overwrites the PPT states to ON, to enable setting the
    # frames to send on the dssc control in a test as the control
    # inherits states from PPTs and can only set frames when ON
    # or OFF, not the default's UNKNOWN.
    state = Overwrite(defaultValue=State.ON)

    def __init__(self, configuration):
        self.init_done = False
        super().__init__(configuration)

    @Slot(description="Mocking PPT property")
    async def initSystem(self):
        self.init_done = True

    @UInt32(description="Mocking PPT property",
            defaultValue=666)
    async def numFramesToSendOut(self, value):
        self.numFramesToSendOut = value
        print(f"{self.deviceId} frames is now {self.numFramesToSendOut.value}")


@pytest.mark.asyncio
@pytest.mark.timeout(30)
async def test_show_ppts_in_use():
    q1_did = create_instanceId()
    q1_device = MockDevice(
        {"deviceId": q1_did, "fullConfigFileName": "/path/to/conf.conf"}
    )
    q2_did = create_instanceId()
    q2_device = MockDevice(
        {"deviceId": q2_did, "fullConfigFileName": "/path/to/conf.conf"}
    )
    q3_did = create_instanceId()
    q3_device = MockDevice(
        {"deviceId": q3_did, "fullConfigFileName": "/path/to/conf.conf"}
    )
    q4_did = create_instanceId()
    q4_device = MockDevice(
        {"deviceId": q4_did, "fullConfigFileName": "/path/to/conf.conf"}
    )

    daq_controller_id = create_instanceId()
    daq_controller = MockDevice(
        {"deviceId": daq_controller_id}
    )

    power_proc_id = create_instanceId()
    power_proc = MockDevice(
        {"deviceId": power_proc_id}
    )

    controller_id = create_instanceId()
    controller = DsscControl(
        {
            "deviceId": controller_id,
            "pptConfig": [
                {"deviceId": q1_did, "quadrantId": "Q1", "connectIt": True},
                {"deviceId": q2_did, "quadrantId": "Q2", "connectIt": False},
                {"deviceId": q3_did, "quadrantId": "Q3", "connectIt": True},
                {"deviceId": q4_did, "quadrantId": "Q4", "connectIt": True},
            ],
            "runController": daq_controller_id,
            "powerProcedure": power_proc_id,
            
        }
    )

    instances = {
        controller_id: controller,
        daq_controller_id: daq_controller,
        power_proc_id: power_proc,
        q1_did: q1_device,
        q2_did: q2_device,
        q3_did: q3_device,
        q4_did: q4_device,
    }
    async with AsyncDeviceContext(**instances) as ctx:
        proxy = await connectDevice(controller_id)
        # Test initialization was correct
        assert proxy.connectedPptDev == "Q1, Q3, Q4"


@pytest.mark.asyncio
@pytest.mark.timeout(30)
async def test_frames_monitoring():
    q1_did = create_instanceId()
    q1_device = MockDevice(
        {"deviceId": q1_did, "fullConfigFileName": "/path/to/conf.conf"}
    )
    q2_did = create_instanceId()
    q2_device = MockDevice(
        {"deviceId": q2_did, "fullConfigFileName": "/path/to/conf.conf"}
    )
    q3_did = create_instanceId()
    q3_device = MockDevice(
        {"deviceId": q3_did, "fullConfigFileName": "/path/to/conf.conf"}
    )
    q4_did = create_instanceId()
    q4_device = MockDevice(
        {"deviceId": q4_did, "fullConfigFileName": "/path/to/conf.conf"}
    )

    daq_controller_id = create_instanceId()
    daq_controller = MockDevice(
        {"deviceId": daq_controller_id}
    )

    power_proc_id = create_instanceId()
    power_proc = MockDevice(
        {"deviceId": power_proc_id}
    )

    controller_id = create_instanceId()
    controller = DsscControl(
        {
            "deviceId": controller_id,
            "pptConfig": [
                {"deviceId": q1_did, "quadrantId": "Q1", "connectIt": True},
                {"deviceId": q2_did, "quadrantId": "Q2", "connectIt": True},
                {"deviceId": q3_did, "quadrantId": "Q3", "connectIt": True},
                {"deviceId": q4_did, "quadrantId": "Q4", "connectIt": True},
            ],
            "runController": daq_controller_id,
            "powerProcedure": power_proc_id,
            "expertMode": True,
        }
    )

    instances = {
        controller_id: controller,
        daq_controller_id: daq_controller,
        power_proc_id: power_proc,
        q1_did: q1_device,
        q2_did: q2_device,
        q3_did: q3_device,
        q4_did: q4_device,
    }
    async with AsyncDeviceContext(**instances) as ctx:
        ctrl = await connectDevice(controller_id)
        assert ctrl.connectedPptDev == "Q1, Q2, Q3, Q4"

        assert q1_device.numFramesToSendOut == 666
        assert q2_device.numFramesToSendOut == 666
        assert q3_device.numFramesToSendOut == 666
        assert q4_device.numFramesToSendOut == 666

        ctrl.framesToSend = 400
        await sleep(1)
        assert q1_device.numFramesToSendOut == 400
        assert q2_device.numFramesToSendOut == 400
        assert q3_device.numFramesToSendOut == 400
        assert q4_device.numFramesToSendOut == 400

        q1_device.numFramesToSendOut = 5
        await sleep(1)
        assert ctrl.framesToSend == 5
        assert q2_device.numFramesToSendOut == 5
        assert q3_device.numFramesToSendOut == 5
        assert q4_device.numFramesToSendOut == 5


@pytest.mark.asyncio
@pytest.mark.timeout(30)
async def test_server_loads_device():
    serverId = create_instanceId()
    server = create_device_server(serverId, [DsscControl])
    async with AsyncDeviceContext(server=server) as ctx:
        server_instance = ctx.instances["server"]
        assert "DsscControl" in server_instance.plugins
