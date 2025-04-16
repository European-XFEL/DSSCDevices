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
        width=301.0,
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
        text='SIMULATED OUTPUT IS GARBAGE, WIP',
        width=900.0,
        x=5.0,
        y=10.0,
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
    veto0 = DisplayLabelModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.veto0'], parent_component='DisplayComponent', width=31, x=103, y=820)
    asic1 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic1'], parent_component='DisplayComponent', width=31, x=143, y=820)
    veto1 = DisplayLabelModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.veto1'], parent_component='DisplayComponent', width=31, x=143, y=820)
    asic2 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic2'], parent_component='DisplayComponent', width=31, x=183, y=820)
    veto2 = DisplayLabelModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.veto2'], parent_component='DisplayComponent', width=31, x=183, y=820)
    asic3 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic3'], parent_component='DisplayComponent', width=31, x=223, y=820)
    veto3 = DisplayLabelModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.veto3'], parent_component='DisplayComponent', width=31, x=223, y=820)
    asic4 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic4'], parent_component='DisplayComponent', width=31, x=273, y=820)
    veto4 = DisplayLabelModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.veto4'], parent_component='DisplayComponent', width=31, x=273, y=820)
    asic5 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic5'], parent_component='DisplayComponent', width=31, x=313, y=820)
    veto5 = DisplayLabelModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.veto5'], parent_component='DisplayComponent', width=31, x=313, y=820)
    asic6 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic6'], parent_component='DisplayComponent', width=31, x=353, y=820)
    veto6 = DisplayLabelModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.veto6'], parent_component='DisplayComponent', width=31, x=353, y=820)
    asic7 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic7'], parent_component='DisplayComponent', width=31, x=393, y=820)
    veto7 = DisplayLabelModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.veto7'], parent_component='DisplayComponent', width=31, x=393, y=820)
    asic8 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic8'], parent_component='DisplayComponent', width=31, x=103, y=790)
    veto8 = DisplayLabelModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.veto8'], parent_component='DisplayComponent', width=31, x=103, y=790)
    asic9 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic9'], parent_component='DisplayComponent', width=31, x=143, y=790)
    veto9 = DisplayLabelModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.veto9'], parent_component='DisplayComponent', width=31, x=143, y=790)
    asic10 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic10'], parent_component='DisplayComponent', width=31, x=183, y=790)
    veto10 = DisplayLabelModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.veto10'], parent_component='DisplayComponent', width=31, x=183, y=790)
    asic11 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic11'], parent_component='DisplayComponent', width=31, x=223, y=790)
    veto11 = DisplayLabelModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.veto11'], parent_component='DisplayComponent', width=31, x=223, y=790)
    asic12 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic12'], parent_component='DisplayComponent', width=31, x=273, y=790)
    veto12 = DisplayLabelModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.veto12'], parent_component='DisplayComponent', width=31, x=273, y=790)
    asic13 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic13'], parent_component='DisplayComponent', width=31, x=313, y=790)
    veto13 = DisplayLabelModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.veto13'], parent_component='DisplayComponent', width=31, x=313, y=790)
    asic14 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic14'], parent_component='DisplayComponent', width=31, x=353, y=790)
    veto14 = DisplayLabelModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.veto14'], parent_component='DisplayComponent', width=31, x=353, y=790)
    asic15 = DisplayStateColorModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.asic15'], parent_component='DisplayComponent', width=31, x=393, y=790)
    veto15 = DisplayLabelModel(font_size=10, font_weight='normal', height=21, keys=[f'{device_id}.asicStatus.veto15'], parent_component='DisplayComponent', width=31, x=393, y=790)
    asicsbox1 = RectangleModel(height=71, stroke='#000000', stroke_width=3.0, width=174, x=90, y=780)
    asicsbox2 = RectangleModel(height=71, stroke='#000000', stroke_width=3.0, width=174, x=263, y=780)

    ppt_veto_label = LabelModel(
        font='Source Sans Pro,24,-1,5,75,0,0,0,0,0',
        foreground='#000000',
        height=31,
        parent_component='DisplayComponent',
        text='PPT VETO',
        width=151,
        x=450,
        y=740
    )

    ppt_veto = DisplayLabelModel(
        font_size=24,
        font_weight='bold', # 'normal',
        height=71,
        keys=[f'{device_id}.asicStatus.pptVeto'],
        parent_component='DisplayComponent',
        width=141,
        x=450,
        y=780
    )

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
            veto0,
            asic1,
            veto1,
            asic2,
            veto2,
            asic3,
            veto3,
            asic4,
            veto4,
            asic5,
            veto5,
            asic6,
            veto6,
            asic7,
            veto7,
            asic8,
            veto8,
            asic9,
            veto9,
            asic10,
            veto10,
            asic11,
            veto11,
            asic12,
            veto12,
            asic13,
            veto13,
            asic14,
            veto14,
            asic15,
            veto15,
            asicsbox1,
            asicsbox2,
            ppt_veto_label,
            ppt_veto,
        ]
    )
    return write_scene(scene)
