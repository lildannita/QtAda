cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(QtAda)

set(QtAda_VERSION_MAJOR 0)
set(QtAda_VERSION_MINOR 0)
set(QtAda_VERSION_PATCH 1)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(QT_MIN_VERSION 5.15.2)

set(QTADA_LIB_PREFIX "lib")
set(QTADA_BIN_DIR ${CMAKE_BINARY_DIR}/bin)
set(QTADA_LIB_DIR ${CMAKE_BINARY_DIR}/libs)

find_package(
  Qt5 ${QT_MIN_VERSION}
  COMPONENTS Core
  REQUIRED)

# For ABI detection
find_file(HAVE_ELF_H elf.h)
find_file(HAVE_SYS_ELF_H sys/elf.h)
if(HAVE_ELF_H OR HAVE_SYS_ELF_H)
  set(HAVE_ELF TRUE)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(core)
add_subdirectory(probe)
add_subdirectory(launcher)

configure_file(${CMAKE_SOURCE_DIR}/config.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)
