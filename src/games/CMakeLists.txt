# Add subdirectories for each component
add_subdirectory(fsvp)
add_subdirectory(mallqueenschess)
add_subdirectory(mttt)
add_subdirectory(mtttier)
add_subdirectory(quixo)

set(LIBS games_fsvp games_mallqueenschess games_mttt games_mtttier games_quixo)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/game_list.h)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/game_list.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})
