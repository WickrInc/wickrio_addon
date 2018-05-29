cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)

# Name of the project (will be the name of the plugin)
project (rabbitmq_test)


find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()


# Declare the source files location
file(GLOB SOURCE_FILES "src/*.cpp" "src/*.h")

include_directories(../libs/third_party/amqpcpp/include ../libs/third_party/rabbitmq-c/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs ../libs/third_party/amqpcpp ../libs/third_party/rabbitmq-c)
add_executable(rabbitmq_test ${SOURCE_FILES})



#target_link_libraries(rabbitmq_test libamqpcpp.a librabbitmq.a openssl ${CMAKE_DL_LIBS} OpenSSL::Crypto)
target_link_libraries(rabbitmq_test libamqpcpp.a librabbitmq.a ${CMAKE_DL_LIBS} OpenSSL::SSL OpenSSL::Crypto)
