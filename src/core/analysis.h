#ifndef GAMESMANEXPERIMENT_CORE_ANALYSIS_H_
#define GAMESMANEXPERIMENT_CORE_ANALYSIS_H_
#include <stdint.h>

#include "core/data_structures/int64_array.h"
#include "core/gamesman_types.h"

typedef struct Analysis {
    int64_t total_positions;
    int64_t total_legal_positions;

    int largest_found_remoteness;
    TierPosition largest_remoteness_position;

    int64_t win_count;
    int64_t lose_count;
    int64_t tie_count;
    int64_t draw_count;

    Int64Array win_summary;
    Int64Array lose_summary;
    Int64Array tie_summary;
} Analysis;

void AnalysisInit(Analysis *analysis);
void AnalysisDestroy(Analysis *analysis);

double AnalysisGetHashEfficiency(const Analysis *analysis);

void AnalysisPrintSummary(const Analysis *analysis);

#endif  // GAMESMANEXPERIMENT_CORE_ANALYSIS_H_
