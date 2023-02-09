# Test the DsscASICreset device.
import pytest

from DsscControl.DsscASICreset import DsscASICreset
from karabo.middlelayer import Hash

class mock_proxy:
    def __init__(self, deviceId: str):
        self.deviceId = deviceId

@pytest.mark.parametrize(
    "proxies, disconnected_count",
    [
     ({}, 4),
     ({
        0: mock_proxy("PPT_Q1"),
        1: mock_proxy("PPT_Q2"),
        2: mock_proxy("PPT_Q3"),
        3: mock_proxy("PPT_Q4"),
    }, 0),
     ({
        0: mock_proxy("PPT_Q1"),
        1: mock_proxy("PPT_Q2"),
        3: mock_proxy("PPT_Q4"),
    }, 1),
     ({
        0: mock_proxy("PPT_Q1"),
        3: mock_proxy("PPT_Q4"),
    }, 2),
     ({
        0: mock_proxy("PPT_Q1"),
    }, 3),
    ]
)
def test_request_scene(proxies, disconnected_count):
    device = DsscASICreset(Hash())

    assert device.availableScenes == ["overview"]
    device.ppt_by_indx = proxies 

    res = device.requestScene({"name": "overview"})
    assert res["payload"]["name"] == "overview"
    
    scene = res["payload"]["data"]
    assert scene.count("disconnected") == disconnected_count

    for px in proxies.values():
        assert scene.count(px.deviceId) == 1
