set(QTADA_CORE_BASENAME "core")

set(core_HDRS Probe.hpp)
set(core_SRCS Probe.cpp)

qt5_wrap_cpp(core_SRCS ${core_HDRS} TARGET core)

add_library(core SHARED ${core_SRCS} ${core_HDRS})
set_target_properties(
  core
  PROPERTIES PREFIX ${QTADA_LIB_PREFIX}
             OUTPUT_NAME ${QTADA_CORE_BASENAME}
             LIBRARY_OUTPUT_DIRECTORY ${QTADA_LIB_DIR}
             RUNTIME_OUTPUT_DIRECTORY ${QTADA_LIB_DIR})
target_link_libraries(core PRIVATE Qt5::Core)
