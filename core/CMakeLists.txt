set(QTADA_CORE_BASENAME "core")

set(QTADA_CORE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

set(core_MOC_HDRS
  Probe.hpp
  MetaObjectHandler.hpp
  UserEventFilter.hpp
  WidgetEventFilters.hpp)
set(core_HDRS
  ${core_MOC_HDRS}
  ProbeGuard.hpp
  MetaTypeDeclarations.hpp
  utils/Execution.hpp
  utils/FilterUtils.hpp)
set(core_SRCS
  Probe.cpp
  ProbeGuard.cpp
  MetaObjectHandler.cpp
  UserEventFilter.cpp
  WidgetEventFilters.cpp
  utils/Execution.cpp
  utils/FilterUtils.cpp)
qt5_wrap_cpp(core_SRCS ${core_MOC_HDRS} TARGET core)

add_library(core SHARED ${core_SRCS} ${core_HDRS})
set_target_properties(core PROPERTIES PREFIX ${QTADA_LIB_PREFIX}
                                      OUTPUT_NAME ${QTADA_CORE_BASENAME}
                                      LIBRARY_OUTPUT_DIRECTORY ${QTADA_LIB_DIR}
                                      RUNTIME_OUTPUT_DIRECTORY ${QTADA_LIB_DIR})
target_link_libraries(core PRIVATE Qt5::Core
                                   Qt5::CorePrivate
                                   Qt5::Quick
                                   Qt5::Widgets)
