#include "core/analysis.h"

#include <assert.h>    // assert
#include <inttypes.h>  // PRId64
#include <stdbool.h>   // bool, true, false
#include <stdint.h>    // int64_t
#include <stdio.h>     // printf, sprintf, fprintf, stderr
#include <string.h>    // memset

#include "core/data_structures/int64_array.h"
#include "core/gamesman_types.h"
#include "core/misc.h"

static const TierPosition kIllegalTierPosition = {.tier = -1, .position = -1};

static const int kFirstLineReservedRemotness = -1;
static const int kLastLineReservedRemotness = -2;

static bool Int64ArrayInitSize(Int64Array *array, int64_t size);
static bool TierPositionArrayInitSize(TierPositionArray *array, int64_t size);

static int WidthOf(int64_t n);
static void PrintDashedLine(int column_width, int num_headers);
static int PrintSummaryHeader(int column_width);
static void PrintSummaryLine(const Analysis *analysis, int remoteness,
                             int column_width, bool canonical);

static void CountWin(Analysis *analysis, TierPosition tier_position,
                     int remoteness, bool is_canonical);
static void CountLose(Analysis *analysis, TierPosition tier_position,
                      int remoteness, bool is_canonical);
static void CountTie(Analysis *analysis, TierPosition tier_position,
                     int remoteness, bool is_canonical);
static void CountDraw(Analysis *analysis, TierPosition tier_position,
                      int remoteness, bool is_canonical);

static void AggregatePositions(Analysis *dest, const Analysis *src,
                               int remoteness);
static void AggregateCanonicalPositions(Analysis *dest, const Analysis *src,
                                        int remoteness);

// -----------------------------------------------------------------------------

int AnalysisInit(Analysis *analysis) {
    memset(analysis, 0, sizeof(*analysis));

    analysis->hash_size = -1;  // Unset.

    static const int64_t size = kNumRemotenesses;
    bool success = true;
    success &= Int64ArrayInitSize(&analysis->win_summary, size);
    success &= Int64ArrayInitSize(&analysis->lose_summary, size);
    success &= Int64ArrayInitSize(&analysis->tie_summary, size);
    success &= TierPositionArrayInitSize(&analysis->win_examples, size);
    success &= TierPositionArrayInitSize(&analysis->lose_examples, size);
    success &= TierPositionArrayInitSize(&analysis->tie_examples, size);

    success &= Int64ArrayInitSize(&analysis->canonical_win_summary, size);
    success &= Int64ArrayInitSize(&analysis->canonical_lose_summary, size);
    success &= Int64ArrayInitSize(&analysis->canonical_tie_summary, size);
    success &=
        TierPositionArrayInitSize(&analysis->canonical_win_examples, size);
    success &=
        TierPositionArrayInitSize(&analysis->canonical_lose_examples, size);
    success &=
        TierPositionArrayInitSize(&analysis->canonical_tie_examples, size);
    if (!success) {
        AnalysisDestroy(analysis);
        return 1;
    }

    analysis->draw_example = kIllegalTierPosition;
    analysis->canonical_draw_example = kIllegalTierPosition;

    analysis->position_with_most_moves = kIllegalTierPosition;
    analysis->longest_win_position = kIllegalTierPosition;
    analysis->longest_lose_position = kIllegalTierPosition;
    analysis->longest_tie_position = kIllegalTierPosition;

    analysis->max_num_moves = -1;
    analysis->largest_win_remoteness = -1;
    analysis->largest_lose_remoteness = -1;
    analysis->largest_tie_remoteness = -1;

    return 0;
}

void AnalysisDestroy(Analysis *analysis) {
    Int64ArrayDestroy(&analysis->win_summary);
    Int64ArrayDestroy(&analysis->lose_summary);
    Int64ArrayDestroy(&analysis->tie_summary);
    TierPositionArrayDestroy(&analysis->win_examples);
    TierPositionArrayDestroy(&analysis->lose_examples);
    TierPositionArrayDestroy(&analysis->tie_examples);

    Int64ArrayDestroy(&analysis->canonical_win_summary);
    Int64ArrayDestroy(&analysis->canonical_lose_summary);
    Int64ArrayDestroy(&analysis->canonical_tie_summary);
    TierPositionArrayDestroy(&analysis->canonical_win_examples);
    TierPositionArrayDestroy(&analysis->canonical_lose_examples);
    TierPositionArrayDestroy(&analysis->canonical_tie_examples);

    memset(analysis, 0, sizeof(*analysis));
}

