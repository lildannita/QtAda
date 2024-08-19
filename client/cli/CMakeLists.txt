set(qtada_SRCS main.cpp)

add_executable(qtada ${qtada_SRCS})
set_target_properties(qtada PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${QTADA_BIN_DIR})
target_link_libraries(qtada PRIVATE common
                                    launcher
                                    gui
                                    Qt5::Core
                                    Qt5::Widgets)
target_include_directories(qtada PRIVATE ${QTADA_COMMON_INCLUDE_DIR}
                                         ${QTADA_LAUNCHER_INCLUDE_DIR}
                                         ${QTADA_GUI_INCLUDE_DIR})
install(TARGETS qtada
        DESTINATION ${QTADA_BIN_DIR}
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                    GROUP_EXECUTE GROUP_READ
                    WORLD_EXECUTE WORLD_READ)
