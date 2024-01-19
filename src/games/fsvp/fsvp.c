#include "games/fsvp/fsvp.h"

#include <assert.h>    // assert
#include <ctype.h>     // isdigit
#include <inttypes.h>  // PRId64
#include <stdbool.h>   // bool, true
#include <stddef.h>    // NULL
#include <stdint.h>    // int64_t
#include <stdio.h>     // sprintf
#include <stdlib.h>    // atoi

#include "core/solvers/regular_solver/regular_solver.h"
#include "core/types/gamesman_types.h"

static int FsvpInit(void *aux);
static int FsvpFinalize(void);

static const GameVariant *FsvpGetCurrentVariant(void);
static int FsvpSetVariantOption(int option, int selection);

static int64_t FsvpGetNumPositions(void);
static Position FsvpGetInitialPosition(void);

static MoveArray FsvpGenerateMoves(Position position);
static Value FsvpPrimitive(Position position);
static Position FsvpDoMove(Position position, Move move);
static bool FsvpIsLegalPosition(Position position);

static int FsvpPositionToString(Position position, char *buffer);
static int FsvpMoveToString(Move move, char *buffer);
static bool FsvpIsValidMoveString(ReadOnlyString move_string);
static Move FsvpStringToMove(ReadOnlyString move_string);

// Variants

#define VARIANT_SIZE_MAX 100
static ConstantReadOnlyString kFsvpGameSizeChoices[] = {
    "4",  "5",  "6",  "7",  "8",  "9",  "10", "11", "12",
    "20", "50", "60", "70", "80", "90", "100"
};

static const GameVariantOption kFsvpGameSize = {
    .name = "size",
    .num_choices =
        sizeof(kFsvpGameSizeChoices) / sizeof(kFsvpGameSizeChoices[0]),
    .choices = kFsvpGameSizeChoices,
};

#define NUM_OPTIONS 2  // 1 option and 1 zero-terminator.
static GameVariantOption options[NUM_OPTIONS];
static int selections[NUM_OPTIONS] = {6, 0};
#undef NUM_OPTIONS
static GameVariant current_variant;

// Solver API Setup

static const RegularSolverApi kFsvpSolverApi = {
    .GetNumPositions = FsvpGetNumPositions,
    .GetInitialPosition = FsvpGetInitialPosition,

    .GenerateMoves = FsvpGenerateMoves,
    .Primitive = FsvpPrimitive,
    .DoMove = FsvpDoMove,
    .IsLegalPosition = FsvpIsLegalPosition,
};

// Gameplay API Setup

static const GameplayApiCommon kFsvpGameplayApiCommon = {
    .GetInitialPosition = FsvpGetInitialPosition,
    .position_string_length_max = 100,

    .move_string_length_max = 7,
    .MoveToString = FsvpMoveToString,

    .IsValidMoveString = FsvpIsValidMoveString,
    .StringToMove = FsvpStringToMove,
};

static const GameplayApiRegular kFsvpGameplayApiRegular = {
    .PositionToString = FsvpPositionToString,

    .GenerateMoves = FsvpGenerateMoves,
    .DoMove = FsvpDoMove,
    .Primitive = FsvpPrimitive,
};

static const GameplayApi kFsvpGameplayApi = {
    .common = &kFsvpGameplayApiCommon,
    .regular = &kFsvpGameplayApiRegular,
};

// -----------------------------------------------------------------------------

const Game kFsvp = {
    .name = "fsvp",
    .formal_name = "Fair Shares and Varied Pairs",
    .solver = &kRegularSolver,
    .solver_api = &kFsvpSolverApi,
    .gameplay_api = &kFsvpGameplayApi,
    .uwapi = NULL,  // TODO

    .Init = FsvpInit,
    .Finalize = FsvpFinalize,

    .GetCurrentVariant = FsvpGetCurrentVariant,
    .SetVariantOption = FsvpSetVariantOption,
};

// -----------------------------------------------------------------------------

// Helper structs and global variables

