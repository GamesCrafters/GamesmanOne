set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/uwapi_regular.h
    ${CMAKE_CURRENT_SOURCE_DIR}/uwapi_tier.h
    ${CMAKE_CURRENT_SOURCE_DIR}/uwapi.h)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})
