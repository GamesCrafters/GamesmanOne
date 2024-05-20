# Add subdirectories for each component
add_subdirectory(mgz)
add_subdirectory(mt19937)

set(LIBS mgz mt19937)

target_link_libraries(gamesman PRIVATE ${LIBS})
