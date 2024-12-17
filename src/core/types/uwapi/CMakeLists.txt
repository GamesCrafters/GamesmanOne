set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/autogui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/partmove_array.h
    ${CMAKE_CURRENT_SOURCE_DIR}/partmove.h
    ${CMAKE_CURRENT_SOURCE_DIR}/uwapi_regular.h
    ${CMAKE_CURRENT_SOURCE_DIR}/uwapi_tier.h
    ${CMAKE_CURRENT_SOURCE_DIR}/uwapi.h)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/autogui.c
    ${CMAKE_CURRENT_SOURCE_DIR}/partmove_array.c
    ${CMAKE_CURRENT_SOURCE_DIR}/partmove.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})
