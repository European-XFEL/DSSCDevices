from karabo.common.scenemodel.api import (
    DisplayCommandModel,
    DisplayLabelModel,
    DisplayStateColorModel,
    DisplayTextLogModel,
    EditableComboBoxModel,
    LabelModel,
    SceneModel,
    SceneTargetWindow,
    write_scene,
)


def get_scene(device_id):
    scene0 = LabelModel(
        font="Source Sans Pro,22,-1,5,50,0,0,0,0,0",
        foreground="#000000",
        height=41.0,
        parent_component="DisplayComponent",
        text="Current Config.",
        width=221.0,
        x=10.0,
        y=160.0,
    )
    scene1 = DisplayStateColorModel(
        font_size=10,
        height=41.0,
        keys=[f"{device_id}.gainConfigurationState"],
        parent_component="DisplayComponent",
        width=1161.0,
        x=230.0,
        y=160.0,
    )
    scene2 = DisplayLabelModel(
        font_size=16,
        font_weight="bold",
        height=41.0,
        keys=[f"{device_id}.actualGainConfiguration"],
        parent_component="DisplayComponent",
        width=1161.0,
        x=230.0,
        y=160.0,
    )
    scene3 = DisplayCommandModel(
        font_size=10,
        height=81.0,
        keys=[f"{device_id}.apply"],
        parent_component="DisplayComponent",
        width=111.0,
        x=640.0,
        y=20.0,
    )
    scene4 = LabelModel(
        font="Source Sans Pro,22,-1,5,50,0,0,0,0,0",
        foreground="#000000",
        height=41.0,
        parent_component="DisplayComponent",
        text="Desired Config.",
        width=201.0,
        x=10.0,
        y=20.0,
    )
    scene5 = DisplayLabelModel(
        font_size=10,
        height=41.0,
        keys=[f"{device_id}.targetGainConfiguration"],
        parent_component="DisplayComponent",
        width=391.0,
        x=220.0,
        y=20.0,
    )
    scene6 = EditableComboBoxModel(
        height=41.0,
        keys=[f"{device_id}.targetGainConfiguration"],
        parent_component="EditableApplyLaterComponent",
        width=391.0,
        x=220.0,
        y=80.0,
    )
    scene7 = LabelModel(
        font="Source Sans Pro,10,-1,5,50,0,0,0,0,0",
        foreground="#000000",
        height=31.0,
        parent_component="DisplayComponent",
        text="Monitored Quadrants",
        width=131.0,
        x=820.0,
        y=10.0,
    )
    scene8 = DisplayLabelModel(
        font_size=10,
        height=31.0,
        keys=[f"{device_id}.monitoredDevices"],
        parent_component="DisplayComponent",
        width=121.0,
        x=820.0,
        y=40.0,
    )
    scene9 = DisplayTextLogModel(
        height=161.0,
        keys=[f"{device_id}.status"],
        parent_component="DisplayComponent",
        width=431.0,
        x=970.0,
    )
    scene = SceneModel(
        height=211.0,
        width=1399.0,
        children=[
            scene0,
            scene1,
            scene2,
            scene3,
            scene4,
            scene5,
            scene6,
            scene7,
            scene8,
            scene9,
        ],
    )
    return write_scene(scene)
