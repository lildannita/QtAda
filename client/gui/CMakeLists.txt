set(client_UI
#  mainwindow.ui
)
set(client_MOC_HDRS
#  mainwindow.h
)
set(client_HDRS
  ${client_MOC_HDRS})
set(client_SRCS
  main.cpp
#  mainwindow.cpp
)
qt5_wrap_cpp(client_SRCS ${client_MOC_HDRS} TARGET qtada-client)
qt5_wrap_ui(client_SRCS ${client_UI})

add_executable(qtada-client ${client_SRCS} ${client_HDRS})
set_target_properties(qtada-client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${QTADA_BIN_DIR})
target_link_libraries(qtada-client PRIVATE core
                                           launcher
                                           Qt5::Widgets)
