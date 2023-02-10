from karabo.common.scenemodel.api import (
    CheckBoxModel,
    ComboBoxModel,
    DisplayCommandModel,
    DisplayStateColorModel,
    DisplayTextLogModel,
    IntLineEditModel,
    LabelModel,
    SceneModel,
    SceneTargetWindow,
    write_scene,
)


def take_data_scene(device_id):
    scene0 = ComboBoxModel(
        height=31.0,
        keys=[f"{device_id}.takeData.unit"],
        klass="EditableComboBox",
        parent_component="EditableApplyLaterComponent",
        width=101.0,
        x=140.0,
        y=50.0,
    )
    scene1 = IntLineEditModel(
        height=31.0,
        keys=[f"{device_id}.takeData.quantity"],
        parent_component="EditableApplyLaterComponent",
        width=44.0,
        x=90.0,
        y=50.0,
    )
    scene2 = DisplayCommandModel(
        height=41.0,
        keys=[f"{device_id}.takeData.takeData"],
        parent_component="DisplayComponent",
        width=221.0,
        x=20.0,
        y=140.0,
    )
    scene3 = DisplayStateColorModel(
        font_size=10,
        height=191.0,
        keys=[f"{device_id}.state"],
        parent_component="DisplayComponent",
        width=451.0,
        x=270.0,
        y=20.0,
    )
    scene4 = DisplayTextLogModel(
        height=243.0,
        keys=[f"{device_id}.status"],
        parent_component="DisplayComponent",
        width=471.0,
        x=260.0,
        y=10.0,
    )
    scene5 = LabelModel(
        font="Source Sans Pro,11,-1,5,50,0,0,0,0,0",
        height=31.0,
        parent_component="DisplayComponent",
        text="Acquire for",
        width=81.0,
        x=20.0,
        y=50.0,
    )
    scene6 = LabelModel(
        font="Source Sans Pro,11,-1,5,50,0,0,0,0,0",
        foreground="#000000",
        height=21.0,
        parent_component="DisplayComponent",
        text="With runs",
        width=70.0,
        x=20.0,
        y=100.0,
    )
    scene7 = CheckBoxModel(
        height=21.0,
        keys=[f"{device_id}.takeData.daqInTheLoop"],
        klass="EditableCheckBox",
        parent_component="EditableApplyLaterComponent",
        width=25.0,
        x=96.0,
        y=100.0,
    )
    scene = SceneModel(
        height=252.0,
        width=733.0,
        children=[scene0, scene1, scene2, scene3, scene4, scene5, scene6, scene7],
    )
    return write_scene(scene)
