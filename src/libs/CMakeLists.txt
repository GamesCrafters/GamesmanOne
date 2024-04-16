# Add subdirectories for each component
add_subdirectory(mgz)
add_subdirectory(mt19937)

set(LIBS libs_mgz libs_mt19937)

add_library(libs INTERFACE)
target_link_libraries(libs INTERFACE ${LIBS})
