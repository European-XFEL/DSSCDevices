include (${QT_USE_FILE})

set(DsscHDF5Receiver_SRCS
    main.cpp
    ../DsscHDF5ReceiveWidget.cpp
    ../DsscHDF5Receiver.cpp
    ../ImagePreview.cpp
    )

set( CMAKE_AUTOMOC ON )


add_executable(DsscHDF5Receiver ${DsscHDF5Receiver_SRCS})

target_link_libraries(DsscHDF5Receiver
   ${QT_LIBRARIES}
   libDsscHdf5
   PPTDataReceiver
   libRootTreeHandler
)

