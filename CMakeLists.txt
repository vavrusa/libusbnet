# Project
project(libusbnet)
set(MAJOR_VERSION 0)
set(MINOR_VERSION 1)
set(VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION})
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# CMake
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
   set(CMAKE_BUILD_TYPE     Debug)
endif("${CMAKE_BUILD_TYPE}" STREQUAL "")
set(CMAKE_COLOR_MAKEFILE ON)
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# Debug build
message("-- Build type: ${CMAKE_BUILD_TYPE}")
if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
   add_definitions(-DDEBUG)
endif(${CMAKE_BUILD_TYPE} STREQUAL Debug)

# Dependencies
find_package(USB REQUIRED)
include_directories(${LIBUSB_INCLUDE_DIR})

# Subdirectories
add_subdirectory(src)
