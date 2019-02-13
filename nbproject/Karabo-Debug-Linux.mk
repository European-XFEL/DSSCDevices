#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/DsscAsicProcessor/src/DsscAsicProcessor.o \
	${OBJECTDIR}/DsscDataReceiver/src/DsscDataReceiver.o \
	${OBJECTDIR}/DsscDummyTrainGenerator/src/DsscDummyTrainGenerator.o \
	${OBJECTDIR}/DsscLadderParameterTrimming/src/DsscKaraboRegisterConfig.o \
	${OBJECTDIR}/DsscLadderParameterTrimming/src/DsscLadderParameterTrimming.o \
	${OBJECTDIR}/DsscPpt/src/DsscConfigHashWriter.o \
	${OBJECTDIR}/DsscPpt/src/DsscPpt.o \
	${OBJECTDIR}/DsscPpt/src/DsscPptAPI.o \
	${OBJECTDIR}/DsscProcessor/src/DsscProcessor.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L${KARABO}/extern/dssc_minuit/lib -L${KARABO}/extern/lib/dssc -L${KARABO}/extern/lib -L${KARABO}/lib -Wl,-rpath,${KARABO}/lib -Wl,-rpath,${KARABO}/extern/lib/dssc -Wl,-rpath,${KARABO}/extern/dssc_minuit/lib -Wl,-rpath,${KARABO}/extern/lib -lkarabo `pkg-config --libs karaboDependencies` -lDsscDependencies -lCHIPInterface -lSequencer -lUtils -lDsscHdf5 -lPPT -lPPTDataReceiver  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libdsscDevices.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libdsscDevices.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libdsscDevices.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/DsscAsicProcessor/src/DsscAsicProcessor.o: DsscAsicProcessor/src/DsscAsicProcessor.cc 
	${MKDIR} -p ${OBJECTDIR}/DsscAsicProcessor/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -DF2IO -DHAVE_HDF5 -I${KARABO}/include -I${KARABO}/extern/include/dssc/utils -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/dssc/chipinterface -I${KARABO}/extern/include/dssc/configreg -I${KARABO}/extern/include/dssc/dsschdf5 -I${KARABO}/extern/include/dssc/ppt -I${KARABO}/extern/include/dssc/pptdatareceiver -I${KARABO}/extern/include/dssc/sequence -I${KARABO}/extern/dssc_minuit/include -I${KARABO}/extern/include/dssc -I${KARABO}/extern/include `pkg-config --cflags karaboDependencies` -std=c++11  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/DsscAsicProcessor/src/DsscAsicProcessor.o DsscAsicProcessor/src/DsscAsicProcessor.cc

${OBJECTDIR}/DsscDataReceiver/src/DsscDataReceiver.o: DsscDataReceiver/src/DsscDataReceiver.cc 
	${MKDIR} -p ${OBJECTDIR}/DsscDataReceiver/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -DF2IO -DHAVE_HDF5 -I${KARABO}/include -I${KARABO}/extern/include/dssc/utils -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/dssc/chipinterface -I${KARABO}/extern/include/dssc/configreg -I${KARABO}/extern/include/dssc/dsschdf5 -I${KARABO}/extern/include/dssc/ppt -I${KARABO}/extern/include/dssc/pptdatareceiver -I${KARABO}/extern/include/dssc/sequence -I${KARABO}/extern/dssc_minuit/include -I${KARABO}/extern/include/dssc -I${KARABO}/extern/include `pkg-config --cflags karaboDependencies` -std=c++11  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/DsscDataReceiver/src/DsscDataReceiver.o DsscDataReceiver/src/DsscDataReceiver.cc

${OBJECTDIR}/DsscDummyTrainGenerator/src/DsscDummyTrainGenerator.o: DsscDummyTrainGenerator/src/DsscDummyTrainGenerator.cc 
	${MKDIR} -p ${OBJECTDIR}/DsscDummyTrainGenerator/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -DF2IO -DHAVE_HDF5 -I${KARABO}/include -I${KARABO}/extern/include/dssc/utils -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/dssc/chipinterface -I${KARABO}/extern/include/dssc/configreg -I${KARABO}/extern/include/dssc/dsschdf5 -I${KARABO}/extern/include/dssc/ppt -I${KARABO}/extern/include/dssc/pptdatareceiver -I${KARABO}/extern/include/dssc/sequence -I${KARABO}/extern/dssc_minuit/include -I${KARABO}/extern/include/dssc -I${KARABO}/extern/include `pkg-config --cflags karaboDependencies` -std=c++11  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/DsscDummyTrainGenerator/src/DsscDummyTrainGenerator.o DsscDummyTrainGenerator/src/DsscDummyTrainGenerator.cc

${OBJECTDIR}/DsscLadderParameterTrimming/src/DsscKaraboRegisterConfig.o: DsscLadderParameterTrimming/src/DsscKaraboRegisterConfig.cc 
	${MKDIR} -p ${OBJECTDIR}/DsscLadderParameterTrimming/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -DF2IO -DHAVE_HDF5 -I${KARABO}/include -I${KARABO}/extern/include/dssc/utils -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/dssc/chipinterface -I${KARABO}/extern/include/dssc/configreg -I${KARABO}/extern/include/dssc/dsschdf5 -I${KARABO}/extern/include/dssc/ppt -I${KARABO}/extern/include/dssc/pptdatareceiver -I${KARABO}/extern/include/dssc/sequence -I${KARABO}/extern/dssc_minuit/include -I${KARABO}/extern/include/dssc -I${KARABO}/extern/include `pkg-config --cflags karaboDependencies` -std=c++11  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/DsscLadderParameterTrimming/src/DsscKaraboRegisterConfig.o DsscLadderParameterTrimming/src/DsscKaraboRegisterConfig.cc

