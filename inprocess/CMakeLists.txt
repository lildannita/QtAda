set(QTADA_INPROCESS_BASENAME "inprocess")

# set(QTADA_INPROCESS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

set(inprocess_REPS
  InprocessController.rep)
set(inprocess_MOC_HDRS
  InprocessController.hpp
  InprocessDialog.hpp
  PropertiesWatcher.hpp)
set(inprocess_HDRS
  ${inprocess_MOC_HDRS}
  InprocessTools.hpp)
set(inprocess_SRCS
#  InprocessController.cpp
  InprocessDialog.cpp
  PropertiesWatcher.cpp)
qt5_wrap_cpp(gen_SRCS ${inprocess_MOC_HDRS})
qt5_add_resources(gen_SRCS ${QTADA_RESOURCES_DIR}/icons.qrc)
qt5_generate_repc(gen_SRCS ${inprocess_REPS} SOURCE)
qt5_generate_repc(gen_SRCS ${inprocess_REPS} REPLICA)

add_library(inprocess SHARED ${inprocess_SRCS} ${inprocess_HDRS} ${inprocess_REPS} ${gen_SRCS})
set_target_properties(inprocess PROPERTIES PREFIX ${QTADA_LIB_PREFIX}
                                      OUTPUT_NAME ${QTADA_INPROCESS_BASENAME}
                                      LIBRARY_OUTPUT_DIRECTORY ${QTADA_LIB_DIR}
                                      RUNTIME_OUTPUT_DIRECTORY ${QTADA_LIB_DIR})
target_link_libraries(inprocess PRIVATE Qt5::Core
                                        Qt5::Widgets
                                        Qt5::RemoteObjects)
