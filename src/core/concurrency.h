/**
 * @file concurrency.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Header-only concurrency convenience library.
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

#ifndef GAMESMANONE_CORE_CONCURRENCY_H_
#define GAMESMANONE_CORE_CONCURRENCY_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef _OPENMP
#include <omp.h>
#ifdef __cplusplus
#include <atomic>

/**
 * @brief Namespace for atomic types and operations in C++.
 */
#define ATOMIC_NS std::
#else  // Compiling as C
#include <stdatomic.h>

/**
 * @brief This macro is only effective when compiling with C++ and is ignored by
 * C.
 */
#define ATOMIC_NS
#endif  // __cplusplus
#endif  // _OPENMP

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#ifdef _OPENMP

#ifdef __cplusplus

/**
 * @brief Concurrent boolean type.
 */
typedef std::atomic<bool> ConcurrentBool;

/**
 * @brief Concurrent integer type.
 */
typedef std::atomic<int> ConcurrentInt;

/**
 * @brief Concurrent 64-bit integer type.
 */
typedef std::atomic<int64_t> ConcurrentInt64;

/**
 * @brief Concurrent size type.
 */
typedef std::atomic<size_t> ConcurrentSizeType;
#else   // Compiling as C

/**
 * @brief Concurrent boolean type.
 */
typedef atomic_bool ConcurrentBool;

/**
 * @brief Concurrent integer type.
 */
typedef atomic_int ConcurrentInt;

/**
 * @brief Concurrent signed 64-bit integer type.
 */
typedef _Atomic int64_t ConcurrentInt64;

/**
 * @brief Concurrent size type.
 */
typedef atomic_size_t ConcurrentSizeType;
#endif  // __cplusplus

/**
 * @brief Wrapper for the `_Pragma` operator.
 *
 * @param[in] X The expression to pass to `_Pragma`.
 */
#define PRAGMA(X) _Pragma(#X)

/**
 * @brief Convenience macro for OpenMP pragmas.
 *
 * @param[in] expression The OpenMP expression.
 */
#define PRAGMA_OMP(expression) PRAGMA(omp expression)

/**
 * @brief Memory order for the API of this library.
 */
typedef enum {
    /** Relaxed operation. */
    kConcurrencyMemoryOrderRelaxed = ATOMIC_NS memory_order_relaxed,

    /** Consume operation. */
    kConcurrencyMemoryOrderConsume = ATOMIC_NS memory_order_consume,

    /** Acquire operation. */
    kConcurrencyMemoryOrderAcquire = ATOMIC_NS memory_order_acquire,

    /** Release operation. */
    kConcurrencyMemoryOrderRelease = ATOMIC_NS memory_order_release,

    /** Acquire and release operation. */
    kConcurrencyMemoryOrderAcqRel = ATOMIC_NS memory_order_acq_rel,

    /** Sequentially consistent operation. */
    kConcurrencyMemoryOrderSeqCst = ATOMIC_NS memory_order_seq_cst,
} ConcurrencyMemoryOrder;

#else  // _OPENMP not defined.

/**
 * @brief Convenience macro for OpenMP pragmas. Does nothing when OpenMP is
 *        disabled.
 *
 * @param[in] expression The OpenMP expression.
 */
#define PRAGMA_OMP(expression)

/**
 * @brief Concurrent boolean type.
 */
typedef bool ConcurrentBool;

/**
 * @brief Concurrent integer type.
 */
typedef int ConcurrentInt;

/**
 * @brief Concurrent 64-bit integer type.
 */
typedef int64_t ConcurrentInt64;

/**
 * @brief Concurrent size type.
 */
typedef size_t ConcurrentSizeType;

/**
 * @brief Dummy memory order for the API of this library.
 */
