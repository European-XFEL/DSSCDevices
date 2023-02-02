#############################################################################
# Author: samartse
# Created on November, 2019, 07:15 PM
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import asyncio
import datetime
import os
import time
from asyncio import CancelledError, wait_for
from collections import ChainMap

from karabo.middlelayer import (
    AccessMode, Configurable, Device, Hash, Injectable, Node, Slot, State,
    String, UInt32, VectorHash, allCompleted, background, connectDevice,
    gather, setWait, sleep, waitUntilNew
)

from ._version import version as deviceVersion
from .schemata import ConfigPptRowSchema, PptRowSchema


class DsscConfigSetter(Device, Injectable):
    """class for fast setting of configuration files on all DsscPpt karabo devices"""
    # provide version for classVersion property
    __version__ = deviceVersion

    activityCount = UInt32(
        displayedName="Activity count",
        description="Counts waits for events and timeout"
                    " to show that device is rolling"
    )

    ppt_devices = {}
    ppt_dev_indx = {}
    ppt_dev = [] # short list of active ppt devices


    currentConfiguration = ()

    pptConfig = VectorHash(
        rows=PptRowSchema,
        displayedName="Ppt devices to connect",
        defaultValue=[Hash([('deviceId', f"SCS_CDIDET_DSSC/FPGA/PPT_Q{q}"),
                            ('quadrantId', f"Q{q}"), ('connectIt', True)]) for q in range(1, 5)])

    pptConfigFiles = VectorHash(
        rows=ConfigPptRowSchema,
        displayedName="Ppt Configurations",
        defaultValue=[Hash()])

    lastConfig = String(
        displayedName="Last set configuration",
        defaultValue="")

    gainConfiguration = String(
        displayedName="Detector gain",
        defaultValue="")

    async def onInitialization(self):
        """ This method will be called when the device starts.

            Define your actions to be executed after instantiation.
        """
        self.activityCount = 0

        self.tableConfigMapper = {}
        self.updateTableInfo()
        await self.injectConfigSlots()
        await self.connectDevices()
        background(self.pptDevicesGainSetMonitor())


    @Slot(displayedName="Update",
          description="Update device info",
          allowedStates={State.UNKNOWN, State.OFF, State.ACQUIRING, State.ON})
    async def connectDevices(self):

        self.state = State.CHANGING

        to_connect = {}
        for i, row in enumerate(self.pptConfig.value):
            if row[2]:
                to_connect[str(i)] = connectDevice(row[0])

        done, pending, error = await allCompleted(**to_connect, timeout=10)
        error = ChainMap(pending, error)

        if len(error):
            failmsg = [r[0] for i, r in enumerate(self.pptConfig.value)
                       if str(i) in error]
            self.status = "Failed connecting PPT: {}".format(failmsg)
            self.state = State.UNKOWN
            return


        for i, row in enumerate(self.pptConfig.value):
            if str(i) in done.keys():
                self.ppt_devices[row[1]] = done[str(i)]
                self.ppt_dev_indx[done[str(i)]] = i
            else:
                self.ppt_devices[row[1]] = None

        self.ppt_dev = list(done.values())

        self.connectedPptDev = ", ".join(["Q{}".format(str(i + 1)) for i, d
                                          in enumerate(self.ppt_devices) if d])

        print("ppt_devices")
        for key, val in self.ppt_devices.items():
            if val:
                print("key: {}, val: {}".format(key, val.deviceId))
            else:
                print("key: {}, val: {}".format(key, 'None'))
        print("len(ppt_dev): ", len(self.ppt_dev))

        self.updateTableInfo()
        await self.injectConfigSlots()

#        self.state = State.ON

    def updateTableInfo(self):
        for q in range(1, 5):
            quad = "Q{}".format(q)
            self.tableConfigMapper[quad] = {}
            for i, row in enumerate(self.pptConfigFiles.value):
                self.tableConfigMapper[quad][row[q]] = i


    async def injectConfigSlots(self):

        def setConfig(indx):
            @Slot(displayedName=self.pptConfigFiles.value[indx][0],
                  description="Configuration: {}".format(self.pptConfigFiles.value[i][0]),
                  allowedStates={State.ON})
            async def setConfiguration(self):
                self.status = "Setting configuration: {}".format(self.pptConfigFiles.value[i][0])
                self.currentConfiguration = (indx, self.pptConfigFiles.value[indx][0])
                await self.setConfigurationFromIndx()

            return setConfiguration

        for i, row in enumerate(self.pptConfigFiles.value):
            setattr(self.__class__, f"btn{i}", setConfig(i))

        await self.publishInjectedParameters()

    async def setConfigurationFromIndx(self):
        for dev in self.ppt_dev:
            if dev.state != State.ON:
                self.status = "DsscPpt device {} is not ready (must be in ON state)".format(self.ppt_dev_indx[dev])
                self.log.INFO(self.status)
                return

 #       self.state = State.CHANGING
        self.log.INFO(self.status)

        futures2wait = []
        for ppt_device in self.ppt_dev:
            devIndx = self.ppt_dev_indx[ppt_device]
            futures2wait.append(setWait(ppt_device, fullConfigFileName = self.pptConfigFiles.value[self.currentConfiguration[0]][devIndx+1]))
        await asyncio.gather(*futures2wait)
        self.lastConfig = self.currentConfiguration[1]

        
        futures2wait = []
        for ppt_device in self.ppt_dev:
            futures2wait.append(execute(ppt_device, "initSystem"))
        await asyncio.gather(*futures2wait)

#        self.state =State.ON
        self.status = "Configuration {} set".format(self.pptConfigFiles.value[self.currentConfiguration[0]][0])

    async def pptDevicesGainSetMonitor(self):

        while True:
            self.activityCount += 1

            #state_was = [dev.fullConfigFileName for dev in self.ppt_dev]
            state_was = [dev.state for dev in self.ppt_dev]

            confIndex = {}
            for quad, dev in self.ppt_devices.items():
                if dev:
                    if dev.fullConfigFileName in self.tableConfigMapper[quad].keys() and dev.state != State.UNKNOWN:
                        confIndex[quad] = self.tableConfigMapper[quad][dev.fullConfigFileName]
                    else:
                        confIndex[quad] = -1

            confFileNamesIndxs = list(confIndex.values())

            if confFileNamesIndxs.count(confFileNamesIndxs[0]) == len(confFileNamesIndxs):
                if confFileNamesIndxs[0] >=0:
                    self.gainConfiguration = "Gain: {}".format(self.pptConfigFiles.value[confFileNamesIndxs[0]][0])
                else:
                    self.gainConfiguration = "Gain setting unknown"
            else:
                gainstr = ""
                for quad, indx in confIndex.items():
                    if indx >=0:
                        gainstr += "{} : {},  ".format(quad, self.pptConfigFiles.value[indx][0])
                    else:
                        gainstr += "{} : {},  ".format(quad, 'Unknown')

                self.gainConfiguration = 'Different gain gettings, ' + gainstr

            if state_was.count(State.CHANGING) > 0:
                self.state = State.CHANGING
            elif state_was.count(state_was[0]) == len(state_was):
                self.state = state_was[0]
                self.status = str(self.state)
            else:
                self.state = State.STOPPED
                self.status = "PPT's have different states, set the devices to equal state"	

            await waitUntilNew(*state_was)




