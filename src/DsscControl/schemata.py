from karabo.middlelayer import AccessMode, Bool, Configurable, String


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
