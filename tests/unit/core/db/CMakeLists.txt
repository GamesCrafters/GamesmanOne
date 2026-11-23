add_subdirectory(arraydb)

target_sources(gamesman_test PRIVATE
    database_test.cpp
    db_manager_test.cpp
)
