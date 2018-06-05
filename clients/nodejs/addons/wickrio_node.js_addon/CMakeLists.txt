cmake_minimum_required(VERSION 2.8)

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_CXX_STANDARD 11)
set(PROJ_LIBRARIES "amqpcpp")

# Name of the project (will be the name of the plugin)
project (wickrio_addon)


find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()


# Declare the source files location
file(GLOB SOURCE_FILES "src/*.cpp" "src/*.h")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})


# Gives our library file a .node extension without any "lib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# Essential include files to build a node addon,
# You should add this line in every CMake.js based project
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})

include_directories(../libs/third_party/amqpcpp/include ../libs/third_party/rabbitmq-c/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs ${CMAKE_CURRENT_SOURCE_DIR}/../libs/third_party/amqpcpp ${CMAKE_CURRENT_SOURCE_DIR}/../libs/third_party/rabbitmq-c)


#target_link_libraries(rabbitmq_test libamqpcpp.a librabbitmq.a openssl ${CMAKE_DL_LIBS} OpenSSL::Crypto)
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} libamqpcpp.so librabbitmq.so ${CMAKE_DL_LIBS} OpenSSL::SSL OpenSSL::Crypto)
