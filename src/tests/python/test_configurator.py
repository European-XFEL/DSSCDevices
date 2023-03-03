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


from DsscControl.configurator import DsscConfigurator


def create_instanceId():
    return f"test-mdl-{uuid.uuid4()}"


class MockPPT(Device):
    fullConfigFileName = String()

    def __init__(self, configuration):
        self.init_done = False
        super().__init__(configuration)

    @Slot()
    async def initSystem(self):
        self.init_done = True


@pytest.mark.asyncio
@pytest.mark.timeout(30)
async def test_apply_configuration():
    q1_did = create_instanceId()
    q1_device = MockPPT(
        {"_deviceId_": q1_did, "fullConfigFileName": "/path/to/conf.conf"}
    )
    q2_did = create_instanceId()
    q2_device = MockPPT(
        {"_deviceId_": q2_did, "fullConfigFileName": "/path/to/conf.conf"}
    )
    q3_did = create_instanceId()
    q3_device = MockPPT(
        {"_deviceId_": q3_did, "fullConfigFileName": "/path/to/conf.conf"}
    )
    q4_did = create_instanceId()
    q4_device = MockPPT(
        {"_deviceId_": q4_did, "fullConfigFileName": "/path/to/conf.conf"}
    )

    configurator_id = create_instanceId()
    configurator = DsscConfigurator(
        {
            "_deviceId_": configurator_id,
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
            "pptDevices": [
                {"deviceId": q1_did, "quadrantId": "Q1", "use": True},
                {"deviceId": q2_did, "quadrantId": "Q2", "use": False},
                {"deviceId": q3_did, "quadrantId": "Q3", "use": True},
                {"deviceId": q4_did, "quadrantId": "Q4", "use": True},
            ],
        }
    )

    instances = {
        configurator_id: configurator,
        q1_did: q1_device,
        q2_did: q2_device,
        q3_did: q3_device,
        q4_did: q4_device,
    }
    async with AsyncDeviceContext(**instances) as ctx:
        proxy = await connectDevice(configurator_id)
        # Check initialization was correct
        assert proxy.monitoredDevices == "Q1, Q3, Q4"

        # Check matched configurations have human name reported
        assert proxy.actualGainConfiguration == "default"

        # Check mismatched configurations are reported
        q1_device.fullConfigFileName = "bleh"
        await sleep(1)
        assert "Different Settings Applied" in proxy.actualGainConfiguration

        # Check target configurations are validated
        with pytest.raises(ValueError):
            proxy.targetGainConfiguration = "something illegal"

        # Check target configuration applied across selected devices
        proxy.targetGainConfiguration = "bing"
        await proxy.apply()
        await sleep(1)

        assert q1_device.init_done
        assert not q2_device.init_done
        assert q3_device.init_done
        assert q4_device.init_done

        assert proxy.actualGainConfiguration == "bing"


@pytest.mark.asyncio
@pytest.mark.timeout(30)
async def test_missing_proxies():
    configurator_id = create_instanceId()
    configurator = DsscConfigurator(
        {
            "_deviceId_": configurator_id,
            "pptDevices": [
                {"deviceId": "q1_did", "quadrantId": "Q1", "use": True},
                {"deviceId": "q2_did", "quadrantId": "Q2", "use": False},
                {"deviceId": "q3_did", "quadrantId": "Q3", "use": True},
                {"deviceId": "q4_did", "quadrantId": "Q4", "use": True},
            ],
            "availableGainConfigurations": [
                {
                    "description": "default",
                    "q1": "/path/to/conf.conf",
                    "q2": "/path/to/conf.conf",
                    "q3": "/path/to/conf.conf",
                    "q4": "/path/to/conf.conf",
                },
            ],
        }
    )

    async with AsyncDeviceContext(configurator_id=configurator):
        proxy = await connectDevice(configurator_id)
        # Check went to error on initialization as proxies are missing
        assert proxy.state == State.ERROR
        assert "Could not connect" in proxy.status

        # Check that calling apply does nothing
        await proxy.apply()
        assert proxy.state == State.ERROR
        assert "Could not connect" in proxy.status


@pytest.mark.asyncio
@pytest.mark.timeout(30)
async def test_server_loads_device():
    serverId = create_instanceId()
    server = create_device_server(serverId, [DsscConfigurator])
    async with AsyncDeviceContext(server=server) as ctx:
        server_instance = ctx.instances["server"]
        assert "DsscConfigurator" in server_instance.plugins
