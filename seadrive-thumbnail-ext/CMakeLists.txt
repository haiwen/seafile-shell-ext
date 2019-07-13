CMAKE_MINIMUM_REQUIRED(VERSION 3.6)
PROJECT(seadrive_thumbnail_ext CXX)

################################################################################
# Compiler and linker options
ADD_DEFINITIONS(-Wall -DWIN32_LEAN_AND_MEAN -DUNICODE -D_CRT_SECURE_NO_WARNINGS)

SET(CMAKE_CXX_STANDARD 11)

# Disable warnings that are too verbose:
# Reference: https://docs.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/c-cpp-build-errors?view=vs-2019
# C4242: conversion from 'type1' to 'type2', possible loss of data
# C4710: function xx not inlined
# C4711: function xxx selected for inline expansion
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4242 /wd4710 /wd4711")
SET(DEFS ${CMAKE_CURRENT_SOURCE_DIR}/ExportFunctions.def)

################################################################################
# Sources and libraries
SET(SOURCE_FILES
  Reg.cpp
  ../common/applet-connection.cpp
  ../common/log.cpp
  ../common/ext-utils.cpp
  class-factory.cpp
  commands.cpp
  dllmain.cpp
  provider.cpp
  ext-config.cpp
)

SET(LIBS
  shlwapi
  userenv
  crypt32
  gdiplus
  msxml6
  user32
  windowscodecs
)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
  ${CMAKE_CURRENT_SOURCE_DIR}/../seadrive-thumbnail-ext
)

ADD_LIBRARY(seadrive_ext SHARED ${SOURCE_FILES} ${DEFS})
TARGET_LINK_LIBRARIES(
  seadrive_ext
  ${LIBS}
)