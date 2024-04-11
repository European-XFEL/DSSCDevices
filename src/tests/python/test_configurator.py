import uuid

import pytest

import DsscControl.configurator  # Used so to mock shutdowns
from karabo.middlelayer import (
    Device,
    Slot,
    String,
    connectDevice,
    State,
    waitUntil,
    waitUntilNew,
)
from karabo.middlelayer.testing import (
    AsyncDeviceContext,
    create_device_server,
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
async def test_apply_configuration(monkeypatch):
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
                    "filenamePath": "/path/to/conf.conf",
                },
                {
                    "description": "bing",
                    "filenamePath": "/path/to/q1/config.conf",
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

    async def mock_mdl_shutdown(device_id):
        print(f"Shutting down {device_id}")

    monkeypatch.setattr(
        DsscControl.configurator,
        "shutdown",
        mock_mdl_shutdown
    )

    async with AsyncDeviceContext(**instances):
        proxy = await connectDevice(configurator_id)

        async def mock_mdl_ifm(device_id, name):
            instances[device_id].fullConfigFileName = "/path/to/q1/config.conf"

        monkeypatch.setattr(
            DsscControl.configurator,
            "instantiateFromName",
            mock_mdl_ifm
        )

        # Check initialization was correct
        assert proxy.monitoredDevices == "Q1, Q3, Q4"

        # Check matched configurations have friendly description
        await waitUntilNew(proxy.actualGainConfiguration)
        assert proxy.actualGainConfiguration == "default"

        # Check mismatched configurations are reported
        q1_device.fullConfigFileName = "bleh"
        await waitUntilNew(proxy.actualGainConfiguration)
        assert "Different Settings Applied" in proxy.actualGainConfiguration
        assert proxy.gainConfigurationState == State.ERROR.value

        # Check target configurations are validated
        with pytest.raises(ValueError):
            proxy.targetGainConfiguration = "something illegal"

        # Check target configuration applied across selected devices
        proxy.targetGainConfiguration = "bing"
        await proxy.apply()
        await waitUntil(lambda: proxy.state == State.CHANGING)
        await waitUntil(lambda: proxy.state == State.ACTIVE)
        await waitUntilNew(proxy.actualGainConfiguration)

        assert proxy.actualGainConfiguration == "bing"
        assert proxy.gainConfigurationState == State.ON.value


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
                    "filenamePath": "/path/to/conf.conf",
                },
            ],
        }
    )

    async with AsyncDeviceContext(configurator_id=configurator):
        proxy = await connectDevice(configurator_id)
        # Check goes to init waiting for proxies
        assert proxy.state == State.INIT
        assert proxy.gainConfigurationState == State.ERROR.value


@pytest.mark.asyncio
@pytest.mark.timeout(30)
async def test_server_loads_device():
    serverId = create_instanceId()
    server = create_device_server(serverId, [DsscConfigurator])
    async with AsyncDeviceContext(server=server) as ctx:
        server_instance = ctx.instances["server"]
        assert "DsscConfigurator" in server_instance.plugins
