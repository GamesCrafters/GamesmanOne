add_executable(data_structures_bench
    bitset_bench.cpp
    int64_array_bench.cpp
)

target_link_libraries(data_structures_bench PRIVATE
    benchmark::benchmark_main
    data_structures
)
