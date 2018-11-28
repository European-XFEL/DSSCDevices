
set(libSEQUENCER_SRCS
        Sequencer.cpp
        SequencerTrack.cpp
    )

add_library(libSequencer SHARED ${libSEQUENCER_SRCS})

TARGET_LINK_LIBRARIES(
  libSequencer
  libUtils
)




