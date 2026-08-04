add_library_test(data_structures_test data_structures
    SOURCES
    bitset_test.cpp
    hash_test.cpp
    int64_array_test.cpp
    int64_hash_set_test.cpp
    u64x2_hash_set_test.cpp
    COMPILE_DEFINITIONS
    $<$<BOOL:${GAMESMAN_ENABLE_TSAN}>:GAMESMAN_TSAN_ENABLED>
    TIMEOUT
    10
)