typedef enum {

    /** Dummy relaxed operation. */
    kConcurrencyMemoryOrderRelaxed,

    /** Dummy consume operation. */
    kConcurrencyMemoryOrderConsume,

    /** Dummy acquire operation. */
    kConcurrencyMemoryOrderAcquire,

    /** Dummy release operation. */
    kConcurrencyMemoryOrderRelease,

    /** Dummy acquire and release operation. */
    kConcurrencyMemoryOrderAcqRel,

    /** Dummy sequentially consistent operation. */
    kConcurrencyMemoryOrderSeqCst,
} ConcurrencyMemoryOrder;

#endif  // _OPENMP

// ============================== ConcurrentBool ===============================

/**
 * @brief Initializes the `ConcurrentBool` variable at `cb` to `val`.
 *
 * @param[out] cb Pointer to the `ConcurrentBool` to initialize.
 * @param[in] val Value to initialize to.
 */
static inline void ConcurrentBoolInit(ConcurrentBool *cb, bool val) {
#ifdef _OPENMP
    ATOMIC_NS atomic_init(cb, val);
#else
    *cb = val;
#endif
}

/**
 * @brief Returns the current value of the `ConcurrentBool` variable at `cb`.
 *
 * @details In a multithreaded context, the load operation is atomic with
 * default memory order.
 *
 * @param[in] cb Pointer to the `ConcurrentBool` variable to load the value
 *               from.
 *
 * @returns The current value of the `ConcurrentBool` variable.
 */
static inline bool ConcurrentBoolLoad(const ConcurrentBool *cb) {
#ifdef _OPENMP
    return ATOMIC_NS atomic_load(cb);
#else
    return *cb;
#endif
}

/**
 * @brief Returns the current value of the `ConcurrentBool` variable at `cb`
 *        with explicit memory order.
 *
 * @param[in] cb Pointer to the `ConcurrentBool` variable to load the value
 *               from.
 * @param[in] order The explicit memory order to use for the load operation.
 *
 * @returns The current value of the `ConcurrentBool` variable.
 */
static inline bool ConcurrentBoolLoadExplicit(const ConcurrentBool *cb,
                                              ConcurrencyMemoryOrder order) {
#ifdef _OPENMP
    return ATOMIC_NS atomic_load_explicit(cb, order);
#else
    (void)order;
    return *cb;
#endif
}

/**
 * @brief Stores the value `val` into the `ConcurrentBool` variable at `cb`.
 *
 * @details In a multithreaded context, the store operation is atomic with
 * default memory order.
 *
 * @param[out] cb Pointer to the `ConcurrentBool` variable to store the value
 *                into.
 * @param[in] val The value to store.
 */
static inline void ConcurrentBoolStore(ConcurrentBool *cb, bool val) {
#ifdef _OPENMP
    ATOMIC_NS atomic_store(cb, val);
#else
    *cb = val;
#endif
}

/**
 * @brief Stores the value `val` into the `ConcurrentBool` variable at `cb` with
 * explicit memory order.
 *
 * @param[out] cb Pointer to the `ConcurrentBool` variable to store the value
 *                into.
 * @param[in] val The value to store.
 * @param[in] order The explicit memory order to use for the store operation.
 */
static inline void ConcurrentBoolStoreExplicit(ConcurrentBool *cb, bool val,
                                               ConcurrencyMemoryOrder order) {
#ifdef _OPENMP
    ATOMIC_NS atomic_store_explicit(cb, val, order);
#else
    (void)order;
    *cb = val;
#endif
}

// =============================== ConcurrentInt ===============================

/**
 * @brief Initializes the `ConcurrentInt` variable at `ci` to `val`.
 *
 * @param[out] ci Pointer to the `ConcurrentInt` to initialize.
 * @param[in] val Value to initialize to.
 */
static inline void ConcurrentIntInit(ConcurrentInt *ci, int val) {
#ifdef _OPENMP
    ATOMIC_NS atomic_init(ci, val);
#else
    *ci = val;
#endif
}

