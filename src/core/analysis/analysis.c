#include "core/analysis/analysis.h"

#include <assert.h>    // assert
#include <inttypes.h>  // PRId64
#include <stdbool.h>   // bool, true, false
#include <stdint.h>    // int64_t
#include <stdio.h>     // FILE, sprintf, fprintf, stderr
#include <stdlib.h>    // malloc, free
#include <string.h>    // memset

#include "core/gamesman_types.h"
#include "core/misc.h"

static const TierPosition kIllegalTierPosition = {.tier = -1, .position = -1};
static const int kFirstLineReservedRemotness = -1;
static const int kLastLineReservedRemotness = -2;

static void PrintSummary(FILE *stream, const Analysis *analysis,
                         bool canonical);
static int WidthOf(int64_t n);
static void PrintDashedLine(FILE *stream, int column_width, int num_headers);
static int PrintSummaryHeader(FILE *stream, int column_width);
static void PrintSummaryLine(FILE *stream, const Analysis *analysis,
                             int remoteness, int column_width, bool canonical);

static void CountWin(Analysis *analysis, TierPosition tier_position,
                     int remoteness, bool is_canonical);
static void CountLose(Analysis *analysis, TierPosition tier_position,
                      int remoteness, bool is_canonical);
static void CountTie(Analysis *analysis, TierPosition tier_position,
                     int remoteness, bool is_canonical);
static void CountDraw(Analysis *analysis, TierPosition tier_position,
                      bool is_canonical);

static void AggregatePositions(Analysis *dest, const Analysis *src,
                               int remoteness);
static void AggregateCanonicalPositions(Analysis *dest, const Analysis *src,
                                        int remoteness);

// -----------------------------------------------------------------------------

void AnalysisInit(Analysis *analysis) {
    static const TierPosition kIllegalTierPosition = {.tier = -1,
                                                      .position = -1};
    memset(analysis, 0, sizeof(*analysis));
    analysis->hash_size = -1;  // Unset.

    for (int i = 0; i < kNumRemotenesses; ++i) {
        analysis->win_examples[i] = kIllegalTierPosition;
        analysis->lose_examples[i] = kIllegalTierPosition;
        analysis->tie_examples[i] = kIllegalTierPosition;

        analysis->canonical_win_examples[i] = kIllegalTierPosition;
        analysis->canonical_lose_examples[i] = kIllegalTierPosition;
        analysis->canonical_tie_examples[i] = kIllegalTierPosition;
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
}

void AnalysisDestroy(Analysis *analysis) {
    memset(analysis, 0, sizeof(*analysis));
}

int AnalysisWrite(const Analysis *analysis, int fd) {
    int gzfd = dup(fd);
    gzFile file = GuardedGzdopen(gzfd, "wb");
    int error = GuardedGzwrite(file, analysis, sizeof(*analysis));
    GuardedGzclose(file);
    // No need to close the original fd as it will be closed by the caller.
    return error;
}

int AnalysisRead(Analysis *analysis, int fd) {
    gzFile file = GuardedGzdopen(fd, "rb");
    int error = GuardedGzread(file, analysis, sizeof(*analysis), false);
    // No need to close the file as it will be closed as fd by the caller.
    return error;
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
            CountDraw(analysis, tier_position, is_canonical);
            break;

        default:
            fprintf(stderr, "AnalysisCount: unknown value %d\n", value);
            return -1;
    }
    return 0;
}

// Aggregating

void AnalysisConvertToNoncanonical(Analysis *analysis) {
    analysis->canonical_win_count = 0;
    analysis->canonical_lose_count = 0;
    analysis->canonical_tie_count = 0;
    analysis->canonical_draw_count = 0;
    for (int64_t i = 0; i < kNumRemotenesses; ++i) {
        analysis->canonical_win_summary[i] = 0;
        analysis->canonical_lose_summary[i] = 0;
        analysis->canonical_tie_summary[i] = 0;
        analysis->canonical_win_examples[i] = kIllegalTierPosition;
        analysis->canonical_lose_examples[i] = kIllegalTierPosition;
        analysis->canonical_tie_examples[i] = kIllegalTierPosition;
    }
    analysis->canonical_draw_example = kIllegalTierPosition;
}

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
            return analysis->win_examples[remoteness];

        case kLose:
            return analysis->lose_examples[remoteness];

        case kTie:
            return analysis->tie_examples[remoteness];

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
            return analysis->canonical_win_examples[remoteness];

        case kLose:
            return analysis->canonical_lose_examples[remoteness];

        case kTie:
            return analysis->canonical_tie_examples[remoteness];

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

void AnalysisPrintSummary(FILE *stream, const Analysis *analysis) {
    static const bool kPrintCanonical = false;
    PrintSummary(stream, analysis, kPrintCanonical);
}

void AnalysisPrintCanonicalSummary(FILE *stream, const Analysis *analysis) {
    static const bool kPrintCanonical = true;
    PrintSummary(stream, analysis, kPrintCanonical);
}

