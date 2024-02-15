cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(probe)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(probe_HDRS
    Probe.hpp
)
set(probe_SRCS
    Probe.cpp
)

add_library(probe SHARED ${probe_SRCS})

target_sources(probe PRIVATE ${probe_HDRS})

install(TARGETS probe LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/libs)
