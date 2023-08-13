#include "core/db/naivedb/naivedb.h"

// #include <assert.h>    // assert
// #include <inttypes.h>  // PRId64
// #include <malloc.h>    // malloc, calloc, free
#include <stddef.h>    // NULL
// #include <stdint.h>    // int64_t
#include <stdio.h>     // fprintf, stderr

#include "core/analysis.h"
// #include "core/data_structures/int64_array.h"
// #include "core/gamesman.h"  // tier_solver
#include "core/gamesman_types.h"
// #include "core/misc.h"

#ifdef _OPENMP
#include <omp.h>
#define PRAGMA(X) _Pragma(#X)
#define PRAGMA_OMP_ATOMIC PRAGMA(omp atomic)
#define PRAGMA_OMP_CRITICAL(name) PRAGMA(omp critical(name))
#else
#define PRAGMA
#define PRAGMA_OMP_ATOMIC
#define PRAGMA_OMP_CRITICAL(name)
#endif

static int NaiveDbInit(const char *game_name, int variant, void *aux);
static void NaiveDbFinalize(void);

static int NaiveDbCreateSolvingTier(Tier tier, int64_t size);
static int NaiveDbFlushSolvingTier(void *aux);
static int NaiveDbFreeSolvingTier(void);

static int NaiveDbSetValue(TierPosition tier_position, Value value);
static int NaiveDbSetRemoteness(TierPosition tier_position, int remoteness);
static Value NaiveDbGetValue(TierPosition tier_position);
static int NaiveDbGetRemoteness(TierPosition tier_position);

const Database kNaiveDb = {
    .name = "Naive DB",

    .Init = &NaiveDbInit,
    .Finalize = &NaiveDbFinalize,

    // Solving
    .CreateSolvingTier = &NaiveDbCreateSolvingTier,
    .FlushSolvingTier = &NaiveDbFlushSolvingTier,
    .FreeSolvingTier = &NaiveDbFreeSolvingTier,
    .SetValue = &NaiveDbSetValue,
    .SetRemoteness = &NaiveDbSetRemoteness,

    // Probing
    .GetValue = &NaiveDbGetValue,
    .GetRemoteness = &NaiveDbGetRemoteness
};

// File name = <prefix>/[<extra>/...]/<tier>

typedef struct NaiveDbEntry {
    Value value;
    int remoteness;
} NaiveDbEntry;

static Tier current_tier = -1;
static NaiveDbEntry *records = NULL;
static Analysis tier_analysis;

static int NaiveDbCreateSolvingTier(Tier tier, int64_t size) {
    current_tier = tier;

    AnalysisInit(&tier_analysis);
    tier_analysis.total_positions = size;

    free(records);
    records = (NaiveDbEntry *)calloc(size, sizeof(NaiveDbEntry));
    if (records == NULL) {
        fprintf(stderr, "NaiveDbCreateSolvingTier: failed to calloc records.\n");
        return false;
    }
    return true;
}

bool DbLoadTier(Tier tier) {
    int64_t size = tier_solver.GetTierSize(tier);
    free(records);
    records = (NaiveDbEntry *)malloc(size * sizeof(NaiveDbEntry));
    if (records == NULL) {
        fprintf(stderr, "DbCreateTier: failed to malloc records.\n");
        return false;
    }
    char filename[100];
    sprintf(filename, "%" PRId64, tier);
    FILE *dbfile = fopen(filename, "rb");
    assert(dbfile);
    int64_t n = fread(records, sizeof(NaiveDbEntry), size, dbfile);
    assert(n == size);
    (void)n;
    fclose(dbfile);
    return true;
}

void DbSave(Tier tier) {
    int64_t size = tier_solver.GetTierSize(tier);
    char filename[100];
    sprintf(filename, "%" PRId64, tier);
    FILE *dbfile = fopen(filename, "wb");
    int64_t n = fwrite(records, sizeof(NaiveDbEntry), size, dbfile);
    assert(n == size);
    (void)n;
    fclose(dbfile);
}

Value DbGetValue(Position position) {
    assert(records);
    return records[position].value;
}

int DbGetRemoteness(Position position) {
    assert(records);
    return records[position].remoteness;
}

static void UpdateCountAndSummary(int remoteness, int64_t *count,
                                  Int64Array *summary) {
    PRAGMA_OMP_ATOMIC
    ++(*count);

    // Fill summary array with zero entries up to remoteness.
    PRAGMA_OMP_CRITICAL(UpdateCountAndSummary) {
        while (summary->size <= remoteness) {
            Int64ArrayPushBack(summary, 0);
        }
    }

    PRAGMA_OMP_ATOMIC
    ++summary->array[remoteness];
}

static void UpdateTierAnalysis(Position position, Value value, int remoteness) {
    PRAGMA_OMP_ATOMIC
    ++tier_analysis.total_legal_positions;

    PRAGMA_OMP_CRITICAL(UpdateTierAnalysis) {
        if (remoteness > tier_analysis.largest_found_remoteness) {
            tier_analysis.largest_found_remoteness = remoteness;
            tier_analysis.largest_remoteness_position =
                (TierPosition){current_tier, position};
        }
    }
    switch (value) {
        case kWin:
            UpdateCountAndSummary(remoteness, &tier_analysis.win_count,
                                  &tier_analysis.win_summary);
            break;

        case kLose:
            UpdateCountAndSummary(remoteness, &tier_analysis.lose_count,
                                  &tier_analysis.lose_summary);
            break;

        case kTie:
            UpdateCountAndSummary(remoteness, &tier_analysis.tie_count,
                                  &tier_analysis.tie_summary);
            break;

        case kDraw:
            PRAGMA_OMP_ATOMIC
            ++tier_analysis.draw_count;
            break;

        default:
            NotReached("DbSetValueRemoteness: unknown value.\n");
    }
}

void DbSetValueRemoteness(Position position, Value value, int remoteness) {
    records[position].value = value;
    records[position].remoteness = remoteness;
    UpdateTierAnalysis(position, value, remoteness);
}

static void DumpSummaryToGlobal(Int64Array *global_summary,
                                Int64Array *tier_summary) {
    while (global_summary->size < tier_summary->size) {
        Int64ArrayPushBack(global_summary, 0);
    }
    for (int64_t remoteness = 0; remoteness < tier_summary->size;
         ++remoteness) {
        global_summary->array[remoteness] += tier_summary->array[remoteness];
    }
}

void DbDumpTierAnalysisToGlobal(void) {
    assert(tier_analysis.total_positions != 0);
    global_analysis.total_positions += tier_analysis.total_positions;
    global_analysis.total_legal_positions +=
        tier_analysis.total_legal_positions;
    global_analysis.win_count += tier_analysis.win_count;
    global_analysis.lose_count += tier_analysis.lose_count;
    global_analysis.tie_count += tier_analysis.tie_count;
    global_analysis.draw_count += tier_analysis.draw_count;
    if (tier_analysis.largest_found_remoteness >
        global_analysis.largest_found_remoteness) {
        global_analysis.largest_found_remoteness =
            tier_analysis.largest_found_remoteness;
        global_analysis.largest_remoteness_position =
            tier_analysis.largest_remoteness_position;
    }
    DumpSummaryToGlobal(&global_analysis.win_summary,
                        &tier_analysis.win_summary);
    DumpSummaryToGlobal(&global_analysis.lose_summary,
                        &tier_analysis.lose_summary);
    DumpSummaryToGlobal(&global_analysis.tie_summary,
                        &tier_analysis.tie_summary);
}

#undef PRAGMA
#undef PRAGMA_OMP_ATOMIC
#undef PRAGMA_OMP_CRITICAL
