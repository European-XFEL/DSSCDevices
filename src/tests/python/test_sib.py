import json
import pytest

from karabo.bound.testing import eventLoop, ServerContext, sleepUntil
from karabo.bound import Configurator, Hash

from DsscControl.sib import DsscSIB


@pytest.mark.timeout(30)
def test_sib_instantiation(eventLoop):
    device_config = {
        "SIBTestDevice": {
            "classId": "DsscSIB",
            "hostname": "1.2.3.4",
         }
    }

    init = json.dumps(device_config)
    server = ServerContext(
        "sibTestServer",
        ["log.level=DEBUG", f"init={init}"])
    with server:
        remote = server.remote()
        print(remote.getDevices())
        sleepUntil(lambda: "SIBTestDevice" in remote.getDevices(), timeout=10)


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

    ret = DsscSIB.log_v2_parser.parse('SIB_MASTER:1!LocalQloop:0!GlobalQloop:0!QloopAddress:0x0!SIBs:0')
    expected = {
        'sibMaster': 1,
        'localQloop': 0,
        'globalQloop': 0,
        'qloopAddress': 0x0,
        'sibs': 0,
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

    ret = DsscSIB.tstatus_v2_parser.parse('SIB[0]!T_STATUS:2[PPFC:2!IOB:3!Therm:0!ASIC:3]!P_STATUS:1!H_STATUS:0!CABLE_STATUS:0!COOL_STATUS:1!EXP_STATUS:1')
    expected = {
        'tStatus': 2,
        'ppfc': 2,
        'iob': 3,
        'therm': 0,
        'asic': 3,
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

    ret = DsscSIB.t1_v2_parser.parse('SIB[0]!DP1:-65535.0[3]!DP2:12.5[0]!H1:-65535.0!H2:34.2!T1:-65535.0!T2:30.0')
    expected = {
        'dp1': -65535.0,
        'dp1Status': 3,
        'dp2': 12.5,
        'dp2Status': 0,
        'h1': -65535.0,
        'h2': 34.2,
        't1': -65535.0,
        't2': 30.0,
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

    ret = DsscSIB.ntc_parser.parse('SIB[0]!NTC[0]!NTC1:16.5[0]!NTC2:65535.0[3]!NTC3:65535.0[3]!NTC4:65535.0[3]')
    expected = {
        'ntc1': 16.5,
        'ntc1Status': 0,
        'ntc2': 65535.0,
        'ntc2Status': 3,
        'ntc3': 65535.0,
        'ntc3Status': 3,
        'ntc4': 65535.0,
        'ntc4Status': 3
    }
    assert ret.named == expected

    ret = DsscSIB.log_counter_v2_parser.parse("LOG[72][3->19: 0:0:0:1:2:0:1]")
    expected = {
        'logCounter': 72,
        'decision': '3->19: 0:0:0:1:2:0:1'
    }
    assert ret.named == expected


@pytest.mark.skip(reason="Refactoring of monkeypatching required - no way to call method directly")
@pytest.mark.timeout(30)
@pytest.mark.parametrize(
    "line, expected",
    [
        # Firmware v2
        ('SIB[0]|NTC[0]|NTC1:16.5[0]|NTC2:65535.0[3]|NTC3:65535.0[3]|NTC4:65535.0[3]',
         {'ntc1': 16.5, 'ntc2': float('nan'), 'ntc2Status': 3, 'ntc3': float('nan'),
          'ntc3Status': 3, 'ntc4': float('nan'), 'ntc4Status': 3}),

        # Firmware v1, v2
        ('(16)[0]-5.0:-4.2',
         {'asic_nr': 16, 'ASICXX.status': 0, 'ASICXX.t0': -5.0, 'ASICXX.t1': -4.2,}),

        # Firmware v1, v2
        ('ASIC|TrainID:42', {'trainId': 42}),

        # Firmware v2
        ('SIB[0]|DP1:-65535.0[3]|DP2:12.5[0]|H1:-65535.0|H2:34.2|T1:-65535.0|T2:30.0',
         {'dp1': float('nan'), 'dp1Status': 3, 'dp2': 12.5, 'dp2Status': 0, 'h1': float('nan'),
          'h2': 34.2, 't1': float('nan'), 't2': 30.0,}),

        # Firmware v1
        ('SIB[0]|H1:-65535.0[3]|H2:20.5[0]|T1:-65535.0|T2:32.0',
         {'h1': float('nan'), 'h1Status': 3, 'h2': 20.5,
          'h2Status': 0, 't1': float('nan'), 't2': 32.0}),

        # Firmware v1, v2
        ('SIB[0]|MG:[1]|P1:0.9[0]|P2:-65535.0[3]|P3:-65535.0[3]|P4:1.9[0]',
         {'mg': 1, 'p1': 0.9, 'p1Status': 0, 'p2': float('nan'), 'p2Status': 3,
         'p3': float('nan'), 'p3Status': 3, 'p4': 1.9, 'p4Status': 0,}),

        # Firmware v1, v2
        ('SIB[0]|IOB[0]|IOB_T1:-0.8[0]|IOB_T2:-1.4[0]|IOB_T3:-3.4[0]|IOB_T4:-1.3[0]',
         {'iob': 0, 'iobT1': -0.8, 'iobT1Status': 0, 'iobT2': -1.4, 'iobT2Status': 0,
         'iobT3': -3.4, 'iobT3Status': 0, 'iobT4': -1.3,' iobT4Status': 0,}),

        # Firmware v1, v2
        ('SIB[0]|PPFC[0]|PPFC_T1:17.5[0]|PPFC_T2:65535.0[3]|PPFC_T3:65535.0[3]|PPFC_T4:65535.0[3]',
         {'ppfcT1': 17.5, 'ppfcT1Status': 0, 'ppfcT2': 65535.0, 'ppfcT2Status': 3,
         'ppfcT3': 65535.0, 'ppfcT3Status': 3, 'ppfcT4': 65535.0, 'ppfcT4Status': 3}),

        # Firmware v1, v2
        ('SIB[0]|VCCSUM:3.2[1]|LV:0|HV1:0|HV2:0|PICO:0',
         {'vccSum': 3.2, 'vccSumStatus': 1, 'lv': 0, 'hv1': 0, 'hv2': 0, 'pico': 0,}),

        # Firmware v2
        ('SIB[0]|T_STATUS:2[PPFC:2|IOB:3|Therm:0|ASIC:3]|P_STATUS:1|H_STATUS:0|CABLE_STATUS:0|COOL_STATUS:1|EXP_STATUS:1',
         {'tStatus': 2, 'ppfc': 2, 'iob': 3, 'therm': 0, 'asic': 3, 'pStatus': 1,
         'hStatus': 0, 'cableStatus': 0, 'coolStatus': 1, 'expStatus': 1,}),

        # Firmware v1
        ('SIB[0]|T_STATUS:2[PPFC:0|IOB:0|ASIC:2]|P_STATUS:1|H_STATUS:0|CABLE_STATUS:0|COOL_STATUS:1|EXP_STATUS:1',
         {'tStatus': 2, 'ppfc': 0, 'iob': 0, 'asic': 2, 'pStatus': 1,
         'hStatus': 0, 'cableStatus': 0, 'coolStatus': 1, 'expStatus': 1,}),

        # Firmware v1, v2
        ('SIB[0]|LastTrainID:42|SIB STATE:2|SIB Operation Mode:0',
         {'lastTrainId': 42, 'sibState': 2, 'som': 0}),

        # Firmware v2
        ('SIB_MASTER:1|LocalQloop:0|GlobalQloop:0|QloopAddress:0x0|SIBs:0',
         {'sibMaster': 1, 'localQloop': 0, 'globalQloop': 0,
         'qloopAddress': 0x0, 'sibs': 0,}),

        # Firmware v1
        ('LOG[21357];SIB_MASTER:1|LocalQloop:0|GlobalQloop:0|QloopAddress:0x0|SIBs:2',
         {'logCounter': 21357, 'sibMaster': 1, 'localQloop': 0,
         'globalQloop': 0, 'qloopAddress': 0x0, 'sibs': 2,}),

        # Firmware v2
        ('LOG[72][3->19: 0:0:0:1:2:0:1]',
         {'logCounter': 72, 'decision': '3->19: 0:0:0:1:2:0:1'}),
    ]
)
def test_process_data_row(eventLoop, capsys, monkeypatch, line, expected):
    def mock_set(hash_, ts=None):
        h = Hash()
        for k, v in expected.items():
            h[k] = v

        assert hash_ == h, "+++ FAILED"

    monkeypatch.setattr(DsscSIB, "set", mock_set)

    config = {"SIBTestDevice": {"classId": "DsscSIB",
                                "hostname": "1.2.3.4",}}
    init = json.dumps(config)
    server = ServerContext(
        "sibTestServer",
        [f"init={init}", "deviceClasses=DsscSIB"])
    with server:
        remote = server.remote()
        sleepUntil(lambda: "SIBTestDevice" in remote.getDevices(), timeout=5)
        sib = remote.get("SIBTestDevice")

        sib.process_data_row(line, ts=None)

    out = capsys.readouterr().out
    # Check that the device could set the properties correctly
    assert "Exception" not in out
    # Check that assert in mock_set did not raise
    assert "+++ FAILED" not in out

@pytest.mark.timeout(30)
def test_sib_instantiation(eventLoop):
    device_config = {
        "SIBTestDevice": {
            "classId": "DsscSIB",
            "hostname": "1.2.3.4",
         }
    }

    init = json.dumps(device_config)
    server = ServerContext(
        "sibTestServer",
        [f"init={init}"])
    with server:
        remote = server.remote()
        sleepUntil(lambda: "SIBTestDevice" in remote.getDevices(), timeout=10)
