cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)

# Name of the project (will be the name of the plugin)
project (zeromq_test)


find_package(ZeroMQ QUIET)

# libzmq autotools install: fallback to pkg-config
if(NOT ZeroMQ_FOUND)
    message(STATUS "CMake libzmq package not found, trying again with pkg-config (normal install of zeromq)")
    list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/libzmq-pkg-config)
    find_package(ZeroMQ REQUIRED)
endif()

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()


# Declare the source files location
file(GLOB SOURCE_FILES "src/*.cpp" "src/*.h")

include_directories(/usr/local/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs)
add_executable(zeromq_test ${SOURCE_FILES})


target_link_libraries(zeromq_test libzmq ${CMAKE_DL_LIBS} OpenSSL::SSL OpenSSL::Crypto)
