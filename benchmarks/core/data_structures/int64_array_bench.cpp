#include <benchmark/benchmark.h>

#include <cstdint>

extern "C" {
#include "core/data_structures/int64_array.h"
}

// -----------------------------------------------------------------------------
// BM_Int64ArrayInit
// -----------------------------------------------------------------------------
static void BM_Int64ArrayInit(benchmark::State& state) {
    for (auto _ : state) {
        Int64Array array;
        Int64ArrayInit(&array);

        state.PauseTiming();
        Int64ArrayDestroy(&array);
        state.ResumeTiming();
    }
}
BENCHMARK(BM_Int64ArrayInit)->Unit(benchmark::kMicrosecond);

// -----------------------------------------------------------------------------
// BM_Int64ArrayInitCopy
// -----------------------------------------------------------------------------
static void BM_Int64ArrayInitCopy(benchmark::State& state) {
    const int64_t size = state.range(0);

    Int64Array src;
    Int64ArrayInit(&src);
    Int64ArrayResize(&src, size);

    for (auto _ : state) {
        Int64Array dest;
        Int64ArrayInitCopy(&dest, &src);

        state.PauseTiming();
        Int64ArrayDestroy(&dest);
        state.ResumeTiming();
    }

    Int64ArrayDestroy(&src);
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_Int64ArrayInitCopy)
    ->Arg(0)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Range(8, 1LL << 30)
    ->Unit(benchmark::kMicrosecond)
    ->Complexity();

// -----------------------------------------------------------------------------
// BM_Int64ArrayDestroy
// -----------------------------------------------------------------------------
static void BM_Int64ArrayDestroy(benchmark::State& state) {
    const int64_t size = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();
        Int64Array array;
        Int64ArrayInit(&array);
        Int64ArrayResize(&array, size);
        state.ResumeTiming();

        Int64ArrayDestroy(&array);
    }
    state.SetComplexityN(state.range(0));
}
// Limiting largest size to 4k because the runtime of BM_Int64ArrayDestroy is
// highly irregular and takes way too long to benchmark for larger sizes.
BENCHMARK(BM_Int64ArrayDestroy)
    ->Arg(0)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Range(8, 4LL << 10)
    ->Unit(benchmark::kMicrosecond)
    ->Complexity();

// -----------------------------------------------------------------------------
// BM_Int64ArrayPushBack
// -----------------------------------------------------------------------------
static void BM_Int64ArrayPushBack(benchmark::State& state) {
    const int64_t num_pushes = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();
        Int64Array array;
        Int64ArrayInit(&array);
        state.ResumeTiming();

        for (int64_t i = 0; i < num_pushes; ++i) {
            benchmark::DoNotOptimize(Int64ArrayPushBack(&array, i));
        }

        state.PauseTiming();
        Int64ArrayDestroy(&array);
        state.ResumeTiming();
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_Int64ArrayPushBack)
    ->Arg(0)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Range(8, 1LL << 30)
    ->Unit(benchmark::kMicrosecond)
    ->Complexity();

// -----------------------------------------------------------------------------
// BM_Int64ArrayResize
// -----------------------------------------------------------------------------
static void BM_Int64ArrayResize(benchmark::State& state) {
    const int64_t size = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();
        Int64Array array;
        Int64ArrayInit(&array);
        state.ResumeTiming();

        benchmark::DoNotOptimize(Int64ArrayResize(&array, size));

        state.PauseTiming();
        Int64ArrayDestroy(&array);
        state.ResumeTiming();
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_Int64ArrayResize)
    ->Arg(0)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Range(8, 1LL << 30)
    ->Unit(benchmark::kMicrosecond)
    ->Complexity();
