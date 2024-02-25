set(QTADA_PROBE_BASENAME "QtAda_probe")
set(QTADA_PROBE_BASENAME
    ${QTADA_PROBE_BASENAME}
    PARENT_SCOPE)

set(probe_HDRS Hooks.hpp)
set(probe_SRCS Hooks.cpp)

add_library(probe SHARED ${probe_SRCS} ${probe_HDRS})
set_target_properties(
  probe
  PROPERTIES PREFIX ${QTADA_LIB_PREFIX}
             OUTPUT_NAME ${QTADA_PROBE_BASENAME}
             LIBRARY_OUTPUT_DIRECTORY ${QTADA_LIB_DIR}
             RUNTIME_OUTPUT_DIRECTORY ${QTADA_LIB_DIR})
target_link_libraries(probe PRIVATE core Qt5::CorePrivate)

# install(TARGETS probe LIBRARY DESTINATION ${QTADA_LIB_DIR})
