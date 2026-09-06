add_executable(data_structures_bench
    bitset_bench.cpp
    int64_array_bench.cpp
    int64_hash_set_bench.cpp
    u64x2_static_hash_set_bench.cpp
)

target_link_libraries(data_structures_bench PRIVATE
    benchmark::benchmark_main
    common_flags
    data_structures
)
