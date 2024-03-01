set(QTADA_PROBE_BASENAME "probe")
set(QTADA_PROBE_BASENAME
    ${QTADA_PROBE_BASENAME}
    PARENT_SCOPE)

set(probe_HDRS ProbeInitializer.hpp)
set(probe_SRCS Hooks.cpp ProbeInitializer.cpp)

qt5_wrap_cpp(probe_SRCS ${probe_HDRS} TARGET probe)

add_library(probe SHARED ${probe_SRCS} ${probe_HDRS})
set_target_properties(
  probe
  PROPERTIES PREFIX ${QTADA_LIB_PREFIX}
             OUTPUT_NAME ${QTADA_PROBE_BASENAME}
             LIBRARY_OUTPUT_DIRECTORY ${QTADA_LIB_DIR}
             RUNTIME_OUTPUT_DIRECTORY ${QTADA_LIB_DIR})
target_link_libraries(probe PRIVATE core Qt5::CorePrivate)
target_include_directories(probe PRIVATE ${QTADA_CORE_INCLUDE_DIR})
