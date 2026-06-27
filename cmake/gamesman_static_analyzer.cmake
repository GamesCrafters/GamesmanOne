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
    find_program(RUN_CLANG_TIDY_EXE NAMES "run-clang-tidy" "run-clang-tidy.py")
    if(CLANG_TIDY_EXE AND RUN_CLANG_TIDY_EXE)
        message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
        message(STATUS "Found run-clang-tidy: ${RUN_CLANG_TIDY_EXE}")
        # Write a .clang-tidy file that disables all checks
        file(WRITE "${CMAKE_BINARY_DIR}/.clang-tidy"
            "# Disable all checks in this folder.\nChecks: '-*'\n"
        )
        message(STATUS "Generated .clang-tidy ignore file for FetchContent dependencies.")

        # Create a custom target that does not compile code
        add_custom_target(tidy
            COMMAND ${RUN_CLANG_TIDY_EXE} -p ${CMAKE_BINARY_DIR}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running clang-tidy in parallel (No compilation)..."
        )
    else()
        message(WARNING "clang-tidy tools not found. Static analysis disabled.")
    endif()
endif()

# Add other compilers and flags as necessary
