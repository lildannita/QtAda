cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(hooks)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#find_package(Qt5 ${QT_MIN_VERSION} COMPONENTS
#    Core
#REQUIRED)

set(hooks_HDRS
    Hooks.hpp
)
set(hooks_SRCS
    Hooks.cpp
)

add_library(hooks SHARED ${hooks_SRCS})

target_sources(hooks PRIVATE ${hooks_HDRS})

install(TARGETS hooks LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/libs)
