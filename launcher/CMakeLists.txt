set(QTADA_LAUNCHER_BASENAME "launcher")

set(QTADA_LAUNCHER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

set(injector_MOC_HDRS
  injector/AbstractInjector.hpp
  injector/ProcessInjector.hpp
  injector/PreloadInjector.hpp
)
set(injector_SRCS
  injector/AbstractInjector.cpp
  injector/ProcessInjector.cpp
  injector/PreloadInjector.cpp
)
set(launcher_MOC_HDRS
  ${injector_MOC_HDRS}
  Launcher.hpp
)
set(launcher_HDRS
  ${launcher_MOC_HDRS}
  LauncherUtils.hpp
  ProbeABI.hpp
  ProbeDetector.hpp
  LauncherLog.hpp
  LaunchOptions.hpp
)
set(launcher_SRCS
  ${injector_SRCS}
  Launcher.cpp
  LauncherUtils.cpp
  ProbeABI.cpp
  ProbeDetector.cpp
  LauncherLog.cpp
  LaunchOptions.cpp
)
qt5_wrap_cpp(gen_SRCS ${launcher_MOC_HDRS})

add_library(launcher SHARED ${launcher_SRCS}
                            ${launcher_HDRS}
                            ${gen_SRCS})
set_target_properties(launcher PROPERTIES PREFIX ${QTADA_LIB_PREFIX}
                                          OUTPUT_NAME ${QTADA_LAUNCHER_BASENAME}
                                          LIBRARY_OUTPUT_DIRECTORY ${QTADA_LIB_DIR}
                                          RUNTIME_OUTPUT_DIRECTORY ${QTADA_LIB_DIR})
target_link_libraries(launcher PRIVATE common
                                       inprocess
                                       Qt5::Core
                                       Qt5::Widgets
                                       Qt5::Network)
target_include_directories(launcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                            ${QTADA_COMMON_INCLUDE_DIR}
                                            ${QTADA_INPROCESS_INCLUDE_DIR})
