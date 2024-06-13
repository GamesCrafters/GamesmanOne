# Define the executable.
add_executable(gamesman main.c)

# Check libraries.
find_package(ZLIB REQUIRED)                                 # ZLIB
find_package(LibLZMA REQUIRED)                              # LibLZMA (XZ Utils)
list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/res")  # (add) json-c
find_package(json-c REQUIRED CONFIG)                        # (check) json-c
if(NOT DISABLE_OPENMP)                                      # OpenMP
  find_package(OpenMP)
endif()
if(USE_MPI)                                                 # MPI
  find_package(MPI REQUIRED)
  target_compile_definitions(gamesman PRIVATE USE_MPI)
endif()

# Add subdirectories for each component.
add_subdirectory(core)
add_subdirectory(games)
add_subdirectory(libs)

# Link libraries.
target_link_libraries(gamesman PRIVATE ZLIB::ZLIB)          # ZLIB
target_link_libraries(gamesman PRIVATE LibLZMA::LibLZMA)    # LibLZMA (XZ Utils)
target_link_libraries(gamesman PRIVATE json-c::json-c)      # json-c
if(OpenMP_FOUND)                                            # OpenMP
  target_link_libraries(gamesman PRIVATE OpenMP::OpenMP_C)
endif()
if(USE_MPI)                                                 # MPI
target_link_libraries(gamesman PRIVATE MPI::MPI_C)
endif()
