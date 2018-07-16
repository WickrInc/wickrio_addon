cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -pthread")

# Name of the project (will be the name of the plugin)
project (cpp_test)


# Declare the source files location
file(GLOB SOURCE_FILES "src/*.cpp" "src/*.h")

include_directories(/usr/local/include ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/WickrIOClientAPI/src)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/WickrIOClientAPI/build)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} libWickrIOClientAPI.a ${CMAKE_DL_LIBS})

