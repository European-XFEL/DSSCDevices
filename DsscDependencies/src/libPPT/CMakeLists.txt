set(PPT_SRCS
    DSSC_PPT.cpp
    DSSC_PPT_FTP.cpp
)

add_library(PPT SHARED ${PPT_SRCS})

TARGET_LINK_LIBRARIES(
  PPT
  libCHIPInterface
  libSequencer
  libConfigReg
  libUtils
)
