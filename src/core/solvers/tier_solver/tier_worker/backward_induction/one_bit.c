/**
 * @file one_bit.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the One-Bit solving algorithm for the Tier Solver.
 * See one_bit.h for usage.
 * @version 1.0.0
 * @date 2025-06-23
 *
 * @copyright This file is part of GAMESMAN, The Finite, Two-person
 * Perfect-Information Game Generator released under the GPL:
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "core/solvers/tier_solver/tier_worker/backward_induction/one_bit.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "core/concurrency.h"
#include "core/constants.h"
#include "core/data_structures/bitset.h"
#include "core/data_structures/concurrent_bitset.h"
#include "core/db/arraydb/arraydb.h"
#include "core/db/db_manager.h"
#include "core/misc.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/base.h"
#include "core/types/database/db_probe.h"
#include "core/types/gamesman_error.h"
#include "core/types/tier_hash_map.h"
#include "core/types/tier_hash_set.h"
#include "libs/lz4_utils/lz4_utils.h"

// Read-only reference to the API functions from tier_manager.
static const TierSolverApi *api_internal;

// Maximum path length
enum { kMaxPathLength = 4096 };

#ifdef _OPENMP
// Number of tasks to generate for each loop iteration in omp taskloop
// constructs. Generating more tasks per thread improves workload distribution
// but increases task generation overhead.
enum { kOmpLoopTaskPerThread = 8 };

// Dummy variables for dependency management.
static struct {
    char cpu;     // Block by a previous task with high CPU usage
    char seg[3];  // Block by one of the three ArrayDb solving segment buffers
} dep;
#endif  // _OPENMP

static struct {
    // Database path prefix.
    const char *path_prefix;

    // Number of positions in each database compression block.
    int64_t db_chunk_size;

    // Number of threads available.
    int num_threads;

    // LZ4 compression level.
    int lz4_level;
} config;

// Current game information.
static struct {
    // The tier being solved.
    Tier tier;

    // Size of the tier being solved.
    int64_t tier_size;

    // Total number of child tiers.
    int num_child_tiers;

    // Array of all child tiers.
    Tier child_tiers[kTierSolverNumChildTiersMax];

    // Size of each child tier, parrallel to child_tiers.
    int64_t child_tier_sizes[kTierSolverNumChildTiersMax];

    // Prefix sum of child_tier_sizes.
    int64_t child_tier_size_offsets[kTierSolverNumChildTiersMax + 1];

    // Map from tiers in the current tier group (game.tier and child tiers) to
    // size offsets.
    TierHashMap tier_to_size_offset;
} game;

// Maximum discovered remotenesses.
static struct {
    ConcurrentInt win_lose;  // Max win/loss remoteness discovered.
    ConcurrentInt tie;       // Max tie remoteness discovered
    bool set;                // Whether the values have been set.
} max_remoteness;

// Settings and buffers for chunking the solving tier
static struct {
    int64_t size;        // Number of positions in each chunk.
    int64_t count;       // Total number of chunks.
    Bitset *seq_buf[2];  // Sequential bitsets for I/O buffering
} chunking;

// Shared in-memory random access bitset: one bit per position in the solving
// tier group, hence the name of the solving strategy.
static ConcurrentBitset *rand_bitset;

// ------------------------------ Step0Initialize ------------------------------

static void Step0_0SetupChildTiers(void) {
    Tier raw[kTierSolverNumChildTiersMax];
    int num_raw = api_internal->GetChildTiers(game.tier, raw);
    TierHashSet dedup;
    TierHashSetInit(&dedup, 0.5);
    TierHashMapInit(&game.tier_to_size_offset, 0.5);
    game.num_child_tiers = 0;
    game.child_tier_size_offsets[0] = game.tier_size;
    TierHashMapSet(&game.tier_to_size_offset, game.tier, 0);
    for (int i = 0; i < num_raw; ++i) {
        Tier canonical = api_internal->GetCanonicalTier(raw[i]);
        if (TierHashSetAdd(&dedup, canonical)) {
            // Push the canonical child tier into the array of child tiers.
            game.child_tiers[game.num_child_tiers] = canonical;

            // Set the size of the child tier.
            int64_t size = api_internal->GetTierSize(canonical);
            game.child_tier_sizes[game.num_child_tiers] = size;

            // Calculate next prefix sum for size offset into the random access
            // bitset.
            game.child_tier_size_offsets[game.num_child_tiers + 1] =
                game.child_tier_size_offsets[game.num_child_tiers] + size;

            // Put the current sum into the offset map.
            TierHashMapSet(&game.tier_to_size_offset, canonical,
                           game.child_tier_size_offsets[game.num_child_tiers]);
            ++game.num_child_tiers;
        }
    }
    TierHashSetDestroy(&dedup);
}

static int64_t NextMultiple(int64_t n, int64_t mult) {
    return RoundUpDivide(n, mult) * mult;
}

/**
 * @brief Returns the maximum number of positions in each chunk that the given
 * \p mem can handle.
 *
 * @details The solver may hold a chunk of the on-disk DB and a chunk of the
 * sequential access bitset at the same time. The ArrayDb uses
 * kArrayDbRecordSize bytes per position and the bitset uses 1/8 bytes per
 * position but with the final memory usage rounded to the next integral value
 * of bytes. The solver uses a rolling buffer so there will be at most 3 chunks
 * of DB and 2 chunks of the sequential bitset loaded at the same time. Let x be
 * the number of positions in each chunk. The memory requirement is therefore
 *
 *     3 * kArrayDbRecordSize * x + 2 * (x + 7) / 8 <= mem
 *
 * Rearranging the above inequality gives the formula that is used in this
 * function.
 */
