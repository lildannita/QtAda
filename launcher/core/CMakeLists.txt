set(QTADA_LAUNCHER_BASENAME "launcher")

set(injector_HDRS injector/AbstractInjector.hpp injector/ProcessInjector.hpp
                  injector/PreloadInjector.hpp)
set(injector_SRCS injector/AbstractInjector.cpp injector/ProcessInjector.cpp
                  injector/PreloadInjector.cpp)

set(launcher_HDRS
    Launcher.hpp
    LauncherUtils.hpp
    ProbeABI.hpp
    ProbeDetector.hpp
    LauncherLog.hpp
    LaunchOptions.hpp
    ${injector_HDRS})
set(launcher_SRCS
    Launcher.cpp
    LauncherUtils.cpp
    ProbeABI.cpp
    ProbeDetector.cpp
    LauncherLog.cpp
    LaunchOptions.cpp
    ${injector_SRCS})

qt5_wrap_cpp(launcher_SRCS ${launcher_HDRS} TARGET launcher)

add_library(launcher SHARED ${launcher_SRCS} ${launcher_HDRS})
set_target_properties(
  launcher
  PROPERTIES PREFIX ${QTADA_LIB_PREFIX}
             OUTPUT_NAME ${QTADA_LAUNCHER_BASENAME}
             LIBRARY_OUTPUT_DIRECTORY ${QTADA_LIB_DIR}
             RUNTIME_OUTPUT_DIRECTORY ${QTADA_LIB_DIR})
target_link_libraries(launcher PUBLIC Qt5::Core)

target_include_directories(launcher PUBLIC ${QTADA_LAUNCHER_INCLUDE_DIR})
