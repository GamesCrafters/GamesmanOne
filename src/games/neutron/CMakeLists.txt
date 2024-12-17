set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/neutron.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/neutron.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})
