#include <benchmark/benchmark.h>
#include <stdint.h>

extern "C" {
#include "core/data_structures/bitset.h"
}

static void BM_BitsetSet(benchmark::State& state) {
    // Setup: Retrieve the size from the benchmark parameters
    int64_t num_bits = state.range(0);

    // Allocate the bitset outside the timed loop
    Bitset* bs = BitsetCreate(num_bits);
    int64_t index = 0;

    // The timed loop: Google Benchmark measures the time spent inside this loop
    for (auto _ : state) {
        bool prev = BitsetSet(bs, index);

        // Prevent the compiler from optimizing away the function call
        // by pretending we use the return value.
        benchmark::DoNotOptimize(prev);

        // Increment the index and wrap around to avoid out-of-bounds behavior
        index = (index + 1) % num_bits;
    }

    // Teardown: Clean up memory outside the timed loop
    BitsetDestroy(bs);
}

static void BM_BitsetTest(benchmark::State& state) {
    int64_t num_bits = state.range(0);
    Bitset* bs = BitsetCreate(num_bits);

    // Optional: Pre-fill some data if your test performance depends on bit
    // states
    BitsetSet(bs, num_bits / 2);

    int64_t index = 0;

    for (auto _ : state) {
        bool val = BitsetTest(bs, index);
        benchmark::DoNotOptimize(val);

        index = (index + 1) % num_bits;
    }

    BitsetDestroy(bs);
}

// Register the benchmarks and pass arguments (e.g., number of bits)
// Here, we test with 1,024 bits and 65,536 bits.
BENCHMARK(BM_BitsetSet)->Arg(1024)->Arg(65536);
BENCHMARK(BM_BitsetTest)->Arg(1024)->Arg(65536);
