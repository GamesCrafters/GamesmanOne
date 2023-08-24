/**
 * @file gamesman_types.h
 * @author Robert Shi (robertyishi@berkeley.edu): adapted and further optimized
 * for efficiency and readability
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Declarations of GAMESMAN types.
 * @version 1.0
 * @date 2023-08-19
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

#ifndef GAMESMANEXPERIMENT_CORE_GAMESMAN_TYPES_H_
#define GAMESMANEXPERIMENT_CORE_GAMESMAN_TYPES_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/data_structures/int64_array.h"
#include "core/data_structures/int64_hash_map.h"
#include "core/data_structures/int64_queue.h"

/**
 * @brief Tier as a 64-bit integer.
 */
typedef int64_t Tier;

/**
 * @brief Game position as a 64-bit integer hash.
 */
typedef int64_t Position;

/**
 * @brief Game move as a 64-bit integer.
 */
typedef int64_t Move;

/**
 * @brief Possible values of a game position.
 *
 * @note Always make sure that kUndecided is 0 as other components rely on this
 * assumption.
 */
typedef enum { kUndecided, kLose, kDraw, kTie, kWin, kErrorValue } Value;

/**
 * @brief Dynamic Position array.
 */
typedef Int64Array PositionArray;

/**
 * @brief Linear-probing Position hash set using Int64HashMap.
 */
typedef Int64HashMap PositionHashSet;

/**
 * @brief Dynamic Move array.
 */
typedef Int64Array MoveArray;

/**
 * @brief Dynamic Tier array.
 */
typedef Int64Array TierArray;

/**
 * @brief Dynamic Tier stack using Int64Array.
 */
typedef Int64Array TierStack;

/**
 * @brief Dynamic Tier queue using Int64Queue.
 */
typedef Int64Queue TierQueue;

/**
 * @brief Linear-probing Tier hash map using Int64HashMap.
 */
typedef Int64HashMap TierHashMap;

/**
 * @brief Iterator for TierHashMap.
 */
typedef Int64HashMapIterator TierHashMapIterator;

/**
 * @brief Linear-probing Tier hash set using Int64HashMap.
 */
typedef Int64HashMap TierHashSet;

/**
 * @brief Tier and Position. In Tier games, a position is uniquely identified by
 * the Tier it belongs and its Position hash inside that tier.
 */
typedef struct TierPosition {
    Tier tier;
    Position position;
} TierPosition;

/**
 * @brief Dynamic TierPositionArray.
 */
typedef struct TierPositionArray {
    TierPosition *array;
    int64_t size;
    int64_t capacity;
} TierPositionArray;

/**
 * @brief Entry in a TierPositionHashSet.
 */
typedef struct TierPositionHashSetEntry {
    TierPosition key;
    bool used;
} TierPositionHashSetEntry;

/**
 * @brief Linear-probing TierPosition hash set.
 */
typedef struct TierPositionHashSet {
    TierPositionHashSetEntry *entries;
    int64_t capacity;
    int64_t size;
    double max_load_factor;
} TierPositionHashSet;

/**
 * @brief Constant limits of GAMESMAN types.
 */
typedef enum GamesmanTypesLimits {
    /**
     * Largest remoteness expected. Increase this value and recompile if
     * this value is not large enough for a game in the future.
     */
    kRemotenessMax = 1023,
    kDbNameLengthMax = 31,
    kDbFormalNameLengthMax = 63,
    kSolverOptionNameLengthMax = 63,
    kSolverNameLengthMax = 63,
    kGameVariantOptionNameMax = 63,
    kGameNameLengthMax = 31,
    kGameFormalNameLengthMax = 127,
} GamesmanTypesLimits;

/**
 * @brief Database probe which can be used to probe the database on permanent
 * storage (disk). To access in-memory DB, use Database's "Solving API" instead.
 */
typedef struct DbProbe {
    Tier tier;
    void *buffer;
    int64_t begin;
    int64_t size;
} DbProbe;

/**
 * @brief Generic Tier Database type.
 * @note To implement a new Database module, correctly set the name of the new
 * DB and set each member function pointer to a function specific to the module.
 *
 * @note ALL Databases are Tier Databases because the game designer does not
 * have to interact with a Database directly.
 */
