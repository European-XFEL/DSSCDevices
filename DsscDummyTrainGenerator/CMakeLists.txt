
set(DsscDummyTrainGenerator_SRCS    
    src/DsscDummyTrainGenerator.cc
)

add_library(DsscDummyTrainGenerator SHARED ${DsscDummyTrainGenerator_SRCS})

TARGET_LINK_LIBRARIES(DsscDummyTrainGenerator
  DsscDependencies    
)

