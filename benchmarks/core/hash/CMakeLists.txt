add_executable(hash_bench
    simd_two_piece_bench.cpp
)

target_link_libraries(hash_bench PRIVATE
    benchmark::benchmark_main
    common_flags
    gamesman_hash
)
