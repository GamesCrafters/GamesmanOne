#include "core/db_manager.h"

#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // exit, EXIT_FAILURE

#include "core/gamesman_types.h"

static Database *current_db;

static bool IsValidDbName(const char *name) {
    bool terminates = false;
    for (int i = 0; i < (int)kDbNameLengthMax + 1; ++i) {
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
    if (!IsValidDbName(db->name)) {
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

int DbManagerInitDb(const Solver *solver) {
    if (current_db != NULL) current_db->Finalize();
    current_db = NULL;

    if (!BasicDbApiImplemented(solver->db)){
        fprintf(stderr,
                "DbManagerInitDb: The %s does not have all the required "
                "functions implemented and cannot be used.\n");
        return -1;
    }
    current_db = solver->db;
    return 0;
}

int DbManagerSetValue(Position position, Value value) {
    return current_db->SetValue(position, value);
}

int DbManagerSetRemoteness(Position position, int remoteness) {
    return current_db->SetRemoteness(position, remoteness);
}

Value DbManagerGetValue(TierPosition tier_position) {
    return current_db->GetValue(tier_position);
}

int DbManagerGetRemoteness(TierPosition tier_position) {
    return current_db->GetRemoteness(tier_position);
}
