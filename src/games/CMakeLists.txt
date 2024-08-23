# Add subdirectories for each component
add_subdirectory(dshogi)
add_subdirectory(fsvp)
add_subdirectory(mallqueenschess)
add_subdirectory(mttt)
add_subdirectory(mtttier)
add_subdirectory(neutron)
add_subdirectory(quixo)
add_subdirectory(teeko)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/game_list.h)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/game_list.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})
