set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/context.h
            ${CMAKE_CURRENT_SOURCE_DIR}/generic_hash.h
            ${CMAKE_CURRENT_SOURCE_DIR}/two_piece.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/context.c
            ${CMAKE_CURRENT_SOURCE_DIR}/generic_hash.c
            ${CMAKE_CURRENT_SOURCE_DIR}/two_piece.c)

add_library(generic_hash STATIC ${HEADERS} ${SOURCES})
target_link_libraries(generic_hash PRIVATE common_flags)
target_link_libraries(generic_hash PRIVATE data_structures)
target_link_libraries(generic_hash PRIVATE misc)
check_c_compiler_flag("-mbmi2" SUPPORTS_BMI2)
if(SUPPORTS_BMI2)
    target_compile_options(generic_hash PRIVATE "-mbmi2")
    target_compile_definitions(generic_hash PRIVATE GAMESMAN_HAS_BMI2)
endif()

target_link_libraries(gamesman PRIVATE generic_hash)
