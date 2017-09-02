cmake_minimum_required (VERSION 2.8 FATAL_ERROR)
project (luaXroot)

#Specify the install prefix as the exec directory unless specified
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	message(STATUS "Install Prefix not specified.")
	file(MAKE_DIRECTORY exec)
	get_filename_component(INSTALL_DIR ${CMAKE_SOURCE_DIR}/exec REALPATH)
	set(CMAKE_INSTALL_PREFIX ${INSTALL_DIR} CACHE PATH
			"Install Prefix" FORCE)
endif ()
message(STATUS "Installing to ${CMAKE_INSTALL_PREFIX}")

#Define the default build type
IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE Release CACHE STRING
		"Build type, options are: None Debug Release
			RelWithDebInfo MinSizeRel."
		FORCE)
	message(STATUS "Build type not defined, using default.")
ENDIF(NOT CMAKE_BUILD_TYPE)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if (CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -Wno-unused-but-set-variable -pedantic")
endif()

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -O3")

#Add custom modules to module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

set (CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH};${CMAKE_INSTALL_PREFIX}")

#include(${CMAKE_MODULE_PATH}/ExportDYLD.cmake)
#CheckForExportDYLD()

#Find the ROOT package and configure the flags and paths
find_package (ROOT REQUIRED)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ROOT_CXX_FLAGS} -fPIC")
include_directories(${ROOT_INCLUDE_DIR})
link_directories(${ROOT_LIBRARY_DIR})

add_definitions(-DLINUX -D_FILE_OFFSET_BITS=64)

include_directories(lua)
add_subdirectory(lua)

include_directories(include)
add_subdirectory(source)