// Discovering

void AnalysisSetHashSize(Analysis *analysis, int64_t hash_size) {
    analysis->hash_size = hash_size;
}

void AnalysisDiscoverMoves(Analysis *analysis, TierPosition tier_position,
                           int num_moves) {
    analysis->move_count += num_moves;
    if (num_moves > analysis->max_num_moves) {
        analysis->max_num_moves = num_moves;
        analysis->position_with_most_moves = tier_position;
    }
}

// Counting

int AnalysisCount(Analysis *analysis, TierPosition tier_position, Value value,
                  int remoteness, bool is_canonical) {
    switch (value) {
        case kWin:
            CountWin(analysis, tier_position, remoteness, is_canonical);
            break;

        case kLose:
            CountLose(analysis, tier_position, remoteness, is_canonical);
            break;

        case kTie:
            CountTie(analysis, tier_position, remoteness, is_canonical);
            break;

        case kDraw:
            CountDraw(analysis, tier_position, remoteness, is_canonical);
            break;

        default:
            fprintf(stderr, "AnalysisCount: unknown value %d\n", value);
            return -1;
    }
    return 0;
}

// Aggregating

void AnalysisAggregate(Analysis *dest, const Analysis *src) {
    dest->hash_size += src->hash_size;
    dest->win_count += src->win_count;
    dest->lose_count += src->lose_count;
    dest->tie_count += src->tie_count;
    dest->draw_count += src->draw_count;
    dest->move_count += src->move_count;
    dest->canonical_win_count += src->canonical_win_count;
    dest->canonical_lose_count += src->canonical_lose_count;
    dest->canonical_tie_count += src->canonical_tie_count;
    dest->canonical_draw_count += src->canonical_draw_count;

    for (int remoteness = 0; remoteness < kRemotenessMax; ++remoteness) {
        AggregatePositions(dest, src, remoteness);
        AggregateCanonicalPositions(dest, src, remoteness);
    }

    if (src->max_num_moves > dest->max_num_moves) {
        dest->max_num_moves = src->max_num_moves;
        dest->position_with_most_moves = src->position_with_most_moves;
    }

    if (src->largest_win_remoteness > dest->largest_win_remoteness) {
        dest->largest_win_remoteness = src->largest_win_remoteness;
        dest->longest_win_position = src->longest_win_position;
    }
    if (src->largest_lose_remoteness > dest->largest_lose_remoteness) {
        dest->largest_lose_remoteness = src->largest_lose_remoteness;
        dest->longest_lose_position = src->longest_lose_position;
    }
    if (src->largest_tie_remoteness > dest->largest_tie_remoteness) {
        dest->largest_tie_remoteness = src->largest_tie_remoteness;
        dest->longest_tie_position = src->longest_tie_position;
    }
}

// Post-Analysis

TierPosition AnalysisGetExamplePosition(const Analysis *analysis, Value value,
                                        int remoteness) {
    switch (value) {
        case kWin:
            return analysis->win_examples.array[remoteness];

        case kLose:
            return analysis->lose_examples.array[remoteness];

        case kTie:
            return analysis->tie_examples.array[remoteness];

        case kDraw:
            return analysis->draw_example;

        default:
            fprintf(stderr, "AnalysisGetExamplePosition: unknown value %d\n",
                    value);
            return kIllegalTierPosition;
    }
}

TierPosition AnalysisGetExampleCanonicalPosition(const Analysis *analysis,
                                                 Value value, int remoteness) {
    switch (value) {
        case kWin:
            return analysis->canonical_win_examples.array[remoteness];

        case kLose:
            return analysis->canonical_lose_examples.array[remoteness];

        case kTie:
            return analysis->canonical_tie_examples.array[remoteness];

        case kDraw:
            return analysis->canonical_draw_example;

        default:
            fprintf(stderr,
                    "AnalysisGetExampleCanonicalPosition: unknown value %d\n",
                    value);
            return kIllegalTierPosition;
    }
}

int64_t AnalysisGetNumReachablePositions(const Analysis *analysis) {
    return analysis->win_count + analysis->lose_count + analysis->tie_count +
           analysis->draw_count;
}

