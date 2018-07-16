cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)

# Name of the project
project (WickrIOClientAPI)


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
#link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs)


link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/WickrIOClientAPI/build
${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/third_party/libzmq/build/lib)


# OLD WAY
#add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
#target_link_libraries(${PROJECT_NAME} libzmq.a ${CMAKE_DL_LIBS} OpenSSL::SSL OpenSSL::Crypto)


## Create static library (by joining the new objects and the dependencies)
ADD_LIBRARY("${PROJECT_NAME}-static" STATIC ${SOURCE_FILES})
add_custom_command(OUTPUT lib${PROJECT_NAME}.a
                   COMMAND rm ARGS -f *.o
                   COMMAND ar ARGS -x ${CMAKE_BINARY_DIR}/lib${PROJECT_NAME}-static.a
                   COMMAND rename ARGS 's/^/lib${PROJECT_NAME}-static./g' *.o
                   COMMAND rename ARGS 's/\\.o/.otmp/g' *.o
                   COMMAND ar ARGS -x ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/third_party/libzmq/build/lib/libzmq.a
                   COMMAND rename ARGS 's/^/libzmq./g' *.o
                   COMMAND rename ARGS 's/\\.o/.otmp/g' *.o
                   COMMAND rename ARGS 's/\\.otmp/.o/g' *.otmp
                   COMMAND ar ARGS -r lib${PROJECT_NAME}.a *.o
                   COMMAND rm ARGS -f *.o
                   DEPENDS "${PROJECT_NAME}-static")

add_custom_target(${PROJECT_NAME} ALL DEPENDS lib${PROJECT_NAME}.a)

