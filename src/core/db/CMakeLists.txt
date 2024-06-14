# Add subdirectories for each component
add_subdirectory(arraydb)
add_subdirectory(bpdb)
add_subdirectory(naivedb)

set(LIBS core_db_bpdb core_db_naivedb)

set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/db_manager.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/db_manager.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})