static int64_t CalcChunkSize(size_t mem) {
    int64_t x = (mem * 4 - 7) / (12 * kArrayDbRecordSize + 1);

    // We need to process at least ceil(tier_size / x) chunks using rolling
    // buffers. Split the total number of positions into each chunk as evenly
    // as possible to maximize pipeline utilization.
    x = RoundUpDivide(game.tier_size, RoundUpDivide(game.tier_size, x));

    // Round the number of positions to the next multiple of 64, which is the
    // number of bits in each block in the Bitset implementation.
    return NextMultiple(x, 64);
}

static bool Step0_1AllocateMemory(size_t memlimit) {
    // Calculate the size of the current tier group.
    int64_t tier_group_size =
        game.tier_size + game.child_tier_size_offsets[game.num_child_tiers];

    // Size of the random access concurrent bitset
    size_t mem_bitset = ConcurrentBitsetMemRequired(tier_group_size);

    // Size of the database probes for each thread
    // TODO: do not calculate from DB implementation details; instead, the
    // database API should expose this.
    size_t mem_db_probes =
        config.num_threads * config.db_chunk_size * kArrayDbRecordSize;

    // Calculate the minimum required memory. The chunks for the solving tier
    // can be made arbitrarily small so they are not included here.
    size_t mem_required = mem_bitset + mem_db_probes;
    if (memlimit < mem_required) return false;

    // Calculate chunk size and number of chunks to be created. Then make sure
    // that we won't create too many chunk files on disk.
    chunking.size = CalcChunkSize(memlimit - mem_required);
    chunking.count = RoundUpDivide(game.tier_size, chunking.size);
    if (chunking.count > 4096) return false;

    // Allocate the random access concurrent bitset.
    rand_bitset = ConcurrentBitsetCreate(tier_group_size);
    if (!rand_bitset) return false;

    // Allocate DB and sequential access bitset rolling buffers
    if (DbManagerSegmentationMaxNumBuffers() < 3) return false;
    if (DbManagerSegmentationCreateBuffers(game.tier, 3, chunking.size) !=
        kNoError) {
        return false;
    }
    chunking.seq_buf[0] = BitsetCreate(chunking.size);
    chunking.seq_buf[1] = BitsetCreate(chunking.size);

    return chunking.seq_buf[0] && chunking.seq_buf[1];
}

static bool Step0Initialize(const TierSolverApi *api, int64_t db_chunk_size,
                            Tier tier, size_t memlimit) {
    // Set API and other constants.
    assert(api && api->GetCanonicalParentPositions);
    api_internal = api;
    config.db_chunk_size = db_chunk_size;
    config.num_threads = ConcurrencyGetOmpNumThreads();
    config.path_prefix = DbManagerGetPath();

    // Initialize max remoteness values to 0.
    ConcurrentIntInit(&max_remoteness.win_lose, 0);
    ConcurrentIntInit(&max_remoteness.tie, 0);
    max_remoteness.set = false;

    // Initialize the child tier array.
    game.tier = tier;
    game.tier_size = api_internal->GetTierSize(tier);
    Step0_0SetupChildTiers();

    // Plan memory usage ahead and create the random access concurrent bitset.
    return Step0_1AllocateMemory(memlimit);
}