typedef struct Board {
    // index 0 is unused.
    int counts[VARIANT_SIZE_MAX + 1];
} Board;

static int variant_size;
static int partition[VARIANT_SIZE_MAX + 1][VARIANT_SIZE_MAX + 1];
static int proper_factors[VARIANT_SIZE_MAX + 1][VARIANT_SIZE_MAX + 1];

// Helper functions

static Position Hash(const Board *board);
static Board Unhash(Position hashed);

// API Implementation

static int FsvpInit(void *aux) {
    (void)aux;  // Unused.

    // Initialize variant options.
    options[0] = kFsvpGameSize;
    current_variant.options = options;
    current_variant.selections = selections;
    variant_size = 10;  // Default variant.

    // Fill up the partition number table.
    partition[0][0] = 1;  // Base case
    for (int i = 0; i <= VARIANT_SIZE_MAX; ++i) {
        for (int j = 1; j <= VARIANT_SIZE_MAX; ++j) {
            if (i < j) {
                partition[i][j] = partition[i][i];
            } else {
                partition[i][j] = partition[i][j - 1] + partition[i - j][j];
            }
        }
    }

    // Fill up the proper factors table.
    for (int i = 1; i <= VARIANT_SIZE_MAX; ++i) {
        int num_factors = 0;
        for (int j = 1; j <= i / 2; ++j) {
            if (i % j == 0) proper_factors[i][num_factors++] = j;
        }
    }

    return kNoError;
}

static int FsvpFinalize(void) { return kNoError; }

static const GameVariant *FsvpGetCurrentVariant(void) {
    return &current_variant;
}

static int FsvpSetVariantOption(int option, int selection) {
    if (option != 0 || selection < 0 || selection > VARIANT_SIZE_MAX - 1) {
        return kRuntimeError;
    }

    selections[0] = selection;
    variant_size = atoi(kFsvpGameSizeChoices[selection]);
    printf("setting size to %d\n", variant_size);
    return kNoError;
}

static int64_t FsvpGetNumPositions(void) {
    return partition[variant_size][variant_size];
}

static Position FsvpGetInitialPosition(void) {
    // The partition that contains only the max value is guaranteed
    // to be of index 0 in a reverse lexicographical order.
    if (variant_size % 2 == 0) return 0;
    return 1;
}

// Encoding of a move: the LSB is 0 if it's a splitting move, otherwise it's
// a combining move; splitting moves that "split x into y's" are encoded as
// (variant_size*x+y); combining moves that "combines x and y" are also
// encoded as (variant_size*x+y).
static Move ConstructMove(bool splitting, int x, int y) {
    return (Move)(((x * variant_size + y) << 1) | splitting);
}

static MoveArray FsvpGenerateMoves(Position position) {
    Board board = Unhash(position);
    MoveArray moves;
    MoveArrayInit(&moves);

    // Splitting moves
    for (int i = 1; i <= variant_size; ++i) {
        if (board.counts[i] == 0) continue;
        int j = 0;
        while (proper_factors[i][j] != 0) {
            Move move = ConstructMove(true, i, proper_factors[i][j]);
            MoveArrayAppend(&moves, move);
            ++j;
        }
    }

    // Combining moves
    int available[VARIANT_SIZE_MAX], size = 0;
    for (int i = variant_size; i > 0; --i) {
        if (board.counts[i] > 0) {  // Collect all available counts.
            available[size++] = i;
        }
    }

    for (int i = 0; i < size; ++i) {
        // two numbers must not equal to each other.
        for (int j = i + 1; j < size; ++j) {
            Move move = ConstructMove(false, available[i], available[j]);
            MoveArrayAppend(&moves, move);
        }
    }

    return moves;
}
#undef VARIANT_SIZE_MAX

static Value FsvpPrimitive(Position position) {
    // The partition that contains all ones is guaranteed to be of index
    // (NumPositions - 1) in a reverse lexicographical order.
    if (position == FsvpGetNumPositions() - 1) return kLose;
    return kUndecided;
}

