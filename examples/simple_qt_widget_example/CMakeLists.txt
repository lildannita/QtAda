set(example_UI mainwindow.ui)

set(example_MOC_HEADERS mainwindow.h)

set(example_SOURCES main.cpp mainwindow.cpp ${example_MOC_HEADERS})

qt5_wrap_ui(example_SOURCES ${example_UI})
qt5_wrap_cpp(example_SOURCES ${example_MOC_HEADERS})

add_executable(simple_qt_widget_example ${example_SOURCES})
target_link_libraries(simple_qt_widget_example PRIVATE Qt5::Widgets)
set_target_properties(
  simple_qt_widget_example PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                      ${QTADA_EXAMPLES_BIN_DIR})