typedef struct Database {
    /** Internal name of the Database. Must contain no white spaces. */
    char name[kDbNameLengthMax + 1];

    /** Human-readable name of the Database. */
    char formal_name[kDbFormalNameLengthMax + 1];

    /**
     * @brief Initializes the Database.
     *
     * @param game_name Internal name of the game.
     * @param variant Index of the selected game variant.
     * @param path Path to the directory to which the Database have full access.
     * The Database may choose to store files directly inside this directory or
     * make sub-directories as the DB designer sees fit.
     * @param aux Auxiliary parameter.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Init)(const char *game_name, int variant, const char *path,
                void *aux);

    /** @brief Finalizes the Database, freeing all allocated space. */
    void (*Finalize)(void);

    // Solving API

    /**
     * @brief Creates a in-memory DB for solving of the given TIER of SIZE
     * positions.
     * @note This function is part of the Solving API.
     *
     * @param tier Tier to be solved and stored in memory.
     * @param size Size of the TIER in number of Positions.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*CreateSolvingTier)(Tier tier, int64_t size);

    /**
     * @brief Flushes the in-memory DB to disk.
     * @note This function is part of the Solving API.
     *
     * @param aux Auxiliary parameter.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*FlushSolvingTier)(void *aux);

    /**
     * @brief Frees the in-memory DB.
     * @note This function is part of the Solving API.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*FreeSolvingTier)(void);

    /**
     * @brief Sets the value of POSITION to VALUE.
     * @note This function is part of the Solving API.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*SetValue)(Position position, Value value);

    /**
     * @brief Sets the remoteness of POSITION to REMOTENESS.
     * @note This function is part of the Solving API.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*SetRemoteness)(Position position, int remoteness);

    /**
     * @brief Returns the value of the given POSITION from in-memory DB.
     * @note This function is part of the Solving API.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    Value (*GetValue)(Position position);

    /**
     * @brief Returns the remoteness of the given POSITION from in-memory DB.
     * @note This function is part of the Solving API.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*GetRemoteness)(Position position);

    // Probing API

    /**
     * @brief Initializes the given Database PROBE.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*ProbeInit)(DbProbe *probe);

    /**
     * @brief Frees the given Database PROBE.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*ProbeDestroy)(DbProbe *probe);

    /**
     * @brief Probes the value of TIER_POSITION from permanent storage using
     * PROBE and returns it.
     *
     * @note Assumes PROBE is initialized. Results in undefined behavior if
     * PROBE is NULL or uninitialized.
     *
     * @param probe Database probe initialized using the ProbeInit() function.
     * @param tier_position Probe the value of this TierPosition.
     *
     * @return Value of TIER_POSITION probed from pernament storage, or
     * kErrorValue if TIER_POSITION is not found.
     */
    Value (*ProbeValue)(DbProbe *probe, TierPosition tier_position);

    /**
     * @brief Probes the remoteness of TIER_POSITION from permanent storage
     * using PROBE and returns it.
     *
     * @note Assumes PROBE is initialized. Results in undefined behavior if
     * PROBE is NULL or uninitialized.
     *
     * @param probe Database probe initialized using the ProbeInit() function.
     * @param tier_position Probe the remoteness of this TierPosition.
     *
     * @return Remoteness of TIER_POSITION probed from pernament storage, or
     * -1 if TIER_POSITION is not found.
     */
    int (*ProbeRemoteness)(DbProbe *probe, TierPosition tier_position);
} Database;

/** @brief Solver option for display in GAMESMAN interactive mode. */
typedef struct SolverOption {
    /** Human-readable name of the option. */
    char name[kSolverOptionNameLengthMax + 1];

    /** Number of choices associated with the option. */
    int num_choices;

    /** An array of strings, where each string is a name of a choice. Length =
     * num_choices.  */
    const char *const *choices;
} SolverOption;

/** @brief Solver configuration as an array of selected solver options. */
typedef struct SolverConfiguration {
    /**
     * Zero-terminated array of solver options. The user of this struct must
     * make sure that the last item in this array is completely zeroed out.
     */
    const SolverOption *options;

    /**
     * Array of selected choice indices to each option. Zero-terminated and
     * aligned to the options array (same number of options and selections.)
     */
    const int *selections;
} SolverConfiguration;

/**
 * @brief Generic Solver type.
 * @note To implement a new Solver module, correctly set the name of the new
 * Solver and set each member function pointer to a function specific to the
 * module.
 *
 * @note A Solver can either be a regular solver or a tier solver. The actual
 * behavior and requirements of the solver is decided by the Solver and
 * reflected on its SOLVER_API, which is a custom struct defined in the Solver
 * module and implemented by the game developer. Therefore, the game developer
 * must decide which Solver to use and implement the required Solver API
 * functions.
 */