/**
 * @brief Returns the current value of the `ConcurrentInt` variable at `ci`.
 *
 * @details In a multithreaded context, the load operation is atomic with
 * default memory order.
 *
 * @param[in] ci Pointer to the `ConcurrentInt` variable to load the value
 *               from.
 *
 * @returns The current value of the `ConcurrentInt` variable.
 */
static inline int ConcurrentIntLoad(const ConcurrentInt *ci) {
#ifdef _OPENMP
    return ATOMIC_NS atomic_load(ci);
#else
    return *ci;
#endif
}

/**
 * @brief Returns the current value of the `ConcurrentInt` variable at `ci` with
 *        explicit memory order.
 *
 * @param[in] ci Pointer to the `ConcurrentInt` variable to load the value
 *               from.
 * @param[in] order The explicit memory order to use for the load operation.
 *
 * @returns The current value of the `ConcurrentInt` variable.
 */
static inline int ConcurrentIntLoadExplicit(const ConcurrentInt *ci,
                                            ConcurrencyMemoryOrder order) {
#ifdef _OPENMP
    return ATOMIC_NS atomic_load_explicit(ci, order);
#else
    (void)order;
    return *ci;
#endif
}

/**
 * @brief Stores the value `val` into the `ConcurrentInt` variable at `ci`.
 *
 * @details In a multithreaded context, the store operation is atomic with
 * default memory order.
 *
 * @param[out] ci Pointer to the `ConcurrentInt` variable to store the value
 *                into.
 * @param[in] val The value to store.
 */
static inline void ConcurrentIntStore(ConcurrentInt *ci, int val) {
#ifdef _OPENMP
    ATOMIC_NS atomic_store(ci, val);
#else
    *ci = val;
#endif
}

/**
 * @brief Stores the value `val` into the `ConcurrentInt` variable at `ci` with
 *        explicit memory order.
 *
 * @param[out] ci Pointer to the `ConcurrentInt` variable to store the value
 *                into.
 * @param[in] val The value to store.
 * @param[in] order The explicit memory order to use for the store operation.
 */
static inline void ConcurrentIntStoreExplicit(ConcurrentInt *ci, int val,
                                              ConcurrencyMemoryOrder order) {
#ifdef _OPENMP
    ATOMIC_NS atomic_store_explicit(ci, val, order);
#else
    (void)order;
    *ci = val;
#endif
}

/**
 * @brief Replaces the value pointed by `ci` with the maximum of its original
 *        value and `val`.
 *
 * @param[in,out] ci Pointer to the `ConcurrentInt` variable to store the
 *                   maximum value into.
 * @param[in] val Another value.
 *
 * @returns The value pointed by `ci` immediately preceding the effects of this
 *          function.
 */
static inline int ConcurrentIntMax(ConcurrentInt *ci, int val) {
#ifdef _OPENMP
    int old = ATOMIC_NS atomic_load(ci);
    while (val > old) {
        if (ATOMIC_NS atomic_compare_exchange_weak(ci, &old, val)) {
            break;
        }
    }
    return old;
#else
    int ret = *ci;
    if (val > ret) *ci = val;
    return ret;
#endif
}

/**
 * @brief Replaces the value pointed by `ci` with the maximum of its original
 *        value and `val` with explicit memory order.
 *
 * @param[in,out] ci Pointer to the `ConcurrentInt` variable to store the
 *                   maximum value into.
 * @param[in] val Another value.
 * @param[in] success The explicit memory order to use for the compare and
 *                    exchange operation on success.
 * @param[in] failure The explicit memory order to use for the compare and
 *                    exchange operation on failure.
 *
 * @returns The value pointed by `ci` immediately preceding the effects of this
 *          function.
 */
static inline int ConcurrentIntMaxExplicit(ConcurrentInt *ci, int val,
                                           ConcurrencyMemoryOrder success,
                                           ConcurrencyMemoryOrder failure) {
#ifdef _OPENMP
    int old = ATOMIC_NS atomic_load_explicit(ci, failure);
    while (val > old) {
        if (ATOMIC_NS atomic_compare_exchange_weak_explicit(ci, &old, val,
                                                            success, failure)) {
            break;
        }
    }
    return old;
#else
    (void)success;
    (void)failure;
    int ret = *ci;
    if (val > ret) *ci = val;
    return ret;
#endif
}

