CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9)

PROJECT(shellext-fix)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

INCLUDE_DIRECTORIES(
  src
)

SET(SRC
    ${PROJECT_SOURCE_DIR}/src/registry.cpp
    ${PROJECT_SOURCE_DIR}/src/main.cpp
)

SET(CMAKE_RC_COMPILE_OBJECT
      "<CMAKE_RC_COMPILER> -i <SOURCE> -o <OBJECT>")

## Add resource file
SET(SOURCE_FILES resources/shellext-fix.rc)

ADD_EXECUTABLE(shellext-fix ${SOURCE_FILES} ${SRC})

## Add extern lib
TARGET_LINK_LIBRARIES(shellext-fix shlwapi)

INSTALL(TARGETS shellext-fix DESTINATION bin)