// -------------------------- Step1ScanTierAndInitDb --------------------------

static int64_t I64Min(int64_t a, int64_t b) { return a < b ? a : b; }

static bool IsCanonicalPosition(Position pos) {
    TierPosition tp = {.tier = game.tier, .position = pos};
    return api_internal->GetCanonicalPosition(tp) == pos;
}

static void ScanDbChunk(int slot, int chunk) {
    Position begin_pos = chunk * chunking.size;
    Position end_pos = I64Min(begin_pos + chunking.size, game.tier_size);

    PRAGMA_OMP(taskloop num_tasks(config.num_threads * kOmpLoopTaskPerThread))
    for (Position pos = begin_pos; pos < end_pos; ++pos) {
        TierPosition tp = {.tier = game.tier, .position = pos};
        int64_t rec_idx = pos - begin_pos;

        // Assign (undecided, 0) to illegal positions and non-canonical
        // positions.
        if (!api_internal->IsLegalPosition(tp) || !IsCanonicalPosition(pos)) {
            DbManagerSegmentationSetValueRemoteness(slot, rec_idx, kUndecided,
                                                    0);
            continue;
        }

        Value val = api_internal->Primitive(tp);
        // Assign (draw, 0) to non-primitive positions.
        if (val == kUndecided) {
            DbManagerSegmentationSetValueRemoteness(slot, rec_idx, kDraw, 0);
            continue;
        }

        // If the position is primitive, assign its primitive value and
        // remoteness 0.
        DbManagerSegmentationSetValueRemoteness(slot, rec_idx, val, 0);
    }
}

static void Step1ScanTierAndInitDb(void) {
    PRAGMA_OMP(parallel)
    PRAGMA_OMP(single)
    for (int64_t i = 0; i < chunking.count; ++i) {
        int slot = i % 2;

        // Scan for illegal, non-canonical, and primitive positions
        PRAGMA_OMP(task depend(inout : dep.seg[slot], dep.cpu))
        ScanDbChunk(slot, i);

        // Write the chunk to disk
        PRAGMA_OMP(task depend(inout : dep.seg[slot]))
        DbManagerSegmentationFlush(slot, i);
    }
}

// ---------------------------- Step2IterateWinLose ----------------------------

static void GenerateParentsFromTierPosition(TierPosition child) {
    Position parents[kTierSolverNumParentPositionsMax];
    int num_parents =
        api_internal->GetCanonicalParentPositions(child, game.tier, parents);
    for (int i = 0; i < num_parents; ++i) {
        ConcurrentBitsetSet(rand_bitset, parents[i], memory_order_relaxed);
    }
}

static void GenerateParentsFromDbChunk(int slot, int chunk, Value val,
                                       int remoteness) {
    Position begin_pos = chunk * chunking.size;
    Position end_pos = I64Min(begin_pos + chunking.size, game.tier_size);

    PRAGMA_OMP(taskloop num_tasks(config.num_threads * kOmpLoopTaskPerThread))
    for (Position pos = begin_pos; pos < end_pos; ++pos) {
        // Skip position if its value or remoteness does not match
        int64_t rec_idx = pos - begin_pos;
        Value pos_val = DbManagerSegmentationGetValue(slot, rec_idx);
        if (pos_val != val) continue;
        int pos_remoteness = DbManagerSegmentationGetRemoteness(slot, rec_idx);
        if (pos_remoteness != remoteness) continue;

        TierPosition child = {.tier = game.tier, .position = pos};
        GenerateParentsFromTierPosition(child);
    }
}

static void GenerateParentsFromDbSolving(Value val, int remoteness) {
    PRAGMA_OMP(parallel)
    PRAGMA_OMP(single)
    for (int64_t i = 0; i < chunking.count; ++i) {
        int slot = i % 2;

        // Load a chunk of DB into memory
        PRAGMA_OMP(task depend(inout : dep.seg[slot]))
        DbManagerSegmentationLoad(slot, i);

        // Generate parents from loaded DB into random access bitset
        PRAGMA_OMP(task depend(inout : dep.seg[slot], dep.cpu))
        GenerateParentsFromDbChunk(slot, i, val, remoteness);
    }
}

