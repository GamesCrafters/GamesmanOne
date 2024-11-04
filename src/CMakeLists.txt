# Define the executable.
add_executable(gamesman main.c)
target_compile_options(gamesman PRIVATE
  $<$<C_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# Check and enable link-time optimizations if possible.
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_support_error)
if(NOT ipo_supported)
  message(STATUS "IPO / LTO not supported: <${ipo_support_error}>")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "IPO / LTO disabled for debugging")
else()
  message(STATUS "IPO / LTO enabled")
  set_property(TARGET gamesman PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Enable static analyzer if possible when build type is "Debug"
include(CheckCCompilerFlag)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")

  # GCC
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # Check if the C compiler supports -fanalyzer
    check_c_compiler_flag(-fanalyzer HAS_C_ANALYZER_FLAG)
    # Add the -fanalyzer flag if supported
    if(HAS_C_ANALYZER_FLAG)
      target_compile_options(gamesman PRIVATE $<$<COMPILE_LANGUAGE:C>:-fanalyzer>)
    endif()
  endif()

  # Clang Compiler
  if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "AppleClang")
    # Check if the compiler supports static analyzer flags
    # Clang's static analyzer is typically invoked via scan-build, but we can use analyzer flags
    check_c_compiler_flag(-fsyntax-only HAS_C_ANALYZER_FLAG)
    # Add the flags if supported
    if(HAS_C_ANALYZER_FLAG)
      target_compile_options(gamesman PRIVATE $<$<COMPILE_LANGUAGE:C>:-fsyntax-only -Wunused -Wuninitialized>)
    endif()
  endif()

  # MSVC Compiler
  if(MSVC)
    # For MSVC, enable code analysis with /analyze flag
    check_c_compiler_flag("/analyze" HAS_C_ANALYZER_FLAG)
    if(HAS_C_ANALYZER_FLAG)
      target_compile_options(gamesman PRIVATE $<$<COMPILE_LANGUAGE:C>:/analyze>)
    endif()
  endif()

  # Add other compilers and flags as necessary
endif()

# Check dependent packages.
find_package(ZLIB REQUIRED) # ZLIB
list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/res") # Add directory for additional libraries
find_package(json-c REQUIRED CONFIG) # json-c
find_package(liblzma 5.4.0 REQUIRED CONFIG) # liblzma
find_package(lz4 1.10.0 REQUIRED CONFIG) # liblz4
if(NOT DISABLE_OPENMP) # OpenMP
  find_package(OpenMP)
endif()
if(USE_MPI) # MPI
  find_package(MPI REQUIRED)
  target_compile_definitions(gamesman PRIVATE USE_MPI)
endif()

# Add subdirectories for each component.
add_subdirectory(core)
add_subdirectory(games)
add_subdirectory(libs)

# Link libraries.
target_link_libraries(gamesman PRIVATE ZLIB::ZLIB) # ZLIB
target_link_libraries(gamesman PRIVATE json-c::json-c) # json-c
target_link_libraries(gamesman PRIVATE lz4::lz4) # liblz4
target_link_libraries(gamesman PRIVATE liblzma::liblzma) # liblzma
if(OpenMP_FOUND) # OpenMP (optional)
  target_link_libraries(gamesman PRIVATE OpenMP::OpenMP_C)
endif()
if(USE_MPI) # MPI (optional)
  target_link_libraries(gamesman PRIVATE MPI::MPI_C)
endif()
