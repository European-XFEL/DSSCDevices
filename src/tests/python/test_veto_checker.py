import uuid

import numpy as np
import pytest
from DsscControl.veto_checker import DsscVetoCheck
from karabo.middlelayer import (
    Bool,
    Device,
    Hash,
    State,
    UInt32,
    VectorUInt32,
    connectDevice,
)
from karabo.middlelayer.testing import (
    AsyncDeviceContext,
    create_device_server,
    event_loop,
)

from sample_veto_pattern import PATTERN

def create_instanceId():
    return f"test-mdl-{uuid.uuid4()}"


class MockDevice(Device):
    """Mock PPT and Clock&ControlMonitor devices."""
    numPreBurstVetos = UInt32(defaultValue=10)
    numFramesToSendOut = UInt32(defaultValue=182)
    veto_pattern = VectorUInt32(defaultValue=PATTERN)
    send_dummy_dr_data = Bool(defaultValue=False)


@pytest.mark.asyncio
@pytest.mark.timeout(30)
async def test_device(event_loop):
    ppt_did = create_instanceId()
    ppt_device = MockDevice({"_deviceId_": ppt_did})

    ccmon_did = create_instanceId()
    ccmon_device = MockDevice({"_deviceId_": ccmon_did})

    veto_check_did = create_instanceId()
    veto_check = DsscVetoCheck(
        {
            "_deviceId_": veto_check_did,
            "ccmon": ccmon_did,
            "pptControl": ppt_did,
         }
    )

    instances = {
        ppt_did: ppt_device,
        ccmon_did: ccmon_device,
        veto_check_did: veto_check,
    }

    async with AsyncDeviceContext(**instances):
        proxy = await connectDevice(veto_check_did)
        # Check initialization was correct
        assert proxy.notOkCount == 0
        assert proxy.ok == State.ON.value
        assert proxy.state == State.PASSIVE


@pytest.mark.asyncio
@pytest.mark.timeout(30)
async def test_server_loads_device():
    serverId = create_instanceId()
    server = create_device_server(serverId, [DsscVetoCheck])
    async with AsyncDeviceContext(server=server) as ctx:
        server_instance = ctx.instances["server"]
        assert "DsscVetoCheck" in server_instance.plugins


@pytest.mark.skip(reason="feature not done yet")
def test_validate_data():
    # Test detector not sending data
    det_data = Hash(
        "image.cellId",
        Hash(
            "type", "UInt32",
            "isBigEndian", "<",
            "data", [],
            "shape", np.array([0, 0]),
        ),
        "image.pulseId",
        Hash(
            "type", "UInt32",
            "isBigEndian", "<",
            "data", [],
            "shape", np.array([0, 0]),
        ),
    )
    ok, msg, data = DsscVetoCheck.validate_data(det_data, ([], []))
    assert not ok
    assert "not sending data" in msg
    assert data is None

    # Test corrupted cell IDs
    det_data = Hash(
        "image.cellId",
        Hash(
            "type", 16,
            "isBigEndian", False,
            "data", np.arange(790, 802),
            "shape", np.array([12, 1]),
        ),
        "image.pulseId",
        Hash(
            "type", 16,
            "isBigEndian", False,
            "data", np.arange(12),
            "shape", np.array([12, 1]),
        ),
    )

    ok, msg, data = DsscVetoCheck.validate_data(det_data, ([], []))
    assert not ok
    assert "over 800" in msg
    assert isinstance(data, tuple)

    # Test cell ids not used in expected order
    det_data["image.cellId"]["data"] = np.arange(12)
    ok, msg, data = DsscVetoCheck.validate_data(det_data, ([], []))
    assert not ok
    assert "Cell ID" in msg
    assert isinstance(data, tuple)

    # Test pulse ids not used in expected order
    sim_data = (np.arange(12), [])
    ok, msg, data = DsscVetoCheck.validate_data(det_data, sim_data)
    assert not ok
    assert "Pulse ID" in msg
    assert isinstance(data, tuple)

    # Test all ok
    sim_data = (np.arange(12), np.arange(12))
    ok, msg, data = DsscVetoCheck.validate_data(det_data, sim_data)
    assert ok
    assert not msg
    assert isinstance(data, tuple)


@pytest.mark.skip(reason="feature not done yet")
def test_veto_pattern_to_sim_data():
    # The following is read data from the detector, with pattern:
    # "Veto these range(10, 1300, 160)", but truncated for brevity (79-95)
    preveto = 10
    cells = 16
    pattern = (
        20559, 20560, 20561, 20562, 20563, 20564, 20565, 20566, 20567, 20568,
        20569, 20570, 20571, 20572, 20573, 20574,
    )

    expected_cell_id = [
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
        28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
        46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
        92, 93, 94, 95, 96, 97,
    ]

    expected_pulse_id = [
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
        28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
        46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
        82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
        100, 101, 102, 103, 104, 105, 106, 107,
    ]

    cell_id, pulse_id = DsscVetoCheck.veto_pattern_to_sim_data(
        pattern, preveto, cells
    )

    assert np.all(cell_id == expected_cell_id)
    assert np.all(pulse_id == expected_pulse_id)
