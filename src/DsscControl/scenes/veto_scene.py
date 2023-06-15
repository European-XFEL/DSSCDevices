from karabo.common.scenemodel.api import (
    BoxLayoutModel,
    DisplayLabelModel,
    DisplayStateColorModel,
    DisplayTextLogModel,
    LabelModel,
    SceneModel,
    SceneTargetWindow,
    VectorXYGraphModel,
    write_scene,
)


def overview(device_id):
    state = DisplayStateColorModel(
        font_size=10, 
        height=311.0,
        keys=[f'{device_id}.state'],
        parent_component='DisplayComponent', 
        width=281.0,
        x=60.0,
        y=110.0,
    )

    status = DisplayTextLogModel(
        height=361.0,
        keys=[f'{device_id}.status'],
        parent_component='DisplayComponent',
        width=307.0,
        x=50.0,
        y=100.0,
    )

    notOkCount_label = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', 
        foreground='#000000',
        height=31.0,
        parent_component='DisplayComponent',
        text='Invalid Count',
        width=151.0,
        x=40.0,
        y=610.0,
    )

    notOkCount = DisplayLabelModel(
        font_size=10,
        height=31.0,
        keys=[f'{device_id}.notOkCount'],
        parent_component='DisplayComponent',
        width=150.0,
        x=191.0, 
        y=610.0,
    )

    not_ok = BoxLayoutModel(
        height=31.0,
        width=301.0,
        x=40.0,
        y=610.0,
        children=[notOkCount_label, notOkCount],
    )

    det_data_label = LabelModel(
        font='Source Sans Pro,24,-1,5,75,0,0,0,0,0',
        height=41.0,
        parent_component='DisplayComponent',
        text='DETECTOR',
        width=160.0,
        x=610.0,
        y=90.0,
    )

    det_data = VectorXYGraphModel(
        height=251.0,
        keys=[
            f'{device_id}.output.schema.data.detPulseId',
            f'{device_id}.output.schema.data.detCellId'],
        parent_component='DisplayComponent', 
        width=451.0,
        x=450.0,
        y=130.0,
    )

    sim_data_label = LabelModel(
        font='Source Sans Pro,24,-1,5,75,0,0,0,0,0',
        height=41.0,
        parent_component='DisplayComponent',
        text='SIMULATED',
        width=171.0,
        x=610.0,
        y=430.0,
    )

    sim_data = VectorXYGraphModel(
        height=251.0,
        keys=[
            f'{device_id}.output.schema.data.simPulseId',
            f'{device_id}.output.schema.data.simCellId'], 
        parent_component='DisplayComponent', 
        width=451.0,
        x=450.0,
        y=470.0,
    )

    not_ok_state = DisplayStateColorModel(
        font_size=10,
        height=131.0,
        keys=[f'{device_id}.ok'],
        parent_component='DisplayComponent',
        width=301.0,
        x=40.0,
        y=480.0,
    )

    not_ok_label = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0',
        foreground='#000000',
        height=31.0,
        parent_component='DisplayComponent',
        text='Data Ok',
        width=53.0,
        x=50.0,
        y=490.0,
    )

    did = DisplayLabelModel(
        font_size=30,
        height=51.0,
        parent_component='DisplayComponent',
        keys=[f'{device_id}.detOutput'],
        width=900.0,
        x=5.0,
        y=10.0,
    )

    scene = SceneModel(
        height=731.0,
        width=909.0,
        children=[
            state,
            status,
            not_ok,
            det_data_label,
            det_data,
            sim_data_label,
            sim_data,
            not_ok_state,
            not_ok_label,
            did,
        ]
    )
    return write_scene(scene)
