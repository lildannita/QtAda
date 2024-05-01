set(QTADA_GUI_BASENAME "gui")

set(QTADA_GUI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

set(gui_UI
  MainGui.ui)
set(gui_MOC_HDRS
  InitDialog.hpp
  MainGui.hpp)
set(gui_HDRS
  ${gui_MOC_HDRS}
  GuiTools.hpp)
set(gui_SRCS
  InitDialog.cpp
  MainGui.cpp
  GuiTools.cpp)
qt5_wrap_cpp(gen_SRCS ${gui_MOC_HDRS})
qt5_wrap_ui(gen_SRCS ${gui_UI})
qt5_add_resources(gen_SRCS ${QTADA_RESOURCES_DIR}/main.qrc)

add_library(gui SHARED ${gui_SRCS}
                       ${gui_HDRS}
                       ${gen_SRCS})
set_target_properties(gui PROPERTIES PREFIX ${QTADA_LIB_PREFIX}
                                     OUTPUT_NAME ${QTADA_GUI_BASENAME}
                                     LIBRARY_OUTPUT_DIRECTORY ${QTADA_LIB_DIR}
                                     RUNTIME_OUTPUT_DIRECTORY ${QTADA_LIB_DIR})
target_link_libraries(gui PRIVATE common
                                  inprocess
                                  launcher
                                  Qt5::Core
                                  Qt5::Widgets)
target_include_directories(gui PRIVATE ${QTADA_COMMON_INCLUDE_DIR}
                                       ${QTADA_LAUNCHER_INCLUDE_DIR})
