# GCC
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # Check if the C compiler supports -fanalyzer
    check_c_compiler_flag(-fanalyzer HAS_C_ANALYZER_FLAG)
    # Add the -fanalyzer flag if supported
    if(HAS_C_ANALYZER_FLAG)
        message(STATUS "Found static analyzer flag: -fanalyzer")
        target_compile_options(common_flags INTERFACE $<$<COMPILE_LANGUAGE:C>:-fanalyzer>)
    else()
        message(WARNING "Failed to find static analyzer flag for GNU compiler. Compile-time static analysis disabled.")
    endif()
endif()

# Clang Compiler
if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "AppleClang")
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
        set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_EXE}" "-checks=-*,bugprone-*,performance-*,warning-*")
    else()
        message(WARNING "clang-tidy not found. Compile-time static analysis disabled.")
    endif()
endif()

# Add other compilers and flags as necessary
