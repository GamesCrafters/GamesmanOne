add_library_test(data_structures_test data_structures
    SOURCES
    bitset_test.cpp
    hash_test.cpp
    COMPILE_DEFINITIONS
    $<$<BOOL:${ENABLE_TSAN}>:GAMESMAN_TSAN_ENABLED>
)
