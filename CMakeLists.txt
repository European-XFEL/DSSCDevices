set (CMAKE_VERBOSE_MAKEFILE off )

cmake_policy(SET CMP0012 OLD)

cmake_minimum_required(VERSION 2.6.0)

option(BUILD_SHARED_LIBS ON)

project(DsscKaraboDevices)

#automatically includes the current source and binary directories
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

#SET(CMAKE_CXX_FLAGS "-Wall -std=c++11 -O2") # add "-g" for debugger -O3 for optimization
SET(CMAKE_CXX_FLAGS "-Wall -std=c++11 -g") # add "-g" for debugger -O3 for optimization

SET(CMAKE_EXE_LINKER_FLAGS "-Xlinker --no-as-needed")

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    message(STATUS "Found OpenMP: Will build with 'parallel for' enabled")
endif()

SET(PPT_SETUP TRUE)
SET(F2 TRUE)


#SET(F1TYPE TRUE)
SET(F2TYPE TRUE)
SET(HDF5_FOUND 1)
add_definitions(-DHAVE_HDF5)

if(F1TYPE)
 message(STATUS "Compile For DSSC_F1 setup")
 add_definitions( -DF1IO )
endif(F1TYPE)

if(F2TYPE)
  message(STATUS "Compile For DSSC_F2 setup")
  add_definitions( -DF2IO )
endif(F2TYPE)

SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR} )

message(STATUS)
message(STATUS "Searching for libraries:")


# tell cmake where to search for libraries:
file (STRINGS "$ENV{HOME}/.karabo/karaboFramework" KARABO)
message(STATUS "Karabo Directory is: " ${KARABO})

link_directories(
   ${KARABO}/lib
   ${KARABO}/extern/lib
   ${CMAKE_CURRENT_SOURCE_DIR}/build/libs
   ${CMAKE_CURRENT_SOURCE_DIR}/DsscDependencies/src/Minuit2/Minuit2-5.34.14/Minuit-build/lib/
)

message(STATUS "GSL Libraries")
message(STATUS ${GSL_LIBRARIES})

SET( DEPENDENC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/DsscDependencies/src )

include_directories(
  ${KARABO}/include
  ${KARABO}/extern/include
  ${KARABO}/extern/include/hdf5
  ${DEPENDENC_DIR}
  ${DEPENDENC_DIR}/Minuit2/Minuit2-5.34.14/Minuit-build/include/
  ${DEPENDENC_DIR}/libUtils
  ${DEPENDENC_DIR}/libDsscHdf5
  ${DEPENDENC_DIR}/libConfigReg
  ${DEPENDENC_DIR}/libSequencer
  ${DEPENDENC_DIR}/libCHIPInterface
  ${DEPENDENC_DIR}/libPPT
  ${DEPENDENC_DIR}/libPPTDataReceiver
)

add_custom_target(update
  /usr/bin/svn update ../* ../DsscDependencies/src/*
  )

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/libs)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${KARABO}/devices/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/bin)
set(CMAKE_BUILD_FILES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/)

if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/DsscDependencies/src/Minuit2/Minuit2-5.34.14/Minuit-build)
  message(STATUS "Minuit Library already compiled")
else()
  message(STATUS "Minuit Library not found will compile library")
  execute_process( COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/DsscDependencies/src/Minuit2/build.sh
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/DsscDependencies/src/Minuit2
                 )
endif()

add_subdirectory(DsscDependencies)

if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/DsscLadderParameterTrimming)
add_subdirectory(DsscLadderParameterTrimming)
endif()

if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/DsscPpt)
add_subdirectory(DsscPpt)
endif()


if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/DsscAsicProcessor)
add_subdirectory(DsscAsicProcessor)
endif()


if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/DsscDataReceiver)
add_subdirectory(DsscDataReceiver)
endif()


if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/DsscProcessor)
add_subdirectory(DsscProcessor)
endif()


if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/DsscDummyTrainGenerator)
add_subdirectory(DsscDummyTrainGenerator)
endif()


message(STATUS "CFLAGS Enabled: ${CMAKE_CXX_FLAGS}")
message(STATUS)
message(STATUS)

message(STATUS "--------------------------------")
if(F1TYPE)
  message(STATUS "Compile For DSSC_F1 setup")
endif(F1TYPE)

if(F2TYPE)
  message(STATUS "Compile For DSSC_F2 setup")
endif(F2TYPE)
message(STATUS "--------------------------------")

