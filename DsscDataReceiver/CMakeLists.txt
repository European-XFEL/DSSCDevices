
set(DsscDataReceiver_SRCS
    src/DsscDataReceiver.cc
)

add_library(DsscDataReceiver SHARED ${DsscDataReceiver_SRCS})

TARGET_LINK_LIBRARIES(DsscDataReceiver
  DsscDependencies
  libUtils
  libDsscHdf5
)