void AnalysisPrintEverything(FILE *stream, const Analysis *analysis) {
    fprintf(stream, "Hash size: %" PRId64 "\n", analysis->hash_size);
    fprintf(stream, "Winning positions: %" PRId64 " (%" PRId64 " canonical)\n",
            analysis->win_count, analysis->canonical_win_count);
    fprintf(stream, "Losing positions: %" PRId64 " (%" PRId64 " canonical)\n",
            analysis->lose_count, analysis->canonical_lose_count);
    fprintf(stream, "Tying positions: %" PRId64 " (%" PRId64 " canonical)\n",
            analysis->tie_count, analysis->canonical_tie_count);
    fprintf(stream, "Drawing positions: %" PRId64 " (%" PRId64 " canonical)\n",
            analysis->draw_count, analysis->canonical_draw_count);
    fprintf(stream, "Total moves: %" PRId64 "\n\n", analysis->move_count);
    AnalysisPrintSummary(stream, analysis);
    fprintf(stream, "\n");
    AnalysisPrintCanonicalSummary(stream, analysis);
    fprintf(stream, "\n");
    fprintf(stream,
            "Position %" PRId64 " in tier %" PRId64
            " has the most number of available moves: %d\n",
            analysis->position_with_most_moves.position,
            analysis->position_with_most_moves.tier, analysis->max_num_moves);

    static ConstantReadOnlyString longest_position_format =
        "Longest %s starts from position %" PRId64 " in tier %" PRId64
        ", which is of remoteness %d\n";
    static ConstantReadOnlyString not_available_format =
        "No %s positions were found\n";
    if (analysis->largest_win_remoteness >= 0) {
        fprintf(stream, longest_position_format, "win",
                analysis->longest_win_position.position,
                analysis->longest_win_position.tier,
                analysis->largest_win_remoteness);
    } else {
        fprintf(stream, not_available_format, "winning");
    }

    if (analysis->largest_lose_remoteness >= 0) {
        fprintf(stream, longest_position_format, "lose",
                analysis->longest_lose_position.position,
                analysis->longest_lose_position.tier,
                analysis->largest_lose_remoteness);
    } else {
        fprintf(stream, not_available_format, "losing");
    }

    if (analysis->largest_tie_remoteness >= 0) {
        fprintf(stream, longest_position_format, "tie",
                analysis->longest_tie_position.position,
                analysis->longest_tie_position.tier,
                analysis->largest_tie_remoteness);
    } else {
        fprintf(stream, not_available_format, "tying");
    }
    fprintf(stream, "\n\n\n");
}

// -----------------------------------------------------------------------------

static void PrintSummary(FILE *stream, const Analysis *analysis,
                         bool canonical) {
    int column_width = WidthOf(analysis->hash_size) + 1;
    if (column_width < kInt32Base10StringLengthMax + 1) {
        column_width = kInt32Base10StringLengthMax + 1;
    }
    int num_headers = PrintSummaryHeader(stream, column_width);
    PrintDashedLine(stream, column_width, num_headers);
    PrintSummaryLine(stream, analysis, kFirstLineReservedRemotness,
                     column_width, canonical);
    for (int remoteness = AnalysisGetLargestRemoteness(analysis);
         remoteness >= 0; --remoteness) {
        PrintSummaryLine(stream, analysis, remoteness, column_width, canonical);
    }
    PrintDashedLine(stream, column_width, num_headers);
    PrintSummaryLine(stream, analysis, kLastLineReservedRemotness, column_width,
                     canonical);
    if (!canonical) {
        fprintf(stream, "\n\tHash space: %" PRId64 " | Hash efficiency: %f\n",
                analysis->hash_size, AnalysisGetHashEfficiency(analysis));
    }
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

static void PrintDashedLine(FILE *stream, int column_width, int num_headers) {
    fprintf(stream, "\t");
    for (int i = 0; i < num_headers; ++i) {
        for (int j = 0; j < column_width; ++j) {
            fprintf(stream, "-");
        }
    }
    fprintf(stream, "---\n");
}

static int PrintSummaryHeader(FILE *stream, int column_width) {
    static ReadOnlyString headers[] = {
        "Remoteness", "Win", "Lose", "Tie", "Draw", "Total",
    };
    const int num_headers = sizeof(headers) / sizeof(headers[0]);

    fprintf(stream, "\t");
    for (int i = 0; i < num_headers; ++i) {
        int spaces = column_width - strlen(headers[i]);
        for (int j = 0; j < spaces; ++j) {
            fprintf(stream, " ");
        }
        fprintf(stream, "%s", headers[i]);
    }
    fprintf(stream, "\n");
    return num_headers;
}

static void PrintSummaryLine(FILE *stream, const Analysis *analysis,
                             int remoteness, int column_width, bool canonical) {
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
        fprintf(stream, format, "Inf", 0, 0, 0, draw_count, draw_count);
    } else if (last_line) {
        if (canonical) {
            fprintf(stream, format, "Totals", analysis->canonical_win_count,
                    analysis->canonical_lose_count,
                    analysis->canonical_tie_count,
                    analysis->canonical_draw_count,
                    AnalysisGetNumCanonicalPositions(analysis));
        } else {
            fprintf(stream, format, "Totals", analysis->win_count,
                    analysis->lose_count, analysis->tie_count,
                    analysis->draw_count,
                    AnalysisGetNumReachablePositions(analysis));
        }
    } else {
        int64_t win_count, lose_count, tie_count;
        if (canonical) {
            win_count = analysis->canonical_win_summary[remoteness];
            lose_count = analysis->canonical_lose_summary[remoteness];
            tie_count = analysis->canonical_tie_summary[remoteness];
        } else {
            win_count = analysis->win_summary[remoteness];
            lose_count = analysis->lose_summary[remoteness];
            tie_count = analysis->tie_summary[remoteness];
        }
        int64_t total = win_count + lose_count + tie_count;
        char remoteness_string[kInt32Base10StringLengthMax + 1];
        sprintf(remoteness_string, "%d", remoteness);
        fprintf(stream, format, remoteness_string, win_count, lose_count,
                tie_count, 0, total);
    }
}

