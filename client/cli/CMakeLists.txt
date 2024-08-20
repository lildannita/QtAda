set(qtada_SRCS main.cpp)

add_executable(qtada ${qtada_SRCS})
set_target_properties(qtada PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${QTADA_BIN_DIR})
target_link_libraries(qtada PRIVATE common
                                    launcher
                                    Qt5::Core
                                    Qt5::Widgets)
target_include_directories(qtada PRIVATE ${QTADA_COMMON_INCLUDE_DIR}
                                         ${QTADA_LAUNCHER_INCLUDE_DIR})
if (BUILD_QTADA_CLIENT)
  target_link_libraries(qtada PRIVATE gui)
  target_include_directories(qtada PRIVATE ${QTADA_GUI_INCLUDE_DIR})
endif()

# TODO: (разобраться позже) сейчас развертка в системе
#       происходит посредством установки ссылки в /usr/local/bin
#       через Bash, но это можно сделать подобным образом:
# install(TARGETS qtada
#         DESTINATION bin
#         PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
#                     GROUP_EXECUTE GROUP_READ
#                     WORLD_EXECUTE WORLD_READ)