typedef struct Solver {
    /** Human-readable name of the solver. */
    char name[kSolverNameLengthMax + 1];

    /** Database used by the solver. */
    const Database *db;

    /**
     * @brief Initializes the Solver.
     * @note The user is responsible for passing the correct solver API that
     * appies to the current Solver. Passing NULL or an incompatible Solver API
     * results in undefined behavior.
     *
     * @param solver_api Pointer to a struct that contains the implemented
     * Solver API functions. The game developer is responsible for using the
     * correct type of Solver API that applies to the current Solver,
     * implementing and setting up required API functions, and casting the
     * pointer to a generic constant pointer (const void *) type.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Init)(const void *solver_api);

    /**
     * @brief Finalizes the Solver, freeing all allocated memory.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Finalize)(void);

    /**
     * @brief Runs the solver to solve the current game. Also stores the result
     * if a Database is set for the current Solver.
     *
     * @param aux Auxiliary parameter.
     */
    int (*Solve)(void *aux);

    /**
     * @brief Returns the solving status of the current game.
     *
     * @return Status code as defined by the actual Solver module.
     */
    int (*GetStatus)(void);

    /** @brief Returns the current configuration of this Solver. */
    const SolverConfiguration *(*GetCurrentConfiguration)(void);

    /**
     * @brief Sets the solver option with index OPTION to the choice of index
     * SELECTION.
     *
     * @param option Solver option index.
     * @param selection Choice index to select.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*SetOption)(int option, int selection);
} Solver;

/** @brief Game variant option for display in GAMESMAN interactive mode. */
typedef struct GameVariantOption {
    /** Human-readable name of the option. */
    char name[kGameVariantOptionNameMax + 1];

    /** Number of choices associate with the option. */
    int num_choices;

    /** An array of strings, where each string is a name of a choice. Length =
     * num_choices.  */
    const char *const *choices;
} GameVariantOption;

/**
 * @brief Game variant as an array of selected variant options.
 *
 * @details A game variant is determined by a set of variant options. Each
 * variant option decides some aspect of the game rule. The game developer is
 * responsible for providing the possible choices for each one of the variant
 * options as strings (see GameVariantOption::choices). The user of GAMESMAN
 * interactive can then set the variant by selecting a value for each option
 * using the game-specific SetVariantOption().
 *
 * @example A Tic-Tac-Toe game can be generalized and played on a M by N board
 * with a goal of connecting K pieces in a row. Then, we can have three game
 * variant options "dimension M", "dimension N", and "number of pieces to
 * connect (K)." A board too small can make the game less interesting, whereas a
 * board too large can render the game unsolvable. Therefore, the game developer
 * decides to allow M, N, and K to all be in the range [2, 5], and sets the
 * corresponding choices to {"2", "3", "4", "5"}, for each one of the three
 * GameVariantOptions.
 */
typedef struct GameVariant {
    /**
     * Zero-terminated array of game variant options. The user of this struct
     * must make sure that the last item in this array is completely zeroed out.
     */
    const GameVariantOption *options;

    /**
     * Array of selected choice indices to each option. Zero-terminated and
     * aligned to the options array (same number of options and selections.)
     */
    const int *selections;
} GameVariant;

typedef struct GameplayApi {
    Tier (*GetInitialTier)(void);
    Position (*GetInitialPosition)(void);

    int position_string_length_max;
    int (*PositionToString)(Position position, char *buffer);
    int (*TierPositionToString)(TierPosition tier_position, char *buffer);

    int move_string_length_max;
    int (*MoveToString)(Move move, char *buffer);

    bool (*IsValidMoveString)(const char *move_string);
    Move (*StringToMove)(const char *move_string);

    MoveArray (*GenerateMoves)(Position position);
    MoveArray (*TierGenerateMoves)(TierPosition tier_position);

    Position (*DoMove)(Position position, Move move);
    TierPosition (*TierDoMove)(TierPosition tier_position, Move move);

    Value (*Primitive)(Position position);
    Value (*TierPrimitive)(TierPosition tier_position);

    Position (*GetCanonicalPosition)(Position position);
    Position (*TierGetCanonicalPosition)(TierPosition tier_position);

    Tier (*GetCanonicalTier)(Tier tier);
    Position (*GetPositionInSymmetricTier)(TierPosition tier_position,
                                           Tier symmetric);
} GameplayApi;

typedef struct Game {
    char name[kGameNameLengthMax + 1];
    char formal_name[kGameFormalNameLengthMax + 1];
    const Solver *solver;
    const void *solver_api;
    const GameplayApi *gameplay_api;

    int (*Init)(void *aux);
    int (*Finalize)(void);

    const GameVariant *(*GetCurrentVariant)(void);
    int (*SetVariantOption)(int option, int selection);
} Game;

typedef enum IntBase10StringLengthLimits {
    kInt8Base10StringLengthMax = 4,     // [-128, 127]
    kUint8Base10StringLengthMax = 3,    // [0, 255]
    kInt16Base10StringLengthMax = 6,    // [-32768, 32767]
    kUint16Base10StringLengthMax = 5,   // [0, 65535]
    kInt32Base10StringLengthMax = 11,   // [-2147483648, 2147483647]
    kUint32Base10StringLengthMax = 10,  // [0, 4294967295]

    // [-9223372036854775808, 9223372036854775807]
    kInt64Base10StringLengthMax = 20,
    kUint64Base10StringLengthMax = 20,  // [0, 18446744073709551615]
} IntBase10StringLengthLimits;

#endif  // GAMESMANEXPERIMENT_CORE_GAMESMAN_TYPES_H_
