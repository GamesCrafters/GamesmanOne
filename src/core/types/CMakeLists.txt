# Add subdirectories for each component
add_subdirectory(database)
add_subdirectory(game)
add_subdirectory(gameplay_api)
add_subdirectory(solver)
add_subdirectory(uwapi)

set(LIBS core_types_database core_types_game core_types_gameplay_api
         core_types_solver core_types_uwapi)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/base.h
    ${CMAKE_CURRENT_SOURCE_DIR}/gamesman_error.h
    ${CMAKE_CURRENT_SOURCE_DIR}/gamesman_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/move_array.h
    ${CMAKE_CURRENT_SOURCE_DIR}/position_array.h
    ${CMAKE_CURRENT_SOURCE_DIR}/position_hash_set.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_array.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_hash_map.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_hash_set.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_position_array.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_position_hash_set.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_queue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_stack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_to_ptr_chained_hash_map.h)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/move_array.c
    ${CMAKE_CURRENT_SOURCE_DIR}/position_array.c
    ${CMAKE_CURRENT_SOURCE_DIR}/position_hash_set.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_array.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_hash_map.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_hash_set.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_position_array.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_position_hash_set.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_stack.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_to_ptr_chained_hash_map.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})
