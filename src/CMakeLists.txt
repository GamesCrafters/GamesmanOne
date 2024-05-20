# Define the executable.
add_executable(gamesman main.c)

# Check ZLIB.
find_package(ZLIB REQUIRED)
target_link_libraries(gamesman PRIVATE ZLIB::ZLIB)

# Check json-c.
list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/res")
find_package(json-c REQUIRED CONFIG)
target_link_libraries(gamesman PRIVATE json-c::json-c)

# Check OpenMP.
if(NOT DISABLE_OPENMP)
  find_package(OpenMP)
  if(OpenMP_FOUND)
    target_link_libraries(gamesman PRIVATE OpenMP::OpenMP_C)
  endif()
endif()

# Check MPI.
if(USE_MPI)
  find_package(MPI REQUIRED)
  target_link_libraries(gamesman PRIVATE MPI::MPI_C)
  target_compile_definitions(gamesman PRIVATE USE_MPI)
endif()

# Add subdirectories for each component
add_subdirectory(core)
add_subdirectory(games)
add_subdirectory(libs)