static Position FsvpDoMove(Position position, Move move) {
    Board board = Unhash(position);
    bool splitting = move & 1;
    move >>= 1;
    int x = move / variant_size, y = move % variant_size;
    if (splitting) {
        assert(board.counts[x] >= 1 && x % y == 0 && x != y);
        --board.counts[x];
        board.counts[y] += x / y;
    } else {  // combining
        assert(board.counts[x] >= 1 && board.counts[y] >= 1);
        --board.counts[x];
        --board.counts[y];
        ++board.counts[x + y];
    }

    return Hash(&board);
}

static bool FsvpIsLegalPosition(Position position) {
    // The hash is 100% efficient.
    (void)position;  // Unused.
    return true;
}

static int FsvpPositionToString(Position position, char *buffer) {
    // Format: "{ a, b, c, ..., z, }"
    Board board = Unhash(position);
    int size = 0;
    size += sprintf(buffer, "{ ");
    for (int i = variant_size; i > 0; --i) {
        for (int count = 0; count < board.counts[i]; ++count) {
            size += sprintf(buffer + size, "%d, ", i);
        }
    }
    size += sprintf(buffer + size, "}");

    return kNoError;
}

static int FsvpMoveToString(Move move, char *buffer) {
    bool splitting = move & 1;
    int size = 0;
    if (splitting) {
        size += sprintf(buffer + size, "s ");
    } else {  // combining
        size += sprintf(buffer + size, "c ");
    }
    move >>= 1;
    size += sprintf(buffer + size, "%" PRId64 " ", move / variant_size);  // x
    size += sprintf(buffer + size, "%" PRId64, move % variant_size);      // y

    return kNoError;
}

static bool FsvpIsValidMoveString(ReadOnlyString move_string) {
    if (move_string[0] != 's' && move_string[0] != 'c') return false;
    int i = 2;

    // Check first number (1 or 2 digits)
    if (!isdigit(move_string[i++])) return false;
    if (isdigit(move_string[i])) ++i;

    // Check for space after first number
    if (move_string[i++] != ' ') return false;

    // Check second number (1 or 2 digits)
    if (!isdigit(move_string[i++])) return false;
    if (isdigit(move_string[i])) ++i;

    // Check if the string ends after the second number
    if (move_string[i] != '\0') return false;

    return true;
}

// Assumes valid move string.
static Move FsvpStringToMove(ReadOnlyString move_string) {
    bool splitting = move_string[0] == 's';
    int i = 2;
    int values[2];
    for (int number = 0; number < 2; ++number) {
        char tmp[3];
        tmp[0] = move_string[i];
        if (isdigit(move_string[i + 1])) {
            tmp[1] = move_string[i + 1];
            tmp[2] = '\0';
            i += 3;
        } else {
            tmp[1] = '\0';
            i += 2;
        }
        values[number] = atoi(tmp);
    }

    return ConstructMove(splitting, values[0], values[1]);
}

// Helper functions

/**
 * @cite Wouter M. (https://mathoverflow.net/users/35587/wouter-m), Computing
 * the lexicographic indices of integer partition, URL (version: 2013-10-18):
 * https://mathoverflow.net/q/145186
 */
static Position Hash(const Board *board) {
    Position ret = 0;
    int sum = 0;
    for (int i = 1; i <= variant_size; ++i) {
        for (int j = 0; j < board->counts[i]; ++j) {
            sum += i;
            ret += partition[sum][i - 1];
        }
    }
    return FsvpGetNumPositions() - ret - 1;
}

static Board Unhash(Position hashed) {
    Board ret = {0};
    int diff = FsvpGetNumPositions() - hashed - 1;
    int sum = variant_size, i;
    while (diff > 0) {
        for (i = 1; i <= variant_size; ++i) {
            if (partition[sum][i] > diff) break;
        }
        diff -= partition[sum][i - 1];
        sum -= i;
        ++ret.counts[i];
    }
    ret.counts[1] += sum;

    return ret;
}
