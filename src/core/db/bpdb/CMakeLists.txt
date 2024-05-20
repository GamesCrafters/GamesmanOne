set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/bparray.h
    ${CMAKE_CURRENT_SOURCE_DIR}/bpdb_file.h
    ${CMAKE_CURRENT_SOURCE_DIR}/bpdb_lite.h
    ${CMAKE_CURRENT_SOURCE_DIR}/bpdb_probe.h
    ${CMAKE_CURRENT_SOURCE_DIR}/bpdict.h)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/bparray.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bpdb_file.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bpdb_lite.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bpdb_probe.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bpdict.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})
