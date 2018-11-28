
set(PPTDataReceiver_SRCS
  ThreadSafeQueue.hh
  RingStorage.hh
  TrainDataRingStorage.hh
  DsscPacketReceiverSimple.cpp
  DsscPacketReceiverNoSort.cpp
  DsscTrainSorter.cpp
  )


add_library(PPTDataReceiver SHARED ${PPTDataReceiver_SRCS})

TARGET_LINK_LIBRARIES( PPTDataReceiver
  PPT
  libUtils
)
