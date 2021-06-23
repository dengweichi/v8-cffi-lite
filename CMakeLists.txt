cmake_minimum_required(VERSION 3.5.1)
project(v8_ffi)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_VERBOSE_MAKEFILE ON)
# 只支持linux系统
if(NOT(CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    message(SEND_ERROR "only support linux system")
endif()

# 只支持64位系统
if(NOT(CMAKE_SIZEOF_VOID_P EQUAL 8))
    message(SEND_ERROR "only support 64 bits")
endif()

# 只支持x86-64位处理器
if(NOT(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64"))
    message(SEND_ERROR "only support x86-64")
endif()

# 如果没有拉去lib-cffi 库。执行拉取
if (NOT EXISTS "${PROJECT_SOURCE_DIR}/lib-cffi/CMakeLists.txt" )
    message(STATUS "lib-cffi download ........")
    execute_process(COMMAND git clone https://github.com/dengweichi/lib-cffi.git  WORKING_DIRECTORY  ${PROJECT_SOURCE_DIR})
endif()

# 判断是否拉取lib-cffi库
if (NOT EXISTS "${PROJECT_SOURCE_DIR}/lib-cffi/CMakeLists.txt" )
    message(SEND_ERROR "please execute command git clone https://github.com/dengweichi/lib-cffi.git in root dir")
endif ()

add_subdirectory(./lib-cffi)
include_directories(./lib-cffi/include)

include_directories(${PROJECT_SOURCE_DIR}/v8/include/)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -pthread -DV8_COMPRESS_POINTERS")
add_executable(${PROJECT_NAME}
        ./src/main.cpp)
target_link_libraries(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/v8/lib/libv8_monolith.a ${PROJECT_SOURCE_DIR}/v8/lib/libv8_libplatform.a ${PROJECT_SOURCE_DIR}/v8/lib/libv8_libbase.a)
target_link_libraries(${PROJECT_NAME} lib-cffi lib-cffi)