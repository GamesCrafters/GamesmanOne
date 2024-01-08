#ifndef GAMESMANONE_CORE_TYPES_DATABASE_DB_PROBE_H
#define GAMESMANONE_CORE_TYPES_DATABASE_DB_PROBE_H

#include <stdint.h>  // int64_t

#include "core/types/base.h"

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

#endif  // GAMESMANONE_CORE_TYPES_DATABASE_DB_PROBE_H
