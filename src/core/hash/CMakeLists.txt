check_c_compiler_flag("-mbmi2" SUPPORTS_BMI2)

set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/generic_context.h
    ${CMAKE_CURRENT_SOURCE_DIR}/generic.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/generic_context.c
    ${CMAKE_CURRENT_SOURCE_DIR}/generic.c)

set(BMI2_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/x86_simd_two_piece.h)

set(BMI2_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/x86_simd_two_piece.c)

add_library(gamesman_hash STATIC ${HEADERS} ${SOURCES})
target_link_libraries(gamesman_hash PRIVATE common_flags)
target_link_libraries(gamesman_hash PRIVATE data_structures)
target_link_libraries(gamesman_hash PRIVATE gamesman_memory)
target_link_libraries(gamesman_hash PRIVATE gamesman_math)
if(SUPPORTS_BMI2)
    target_compile_options(gamesman_hash PRIVATE "-mbmi2")
    target_compile_options(gamesman_hash PRIVATE "-msse2")
    target_compile_options(gamesman_hash PRIVATE "-msse4.1")
    target_sources(gamesman_hash PRIVATE ${BMI2_HEADERS} ${BMI2_HEADERS})
    target_compile_definitions(gamesman_hash PRIVATE GAMESMAN_HAS_BMI2)
endif()
if(OpenMP_FOUND) # OpenMP (optional)
    target_link_libraries(gamesman_hash PRIVATE OpenMP::OpenMP_C)
endif()

target_link_libraries(gamesman_core PRIVATE gamesman_hash)
