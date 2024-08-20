set(QTADA_COMMON_BASENAME "common")

set(QTADA_COMMON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

# TODO: Пока что оставляем видимость всех символов в этой библиотеке, так
# как по идее библиотека и является хранилищем "общих" данных и функций.
# Когда будет закончен рефакторинг, это нужно будет убрать.
set(CMAKE_C_VISIBILITY_PRESET default)
set(CMAKE_CXX_VISIBILITY_PRESET default)
set(CMAKE_VISIBILITY_INLINES_HIDDEN false)

set(common_HDRS Settings.hpp Common.hpp Paths.hpp)
set(common_SRCS Settings.cpp)

add_library(common SHARED ${common_SRCS} ${common_HDRS})
set_target_properties(common PROPERTIES PREFIX ${QTADA_LIB_PREFIX}
                                        OUTPUT_NAME ${QTADA_COMMON_BASENAME}
                                        LIBRARY_OUTPUT_DIRECTORY ${QTADA_LIB_DIR}
                                        RUNTIME_OUTPUT_DIRECTORY ${QTADA_LIB_DIR})
target_link_libraries(common PRIVATE Qt5::Core)
