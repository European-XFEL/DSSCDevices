"""Validate the veto of a DSSC module.

Connect to a ClockAndControl and the output of a detector and checks that the
pulse Ids fit the veto pattern, and that detector data values are in range.
"""
import time
from asyncio import Lock, sleep
from queue import Queue
from typing import List, Optional, Tuple, Union

import numpy as np
from karabo.middlelayer import (AccessMode, Configurable, Device, Hash,
                                InputChannel, Node, OutputChannel, Overwrite,
                                State, String, UInt32, VectorString,
                                VectorUInt32, background, connectDevice,
                                get_array_data, slot, waitUntilNew)

from .scenes.veto_scene import overview


class DataNode(Configurable):
    """Class describing the output channel."""

    detCellId = VectorUInt32(
        displayedName="Detector Cell ID",
        accessMode=AccessMode.READONLY,
    )
    detPulseId = VectorUInt32(
        displayedName="Detector Pulse ID",
        accessMode=AccessMode.READONLY,
    )

    simCellId = VectorUInt32(
        displayedName="Simulated Cell ID",
        accessMode=AccessMode.READONLY,
    )
    simPulseId = VectorUInt32(
        displayedName="Simulated Pulse ID",
        accessMode=AccessMode.READONLY,
    )


class ChannelNode(Configurable):
    data = Node(DataNode)


class AsicsNode(Configurable):
    pptVeto = UInt32(
        displayedName="PPT Veto",
        description="Veto code the PPT received",
        defaultValue=0,
        accessMode=AccessMode.READONLY,
    )
    for idx in range(0, 16):
        locals()[f"asic{idx}"] = String(
           displayedName=f"ASIC {idx} State",
           description="ON if ok, ERROR if not ok, PASSIVE if broken",
           enum=State,
           defaultValue=State.UNKNOWN,
           displayType="State",
           options={
                State.UNKNOWN,
                State.ON,
                State.ERROR,
                State.PASSIVE,
            },
           accessMode=AccessMode.READONLY,
        )
        locals()[f"veto{idx}"] = UInt32(
            displayedName=f"ASIC {idx} Veto",
            description="ASIC Veto code received, should be same as PPT Veto",
            defaultValue=0,
            accessMode=AccessMode.READONLY,
        )


