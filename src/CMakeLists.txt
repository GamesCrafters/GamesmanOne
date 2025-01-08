# Check and enable link-time optimizations for "Release" builds if possible.
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_support_error)
if(NOT ipo_supported)
  message(STATUS "IPO / LTO not supported: <${ipo_support_error}>")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "IPO / LTO disabled for debugging")
else()
  message(STATUS "IPO / LTO enabled")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif()

# Define the executable.
add_executable(gamesman main.c)

# Add common flags.
target_link_libraries(gamesman PRIVATE common_flags)

# Check dependent packages.
list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/res") # Add directory for additional libraries
find_package(json-c REQUIRED CONFIG) # json-c
if(NOT DISABLE_OPENMP) # OpenMP
  find_package(OpenMP)
  if(OpenMP_FOUND)
    message(STATUS "OpenMP multithreading enabled")
  else()
    message(STATUS "OpenMP not found, configuring without multithreading")
  endif()
else()
  message(STATUS "OpenMP multithreading disabled")
endif()
if(USE_MPI) # MPI
  find_package(MPI REQUIRED)
  message(STATUS "MPI enabled")
  target_compile_definitions(gamesman PRIVATE USE_MPI)
endif()

# Execute subdirectories.
add_subdirectory(core)
add_subdirectory(games)
add_subdirectory(libs)

# Link libraries.
target_link_libraries(gamesman PRIVATE json-c::json-c) # json-c
if(OpenMP_FOUND) # OpenMP (optional)
  target_link_libraries(gamesman PRIVATE OpenMP::OpenMP_C)
endif()
if(USE_MPI) # MPI (optional)
  target_link_libraries(gamesman PRIVATE MPI::MPI_C)
endif()
