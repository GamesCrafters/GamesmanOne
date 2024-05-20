set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/gameplay_api_common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/gameplay_api_regular.h
    ${CMAKE_CURRENT_SOURCE_DIR}/gameplay_api_tier.h
    ${CMAKE_CURRENT_SOURCE_DIR}/gameplay_api.h)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})
