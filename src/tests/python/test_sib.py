import pytest

from DsscSIB.DsscSIB import DsscSIB
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


def test_parsers():
    # The parsers expect ! instead of the | sent by the SIB, as the
    # pipe is a special character for the parser library.
    # The receiver in the device does the conversion, as well as stripping the
    # delimiters.
    ret = DsscSIB.log_parser.parse('LOG[21357];SIB_MASTER:1!LocalQloop:0!GlobalQloop:0!QloopAddress:0x0!SIBs:2')
    expected = {
        'logCounter': 21357,
        'sibMaster': 1,
        'localQloop': 0,
        'globalQloop': 0,
        'qloopAddress': 0x0,
        'sibs': 2,
    }
    assert ret.named == expected

    ret = DsscSIB.last_trainid_parser.parse('SIB[0]!LastTrainID:42!SIB STATE:2!SIB Operation Mode:0')
    expected = {
        'lastTrainId': 42,
        'sibState': 2,
        'som': 0
    }
    assert ret.named == expected

    ret = DsscSIB.tstatus_parser.parse('SIB[0]!T_STATUS:2[PPFC:0!IOB:0!ASIC:2]!P_STATUS:1!H_STATUS:0!CABLE_STATUS:0!COOL_STATUS:1!EXP_STATUS:1')
    expected = {
        'tStatus': 2,
        'ppfc': 0,
        'iob': 0,
        'asic': 2,
        'pStatus': 1,
        'hStatus': 0,
        'cableStatus': 0,
        'coolStatus': 1,
        'expStatus': 1,
    }
    assert ret.named == expected

    ret = DsscSIB.vccsum_parser.parse('SIB[0]!VCCSUM:3.2[1]!LV:0!HV1:0!HV2:0!PICO:0')
    expected = {
        'vccSum': 3.2,
        'vccSumStatus': 1,
        'lv': 0,
        'hv1': 0,
        'hv2': 0,
        'pico': 0,
    }
    assert ret.named == expected

    ret = DsscSIB.ppfc_parser.parse('SIB[0]!PPFC[0]!PPFC_T1:17.5[0]!PPFC_T2:65535.0[3]!PPFC_T3:65535.0[3]!PPFC_T4:65535.0[3]')
    expected = {
        'ppfcT1': 17.5,
        'ppfcT1Status': 0,
        'ppfcT2': 65535.0,
        'ppfcT2Status': 3,
        'ppfcT3': 65535.0,
        'ppfcT3Status': 3,
        'ppfcT4': 65535.0,
        'ppfcT4Status': 3,
    }
    assert ret.named == expected

    ret = DsscSIB.iob_parser.parse('SIB[0]!IOB[0]!IOB_T1:-0.8[0]!IOB_T2:-1.4[0]!IOB_T3:-3.4[0]!IOB_T4:-1.3[0]')
    expected = {
        'iob': 0,
        'iobT1': -0.8,
        'iobT1Status': 0,
        'iobT2': -1.4,
        'iobT2Status': 0,
        'iobT3': -3.4,
        'iobT3Status': 0,
        'iobT4': -1.3,
        'iobT4Status': 0,
    } 
    assert ret.named == expected

    ret = DsscSIB.mg_parser.parse('SIB[0]!MG:[1]!P1:0.9[0]!P2:-65535.0[3]!P3:-65535.0[3]!P4:1.9[0]')
    expected = {
        'mg': 1,
        'p1': 0.9,
        'p1Status': 0,
        'p2': -65535.0,
        'p2Status': 3,
        'p3': -65535.0,
        'p3Status': 3,
        'p4': 1.9,
        'p4Status': 0,
    }
    assert ret.named == expected

    ret = DsscSIB.t1_parser.parse('SIB[0]!H1:-65535.0[3]!H2:20.5[0]!T1:-65535.0!T2:32.0')
    expected = {
        'h1': -65535.0,
        'h1Status': 3,
        'h2': 20.5,
        'h2Status': 0,
        't1': -65535.0,
        't2': 32.0,
    }
    assert ret.named == expected

    ret = DsscSIB.asic_trainid_parser.parse('ASIC!TrainID:42')
    expected = {
        'trainId': 42
    }
    assert ret.named == expected

    ret = DsscSIB.asic_parser.parse('(16)[0]-5.0:-4.2')
    expected = {
        'asic_nr': 16,
        'ASICXX.status': 0,
        'ASICXX.t0': -5.0,
        'ASICXX.t1': -4.2,
    }
    assert ret.named == expected
