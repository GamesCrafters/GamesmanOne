/**
 * @file hquery.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the position querying functionality of headless
 * mode.
 * @version 1.1.1
 * @date 2024-12-22
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

#include "core/headless/hquery.h"

#include <assert.h>              // assert
#include <json-c/json_object.h>  // json_object and related functions
#include <stdbool.h>             // bool, true, false
#include <stdbool.h>             // bool
#include <stdint.h>              // int64_t
#include <stdio.h>               // printf

#include "core/constants.h"
#include "core/game_manager.h"
#include "core/headless/hjson.h"
#include "core/headless/hutils.h"
#include "core/solvers/solver_manager.h"
#include "core/types/gamesman_types.h"

static int InitAndCheckGame(ReadOnlyString game_name, int variant_id,
                            ReadOnlyString data_path, bool *is_tier_game);
static bool ImplementsRegularUwapi(const Game *game);
static bool ImplementsTierUwapi(const Game *game);

static int QueryRegular(const Game *game, ReadOnlyString formal_position);
static int QueryTier(const Game *game, ReadOnlyString formal_position);

static int GetStartRegular(const Game *game);
static int GetStartTier(const Game *game);

static int GetRandomRegular(const Game *game);
static int GetRandomTier(const Game *game);

static int JsonPrintPositionResponse(const Game *game, Position position);
static json_object *JsonCreateBasicPositionObject(const Game *game,
                                                  Position position);
static json_object *JsonCreateChildPositionObject(const Game *game,
                                                  Position parent, Move move);
static json_object *JsonCreateParentPositionObject(
    const Game *game, Position position, json_object *moves_array_obj,
    json_object *partmoves_array_obj);

static int JsonPrintTierPositionResponse(const Game *game,
                                         TierPosition tier_position);
static MoveArray GetMovesFromTierPosition(const Game *game,
                                          TierPosition tier_position);
static PartmoveArray GetPartmovesFromTierPosition(const Game *game,
                                                  TierPosition tier_position);
static json_object *JsonCreateBasicTierPositionObject(
    const Game *game, TierPosition tier_position);
static json_object *JsonCreateChildTierPositionObject(const Game *game,
                                                      TierPosition parent,
                                                      Move move);
static json_object *JsonCreateParentTierPositionObject(
    const Game *game, TierPosition tier_position, json_object *moves_array_obj,
    json_object *partmoves_array_obj);
static json_object *JsonCreatePartmoveEdgeObject(const Partmove *pm);

static void JsonPrintStartResponse(ReadOnlyString start,
                                   ReadOnlyString autogui_start);
static void JsonPrintRandomResponse(ReadOnlyString random,
                                    ReadOnlyString autogui_random);
static void JsonPrintSinglePosition(ReadOnlyString position,
                                    ReadOnlyString autogui_position);

// -----------------------------------------------------------------------------

int HeadlessQuery(ReadOnlyString game_name, int variant_id,
                  ReadOnlyString data_path, ReadOnlyString formal_position) {
    bool is_tier_game;
    int error =
        InitAndCheckGame(game_name, variant_id, data_path, &is_tier_game);
    if (error != 0) return error;

    const Game *game = GameManagerGetCurrentGame();
    if (is_tier_game) return QueryTier(game, formal_position);
    return QueryRegular(game, formal_position);
}

int HeadlessGetStart(ReadOnlyString game_name, int variant_id) {
    bool is_tier_game;
    int error = InitAndCheckGame(game_name, variant_id, NULL, &is_tier_game);
    if (error != 0) return error;

    const Game *game = GameManagerGetCurrentGame();
    if (is_tier_game) return GetStartTier(game);
    return GetStartRegular(game);
}

int HeadlessGetRandom(ReadOnlyString game_name, int variant_id) {
    bool is_tier_game;
    int error = InitAndCheckGame(game_name, variant_id, NULL, &is_tier_game);
    if (error != 0) return error;

    const Game *game = GameManagerGetCurrentGame();
    if (is_tier_game) return GetRandomTier(game);
    return GetRandomRegular(game);
}

// -----------------------------------------------------------------------------

static int InitAndCheckGame(ReadOnlyString game_name, int variant_id,
                            ReadOnlyString data_path, bool *is_tier_game) {
    int error = HeadlessInitSolver(game_name, variant_id, data_path);
    if (error != 0) {
        fprintf(stderr, "game initialization failed");
        return error;
    }

    const Game *game = GameManagerGetCurrentGame();
    assert(game != NULL);
    bool implements_regular = ImplementsRegularUwapi(game);
    bool implements_tier = ImplementsTierUwapi(game);
    if (!implements_regular && !implements_tier) {
        fprintf(stderr,
                "missing UWAPI function definition, check the game "
                "implementation\n");
        return kNotImplementedError;
    }

    *is_tier_game = implements_tier;
    return kNoError;
}

static bool ImplementsRegularUwapi(const Game *game) {
    if (game->uwapi == NULL) return false;
    if (game->uwapi->regular == NULL) return false;
    if (game->uwapi->regular->GenerateMoves == NULL) return false;
    if (game->uwapi->regular->DoMove == NULL) return false;
    if (game->uwapi->regular->Primitive == NULL) return false;
    if (game->uwapi->regular->IsLegalFormalPosition == NULL) return false;
    if (game->uwapi->regular->FormalPositionToPosition == NULL) return false;
    if (game->uwapi->regular->PositionToFormalPosition == NULL) return false;
    if (game->uwapi->regular->PositionToAutoGuiPosition == NULL) return false;
    // MoveToFormalMove and GeneratePartmoves are optional.
    if (game->uwapi->regular->MoveToAutoGuiMove == NULL) return false;
    if (game->uwapi->regular->GetInitialPosition == NULL) return false;
    // GetRandomLegalPosition is optional.

    return true;
}

static bool ImplementsTierUwapi(const Game *game) {
    if (game->uwapi == NULL) return false;
    if (game->uwapi->tier == NULL) return false;
    if (game->uwapi->tier->GenerateMoves == NULL) return false;
    if (game->uwapi->tier->DoMove == NULL) return false;
    if (game->uwapi->tier->Primitive == NULL) return false;
    if (game->uwapi->tier->IsLegalFormalPosition == NULL) return false;
    if (game->uwapi->tier->FormalPositionToTierPosition == NULL) return false;
    if (game->uwapi->tier->TierPositionToFormalPosition == NULL) return false;
    if (game->uwapi->tier->TierPositionToAutoGuiPosition == NULL) return false;
    // MoveToFormalMove and GeneratePartmoves are optional.
    if (game->uwapi->tier->MoveToAutoGuiMove == NULL) return false;
    if (game->uwapi->tier->GetInitialTier == NULL) return false;
    if (game->uwapi->tier->GetInitialPosition == NULL) return false;
    // GetRandomLegalTierPosition is optional.

    return true;
}

static int QueryRegular(const Game *game, ReadOnlyString formal_position) {
    bool legal = game->uwapi->regular->IsLegalFormalPosition(formal_position);
    if (!legal) {
        fprintf(stderr, "illegal position");
        return kIllegalGamePositionError;
    }

    Position position =
        game->uwapi->regular->FormalPositionToPosition(formal_position);
    assert(position >= 0);
    return JsonPrintPositionResponse(game, position);
}

static int QueryTier(const Game *game, ReadOnlyString formal_position) {
    bool legal = game->uwapi->tier->IsLegalFormalPosition(formal_position);
    if (!legal) {
        fprintf(stderr, "illegal position");
        return kIllegalGamePositionError;
    }

    TierPosition tier_position =
        game->uwapi->tier->FormalPositionToTierPosition(formal_position);
    assert(tier_position.tier >= 0 && tier_position.position >= 0);
    return JsonPrintTierPositionResponse(game, tier_position);
}

static int GetStartRegular(const Game *game) {
    Position start = game->uwapi->regular->GetInitialPosition();
    if (start < 0) {
        fprintf(
            stderr,
            "illegal initial position, please check the game implementation");
        return kIllegalGamePositionError;
    }

    int ret = 0;
    CString formal_start =
        game->uwapi->regular->PositionToFormalPosition(start);
    CString autogui_start =
        game->uwapi->regular->PositionToAutoGuiPosition(start);
    if (CStringError(&formal_start) || CStringError(&autogui_start)) {
        fprintf(stderr, "out of memory");
        ret = kMallocFailureError;
    } else {
        JsonPrintStartResponse(formal_start.str, autogui_start.str);
    }
    CStringDestroy(&formal_start);
    CStringDestroy(&autogui_start);

    return ret;
}

static int GetStartTier(const Game *game) {
    TierPosition start = {
        .tier = game->uwapi->tier->GetInitialTier(),
        .position = game->uwapi->tier->GetInitialPosition(),
    };
    if (start.tier < 0 || start.position < 0) {
        fprintf(
            stderr,
            "illegal initial position, please check the game implementation");
        return kIllegalGamePositionError;
    }

    int ret = 0;
    CString formal_start =
        game->uwapi->tier->TierPositionToFormalPosition(start);
    CString autogui_start =
        game->uwapi->tier->TierPositionToAutoGuiPosition(start);
    if (CStringError(&formal_start) || CStringError(&autogui_start)) {
        fprintf(stderr, "out of memory");
        ret = kMallocFailureError;
    } else {
        JsonPrintStartResponse(formal_start.str, autogui_start.str);
    }
    CStringDestroy(&formal_start);
    CStringDestroy(&autogui_start);

    return ret;
}

static int GetRandomRegular(const Game *game) {
    if (game->uwapi->regular->GetRandomLegalPosition == NULL) {
        fprintf(stderr, "position randomization not supported");
        return kNotImplementedError;
    }
    Position random = game->uwapi->regular->GetRandomLegalPosition();
    if (random < 0) {
        fprintf(
            stderr,
            "illegal initial position, please check the game implementation");
        return kIllegalGamePositionError;
    }

    int ret = 0;
    CString formal_random =
        game->uwapi->regular->PositionToFormalPosition(random);
    CString autogui_random =
        game->uwapi->regular->PositionToAutoGuiPosition(random);
    if (CStringError(&formal_random) || CStringError(&autogui_random)) {
        fprintf(stderr, "out of memory");
        ret = kMallocFailureError;
    } else {
        JsonPrintRandomResponse(formal_random.str, autogui_random.str);
    }
    CStringDestroy(&formal_random);
    CStringDestroy(&autogui_random);

    return ret;
}

static int GetRandomTier(const Game *game) {
    if (game->uwapi->tier->GetRandomLegalTierPosition == NULL) {
        fprintf(stderr, "position randomization not supported");
        return kNotImplementedError;
    }
    TierPosition random = game->uwapi->tier->GetRandomLegalTierPosition();
    if (random.tier < 0 || random.position < 0) {
        fprintf(
            stderr,
            "illegal initial position, please check the game implementation");
        return kIllegalGamePositionError;
    }

    int ret = 0;
    CString formal_random =
        game->uwapi->tier->TierPositionToFormalPosition(random);
    CString autogui_random =
        game->uwapi->tier->TierPositionToAutoGuiPosition(random);
    if (CStringError(&formal_random) || CStringError(&autogui_random)) {
        fprintf(stderr, "out of memory");
        ret = kMallocFailureError;
    } else {
        JsonPrintRandomResponse(formal_random.str, autogui_random.str);
    }
    CStringDestroy(&formal_random);
    CStringDestroy(&autogui_random);

    return ret;
}

static MoveArray GetMovesFromPosition(const Game *game, Position position) {
    MoveArray moves;
    if (game->uwapi->regular->Primitive(position) == kUndecided) {
        moves = game->uwapi->regular->GenerateMoves(position);
    } else {
        // Calling GenerateMoves on a primitive position is undefined behavior.
        MoveArrayInit(&moves);
    }

    return moves;
}

static PartmoveArray GetPartmovesFromPosition(const Game *game,
                                              Position position) {
    PartmoveArray partmoves;
    if (game->uwapi->regular->GeneratePartmoves == NULL) {
        // Multipart moves not implemented.
        PartmoveArrayInit(&partmoves);
        return partmoves;
    }

    if (game->uwapi->regular->Primitive(position) != kUndecided) {
        // Calling GeneratePartmoves on a primitive position is undefined
        // behavior.
        PartmoveArrayInit(&partmoves);
        return partmoves;
    }

    partmoves = game->uwapi->regular->GeneratePartmoves(position);
    return partmoves;
}

static int JsonPrintPositionResponse(const Game *game, Position position) {
    int ret = 0;
    MoveArray moves = GetMovesFromPosition(game, position);
    PartmoveArray partmoves = GetPartmovesFromPosition(game, position);
    json_object *moves_array_obj = NULL, *child_obj = NULL, *parent_obj = NULL;
    json_object *partmoves_array_obj = NULL, *partmove_edge_obj = NULL;
    if (moves.size < 0 || partmoves.size < 0) {
        fprintf(stderr, "out of memory");
        ret = kMallocFailureError;
        goto _bailout;
    }

    moves_array_obj = json_object_new_array_ext((int)moves.size);
    partmoves_array_obj = json_object_new_array_ext((int)partmoves.size);
    if (moves_array_obj == NULL || partmoves_array_obj == NULL) {
        fprintf(stderr, "out of memory");
        ret = kMallocFailureError;
        goto _bailout;
    }

    // Add moves and corresponding child positions.
    for (int64_t i = 0; i < moves.size; ++i) {
        Move move = moves.array[i];
        child_obj = JsonCreateChildPositionObject(game, position, move);
        if (child_obj == NULL) {
            fprintf(stderr, "out of memory");
            ret = kMallocFailureError;
            goto _bailout;
        }
        json_object_array_add(moves_array_obj, child_obj);
        child_obj = NULL;
    }

    // Add part-moves.
    for (int64_t i = 0; i < partmoves.size; ++i) {
        partmove_edge_obj = JsonCreatePartmoveEdgeObject(&partmoves.array[i]);
        if (partmove_edge_obj == NULL) {
            fprintf(stderr, "out of memory");
            ret = kMallocFailureError;
            goto _bailout;
        }
        json_object_array_add(partmoves_array_obj, partmove_edge_obj);
        partmove_edge_obj = NULL;
    }

    parent_obj = JsonCreateParentPositionObject(game, position, moves_array_obj,
                                                partmoves_array_obj);
    if (parent_obj == NULL) {
        fprintf(stderr, "out of memory");
        ret = kMallocFailureError;
        goto _bailout;
    }
    moves_array_obj = NULL;
    printf("%s\n", json_object_to_json_string(parent_obj));

_bailout:
    MoveArrayDestroy(&moves);
    PartmoveArrayDestroy(&partmoves);
    json_object_put(moves_array_obj);
    json_object_put(child_obj);
    json_object_put(parent_obj);
    json_object_put(partmoves_array_obj);
    json_object_put(partmove_edge_obj);
    return ret;
}

static json_object *JsonCreateBasicPositionObject(const Game *game,
                                                  Position position) {
    json_object *ret = json_object_new_object();
    if (ret == NULL) goto _bailout;

    CString formal_position =
        game->uwapi->regular->PositionToFormalPosition(position);
    CString autogui_position =
        game->uwapi->regular->PositionToAutoGuiPosition(position);
    if (CStringError(&formal_position) || CStringError(&autogui_position)) {
        json_object_put(ret);
        ret = NULL;
        goto _bailout;
    }
    int error = HeadlessJsonAddPosition(ret, formal_position.str);
    error |= HeadlessJsonAddAutoGuiPosition(ret, autogui_position.str);

    TierPosition tier_position = {.tier = kDefaultTier, .position = position};
    Value value = SolverManagerGetValue(tier_position);
    int remoteness = SolverManagerGetRemoteness(tier_position);
    error |= HeadlessJsonAddValue(ret, value);
    error |= HeadlessJsonAddRemoteness(ret, remoteness);
    if (error) {
        json_object_put(ret);
        ret = NULL;
    }

_bailout:
    CStringDestroy(&formal_position);
    CStringDestroy(&autogui_position);
    return ret;
}

static json_object *JsonCreateChildPositionObject(const Game *game,
                                                  Position parent, Move move) {
    Position child = game->uwapi->regular->DoMove(parent, move);
    json_object *ret = JsonCreateBasicPositionObject(game, child);
    if (ret == NULL) return NULL;

    CString formal_move = game->uwapi->regular->MoveToFormalMove(parent, move);
    CString autogui_move =
        game->uwapi->regular->MoveToAutoGuiMove(parent, move);
    if (CStringError(&formal_move) || CStringError(&autogui_move)) {
        json_object_put(ret);
        return NULL;
    }

    int error = HeadlessJsonAddMove(ret, formal_move.str);
    if (!CStringIsNull(&autogui_move)) {  // Only add full-moves.
        error |= HeadlessJsonAddAutoGuiMove(ret, autogui_move.str);
    }
    CStringDestroy(&formal_move);
    CStringDestroy(&autogui_move);
    if (error) {
        json_object_put(ret);
        return NULL;
    }

    return ret;
}

static json_object *JsonCreateParentPositionObject(
    const Game *game, Position position, json_object *moves_array_obj,
    json_object *partmoves_array_obj) {
    //
    json_object *ret = JsonCreateBasicPositionObject(game, position);
    if (ret == NULL) return NULL;

    int error = HeadlessJsonAddMovesArray(ret, moves_array_obj);
    error |= HeadlessJsonAddPartmovesArray(ret, partmoves_array_obj);
    if (error) {
        json_object_put(ret);
        return NULL;
    }

    return ret;
}

static int JsonPrintTierPositionResponse(const Game *game,
                                         TierPosition tier_position) {
    int ret = 0;
    MoveArray moves = GetMovesFromTierPosition(game, tier_position);
    PartmoveArray partmoves = GetPartmovesFromTierPosition(game, tier_position);
    json_object *moves_array_obj = NULL, *child_obj = NULL, *parent_obj = NULL;
    json_object *partmoves_array_obj = NULL, *partmove_edge_obj = NULL;
    if (moves.size < 0 || partmoves.size < 0) {
        fprintf(stderr, "out of memory");
        ret = kMallocFailureError;
        goto _bailout;
    }

    moves_array_obj = json_object_new_array_ext((int)moves.size);
    partmoves_array_obj = json_object_new_array_ext((int)partmoves.size);
    if (moves_array_obj == NULL || partmoves_array_obj == NULL) {
        fprintf(stderr, "out of memory");
        ret = kMallocFailureError;
        goto _bailout;
    }

    // Add moves and corresponding child tier positions.
    for (int64_t i = 0; i < moves.size; ++i) {
        Move move = moves.array[i];
        child_obj =
            JsonCreateChildTierPositionObject(game, tier_position, move);
        if (child_obj == NULL) {
            fprintf(stderr, "out of memory");
            ret = kMallocFailureError;
            goto _bailout;
        }
        json_object_array_add(moves_array_obj, child_obj);
        child_obj = NULL;
    }

    // Add part-moves.
    for (int64_t i = 0; i < partmoves.size; ++i) {
        partmove_edge_obj = JsonCreatePartmoveEdgeObject(&partmoves.array[i]);
        if (partmove_edge_obj == NULL) {
            fprintf(stderr, "out of memory");
            ret = kMallocFailureError;
            goto _bailout;
        }
        json_object_array_add(partmoves_array_obj, partmove_edge_obj);
        partmove_edge_obj = NULL;
    }

    parent_obj = JsonCreateParentTierPositionObject(
        game, tier_position, moves_array_obj, partmoves_array_obj);
    if (parent_obj == NULL) {
        fprintf(stderr, "out of memory");
        ret = kMallocFailureError;
        goto _bailout;
    }
    moves_array_obj = NULL;
    printf("%s\n", json_object_to_json_string(parent_obj));

_bailout:
    MoveArrayDestroy(&moves);
    PartmoveArrayDestroy(&partmoves);
    json_object_put(moves_array_obj);
    json_object_put(child_obj);
    json_object_put(parent_obj);
    json_object_put(partmoves_array_obj);
    json_object_put(partmove_edge_obj);
    return ret;
}

static MoveArray GetMovesFromTierPosition(const Game *game,
                                          TierPosition tier_position) {
    MoveArray moves;
    if (game->uwapi->tier->Primitive(tier_position) == kUndecided) {
        moves = game->uwapi->tier->GenerateMoves(tier_position);
    } else {
        // Calling GenerateMoves on a primitive position is undefined behavior.
        MoveArrayInit(&moves);
    }

    return moves;
}

static PartmoveArray GetPartmovesFromTierPosition(const Game *game,
                                                  TierPosition tier_position) {
    PartmoveArray partmoves;
    if (game->uwapi->tier->GeneratePartmoves == NULL) {
        // Multipart moves not implemented.
        PartmoveArrayInit(&partmoves);
        return partmoves;
    }

    if (game->uwapi->tier->Primitive(tier_position) != kUndecided) {
        // Calling GeneratePartmoves on a primitive position is undefined
        // behavior.
        PartmoveArrayInit(&partmoves);
        return partmoves;
    }

    partmoves = game->uwapi->tier->GeneratePartmoves(tier_position);
    return partmoves;
}

static json_object *JsonCreateBasicTierPositionObject(
    const Game *game, TierPosition tier_position) {
    //
    json_object *ret = json_object_new_object();
    if (ret == NULL) goto _bailout;

    CString formal_position =
        game->uwapi->tier->TierPositionToFormalPosition(tier_position);
    CString autogui_position =
        game->uwapi->tier->TierPositionToAutoGuiPosition(tier_position);
    if (CStringError(&formal_position) || CStringError(&autogui_position)) {
        json_object_put(ret);
        ret = NULL;
        goto _bailout;
    }
    int error = HeadlessJsonAddPosition(ret, formal_position.str);
    error |= HeadlessJsonAddAutoGuiPosition(ret, autogui_position.str);

    Value value = SolverManagerGetValue(tier_position);
    int remoteness = SolverManagerGetRemoteness(tier_position);
    error |= HeadlessJsonAddValue(ret, value);
    error |= HeadlessJsonAddRemoteness(ret, remoteness);
    if (error) {
        json_object_put(ret);
        ret = NULL;
    }

_bailout:
    CStringDestroy(&formal_position);
    CStringDestroy(&autogui_position);
    return ret;
}

static json_object *JsonCreateChildTierPositionObject(const Game *game,
                                                      TierPosition parent,
                                                      Move move) {
    TierPosition child = game->uwapi->tier->DoMove(parent, move);
    json_object *ret = JsonCreateBasicTierPositionObject(game, child);
    if (ret == NULL) return NULL;

    CString formal_move = game->uwapi->tier->MoveToFormalMove(parent, move);
    CString autogui_move = game->uwapi->tier->MoveToAutoGuiMove(parent, move);
    if (CStringError(&formal_move) || CStringError(&autogui_move)) {
        json_object_put(ret);
        return NULL;
    }

    int error = HeadlessJsonAddMove(ret, formal_move.str);
    if (!CStringIsNull(&autogui_move)) {  // Only add full-moves.
        error |= HeadlessJsonAddAutoGuiMove(ret, autogui_move.str);
    }
    CStringDestroy(&formal_move);
    CStringDestroy(&autogui_move);
    if (error) {
        json_object_put(ret);
        return NULL;
    }

    return ret;
}

static json_object *JsonCreateParentTierPositionObject(
    const Game *game, TierPosition tier_position, json_object *moves_array_obj,
    json_object *partmoves_array_obj) {
    //
    json_object *ret = JsonCreateBasicTierPositionObject(game, tier_position);
    if (ret == NULL) return NULL;

    int error = HeadlessJsonAddMovesArray(ret, moves_array_obj);
    error |= HeadlessJsonAddPartmovesArray(ret, partmoves_array_obj);
    if (error) {
        json_object_put(ret);
        return NULL;
    }

    return ret;
}

static json_object *JsonCreatePartmoveEdgeObject(const Partmove *pm) {
    json_object *ret = json_object_new_object();
    if (ret == NULL) return ret;

    int error = HeadlessJsonAddAutoGuiMove(ret, pm->autogui_move.str);
    error |= HeadlessJsonAddMove(ret, pm->formal_move.str);
    if (!CStringIsNull(&pm->from)) {
        error |= HeadlessJsonAddFrom(ret, pm->from.str);
    }
    if (!CStringIsNull(&pm->to)) {
        error |= HeadlessJsonAddTo(ret, pm->to.str);
    }
    if (!CStringIsNull(&pm->full)) {
        error |= HeadlessJsonAddFull(ret, pm->full.str);
    }
    if (error) {
        json_object_put(ret);
        ret = NULL;
    }

    return ret;
}

static void JsonPrintStartResponse(ReadOnlyString formal_start,
                                   ReadOnlyString autogui_start) {
    JsonPrintSinglePosition(formal_start, autogui_start);
}

static void JsonPrintRandomResponse(ReadOnlyString formal_random,
                                    ReadOnlyString autogui_random) {
    JsonPrintSinglePosition(formal_random, autogui_random);
}

static void JsonPrintSinglePosition(ReadOnlyString position,
                                    ReadOnlyString autogui_position) {
    json_object *response = json_object_new_object();
    HeadlessJsonAddPosition(response, position);
    HeadlessJsonAddAutoGuiPosition(response, autogui_position);
    printf("%s\n", json_object_to_json_string(response));
    json_object_put(response);
}
