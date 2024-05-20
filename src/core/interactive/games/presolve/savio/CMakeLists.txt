set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/partition_select.h
            ${CMAKE_CURRENT_SOURCE_DIR}/script_setup.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/partition_select.c
            ${CMAKE_CURRENT_SOURCE_DIR}/script_setup.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})
