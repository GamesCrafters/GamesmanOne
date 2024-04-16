# Define the executable
add_executable(gamesman main.c)

target_link_libraries(gamesman PRIVATE json-c::json-c ZLIB::ZLIB)

find_package(OpenMP)
if(OpenMP_FOUND)
  target_link_libraries(gamesman PRIVATE OpenMP::OpenMP_C)
endif()

# Add subdirectories for each component
add_subdirectory(core)
add_subdirectory(games)
add_subdirectory(libs)

set(LIBS core games libs)
