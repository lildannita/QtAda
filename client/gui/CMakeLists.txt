set(QTADA_GUI_BASENAME "qtadaGUI")
set(QTADA_GUI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

set(gui_UI
  MainWindow.ui
)
set(gui_MOC_HDRS
  MainWindow.hpp
)
set(gui_HDRS
  ${gui_MOC_HDRS})
set(gui_SRCS
  MainWindow.cpp
)
qt5_wrap_cpp(gui_SRCS ${gui_MOC_HDRS} TARGET qtadaGUI)
qt5_wrap_ui(gui_SRCS ${gui_UI})

add_library(qtadaGUI SHARED ${gui_SRCS} ${gui_HDRS})
set_target_properties(
  qtadaGUI
  PROPERTIES PREFIX ${QTADA_LIB_PREFIX}
             OUTPUT_NAME ${QTADA_GUI_BASENAME}
             LIBRARY_OUTPUT_DIRECTORY ${QTADA_LIB_DIR}
             RUNTIME_OUTPUT_DIRECTORY ${QTADA_LIB_DIR})
target_link_libraries(qtadaGUI PRIVATE core
                                       launcher
                                PUBLIC Qt5::Widgets)
target_include_directories(qtadaGUI PRIVATE ${QTADA_LAUNCHER_INCLUDE_DIR})
