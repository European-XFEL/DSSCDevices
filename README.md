# Dssc Devices  

## Developing C++ devices (DsscPPT, LadderParameterTrimming)  

It is recommended to make git ignore changes to the configuration files found in `src/ConfigFiles`:  

```text
git update-index --skip-worktree src/ConfigFiles
```

### Compiling

1. Goto the device source root directory and generate its build files with cmake:  

```bash
cd $KARABO/devices/DsscDevices
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=$KARABO/extern ..
```

`CMAKE_BUILD_TYPE` can also be set to `Release`.  

3. Build the device:

```bash
cd $KARABO/devices/DsscDevices
cmake --build .
```

`make` can also be used as long as the Makefile generator is used by cmake.  

### Testing

After building, a shared library is generated at `dist/<configuration>/<system>/libDsscDevices.so`.  
A soft-link to `libDsscDevices.so` is created in the `$KARABO/plugins` where Karabo loads the library from.  

Tests use `ctest` and can be ran from the build directory:  

```bash
cd $KARABO/devices/DsscDevices/build/DsscDevices
ctest -VV
```

### Running

Start a server with `karabo-cppserver serverId=cppServer/dssc_ppt deviceClasses=DsscPpt,DsscLadderParameterTrimming`.  
