cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(QtAda)

# Для проверки на утечки:
# set(SANITIZE_FLAGS "-fsanitize=address -fno-omit-frame-pointer -g")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZE_FLAGS}")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZE_FLAGS}")

set(QtAda_VERSION_MAJOR 1)
set(QtAda_VERSION_MINOR 1)
set(QtAda_VERSION_PATCH 0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(QT_MIN_VERSION 5.15.2)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN true)

set(QTADA_LIB_PREFIX "libQtAda_")
set(QTADA_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(QTADA_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/libs)
set(QTADA_EXAMPLES_BIN_DIR ${QTADA_BIN_DIR}/examples)
set(QTADA_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)

find_package(
  Qt5 ${QT_MIN_VERSION}
  COMPONENTS Core
             Quick
             Widgets
             RemoteObjects
             Concurrent
             Test
  REQUIRED)

# For ABI detection
find_file(HAVE_ELF_H elf.h)
find_file(HAVE_SYS_ELF_H sys/elf.h)
if(HAVE_ELF_H OR HAVE_SYS_ELF_H)
  set(HAVE_ELF TRUE)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

option(BUILD_QTADA_CLIENT "BUILD GUI CLIENT" ON)

add_subdirectory(common)
add_subdirectory(inprocess)
add_subdirectory(core)
add_subdirectory(probe)
add_subdirectory(launcher)
add_subdirectory(client)

add_custom_target(qtada_ci DEPENDS common inprocess core probe launcher qtada)
if(BUILD_QTADA_CLIENT)
  add_custom_target(qtada_all DEPENDS common inprocess core probe launcher gui qtada)
endif()

option(DEBUG_RUN "DEBUG PROBE FOR RUNNING" OFF)
option(DEBUG_RECORD "DEBUG PROBE FOR RECORDING" OFF)
if (DEBUG_RUN OR DEBUG_RECORD)
  set(DEBUG_BUILD TRUE)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)

option(BUILD_EXAMPLES "Build example projects" OFF)
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
