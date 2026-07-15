set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/xfile.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/xfile.c)

add_library(gamesman_io STATIC ${HEADERS} ${SOURCES})
target_link_libraries(gamesman_io PRIVATE common_flags)
find_package(ZLIB REQUIRED)
target_link_libraries(gamesman_io PRIVATE ZLIB::ZLIB)
