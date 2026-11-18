import numpy as np
from karabo.middlelayer import (AccessMode, Bool, Configurable, DaqDataType, Int32,
                                Node, String, VectorInt32, VectorUInt8, UInt32, UInt64)


class MeasSettingsNode(Configurable):
    settings = VectorInt32(displayedName="settings")
    numSettings = UInt32(displayedName="# Settings")
    settingName = VectorUInt8(displayedName="Setting name")


class MeasurementSettings(Configurable):
    setting1 = Node(MeasSettingsNode)
    setting2 = Node(MeasSettingsNode)


class QuadrantMeasurementTrainId(Configurable):
    startTrainId = UInt64(displayedName="startTrainId")
    endTrainId = UInt64(displayedName="endTrainId")

class ConfigFilePath(Configurable):
    savedConfigPath_Q1 = VectorUInt8(displayedName="saved config path for Q1")  # string
    savedConfigPath_Q2 = VectorUInt8(displayedName="saved config path for Q2")  # string
    savedConfigPath_Q3 = VectorUInt8(displayedName="saved config path for Q3")  # string
    savedConfigPath_Q4 = VectorUInt8(displayedName="saved config path for Q4")  # string


class MeasurementInfo(Configurable):
    daqDataType = DaqDataType.TRAIN

    singleRunMeasurementType = Bool(displayedName="measurement in single DAQ run")

    measurementName = VectorUInt8(displayedName="measurement name")  # string

    measurementStartRunDirectory = VectorUInt8(
        displayedName="measurement start directory")  # string

    totalMeasurements = UInt32(displayedName="total measurements")

    numIterations = UInt32(displayedName="iterations")

    numMeasurementsInSweep = UInt32(displayedName="measurements in sweep")

    numPreBurstVetos = UInt32(displayedName="pre burst vetos")

    availableASICs = UInt64(displayedName="available_ASICs")

    connectedPPTs = VectorUInt8(displayedName="connected quadrants")  # string

    numMeasurementParams = UInt32(displayedName="measurement params")

    timeStamp = VectorUInt8(displayedName="timestamp")  # string

    measParamNames = VectorUInt8(displayedName="meas. param names")

    ladderMode = UInt32(displayedName="ladder mode")

    activeModule = UInt32(displayedName="active module")

    injType = VectorUInt8(displayedName="injection type")  # string

    injSkipType = VectorUInt8(displayedName="injection skip type")  # string

    columnSelect = VectorUInt8(displayedName="columnSelect string")  # string

    numOfParallelColumns = UInt32(displayedName="number of parallel columns in Injection")

    numOfSweeps = UInt32(displayedName="number of sweeps/scans in measurements")

    pixColumnSelect = UInt32(displayedName="section in injection sweep")

    measurementSettings = Node(MeasurementSettings,
                               displayedName="measurement settings")

    configFilePath = Node(ConfigFilePath,
                          displayedName="Quadrants configuration file paths")

    currentSetting1Val = Int32(displayedName="current value setting 1")
    currentSetting2Val = Int32(displayedName="current value setting 2")
    currentMeasurementNumber = UInt32(displayedName="current measurement number")

    trainIdDataQ1 = Node(QuadrantMeasurementTrainId,
                         displayedName="Q1MeasurementTrainId")
    trainIdDataQ2 = Node(QuadrantMeasurementTrainId,
                         displayedName="Q2MeasurementTrainId")
    trainIdDataQ3 = Node(QuadrantMeasurementTrainId,
                         displayedName="Q3MeasurementTrainId")
    trainIdDataQ4 = Node(QuadrantMeasurementTrainId,
                         displayedName="Q4MeasurementTrainId")


class BaseDeviceRowSchema(Configurable):
    deviceId = String(
        displayedName="DeviceId",
        defaultValue="")
    connectIt = Bool(
        displayedName="Connect to Device",
        defaultValue=True)


class ConfigPptRowSchema(Configurable):
    configDescription = String(
        displayedName="Config. description",
        defaultValue="")
    ppt1Dev = String(
        displayedName="ppt1ConfigFileName",
        defaultValue="")
    ppt2Dev = String(
        displayedName="ppt2ConfigFileName",
        defaultValue="")
    ppt3Dev = String(
        displayedName="ppt3ConfigFileName",
        defaultValue="")
    ppt4Dev = String(
        displayedName="ppt4ConfigFileName",
        defaultValue="")




class PptRowSchema(Configurable):
    deviceId = String(
        displayedName="DeviceId",
        defaultValue="")
    quadrantId = String(
        displayedName="Quadrant",
        defaultValue="Q1")
    connectIt = Bool(
        displayedName="Connect to Device",
        defaultValue=True)


class PptRowSchema2(Configurable):
    deviceId = String(
        displayedName="DeviceId",
        defaultValue="")
    quadrantId = String(
        displayedName="Quadrant",
        defaultValue="Q1",
        accessMode=AccessMode.READONLY)