${OBJECTDIR}/DsscLadderParameterTrimming/src/DsscLadderParameterTrimming.o: DsscLadderParameterTrimming/src/DsscLadderParameterTrimming.cc 
	${MKDIR} -p ${OBJECTDIR}/DsscLadderParameterTrimming/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -DF2IO -DHAVE_HDF5 -I${KARABO}/include -I${KARABO}/extern/include/dssc/utils -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/dssc/chipinterface -I${KARABO}/extern/include/dssc/configreg -I${KARABO}/extern/include/dssc/dsschdf5 -I${KARABO}/extern/include/dssc/ppt -I${KARABO}/extern/include/dssc/pptdatareceiver -I${KARABO}/extern/include/dssc/sequence -I${KARABO}/extern/dssc_minuit/include -I${KARABO}/extern/include/dssc -I${KARABO}/extern/include `pkg-config --cflags karaboDependencies` -std=c++11  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/DsscLadderParameterTrimming/src/DsscLadderParameterTrimming.o DsscLadderParameterTrimming/src/DsscLadderParameterTrimming.cc

${OBJECTDIR}/DsscPpt/src/DsscConfigHashWriter.o: DsscPpt/src/DsscConfigHashWriter.cc 
	${MKDIR} -p ${OBJECTDIR}/DsscPpt/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -DF2IO -DHAVE_HDF5 -I${KARABO}/include -I${KARABO}/extern/include/dssc/utils -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/dssc/chipinterface -I${KARABO}/extern/include/dssc/configreg -I${KARABO}/extern/include/dssc/dsschdf5 -I${KARABO}/extern/include/dssc/ppt -I${KARABO}/extern/include/dssc/pptdatareceiver -I${KARABO}/extern/include/dssc/sequence -I${KARABO}/extern/dssc_minuit/include -I${KARABO}/extern/include/dssc -I${KARABO}/extern/include `pkg-config --cflags karaboDependencies` -std=c++11  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/DsscPpt/src/DsscConfigHashWriter.o DsscPpt/src/DsscConfigHashWriter.cc

${OBJECTDIR}/DsscPpt/src/DsscPpt.o: DsscPpt/src/DsscPpt.cc 
	${MKDIR} -p ${OBJECTDIR}/DsscPpt/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -DF2IO -DHAVE_HDF5 -I${KARABO}/include -I${KARABO}/extern/include/dssc/utils -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/dssc/chipinterface -I${KARABO}/extern/include/dssc/configreg -I${KARABO}/extern/include/dssc/dsschdf5 -I${KARABO}/extern/include/dssc/ppt -I${KARABO}/extern/include/dssc/pptdatareceiver -I${KARABO}/extern/include/dssc/sequence -I${KARABO}/extern/dssc_minuit/include -I${KARABO}/extern/include/dssc -I${KARABO}/extern/include `pkg-config --cflags karaboDependencies` -std=c++11  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/DsscPpt/src/DsscPpt.o DsscPpt/src/DsscPpt.cc

${OBJECTDIR}/DsscPpt/src/DsscPptAPI.o: DsscPpt/src/DsscPptAPI.cc 
	${MKDIR} -p ${OBJECTDIR}/DsscPpt/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -DF2IO -DHAVE_HDF5 -I${KARABO}/include -I${KARABO}/extern/include/dssc/utils -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/dssc/chipinterface -I${KARABO}/extern/include/dssc/configreg -I${KARABO}/extern/include/dssc/dsschdf5 -I${KARABO}/extern/include/dssc/ppt -I${KARABO}/extern/include/dssc/pptdatareceiver -I${KARABO}/extern/include/dssc/sequence -I${KARABO}/extern/dssc_minuit/include -I${KARABO}/extern/include/dssc -I${KARABO}/extern/include `pkg-config --cflags karaboDependencies` -std=c++11  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/DsscPpt/src/DsscPptAPI.o DsscPpt/src/DsscPptAPI.cc

${OBJECTDIR}/DsscProcessor/src/DsscProcessor.o: DsscProcessor/src/DsscProcessor.cc 
	${MKDIR} -p ${OBJECTDIR}/DsscProcessor/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -DF2IO -DHAVE_HDF5 -I${KARABO}/include -I${KARABO}/extern/include/dssc/utils -I${KARABO}/extern/include/hdf5 -I${KARABO}/extern/include/dssc/chipinterface -I${KARABO}/extern/include/dssc/configreg -I${KARABO}/extern/include/dssc/dsschdf5 -I${KARABO}/extern/include/dssc/ppt -I${KARABO}/extern/include/dssc/pptdatareceiver -I${KARABO}/extern/include/dssc/sequence -I${KARABO}/extern/dssc_minuit/include -I${KARABO}/extern/include/dssc -I${KARABO}/extern/include `pkg-config --cflags karaboDependencies` -std=c++11  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/DsscProcessor/src/DsscProcessor.o DsscProcessor/src/DsscProcessor.cc

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libdsscDevices.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
