set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/combinatorics.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/combinatorics.c)

add_library(gamesman_math STATIC ${HEADERS} ${SOURCES})
target_link_libraries(gamesman_math PRIVATE common_flags)
