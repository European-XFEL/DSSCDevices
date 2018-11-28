
message("-- Include Dependencies from:")
message("   " ${DEPENDENC_DIR})

add_subdirectory(${DEPENDENC_DIR}/libUtils)
add_subdirectory(${DEPENDENC_DIR}/libDsscHdf5)
add_subdirectory(${DEPENDENC_DIR}/libSequencer)
add_subdirectory(${DEPENDENC_DIR}/libConfigReg)
add_subdirectory(${DEPENDENC_DIR}/libCHIPInterface)
add_subdirectory(${DEPENDENC_DIR}/libPPT)
add_subdirectory(${DEPENDENC_DIR}/libPPTDataReceiver)


set(DsscDependencies_SRCS
    src/DsscDependencies.cpp
)

add_library(DsscDependencies SHARED ${DsscDependencies_SRCS})

TARGET_LINK_LIBRARIES(DsscDependencies
  libUtils
  libDsscHdf5
  PPT
  PPTDataReceiver
)
