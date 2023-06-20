# DSSC SIB

The Safety Interlock Board exposes the experimental conditions to the detector.

Two revisions are supported: firmware version 1 targeting the miniSDD sensor, and version 2 which supports both miniSDD and DEPFET sensor.

## Decision Tree

The SIB uses a decision tree to establish whether the operating conditions are safe.  
These are based on the qloop status, cable status, experiment conditions, temperature, humidity, and pressure.
It is exposed as the `decision` property, where each field has a value of:

- `0`: ok
- `1`: error 
- `2`: warning
- `3`: undefined


## vssSum

This property exposes the sum of voltages across different ASIC power channels.  
The detector has different levels of power (off, high voltage, source, ASIcs).  
When `vccSum` equals to ~3 volts, we know that the detector is fully powered (ie. ASICs on).  
