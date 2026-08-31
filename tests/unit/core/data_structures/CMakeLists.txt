add_library_test(data_structures_test data_structures
    SOURCES
    bitset_test.cpp
    concurrent_bitset_test.cpp
    cstring_test.cpp
    hash_test.cpp
    int64_array_test.cpp
    int64_hash_set_test.cpp
    int64_queue_test.cpp
    int64_to_ptr_chained_hash_map_test.cpp
    u64x2_static_hash_set_test.cpp
    COMPILE_DEFINITIONS
    $<$<BOOL:${GAMESMAN_ENABLE_TSAN}>:GAMESMAN_TSAN_ENABLED>
    EXTRA_LIBS
    gamesman_memory
    TIMEOUT
    10
)