int64_t AnalysisGetNumCanonicalPositions(const Analysis *analysis) {
    return analysis->canonical_win_count + analysis->canonical_lose_count +
           analysis->canonical_tie_count + analysis->canonical_draw_count;
}

int64_t AnalysisGetNumNonCanonicalPositions(const Analysis *analysis) {
    int64_t num_reachable = AnalysisGetNumReachablePositions(analysis);
    int64_t num_canonical = AnalysisGetNumCanonicalPositions(analysis);
    return num_reachable - num_canonical;
}

double AnalysisGetSymmetryFactor(const Analysis *analysis) {
    int64_t num_reachable = AnalysisGetNumReachablePositions(analysis);
    int64_t num_canonical = AnalysisGetNumCanonicalPositions(analysis);
    return (double)num_canonical / (double)num_reachable;
}

double AnalysisGetAverageBranchingFactor(const Analysis *analysis) {
    int64_t num_reachable = AnalysisGetNumReachablePositions(analysis);
    return (double)analysis->move_count / (double)num_reachable;
}

double AnalysisGetHashEfficiency(const Analysis *analysis) {
    int64_t num_reachable = AnalysisGetNumReachablePositions(analysis);
    return (double)num_reachable / (double)analysis->hash_size;
}

int AnalysisGetLargestRemoteness(const Analysis *analysis) {
    int max = analysis->largest_win_remoteness;
    if (analysis->largest_lose_remoteness > max) {
        max = analysis->largest_lose_remoteness;
    }
    if (analysis->largest_tie_remoteness > max) {
        max = analysis->largest_tie_remoteness;
    }

    return max;
}

double AnalysisGetWinRatio(const Analysis *analysis) {
    int64_t num_reachable = AnalysisGetNumReachablePositions(analysis);
    return (double)analysis->win_count / (double)num_reachable;
}

double AnalysisGetLoseRatio(const Analysis *analysis) {
    int64_t num_reachable = AnalysisGetNumReachablePositions(analysis);
    return (double)analysis->lose_count / (double)num_reachable;
}

double AnalysisGetTieRatio(const Analysis *analysis) {
    int64_t num_reachable = AnalysisGetNumReachablePositions(analysis);
    return (double)analysis->tie_count / (double)num_reachable;
}

double AnalysisGetDrawRatio(const Analysis *analysis) {
    int64_t num_reachable = AnalysisGetNumReachablePositions(analysis);
    return (double)analysis->draw_count / (double)num_reachable;
}

double AnalysisGetCanonicalWinRatio(const Analysis *analysis) {
    int64_t num_canonical = AnalysisGetNumCanonicalPositions(analysis);
    return (double)analysis->canonical_win_count / (double)num_canonical;
}

double AnalysisGetCanonicalLoseRatio(const Analysis *analysis) {
    int64_t num_canonical = AnalysisGetNumCanonicalPositions(analysis);
    return (double)analysis->canonical_lose_count / (double)num_canonical;
}

double AnalysisGetCanonicalTieRatio(const Analysis *analysis) {
    int64_t num_canonical = AnalysisGetNumCanonicalPositions(analysis);
    return (double)analysis->canonical_tie_count / (double)num_canonical;
}

double AnalysisGetCanonicalDrawRatio(const Analysis *analysis) {
    int64_t num_canonical = AnalysisGetNumCanonicalPositions(analysis);
    return (double)analysis->canonical_draw_count / (double)num_canonical;
}

static void PrintSummary(const Analysis *analysis, bool canonical) {
    int column_width = WidthOf(analysis->hash_size) + 1;
    if (column_width < kInt32Base10StringLengthMax + 1) {
        column_width = kInt32Base10StringLengthMax + 1;
    }
    int num_headers = PrintSummaryHeader(column_width);
    PrintDashedLine(column_width, num_headers);
    PrintSummaryLine(analysis, kFirstLineReservedRemotness, column_width,
                     canonical);
    for (int remoteness = AnalysisGetLargestRemoteness(analysis);
         remoteness >= 0; --remoteness) {
        PrintSummaryLine(analysis, remoteness, column_width, canonical);
    }
    PrintDashedLine(column_width, num_headers);
    PrintSummaryLine(analysis, kLastLineReservedRemotness, column_width,
                     canonical);
    printf("\n\tTotal positions visited: %" PRId64 "\n", analysis->hash_size);
}

