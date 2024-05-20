set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/game_variant_option.h
    ${CMAKE_CURRENT_SOURCE_DIR}/game_variant.h
    ${CMAKE_CURRENT_SOURCE_DIR}/game.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/game_variant.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})
