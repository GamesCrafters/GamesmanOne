#include <benchmark/benchmark.h>
#include <emmintrin.h>

#include <array>
#include <cstdint>
#include <random>
#include <vector>

extern "C" {
#include "core/hash/x86_simd_two_piece.h"
#include "core/types/base.h"
}

// ============================================================================
// Benchmark Fixture Setup
// ============================================================================
class TwoPieceHashFixture : public benchmark::Fixture {
   public:
    static constexpr int kRows = 5;
    static constexpr int kCols = 5;
    static constexpr int kNumX = 10;
    static constexpr int kNumO = 10;
    static constexpr size_t kNumSamples = 65536;
    static constexpr size_t kSampleMask = kNumSamples - 1;

    // Wrapper to avoid -Wignored-attributes
    struct SimdBoard {
        alignas(16) __m128i vec;
    };

    std::vector<SimdBoard> random_boards_simd;
    std::vector<std::array<uint64_t, 2>> random_boards_mem;
    std::vector<Position> random_hashes;
    std::vector<int> random_turns;
    X86SimdTwoPieceHashContext context;

    void SetUp(::benchmark::State& state) override {
        if (X86SimdTwoPieceHashContextInit(&context, kRows, kCols) != 0) {
            state.SkipWithError(
                "Failed to initialize hash context. Skipping...");
            return;
        }

        int64_t total_positions =
            X86SimdTwoPieceHashGetNumPositions(&context, kNumX, kNumO);

        std::mt19937_64 rng(42);
        std::uniform_int_distribution<Position> dist(0, total_positions - 1);

        random_boards_simd.reserve(kNumSamples);
        random_boards_mem.reserve(kNumSamples);
        random_hashes.reserve(kNumSamples);
        random_turns.reserve(kNumSamples);

        for (size_t i = 0; i < kNumSamples; ++i) {
            Position hash = dist(rng);
            int turn = X86SimdTwoPieceHashGetTurn(hash);

            random_hashes.push_back(hash);
            random_turns.push_back(turn);

            random_boards_simd.push_back(
                {X86SimdTwoPieceHashUnhash(&context, hash, kNumX, kNumO)});

            std::array<uint64_t, 2> mem_board;
            X86SimdTwoPieceHashUnhashMem(&context, hash, kNumX, kNumO,
                                         mem_board.data());
            random_boards_mem.push_back(mem_board);
        }
    }

    // Commented out unused parameter name
    void TearDown(const ::benchmark::State& /*state*/) override {
        X86SimdTwoPieceHashContextDestroy(&context);
    }
};

// ============================================================================
// 1. Benchmark: X86SimdTwoPieceHashHash (SIMD Register Input)
// ============================================================================
BENCHMARK_F(TwoPieceHashFixture, BM_HashRegister)(benchmark::State& state) {
    size_t idx = 0;
    for (auto _ : state) {
        Position hash = X86SimdTwoPieceHashHash(
            &context, random_boards_simd[idx].vec, random_turns[idx]);
        benchmark::DoNotOptimize(hash);

        // Bitwise AND is faster than modulo for powers of 2
        idx = (idx + 1) & kSampleMask;
    }
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// 2. Benchmark: X86SimdTwoPieceHashHashMem (Memory Buffer Input)
// ============================================================================
BENCHMARK_F(TwoPieceHashFixture, BM_HashMem)(benchmark::State& state) {
    size_t idx = 0;
    for (auto _ : state) {
        Position hash = X86SimdTwoPieceHashHashMem(
            &context, random_boards_mem[idx].data(), random_turns[idx]);
        benchmark::DoNotOptimize(hash);

        idx = (idx + 1) & kSampleMask;
    }
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// 3. Benchmark: X86SimdTwoPieceHashUnhash (Unhashing to SIMD Register)
// ============================================================================
BENCHMARK_F(TwoPieceHashFixture, BM_UnhashRegister)(benchmark::State& state) {
    size_t idx = 0;
    for (auto _ : state) {
        __m128i unhashed_board = X86SimdTwoPieceHashUnhash(
            &context, random_hashes[idx], kNumX, kNumO);
        benchmark::DoNotOptimize(unhashed_board);

        idx = (idx + 1) & kSampleMask;
    }
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// 4. Benchmark: X86SimdTwoPieceHashUnhashMem (Unhashing to Memory Buffer)
// ============================================================================
BENCHMARK_F(TwoPieceHashFixture, BM_UnhashMem)(benchmark::State& state) {
    size_t idx = 0;
    uint64_t out_patterns[2];

    for (auto _ : state) {
        X86SimdTwoPieceHashUnhashMem(&context, random_hashes[idx], kNumX, kNumO,
                                     out_patterns);
        benchmark::DoNotOptimize(out_patterns);
        benchmark::ClobberMemory();  // Ensures memory store isn't optimized
                                     // away

        idx = (idx + 1) & kSampleMask;
    }
    state.SetItemsProcessed(state.iterations());
}