void AnalysisPrintSummary(const Analysis *analysis) {
    PrintSummary(analysis, false);
}

void AnalysisPrintCanonicalSummary(const Analysis *analysis) {
    PrintSummary(analysis, true);
}

// -----------------------------------------------------------------------------

static bool Int64ArrayInitSize(Int64Array *array, int64_t size) {
    Int64ArrayInit(array);
    return Int64ArrayResize(array, size);
}

static bool TierPositionArrayInitSize(TierPositionArray *array, int64_t size) {
    TierPositionArrayInit(array);
    return TierPositionArrayResize(array, size);
}

static int WidthOf(int64_t n) {
    assert(n >= 0);
    int width = 0;
    do {
        ++width;
        n /= 10;
    } while (n != 0);
    return width;
}

static void PrintDashedLine(int column_width, int num_headers) {
    printf("\t");
    for (int i = 0; i < num_headers; ++i) {
        for (int j = 0; j < column_width; ++j) {
            printf("-");
        }
    }
    printf("---\n");
}

static int PrintSummaryHeader(int column_width) {
    static ReadOnlyString headers[] = {
        "Remoteness", "Win", "Lose", "Tie", "Draw", "Total",
    };
    const int num_headers = sizeof(headers) / sizeof(headers[0]);

    printf("\t");
    for (int i = 0; i < num_headers; ++i) {
        int spaces = column_width - strlen(headers[i]);
        for (int j = 0; j < spaces; ++j) {
            printf(" ");
        }
        printf("%s", headers[i]);
    }
    printf("\n");
    return num_headers;
}

static void PrintSummaryLine(const Analysis *analysis, int remoteness,
                             int column_width, bool canonical) {
    bool first_line = (remoteness == kFirstLineReservedRemotness);
    bool last_line = (remoteness == kLastLineReservedRemotness);
    char format[128];
    sprintf(format,
            "\t%%%s%ds%%%d" PRId64 "%%%d" PRId64 "%%%d" PRId64 "%%%d" PRId64
            "%%%d" PRId64 "\n",
            (last_line ? "-" : ""), column_width, column_width, column_width,
            column_width, column_width, column_width);

    if (first_line) {
        int64_t draw_count =
            canonical ? analysis->canonical_draw_count : analysis->draw_count;
        printf(format, "Inf", 0, 0, 0, draw_count, draw_count);
    } else if (last_line) {
        if (canonical) {
            printf(format, "Totals", analysis->canonical_win_count,
                   analysis->canonical_lose_count,
                   analysis->canonical_tie_count,
                   analysis->canonical_draw_count,
                   AnalysisGetNumCanonicalPositions(analysis));
        } else {
            printf(format, "Totals", analysis->win_count, analysis->lose_count,
                   analysis->tie_count, analysis->draw_count,
                   AnalysisGetNumReachablePositions(analysis));
        }
    } else {
        int64_t win_count, lose_count, tie_count;
        if (canonical) {
            win_count = analysis->canonical_win_summary.array[remoteness];
            lose_count = analysis->canonical_lose_summary.array[remoteness];
            tie_count = analysis->canonical_tie_summary.array[remoteness];
        } else {
            win_count = analysis->win_summary.array[remoteness];
            lose_count = analysis->lose_summary.array[remoteness];
            tie_count = analysis->tie_summary.array[remoteness];
        }
        int64_t total = win_count + lose_count + tie_count;
        char remoteness_string[kInt32Base10StringLengthMax + 1];
        sprintf(remoteness_string, "%d", remoteness);
        printf(format, remoteness_string, win_count, lose_count, tie_count, 0,
               total);
    }
}

static void CountWin(Analysis *analysis, TierPosition tier_position,
                     int remoteness, bool is_canonical) {
    ++analysis->win_count;
    analysis->canonical_win_count += is_canonical;
    ++analysis->win_summary.array[remoteness];
    analysis->canonical_win_summary.array[remoteness] += is_canonical;
    if (analysis->win_examples.array[remoteness].tier == -1) {
        analysis->win_examples.array[remoteness] = tier_position;
    }
    if (analysis->canonical_win_examples.array[remoteness].tier == -1 &&
        is_canonical) {
        //
        analysis->canonical_win_examples.array[remoteness] = tier_position;
    }
    if (remoteness > analysis->largest_win_remoteness) {
        analysis->largest_win_remoteness = remoteness;
        analysis->longest_win_position = tier_position;
    }
}