// ============================== ConcurrentInt64 ==============================

/**
 * @brief Initializes the `ConcurrentInt64` variable at `ci64` to `val`.
 *
 * @param[out] ci64 Pointer to the `ConcurrentInt64` to initialize.
 * @param[in] val Value to initialize to.
 */
static inline void ConcurrentInt64Init(ConcurrentInt64 *ci64, int64_t val) {
#ifdef _OPENMP
    ATOMIC_NS atomic_init(ci64, val);
#else
    *ci64 = val;
#endif
}

/**
 * @brief Returns the current value of the `ConcurrentInt64` variable at `ci64`.
 *
 * @details In a multithreaded context, the load operation is atomic with
 * default memory order.
 *
 * @param[in] ci64 Pointer to the `ConcurrentInt64` variable to load the value
 *                 from.
 *
 * @returns The current value of the `ConcurrentInt64` variable.
 */
static inline int64_t ConcurrentInt64Load(const ConcurrentInt64 *ci64) {
#ifdef _OPENMP
    return ATOMIC_NS atomic_load(ci64);
#else
    return *ci64;
#endif
}

/**
 * @brief Returns the current value of the `ConcurrentInt64` variable at `ci64`
 *        with explicit memory order.
 *
 * @param[in] ci64 Pointer to the `ConcurrentInt64` variable to load the value
 *                 from.
 * @param[in] order The explicit memory order to use for the load operation.
 *
 * @returns The current value of the `ConcurrentInt64` variable.
 */
static inline int64_t ConcurrentInt64LoadExplicit(
    const ConcurrentInt64 *ci64, ConcurrencyMemoryOrder order) {
#ifdef _OPENMP
    return ATOMIC_NS atomic_load_explicit(ci64, order);
#else
    (void)order;
    return *ci64;
#endif
}

/**
 * @brief Stores the value `val` into the `ConcurrentInt64` variable at `ci64`.
 *
 * @details In a multithreaded context, the store operation is atomic with
 * default memory order.
 *
 * @param[out] ci64 Pointer to the `ConcurrentInt64` variable to store the value
 *                  into.
 * @param[in] val The value to store.
 */
static inline void ConcurrentInt64Store(ConcurrentInt64 *ci64, int64_t val) {
#ifdef _OPENMP
    ATOMIC_NS atomic_store(ci64, val);
#else
    *ci64 = val;
#endif
}

/**
 * @brief Stores the value `val` into the `ConcurrentInt64` variable at `ci64`
 *        with explicit memory order.
 *
 * @param[out] ci64 Pointer to the `ConcurrentInt64` variable to store the value
 *                  into.
 * @param[in] val The value to store.
 * @param[in] order The explicit memory order to use for the store operation.
 */
static inline void ConcurrentInt64StoreExplicit(ConcurrentInt64 *ci64,
                                                int64_t val,
                                                ConcurrencyMemoryOrder order) {
#ifdef _OPENMP
    ATOMIC_NS atomic_store_explicit(ci64, val, order);
#else
    (void)order;
    *ci64 = val;
#endif
}

// ============================ ConcurrentSizeType ============================

/**
 * @brief Initializes the `ConcurrentSizeType` variable at `cs` to `val`.
 *
 * @param[out] cs Pointer to the `ConcurrentSizeType` to initialize.
 * @param[in] val Value to initialize to.
 */
static inline void ConcurrentSizeTypeInit(ConcurrentSizeType *cs, size_t val) {
#ifdef _OPENMP
    ATOMIC_NS atomic_init(cs, val);
#else
    *cs = val;
#endif  // _OPENMP
}

