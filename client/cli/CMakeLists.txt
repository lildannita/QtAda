set(qtada_SRCS main.cpp)

add_executable(qtada ${qtada_SRCS})
set_target_properties(qtada PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${QTADA_BIN_DIR})
target_link_libraries(qtada PRIVATE launcher
                                    qtadaGUI)
target_include_directories(qtada PRIVATE ${QTADA_LAUNCHER_INCLUDE_DIR}
                                         ${QTADA_GUI_INCLUDE_DIR})
