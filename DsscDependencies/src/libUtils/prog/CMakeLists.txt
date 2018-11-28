include (${QT_USE_FILE})

set(UtilsTest_SRCS main.cpp)

add_executable(UtilsTest ${UtilsTest_SRCS})

target_link_libraries(UtilsTest
   libUtils
)
