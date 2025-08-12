import uuid

import pytest
from karabo.middlelayer import (
    Device,
    Slot,
    String,
    connectDevice,
    isSet,
    sleep,
    State,
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
    # PPT
    fullConfigFileName = String()

    def __init__(self, configuration):
        self.init_done = False
        super().__init__(configuration)

    @Slot()
    async def initSystem(self):
        """PPT"""
        self.init_done = True


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
            "availableGainConfigurations": [
                {
                    "description": "default",
                    "q1": "/path/to/conf.conf",
                    "q2": "/path/to/conf.conf",
                    "q3": "/path/to/conf.conf",
                    "q4": "/path/to/conf.conf",
                },
                {
                    "description": "bing",
                    "q1": "/path/to/q1/config.conf",
                    "q2": "/path/to/q2/config.conf",
                    "q3": "/path/to/q3/config.conf",
                    "q4": "/path/to/q4/config.conf",
                },
            ],
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
        q1_did: q1_device,
        q2_did: q2_device,
        q3_did: q3_device,
        q4_did: q4_device,
    }
    async with AsyncDeviceContext(**instances) as ctx:
        proxy = await connectDevice(controller_id)
        proxy.connectDevices()
        # Test initialization was correct
        assert proxy.connectedPptDev == "Q1, Q3, Q4"


@pytest.mark.asyncio
@pytest.mark.timeout(30)
async def test_server_loads_device():
    serverId = create_instanceId()
    server = create_device_server(serverId, [DsscControl])
    async with AsyncDeviceContext(server=server) as ctx:
        server_instance = ctx.instances["server"]
        assert "DsscControl" in server_instance.plugins
