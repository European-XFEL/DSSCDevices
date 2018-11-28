

set(DsscProcessor_SRCS
    src/DsscProcessor.cc
)

add_library(DsscProcessor SHARED ${DsscProcessor_SRCS})

TARGET_LINK_LIBRARIES(DsscProcessor
  DsscDependencies
  libUtils
  libDsscHdf5
)
