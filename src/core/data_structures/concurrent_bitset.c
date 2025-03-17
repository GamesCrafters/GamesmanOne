/**
 * @file concurrent_bitset.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Concurrent bitset implementation
 * @version 1.0.0
 * @date 2025-03-16
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
#include "core/data_structures/concurrent_bitset.h"

#include <assert.h>     // assert
#include <stdatomic.h>  // ATOMIC_*_LOCK_FREE, _Atomic, atomic_*
#include <stdbool.h>    // bool, true, false
#include <stddef.h>     // NULL, size_t
#include <stdint.h>     // int64_t
#include <stdlib.h>     // calloc, free

#include "core/concurrency.h"

// Pick the largest lock-free type available
#if (ATOMIC_LLONG_LOCK_FREE == 2)
typedef unsigned long long BlockType;
#elif (ATOMIC_LONG_LOCK_FREE == 2)
typedef unsigned long BlockType;
#else
typedef unsigned int BlockType;
#endif
typedef _Atomic BlockType AtomicBlockType;
static const int64_t kBitsPerBlock = sizeof(BlockType) * 8;
static const BlockType kOne = 1;

struct ConcurrentBitset {
    int64_t num_bits;
    AtomicBlockType data[];
};

static int64_t NumBitsToNumBlocks(int64_t num_bits) {
    return (num_bits + kBitsPerBlock - 1) / kBitsPerBlock;
}

ConcurrentBitset *ConcurrentBitsetCreate(int64_t num_bits) {
    if (num_bits < 0) return 0;
    int64_t num_blocks = NumBitsToNumBlocks(num_bits);
    size_t alloc_size =
        sizeof(ConcurrentBitset) + num_blocks * sizeof(AtomicBlockType);
    ConcurrentBitset *ret = (ConcurrentBitset *)calloc(alloc_size, 1);
    if (ret == NULL) return ret;

    ret->num_bits = num_bits;
    return ret;
}

void ConcurrentBitsetDestroy(ConcurrentBitset *s) { free(s); }

int64_t ConcurrentBitsetGetNumBits(const ConcurrentBitset *s) {
    return s->num_bits;
}

static int64_t BlockIndex(int64_t bit_index) {
    return bit_index / kBitsPerBlock;
}

static int64_t BitOffset(int64_t bit_index) {
    return bit_index % kBitsPerBlock;
}

bool ConcurrentBitsetSet(ConcurrentBitset *s, int64_t bit_index,
                         memory_order order) {
    assert(bit_index >= 0 && bit_index < s->num_bits);
    int64_t bit_offset = BitOffset(bit_index);
    int64_t block_index = BlockIndex(bit_index);
    BlockType mask = kOne << bit_offset;
    BlockType previous =
        atomic_fetch_or_explicit(&s->data[block_index], mask, order);

    return previous & mask;
}

bool ConcurrentBitsetReset(ConcurrentBitset *s, int64_t bit_index,
                           memory_order order) {
    assert(bit_index >= 0 && bit_index < s->num_bits);
    int64_t bit_offset = BitOffset(bit_index);
    int64_t block_index = BlockIndex(bit_index);
    BlockType mask = kOne << bit_offset;
    BlockType previous =
        atomic_fetch_and_explicit(&s->data[block_index], ~mask, order);

    return previous & mask;
}

bool ConcurrentBitsetTest(ConcurrentBitset *s, int64_t bit_index,
                          memory_order order) {
    assert(bit_index >= 0 && bit_index < s->num_bits);
    int64_t bit_offset = BitOffset(bit_index);
    int64_t block_index = BlockIndex(bit_index);
    BlockType mask = kOne << bit_offset;
    BlockType block = atomic_load_explicit(&s->data[block_index], order);

    return block & mask;
}

size_t ConcurrentBitsetGetSerializedSize(const ConcurrentBitset *s) {
    int64_t num_blocks = NumBitsToNumBlocks(s->num_bits);

    return num_blocks * sizeof(BlockType);
}

void ConcurrentBitsetSerialize(const ConcurrentBitset *s, void *buf) {
    BlockType *out = (BlockType *)buf;
    int64_t num_blocks = NumBitsToNumBlocks(s->num_bits);
    PRAGMA_OMP_PARALLEL_FOR
    for (int64_t i = 0; i < num_blocks; ++i) {
        out[i] = atomic_load_explicit(&s->data[i], memory_order_relaxed);
    }
}

void ConcurrentBitsetDeserialize(ConcurrentBitset *s, const void *buf) {
    const BlockType *in = (const BlockType *)buf;
    int64_t num_blocks = NumBitsToNumBlocks(s->num_bits);
    PRAGMA_OMP_PARALLEL_FOR
    for (int64_t i = 0; i < num_blocks; ++i) {
        atomic_store_explicit(&s->data[i], in[i], memory_order_relaxed);
    }
}