class DsscVetoCheck(Device):
    state = Overwrite(
        options={State.PASSIVE, State.PROCESSING},
        defaultValue=State.PASSIVE,
    )

    output = OutputChannel(ChannelNode)

    ccmon = String(
        displayedName="CCMON",
        description="The detector's clock and control, "
                    "used to get the veto pattern",
        accessMode=AccessMode.INITONLY,
    )

    pptControl = String(
        displayedName="",
        description="The DSSC control, used to get the preveto burst. "
                    "It can be either of PPT or Control device",
        accessMode=AccessMode.INITONLY,
    )

    # This is a State String, not a Bool, as it has 3 possible states
    ok = String(
        displayedName="Data Ok",
        description="ON when ok, ERROR when not ok, UNKNOWN when no data",
        enum=State,
        defaultValue=State.ON,
        displayType="State",
        options={State.ON, State.ERROR, State.UNKNOWN},
        accessMode=AccessMode.READONLY,
    )

    notOkCount = UInt32(
        displayedName="Invalid Data Count",
        description="Consecutive Invalid Data Count",
        defaultValue=0,
        accessMode=AccessMode.READONLY,
    )

    asicStatus = Node(
        AsicsNode,
        displayedName="ASICs Status",
    )

    def __init__(self, configuration):
        super().__init__(configuration)
        self.sim_data: Tuple[List[int], List[int]] = None
        self.is_ppt_sending_dummy: bool = False
        self.lock: Lock = Lock()

        # -100 to stay in PASSIVE mode on first iteration until data comes
        self.last_update_time: float = time.time() - 100

    async def onInitialization(self):
        self.ccmon_proxy = await connectDevice(self.ccmon)
        self.ppt_proxy = await connectDevice(self.pptControl)
        background(self.monitor_properties)
        background(self.state_timer)
        self.status = "Waiting for data"

    @InputChannel(
        displayedName="Calibration Pipeline Output",
        description="CORRECT_DEVICE:dataOutput",
        raw=True,
    )
    async def input(self, det_data: Hash, meta: Hash):
        async with self.lock:
            self.last_update_time = time.time()
            asic_states, asic_vetos, ppt_veto = self.validate_asic_vetos(
                det_data,
                self.is_ppt_sending_dummy
            )
            ok, msg, data = self.validate_data(det_data, self.sim_data)

        if self.state == State.PASSIVE:
            self.state = State.PROCESSING

        ok_state = State.ON if ok else State.ERROR
        if ok_state != self.ok:
            self.ok = ok_state
            self.notOkCount = 0

        if self.status.value != msg:
            self.status = msg

        if not ok:
            self.notOkCount = self.notOkCount.value + 1

        if data is not None:  # Forward for GUI display
            det_cell_id, det_pulse_id = data
        else:
            det_cell_id, det_pulse_id = [], []

        if self.sim_data is not None:
            sim_cell_id, sim_pulse_id = self.sim_data
        else:
            sim_cell_id, sim_pulse_id = [], []

        h = Hash(
            "data.detCellId", det_cell_id,
            "data.detPulseId", det_pulse_id,
            "data.simCellId", sim_cell_id,
            "data.simPulseId", sim_pulse_id,
        )
        await self.output.writeRawData(h)

        if ppt_veto != self.asicStatus.pptVeto:
            self.asicStatus.pptVeto = ppt_veto

        for idx, asic_state in enumerate(asic_states):
            curr_asic_state = getattr(self.asicStatus, f"asic{idx}").value
            if asic_state != curr_asic_state:
                setattr(self.asicStatus, f"asic{idx}", asic_state)

        for idx, asic_veto in enumerate(asic_vetos):
            curr_asic_veto = getattr(self.asicStatus, f"veto{idx}").value
            if asic_veto != curr_asic_veto:
                setattr(self.asicStatus, f"veto{idx}", asic_veto)

    @staticmethod
    def validate_asic_vetos(
        det_data: Hash,
        is_dummy: bool
    ) -> Union[List[str], List[int], int]:
        d = det_data["detector.data"]
        ppt_veto = d[0] + d[1] * 256  # 2 uint8_t to uint16_t

        if is_dummy:
            return [State.UNKNOWN] * 16, [0] * 16, 0

        asic_states = []
        asic_vetos = []
        for idx in range(0, 16):
            asic_veto = d[162 + idx*16] + (d[163 + idx*16] * 256)
            asic_state = State.ON
            if asic_veto != ppt_veto:
                asic_state = State.ERROR
            if asic_veto == 384 + idx:
                asic_state = State.PASSIVE
            asic_states.append(asic_state)
            asic_vetos.append(asic_veto)

        return asic_states, asic_vetos, ppt_veto

    @staticmethod
    def validate_data(
        det_data: Hash,
        sim_data: Tuple,
    ) -> Union[bool, str, Optional[Tuple]]:
        """Process the data from the detector.

        Return an ok boolean, a message, and the decoded data.
        data (when ok).

        The ok flag describes whether the data is considered sane.
        The message describes the encountered error if ok is False, else
        reports "ok".
        The pulse_id and cell_id data are returned together as two arrays in
        the data tuple, when available.

        This function checks whether the detector is sending data;
        If so, whether the reported cell_ids make sense and if it matches the
        simulated data.
        """
        sim_cell_id, sim_pulse_id = None, None
        if sim_data is not None:
            sim_cell_id, sim_pulse_id = sim_data
        try:
            det_cell_id = get_array_data(det_data, path="image.cellId").squeeze()  # noqa
            det_pulse_id = get_array_data(det_data, path="image.pulseId").squeeze()  # noqa

            assert not np.any(det_cell_id[det_cell_id > 800]), "Cell IDs over 800"  # noqa
            # assert np.all(det_cell_id == sim_cell_id), "Cell ID not matching simulation"  # noqa
            # assert np.all(det_pulse_id == sim_pulse_id), "Pulse ID not matching simulation"  # noqa

        except ValueError:
            return False, "Detector not sending data", None
        except AssertionError as e:
            return False, str(e), (det_cell_id, det_pulse_id)
        else:
            return True, 'ok', (det_cell_id, det_pulse_id)

    async def monitor_properties(self):
        while True:
            async with self.lock:
                self.sim_data = self.veto_pattern_to_sim_data(
                    self.ccmon_proxy.veto_sim_memory.value,
                    self.ppt_proxy.numPreBurstVetos.value,
                    self.ppt_proxy.numFramesToSendOut.value,
                )
            self.is_ppt_sending_dummy = self.ppt_proxy.send_dummy_dr_data

            await waitUntilNew(
                self.ccmon_proxy.veto_sim_memory,
                self.ppt_proxy.numPreBurstVetos,
                self.ppt_proxy.numFramesToSendOut,
                self.ppt_proxy.send_dummy_dr_data,
            )

    @staticmethod
    def veto_pattern_to_sim_data(
        pattern: List[int],
        preveto: int,
        frames: int,
    ) -> Tuple[List[int], List[int]]:
        """Convert the veto bit pattern into the expected detector output.

        pattern is a list coming from the Clock&Control, indicating the veto
        type for each pulse in the train.

        The first 3 bits indicate the veto type:
            0b111: Golden  (True)
            0b110: Regular (True)
            0b101: None    (False)

        This information is used to create simulated detector output: in which
        cell is the data for said pulse.
        Information on the type of veto (regular or golden) are dropped, as
        only matters whether the pulse is used or not.

        The detector preveto is also taken into consideration to validate the
        pattern for the given number of frames it is configured to send.

        This function returns a `cell_id` array describing in which order are
        the memory cells used, and another `pulse_id` describing the veto
        pattern.

        Together they can be combined in an XYGraph to show which memory cell
        is used for each pulse in a train.
        """
        veto_latency = 82  # Detector specific™: ASIC latency + 2

        veto = [False] * veto_latency
        veto += [True] * preveto

        veto += [False if (e >> 12) == 0b101 else True for e in pattern]
        veto = [0] * veto_latency
        veto += [1] * preveto

        veto += [0 if (e >> 12) == 0b101 else 1 for e in pattern]

        cell_id = []
        pulse_id = []

        fifo = Queue(veto_latency)

        # len(veto): veto_latency (82) + preveto (variable) + cc pattern (2700)
        for pulse in range(800):
            if veto[pulse]:
                # Although the fifo starts empty, it gets at least the first
                # {veto_latency} quantity as the else block is hit first.
                # This is guaranteed due to detector limitations.
                # Refer to the documentation for detailed information.
                addr = fifo.get()
                x = cell_id.index(addr)
                cell_id.pop(x)
                # pulse_id.pop(x)
            else:
                addr = len(cell_id)

            if fifo.full():
                fifo.get()  # Make some space
            fifo.put(addr)

            # Append the current pulse and cell IDs.
            cell_id.append(addr)
            pulse_id.append(pulse)

            if addr >= 799:  # The maximum frames the detector can send
                break

        return cell_id, pulse_id

    async def state_timer(self):
        timeout = 10  # seconds
        while True:
            async with self.lock:
                if (time.time() - self.last_update_time > timeout
                        and self.state == State.PROCESSING):
                    self.state = State.PASSIVE
                    self.status = f"No Data in last {timeout} seconds."
                    self.ok = State.UNKNOWN
                    self.notOkCount = 0
                    for idx in range(16):
                        setattr(self.asicStatus, f"asic{idx}", State.UNKNOWN)
            await sleep(5)

    availableScenes = VectorString(
        displayType="Scenes",
        accessMode=AccessMode.READONLY,
        defaultValue=["overview"],
    )

    @slot
    def requestScene(self, params):
        name = params.get("name", default="overview")
        payload = Hash(
            "success", True,
            "name", name,
            "data", overview(self.deviceId),
        )
        return Hash(
            "type", "deviceScene",
            "origin", self.deviceId,
            "payload", payload,
        )
