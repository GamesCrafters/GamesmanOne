/**
 * @file db_manager.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Database manager module implementation.
 * @version 2.1.0
 * @date 2025-06-23
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core/constants.h"
#include "core/gamesman_memory.h"
#include "core/types/base.h"
#include "core/types/database/database.h"
#include "core/types/database/db_probe.h"
#include "core/types/gamesman_status.h"
#include "libs/io/xfile.h"

const Database *db_manager_current_db = NULL;

static bool IsValidDbName(ReadOnlyString name) {
    bool terminates = false;
    for (int i = 0; i < kDbFormalNameLengthMax + 1; ++i) {
        if (name[i] == '\0') {
            terminates = true;
            break;
        }
    }
    if (!terminates) {  // Uninitialized name.
        return false;
    }
    if (name[0] == '\0') {  // Empty name is not allowed.
        return false;
    }

    return true;
}

static bool BasicDbApiImplemented(const Database *db) {
    if (!IsValidDbName(db->formal_name)) {
        fprintf(stderr,
                "BasicDbApiImplemented: (BUG) A Database does not have its "
                "name properly initialized. Aborting...\n");
        _exit(kNotImplementedError);
    }
    if (db->Init == NULL) {
        return false;
    }
    if (db->FlushSolvingTier == NULL) {
        return false;
    }
    if (db->Finalize == NULL) {
        return false;
    }
    if (db->SetValue == NULL) {
        return false;
    }
    if (db->SetRemoteness == NULL) {
        return false;
    }
    if (db->GetValue == NULL) {
        return false;
    }
    if (db->GetRemoteness == NULL) {
        return false;
    }

    return true;
}

static char *SetupDbPath(const Database *db, ReadOnlyString game_name,
                         int variant, ReadOnlyString data_path,
                         bool read_only) {
    // path = "<data_path>/<game_name>/<variant>/<db_name>/"
    if (data_path == NULL) {
        data_path = "data";
    }

    char *path = NULL;
    int path_length = (int)strlen(data_path) + 1;  // +1 for '/'.
    path_length += (int)strlen(game_name) + 1;
    path_length += kInt32Base10StringLengthMax + 1;
    path_length += (int)strlen(db->name) + 1;
    path = (char *)GamesmanCallocWhole((path_length + 1), sizeof(char));
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
        GamesmanFree(path);
        return NULL;
    }

    if (!read_only && MkdirRecursive(path) != 0) {
        fprintf(stderr,
                "SetupDbPath: failed to create path in the file system.\n");
        GamesmanFree(path);
        return NULL;
    }

    return path;
}

int DbManagerInitDb(const Database *db, bool read_only,
                    ReadOnlyString game_name, int variant,
                    ReadOnlyString data_path, GetTierNameFunc GetTierName,
                    void *aux) {
    if (db_manager_current_db != NULL) {
        db_manager_current_db->Finalize();
    }
    db_manager_current_db = NULL;

    if (!BasicDbApiImplemented(db)) {
        fprintf(stderr,
                "DbManagerInitDb: The %s does not have all the required "
                "functions implemented and cannot be used.\n",
                db->formal_name);
        return kNotImplementedError;
    }
    db_manager_current_db = db;

    char *path = SetupDbPath(db_manager_current_db, game_name, variant,
                             data_path, read_only);
    if (path == NULL) {
        return kFileSystemError;
    }

    int error =
        db_manager_current_db->Init(game_name, variant, path, GetTierName, aux);
    GamesmanFree(path);

    return error;
}
