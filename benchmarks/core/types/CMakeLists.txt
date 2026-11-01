add_executable(core_types_bench
    tier_position_hash_set_bench.cpp
    tier_position_static_hash_set_bench.cpp
)

target_link_libraries(core_types_bench PRIVATE
    benchmark::benchmark_main
    common_flags
    gamesman_core
)
