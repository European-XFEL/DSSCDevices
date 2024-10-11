from karabo.common.scenemodel.api import (
    BoxLayoutModel,
    DisplayLabelModel,
    DisplayStateColorModel,
    DisplayTextLogModel,
    LabelModel,
    RectangleModel,
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

    did = LabelModel(
        font='Source Sans Pro,24,-1,5,99,0,0,0,0,0', 
        height=61,
        parent_component='DisplayComponent',
        text='NOT VALIDATED YET',
        width=291,
        x=550,
        y=540,
    )

    asics_label = LabelModel(
        font='Source Sans Pro,24,-1,5,75,0,0,0,0,0',
        height=28,
        parent_component='DisplayComponent',
        text='ASICS STATES',
        width=201,
        x=180,
        y=740
    )

    asic0 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic0'], parent_component='DisplayComponent', width=31, x=103, y=820)
    asic1 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic1'], parent_component='DisplayComponent', width=31, x=143, y=820)
    asic2 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic2'], parent_component='DisplayComponent', width=31, x=183, y=820)
    asic3 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic3'], parent_component='DisplayComponent', width=31, x=223, y=820)
    asic4 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic4'], parent_component='DisplayComponent', width=31, x=273, y=820)
    asic5 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic5'], parent_component='DisplayComponent', width=31, x=313, y=820)
    asic6 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic6'], parent_component='DisplayComponent', width=31, x=353, y=820)
    asic7 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic7'], parent_component='DisplayComponent', width=31, x=393, y=820)
    asic8 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic8'], parent_component='DisplayComponent', width=31, x=103, y=790)
    asic9 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic9'], parent_component='DisplayComponent', width=31, x=143, y=790)
    asic10 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic10'], parent_component='DisplayComponent', width=31, x=183, y=790)
    asic11 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic11'], parent_component='DisplayComponent', width=31, x=223, y=790)
    asic12 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic12'], parent_component='DisplayComponent', width=31, x=273, y=790)
    asic13 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic13'], parent_component='DisplayComponent', width=31, x=313, y=790)
    asic14 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic14'], parent_component='DisplayComponent', width=31, x=353, y=790)
    asic15 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic15'], parent_component='DisplayComponent', width=31, x=393, y=790)
    asicsbox1 = RectangleModel(height=71, stroke='#000000', stroke_width=3.0, width=174, x=90, y=780)
    asicsbox2 = RectangleModel(height=71, stroke='#000000', stroke_width=3.0, width=174, x=263, y=780)

    scene = SceneModel(
        height=882,
        width=909,
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
            asics_label,
            asic0,
            asic1,
            asic2,
            asic3,
            asic4,
            asic5,
            asic6,
            asic7,
            asic8,
            asic9,
            asic10,
            asic11,
            asic12,
            asic13,
            asic14,
            asic15,
            asicsbox1,
            asicsbox2,
        ]
    )
    return write_scene(scene)
