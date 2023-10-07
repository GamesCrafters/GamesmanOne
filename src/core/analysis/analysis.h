#ifndef GAMESMANEXPERIMENT_CORE_ANALYSIS_ANALYSIS_H_
#define GAMESMANEXPERIMENT_CORE_ANALYSIS_ANALYSIS_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t
#include <stdio.h>    // FILE

#include "core/gamesman_types.h"

typedef struct Analysis {
    int64_t hash_size;  /**< Number of hash values defined. */
    int64_t win_count;  /**< Number of winning positions in total. */
    int64_t lose_count; /**< Number of losing positions in total. */
    int64_t tie_count;  /**< Number of tying positions in total. */
    int64_t draw_count; /**< Number of drawing positions in total. */
    int64_t move_count; /**< Number of moves in total. */

    /** Number of canonical winning positions in total. */
    int64_t canonical_win_count;
    /** Number of canonical losing positions in total. */
    int64_t canonical_lose_count;
    /** Number of canonical tying positions in total. */
    int64_t canonical_tie_count;
    /** Number of canonical drawing positions in total. */
    int64_t canonical_draw_count;

    /** Number of winning positions of each remoteness as an array. */
    int64_t win_summary[kNumRemotenesses];
    /** Number of losing positions of each remoteness as an array. */
    int64_t lose_summary[kNumRemotenesses];
    /** Number of tying positions of each remoteness as an array. */
    int64_t tie_summary[kNumRemotenesses];
    /** Example winning positions of each remoteness as an array. */
    TierPosition win_examples[kNumRemotenesses];
    /** Example losing positions of each remoteness as an array. */
    TierPosition lose_examples[kNumRemotenesses];
    /** Example tying positions of each remoteness as an array. */
    TierPosition tie_examples[kNumRemotenesses];
    /** An example drawing position. */
    TierPosition draw_example;

    /** Number of canonical winning positions of each remoteness as an array. */
    int64_t canonical_win_summary[kNumRemotenesses];
    /** Number of canonical losing positions of each remoteness as an array. */
    int64_t canonical_lose_summary[kNumRemotenesses];
    /** Number of canonical tying positions of each remoteness as an array. */
    int64_t canonical_tie_summary[kNumRemotenesses];
    /** Example canonical winning positions of each remoteness as an array. */
    TierPosition canonical_win_examples[kNumRemotenesses];
    /** Example canonical losing positions of each remoteness as an array. */
    TierPosition canonical_lose_examples[kNumRemotenesses];
    /** Example canonical tying positions of each remoteness as an array. */
    TierPosition canonical_tie_examples[kNumRemotenesses];
    /** An example canonical drawing position. */
    TierPosition canonical_draw_example;

    /** An example position that has the most moves. */
    TierPosition position_with_most_moves;
    /** An example winning position that has the largest remoteness. */
    TierPosition longest_win_position;
    /** An example losing position that has the largest remoteness. */
    TierPosition longest_lose_position;
    /** An example tying position that has the largest remoteness. */
    TierPosition longest_tie_position;

    int max_num_moves;           /**< Max number of moves of any position. */
    int largest_win_remoteness;  /**< Largest winning remoteness. */
    int largest_lose_remoteness; /**< Largest losing remoteness. */
    int largest_tie_remoteness;  /**< Largest tying remoteness. */
} Analysis;

void AnalysisInit(Analysis *analysis);
void AnalysisDestroy(Analysis *analysis);

int AnalysisWrite(const Analysis *analysis, int fd);
int AnalysisRead(Analysis *analysis, int fd);

// Discovering

void AnalysisSetHashSize(Analysis *analysis, int64_t hash_size);
void AnalysisDiscoverMoves(Analysis *analysis, TierPosition tier_position,
                           int num_moves);

// Counting

int AnalysisCount(Analysis *analysis, TierPosition tier_position, Value value,
                  int remoteness, bool is_canonical);

// Aggregating

void AnalysisConvertToNoncanonical(Analysis *analysis);
void AnalysisAggregate(Analysis *dest, const Analysis *src);

// Post-Analysis

TierPosition AnalysisGetExamplePosition(const Analysis *analysis, Value value,
                                        int remoteness);
TierPosition AnalysisGetExampleCanonicalPosition(const Analysis *analysis,
                                                 Value value, int remoteness);
int64_t AnalysisGetNumReachablePositions(const Analysis *analysis);
int64_t AnalysisGetNumCanonicalPositions(const Analysis *analysis);
int64_t AnalysisGetNumNonCanonicalPositions(const Analysis *analysis);
double AnalysisGetSymmetryFactor(const Analysis *analysis);
double AnalysisGetAverageBranchingFactor(const Analysis *analysis);
double AnalysisGetHashEfficiency(const Analysis *analysis);
int AnalysisGetLargestRemoteness(const Analysis *analysis);

double AnalysisGetWinRatio(const Analysis *analysis);
double AnalysisGetLoseRatio(const Analysis *analysis);
double AnalysisGetTieRatio(const Analysis *analysis);
double AnalysisGetDrawRatio(const Analysis *analysis);

double AnalysisGetCanonicalWinRatio(const Analysis *analysis);
double AnalysisGetCanonicalLoseRatio(const Analysis *analysis);
double AnalysisGetCanonicalTieRatio(const Analysis *analysis);
double AnalysisGetCanonicalDrawRatio(const Analysis *analysis);

void AnalysisPrintSummary(FILE *stream, const Analysis *analysis);
void AnalysisPrintCanonicalSummary(FILE *stream, const Analysis *analysis);
void AnalysisPrintStatistics(FILE *stream, const Analysis *analysis);
void AnalysisPrintPositionWithMostMoves(FILE *stream, const Analysis *analysis);
void AnalysisPrintEverything(FILE *stream, const Analysis *analysis);

#endif  // GAMESMANEXPERIMENT_CORE_ANALYSIS_ANALYSIS_H_
