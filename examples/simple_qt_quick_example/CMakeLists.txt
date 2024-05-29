set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(example_SOURCES
  main.cpp
  qml.qrc
  QmlAutoRecord.cpp
  QmlAutoRecord.hpp)

add_executable(simple_qt_quick_example ${example_SOURCES})
set_target_properties(simple_qt_quick_example PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${QTADA_EXAMPLES_BIN_DIR})
target_link_libraries(simple_qt_quick_example PRIVATE Qt5::Core
                                                      Qt5::Quick
                                                      Qt5::Test)
