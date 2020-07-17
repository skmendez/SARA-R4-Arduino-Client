# Uncompilable CMake File to enable project editing with CLion IDE

cmake_minimum_required(VERSION 3.13)
project(sara-r4-arduino-client)

include_directories("${CMAKE_CURRENT_LIST_DIR}/src")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_CXX_COMPILER_WORKS 1)
SET(CMAKE_CXX_COMPILER "$ENV{HOMEDRIVE}$ENV{HOMEPATH}/.platformio/packages/toolchain-xtensa/bin/xtensa-lx106-elf-g++.exe")
SET(CMAKE_CXX_FLAGS "-fno-rtti -std=c++11 -Os -mlongcalls -mtext-section-literals -falign-functions=4 -U__STRICT_ANSI__ -ffunction-sections -fdata-sections -fno-exceptions -Wall")
set(CMAKE_CXX_STANDARD 11)

# Include your relevant Arduino files in this file in order for Clion to understand your project
# In my case, I just used the include_directories that were contained within my CMakeListsPrivate.txt for PlatformIO
include(CMakeListsArduinoReferences.txt)


FILE(GLOB_RECURSE EXAMPLE_FILES ${CMAKE_CURRENT_LIST_DIR}/examples/*.*)

FILE(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_LIST_DIR}/src/*.*)

list(APPEND SOURCE_FILES ${EXAMPLE_FILES})

add_executable(sara-r4-arduino-client ${SOURCE_FILES})