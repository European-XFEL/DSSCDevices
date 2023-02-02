# Dssc Devices  

## DSSC Control

A middlelayer device to configure several PPT device, take darks, and more.  
Its soft interlock is documented in [doc/soft_interlock.md](doc/soft_interlock.md).


## Contributing

It is recommended to make git ignore changes to the configuration files found in `src/ConfigFiles`:  

```text
git update-index --skip-worktree src/ConfigFiles
```

### Python (DsscControl)

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
    karabo-cppserver serverId=cppServer/dssc deviceClasses=DsscPpt,DsscLadderParameterTrimming
    karabo-middlelayerserver serverId=middlelayerServer/dssc  deviceClasses=DsscControl
```
