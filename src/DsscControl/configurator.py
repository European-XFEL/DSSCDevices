"""Set and monitor configurations across PPTs.

This device monitors several PPTs and ensures they have the same configuration.
"""
from asyncio import Lock, wait_for
from pathlib import Path

from karabo.middlelayer import (
    AccessMode,
    Bool,
    Configurable,
    Device,
    Hash,
    Overwrite,
    Slot,
    State,
    String,
    VectorHash,
    VectorString,
    allCompleted,
    background,
    connectDevice,
    setWait,
    slot,
    waitUntilNew,
)

from ._version import version as deviceVersion
from .scenes.configurator_scene import get_scene


class PptRowSchema(Configurable):
    deviceId = String()
    quadrantId = String()
    use = Bool()


class ConfigurationRowSchema(Configurable):
    description = String(displayedName="Config. description", defaultValue="")
    q1 = String(displayedName="Q1 ConfigFileName", defaultValue="")
    q2 = String(displayedName="Q2 ConfigFileName", defaultValue="")
    q3 = String(displayedName="Q3 ConfigFileName", defaultValue="")
    q4 = String(displayedName="Q4 ConfigFileName", defaultValue="")


class DsscConfigurator(Device):
    __version__ = deviceVersion

    state = Overwrite(
        options={State.ACTIVE, State.INIT, State.ERROR, State.CHANGING},
        defaultValue=State.INIT,
    )

    pptDevices = VectorHash(
        rows=PptRowSchema,
        displayedName="PPT devices to connect",
        defaultValue=[
            Hash(
                "deviceId", f"SCS_CDIDET_DSSC/FPGA/PPT_Q{q}",
                "quadrantId", f"Q{q}",
                "use", True,
            ) 
            for q in range(1, 5)]
    )

    monitoredDevices = String(
        displayedName="Monitored Devices",
        defaultValue="",
        accessMode=AccessMode.READONLY,
    )

    availableGainConfigurations = VectorHash(
        rows=ConfigurationRowSchema,
        displayedName="Configurations",
        defaultValue=[Hash()],
    )

    gainConfigurationState = String(
        displayedName="Configuration State",
        description="Shows whether PPTs have matching configurations. "
                    "Incorrect until proven otherwise.",
        defaultValue=State.ERROR.value,
        displayType="State",
        options={State.ON.value, State.ERROR.value},
        accessMode=AccessMode.READONLY,
    )

    actualGainConfiguration = String(
        displayedName="Applied Gain Configuration",
        defaultValue="",
        accessMode=AccessMode.READONLY,
    )

    targetGainConfiguration = String(
        displayedName="Desired Gain Configuration",
        defaultValue="",
        options=[""],
        accessMode=AccessMode.RECONFIGURABLE,
    )

    # Lock used to block state check when applying new configurations
    # to avoid spurrious updates
    lock = None

    async def onInitialization(self):
        self.state = State.INIT
        configs = [row[0] for row in self.availableGainConfigurations.value]

        self.__class__.targetGainConfiguration = Overwrite(
            options=configs,
            defaultValue=configs[0],
        )
        await self.publishInjectedParameters()

        ppts = {}
        for row in self.pptDevices.value:
            did, qid, use = row
            if use:
                ppts[qid] = wait_for(connectDevice(did), 5)

        ppts, _, errors = await allCompleted(**ppts)
        if errors:
            self.state = State.ERROR
            msg = f"Could not connect to {list(errors.keys())}"
            self.status = msg
            msg += f" {errors}"
            self.log.INFO(msg)
            return

        self.monitoredDevices = ", ".join(sorted(ppts.keys()))
        self.ppts = ppts

        self.lock = Lock()

        background(self.monitor_configs())
        self.status = "Monitoring"
        self.state = State.ACTIVE

    async def monitor_configs(self):
        while True:
            async with self.lock:
                configs = {
                    qid: Path(ppt.fullConfigFileName).name
                    for qid, ppt in self.ppts.items()
                }
                fnames = list(configs.values())

                identical = all(fname == fnames[0] for fname in fnames)

                if not identical:
                    msg = "Different Settings Applied: "
                    msg += ", ".join(f"{qid}: {fname}" for qid, fname in configs.items())
                    self.actualGainConfiguration = msg
                    self.log.INFO(msg)
                    self.gainConfigurationState = State.ERROR.value
                else:
                    # Convert from filenames to human 
                    row = [row for row in self.availableGainConfigurations.value
                            if fnames[0] in row[1]]
                    if row:
                        name, *_ = row[0]
                    else:
                        name = fnames[0]
                        self.log.INFO(f"Strange: configuration {name} is correct, but not know to me")

                    self.actualGainConfiguration = name
                    self.gainConfigurationState = State.ON.value

            configs = [ppt.fullConfigFileName for ppt in self.ppts.values()]
            await waitUntilNew(*configs)

    @Slot(
        displayedName="Apply",
        description="Apply the selected config to the PPTs",
        allowedStates={State.ACTIVE},
    )
    async def apply(self):
        background(self._apply)

    async def _apply(self):
        self.state = State.CHANGING
        self.status = f"Applying {self.targetGainConfiguration}"
        # Convert from human to filenames
        row, = [row for row in self.availableGainConfigurations.value
                if row[0] == self.targetGainConfiguration]
        name, q1, q2, q3, q4 = row

        d = {
            "Q1": q1,
            "Q2": q2,
            "Q3": q3,
            "Q4": q4,
        }

        coros = {qid: setWait(ppt, fullConfigFileName=d[qid])
                 for qid, ppt in self.ppts.items()}

        async with self.lock:
            done, _, errors = await allCompleted(**coros)

        if errors:
            msg = f"Could not set {name} on "
            msg += ", ".join(errors.keys())
            self.status = msg

            msg += f" {errors}"
            self.log.INFO(msg)

            self.state = State.ERROR
            return

        msg = f"Applied {name}"
        self.status = msg
        self.log.INFO(msg)

        self.status = f"Reinitializing detector"

        coros = {qid: ppt.initSystem() for qid, ppt in self.ppts.items()}
        done, _, errors = await allCompleted(**coros)

        if errors:
            msg = f"Could not init system on "
            msg += ", ".join(errors.keys())

            self.status = msg

            msg += f" {errors}"
            self.log.INFO(msg)

            self.state = State.ERROR
            return

        msg = "Reinitialized detector"
        self.state = State.ACTIVE

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
            "data", get_scene(self.deviceId),
        )
        return Hash(
            "type", "deviceScene",
            "origin", self.deviceId,
            "payload", payload
        )
