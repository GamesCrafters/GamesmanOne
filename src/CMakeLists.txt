# Define the library that contains all application logic except the main entry point (main.c).
add_library(gamesman_core OBJECT)

# Add common flags.
target_link_libraries(gamesman_core PRIVATE common_flags)

# Execute subdirectories.
add_subdirectory(core)
add_subdirectory(games)
add_subdirectory(libs)

# Link libraries.
target_link_libraries(gamesman_core PRIVATE json-c) # json-c
if(OpenMP_FOUND) # OpenMP (optional)
  target_link_libraries(gamesman_core PRIVATE OpenMP::OpenMP_C)
endif()
if(USE_MPI) # MPI (optional)
  target_compile_definitions(gamesman_core PRIVATE USE_MPI)
  target_link_libraries(gamesman_core PRIVATE MPI::MPI_C)
endif()

# Define the GAMESMAN executable
add_executable(gamesman main.c)
target_link_libraries(gamesman PRIVATE gamesman_core)
if(OpenMP_FOUND) # The main function also uses _OPENMP
  target_link_libraries(gamesman PRIVATE OpenMP::OpenMP_C)
endif()
