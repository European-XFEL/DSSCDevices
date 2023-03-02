from karabo.common.scenemodel.api import (
    CheckBoxModel,
    ColorBoolModel,
    DisplayCommandModel,
    FixedLayoutModel,
    LabelModel,
    LineModel,
    SceneModel,
    SceneTargetWindow,
    write_scene,
)


def get_scene(
    this_device_id: str,
    ppt_q1_id: str,
    ppt_q2_id: str,
    ppt_q3_id: str,
    ppt_q4_id: str
) -> str:
    """Create a default scene for the DSSC ASIC Reset device."""

    scene00 = DisplayCommandModel(
        height=36.0,
        keys=[f"{this_device_id}.asic0"],
        parent_component="DisplayComponent",
        width=38.0,
        x=340.0,
        y=489.0,
    )
    scene01 = DisplayCommandModel(
        height=36.0,
        keys=[f"{this_device_id}.asic1"],
        parent_component="DisplayComponent",
        width=37.0,
        x=379.0,
        y=489.0,
    )
    scene02 = DisplayCommandModel(
        height=36.0,
        keys=[f"{this_device_id}.asic2"],
        parent_component="DisplayComponent",
        width=38.0,
        x=417.0,
        y=489.0,
    )
    scene03 = DisplayCommandModel(
        height=36.0,
        keys=[f"{this_device_id}.asic3"],
        parent_component="DisplayComponent",
        width=38.0,
        x=456.0,
        y=489.0,
    )
    scene04 = DisplayCommandModel(
        height=36.0,
        keys=[f"{this_device_id}.asic4"],
        parent_component="DisplayComponent",
        width=38.0,
        x=494.0,
        y=489.0,
    )
    scene05 = DisplayCommandModel(
        height=36.0,
        keys=[f"{this_device_id}.asic5"],
        parent_component="DisplayComponent",
        width=38.0,
        x=533.0,
        y=489.0,
    )
    scene06 = DisplayCommandModel(
        height=36.0,
        keys=[f"{this_device_id}.asic6"],
        parent_component="DisplayComponent",
        width=38.0,
        x=572.0,
        y=489.0,
    )
    scene07 = DisplayCommandModel(
        height=36.0,
        keys=[f"{this_device_id}.asic7"],
        parent_component="DisplayComponent",
        width=39.0,
        x=611.0,
        y=489.0,
    )
    scene08 = DisplayCommandModel(
        height=37.0,
        keys=[f"{this_device_id}.asic8"],
        parent_component="DisplayComponent",
        width=37.0,
        x=341.0,
        y=524.0,
    )
    scene09 = DisplayCommandModel(
        height=37.0,
        keys=[f"{this_device_id}.asic10"],
        parent_component="DisplayComponent",
        width=38.0,
        x=417.0,
        y=523.0,
    )
    scene010 = DisplayCommandModel(
        height=38.0,
        keys=[f"{this_device_id}.asic9"],
        parent_component="DisplayComponent",
        width=37.0,
        x=379.0,
        y=523.0,
    )
    scene011 = DisplayCommandModel(
        height=37.0,
        keys=[f"{this_device_id}.asic11"],
        parent_component="DisplayComponent",
        width=39.0,
        x=456.0,
        y=523.0,
    )
    scene012 = DisplayCommandModel(
        height=37.0,
        keys=[f"{this_device_id}.asic12"],
        parent_component="DisplayComponent",
        width=37.0,
        x=495.0,
        y=523.0,
    )
    scene013 = DisplayCommandModel(
        height=37.0,
        keys=[f"{this_device_id}.asic13"],
        parent_component="DisplayComponent",
        width=37.0,
        x=533.0,
        y=523.0,
    )
    scene014 = DisplayCommandModel(
        height=37.0,
        keys=[f"{this_device_id}.asic14"],
        parent_component="DisplayComponent",
        width=39.0,
        x=572.0,
        y=523.0,
    )
    scene015 = DisplayCommandModel(
        height=37.0,
        keys=[f"{this_device_id}.asic15"],
        parent_component="DisplayComponent",
        width=39.0,
        x=611.0,
        y=523.0,
    )
    scene016 = CheckBoxModel(
        height=21.0,
        keys=[f"{this_device_id}.boolASIC0"],
        parent_component="DisplayComponent",
        width=21.0,
        x=349.0,
        y=470.0,
    )
    scene017 = CheckBoxModel(
        height=21.0,
        keys=[f"{this_device_id}.boolASIC1"],
        parent_component="DisplayComponent",
        width=21.0,
        x=387.0,
        y=470.0,
    )
    scene018 = CheckBoxModel(
        height=21.0,
        keys=[f"{this_device_id}.boolASIC2"],
        parent_component="DisplayComponent",
        width=21.0,
        x=426.0,
        y=470.0,
    )
    scene019 = CheckBoxModel(
        height=21.0,
        keys=[f"{this_device_id}.boolASIC3"],
        parent_component="DisplayComponent",
        width=21.0,
        x=465.0,
        y=470.0,
    )
    scene020 = CheckBoxModel(
        height=21.0,
        keys=[f"{this_device_id}.boolASIC4"],
        parent_component="DisplayComponent",
        width=21.0,
        x=503.0,
        y=470.0,
    )
    scene021 = CheckBoxModel(
        height=21.0,
        keys=[f"{this_device_id}.boolASIC5"],
        parent_component="DisplayComponent",
        width=21.0,
        x=542.0,
        y=470.0,
    )
    scene022 = CheckBoxModel(
        height=21.0,
        keys=[f"{this_device_id}.boolASIC6"],
        parent_component="DisplayComponent",
        width=21.0,
        x=581.0,
        y=470.0,
    )
    scene023 = CheckBoxModel(
        height=21.0,
        keys=[f"{this_device_id}.boolASIC7"],
        parent_component="DisplayComponent",
        width=21.0,
        x=620.0,
        y=470.0,
    )
    scene024 = CheckBoxModel(
        height=21.0,
        keys=[f"{this_device_id}.boolASIC8"],
        parent_component="DisplayComponent",
        width=21.0,
        x=350.0,
        y=559.0,
    )
    scene025 = CheckBoxModel(
        height=21.0,
        keys=[f"{this_device_id}.boolASIC9"],
        parent_component="DisplayComponent",
        width=21.0,
        x=387.0,
        y=559.0,
    )
    scene026 = CheckBoxModel(
        height=21.0,
        keys=[f"{this_device_id}.boolASIC10"],
        parent_component="DisplayComponent",
        width=21.0,
        x=425.0,
        y=558.0,
    )
    scene027 = CheckBoxModel(
        height=21.0,
        keys=[f"{this_device_id}.boolASIC11"],
        parent_component="DisplayComponent",
        width=21.0,
        x=465.0,
        y=558.0,
    )
    scene028 = CheckBoxModel(
        height=21.0,
        keys=[f"{this_device_id}.boolASIC12"],
        parent_component="DisplayComponent",
        width=21.0,
        x=503.0,
        y=558.0,
    )
    scene029 = CheckBoxModel(
        height=21.0,
        keys=[f"{this_device_id}.boolASIC13"],
        parent_component="DisplayComponent",
        width=21.0,
        x=541.0,
        y=558.0,
    )
    scene030 = CheckBoxModel(
        height=21.0,
        keys=[f"{this_device_id}.boolASIC14"],
        parent_component="DisplayComponent",
        width=21.0,
        x=581.0,
        y=558.0,
    )
    scene031 = CheckBoxModel(
        height=21.0,
        keys=[f"{this_device_id}.boolASIC15"],
        parent_component="DisplayComponent",
        width=21.0,
        x=619.0,
        y=558.0,
    )
    scene0 = FixedLayoutModel(
        height=109.0,
        width=309.0,
        x=340.0,
        y=470.0,
        children=[
            scene00,
            scene01,
            scene02,
            scene03,
            scene04,
            scene05,
            scene06,
            scene07,
            scene08,
            scene09,
            scene010,
            scene011,
            scene012,
            scene013,
            scene014,
            scene015,
            scene016,
            scene017,
            scene018,
            scene019,
            scene020,
            scene021,
            scene022,
            scene023,
            scene024,
            scene025,
            scene026,
            scene027,
            scene028,
            scene029,
            scene030,
            scene031,
        ],
    )
    scene1 = LineModel(
        stroke="#000000", x=350.0, x1=350.0, x2=640.0, y=240.0, y1=240.0, y2=240.0
    )
    scene2 = LineModel(
        stroke="#000000", x=499.0, x1=499.0, x2=499.0, y=50.0, y1=50.0, y2=430.0
    )
    scene3 = DisplayCommandModel(
        height=31.0,
        keys=[f"{ppt_q4_id}.checkASICReset"],
        parent_component="DisplayComponent",
        width=141.0,
        x=350.0,
        y=40.0,
    )
    scene4 = DisplayCommandModel(
        height=31.0,
        keys=[f"{ppt_q1_id}.checkASICReset"],
        parent_component="DisplayComponent",
        width=141.0,
        x=510.0,
        y=40.0,
    )
    scene5 = ColorBoolModel(
        height=31.0,
        keys=[f"{this_device_id}.boolQ1M4"],
        parent_component="DisplayComponent",
        width=31.0,
        x=620.0,
        y=80.0,
    )
    scene6 = ColorBoolModel(
        height=31.0,
        keys=[f"{this_device_id}.boolQ1M3"],
        parent_component="DisplayComponent",
        width=31.0,
        x=620.0,
        y=120.0,
    )
    scene7 = ColorBoolModel(
        height=31.0,
        keys=[f"{this_device_id}.boolQ1M2"],
        parent_component="DisplayComponent",
        width=31.0,
        x=620.0,
        y=160.0,
    )
    scene8 = ColorBoolModel(
        height=31.0,
        keys=[f"{this_device_id}.boolQ1M1"],
        parent_component="DisplayComponent",
        width=31.0,
        x=620.0,
        y=200.0,
    )
    scene9 = DisplayCommandModel(
        height=31.0,
        keys=[f"{this_device_id}.activateQ1M4"],
        parent_component="DisplayComponent",
        width=103.0,
        x=510.0,
        y=80.0,
    )
    scene10 = DisplayCommandModel(
        height=31.0,
        keys=[f"{this_device_id}.activateQ1M3"],
        parent_component="DisplayComponent",
        width=103.0,
        x=510.0,
        y=120.0,
    )
    scene11 = DisplayCommandModel(
        height=31.0,
        keys=[f"{this_device_id}.activateQ1M2"],
        parent_component="DisplayComponent",
        width=103.0,
        x=510.0,
        y=160.0,
    )
    scene12 = DisplayCommandModel(
        height=30.0,
        keys=[f"{this_device_id}.activateQ1M1"],
        parent_component="DisplayComponent",
        width=103.0,
        x=510.0,
        y=201.0,
    )
    scene13 = DisplayCommandModel(
        height=31.0,
        keys=[f"{this_device_id}.activateQ4M1"],
        parent_component="DisplayComponent",
        width=99.0,
        x=390.0,
        y=80.0,
    )
    scene14 = DisplayCommandModel(
        height=31.0,
        keys=[f"{this_device_id}.activateQ4M2"],
        parent_component="DisplayComponent",
        width=99.0,
        x=390.0,
        y=120.0,
    )
    scene15 = DisplayCommandModel(
        height=31.0,
        keys=[f"{this_device_id}.activateQ4M3"],
        parent_component="DisplayComponent",
        width=99.0,
        x=390.0,
        y=160.0,
    )
    scene16 = DisplayCommandModel(
        height=31.0,
        keys=[f"{this_device_id}.activateQ4M4"],
        parent_component="DisplayComponent",
        width=101.0,
        x=390.0,
        y=200.0,
    )
    scene17 = DisplayCommandModel(
        height=31.0,
        keys=[f"{ppt_q3_id}.checkASICReset"],
        parent_component="DisplayComponent",
        width=141.0,
        x=350.0,
        y=410.0,
    )
    scene18 = DisplayCommandModel(
        height=31.0,
        keys=[f"{this_device_id}.activateQ3M1"],
        parent_component="DisplayComponent",
        width=101.0,
        x=390.0,
        y=250.0,
    )
    scene19 = DisplayCommandModel(
        height=31.0,
        keys=[f"{this_device_id}.activateQ3M2"],
        parent_component="DisplayComponent",
        width=101.0,
        x=390.0,
        y=290.0,
    )
    scene20 = DisplayCommandModel(
        height=31.0,
        keys=[f"{this_device_id}.activateQ3M3"],
        parent_component="DisplayComponent",
        width=101.0,
        x=390.0,
        y=330.0,
    )
    scene21 = DisplayCommandModel(
        height=31.0,
        keys=[f"{this_device_id}.activateQ3M4"],
        parent_component="DisplayComponent",
        width=101.0,
        x=390.0,
        y=370.0,
    )
    scene22 = DisplayCommandModel(
        height=31.0,
        keys=[f"{ppt_q2_id}.checkASICReset"],
        parent_component="DisplayComponent",
        width=141.0,
        x=510.0,
        y=410.0,
    )
    scene23 = DisplayCommandModel(
        height=31.0,
        keys=[f"{this_device_id}.activateQ2M4"],
        parent_component="DisplayComponent",
        width=101.0,
        x=510.0,
        y=250.0,
    )
    scene24 = DisplayCommandModel(
        height=30.0,
        keys=[f"{this_device_id}.activateQ2M3"],
        parent_component="DisplayComponent",
        width=103.0,
        x=510.0,
        y=290.0,
    )
    scene25 = DisplayCommandModel(
        height=31.0,
        keys=[f"{this_device_id}.activateQ2M2"],
        parent_component="DisplayComponent",
        width=103.0,
        x=510.0,
        y=330.0,
    )
    scene26 = DisplayCommandModel(
        height=31.0,
        keys=[f"{this_device_id}.activateQ2M1"],
        parent_component="DisplayComponent",
        width=103.0,
        x=510.0,
        y=370.0,
    )
    scene27 = ColorBoolModel(
        height=31.0,
        keys=[f"{this_device_id}.boolQ2M1"],
        parent_component="DisplayComponent",
        width=31.0,
        x=620.0,
        y=370.0,
    )
    scene28 = ColorBoolModel(
        height=31.0,
        keys=[f"{this_device_id}.boolQ2M3"],
        parent_component="DisplayComponent",
        width=31.0,
        x=620.0,
        y=290.0,
    )
    scene29 = ColorBoolModel(
        height=31.0,
        keys=[f"{this_device_id}.boolQ2M4"],
        parent_component="DisplayComponent",
        width=31.0,
        x=620.0,
        y=250.0,
    )
    scene30 = ColorBoolModel(
        height=31.0,
        keys=[f"{this_device_id}.boolQ2M2"],
        parent_component="DisplayComponent",
        width=31.0,
        x=620.0,
        y=330.0,
    )
    scene31 = ColorBoolModel(
        height=31.0,
        keys=[f"{this_device_id}.boolQ4M1"],
        parent_component="DisplayComponent",
        width=31.0,
        x=350.0,
        y=80.0,
    )
    scene32 = ColorBoolModel(
        height=31.0,
        keys=[f"{this_device_id}.boolQ4M2"],
        parent_component="DisplayComponent",
        width=31.0,
        x=350.0,
        y=120.0,
    )
    scene33 = ColorBoolModel(
        height=31.0,
        keys=[f"{this_device_id}.boolQ4M3"],
        parent_component="DisplayComponent",
        width=31.0,
        x=350.0,
        y=160.0,
    )
    scene34 = ColorBoolModel(
        height=31.0,
        keys=[f"{this_device_id}.boolQ4M4"],
        parent_component="DisplayComponent",
        width=31.0,
        x=350.0,
        y=200.0,
    )
    scene35 = ColorBoolModel(
        height=31.0,
        keys=[f"{this_device_id}.boolQ3M4"],
        parent_component="DisplayComponent",
        width=31.0,
        x=350.0,
        y=370.0,
    )
    scene36 = ColorBoolModel(
        height=31.0,
        keys=[f"{this_device_id}.boolQ3M3"],
        parent_component="DisplayComponent",
        width=31.0,
        x=350.0,
        y=330.0,
    )
    scene37 = ColorBoolModel(
        height=31.0,
        keys=[f"{this_device_id}.boolQ3M1"],
        parent_component="DisplayComponent",
        width=31.0,
        x=350.0,
        y=250.0,
    )
    scene38 = ColorBoolModel(
        height=31.0,
        keys=[f"{this_device_id}.boolQ3M2"],
        parent_component="DisplayComponent",
        width=31.0,
        x=350.0,
        y=290.0,
    )
    label = LabelModel(
        font="Source Sans Pro,10,-1,5,50,0,0,0,0,0",
        height=181.0,
        parent_component="DisplayComponent",
        text="Select Module, then select ASIC.<br/> Click <b><i>Check ASIC Reset</i></b> a number of times.<br/>Follow the PPT logs to see whether the ASIC is responding.",
        width=871.0,
        x=10.0,
        y=10.0,
    )
    scene = SceneModel(
        children=[
            label,
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
            scene10,
            scene11,
            scene12,
            scene13,
            scene14,
            scene15,
            scene16,
            scene17,
            scene18,
            scene19,
            scene20,
            scene21,
            scene22,
            scene23,
            scene24,
            scene25,
            scene26,
            scene27,
            scene28,
            scene29,
            scene30,
            scene31,
            scene32,
            scene33,
            scene34,
            scene35,
            scene36,
            scene37,
            scene38,
        ]
    )
    return write_scene(scene)
