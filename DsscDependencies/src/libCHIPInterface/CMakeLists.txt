if(F2)
  set(libCHIPInterface_SRCS
    CHIPInterface.cpp
    F2CHIPInterface.cpp
    MultiModuleInterface.cpp
    CHIPFullConfig.cpp
    PPTFullConfig.cpp
    CHIPTrimmer.cpp
    CHIPGainConfigurator.cpp
    CalibrationDataGenerator.cpp
  )
endif(F2)

if(F2B)
  set(libCHIPInterface_SRCS
    CHIPInterface.cpp
    F2BCHIPInterface.cpp
    MultiModuleInterface.cpp
    CHIPFullConfig.cpp
    PPTFullConfig.cpp
    CHIPTrimmer.cpp
    CHIPGainConfigurator.cpp
    CalibrationDataGenerator.cpp
  )
endif(F2B)

add_library(libCHIPInterface SHARED ${libCHIPInterface_SRCS})
