add_subdirectory(arraydb)

target_sources(gamesman_test PRIVATE
    db_manager_test.cpp
)
