# Add subdirectories for each component
add_subdirectory(lz4_utils)
add_subdirectory(mgz)
add_subdirectory(mt19937)
add_subdirectory(xzra)

set(LIBS lz4_utils mgz mt19937 xzra)

target_link_libraries(gamesman PRIVATE ${LIBS})