static void CountLose(Analysis *analysis, TierPosition tier_position,
                      int remoteness, bool is_canonical) {
    ++analysis->lose_count;
    analysis->canonical_lose_count += is_canonical;
    ++analysis->lose_summary.array[remoteness];
    analysis->canonical_lose_summary.array[remoteness] += is_canonical;
    if (analysis->lose_examples.array[remoteness].tier == -1) {
        analysis->lose_examples.array[remoteness] = tier_position;
    }
    if (analysis->canonical_lose_examples.array[remoteness].tier == -1 &&
        is_canonical) {
        //
        analysis->canonical_lose_examples.array[remoteness] = tier_position;
    }
    if (remoteness > analysis->largest_lose_remoteness) {
        analysis->largest_lose_remoteness = remoteness;
        analysis->longest_lose_position = tier_position;
    }
}

static void CountTie(Analysis *analysis, TierPosition tier_position,
                     int remoteness, bool is_canonical) {
    ++analysis->tie_count;
    analysis->canonical_tie_count += is_canonical;
    ++analysis->tie_summary.array[remoteness];
    analysis->canonical_tie_summary.array[remoteness] += is_canonical;
    if (analysis->tie_examples.array[remoteness].tier == -1) {
        analysis->tie_examples.array[remoteness] = tier_position;
    }
    if (analysis->canonical_tie_examples.array[remoteness].tier == -1 &&
        is_canonical) {
        //
        analysis->canonical_tie_examples.array[remoteness] = tier_position;
    }
    if (remoteness > analysis->largest_tie_remoteness) {
        analysis->largest_tie_remoteness = remoteness;
        analysis->longest_tie_position = tier_position;
    }
}

static void CountDraw(Analysis *analysis, TierPosition tier_position,
                      int remoteness, bool is_canonical) {
    ++analysis->draw_count;
    analysis->canonical_draw_count += is_canonical;
    if (analysis->draw_example.tier == -1) {
        analysis->draw_example = tier_position;
    }
    if (analysis->canonical_draw_example.tier == -1 && is_canonical) {
        analysis->canonical_draw_example = tier_position;
    }
}

static void AggregatePositions(Analysis *dest, const Analysis *src,
                               int remoteness) {
    dest->win_summary.array[remoteness] += src->win_summary.array[remoteness];
    dest->lose_summary.array[remoteness] += src->lose_summary.array[remoteness];
    dest->tie_summary.array[remoteness] += src->tie_summary.array[remoteness];

    if (dest->win_examples.array[remoteness].tier == -1) {
        dest->win_examples.array[remoteness] =
            src->win_examples.array[remoteness];
    }
    if (dest->lose_examples.array[remoteness].tier == -1) {
        dest->lose_examples.array[remoteness] =
            src->lose_examples.array[remoteness];
    }
    if (dest->tie_examples.array[remoteness].tier == -1) {
        dest->tie_examples.array[remoteness] =
            src->tie_examples.array[remoteness];
    }
    if (dest->draw_example.tier == -1) {
        dest->draw_example = src->draw_example;
    }
}

static void AggregateCanonicalPositions(Analysis *dest, const Analysis *src,
                                        int remoteness) {
    dest->canonical_win_summary.array[remoteness] +=
        src->canonical_win_summary.array[remoteness];
    dest->canonical_lose_summary.array[remoteness] +=
        src->canonical_lose_summary.array[remoteness];
    dest->canonical_tie_summary.array[remoteness] +=
        src->canonical_tie_summary.array[remoteness];

    if (dest->canonical_win_examples.array[remoteness].tier == -1) {
        dest->canonical_win_examples.array[remoteness] =
            src->canonical_win_examples.array[remoteness];
    }
    if (dest->canonical_lose_examples.array[remoteness].tier == -1) {
        dest->canonical_lose_examples.array[remoteness] =
            src->canonical_lose_examples.array[remoteness];
    }
    if (dest->canonical_tie_examples.array[remoteness].tier == -1) {
        dest->canonical_tie_examples.array[remoteness] =
            src->canonical_tie_examples.array[remoteness];
    }
    if (dest->canonical_draw_example.tier == -1) {
        dest->canonical_draw_example = src->canonical_draw_example;
    }
}
