#include <benchmark/benchmark.h>

#include <algorithm>
#include <random>
#include <vector>

extern "C" {
#include "core/types/base.h"
#include "core/types/tier_position_static_hash_set.h"
}

// Helper function to generate N elements where ~75% are duplicates.
static std::vector<TierPosition> GenerateElements(int N) {
    std::vector<TierPosition> elements;
    elements.reserve(N);

    // To get ~75% duplicates, we need ~25% unique items.
    int unique_count = std::max(1, N / 4);

    for (int i = 0; i < N; ++i) {
        TierPosition tp;
        // Cycle through the unique elements to generate duplicates
        tp.tier = i % unique_count;
        tp.position =
            (i % unique_count) * 1000003LL;  // arbitrary hashing factor
        elements.push_back(tp);
    }

    // Shuffle to simulate realistic random insertion orders
    std::mt19937 rng(42);
    std::shuffle(elements.begin(), elements.end(), rng);

    return elements;
}

template <uint64_t N>
static void BM_TierPositionStaticHashSetAdd(benchmark::State& state) {
    // GenerateElements still takes N, but now N is a compile-time constant
    std::vector<TierPosition> elements = GenerateElements(N);

    for (auto _ : state) {
        // Now N is a constant expression, so no VLA warning and no narrowing
        // error!
        DECLARE_TIER_POSITION_STATIC_HASH_SET(set,
                                              static_cast<uint64_t>(N << 1));

        for (const auto& el : elements) {
            TierPositionStaticHashSetAdd(&set, el);
        }
    }

    state.SetItemsProcessed(state.iterations() * N);
}

// Register the benchmark for the specific sizes you want to test
BENCHMARK_TEMPLATE(BM_TierPositionStaticHashSetAdd, 2);
BENCHMARK_TEMPLATE(BM_TierPositionStaticHashSetAdd, 8);
BENCHMARK_TEMPLATE(BM_TierPositionStaticHashSetAdd, 32);
BENCHMARK_TEMPLATE(BM_TierPositionStaticHashSetAdd, 128);
BENCHMARK_TEMPLATE(BM_TierPositionStaticHashSetAdd, 512);
BENCHMARK_TEMPLATE(BM_TierPositionStaticHashSetAdd, 1024);
BENCHMARK_TEMPLATE(BM_TierPositionStaticHashSetAdd, 4096);
