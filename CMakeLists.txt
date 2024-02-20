cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(QtAda)

set(QtAda_VERSION_MAJOR 0)
set(QtAda_VERSION_MINOR 0)
set(QtAda_VERSION_PATCH 1)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(QT_MIN_VERSION 5.15.2)

add_subdirectory(hooks)

find_package(Qt5 ${QT_MIN_VERSION} COMPONENTS
    Core
REQUIRED)

# For ABI detection
find_file(HAVE_ELF_H elf.h)
find_file(HAVE_SYS_ELF_H sys/elf.h)
if(HAVE_ELF_H OR HAVE_SYS_ELF_H)
    set(HAVE_ELF TRUE)
endif()

#set(core_HDRS
#)
#set(core_SRCS
#)

set(launcher_HDRS
    launcher/Launcher.hpp
    launcher/LauncherUtils.hpp
    launcher/ProbeABI.hpp
    launcher/ProbeDetector.hpp
    launcher/LauncherLog.hpp
)
set(launcher_SRCS
    launcher/Launcher.cpp
    launcher/LauncherUtils.cpp
    launcher/ProbeABI.cpp
    launcher/ProbeDetector.cpp
    launcher/LauncherLog.cpp
)

set(qtada_SRCS
    main.cpp
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

#add_library(core OBJECT ${core_SRCS} ${core_HDRS})
#target_link_libraries(core
#    PUBLIC Qt5::Core
#)

add_library(launcher OBJECT ${launcher_SRCS} ${launcher_HDRS})
target_link_libraries(launcher
    PUBLIC Qt5::Core
)

add_executable(QtAda ${qtada_SRCS})
target_link_libraries(QtAda
    PRIVATE #[[core]] launcher
)
