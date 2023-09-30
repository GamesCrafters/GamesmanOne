#ifndef GAMESMANEXPERIMENT_CORE_ANALYSIS_H_
#define GAMESMANEXPERIMENT_CORE_ANALYSIS_H_

#include <stdint.h>  // int64_t

#include "core/data_structures/int64_array.h"
#include "core/gamesman_types.h"

typedef struct Analysis {
    int64_t tier_size;     /**< Number of hash values defined. */
    int64_t win_count;     /**< Number of winning positions in total. */
    int64_t lose_count;    /**< Number of losing positions in total. */
    int64_t tie_count;     /**< Number of tying positions in total. */
    int64_t draw_count;    /**< Number of drawing positions in total. */
    int64_t move_count;    /**< Number of moves in total. */
    int64_t num_canonical; /**< Number of canonical positions in total. */

    /** Number of winning positions of each remoteness as an array. */
    Int64Array win_summary;
    /** Number of losing positions of each remoteness as an array. */
    Int64Array lose_summary;
    /** Number of tying positions of each remoteness as an array. */
    Int64Array tie_summary;
    /** Example winning positions of each remoteness as an array. */
    TierPositionArray win_examples;
    /** Example losing positions of each remoteness as an array. */
    TierPositionArray lose_examples;
    /** Example tying positions of each remoteness as an array. */
    TierPositionArray tie_examples;

    /** An example position that has the most moves. */
    TierPosition position_with_most_moves;
    /** An example winning position that has the most losing moves. */
    TierPosition win_position_with_most_losing_moves;
    /** An example tying position that has the most losing moves. */
    TierPosition tie_position_with_most_losing_moves;

    /** An example winning position that has the largest remoteness. */
    TierPosition longest_win_position;
    /** An example losing position that has the largest remoteness. */
    TierPosition longest_lose_position;
    /** An example tying position that has the largest remoteness. */
    TierPosition longest_tie_position;
    
    int largest_win_remoteness;     /**< Largest winning remoteness. */
    int largest_lose_remoteness;    /**< Largest losing remoteness. */
    int largest_tie_remoteness;     /**< Largest tying remoteness. */
} Analysis;

void AnalysisInit(Analysis *analysis);
void AnalysisDestroy(Analysis *analysis);

double AnalysisGetHashEfficiency(const Analysis *analysis);

void AnalysisPrintSummary(const Analysis *analysis);

#endif  // GAMESMANEXPERIMENT_CORE_ANALYSIS_H_
