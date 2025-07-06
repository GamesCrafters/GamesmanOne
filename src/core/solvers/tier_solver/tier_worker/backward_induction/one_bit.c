#include "core/solvers/tier_solver/tier_worker/backward_induction/one_bit.h"

#include <assert.h>     // assert
#include <limits.h>     // PATH_MAX
#include <stdatomic.h>  // memory_order_relaxed
#include <stdbool.h>    // bool, true, false
#include <stddef.h>     // size_t, NULL
#include <stdint.h>     // int64_t
#include <stdio.h>      // sprintf

#include "core/concurrency.h"
#include "core/constants.h"
#include "core/data_structures/bitset.h"
#include "core/data_structures/concurrent_bitset.h"
#include "core/db/arraydb/arraydb.h"
#include "core/db/db_manager.h"
#include "core/gamesman_memory.h"
#include "core/misc.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/solvers/tier_solver/tier_worker.h"
#include "core/types/gamesman_types.h"
#include "libs/lz4_utils/lz4_utils.h"
#include "libs/xzra/xzra.h"

// Read-only reference to the API functions from tier_manager.
static const TierSolverApi *api_internal;

// Number of positions in each database compression block.
static int64_t current_db_chunk_size;

static Tier this_tier;          // The tier being solved.
static int64_t this_tier_size;  // Size of the tier being solved.

static Tier child_tiers[kTierSolverNumChildTiersMax];  // Array of child tiers.

// Size of each child tier.
static int64_t child_tier_sizes[kTierSolverNumChildTiersMax];

// Prefix sum of child_tier_sizes.
static int64_t child_tier_size_offsets[kTierSolverNumChildTiersMax + 1];

// Map from tiers in the current tier group (this_tier and child tiers) to
// size offsets.
static TierHashMap tier_to_size_offset;

static int num_child_tiers;  // Number of child tiers in total.

static int max_win_lose_remoteness;  // Max win/loss remoteness discovered
static int max_tie_remoteness;       // Max tie remoteness discovered
static int num_threads;              // Number of threads available.

static int64_t chunk_size;
static int64_t num_chunks;
static Bitset *seq_buf[2];

static ConcurrentBitset *rand_bitset;
static char dep_chunk;
static char dep_seg[3];

static int lz4_level = 0;
static const char *path_prefix;

// ------------------------------ Step0Initialize ------------------------------

