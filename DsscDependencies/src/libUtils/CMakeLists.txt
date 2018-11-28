
ADD_DEFINITIONS(
  -Wall
  -Wpedantic
)

FIND_PACKAGE(GSL REQUIRED)
include_directories(${GSL_INCLUDE_DIRS} ${GSLCBLAS_INCLUDE_DIRS})
set(LIBS ${LIBS} ${GSL_LIBRARIES} ${GSLCBLAS_LIBRARIES})

set(Utils_SRCS
  utils.cpp
  DataHisto.cpp
  FitUtils.cpp
  SimUtils.cpp
  DsscCalibrationDB.cpp
  SimpleThreadSafeQueue.cpp
  DsscGainParamMap.cpp
  DsscTrainDataProcessor.cpp
  DsscSramBlacklist.cpp
  DsscProgressBar.cpp
  DsscTrainData.cpp
  PixelDataArray.cpp
  VariableIterator.cpp
  SpectrumFitResult.cpp
  DsscModuleInfo.cpp
)

add_library(libUtils SHARED ${Utils_SRCS})

target_link_libraries(libUtils
  Minuit2
  ${GSL_LIBRARIES}
)

