cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(QtAda)

set(QtAda_VERSION_MAJOR 0)
set(QtAda_VERSION_MINOR 0)
set(QtAda_VERSION_PATCH 1)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(QT_MIN_VERSION 5.15.2)

find_package(Qt5 ${QT_MIN_VERSION} COMPONENTS
    Core
REQUIRED)

add_subdirectory(probe)

set(core_HDRS
    core/Launcher.hpp
)
set(core_SRCS
    core/Launcher.cpp
)

set(qtada_SRCS
    main.cpp
)

add_library(core OBJECT ${core_SRCS} ${core_HDRS})
target_link_libraries(core
    PUBLIC Qt5::Core
)

add_executable(QtAda ${qtada_SRCS})
target_link_libraries(QtAda
    PRIVATE core
)
