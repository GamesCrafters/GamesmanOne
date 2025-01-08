set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/gz64.h
            ${CMAKE_CURRENT_SOURCE_DIR}/mgz.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/gz64.c
            ${CMAKE_CURRENT_SOURCE_DIR}/mgz.c)

add_library(mgz STATIC ${HEADERS} ${SOURCES})
target_link_libraries(mgz PRIVATE common_flags)
