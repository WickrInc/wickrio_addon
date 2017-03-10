cmake_minimum_required(VERSION 2.8.11)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")

if (NOT PKG_CONFIG_FOUND)
    find_package(PkgConfig REQUIRED)
endif()

#macro(use_c99)
#  if (CMAKE_VERSION VERSION_LESS "3.1")
#    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
#      set (CMAKE_C_FLAGS "--std=gnu99 ${CMAKE_C_FLAGS}")
#    endif ()
#  else ()
#    set (CMAKE_C_STANDARD 99)
#  endif ()
#endmacro(use_c99)
#
#use_c99()

#if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR
#    ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
#    set(warnings "-Wall -Wextra -Werror")
#elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
#    set(warnings "/W4 /wd4512 /WX /EHsc")
#        # Disabled Warnings:
#        #   4512 "assignment operator could not be generated"
#        #        This warning provides no useful information and will occur in
#        #        well formed programs.
#        #        <http://msdn.microsoft.com/en-us/library/hsyx7kbz.aspx>
#endif()

project(WickrBase)

message("In Wickr Base unit test")

file(GLOB WickrBaseUnitTestSources *.cpp cspec/*.c)
message("WickrBaseUnitTestSources = ${WickrBaseUnitTestSources}")
file(GLOB WickrBaseUnitTestHeaders *.h cspec/*.h)
message("WickrBaseUnitTestHeaders = ${WickrBaseUnitTestHeaders}")

if(WIN32)
    file(GLOB LIBRARIES "../../localRepo/win32/debug/wickr-desktop-base.a")
elseif(APPLE)
    file(GLOB LIBRARIES "../../localRepo/mac/debug/libwickr-desktop-base.a")
elseif(UNIX)
    file(GLOB LIBRARIES "../../localRepo/linux/debug/libwickr-desktop-base.a")
else()
    message("OS not supported!")
endif()
message("LIBRARIES = ${LIBRARIES}")

add_executable(wickrbase_test ${WickrBaseUnitTestSources} ${WickrBaseUnitTestHeaders})

if (NOT APPLE AND UNIX)
else()
set_target_properties(wickrbase_test PROPERTIES LINKER_LANGUAGE Cxx) 
endif()

target_link_libraries(wickrbase_test ${LIBRARIES}) 

foreach(FILE ${WickrBaseUnitTestSources} ${WickrBaseUnitTestHeaders})
    # Get the directory of the source file
    if (CMAKE_VERSION VERSION_LESS "3.0")
        get_filename_component(PARENT_DIR "${FILE}" PATH)
    else()
        get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)
    endif()

    # Remove common directory prefix to make the group
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")

    # Make sure we are using windows slashes
    string(REPLACE "/" "\\" GROUP "${GROUP}")

    set(GROUP "${GROUP}")

# COMMENT OUT
#    # Group into "Source Files" and "Header Files"
#    if ("${FILE}" MATCHES ".*\\.c")
#       set(GROUP "Source Files${GROUP}")
#    elseif("${FILE}" MATCHES ".*\\.h")
#       set(GROUP "Header Files${GROUP}")
#    endif()

    source_group("${GROUP}" FILES "${FILE}")
endforeach()

target_include_directories(wickrbase_test PUBLIC . cspec cspec/inc)

#add_dependencies(wickrbase_test wickr_crypto_lib)

#target_link_libraries(wickrbase_test wickr_crypto_lib)

if (WIN32)
  set(CMAKE_LIBRARY_PATH_FLAG "LIBPATH:")
endif()
