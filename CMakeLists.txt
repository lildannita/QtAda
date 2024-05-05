cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(QtAda)

set(QtAda_VERSION_MAJOR 0)
set(QtAda_VERSION_MINOR 1)
set(QtAda_VERSION_PATCH 0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(QT_MIN_VERSION 5.15.2)

set(QTADA_LIB_PREFIX "libQtAda_")
set(QTADA_BIN_DIR ${CMAKE_BINARY_DIR}/bin)
set(QTADA_LIB_DIR ${CMAKE_BINARY_DIR}/libs)
set(QTADA_EXAMPLES_BIN_DIR ${QTADA_BIN_DIR}/examples)
set(QTADA_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)

find_package(
  Qt5 ${QT_MIN_VERSION}
  COMPONENTS Core
             Quick
             Widgets
             RemoteObjects
  REQUIRED)

# For ABI detection
find_file(HAVE_ELF_H elf.h)
find_file(HAVE_SYS_ELF_H sys/elf.h)
if(HAVE_ELF_H OR HAVE_SYS_ELF_H)
  set(HAVE_ELF TRUE)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(common)
add_subdirectory(inprocess)
add_subdirectory(core)
add_subdirectory(probe)
add_subdirectory(launcher)
add_subdirectory(client)

configure_file(${CMAKE_SOURCE_DIR}/config.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_subdirectory(examples)