/**
 * @brief Returns the current value of the `ConcurrentSizeType` variable at
 *        `cs`.
 *
 * @details In a multithreaded context, the load operation is atomic with
 * default memory order.
 *
 * @param[in] cs Pointer to the `ConcurrentSizeType` variable to load the value
 *               from.
 *
 * @returns The current value of the `ConcurrentSizeType` variable.
 */
static inline size_t ConcurrentSizeTypeLoad(const ConcurrentSizeType *cs) {
#ifdef _OPENMP
    return ATOMIC_NS atomic_load(cs);
#else
    return *cs;
#endif  // _OPENMP
}

/**
 * @brief Returns the current value of the `ConcurrentSizeType` variable at `cs`
 *        with explicit memory order.
 *
 * @param[in] cs Pointer to the `ConcurrentSizeType` variable to load the value
 *               from.
 * @param[in] order The explicit memory order to use for the load operation.
 *
 * @returns The current value of the `ConcurrentSizeType` variable.
 */
static inline size_t ConcurrentSizeTypeLoadExplicit(
    const ConcurrentSizeType *cs, ConcurrencyMemoryOrder order) {
#ifdef _OPENMP
    return ATOMIC_NS atomic_load_explicit(cs, order);
#else
    (void)order;
    return *cs;
#endif  // _OPENMP
}

/**
 * @brief Subtracts `val` from the `ConcurrentSizeType` variable at `cs` if and
 *        only if its current value is greater than or equal to `val`.
 *
 * @details In a multithreaded context, the subtraction is atomic with default
 * memory order.
 *
 * @param[in,out] cs Pointer to the `ConcurrentSizeType` variable to subtract
 *                   the value from.
 * @param[in] val Value to subtract.
 *
 * @retval true If the subtraction is successfully completed.
 * @retval false If the current value of `cs` is smaller than `val`.
 */
static inline bool ConcurrentSizeTypeSubtractIfGreaterEqual(
    ConcurrentSizeType *cs, size_t val) {
#ifdef _OPENMP
    size_t cur_size = ATOMIC_NS atomic_load(cs);
    while (cur_size >= val) {
        bool success = ATOMIC_NS atomic_compare_exchange_weak(cs, &cur_size,
                                                              cur_size - val);
        if (success) return true;
    }
    return false;
#else
    if (*cs < val) return false;
    *cs -= val;
    return true;
#endif  // _OPENMP
}

/**
 * @brief Subtracts `val` from the `ConcurrentSizeType` variable at `cs` if and
 *        only if its current value is greater than or equal to `val`, with
 *        explicit memory order.
 *
 * @param[in,out] cs Pointer to the `ConcurrentSizeType` variable to subtract
 *                   the value from.
 * @param[in] val Value to subtract.
 * @param[in] success The explicit memory order to use for the compare and
 *                    exchange operation on success.
 * @param[in] failure The explicit memory order to use for the compare and
 *                    exchange operation on failure.
 *
 * @retval true If the subtraction is successfully completed.
 * @retval false If the current value of `cs` is smaller than `val`.
 */
static inline bool ConcurrentSizeTypeSubtractIfGreaterEqualExplicit(
    ConcurrentSizeType *cs, size_t val, ConcurrencyMemoryOrder success,
    ConcurrencyMemoryOrder failure) {
#ifdef _OPENMP
    size_t cur_size = ATOMIC_NS atomic_load_explicit(cs, failure);
    while (cur_size >= val) {
        bool op_success = ATOMIC_NS atomic_compare_exchange_weak_explicit(
            cs, &cur_size, cur_size - val, success, failure);
        if (op_success) return true;
    }
    return false;
#else
    (void)success;
    (void)failure;
    if (*cs < val) return false;
    *cs -= val;
    return true;
#endif  // _OPENMP
}