static void UpdateMaxRemotenesses(Value val, int remoteness) {
    switch (val) {
        case kLose:
        case kWin:
            ConcurrentIntMax(&max_remoteness.win_lose, remoteness);
            break;

        case kTie:
            ConcurrentIntMax(&max_remoteness.tie, remoteness);
            break;

        default:
            return;
    }
}

static void GenerateParentsFromChildTierDb(int child_tier_idx, Value val,
                                           int remoteness) {
    PRAGMA_OMP(parallel) {
        DbProbe probe;
        DbManagerProbeInit(&probe);
        TierPosition child = {.tier = game.child_tiers[child_tier_idx]};
        PRAGMA_OMP(for schedule(dynamic, config.db_chunk_size))
        for (Position pos = 0; pos < game.child_tier_sizes[child_tier_idx];
             ++pos) {
            child.position = pos;
            int child_remoteness = DbManagerProbeRemoteness(&probe, child);
            Value child_val = DbManagerProbeValue(&probe, child);
            if (!max_remoteness.set) {
                UpdateMaxRemotenesses(child_val, child_remoteness);
            }

            if (child_val != val || child_remoteness != remoteness) continue;
            GenerateParentsFromTierPosition(child);
        }
        DbManagerProbeDestroy(&probe);
    }
}

static void GenerateParentsFromDbChildTiers(Value val, int remoteness) {
    for (int i = 0; i < game.num_child_tiers; ++i) {
        GenerateParentsFromChildTierDb(i, val, remoteness);
    }
}

/**
 * @brief Generates parent positions of solved child positions of value \p val
 * and remoteness \p remoteness from both solving DB and child tier DBs and
 * marks them in the random access bitset.
 */
static void GenerateParentsFromDb(Value val, int remoteness) {
    ConcurrentBitsetResetAll(rand_bitset);
    GenerateParentsFromDbSolving(val, remoteness);
    GenerateParentsFromDbChildTiers(val, remoteness);
}

static void RemoveSolvedPositionsFromDbChunk(int slot, int chunk) {
    Position begin_pos = chunk * chunking.size;
    Position end_pos = I64Min(begin_pos + chunking.size, game.tier_size);

    PRAGMA_OMP(taskloop num_tasks(config.num_threads * kOmpLoopTaskPerThread))
    for (Position pos = begin_pos; pos < end_pos; ++pos) {
        // If the position has been solved, remove it from the random access
        // bitset
        Value pos_val = DbManagerSegmentationGetValue(slot, pos - begin_pos);
        if (pos_val != kDraw) {
            ConcurrentBitsetReset(rand_bitset, pos, memory_order_relaxed);
        }
    }
}

/**
 * @brief Removes positions that are already solved from the random access
 * bitset.
 */
static void Step2_0_0RemoveSolvedPositions(void) {
    PRAGMA_OMP(parallel)
    PRAGMA_OMP(single)
    for (int64_t i = 0; i < chunking.count; ++i) {
        int slot = i % 2;

        // Load a chunk of DB into memory
        PRAGMA_OMP(task depend(inout : dep.seg[slot]))
        DbManagerSegmentationLoad(slot, i);

        // Remove solved positions from the random access bitset
        PRAGMA_OMP(task depend(inout : dep.seg[slot], dep.cpu))
        RemoveSolvedPositionsFromDbChunk(slot, i);
    }
}

static void CopyRandToSeqChunkInMem(Bitset *seq, int chunk) {
    Position begin_pos = chunk * chunking.size;
    Position end_pos = I64Min(begin_pos + chunking.size, game.tier_size);
    for (Position pos = begin_pos; pos < end_pos; ++pos) {
        bool bit = ConcurrentBitsetTest(rand_bitset, pos, memory_order_relaxed);
        BitsetSetTo(seq, pos - begin_pos, bit);
    }
}

static void StoreSeqChunk(Bitset *seq, int chunk) {
    static char tmp_path[kMaxPathLength], path[kMaxPathLength];
    sprintf(tmp_path, "%s/seq_%d.lz4.tmp", config.path_prefix, chunk);
    sprintf(path, "%s/seq_%d.lz4", config.path_prefix, chunk);
    size_t size = BitSetGetSerializedSize(seq);
    Lz4UtilsCompressBufferToFile(BitsetGetRawData(seq), size, config.lz4_level,
                                 tmp_path, NULL);
    GuardedRename(tmp_path, path);
}

