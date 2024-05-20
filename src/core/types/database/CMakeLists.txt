set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/database.h
            ${CMAKE_CURRENT_SOURCE_DIR}/db_probe.h)

target_sources(gamesman PRIVATE ${HEADERS})
