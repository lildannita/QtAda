set(example_UI
  mainwindow.ui)
set(example_MOC_HEADERS
  mainwindow.h
  dialog.h
  customDelegate.h)
set(example_SOURCES
  ${example_MOC_HEADERS}
  main.cpp
  mainwindow.cpp)
qt5_wrap_ui(example_SOURCES ${example_UI})
qt5_wrap_cpp(example_SOURCES ${example_MOC_HEADERS})

add_executable(simple_qt_widget_example ${example_SOURCES}
    )
set_target_properties(simple_qt_widget_example PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${QTADA_EXAMPLES_BIN_DIR})
target_link_libraries(simple_qt_widget_example PRIVATE Qt5::Widgets)
