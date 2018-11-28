

set(ConfigReg_SRCS
    ConfigReg.cpp
    ModuleSet.cpp
    ConfigSignals.cpp
)

add_library(libConfigReg SHARED ${ConfigReg_SRCS})

target_link_libraries(libConfigReg
  libUtils
)
