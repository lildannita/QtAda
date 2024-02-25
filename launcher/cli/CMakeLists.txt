set(qtada_SRCS main.cpp)

add_executable(qtada ${qtada_SRCS})

target_link_libraries(qtada PRIVATE launcher)

target_include_directories(qtada PRIVATE ${QTADA_LAUNCHER_INCLUDE_DIR})

set_target_properties(qtada PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                       ${QTADA_BIN_DIR})
