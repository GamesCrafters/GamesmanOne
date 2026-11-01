set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/xstring.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/xstring.c)

add_library(gamesman_string STATIC ${HEADERS} ${SOURCES})
target_link_libraries(gamesman_string PRIVATE common_flags)