static void Step2_0_1DumpRandToSeq(void) {
    PRAGMA_OMP(parallel)
    PRAGMA_OMP(single)
    for (int64_t i = 0; i < chunking.count; ++i) {
        int slot = i % 2;

        // In-memory bitset copy
        PRAGMA_OMP(task depend(inout : dep.seg[slot], dep.cpu))
        CopyRandToSeqChunkInMem(chunking.seq_buf[slot], i);

        // Remove solved positions from random access bitset
        PRAGMA_OMP(task depend(inout : dep.seg[slot]))
        StoreSeqChunk(chunking.seq_buf[slot], i);
    }
}

static void LoadWinPosFromDbChunk(int slot, int chunk, int remoteness) {
    Position begin_pos = chunk * chunking.size;
    Position end_pos = I64Min(begin_pos + chunking.size, game.tier_size);

    PRAGMA_OMP(taskloop num_tasks(config.num_threads * kOmpLoopTaskPerThread))
    for (Position pos = begin_pos; pos < end_pos; ++pos) {
        // Skip position if its value or remoteness does not match
        int64_t rec_idx = pos - begin_pos;
        Value pos_val = DbManagerSegmentationGetValue(slot, rec_idx);
        if (pos_val != kWin) continue;
        int pos_remoteness = DbManagerSegmentationGetRemoteness(slot, rec_idx);
        if (pos_remoteness > remoteness) continue;

        // If pos is a "win in <= remoteness" position, mark it.
        ConcurrentBitsetSet(rand_bitset, pos, memory_order_relaxed);
    }
}

static void LoadWinPosFromDbSolving(int remoteness) {
    PRAGMA_OMP(parallel)
    PRAGMA_OMP(single)
    for (int64_t i = 0; i < chunking.count; ++i) {
        int slot = i % 2;

        // Read a chunk of DB into memory
        PRAGMA_OMP(task depend(inout : dep.seg[slot]))
        DbManagerSegmentationLoad(slot, i);

        // Load "win in <= N" positions into the random access bitset
        PRAGMA_OMP(task depend(inout : dep.seg[slot], dep.cpu))
        LoadWinPosFromDbChunk(slot, i, remoteness);
    }
}

static void LoadWinPosFromChildTierDb(int child_tier_idx, int remoteness) {
    int64_t offset = game.child_tier_size_offsets[child_tier_idx];
    PRAGMA_OMP(parallel) {
        DbProbe probe;
        DbManagerProbeInit(&probe);
        TierPosition tp = {.tier = game.child_tiers[child_tier_idx]};
        PRAGMA_OMP(for schedule(dynamic, config.db_chunk_size))
        for (Position pos = 0; pos < game.child_tier_sizes[child_tier_idx];
             ++pos) {
            tp.position = pos;
            Value val = DbManagerProbeValue(&probe, tp);
            if (val != kWin) continue;
            int pos_rmt = DbManagerProbeRemoteness(&probe, tp);
            if (pos_rmt > remoteness) continue;

            ConcurrentBitsetSet(rand_bitset, pos + offset,
                                memory_order_relaxed);
        }
        DbManagerProbeDestroy(&probe);
    }
}

static void LoadWinPosFromDbChildTiers(int remoteness) {
    for (int i = 0; i < game.num_child_tiers; ++i) {
        LoadWinPosFromChildTierDb(i, remoteness);
    }
}

/**
 * @brief Loads all win-in-at-most-N positions, where N is \p remoteness , from
 * the solving DB and the databases of all child tiers into the random access
 * bitset.
 */
static void Step2_0_2LoadWinPosFromDb(int remoteness) {
    ConcurrentBitsetResetAll(rand_bitset);
    LoadWinPosFromDbSolving(remoteness);
    LoadWinPosFromDbChildTiers(remoteness);
}

static void ReadDbAndSeqChunk(int slot, int chunk) {
    static char seq_filename[kMaxPathLength];
    DbManagerSegmentationLoad(slot, chunk);
    sprintf(seq_filename, "%s/seq_%d.lz4", config.path_prefix, chunk);
    Bitset *seq = chunking.seq_buf[slot];
    size_t size = BitSetGetSerializedSize(seq);
    Lz4UtilsDecompressFileToBuffer(seq_filename, BitsetGetRawData(seq), size,
                                   NULL);
}

