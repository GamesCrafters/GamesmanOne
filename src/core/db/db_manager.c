/**
 * @file db_manager.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Database manager module implementation.
 * @version 1.0
 * @date 2023-08-19
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

#include <inttypes.h>  // PRId64
#include <stdbool.h>   // bool, true, false
#include <stddef.h>    // NULL
#include <stdio.h>     // fprintf, stderr
#include <stdlib.h>    // exit, EXIT_FAILURE
#include <string.h>    // strlen

#include "core/gamesman_types.h"
#include "core/misc.h"

static const Database *current_db;
static const Database *control_group_db;

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

// Assumes current_db has been set.
static char *SetupDbPath(const Database *db, ReadOnlyString game_name,
                         int variant) {
    // path = "data/<game_name>/<variant>/<db_name>/"
    static ConstantReadOnlyString kDataPath = "data";
    char *path = NULL;

    int path_length = strlen(kDataPath) + 1;  // +1 for '/'.
    path_length += strlen(game_name) + 1;
    path_length += kInt32Base10StringLengthMax + 1;
    path_length += strlen(db->name) + 1;
    path = (char *)calloc((path_length + 1), sizeof(char));
    if (path == NULL) {
        fprintf(stderr, "SetupDbPath: failed to calloc path.\n");
        return NULL;
    }
    int actual_length = snprintf(path, path_length, "%s/%s/%d/%s/", kDataPath,
                                 game_name, variant, db->name);
    if (actual_length >= path_length) {
        fprintf(stderr,
                "SetupDbPath: (BUG) not enough space was allocated for "
                "path. Please check the implementation of this "
                "function.\n");
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

int DbManagerInitDb(const Database *db, ReadOnlyString game_name, int variant,
                    void *aux) {
    if (current_db != NULL) current_db->Finalize();
    current_db = NULL;

    if (!BasicDbApiImplemented(db)) {
        fprintf(stderr,
                "DbManagerInitDb: The %s does not have all the required "
                "functions implemented and cannot be used.\n",
                db->formal_name);
        return -1;
    }
    current_db = db;

    char *path = SetupDbPath(current_db, game_name, variant);
    int error = current_db->Init(game_name, variant, path, aux);
    free(path);

    return error;
}

void DbManagerFinalizeDb(void) {
    current_db->Finalize();
    current_db = NULL;
    if (control_group_db != NULL) {
        control_group_db->Finalize();
        control_group_db = NULL;
    }
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

int DbManagerInitControlGroupDb(const Database *control,
                                ReadOnlyString game_name, int variant,
                                void *aux) {
    if (control_group_db != NULL) control_group_db->Finalize();
    control_group_db = NULL;

    if (!BasicDbApiImplemented(control)) {
        fprintf(stderr,
                "DbManagerInitControlGroupDb: The %s does not have all the "
                "required "
                "functions implemented and cannot be used.\n",
                control->formal_name);
        return -1;
    }
    control_group_db = control;

    char *path = SetupDbPath(control_group_db, game_name, variant);
    return control_group_db->Init(game_name, variant, path, aux);
}

int DbManagerTestTier(Tier tier, int64_t size) {
    DbProbe c_probe, t_probe;
    control_group_db->ProbeInit(&c_probe);
    current_db->ProbeInit(&t_probe);
    int error = 0;

    for (int64_t i = 0; i < size; ++i) {
        TierPosition tier_position = {.tier = tier, .position = i};
        Value c_value = control_group_db->ProbeValue(&c_probe, tier_position);
        Value t_value = current_db->ProbeValue(&t_probe, tier_position);
        if (c_value != t_value) {
            printf("Inconsistent value at position %" PRId64 " in tier %" PRId64
                   ". Control group value: %d, experimental group value: %d.\n",
                   tier, i, c_value, t_value);
            error = 1;
            goto _bailout;
        }

        int c_rmt = control_group_db->ProbeRemoteness(&c_probe, tier_position);
        int t_rmt = current_db->ProbeRemoteness(&t_probe, tier_position);
        if (c_rmt != t_rmt) {
            printf("Inconsistent remoteness at position %" PRId64
                   " in tier %" PRId64
                   ". Control group remoteness: %d, experimental group "
                   "remoteness: %d.\n",
                   tier, i, c_rmt, t_rmt);
            error = 2;
            goto _bailout;
        }
    }

_bailout:
    control_group_db->ProbeDestroy(&c_probe);
    current_db->ProbeDestroy(&t_probe);
    return error;
}
