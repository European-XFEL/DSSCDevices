import pytest

from DsscSIB import DsscSIB
from karabo.bound import Configurator, Hash, PythonDevice


def test_sib_instantiation():
    sib = Configurator(PythonDevice).create(
        "DsscSIB",
        Hash(
            "Logger.priority", "DEBUG", "deviceId", "DSSC_SIB_0", "hostname", "1.2.3.4"
        ),
    )
    sib.startFsm()


def test_sib_no_ip():
    with pytest.raises(RuntimeError):
        Configurator(PythonDevice).create(
            "DsscSIB", Hash("Logger.priority", "DEBUG", "deviceId", "DSSC_SIB_0")
        )
