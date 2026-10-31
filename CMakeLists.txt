cmake_minimum_required(VERSION 3.19)
project(CMakeWorkspace LANGUAGES C CXX ASM)

# Include the shared AVR helpers
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/avr.help.cmake)

# Automatically load all directories
file(GLOB ALL_ENTRIES "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(ENTRY ${ALL_ENTRIES})
  if(IS_DIRECTORY "${ENTRY}" AND EXISTS "${ENTRY}/CMakeLists.txt" AND NOT "${ENTRY}" MATCHES "out$")
    message(STATUS "[Workspace] Loading ${ENTRY}...")
    add_subdirectory("${ENTRY}")
  endif()
endforeach()
