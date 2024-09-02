# Define the executable.
add_executable(gamesman main.c)

# Add compiler warning flags.
target_compile_options(gamesman PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
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

# Check libraries.
find_package(PkgConfig REQUIRED)                            # pkg-config
pkg_check_modules(lz4 REQUIRED IMPORTED_TARGET liblz4)      # (pkg-config check) liblz4
find_package(ZLIB REQUIRED)                                 # ZLIB
find_package(LibLZMA REQUIRED)                              # LibLZMA (XZ Utils)
list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/res")  # (add) json-c
find_package(json-c REQUIRED CONFIG)                        # (check) json-c
if(NOT DISABLE_OPENMP)                                      # OpenMP
  find_package(OpenMP)
  if(OpenMP_FOUND)
    message(STATUS "OpenMP multithreading enabled")
  else()
    message(STATUS "OpenMP not found, configuring without multithreading")
  endif()
else()
  message(STATUS "OpenMP multithreading disabled")
endif()
if(USE_MPI)                                                 # MPI
  find_package(MPI REQUIRED)
  message(STATUS "MPI enabled")
  target_compile_definitions(gamesman PRIVATE USE_MPI)
endif()

# Add subdirectories for each component.
add_subdirectory(core)
add_subdirectory(games)
add_subdirectory(libs)

# Link libraries.
target_link_libraries(gamesman PRIVATE PkgConfig::lz4)      # PkgConfig::lz4
target_link_libraries(gamesman PRIVATE ZLIB::ZLIB)          # ZLIB
target_link_libraries(gamesman PRIVATE LibLZMA::LibLZMA)    # LibLZMA (XZ Utils)
target_link_libraries(gamesman PRIVATE json-c::json-c)      # json-c
if(OpenMP_FOUND)                                            # OpenMP
  target_link_libraries(gamesman PRIVATE OpenMP::OpenMP_C)
endif()
if(USE_MPI)                                                 # MPI
  target_link_libraries(gamesman PRIVATE MPI::MPI_C)
endif()