static int64_t GetChildTierOffset(Tier child) {
    TierHashMapIterator it = TierHashMapGet(&game.tier_to_size_offset, child);
    assert(TierHashMapIteratorIsValid(&it));

    return TierHashMapIteratorValue(&it);
}

static bool ProveLosingParentsChunk(int slot, const Bitset *seq, int chunk,
                                    int remoteness) {
    Position begin_pos = chunk * chunking.size;
    Position end_pos = I64Min(begin_pos + chunking.size, game.tier_size);

    bool advance = false;
    // clang-format off
    PRAGMA_OMP(taskloop num_tasks(config.num_threads * kOmpLoopTaskPerThread)
                   reduction(|| : advance))
    // clang-format on
    for (Position pos = begin_pos; pos < end_pos; ++pos) {
        // Skip positions that are not parents to be proved.
        if (!BitsetTest(seq, pos - begin_pos)) continue;

        TierPosition tp = {.tier = game.tier, .position = pos};
        TierPosition children[kTierSolverNumChildPositionsMax];
        int num_children =
            api_internal->GetCanonicalChildPositions(tp, children);

        // Test if the current position has been solved.
        bool solved = true;
        for (int i = 0; i < num_children && solved; ++i) {
            int64_t offset = GetChildTierOffset(children[i].tier);
            int64_t index_in_rand = offset + children[i].position;
            solved = ConcurrentBitsetTest(rand_bitset, index_in_rand,
                                          memory_order_relaxed);
        }

        // If the position has now been solved, it must be lose in N + 1.
        if (solved) {
            DbManagerSegmentationSetValueRemoteness(slot, pos - begin_pos,
                                                    kLose, remoteness + 1);
            advance = true;
        }
    }

    return advance;
}

static bool Step2_0_3ProveLosingParents(int remoteness) {
    bool advance = false;

    PRAGMA_OMP(parallel reduction(task, || : advance))
    PRAGMA_OMP(single)
    for (int64_t i = 0; i < chunking.count; ++i) {
        int slot = i % 2;

        // Read a chunk of DB and a chunk of the sequential access bitset into
        // memory
        PRAGMA_OMP(task depend(inout : dep.seg[slot], chunking.seq_buf[slot]))
        ReadDbAndSeqChunk(slot, i);

        // Prove losing parents in DB chunk
        // clang-format off
        PRAGMA_OMP(task
                    in_reduction(|| : advance)
                    depend(inout : dep.seg[slot], chunking.seq_buf[slot], dep.cpu))
        // clang-format on
        advance |= ProveLosingParentsChunk(slot, chunking.seq_buf[slot], i,
                                           remoteness);

        // Flush DB chunk
        PRAGMA_OMP(task depend(inout : dep.seg[slot]))
        DbManagerSegmentationFlush(slot, i);
    }

    return advance;
}

static bool Step2_0IterateWin(int pass) {
    GenerateParentsFromDb(kWin, pass);
    Step2_0_0RemoveSolvedPositions();
    Step2_0_1DumpRandToSeq();
    Step2_0_2LoadWinPosFromDb(pass);

    return Step2_0_3ProveLosingParents(pass);
}

static bool ProveWinningOrTyingParentsChunk(int slot, int chunk, Value val,
                                            int remoteness) {
    Position begin_pos = chunk * chunking.size;
    Position end_pos = I64Min(begin_pos + chunking.size, game.tier_size);

    bool advance = false;
    // clang-format off
    PRAGMA_OMP(taskloop num_tasks(config.num_threads * kOmpLoopTaskPerThread)
                   reduction(|| : advance))
    // clang-format on
    for (Position pos = begin_pos; pos < end_pos; ++pos) {
        if (!ConcurrentBitsetTest(rand_bitset, pos, memory_order_relaxed)) {
            continue;  // Not a parent position to be proved.
        }
        int64_t rec_idx = pos - begin_pos;

        // If the position has not been solved, mark it as win/tie in N+1.
        Value pos_val = DbManagerSegmentationGetValue(slot, rec_idx);
        if (pos_val == kDraw) {
            DbManagerSegmentationSetValueRemoteness(slot, rec_idx, val,
                                                    remoteness + 1);
            advance = true;
        }
    }

    return advance;
}

/**
 * @brief Scans the solving DB and the random access bitset in lockstep. For
 * each set bit, test if it is already solved in DB - if not, mark it
 * as winning/tying in N+1 in DB where the actual value is given as \p val and N
 * is given by \p remoteness ; otherwise, skip it. Returns \c true if at least
 * one new position becomes solved, or \c false otherwise.
 */
