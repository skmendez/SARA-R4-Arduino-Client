# Uncompilable CMake File to enable project editing with CLion IDE

cmake_minimum_required(VERSION 3.13)
project(sara-r4-arduino-client)

include_directories("${CMAKE_CURRENT_LIST_DIR}/src")

# Include your relevant Arduino files in this file in order for Clion to understand your project
# In my case, I just used the include_directories that were contained within my CMakeListsPrivate.txt for PlatformIO
include(CMakeListsArduinoReferences.txt)

set(CMAKE_CXX_FLAGS -std=c++11)


FILE(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_LIST_DIR}/src/*.*)

add_executable(sara-r4-arduino-client ${SOURCE_FILES})