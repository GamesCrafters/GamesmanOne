# Add subdirectories for each component
add_subdirectory(dshogi)
add_subdirectory(fsvp)
add_subdirectory(gates)
add_subdirectory(gobblet)
add_subdirectory(kaooa)
add_subdirectory(mallqueenschess)
add_subdirectory(mttt)
add_subdirectory(mtttier)
add_subdirectory(neutron)
add_subdirectory(quixo)
add_subdirectory(teeko)
add_subdirectory(winkers)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/game_list.h)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/game_list.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})
