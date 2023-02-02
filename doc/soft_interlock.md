# DSSC Control Soft interlock

This devices adds a layer of software interlock to prevent DEPFET-based modules to take data while being powered up or down.   
It is not intended as a safety mechanism, but rather to reduce operator cognitive load.

To implement this, all PPT devices, as well as the Power Procedure (used to power on and off the detector) are locked by this device, their slots duplicated here to provide a single point of interaction in which the states can be monitored. 

Thus, the device can either be powering on/off, or take data.  

## State Fusion

In `DsscControl.state_fusion`, the states from the PPT and Power Procedure devices are constantly monitored as a background task.
States are explicitly listed, using `if` statements, rather than `if/elif/else` in order to clarify the intended state machine.
This is based on the following state machine:

- If PPT devices do not all have the same state, we go to `State.ERROR`.
- If PPT devices are idle, we inherit that state.
- If any device is changing, we go to `State.CHANGING`.
- If the Power Procedure is in error, we go to `State.ERROR`.
- If PPT devices are acquiring, we inherit that state.

If acquiring, the power slots are disabled.  
If powering on/off, the acquisition slots are disabled.
If changing or in error, the power slots and acquisition slots are disabled.

The complete state list is the following:  

### Power Procedure

- State.CHANGING: power ramping up or down
- State.PASSIVE: power off
- State.ACTIVE: power on
- State.ENGAGED: power on
- State.ON: power on
- State.ERROR: undefined state (eg. hardware not replying, base mpod device down)

### PPT Devices

- State.UNKNOWN: undefined hardware state (ie. hardware powered off or karabo device disconnected)
- State.OPENING: connecting to hardware
- State.ON: powered on, idling
- State.CHANGING: setting operating conditions on hardware
- State.STARTED: readying the detector for continuous mode acquisition
- State.ACQUIRING: acquiring data
- State.CLOSING: disconnecting from hardware
- State.ERROR: invalid parameters or reflects hardware error state


### Power Procedure Slots

The detector is being powered on in stages, from ASICs to high voltage.  
This is achieved by using a number of different states to disable the slots in Karabo, thus ensuring that ramping is done in the correct order.  

The slots are replicated here in this device, but not disabled, although calling them in the wrong order will do nothing.

## Expert Mode

The device contains a `expertMode` boolean, aking to a commissioning mode, where the locks are not enforced, so that more minute features of the detector, such as registers, can be accessed.  
