set(QTADA_PROJECT_BASENAME "project")

set(QTADA_PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

set(project_UI
  ProjectDialog.ui)
set(project_MOC_HDRS
  ProjectAPI.hpp
  ProjectDialog.hpp)
set(project_HDRS
  ${project_MOC_HDRS})
set(project_SRCS
  ProjectAPI.cpp
  ProjectDialog.cpp)

qt5_wrap_cpp(gen_SRCS ${project_MOC_HDRS})
qt5_wrap_ui(gen_SRCS ${project_UI})
qt5_add_resources(gen_SRCS ${QTADA_RESOURCES_DIR}/project/project.qrc)

add_library(project SHARED ${project_SRCS}
                           ${project_HDRS}
                           ${gen_SRCS})
set_target_properties(project PROPERTIES PREFIX ${QTADA_LIB_PREFIX}
                                         OUTPUT_NAME ${QTADA_PROJECT_BASENAME}
                                         LIBRARY_OUTPUT_DIRECTORY ${QTADA_LIB_DIR}
                                         RUNTIME_OUTPUT_DIRECTORY ${QTADA_LIB_DIR})
target_link_libraries(project PRIVATE common
                                      Qt5::Core
                                      Qt5::Widgets)
 target_include_directories(project PRIVATE ${QTADA_COMMON_INCLUDE_DIR})