static bool ProveWinningOrTyingParents(Value val, int remoteness) {
    bool advance = false;

    PRAGMA_OMP(parallel reduction(task, || : advance))
    PRAGMA_OMP(single)
    for (int64_t i = 0; i < chunking.count; ++i) {
        int slot = i % 3;  // Read, modify, write.

        // Read in a chunk of DB
        PRAGMA_OMP(task depend(inout : dep.seg[slot]))
        DbManagerSegmentationLoad(slot, i);

        // clang-format off
        // Prove parents
        PRAGMA_OMP(task
                    in_reduction(|| : advance)
                    depend(inout : dep.seg[slot], chunking.seq_buf[slot], dep.cpu))
        // clang-format on
        advance |= ProveWinningOrTyingParentsChunk(slot, i, val, remoteness);

        // Write DB chunk
        PRAGMA_OMP(task depend(inout : dep.seg[slot]))
        DbManagerSegmentationFlush(slot, i);
    }

    return advance;
}

static bool Step2_1IterateLose(int pass) {
    GenerateParentsFromDb(kLose, pass);

    return ProveWinningOrTyingParents(kWin, pass);
}

static void Step2IterateWinLose(void) {
    int pass = 0;
    bool advance = true;
    while (pass <= ConcurrentIntLoad(&max_remoteness.win_lose) || advance) {
        // Cannot use || here because it short-circuits.
        advance = Step2_0IterateWin(pass) || Step2_1IterateLose(pass);
        ++pass;
        max_remoteness.set = true;
    }
}

// ------------------------------ Step3IterateTie ------------------------------

static void Step3IterateTie(void) {
    int pass = 0;
    bool advance = true;
    while (pass <= ConcurrentIntLoad(&max_remoteness.tie) || advance) {
        GenerateParentsFromDb(kTie, pass);
        advance = ProveWinningOrTyingParents(kTie, pass);
        ++pass;
    }
}

// ---------------------------- Step4ConsolidateDb ----------------------------

static int Step4ConsolidateDb(void) {
    return DbManagerSegmentationConsolidate(game.tier_size, chunking.count);
}

// ------------------------------- Step5Cleanup -------------------------------

static void Step5Cleanup(void) {
    api_internal = NULL;
    config.db_chunk_size = 0;
    game.tier = kIllegalTier;
    game.tier_size = 0;
    config.path_prefix = NULL;
    TierHashMapDestroy(&game.tier_to_size_offset);
    game.num_child_tiers = 0;
    ConcurrentIntInit(&max_remoteness.win_lose, 0);
    ConcurrentIntInit(&max_remoteness.tie, 0);
    max_remoteness.set = false;
    config.num_threads = 0;
    chunking.size = 0;
    chunking.count = 0;
    DbManagerSegmentationFreeBuffers();
    for (int i = 0; i < 2; ++i) {
        BitsetDestroy(chunking.seq_buf[i]);
        chunking.seq_buf[i] = NULL;
    }
    ConcurrentBitsetDestroy(rand_bitset);
    rand_bitset = NULL;
}

// ============================================================================
// ============================ TierWorkerBIOneBit ============================
// ============================================================================

int TierWorkerBIOneBit(const TierSolverApi *api, int64_t db_chunk_size,
                       Tier tier, const TierSolverSolveOptions *options,
                       bool *solved) {
    int ret = kMallocFailureError;
    if (!Step0Initialize(api, db_chunk_size, tier, options->memlimit)) {
        goto _bailout;
    }
    Step1ScanTierAndInitDb();
    Step2IterateWinLose();
    Step3IterateTie();
    if (Step4ConsolidateDb() != kNoError) {
        ret = kFileSystemError;
        goto _bailout;
    }

    // Success
    if (solved != NULL) *solved = true;
    ret = kNoError;

_bailout:
    Step5Cleanup();
    return ret;
}

// ============================================================================
// =============================== OneBitMemReq ===============================
// ============================================================================

size_t OneBitMemReq(int64_t tier_group_size) {
#ifdef _OPENMP
    return ConcurrentBitsetMemRequired(tier_group_size);
#else   // _OPENMP not defined
    return (size_t)tier_group_size / 8;  // one bit per position
#endif  // _OPENMP
}
