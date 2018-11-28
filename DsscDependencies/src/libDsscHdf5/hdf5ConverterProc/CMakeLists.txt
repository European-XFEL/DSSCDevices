include (${QT_USE_FILE})

set( CMAKE_AUTOMOC ON )

set(HDF5Packer_SRCS
    ../DsscHDF5Packer.cpp
    ../SelASICPixelsWidget.cpp
    )

set(DsscHDF5ToRootConverter_SRCS
    main.cpp
    ../QDsscHDF5PackerWidget.cpp
    )

add_library(libHdf5Packer SHARED ${HDF5Packer_SRCS})

target_link_libraries(libHdf5Packer
  ${QT_LIBRARIES}
  QConfigRegWidget
  libDsscHdf5
  libUtils
  Logfile
  QLogViewer)

add_executable(DsscHDF5ToRootConverter ${DsscHDF5ToRootConverter_SRCS})

target_link_libraries(DsscHDF5ToRootConverter
   libHdf5Packer
   libRootTreeHandler
)

