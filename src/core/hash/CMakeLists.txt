check_c_compiler_flag("-mbmi2" SUPPORTS_BMI2)

set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/generic_context.h
            ${CMAKE_CURRENT_SOURCE_DIR}/generic.h
            ${CMAKE_CURRENT_SOURCE_DIR}/two_piece.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/generic_context.c
            ${CMAKE_CURRENT_SOURCE_DIR}/generic.c
            ${CMAKE_CURRENT_SOURCE_DIR}/two_piece.c)

set (BMI2_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/x86_simd_two_piece.h)

set (BMI2_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/x86_simd_two_piece.c)

add_library(generic_hash STATIC ${HEADERS} ${SOURCES})
target_link_libraries(generic_hash PRIVATE common_flags)
target_link_libraries(generic_hash PRIVATE data_structures)
target_link_libraries(generic_hash PRIVATE misc)
if(SUPPORTS_BMI2)
    target_compile_options(generic_hash PRIVATE "-mbmi2")
    target_compile_options(generic_hash PRIVATE "-msse2")
    target_sources(generic_hash PRIVATE ${BMI2_HEADERS} ${BMI2_HEADERS})
    target_compile_definitions(generic_hash PRIVATE GAMESMAN_HAS_BMI2)
endif()

target_link_libraries(gamesman PRIVATE generic_hash)