/**
 * @brief Adds `val` to the `ConcurrentSizeType` variable at `cs` and returns
 *        its original value.
 *
 * @details In a multithreaded context, the addition is atomic with default
 * memory order.
 *
 * @param[in,out] cs Pointer to the `ConcurrentSizeType` variable to add the
 *                   value to.
 * @param[in] val Value to add.
 *
 * @returns The original value in `cs` before the addition.
 */
static inline size_t ConcurrentSizeTypeAdd(ConcurrentSizeType *cs, size_t val) {
#ifdef _OPENMP
    return ATOMIC_NS atomic_fetch_add(cs, val);
#else
    size_t ret = *cs;
    *cs += val;
    return ret;
#endif  // _OPENMP
}

/**
 * @brief Adds `val` to the `ConcurrentSizeType` variable at `cs` and returns
 *        its original value, using explicit memory order.
 *
 * @param[in,out] cs Pointer to the `ConcurrentSizeType` variable to add the
 *                   value to.
 * @param[in] val Value to add.
 * @param[in] order The explicit memory order to use for the addition.
 *
 * @returns The original value in `cs` before the addition.
 */
static inline size_t ConcurrentSizeTypeAddExplicit(
    ConcurrentSizeType *cs, size_t val, ConcurrencyMemoryOrder order) {
#ifdef _OPENMP
    return ATOMIC_NS atomic_fetch_add_explicit(cs, val, order);
#else
    (void)order;
    size_t ret = *cs;
    *cs += val;
    return ret;
#endif  // _OPENMP
}

/**
 * @brief Subtracts `val` from the `ConcurrentSizeType` variable at `cs` and
 *        returns its original value.
 *
 * @details In a multithreaded context, the subtraction is atomic with default
 * memory order.
 *
 * @param[in,out] cs Pointer to the `ConcurrentSizeType` variable to subtract
 *                   the value from.
 * @param[in] val Value to subtract.
 *
 * @returns The original value in `cs` before the subtraction.
 */
static inline size_t ConcurrentSizeTypeSubtract(ConcurrentSizeType *cs,
                                                size_t val) {
#ifdef _OPENMP
    return ATOMIC_NS atomic_fetch_sub(cs, val);
#else
    size_t ret = *cs;
    *cs -= val;
    return ret;
#endif  // _OPENMP
}

/**
 * @brief Subtracts `val` from the `ConcurrentSizeType` variable at `cs` and
 *        returns its original value, using explicit memory order.
 *
 * @param[in,out] cs Pointer to the `ConcurrentSizeType` variable to subtract
 *                   the value from.
 * @param[in] val Value to subtract.
 * @param[in] order The explicit memory order to use for the subtraction.
 *
 * @returns The original value in `cs` before the subtraction.
 */
static inline size_t ConcurrentSizeTypeSubtractExplicit(
    ConcurrentSizeType *cs, size_t val, ConcurrencyMemoryOrder order) {
#ifdef _OPENMP
    return ATOMIC_NS atomic_fetch_sub_explicit(cs, val, order);
#else
    (void)order;
    size_t ret = *cs;
    *cs -= val;
    return ret;
#endif  // _OPENMP
}

// ========================== OpenMP Thread Utilities ==========================

/**
 * @brief Returns the number of OpenMP threads available. Returns `1` if
 *        OpenMP is disabled.
 *
 * @returns Number of OpenMP threads available.
 */
static inline int ConcurrencyGetOmpNumThreads(void) {
#ifdef _OPENMP
    return omp_get_max_threads();
#else   // _OPENMP not defined.
    return 1;
#endif  // _OPENMP
}

/**
 * @brief Returns the caller's OpenMP thread ID. Returns `0` if OpenMP is
 *        disabled.
 *
 * @returns The caller's OpenMP thread ID.
 */
static inline int ConcurrencyGetOmpThreadId(void) {
#ifdef _OPENMP
    return omp_get_thread_num();
#else   // _OPENMP not defined, thread 0 is the only available thread.
    return 0;
#endif  // _OPENMP
}

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // GAMESMANONE_CORE_CONCURRENCY_H_
