# Project
project(libusbnet)
set(MAJOR_VERSION 0)
set(MINOR_VERSION 5)
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
include(${CMAKE_MODULE_PATH}/FindUSB.cmake)
find_package(USB REQUIRED)
include_directories(${LIBUSB_INCLUDE_DIR})
if(${LIBUSB_CONST_BUFFERS})
   add_definitions(-DUSE_USB_CONST_BUFFERS)
endif(${LIBUSB_CONST_BUFFERS})

# Documentation
set(DOCUMENTATION_DIR "${CMAKE_SOURCE_DIR}/doc")
include(${CMAKE_MODULE_PATH}/Doxygen.cmake)

# Set library prefixes
SET(LIBDIR "lib${LIB_SUFFIX}")

# Subdirectories
add_subdirectory(src)

# Uninstall
CONFIGURE_FILE(
          "${CMAKE_MODULE_PATH}/UninstallTarget.cmake.in"
          "${CMAKE_CURRENT_BINARY_DIR}/UninstallTarget.cmake"
          IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
          "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/UninstallTarget.cmake")
