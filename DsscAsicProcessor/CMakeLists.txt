
set(DsscAsicProcessor_SRCS    
    src/DsscAsicProcessor.cc
)

add_library(DsscAsicProcessor SHARED ${DsscAsicProcessor_SRCS})

TARGET_LINK_LIBRARIES(DsscAsicProcessor
  DsscDependencies    
)

