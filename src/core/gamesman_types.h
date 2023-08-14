#ifndef GAMESMANEXPERIMENT_CORE_GAMESMAN_TYPES_H_
#define GAMESMANEXPERIMENT_CORE_GAMESMAN_TYPES_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/data_structures/int64_array.h"
#include "core/data_structures/int64_hash_map.h"
#include "core/data_structures/int64_queue.h"

typedef int64_t Position;
typedef int64_t Move;

// Always make sure that kUndecided is 0.
typedef enum { kUndecided, kLose, kDraw, kTie, kWin, kErrorValue } Value;

typedef Int64Array PositionArray;
typedef Int64HashMap PositionHashSet;

typedef Int64Array MoveArray;

typedef int64_t Tier;
typedef Int64Array TierArray;
typedef Int64Array TierStack;
typedef Int64Queue TierQueue;
typedef Int64HashMap TierHashMap;
typedef Int64HashMapIterator TierHashMapIterator;
typedef Int64HashMap TierHashSet;

typedef struct TierPosition {
    Tier tier;
    Position position;
} TierPosition;

typedef struct TierPositionArray {
    TierPosition *array;
    int64_t size;
    int64_t capacity;
} TierPositionArray;

typedef struct TierPositionHashSetEntry {
    TierPosition key;
    bool used;
} TierPositionHashSetEntry;

typedef struct TierPositionHashSet {
    TierPositionHashSetEntry *entries;
    int64_t capacity;
    int64_t size;
    double max_load_factor;
} TierPositionHashSet;

typedef enum GamesmanTypesLimits {
    /**
     * Largest remoteness expected. Increase this value and recompile if
     * this value is not large enough for a game in the future.
     */
    kRemotenessMax = 1023,
    kDbNameLengthMax = 63,
    kSolverOptionNameLengthMax = 63,
    kSolverNameLengthMax = 63,
    kGameVariantOptionNameMax = 63,
    kGameNameLengthMax = 31,
    kGameFormalNameLengthMax = 127,
} GamesmanTypesLimits;

typedef struct DbProbe {
    Tier tier;
    void *buffer;
    int64_t begin;
    int64_t size;
} DbProbe;

typedef struct Database {
    char name[kDbNameLengthMax + 1];

    int (*Init)(const char *game_name, int variant, const char *path,
                void *aux);
    void (*Finalize)(void);

    // Solving
    int (*CreateSolvingTier)(Tier tier, int64_t size);
    int (*FlushSolvingTier)(void *aux);
    int (*FreeSolvingTier)(void);
    int (*SetValue)(Position position, Value value);
    int (*SetRemoteness)(Position position, int remoteness);
    Value (*GetValue)(Position position);
    int (*GetRemoteness)(Position position);

    // Probing
    int (*ProbeInit)(DbProbe *probe);
    int (*ProbeDestroy)(DbProbe *probe);
    Value (*ProbeValue)(DbProbe *probe, TierPosition tier_position);
    int (*ProbeRemoteness)(DbProbe *probe, TierPosition tier_position);
} Database;

typedef struct SolverOption {
    char name[kSolverOptionNameLengthMax + 1];
    int num_choices;
    const char *const *choices;
} SolverOption;

typedef struct SolverConfiguration {
    const SolverOption *options;  // Zero-terminated.
    const int *selections;        // Aligned with options, Zero-terminated.
} SolverConfiguration;

typedef struct Solver {
    char name[kSolverNameLengthMax + 1];
    const Database *db;

    int (*Init)(const void *solver_api);
    int (*Finalize)(void);

    int (*Solve)(void *aux);
    int (*GetStatus)(void);

    const SolverConfiguration *(*GetCurrentConfiguration)(void);
    int (*SetOption)(int option, int selection);
} Solver;

typedef struct GameVariantOption {
    char name[kGameVariantOptionNameMax + 1];
    int num_choices;
    const char *const *choices;
} GameVariantOption;

typedef struct GameVariant {
    const GameVariantOption *options;  // Zero-terminated.
    const int *selections;             // Aligned with options, Zero-terminated.
} GameVariant;

typedef struct GameplayApi {
    Tier default_initial_tier;
    Position default_initial_position;

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
} GameplayApi;

typedef struct Game {
    char name[kGameNameLengthMax + 1];
    char formal_name[kGameFormalNameLengthMax + 1];
    const Solver *solver;
    const void *solver_api;
    const GameplayApi *gameplay_api;
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
