set(QTADA_CORE_BASENAME "core")

set(QTADA_CORE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

set(core_UI_HDRS
  gui/ControlDialog.hpp
  gui/PropertiesWatcher.hpp)
set(core_MOC_HDRS
  ${core_UI_HDRS}
  Probe.hpp
  MetaObjectHandler.hpp
  UserEventFilter.hpp
  GuiEventFilter.hpp
  QuickEventFilter.hpp
  WidgetEventFilter.hpp
  ScriptWriter.hpp)
set(core_HDRS
  ${core_MOC_HDRS}
  GenerationSettings.hpp
  ProbeGuard.hpp
  MetaTypeDeclarations.hpp
  ProcessedObjects.hpp
  LastEvent.hpp
  utils/Tools.hpp
  utils/CommonFilters.hpp
  utils/FilterUtils.hpp
  utils/SocketMessages.hpp)
set(core_SRCS
  GenerationSettings.cpp
  Probe.cpp
  ProbeGuard.cpp
  MetaObjectHandler.cpp
  UserEventFilter.cpp
  QuickEventFilter.cpp
  WidgetEventFilter.cpp
  LastEvent.cpp
  ScriptWriter.cpp
  utils/FilterUtils.cpp
  utils/Tools.cpp
  gui/ControlDialog.cpp
  gui/PropertiesWatcher.cpp)
qt5_wrap_cpp(core_HDRS ${core_MOC_HDRS} TARGET core)
qt5_add_resources(icons_RSRC ${QTADA_RESOURCES_DIR}/icons.qrc)

add_library(core SHARED ${core_SRCS} ${core_HDRS} ${icons_RSRC})
set_target_properties(core PROPERTIES PREFIX ${QTADA_LIB_PREFIX}
                                      OUTPUT_NAME ${QTADA_CORE_BASENAME}
                                      LIBRARY_OUTPUT_DIRECTORY ${QTADA_LIB_DIR}
                                      RUNTIME_OUTPUT_DIRECTORY ${QTADA_LIB_DIR})
target_link_libraries(core PRIVATE Qt5::Core
                                   Qt5::CorePrivate
                                   Qt5::Quick
                                   Qt5::Widgets
                                   Qt5::Network)
