set(QTADA_CORE_BASENAME "core")

set(QTADA_CORE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

# MetaObjectHandler был создан на ранних этапах разработки QtAda,
# и подразумевалось, что он будет отвечать за отображение
# дерева элементов и их свойств. Но на текущий момент было
# решено "облегчить" задачу отображения дерева элементов и
# их свойств: на стороне UserVerificationFilter подготавливаются
# статические данные и отправляются на сторону клиента в виде
# InprocessController. Возможно, это не совсем правильно, так как
# данные могут измениться, и клиент будет отображать неправильные
# данные, однако, остается надежда, что UserVerificationFilter
# правильно блокирует события, и данные не изменяться.

set(core_MOC_HDRS
# MetaObjectHandler.hpp
  Probe.hpp
  UserEventFilter.hpp
  GuiEventFilter.hpp
  QuickEventFilter.hpp
  WidgetEventFilter.hpp
  UserVerificationFilter.hpp)
set(core_HDRS
  ${core_MOC_HDRS}
# MetaTypeDeclarations.hpp
  ProbeGuard.hpp
  ProcessedObjects.hpp
  LastEvent.hpp
  utils/Tools.hpp
  utils/CommonFilters.hpp
  utils/FilterUtils.hpp)
set(core_SRCS
# MetaObjectHandler.cpp
  Probe.cpp
  ProbeGuard.cpp
  UserEventFilter.cpp
  QuickEventFilter.cpp
  WidgetEventFilter.cpp
  LastEvent.cpp
  UserVerificationFilter.cpp
  utils/FilterUtils.cpp
  utils/Tools.cpp)
qt5_wrap_cpp(gen_SRCS ${core_MOC_HDRS})

add_library(core SHARED ${core_SRCS}
                        ${core_HDRS}
                        ${gen_SRCS})
set_target_properties(core PROPERTIES PREFIX ${QTADA_LIB_PREFIX}
                                      OUTPUT_NAME ${QTADA_CORE_BASENAME}
                                      LIBRARY_OUTPUT_DIRECTORY ${QTADA_LIB_DIR}
                                      RUNTIME_OUTPUT_DIRECTORY ${QTADA_LIB_DIR})
target_link_libraries(core PRIVATE common
                                   inprocess
                                   Qt5::Core
                                   Qt5::CorePrivate
                                   Qt5::Quick
                                   Qt5::Widgets
                                   Qt5::RemoteObjects)
target_include_directories(core PRIVATE ${QTADA_COMMON_INCLUDE_DIR})
