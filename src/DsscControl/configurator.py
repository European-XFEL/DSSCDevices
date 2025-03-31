"""Set and monitor configurations across PPTs.

This device monitors several PPTs and ensures they have the same configuration.
"""
import re

from asyncio import Lock, TimeoutError, sleep, wait_for
from pathlib import Path
from typing import Dict

from karabo.middlelayer import (
    AccessMode,
    Bool,
    Configurable,
    Device,
    DeviceClientBase,
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
    getDevices,
    instantiateFromName,
    slot,
    shutdown,
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
    filenamePath = String(displayedName="Filename Path", defaultValue="")


class DsscConfigurator(DeviceClientBase, Device):
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

    @VectorHash(
        rows=ConfigurationRowSchema,
        displayedName="Configurations",
        defaultValue=[Hash()],
    )
    async def availableGainConfigurations(self, table):
        self.availableGainConfigurations = table

        # Update targetGainConfiguration with the latest available config.
        configs = [row[0] for row in self.availableGainConfigurations.value]

        # Make it selected by default
        self.__class__.targetGainConfiguration = Overwrite(
            options=configs,
            defaultValue=configs[-1],  # Most likely appended here.
        )
        await self.publishInjectedParameters()

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

    # Lock used to block configuration check while creating proxies
    lock = None

    # Dictionary of proxies, used for monitoring new configurations.
    ppts: Dict[str, "proxy"] = {}

    async def onInitialization(self):
        self.state = State.INIT
        configs = [row[0] for row in self.availableGainConfigurations.value]

        self.__class__.targetGainConfiguration = Overwrite(
            options=configs,
            defaultValue=configs[0],
        )
        await self.publishInjectedParameters()

        self.lock = Lock()

        background(self.connect_to_proxies())
        background(self.monitor_configs())

    async def connect_to_proxies(self):
        """Connect to proxies upon instantiation.

        This device may be instantiated before PPTs, as PPTs in turn may
        request their configuration from here.
        As such, this coro runs in the background during the first connection,
        blocking the monitoring.
        """
        ppts_in_use = {}
        for row in self.pptDevices.value:
            did, qid, use = row
            if use:
                ppts_in_use[qid] = did

        connected = False
        async with self.lock:
            while not connected:
                # 10 seconds has shown to be on the safer side
                # while retrieving device schema
                coros = {
                    qid: wait_for(connectDevice(did), 10)
                    for qid, did in ppts_in_use.items()
                }
                ppts, _, errors = await allCompleted(**coros)

                self.ppts = ppts  # Some PPTs might be instantiated

                if errors:
                    msg = f"Could not connect to {list(errors.keys())}"
                    self.status = msg
                    msg += f" {errors}"
                    self.log.INFO(msg)
                else:
                    self.state = State.ACTIVE
                    self.status = "Monitoring"
                    self.monitoredDevices = ", ".join(sorted(self.ppts.keys()))
                    connected = True
            await sleep(5)

    async def monitor_configs(self):
        while True:
            async with self.lock:
                configs = {
                    qid: Path(ppt.fullConfigFileName).name
                    for qid, ppt in self.ppts.items()
                }
                fnames = list(re.sub(f"^{qid}", "", fname)
                              for qid, fname in configs.items())
                identical = all(fname == fnames[0] for fname in fnames)

                if not identical:
                    msg = "Different Settings Applied: "
                    msg += ", ".join(f"{qid}: {fname}" for qid, fname in configs.items())  # noqa
                    self.actualGainConfiguration = msg
                    self.log.INFO(msg)
                    self.gainConfigurationState = State.ERROR.value
                else:
                    # Get description from filenames
                    qid, ppt = list(self.ppts.items())[0]

                    # The PPT has its quadrantId in the filename, whereas
                    # we store generic formattable filenames.
                    # It is important to look at the full path as
                    # configurations may have the same name, although some
                    # details differ (eg. reset length)
                    filenamePath = ppt.fullConfigFileName.value
                    filenamePath = filenamePath.replace(qid, '{}')

                    row = self.availableGainConfigurations.where_value(
                        'filenamePath',
                        filenamePath
                    )

                    if row:  # This configuration has a friendly description
                        desc, *_ = row[0]
                    else:
                        desc = fnames[0]
                        self.log.INFO(f"Strange: configuration {desc} is correct, but not know to me")  # noqa

                    self.actualGainConfiguration = desc.value
                    self.gainConfigurationState = State.ON.value

            configs = [ppt.fullConfigFileName for ppt in self.ppts.values()]
            try:
                # whichever comes first
                await wait_for(waitUntilNew(*configs), 5)
            except TimeoutError:
                pass

    @Slot(
        displayedName="Apply",
        description="Apply the selected config by restarting the PPTs",
        allowedStates={State.ACTIVE, State.INIT},
    )
    async def apply(self):
        background(self._apply)

    async def _apply(self):
        self.state = State.CHANGING
        self.status = f"Applying {self.targetGainConfiguration}"

        dids = {}
        for row in self.pptDevices.value:
            did, qid, use = row
            if use:
                dids[qid] = did

        # Validate that no PPT is initializing.
        if any(px.state == State.INIT for px in self.ppts.values()):
            self.status = "PPTs still initializing, try later."
            return

        # Check for proxies that are alive, as some might be created already
        # but may have gone down meanwhile.
        instantiated_ppts = {
            qid: did for qid, did in dids.items() if did in getDevices()
        }

        # Shutdown PPTs, if any.
        coros = {
            qid: wait_for(shutdown(did), 10)  # delay in case ppt acquiring
            for qid, did in instantiated_ppts.items()
        }
        if coros:
            done, _, errors = await allCompleted(**coros)
            if errors:
                msg = "Could not shutdown "
                msg += ", ".join(errors.keys())
                self.status = msg

                msg += f" {errors}"
                self.log.INFO(msg)

                self.state = State.ERROR
                return

            await sleep(1.5)  # Ensure that the PPTs have time to shut down.

        # Using the ConfigurationManager, instantiate the PPTs
        coros = {
            qid: instantiateFromName(did, name="default")
            for qid, did in dids.items()
        }
        done, _, errors = await allCompleted(**coros)

        if errors:
            msg = "Could not restart "
            msg += ", ".join(errors.keys())
            self.status = msg

            msg += f" {errors}"
            self.log.INFO(msg)

            self.state = State.ERROR
            return

        self.status = "PPTs restarted"
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

    @slot
    def requestConfiguration(self, quadrantId: str):
        """Provide a PPT with its configuration file, as set in this device.

        The PPT is expected to provide its quadrantId as string (eg. "Q1").
        It will be used to format the stored targetGainConfiguration (eg.
        ("GenConfigFiles_V4/Trimmed/{}/GenConf_TG4.2437_nG12_trimmed.conf")
        """
        (desc, fname), = self.availableGainConfigurations.where_value(
                    'description',
                    self.targetGainConfiguration
                )
        fname = fname.value

        # The path in config_filename may have several blanks to fill.
        # Count the blanks and repeatedly fill that amount with quadrantId
        blanks = fname.count("{}")
        config_filename = fname.format(*(blanks * [quadrantId]))

        if (config_filename == fname) and (quadrantId not in fname):
            # The stored target config is not an expected f-string, and
            # it's not for this quadrant; try fixing it
            new = config_filename.replace("Q1", quadrantId)
            new = new.replace("Q2", quadrantId)
            new = new.replace("Q3", quadrantId)
            new = new.replace("Q4", quadrantId)
            # We did our best, let the PPT validate for itself
            self.log.INFO(
                f"Hey, got given {config_filename}, tried to fix it to {new}"
            )
            config_filename = new

        return Hash(
            "type", "fullConfigFileName",  # validate which property is sent
            "origin", self.deviceId,  # where from, for debugging
            "data", config_filename,  # the requested information
            "human", self.targetGainConfiguration,  # desc., for debugging
        )

    async def onDestruction(self):
        # Highlight on scenes that this device is down.
        self.gainConfigurationState = State.ERROR.value
        await sleep(0.1)  # Let event loop process update.
