cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(probe)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(QTADA_PROBE_BASENAME "QtAda_probe")
set(QTADA_PROBE_BASENAME ${QTADA_PROBE_BASENAME} PARENT_SCOPE)
find_package(Qt5 ${QT_MIN_VERSION} COMPONENTS
    Core
REQUIRED)

set(probe_HDRS
    HooksLauncher.cpp
    Hooks.hpp
)
set(probe_SRCS
    Hooks.cpp
)

add_library(probe SHARED ${probe_SRCS})
target_sources(probe PRIVATE ${probe_HDRS})
set_target_properties(probe PROPERTIES
                            PREFIX ${QTADA_LIB_PREFIX}
                            OUTPUT_NAME ${QTADA_PROBE_BASENAME}
                            LIBRARY_OUTPUT_DIRECTORY ${QTADA_LIB_DIR}
                            RUNTIME_OUTPUT_DIRECTORY ${QTADA_LIB_DIR}
)
target_link_libraries(probe Qt::CorePrivate)

install(TARGETS probe LIBRARY DESTINATION ${QTADA_LIB_DIR})
