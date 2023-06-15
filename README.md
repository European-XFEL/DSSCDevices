# DSSC Devices
[![pipeline status](https://git.xfel.eu/karaboDevices/dsscDevices/badges/main/pipeline.svg)](https://git.xfel.eu/karaboDevices/dsscDevices/-/commits/main)
[![coverage report](https://git.xfel.eu/karaboDevices/dsscDevices/badges/main/coverage.svg)](https://git.xfel.eu/karaboDevices/dsscDevices/-/commits/main)

The DSSC, DEPMOS Sensor with Signal Compression, is a detector for soft x-rays in the range of 0.25-6 keV capable of taking  800 frames at 4.5 MHz.

[See the following publication](https://ieeexplore.ieee.org/abstract/document/9419081):
```text
M. Porro et al., "The MiniSDD-Based 1-Mpixel Camera of the DSSC Project for the European XFEL,"
in IEEE Transactions on Nuclear Science, vol. 68, no. 6, pp. 1334-1350, June 2021,
doi: 10.1109/TNS.2021.3076602.
```
This repository contains the Karabo devices for integration at EuXFEL.  
It makes use of a `DsscDependencies` package, the API for interfacing with the
detector.


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
Its features are documented in [doc/sib.md](doc/sib.md)

### DSSC ASIC Reset

A middlelayer device to ease resetting ASICS on the various modules by providing a simple scene.  
It's meant to be used by detector experts during commissioning. As such, it is its own device.

## Developing

It is recommended to make git ignore changes to the configuration files found in `src/ConfigFiles`:  

```text
git update-index --skip-worktree src/ConfigFiles
```

### Python (Control, SIB, ASICReset, Configurator)

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

To run the devices, three servers are needed:  
```bash
karabo-cppserver serverId=cppServer/dssc deviceClasses=DsscPpt,DsscLadderParameterTrimming &
karabo-middlelayerserver serverId=middlelayerServer/dssc deviceClasses=DsscControl,Configurator &
karabo-pythonserver serverId=pythonServer/dssc deviceClasses=DsscSIB
```

## Contributing

The developement of this project is done on [EuXFEL's Gitlab](https://git.xfel.eu/karaboDevices/DsscDevices).

This software is released by the European XFEL GmbH as is and without any
warranty under the GPLv3 license.
If you have questions on contributing to the project, please get in touch at
opensource@xfel.eu.

External contributors, i.e. anyone not contractually associated to
the European XFEL GmbH, are asked to sign a Contributor License
Agreement (CLA):

* people contributing as individuals should sign the Individual CLA
* people contributing on behalf of an organization should sign the Entity CLA.

The CLAs can be found in the [Contributor License Agreement](doc/contributor_license_agreement.md) and
[Entity Contributor License Agreement](doc/entity_contributor_license_agreement.md)
documents located in the `doc` directory in this repository.  
Please send signed CLAs to opensource [at] xfel.eu. We'll get in touch with you
then.
We ask for your understanding that we cannot accept external
contributions without a CLA in place. Importantly, with signing the CLA you
acknowledge that

* European XFEL retains all copyrights of the DsscDevices software,
* European XFEL may relicense the DsscDevices software under other
  appropriate open source licenses which the Free Software Foundation
  classifies as Free Software licenses.

However, you are welcome to already suggest modifications you'd like to
contribute by opening a merge/pull request before you send the CLA.

You are free to use this software under the terms of the GPLv3 without signing
a CLA.
