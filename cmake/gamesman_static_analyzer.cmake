# GCC
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # Check if the C compiler supports -fanalyzer
    check_c_compiler_flag(-fanalyzer HAS_C_ANALYZER_FLAG)
    # Add the -fanalyzer flag if supported
    if(HAS_C_ANALYZER_FLAG)
        target_compile_options(common_flags INTERFACE $<$<COMPILE_LANGUAGE:C>:-fanalyzer>)
    endif()
endif()

# Clang Compiler
if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "AppleClang")
    # Check if the compiler supports static analyzer flags
    # Clang's static analyzer is typically invoked via scan-build, but we can use analyzer flags
    check_c_compiler_flag(-fsyntax-only HAS_C_ANALYZER_FLAG)
    # Add the flags if supported
    if(HAS_C_ANALYZER_FLAG)
        target_compile_options(common_flags INTERFACE $<$<COMPILE_LANGUAGE:C>:-fsyntax-only -Wunused -Wuninitialized>)
    endif()
endif()

# MSVC Compiler
if(MSVC)
    # For MSVC, enable code analysis with /analyze flag
    check_c_compiler_flag("/analyze" HAS_C_ANALYZER_FLAG)
    if(HAS_C_ANALYZER_FLAG)
        target_compile_options(common_flags INTERFACE $<$<COMPILE_LANGUAGE:C>:/analyze>)
    endif()
endif()

# Add other compilers and flags as necessary
