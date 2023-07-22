#include "core/analysis.h"

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core/data_structures/int64_array.h"
#include "core/gamesman_types.h"

void AnalysisInit(Analysis *analysis) {
    memset(analysis, 0, sizeof(*analysis));
    Int64ArrayInit(&analysis->win_summary);
    Int64ArrayInit(&analysis->lose_summary);
    Int64ArrayInit(&analysis->tie_summary);
}

void AnalysisDestroy(Analysis *analysis) {
    Int64ArrayDestroy(&analysis->win_summary);
    Int64ArrayDestroy(&analysis->lose_summary);
    Int64ArrayDestroy(&analysis->tie_summary);
    memset(analysis, 0, sizeof(*analysis));
}

double AnalysisGetHashEfficiency(const Analysis *analysis) {
    return (double)analysis->total_legal_positions /
           (double)analysis->total_positions;
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
    const char *headers[] = {"Remoteness", "Win",  "Lose",
                             "Tie",        "Draw", "Total"};
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
                             int column_width) {
    bool first_line = (remoteness == -1);
    bool last_line = (remoteness == -2);
    char format[128];
    sprintf(format,
            "\t%%%s%ds%%%d" PRId64 "%%%d" PRId64 "%%%d" PRId64 "%%%d" PRId64
            "%%%d" PRId64 "\n",
            (last_line ? "-" : ""), column_width, column_width, column_width,
            column_width, column_width, column_width);

    if (first_line) {
        int64_t total_draw = analysis->draw_count;
        printf(format, "Inf", 0, 0, 0, total_draw, total_draw);
    } else if (last_line) {
        printf(format, "Totals", analysis->win_count, analysis->lose_count,
               analysis->tie_count, analysis->draw_count,
               analysis->total_legal_positions);
    } else {
        int64_t win_count = analysis->win_summary.size > remoteness
                                ? analysis->win_summary.array[remoteness]
                                : 0;
        int64_t lose_count = analysis->lose_summary.size > remoteness
                                 ? analysis->lose_summary.array[remoteness]
                                 : 0;
        int64_t tie_count = analysis->tie_summary.size > remoteness
                                ? analysis->tie_summary.array[remoteness]
                                : 0;
        int64_t total = win_count + lose_count + tie_count;
        char remoteness_string[12];  // Covers the range [-2147483647,
                                     // 2147483647].
        sprintf(remoteness_string, "%d", remoteness);
        printf(format, remoteness_string, win_count, lose_count, tie_count, 0,
               total);
    }
}

void AnalysisPrintSummary(const Analysis *analysis) {
    int column_width = WidthOf(analysis->total_legal_positions) + 1;
    if (column_width < 12) column_width = 12;
    int num_headers = PrintSummaryHeader(column_width);
    PrintDashedLine(column_width, num_headers);
    PrintSummaryLine(analysis, -1, column_width);
    for (int remoteness = analysis->largest_found_remoteness; remoteness >= 0;
         --remoteness) {
        PrintSummaryLine(analysis, remoteness, column_width);
    }
    PrintDashedLine(column_width, num_headers);
    PrintSummaryLine(analysis, -2, column_width);
    printf("\n\tTotal positions visited: %" PRId64 "\n",
           analysis->total_legal_positions);
}
