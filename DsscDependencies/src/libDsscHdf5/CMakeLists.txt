
set(DsscHDF5_SRCS
      DsscHDF5Writer.cpp
      DsscHDF5Reader.cpp
      DsscHDF5CorrectionFileReader.cpp
      DsscHDF5MeasurementInfoReader.cpp
      DsscHDF5MeasurementInfoWriter.cpp
      DsscHDF5TrainDataReader.cpp
      DsscHDF5MeanRMSReader.cpp
      DsscHDF5CalibrationDB.cpp
      DsscHDF5TrimmingDataWriter.cpp
      DsscHDF5TrimmingDataReader.cpp
      DsscHDF5CalibrationDataGenerator.cpp
)

add_library(libDsscHdf5 SHARED ${DsscHDF5_SRCS})

target_link_libraries(libDsscHdf5
  ${HDF5_LDFLAGS}
  pthread
  m
)
