/**
 * @file db_manager.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Database manager module implementation.
 * @version 1.2.1
 * @date 2024-02-15
 *
 * @copyright This file is part of GAMESMAN, The Finite, Two-person
 * Perfect-Information Game Generator released under the GPL:
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "core/db/db_manager.h"

#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // exit, EXIT_FAILURE
#include <string.h>   // strlen

#include "core/constants.h"
#include "core/misc.h"
#include "core/types/gamesman_types.h"

static const Database *current_db;
static const Database *ref_db;

static bool BasicDbApiImplemented(const Database *db);
static bool IsValidDbName(ReadOnlyString name);
static char *SetupDbPath(const Database *db, ReadOnlyString game_name,
                         int variant, ReadOnlyString data_path);

// -----------------------------------------------------------------------------

int DbManagerInitDb(const Database *db, ReadOnlyString game_name, int variant,
                    ReadOnlyString data_path, GetTierNameFunc GetTierName,
                    void *aux) {
    if (current_db != NULL) current_db->Finalize();
    current_db = NULL;

    if (!BasicDbApiImplemented(db)) {
        fprintf(stderr,
                "DbManagerInitDb: The %s does not have all the required "
                "functions implemented and cannot be used.\n",
                db->formal_name);
        return kNotImplementedError;
    }
    current_db = db;

    char *path = SetupDbPath(current_db, game_name, variant, data_path);
    int error = current_db->Init(game_name, variant, path, GetTierName, aux);
    free(path);

    return error;
}

int DbManagerInitRefDb(const Database *db, ReadOnlyString game_name,
                       int variant, ReadOnlyString data_path,
                       GetTierNameFunc GetTierName, void *aux) {
    if (ref_db != NULL) ref_db->Finalize();
    ref_db = NULL;

    if (!BasicDbApiImplemented(db)) {
        fprintf(stderr,
                "DbManagerInitDb: The %s does not have all the required "
                "functions implemented and cannot be used.\n",
                db->formal_name);
        return kNotImplementedError;
    }
    ref_db = db;

    char *path = SetupDbPath(ref_db, game_name, variant, data_path);
    int error = ref_db->Init(game_name, variant, path, GetTierName, aux);
    free(path);

    return error;
}

void DbManagerFinalizeDb(void) {
    if (current_db) current_db->Finalize();
    current_db = NULL;
}

void DbManagerFinalizeRefDb(void) {
    if (ref_db) ref_db->Finalize();
    ref_db = NULL;
}

int DbManagerCreateSolvingTier(Tier tier, int64_t size) {
    return current_db->CreateSolvingTier(tier, size);
}

int DbManagerFlushSolvingTier(void *aux) {
    return current_db->FlushSolvingTier(aux);
}

int DbManagerFreeSolvingTier(void) { return current_db->FreeSolvingTier(); }

int DbManagerSetValue(Position position, Value value) {
    return current_db->SetValue(position, value);
}

int DbManagerSetRemoteness(Position position, int remoteness) {
    return current_db->SetRemoteness(position, remoteness);
}

Value DbManagerGetValue(Position position) {
    return current_db->GetValue(position);
}

int DbManagerGetRemoteness(Position position) {
    return current_db->GetRemoteness(position);
}

int DbManagerProbeInit(DbProbe *probe) { return current_db->ProbeInit(probe); }

int DbManagerProbeDestroy(DbProbe *probe) {
    return current_db->ProbeDestroy(probe);
}

Value DbManagerProbeValue(DbProbe *probe, TierPosition tier_position) {
    return current_db->ProbeValue(probe, tier_position);
}

int DbManagerProbeRemoteness(DbProbe *probe, TierPosition tier_position) {
    return current_db->ProbeRemoteness(probe, tier_position);
}

int DbManagerTierStatus(Tier tier) { return current_db->TierStatus(tier); }

int DbManagerRefProbeInit(DbProbe *probe) { return ref_db->ProbeInit(probe); }

int DbManagerRefProbeDestroy(DbProbe *probe) {
    return ref_db->ProbeDestroy(probe);
}
Value DbManagerRefProbeValue(DbProbe *probe, TierPosition tier_position) {
    return ref_db->ProbeValue(probe, tier_position);
}
int DbManagerRefProbeRemoteness(DbProbe *probe, TierPosition tier_position) {
    return ref_db->ProbeRemoteness(probe, tier_position);
}

// -----------------------------------------------------------------------------

static bool BasicDbApiImplemented(const Database *db) {
    if (!IsValidDbName(db->formal_name)) {
        fprintf(stderr,
                "BasicDbApiImplemented: (BUG) A Database does not have its "
                "name properly initialized. Aborting...\n");
        exit(EXIT_FAILURE);
    }
    if (db->Init == NULL) return false;
    if (db->FlushSolvingTier == NULL) return false;
    if (db->Finalize == NULL) return false;
    if (db->SetValue == NULL) return false;
    if (db->SetRemoteness == NULL) return false;
    if (db->GetValue == NULL) return false;
    if (db->GetRemoteness == NULL) return false;

    return true;
}

static bool IsValidDbName(ReadOnlyString name) {
    bool terminates = false;
    for (int i = 0; i < (int)kDbFormalNameLengthMax + 1; ++i) {
        if (name[i] == '\0') {
            terminates = true;
            break;
        }
    }
    if (!terminates) return false;      // Uninitialized name.
    if (name[0] == '\0') return false;  // Empty name is not allowed.
    return true;
}

static char *SetupDbPath(const Database *db, ReadOnlyString game_name,
                         int variant, ReadOnlyString data_path) {
    // path = "<data_path>/<game_name>/<variant>/<db_name>/"
    if (data_path == NULL) data_path = "data";
    char *path = NULL;

    int path_length = strlen(data_path) + 1;  // +1 for '/'.
    path_length += strlen(game_name) + 1;
    path_length += kInt32Base10StringLengthMax + 1;
    path_length += strlen(db->name) + 1;
    path = (char *)calloc((path_length + 1), sizeof(char));
    if (path == NULL) {
        fprintf(stderr, "SetupDbPath: failed to calloc path.\n");
        return NULL;
    }
    int actual_length = snprintf(path, path_length, "%s/%s/%d/%s/", data_path,
                                 game_name, variant, db->name);
    if (actual_length >= path_length) {
        fprintf(stderr,
                "SetupDbPath: (BUG) not enough space was allocated for path. "
                "Please check the implementation of this function.\n");
        free(path);
        return NULL;
    }
    if (MkdirRecursive(path) != 0) {
        fprintf(stderr,
                "SetupDbPath: failed to create path in the file system.\n");
        free(path);
        return NULL;
    }
    return path;
}
