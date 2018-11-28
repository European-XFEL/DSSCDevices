

SET(DEFAULT_CONFIG_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../var/data/ConfigFiles)

if(IS_DIRECTORY ${DEFAULT_CONFIG_FILE_PATH})
  message(STATUS "Default ConfigFiles exitsing at /karabo/var/data/ConfigFiles")
else()
  message(STATUS "Copy ConfigFiles to var/data/ConfigFiles")
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/ConfigFiles DESTINATION ${DEFAULT_CONFIG_FILE_PATH})
endif()

set(DsscLadderParameterTrimming_SRCS
    src/DsscLadderParameterTrimming.cc
    src/DsscKaraboRegisterConfig.cpp
)

add_library(DsscLadderParameterTrimming SHARED ${DsscLadderParameterTrimming_SRCS})

TARGET_LINK_LIBRARIES(DsscLadderParameterTrimming
  DsscDependencies
)
