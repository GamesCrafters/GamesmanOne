/**
 * @file gamesman_types.h
 * @author Robert Shi (robertyishi@berkeley.edu)
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
 * @brief Pointer to a read-only string cannot be used to modify the contents of
 * the string.
 */
typedef const char *ReadOnlyString;

/**
 * @brief Constant pointer to a read-only string. The pointer is constant, which
 * means its value cannot be changed once it is initialized. The string is
 * read-only, which means that the pointer cannot be used to modify the contents
 * of the string.
 */
typedef const ReadOnlyString ConstantReadOnlyString;

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
typedef enum Value {
    kErrorValue = -1,
    kUndecided = 0,
    kLose,
    kDraw,
    kTie,
    kWin,
    kNumValues,  /**< The number of valid Values. */
} Value;

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
 * @brief Linear-probing Tier to int64_t hash map using Int64HashMap.
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
    kNumRemotenesses = 1024,
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
 *
 * @note To implement a new Database module, properly set the name of the new
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
    int (*Init)(ReadOnlyString game_name, int variant, ReadOnlyString path,
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

    /** An array of strings, where each string is the name of a choice. Length =
     * num_choices.  */
    const ConstantReadOnlyString *choices;
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

    /**
     * @brief Initializes the Solver.
     * @note The user is responsible for passing the correct solver API that
     * appies to the current Solver. Passing NULL or an incompatible Solver API
     * results in undefined behavior.
     *
     * @param game_name Internal name of the game.
     * @param variant Index of the current game variant.
     * @param solver_api Pointer to a struct that contains the implemented
     * Solver API functions. The game developer is responsible for using the
     * correct type of Solver API that applies to the current Solver,
     * implementing and setting up required API functions, and casting the
     * pointer to a generic constant pointer (const void *) type.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Init)(ReadOnlyString game_name, int variant, const void *solver_api);

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
    const ConstantReadOnlyString *choices;
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

/**
 * @brief GAMESMAN interactive game play API.
 *
 * @details There are two sets of APIs, one for tier games and one for non-tier
 * games. The game developer should implement exactly one of the two APIs and
 * set all irrelavent fields to zero (0) or NULL. If neither is fully
 * implemented, the game will be rejected by the gameplay system. Implementing
 * both APIs results in undefined behavior.
 */
typedef struct GameplayApi {
    /**
     * @brief Returns the tier in which the initial position belongs to.
     * Different game variants may have different initial tiers.
     *
     * @note Required for TIER games. The system will not recognize the game
     * as a tier game if this function is not implemented.
     */
    Tier (*GetInitialTier)(void);

    /**
     * @brief If the game is a tier game, returns the initial position inside
     * the initial tier. Otherwise, returns the initial position. Different game
     * variants may have different initial positions.
     *
     * @note Required for ALL games. The gameplay system will reject the game if
     * this function is not implemented.
     */
    Position (*GetInitialPosition)(void);

    /**
     * @brief Maximum length of a position string not including the
     * NULL-terminating character ('\0') as a constant integer.
     *
     * @details The gameplay system will try to allocate this amount of space
     * plus one additional byte for the NULL-terminating character ('\0') as
     * buffer for position strings. It is the game developer's responsibility to
     * precalculate this value and make sure that enough space is provided.
     *
     * @note Required for ALL games. The gameplay system will reject the game if
     * this function is not implemented.
     */
    int position_string_length_max;

    /**
     * @brief Converts POSITION into a position string and stores it in BUFFER.
     *
     * @note Assumes that POSITION is valid and BUFFER has enough space to hold
     * the position string. Results in undefined behavior otherwise.
     *
     * @note Required for NON-TIER games. The system will not recognize the
     * game as a non-tier game if this function is not implemented.
     */
    int (*PositionToString)(Position position, char *buffer);

    /**
     * @brief Converts TIER_POSITION into a position string and stores it in
     * BUFFER.
     *
     * @note Assumes that TIER_POSITION is valid and BUFFER has enough space to
     * hold the position string. Results in undefined behavior otherwise.
     *
     * @note Required for TIER games. The system will not recognize the game
     * as a tier game if this function is not implemented.
     */
    int (*TierPositionToString)(TierPosition tier_position, char *buffer);

    /**
     * @brief Maximum length of a move string not including the NULL-terminating
     * character ('\0') as a constant integer.
     *
     * @details The gameplay system will try to allocate this amount of space
     * plus one additional byte for the NULL-terminating character ('\0') as
     * buffer for move strings. It is the game developer's responsibility to
     * precalculate this value and make sure that enough space is provided.
     *
     * @note Required for ALL games. The gameplay system will reject the game if
     * this function is not implemented.
     */
    int move_string_length_max;

    /**
     * @brief Converts MOVE into a move string and stores it in BUFFER.
     *
     * @note Assumes that MOVE is valid and BUFFER has enough space to hold the
     * move string. Results in undefined behavior otherwise.
     *
     * @note Required for ALL games. The gameplay system will reject the game if
     * this function is not implemented.
     */
    int (*MoveToString)(Move move, char *buffer);

    /**
     * @brief Returns true if the given MOVE_STRING is recognized as a valid
     * move string for the current game, or false otherwise.
     *
     * @param move_string User (typically the user of GAMESMAN interactive
     * through the text user interface) provided move string to be validated.
     *
     * @note Required for ALL games. The gameplay system will reject the game if
     * this function is not implemented.
     */
    bool (*IsValidMoveString)(ReadOnlyString move_string);

    /**
     * @brief Converts the MOVE_STRING to a Move and returns it.
     *
     * @note Assmues that the given MOVE_STRING is valid. Results in undefined
     * behavior otherwise. Therefore, it is the developer of the gameplay
     * system's responsibility to validate the MOVE_STRING using the
     * IsValidMoveString() function before calling this function.
     *
     * @note Required for ALL games. The gameplay system will reject the game if
     * this function is not implemented.
     */
    Move (*StringToMove)(ReadOnlyString move_string);

    /**
     * @brief Returns an array of available moves at the given POSITION.
     *
     * @note Assumes that POSITION is valid. Results in undefined behavior
     * otherwise.
     *
     * @note Required for NON-TIER games. The system will not recognize the game
     * as a non-tier game if this function is not implemented.
     */
    MoveArray (*GenerateMoves)(Position position);

    /**
     * @brief Returns an array of available moves at the given TIER_POSITION.
     *
     * @note Assumes that TIER_POSITION is valid. Results in undefined behavior
     * otherwise.
     *
     * @note Required for TIER games. The system will not recognize the game
     * as a tier game if this function is not implemented.
     */
    MoveArray (*TierGenerateMoves)(TierPosition tier_position);

    /**
     * @brief Returns the resulting position after performing the given MOVE at
     * the given POSITION.
     *
     * @note Assumes that POSITION is valid and MOVE is a valid move at
     * POSITION. Passing an illegal position or an illegal move results in
     * undefined behavior.
     *
     * @note Required for NON-TIER games. The system will not recognize the game
     * as a non-tier game if this function is not implemented.
     */
    Position (*DoMove)(Position position, Move move);

    /**
     * @brief Returns the resulting tier position after performing the given
     * MOVE at the given TIER_POSITION.
     *
     * @note Assumes that TIER_POSITION is valid and MOVE is a valid move at
     * TIER_POSITION. Passing an invalid tier, an illegal position within the
     * tier, or an illegal move results in undefined behavior.
     *
     * @note Required for TIER games. The system will not recognize the game
     * as a tier game if this function is not implemented.
     */
    TierPosition (*TierDoMove)(TierPosition tier_position, Move move);

    /**
     * @brief Returns the value of the given POSITION if it is primitive, or
     * kUndecided otherwise.
     *
     * @note Assumes POSITION is valid. Passing an illegal position results in
     * undefined behavior.
     *
     * @note Required for NON-TIER games. The system will not recognize the game
     * as a non-tier game if this function is not implemented.
     */
    Value (*Primitive)(Position position);

    /**
     * @brief Returns the value of the given TIER_POSITION if it is primitive,
     * or kUndecided otherwise.
     *
     * @note Assumes TIER_POSITION is valid. Passing an invalid tier or an
     * illegal position within the tier results in undefined behavior.
     *
     * @note Required for TIER games. The system will not recognize the game
     * as a tier game if this function is not implemented.
     */
    Value (*TierPrimitive)(TierPosition tier_position);

    /**
     * @brief Returns the canonical position that is symmetric to POSITION.
     *
     * @details By convention, a canonical position is one with the smallest
     * hash value in a set of symmetrical positions. For each position[i] within
     * the set including the canonical position itself, calling
     * GetCanonicalPosition on position[i] returns the canonical position.
     *
     * @note Assumes POSITION is valid. Passing an illegal position to this
     * function results in undefined behavior.
     *
     * @note Required for NON-TIER games ONLY IF position symmetry removal
     * optimization was used to solve the game. Set to NULL otherwise.
     * Inconsistent usage of this function between solving and gameplay results
     * in undefined behavior.
     */
    Position (*GetCanonicalPosition)(Position position);

    /**
     * @brief Returns the canonical position within the same tier that is
     * symmetric to TIER_POSITION.
     *
     * @details GAMESMAN currently does not support position symmetry removal
     * across tiers. By convention, a canonical position is one with the
     * smallest hash value in a set of symmetrical positions which all belong to
     * the same tier. For each position[i] within the set including the
     * canonical position itself, calling GetCanonicalPosition on position[i]
     * returns the canonical position.
     *
     * @note Assumes TIER_POSITION is valid. Passing an invalid tier or an
     * illegal position within the tier to this function results in undefined
     * behavior.
     *
     * @note Required for TIER games ONLY IF position symmetry removal
     * optimization was used to solve the game. Set to NULL otherwise.
     * Inconsistent usage of this function between solving and gameplay results
     * in undefined behavior.
     */
    Position (*TierGetCanonicalPosition)(TierPosition tier_position);

    /**
     * @brief Returns the canonical tier symmetric to TIER. Returns TIER if TIER
     * itself is canonical.
     *
     * @details By convention, a canonical tier is one with the smallest tier
     * value in a set of symmetrical tiers. For each tier[i] within the set
     * including the canonical tier itself, calling GetCanonicalTier(tier[i])
     * returns the canonical tier.
     *
     * @note Assumes TIER is valid. Passing an invalid
     * tier to this function results in undefined behavior.
     *
     * @note Required for TIER games ONLY IF tier symmetry removal optimization
     * was used to solve the game. Set to NULL otherwise. Inconsistent usage of
     * this function between solving and gameplay results in undefined behavior.
     */
    Tier (*GetCanonicalTier)(Tier tier);

    /**
     * @brief Returns the position, which is symmetric to the given
     * TIER_POSITION, in SYMMETRIC tier.
     *
     * @note Assumes both TIER_POSITION and SYMMETRIC are valid. Furthermore,
     * assumes that the tier as specified by TIER_POSITION is symmetric to the
     * SYMMETRIC tier; that is, calling GetCanonicalTier(tier_position.tier)
     * and calling GetCanonicalTier(symmetric) returns the same canonical tier.
     *
     * @note Required for TIER games ONLY IF tier symmetry removal optimization
     * was used to solve the game. Set to NULL otherwise. Inconsistent usage of
     * this function between solving and gameplay results in undefined behavior.
     */
    Position (*GetPositionInSymmetricTier)(TierPosition tier_position,
                                           Tier symmetric);
} GameplayApi;

/**
 * @brief Generic Game type.
 *
 * @details A game should have an internal name, a human-readable formal name
 * for textUI display, a Solver to use, a set of implemented API functions for
 * the chosen Solver, a set of implemented API functions for the game play
 * system, functions to initialize and finalize the game module, and functions
 * to get/set the current game variant. The solver interface is required for
 * solving the game. The gameplay interface is required for textUI play loop
 * and debugging. The game variant interface is optional, and may be set to
 * NULL if there is only one variant.
 */
typedef struct Game {
    /** Internal name of the game, must contain no white spaces or special
     * characters. */
    char name[kGameNameLengthMax + 1];

    /** Human-readable name of the game. */
    char formal_name[kGameFormalNameLengthMax + 1];

    /** Solver to use. */
    const Solver *solver;

    /** Pointer to an object containing implemented API functions for the
     * selected Solver. */
    const void *solver_api;

    /** Pointer to a GameplayApi object which contains implemented gameplay API
     * functions. */
    const GameplayApi *gameplay_api;

    /**
     * @brief Initializes the game module.
     *
     * @param aux Auxiliary parameter.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Init)(void *aux);

    /**
     * @brief Finalizes the game module, freeing all allocated memory.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Finalize)(void);

    /**
     * @brief Returns the current variant of the game as a read-only GameVariant
     * object.
     */
    const GameVariant *(*GetCurrentVariant)(void);

    /**
     * @brief Sets the game variant option with index OPTION to the choice of
     * index SELECTION.
     *
     * @param option Index of the option to modify.
     * @param selection Index of the choice to select.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*SetVariantOption)(int option, int selection);
} Game;

/**
 * @brief Precalculated string length limits for integers of different types.
 */
typedef enum IntBase10StringLengthLimits {
    /** int8_t: [-128, 127] */
    kInt8Base10StringLengthMax = 4,

    /** uint8_t: [0, 255] */
    kUint8Base10StringLengthMax = 3,

    /** int16_t: [-32768, 32767] */
    kInt16Base10StringLengthMax = 6,

    /** uint16_t: [0, 65535] */
    kUint16Base10StringLengthMax = 5,

    /** int32_t: [-2147483648, 2147483647] */
    kInt32Base10StringLengthMax = 11,

    /** uint32_t: [0, 4294967295] */
    kUint32Base10StringLengthMax = 10,

    /** int64_t: [-9223372036854775808, 9223372036854775807] */
    kInt64Base10StringLengthMax = 20,

    /** uint64_t: [0, 18446744073709551615] */
    kUint64Base10StringLengthMax = 20,
} IntBase10StringLengthLimits;

typedef enum CommonConstants {
    kBitsPerByte = 8,
} CommonConstants;

// GAMESMAN Types Related Accessor and Mutator Functions.

void PositionArrayInit(PositionArray *array);
void PositionArrayDestroy(PositionArray *array);
bool PositionArrayAppend(PositionArray *array, Position position);
bool PositionArrayContains(PositionArray *array, Position position);

void PositionHashSetInit(PositionHashSet *set, double max_load_factor);
void PositionHashSetDestroy(PositionHashSet *set);
bool PositionHashSetContains(PositionHashSet *set, Position position);
bool PositionHashSetAdd(PositionHashSet *set, Position position);

void MoveArrayInit(MoveArray *array);
void MoveArrayDestroy(MoveArray *array);
bool MoveArrayAppend(MoveArray *array, Move move);
bool MoveArrayPopBack(MoveArray *array);
bool MoveArrayContains(const MoveArray *array, Move move);

void TierArrayInit(TierArray *array);
void TierArrayDestroy(TierArray *array);
bool TierArrayAppend(TierArray *array, Tier tier);

void TierStackInit(TierStack *stack);
void TierStackDestroy(TierStack *stack);
bool TierStackPush(TierStack *stack, Tier tier);
void TierStackPop(TierStack *stack);
Tier TierStackTop(const TierStack *stack);
bool TierStackEmpty(const TierStack *stack);

void TierQueueInit(TierQueue *queue);
void TierQueueDestroy(TierQueue *queue);
bool TierQueueIsEmpty(const TierQueue *queue);
int64_t TierQueueSize(const TierQueue *queue);
bool TierQueuePush(TierQueue *queue, Tier tier);
Tier TierQueuePop(TierQueue *queue);

void TierHashMapInit(TierHashMap *map, double max_load_factor);
void TierHashMapDestroy(TierHashMap *map);
TierHashMapIterator TierHashMapGet(TierHashMap *map, Tier key);
bool TierHashMapSet(TierHashMap *map, Tier tier, int64_t value);
bool TierHashMapContains(TierHashMap *map, Tier tier);

TierHashMapIterator TierHashMapBegin(TierHashMap *map);
Tier TierHashMapIteratorKey(const TierHashMapIterator *it);
int64_t TierHashMapIteratorValue(const TierHashMapIterator *it);
bool TierHashMapIteratorIsValid(const TierHashMapIterator *it);
bool TierHashMapIteratorNext(TierHashMapIterator *iterator, Tier *tier,
                             int64_t *value);

void TierHashSetInit(TierHashSet *set, double max_load_factor);
void TierHashSetDestroy(TierHashSet *set);
bool TierHashSetContains(TierHashSet *set, Tier tier);
bool TierHashSetAdd(TierHashSet *set, Tier tier);

void TierPositionArrayInit(TierPositionArray *array);
void TierPositionArrayDestroy(TierPositionArray *array);
bool TierPositionArrayAppend(TierPositionArray *array,
                             TierPosition tier_position);
TierPosition TierPositionArrayBack(const TierPositionArray *array);

void TierPositionHashSetInit(TierPositionHashSet *set, double max_load_factor);
void TierPositionHashSetDestroy(TierPositionHashSet *set);
bool TierPositionHashSetContains(TierPositionHashSet *set, TierPosition key);
bool TierPositionHashSetAdd(TierPositionHashSet *set, TierPosition key);

int GameVariantToIndex(const GameVariant *variant);

#endif  // GAMESMANEXPERIMENT_CORE_GAMESMAN_TYPES_H_
