DEVICE_STATES = [State.UNKNOWN, State.STOPPED, State.OFF, State.CHANGING, State.RUNNING,
                 State.ON, State.ACQUIRING, State.ERROR, State.INIT, State.ACTIVE]

State.ACTIVE


all the states: State.INIT, State.ERROR, State.OFF, State.UNKNOWN, State.STOPPED

If it is doing operation on the PPT, disallow when powering
Disallow powering if doing operations on the PPT

Update Devices List: State.ERROR, State.OFF, State.UNKNOWN, State.STOPPED  # lock power
Start PPT Devices: State.UNKNOWN, State.OFF, State.STOPPED  # lock power
Init PPT Devices: State.ON, State.OFF  # lock power
Check ASICs/Reset On PPT Devices: State.STOPPED, State.ACTIVE  # lock power

Start Data Sending:  State.ON  # lock power
Stop Data Sending: State.ACQUIRING  # lock power
ABORT: State.ACQUIRING, State.RUNNING  # lock power

Run PixelParameter Sweep: State.ON  # lock power
Run Injection Sweep: State.ON  # lock power
Run BurstParameter Sweep: State.ON  # lock power
Run Burst Acquisition: State.ON, State.ACQUIRING  # lock power
Send measurementInfo Data:  # neutral

1 ASICS On: State.PASSIVE  # lock data taking
2 Source On:  State.ACTIVE # lock data taking
4 HV On: State.ENGAGED  # lock data taking
All Off:  # lock data taking
Emerengency Off:  # lock data taking
HV Off: State.ON   # lock data taking
Source Off: State.ENGAGED  # lock data taking
ASICs Off:  State.ACTIVE # lock data taking

