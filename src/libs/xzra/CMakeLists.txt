set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/xzra.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/xzra.c)

add_library(xzra STATIC ${HEADERS} ${SOURCES})
target_link_libraries(xzra PRIVATE common_flags)
find_package(liblzma 5.4.0 REQUIRED CONFIG)
target_link_libraries(xzra PRIVATE liblzma::liblzma)
