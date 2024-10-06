# Define the executable.
add_executable(gamesman main.c)
target_compile_options(gamesman PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# Check dependent packages.
find_package(PkgConfig REQUIRED)                            # pkg-config
pkg_check_modules(lz4 REQUIRED IMPORTED_TARGET liblz4)      # (pkg-config check) liblz4
find_package(ZLIB REQUIRED)                                 # ZLIB

# Check additional required libraries.
list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/res")  # Add directory for additional libraries
find_package(json-c REQUIRED CONFIG)                        # json-c
find_package(liblzma 5.4.0 REQUIRED CONFIG)                 # liblzma

# Check optional libraries.
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
target_link_libraries(gamesman PRIVATE PkgConfig::lz4)         # PkgConfig::lz4
target_link_libraries(gamesman PRIVATE ZLIB::ZLIB)             # ZLIB
target_link_libraries(gamesman PRIVATE json-c::json-c)         # json-c
target_link_libraries(gamesman PRIVATE liblzma::liblzma) # liblzma
if(OpenMP_FOUND)                                               # OpenMP (optional)
  target_link_libraries(gamesman PRIVATE OpenMP::OpenMP_C)
endif()
if(USE_MPI)                                                    # MPI (optional)
target_link_libraries(gamesman PRIVATE MPI::MPI_C)
endif()