static void CountWin(Analysis *analysis, TierPosition tier_position,
                     int remoteness, bool is_canonical) {
    ++analysis->win_count;
    analysis->canonical_win_count += is_canonical;
    ++analysis->win_summary[remoteness];
    analysis->canonical_win_summary[remoteness] += is_canonical;
    if (analysis->win_examples[remoteness].tier == -1) {
        analysis->win_examples[remoteness] = tier_position;
    }
    if (analysis->canonical_win_examples[remoteness].tier == -1 &&
        is_canonical) {
        //
        analysis->canonical_win_examples[remoteness] = tier_position;
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
    ++analysis->lose_summary[remoteness];
    analysis->canonical_lose_summary[remoteness] += is_canonical;
    if (analysis->lose_examples[remoteness].tier == -1) {
        analysis->lose_examples[remoteness] = tier_position;
    }
    if (analysis->canonical_lose_examples[remoteness].tier == -1 &&
        is_canonical) {
        //
        analysis->canonical_lose_examples[remoteness] = tier_position;
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
    ++analysis->tie_summary[remoteness];
    analysis->canonical_tie_summary[remoteness] += is_canonical;
    if (analysis->tie_examples[remoteness].tier == -1) {
        analysis->tie_examples[remoteness] = tier_position;
    }
    if (analysis->canonical_tie_examples[remoteness].tier == -1 &&
        is_canonical) {
        //
        analysis->canonical_tie_examples[remoteness] = tier_position;
    }
    if (remoteness > analysis->largest_tie_remoteness) {
        analysis->largest_tie_remoteness = remoteness;
        analysis->longest_tie_position = tier_position;
    }
}

static void CountDraw(Analysis *analysis, TierPosition tier_position,
                      bool is_canonical) {
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
    dest->win_summary[remoteness] += src->win_summary[remoteness];
    dest->lose_summary[remoteness] += src->lose_summary[remoteness];
    dest->tie_summary[remoteness] += src->tie_summary[remoteness];

    if (dest->win_examples[remoteness].tier == -1) {
        dest->win_examples[remoteness] = src->win_examples[remoteness];
    }
    if (dest->lose_examples[remoteness].tier == -1) {
        dest->lose_examples[remoteness] = src->lose_examples[remoteness];
    }
    if (dest->tie_examples[remoteness].tier == -1) {
        dest->tie_examples[remoteness] = src->tie_examples[remoteness];
    }
    if (dest->draw_example.tier == -1) {
        dest->draw_example = src->draw_example;
    }
}

static void AggregateCanonicalPositions(Analysis *dest, const Analysis *src,
                                        int remoteness) {
    dest->canonical_win_summary[remoteness] +=
        src->canonical_win_summary[remoteness];
    dest->canonical_lose_summary[remoteness] +=
        src->canonical_lose_summary[remoteness];
    dest->canonical_tie_summary[remoteness] +=
        src->canonical_tie_summary[remoteness];

    if (dest->canonical_win_examples[remoteness].tier == -1) {
        dest->canonical_win_examples[remoteness] =
            src->canonical_win_examples[remoteness];
    }
    if (dest->canonical_lose_examples[remoteness].tier == -1) {
        dest->canonical_lose_examples[remoteness] =
            src->canonical_lose_examples[remoteness];
    }
    if (dest->canonical_tie_examples[remoteness].tier == -1) {
        dest->canonical_tie_examples[remoteness] =
            src->canonical_tie_examples[remoteness];
    }
    if (dest->canonical_draw_example.tier == -1) {
        dest->canonical_draw_example = src->canonical_draw_example;
    }
}
