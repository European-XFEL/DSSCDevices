
SET(DEFAULT_CONFIG_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../var/data/ConfigFiles)

if(IS_DIRECTORY ${DEFAULT_CONFIG_FILE_PATH})
  message(STATUS "Default ConfigFiles exitsing at /karabo/var/data/ConfigFiles")
else()
  message(STATUS "Copy ConfigFiles to var/data/ConfigFiles")
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/ConfigFiles DESTINATION ${DEFAULT_CONFIG_FILE_PATH})
endif()

set(DsscPpt_SRCS
    src/DsscPpt.cc
    src/DSSC_PPT_API.cpp
)

add_library(DsscPpt SHARED ${DsscPpt_SRCS})

TARGET_LINK_LIBRARIES(DsscPpt
  DsscDependencies
  libUtils
)

