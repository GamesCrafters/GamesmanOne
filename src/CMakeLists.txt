# Define the executable.
add_executable(gamesman main.c)
target_compile_options(gamesman PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# Check dependent packages.
find_package(ZLIB REQUIRED)                                 # ZLIB

# Check additional required libraries.
list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/res")  # Add directory for additional libraries
find_package(json-c REQUIRED CONFIG)                        # json-c
# Find the liblzma library

find_library(LIBLZMA_LIBRARY
    NAMES lzma
    HINTS /opt/homebrew/lib /usr/local/lib
    REQUIRED)

# Find the lzma.h header file
find_path(LIBLZMA_INCLUDE_DIR
    NAMES lzma.h
    HINTS /opt/homebrew/include /usr/local/include
    REQUIRED)

find_library(LZ4_LIBRARY
    NAMES lz4
    HINTS /opt/homebrew/lib /usr/local/lib /usr/lib
    REQUIRED)

find_path(LZ4_INCLUDE_DIR
    NAMES lz4.h lz4frame.h
    HINTS /opt/homebrew/include /usr/local/include /usr/include
    REQUIRED)

target_include_directories(gamesman PRIVATE ${LZ4_INCLUDE_DIR})
target_link_libraries(gamesman PRIVATE ${LZ4_LIBRARY})


# Include the directory containing lzma.h
target_include_directories(gamesman PRIVATE ${LIBLZMA_INCLUDE_DIR})

# Link the liblzma library to your target
target_link_libraries(gamesman PRIVATE ${LIBLZMA_LIBRARY})

# find_package(liblzma 5.4.0 REQUIRED CONFIG)                 # liblzma
# find_package(lz4 1.10.0 REQUIRED CONFIG)                    # liblz4

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
target_link_libraries(gamesman PRIVATE ZLIB::ZLIB)             # ZLIB
target_link_libraries(gamesman PRIVATE json-c::json-c)         # json-c
# target_link_libraries(gamesman PRIVATE lz4::lz4)               # liblz4
# target_link_libraries(gamesman PRIVATE liblzma::liblzma)       # liblzma
if(OpenMP_FOUND)                                               # OpenMP (optional)
  target_link_libraries(gamesman PRIVATE OpenMP::OpenMP_C)
endif()
if(USE_MPI)                                                    # MPI (optional)
target_link_libraries(gamesman PRIVATE MPI::MPI_C)
endif()
