set(CMAKE_AUTOMOC ON)

set(example_SOURCES
  main.cpp
  ExampleObject.hpp)

add_executable(simple_qobject_example ${example_SOURCES})
set_target_properties(simple_qobject_example PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${QTADA_EXAMPLES_BIN_DIR})
target_link_libraries(simple_qobject_example PRIVATE Qt5::Core)
