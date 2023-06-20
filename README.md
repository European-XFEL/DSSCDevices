# Dssc Devices
[![pipeline status](https://git.xfel.eu/karaboDevices/dsscDevices/badges/master/pipeline.svg)](https://git.xfel.eu/karaboDevices/dsscDevices/-/commits/master)
[![coverage report](https://git.xfel.eu/karaboDevices/dsscDevices/badges/master/coverage.svg)](https://git.xfel.eu/karaboDevices/dsscDevices/-/commits/master) 


### DSSC PPT

The Patch Panel Transciever is the interface between the FPGA/ASICs and the outside world (configuration or acquisition), using its own protocol over TCP.  
This device is responsible for the configuring a quadrant (4 modules/ladders).  
4 instances of this device are needed to control the whole detector.  

### DSSC Control

A middlelayer device to orchestrate 4 quadrants (PPT devices) of the detector.  
As well as an unified interface for configuration, this device automates various procedures such as taking darks.  
Its soft interlock is documented in [doc/soft_interlock.md](doc/soft_interlock.md).

### DSSC Configurator

A middlelayer device used to set and monitor configurations across quadrants.  
The different quadrants must have the same configuration applied.  
This device monitors that and shows the configuration in its scene in big letters.  
It also eases applying known configuration through a drop-down menu, rather than restarting PPT devices.  
Its architecture is kept simple to eventually merge this within the Control device.

### DSSC SIB

A bound device to read and configure the Safety Interlock Board, which is its own hardware
device mounted next to the PPT.  
It is using an ASCII protocol over TCP.

### DSSC ASIC Reset

A middlelayer device to ease resetting ASICS on the various modules by providing a simple scene.  
It's meant to be used by detector experts during commissioning. As such, it is its own device.

### DSSC Veto Check

The detector sometimes sends out invalid data when configuration was not correctly uploaded to the
detector.  
This is difficult to check, as the detector behaves otherwise normally.  
As such, this device takes into account the veto pattern and ensures that the data coming out matches.  
This is then displayed on a scene and error states can be monitored.  
This requires an instance for each ladder.  

## Contributing

It is recommended to make git ignore changes to the configuration files found in `src/ConfigFiles`:  

```text
git update-index --skip-worktree src/ConfigFiles
```

### Python (Control, SIB, ASICReset, Configurator, VetoCheck)

Do an editable installation of the package:  

```bash
cd $KARABO/devices/DsscDevices
pip install --upgrade -e .
```

Tests are ran using pytest:  

```bash
pytest -vv 
```

###  C++ devices (DsscPPT, LadderParameterTrimming)  

#### Compiling

Generate the project's build files using cmake:  

```bash
cd $KARABO/devices/DsscDevices
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=$KARABO/extern ..
```

`CMAKE_BUILD_TYPE` can also be set to `Release`.  

Build the device:

```bash
cd $KARABO/devices/DsscDevices
cmake --build .
```

`make` can also be used as long as the Makefile generator is used by cmake.  

#### Testing

After building, a shared library is generated at `dist/<configuration>/<system>/libDsscDevices.so`.  
A soft-link to `libDsscDevices.so` is created in the `$KARABO/plugins` where Karabo loads the library from.  

Tests use `ctest` and can be ran from the build directory:  

```bash
cd $KARABO/devices/DsscDevices/build/DsscDevices
ctest -VV
```

### Running

To run the devices, both a C++ and Middlelayer servers are needed:  
```bash
karabo-cppserver serverId=cppServer/dssc deviceClasses=DsscPpt,DsscLadderParameterTrimming &
karabo-middlelayerserver serverId=middlelayerServer/dssc deviceClasses=DsscControl,Configurator,DsscVetoCheck &
karabo-pythonserver serverId=pythonServer/dssc deviceClasses=DsscSIB
```