static void Step0_0SetupChildTiers(void) {
    Tier raw[kTierSolverNumChildTiersMax];
    int num_raw = api_internal->GetChildTiers(this_tier, raw);
    TierHashSet dedup;
    TierHashSetInit(&dedup, 0.5);
    TierHashMapInit(&tier_to_size_offset, 0.5);
    num_child_tiers = 0;
    child_tier_size_offsets[0] = this_tier_size;
    TierHashMapSet(&tier_to_size_offset, this_tier, 0);
    for (int i = 0; i < num_raw; ++i) {
        Tier canonical = api_internal->GetCanonicalTier(raw[i]);
        if (TierHashSetAdd(&dedup, canonical)) {
            // Push the canonical child tier into the array of child tiers.
            child_tiers[num_child_tiers] = canonical;

            // Set the size of the child tier.
            int64_t size = api_internal->GetTierSize(canonical);
            child_tier_sizes[num_child_tiers] = size;

            // Calculate prefix sum for size offset into the random access
            // bitset.
            child_tier_size_offsets[num_child_tiers + 1] =
                child_tier_size_offsets[num_child_tiers] + size;

            // Put the prefix sum into the offset map.
            TierHashMapSet(&tier_to_size_offset, canonical,
                           child_tier_size_offsets[num_child_tiers + 1]);
            ++num_child_tiers;
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
 * sequential access bitset at the same time. The Array DB uses 2 bytes per
 * position and the bitset uses 1/8 bytes per position but with the final
 * memory usage rounded to the next integral value of bytes. The solver uses a
 * rolling buffer so there will be at most 3 chunks of DB and 2 chunks of the
 * sequential bitset loaded at the same time. Let x be the number of positions
 * in each chunk. The memory requirement is therefore
 *
 *     3 * 2 * x + 2 * (x + 7) / 8 <= mem
 *
 * Rearranging the above inequality gives the formula that is used in this
 * function.
 */
static int64_t CalcChunkSize(size_t mem) {
    int64_t ret = (mem * 4 - 7) / 25;

    // Distribute positions into each chunk as evenly as possible.
    ret = RoundUpDivide(this_tier_size, RoundUpDivide(this_tier_size, ret));

    // Round the number of positions to the next multiple of 64, which is the
    // number of bits in each block in the Bitset implementation.
    return NextMultiple(ret, 64);
}

static bool Step0_1AllocateMemory(size_t memlimit) {
    // Calculate the size of the current tier group.
    int64_t tier_group_size = this_tier_size;
    for (int i = 0; i < num_child_tiers; ++i) {
        tier_group_size += child_tier_sizes[i];
    }

    // Size of the random access concurrent bitset
    size_t rand_size = ConcurrentBitsetMemRequired(tier_group_size);

    // While we can make the chunk sizes arbitrarily small, we need memory to
    // load one chunk of consolidated DB in each thread while scanning child
    // tiers.
    size_t min_required =
        rand_size + num_threads * current_db_chunk_size * kArrayDbRecordSize;
    if (memlimit < min_required) return false;

    // Calculate chunk size and number of chunks to be created. Then make sure
    // that we won't create too many chunk files on disk.
    chunk_size = CalcChunkSize(memlimit - min_required);
    num_chunks = RoundUpDivide(this_tier_size, chunk_size);
    if (num_chunks > 4096) return false;

    // Allocate the random access concurrent bitset.
    rand_bitset = ConcurrentBitsetCreate(tier_group_size);

    // Allocate DB and sequential access bitset rolling buffers
    DbManagerCreateSolvingSegmentBuffers(this_tier, 3, chunk_size);
    seq_buf[0] = BitsetCreate(chunk_size);
    seq_buf[1] = BitsetCreate(chunk_size);

    return rand_bitset != NULL;
}

static bool Step0Initialize(const TierSolverApi *api, int64_t db_chunk_size,
                            Tier tier, size_t memlimit) {
    memlimit = memlimit == 0 ? GetPhysicalMemory() / 10 * 9 : memlimit;

    // Set API and other constants.
    assert(api && api->GetCanonicalParentPositions);
    api_internal = api;
    current_db_chunk_size = db_chunk_size;
    num_threads = ConcurrencyGetOmpNumThreads();
    path_prefix = DbManagerGetPathPrefix();

    // Initialize max remoteness values to 0.
    max_win_lose_remoteness = max_tie_remoteness = 0;

    // Initialize the child tier array.
    this_tier = tier;
    this_tier_size = api_internal->GetTierSize(tier);
    Step0_0SetupChildTiers();

    // Plan memory usage ahead and create the random access concurrent bitset.
    return Step0_1AllocateMemory(memlimit);
}

// -------------------------- Step1ScanTierAndInitDb --------------------------

static int64_t I64Min(int64_t a, int64_t b) { return a < b ? a : b; }

static bool IsCanonicalPosition(Position pos) {
    TierPosition tp = {.tier = this_tier, .position = pos};
    return api_internal->GetCanonicalPosition(tp) == pos;
}

static void ScanDbChunk(int slot, int chunk) {
    Position begin_pos = chunk * chunk_size;
    Position end_pos = I64Min(begin_pos + chunk_size, this_tier_size);

    PRAGMA_OMP(taskloop grainsize(128))
    for (Position pos = begin_pos; pos < end_pos; ++pos) {
        TierPosition tp = {.tier = this_tier, .position = pos};
        int64_t rec_idx = pos - begin_pos;

        // Assign (undecided, 0) to illegal positions and non-canonical
        // positions.
        if (!api_internal->IsLegalPosition(tp) || !IsCanonicalPosition(pos)) {
            DbManagerSolvingSegmentSetValueRemoteness(slot, rec_idx, kUndecided,
                                                      0);
            continue;
        }

        Value val = api_internal->Primitive(tp);
        // Assign (draw, 0) to non-primitive positions.
        if (val == kUndecided) {
            DbManagerSolvingSegmentSetValueRemoteness(slot, rec_idx, kDraw, 0);
            continue;
        }

        // If the position is primitive, assign its primitive value and
        // remoteness 0.
        DbManagerSolvingSegmentSetValueRemoteness(slot, rec_idx, val, 0);
    }
}

static void Step1ScanTierAndInitDb(void) {
    PRAGMA_OMP(parallel)
    PRAGMA_OMP(single)
    for (int64_t i = 0; i < num_chunks; ++i) {
        int slot = i % 2;

        // Scan for illegal, non-canonical, and primitive positions
        PRAGMA_OMP(task depend(inout : dep_seg[slot], dep_chunk))
        ScanDbChunk(slot, i);

        // Write the chunk to disk
        PRAGMA_OMP(task depend(inout : dep_seg[slot]))
        DbManagerFlushSolvingSegment(slot, i);
    }
}

// ---------------------------- Step2IterateWinLose ----------------------------

static void GenerateParentsFromTierPosition(TierPosition child) {
    Position parents[kTierSolverNumParentPositionsMax];
    int num_parents =
        api_internal->GetCanonicalParentPositions(child, this_tier, parents);
    for (int i = 0; i < num_parents; ++i) {
        ConcurrentBitsetSet(rand_bitset, parents[i], memory_order_relaxed);
    }
}

static void GenerateParentsFromDbChunk(int slot, int chunk, Value val,
                                       int remoteness) {
    Position begin_pos = chunk * chunk_size;
    Position end_pos = I64Min(begin_pos + chunk_size, this_tier_size);

    PRAGMA_OMP(taskloop grainsize(128))
    for (Position pos = begin_pos; pos < end_pos; ++pos) {
        // Skip position if its value or remoteness does not match
        int64_t rec_idx = pos - begin_pos;
        Value pos_val = DbManagerSolvingSegmentGetValue(slot, rec_idx);
        if (pos_val != val) continue;
        int pos_remoteness =
            DbManagerSolvingSegmentGetRemoteness(slot, rec_idx);
        if (pos_remoteness != remoteness) continue;

        TierPosition child = {.tier = this_tier, .position = pos};
        GenerateParentsFromTierPosition(child);
    }
}

static void GenerateParentsFromDbSolving(Value val, int remoteness) {
    PRAGMA_OMP(parallel)
    PRAGMA_OMP(single)
    for (int64_t i = 0; i < num_chunks; ++i) {
        int slot = i % 2;

        // Load a chunk of DB into memory
        PRAGMA_OMP(task depend(inout : dep_seg[slot]))
        DbManagerLoadSolvingSegment(slot, i);

        // Generate parents from loaded DB into random access bitset
        PRAGMA_OMP(task depend(inout : dep_seg[slot], dep_chunk))
        GenerateParentsFromDbChunk(slot, i, val, remoteness);
    }
}

static void UpdateMaxRemotenesses(Value val, int remoteness) {
    switch (val) {
        case kLose:
        case kWin:
            if (max_win_lose_remoteness < remoteness) {
                max_win_lose_remoteness = remoteness;
            }
            break;

        case kTie:
            if (max_tie_remoteness < remoteness) {
                max_tie_remoteness = remoteness;
            }
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
        TierPosition child = {.tier = child_tiers[child_tier_idx]};
        PRAGMA_OMP(for schedule(dynamic, 128))
        for (Position pos = 0; pos < child_tier_sizes[child_tier_idx]; ++pos) {
            child.position = pos;
            int child_remoteness = DbManagerProbeRemoteness(&probe, child);
            Value child_val = DbManagerProbeValue(&probe, child);
            UpdateMaxRemotenesses(child_val, remoteness);
            if (child_val != val || child_remoteness != remoteness) continue;
            GenerateParentsFromTierPosition(child);
        }
        DbManagerProbeDestroy(&probe);
    }
}

static void GenerateParentsFromDbChildTiers(Value val, int remoteness) {
    for (int i = 0; i < num_child_tiers; ++i) {
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
    Position begin_pos = chunk * chunk_size;
    Position end_pos = I64Min(begin_pos + chunk_size, this_tier_size);

    PRAGMA_OMP(taskloop grainsize(128))
    for (Position pos = begin_pos; pos < end_pos; ++pos) {
        // If the position has been solved, remove it from the random access
        // bitset
        Value pos_val = DbManagerSolvingSegmentGetValue(slot, pos - begin_pos);
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
    for (int64_t i = 0; i < num_chunks; ++i) {
        int slot = i % 2;

        // Load a chunk of DB into memory
        PRAGMA_OMP(task depend(inout : dep_seg[slot]))
        DbManagerLoadSolvingSegment(slot, i);

        // Remove solved positions from the random access bitset
        PRAGMA_OMP(task depend(inout : dep_seg[slot], dep_chunk))
        RemoveSolvedPositionsFromDbChunk(slot, i);
    }
}

static void CopyRandToSeqChunkInMem(Bitset *seq, int chunk) {
    Position begin_pos = chunk * chunk_size;
    Position end_pos = I64Min(begin_pos + chunk_size, this_tier_size);
    for (Position pos = begin_pos; pos < end_pos; ++pos) {
        bool bit = ConcurrentBitsetTest(rand_bitset, pos, memory_order_relaxed);
        BitsetSetTo(seq, pos - begin_pos, bit);
    }
}

static void StoreSeqChunk(Bitset *seq, int chunk) {
    static char tmp_path[PATH_MAX], path[PATH_MAX];
    sprintf(tmp_path, "%s/seq_%d.lz4.tmp", path_prefix, chunk);
    sprintf(path, "%s/seq_%d.lz4", path_prefix, chunk);
    size_t size = BitSetGetSerializedSize(seq);
    Lz4UtilsCompressStream(BitsetGetRawData(seq), size, lz4_level, tmp_path);
    GuardedRename(tmp_path, path);
}

static void Step2_0_1DumpRandToSeq(void) {
    PRAGMA_OMP(parallel)
    PRAGMA_OMP(single)
    for (int64_t i = 0; i < num_chunks; ++i) {
        int slot = i % 2;

        // In-memory bitset copy dependences:
        // 1. Must wait for any previous task operating on the same bitset to
        // finish.
        // 2. Must wait for the previous copy operation to finish.
        PRAGMA_OMP(task depend(inout : dep_seg[slot], dep_chunk))
        CopyRandToSeqChunkInMem(seq_buf[slot], i);

        // Remove solved positions from Rand dependences:
        // 1. Must wait for any previous task operating on the same buffer
        // to finish.
        // 2. Must wait for the previous removal task to finish.
        PRAGMA_OMP(task depend(inout : dep_seg[slot]))
        StoreSeqChunk(seq_buf[slot], i);
    }
}

static void LoadWinPosFromDbChunk(int slot, int chunk, int remoteness) {
    Position begin_pos = chunk * chunk_size;
    Position end_pos = I64Min(begin_pos + chunk_size, this_tier_size);

    PRAGMA_OMP(taskloop grainsize(128))
    for (Position pos = begin_pos; pos < end_pos; ++pos) {
        // Skip position if its value or remoteness does not match
        int64_t rec_idx = pos - begin_pos;
        Value pos_val = DbManagerSolvingSegmentGetValue(slot, rec_idx);
        if (pos_val != kWin) continue;
        int pos_remoteness =
            DbManagerSolvingSegmentGetRemoteness(slot, rec_idx);
        if (pos_remoteness > remoteness) continue;

        // If pos is a "win in <= remoteness" position, mark it.
        ConcurrentBitsetSet(rand_bitset, pos, memory_order_relaxed);
    }
}

static void LoadWinPosFromDbSolving(int remoteness) {
    PRAGMA_OMP(parallel)
    PRAGMA_OMP(single)
    for (int64_t i = 0; i < num_chunks; ++i) {
        int slot = i % 2;

        // Read a chunk of DB into memory
        PRAGMA_OMP(task depend(inout : dep_seg[slot]))
        DbManagerLoadSolvingSegment(slot, i);

        // Load "win in <= N" positions into the random access bitset
        PRAGMA_OMP(task depend(inout : dep_seg[slot], dep_chunk))
        LoadWinPosFromDbChunk(slot, i, remoteness);
    }
}

static void LoadWinPosFromChildTierDb(int child_tier_idx, int remoteness) {
    int64_t offset = child_tier_size_offsets[child_tier_idx];
    PRAGMA_OMP(parallel) {
        DbProbe probe;
        DbManagerProbeInit(&probe);
        TierPosition tp = {.tier = child_tiers[child_tier_idx]};
        PRAGMA_OMP(for schedule(dynamic, 128))
        for (Position pos = 0; pos < child_tier_sizes[child_tier_idx]; ++pos) {
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
    for (int i = 0; i < num_child_tiers; ++i) {
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
    static char seq_filename[PATH_MAX];
    DbManagerLoadSolvingSegment(slot, chunk);
    sprintf(seq_filename, "%s/seq_%d.lz4", path_prefix, chunk);
    Bitset *seq = seq_buf[slot];
    size_t size = BitSetGetSerializedSize(seq);
    Lz4UtilsDecompressFile(seq_filename, BitsetGetRawData(seq), size);
}

static int64_t GetChildTierOffset(Tier child) {
    TierHashMapIterator it = TierHashMapGet(&tier_to_size_offset, child);
    assert(TierHashMapIteratorIsValid(&it));

    return TierHashMapIteratorValue(&it);
}

static bool ProveLosingParentsChunk(int slot, const Bitset *seq, int chunk,
                                    int remoteness) {
    Position begin_pos = chunk * chunk_size;
    Position end_pos = I64Min(begin_pos + chunk_size, this_tier_size);

    bool advance = false;
    PRAGMA_OMP(taskloop grainsize(128) reduction(|| : advance))
    for (Position pos = begin_pos; pos < end_pos; ++pos) {
        // Skip positions that are not parents to be proved.
        if (!BitsetTest(seq, pos - begin_pos)) continue;

        TierPosition tp = {.tier = this_tier, .position = pos};
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
            DbManagerSolvingSegmentSetValueRemoteness(slot, pos - begin_pos,
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
    for (int64_t i = 0; i < num_chunks; ++i) {
        int slot = i % 2;

        // Read a chunk of DB and a chunk of the sequential access bitset into
        // memory
        PRAGMA_OMP(task depend(inout : dep_seg[slot], seq_buf[slot]))
        ReadDbAndSeqChunk(slot, i);

        // Prove losing parents in DB chunk
        // clang-format off
        PRAGMA_OMP(task
                    in_reduction(|| : advance)
                    depend(inout : dep_seg[slot], seq_buf[slot], dep_chunk))
        // clang-format on
        advance |= ProveLosingParentsChunk(slot, seq_buf[slot], i, remoteness);

        // Flush DB chunk
        PRAGMA_OMP(task depend(inout : dep_seg[slot]))
        DbManagerFlushSolvingSegment(slot, i);
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
    Position begin_pos = chunk * chunk_size;
    Position end_pos = I64Min(begin_pos + chunk_size, this_tier_size);

    bool advance = false;
    PRAGMA_OMP(taskloop grainsize(128) reduction(|| : advance))
    for (Position pos = begin_pos; pos < end_pos; ++pos) {
        if (!ConcurrentBitsetTest(rand_bitset, pos, memory_order_relaxed)) {
            continue;  // Not a parent position to be proved.
        }
        int64_t rec_idx = pos - begin_pos;

        // If the position has not been solved, mark it as win/tie in N+1.
        Value pos_val = DbManagerSolvingSegmentGetValue(slot, rec_idx);
        if (pos_val == kDraw) {
            DbManagerSolvingSegmentSetValueRemoteness(slot, rec_idx, val,
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
    for (int64_t i = 0; i < num_chunks; ++i) {
        int slot = i % 3;  // Read, modify, write.

        // DB read dependences:
        // 1. Must wait for any previous task operating on the same buffer to
        // finish.
        // 2. Must wait for the previous read task to finish.
        PRAGMA_OMP(task depend(inout : dep_seg[slot]))
        DbManagerLoadSolvingSegment(slot, i);

        // Proof of parents and write to DB chunk dependences:
        // 1. Must wait for any previous task operating on the same buffers to
        //    finish.
        // 2. Must wait for the previous proof process to finish.

        // clang-format off
        PRAGMA_OMP(task
                    in_reduction(|| : advance)
                    depend(inout : dep_seg[slot], seq_buf[slot], dep_chunk))
        // clang-format on
        advance |= ProveWinningOrTyingParentsChunk(slot, i, val, remoteness);

        // Write DB chunk dependencies:
        // 1. Must wait for any previous task operating on the same buffer to
        //    finish.
        // 2. Must wait for the previous write task to finish.
        PRAGMA_OMP(task depend(inout : dep_seg[slot]))
        DbManagerFlushSolvingSegment(slot, i);
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
    while (pass <= max_win_lose_remoteness || advance) {
        advance = false;
        advance |= Step2_0IterateWin(pass);
        advance |= Step2_1IterateLose(pass);
        ++pass;
    }
}

// ------------------------------ Step3IterateTie ------------------------------

static void Step3IterateTie(void) {
    int pass = 0;
    bool advance = true;
    while (pass <= max_tie_remoteness || advance) {
        GenerateParentsFromDb(kTie, pass);
        advance = ProveWinningOrTyingParents(kTie, pass);
        ++pass;
    }
}

// ---------------------------- Step4ConsolidateDb ----------------------------

static int Step4ConsolidateDb(void) {
    return DbManagerConsolidateSolvingSegments(this_tier_size, num_chunks);
}

// ------------------------------- Step5Cleanup -------------------------------

static void Step5Cleanup(void) {
    api_internal = NULL;
    current_db_chunk_size = 0;
    this_tier = kIllegalTier;
    this_tier_size = 0;
    path_prefix = NULL;
    TierHashMapDestroy(&tier_to_size_offset);
    num_child_tiers = 0;
    max_win_lose_remoteness = 0;
    max_tie_remoteness = 0;
    num_threads = 0;
    chunk_size = 0;
    num_chunks = 0;
    DbManagerFreeSolvingSegmentBuffers();
    for (int i = 0; i < 2; ++i) {
        BitsetDestroy(seq_buf[i]);
        seq_buf[i] = NULL;
    }
    ConcurrentBitsetDestroy(rand_bitset);
    rand_bitset = NULL;
}

// --------------------------------- CompareDb ---------------------------------

static bool CompareDb(void) {
    DbProbe probe, ref_probe;
    if (DbManagerProbeInit(&probe)) return false;
    if (DbManagerRefProbeInit(&ref_probe)) {
        DbManagerProbeDestroy(&probe);
        return false;
    }

    bool success = true;
    for (Position p = 0; p < this_tier_size; ++p) {
        TierPosition tp = {.tier = this_tier, .position = p};
        Value ref_value = DbManagerRefProbeValue(&ref_probe, tp);
        if (ref_value == kUndecided) continue;

        Value actual_value = DbManagerProbeValue(&probe, tp);
        if (actual_value != ref_value) {
            printf("CompareDb: inconsistent value at tier %" PRITier
                   " position %" PRIPos "\n",
                   this_tier, p);
            success = false;
            goto _bailout;
        }

        int actual_remoteness = DbManagerProbeRemoteness(&probe, tp);
        int ref_remoteness = DbManagerRefProbeRemoteness(&ref_probe, tp);
        if (actual_remoteness != ref_remoteness) {
            printf("CompareDb: inconsistent remoteness at tier %" PRITier
                   " position %" PRIPos "\n",
                   this_tier, p);
            success = false;
            goto _bailout;
        }
    }

_bailout:
    DbManagerProbeDestroy(&probe);
    DbManagerRefProbeDestroy(&ref_probe);
    if (success) {
        printf("CompareDb: tier %" PRITier " check passed\n", this_tier);
    }

    return success;
}

// ============================================================================
// ============================ TierWorkerBIOneBit ============================
// ============================================================================

int TierWorkerBIOneBit(const TierSolverApi *api, int64_t db_chunk_size,
                       Tier tier, const TierWorkerSolveOptions *options,
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
    if (options->compare && !CompareDb()) {
        ret = kRuntimeError;
        goto _bailout;
    }

    // Success
    if (solved != NULL) *solved = true;
    ret = kNoError;

_bailout:
    Step5Cleanup();
    return ret;
}
