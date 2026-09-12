# Game Developer's Guide

This guide walks through the steps required to add a new game to GamesmanOne.
It focuses on fulfilling the API contracts expected by the solver, TextUI, and
web frontend, rather than on the implementation details of any particular game.

## Prerequisites

A game in GamesmanOne must satisfy two basic properties to be solvable:

1. **Finite**: The game must have a finite number of positions.
2. **Two-person perfect-information**: Both players have full knowledge of the
   game state at all times.

> [!TIP]
> Read the example games alongside this guide. Use
> [`src/games/mttt/mttt.c`](./mttt/mttt.c)
> as a reference for a single-tier game using the Regular Solver, and
> [`src/games/mtttier/mtttier.c`](./mtttier/mtttier.c)
> as a reference for a tiered game using the Tier Solver.

---

## Step 1: Understand the `Game` Struct

Start by reading
[`src/core/types/game/game.h`](../core/types/game/game.h).
Every game is represented as a single `const Game` object — a struct of
constants and function pointers that wires together all the subsystems.

The required fields are:

| Field | Type | Description |
|---|---|---|
| `name` | `char[]` | Internal name: no whitespace or special characters. Used as a CLI argument. |
| `formal_name` | `char[]` | Human-readable display name. |
| `solver` | `const Solver *` | Pointer to the chosen solver (`&kTierSolver` or `&kRegularSolver`). |
| `solver_api` | `const void *` | Pointer to the filled-in solver API struct (cast to `void *`). |
| `gameplay_api` | `const GameplayApi *` | Pointer to the gameplay API struct (required for TextUI and testing). |
| `Init` | function pointer | Initializes the game module (e.g., sets up hashing). Returns 0 on success. |
| `Finalize` | function pointer | Tears down the game module, freeing all resources. Returns 0 on success. |

The optional fields are:

| Field | Type | Description |
|---|---|---|
| `uwapi` | `const Uwapi *` | Pointer to the UWAPI struct. Required only for web frontend support. |
| `GetCurrentVariant` | function pointer | Returns the current variant. Set to `NULL` if only one variant exists. |
| `SetVariantOption` | function pointer | Sets a variant option. Set to `NULL` if only one variant exists. |

### Variants

A **game variant** is a parameterized rule variation, such as different board
sizes. It is described by a
[`GameVariant`](../core/types/game/game_variant.h)
struct, which holds an array of
[`GameVariantOption`](../core/types/game/game_variant_option.h)
objects, each listing the possible string-labelled choices for that option,
along with a parallel array of the currently selected choice indices.

If your game has no variants, set both `GetCurrentVariant` and
`SetVariantOption` to `NULL`.

---

## Step 2: Choose a Solver

GamesmanOne currently provides two solvers. Read their API headers carefully
before deciding which one to use.

### The Regular Solver

[`src/core/solvers/regular_solver/regular_solver.h`](../core/solvers/regular_solver/regular_solver.h)
— Use this solver for games that are either small (up to a few million
positions) or not tierable (see below).

### The Tier Solver

[`src/core/solvers/tier_solver/tier_solver.h`](../core/solvers/tier_solver/tier_solver.h)
— Use this solver for large games that are tierable.

### What Is a Tierable Game?

A tierable game is one whose positions can be partitioned into more than one
disjoint sets, each called a **tier**, such that the tiers form a directed
acyclic graph (DAG): tier A has a directed edge to tier B if and only if there
exists a position in A with a legal move to a position in B. A good example is
chess, where tiers can be defined by the set of pieces remaining on the board —
removing a piece is an irreversible move, so chess positions naturally organize
into a DAG of tiers. On the other hand, games like All-Queens Chess, where most
positions are mutually reachable, are not tierable.

The most reliable indicator of tierability is the presence of **irreversible
moves**. Loop-free games have no reversible moves at all and are therefore
trivially tierable. For other games, look for moves that cannot be undone —
captures, piece placements, promotions, etc.

Tierability enables two important performance optimizations:

- **Parallelization**: Independent tiers whose child tiers have all been solved
  can be solved concurrently.
- **Memory savings**: Only the positions of the tiers currently being solved
  need to be loaded into RAM. The rest of the database can stay on disk.

**Recommendation**: Use the Tier Solver whenever the game has more than roughly
1 billion positions and is tierable.

---

## Step 3: Implement the Solver API

### Regular Solver API (`RegularSolverApi`)

Defined in
[`regular_solver.h`](../core/solvers/regular_solver/regular_solver.h).

#### Required functions

| Function | Signature (simplified) | Description |
|---|---|---|
| `GetNumPositions` | `int64_t (void)` | Returns the total number of positions (i.e., hash space size). |
| `GetInitialPosition` | `Position (void)` | Returns the starting position. |
| `GenerateMoves` | `int (Position, Move[])` | Fills `moves[]` with legal moves and returns the count. |
| `Primitive` | `Value (Position)` | Returns `kLose`, `kWin`, `kTie`, or `kUndecided`. |
| `DoMove` | `Position (Position, Move)` | Returns the position resulting from applying a move. |
| `IsLegalPosition` | `bool (Position)` | Returns false if the position is definitely unreachable; true otherwise. |

> [!IMPORTANT]
> `IsLegalPosition` is a filter for speed, not a completeness guarantee.
> If it returns `true`, the position must be safe to pass to all other API
> functions. If it returns `false`, the solver will skip that position entirely.
> A conservative implementation that always returns `true` is correct but may
> be slow on large games.

#### Optional functions

| Function | Description |
|---|---|
| `GetCanonicalPosition` | Returns the canonical (lowest hash) representative of a symmetry class. Enables the Position Symmetry Removal optimization. |
| `GetCanonicalChildPositions` | Returns unique canonical child positions directly. An optimization over calling `GenerateMoves` + `DoMove` + `GetCanonicalPosition`. |
| `GetCanonicalParentPositions` | Returns unique canonical parent positions. Required for Retrograde Analysis (see [Step 4](#step-4-implement-getcanonicalparentpositions)). |
| `GetNumberOfCanonicalChildPositions` | Returns the count of unique canonical children. A further optimization when combined with `GetCanonicalChildPositions`. |

### Tier Solver API (`TierSolverApi`)

Defined in
[`tier_solver.h`](../core/solvers/tier_solver/tier_solver.h).

#### Required functions

| Function | Signature (simplified) | Description |
|---|---|---|
| `GetInitialTier` | `Tier (void)` | Returns the tier containing the initial position. |
| `GetChildTiers` | `int (Tier, Tier[])` | Fills `child_tiers[]` with the direct child tiers and returns the count. |
| `GetTierSize` | `int64_t (Tier)` | Returns the number of positions in the given tier (i.e., hash space size of that tier). |
| `GetInitialPosition` | `Position (void)` | Returns the initial position within the initial tier. |
| `Primitive` | `Value (TierPosition)` | Returns `kLose`, `kWin`, `kTie`, or `kUndecided`. |
| `GenerateMoves` | `int (TierPosition, Move[])` | Fills `moves[]` with legal moves and returns the count. |
| `DoMove` | `TierPosition (TierPosition, Move)` | Returns the tier position resulting from applying a move. |
| `IsLegalPosition` | `bool (TierPosition)` | Returns false if the tier position is definitely unreachable; true otherwise. |

> [!NOTE]
> `TierPosition` is a pair `{Tier tier; Position position}`. A `Position` is a
> 64-bit integer hash within its tier, so the same integer value may appear in
> multiple tiers and refer to different board states.

#### Optional functions — Tier symmetry removal

| Function | Description |
|---|---|
| `GetCanonicalTier` | Returns the canonical representative of a symmetry class of tiers. |
| `GetPositionInSymmetricTier` | Translates a position from one tier to its image in a symmetric tier. |

Both functions must be implemented together; implementing only one has no effect.

#### Optional functions — Position symmetry removal

| Function | Description |
|---|---|
| `GetCanonicalPosition` | Returns the canonical (lowest hash) representative of a symmetry class of positions within a tier. |
| `GetNumberOfSymmetries` | Returns the number of positions symmetric to the given one (including itself). Used for more efficient game analysis. |

#### Optional functions — Performance and retrograde analysis

| Function | Description |
|---|---|
| `GetCanonicalParentPositions` | Returns unique canonical parent positions within a given parent tier. Required for Retrograde Analysis (see [Step 4](#step-4-implement-getcanonicalparentpositions)). |
| `GetCanonicalChildPositions` | Returns unique canonical child tier positions directly. An optimization over calling `GenerateMoves` + `DoMove` + `GetCanonicalPosition`. |
| `GetNumberOfCanonicalChildPositions` | Returns the count of unique canonical children. A further optimization when combined with `GetCanonicalChildPositions`. |

#### Optional functions — Visualization and debugging

| Function | Description |
|---|---|
| `GetTierType` | Returns the `TierType` of a tier. If not implemented, all tiers are treated as loopy, which is always correct but may be slower for loop-free tiers. |
| `GetTierName` | Returns a human-readable name for a tier, used as the database file name. If not implemented, the raw tier integer is used as the file name. |

---

## Step 4: Implement `GetCanonicalParentPositions`

The most powerful optional function in both solver APIs is
`GetCanonicalParentPositions`. This function enables **retrograde analysis**:
instead of building and storing the entire transpose (reverse) graph in memory,
the solver can query parent positions on the fly. Without it, the solver must
traverse all positions in the current tier and its child tiers, and store the 
entire reverse graph in memory — all of which is extremely slow and memory-
intensive for large games.

Beyond performance, implementing this function provides a valuable **correctness
check**. Because you must derive parent positions using pure game logic — without
reusing the forward direction logic — any inconsistency between the two
directions will be caught automatically by the solver's test suite. This
independent reimplementation has historically exposed many subtle bugs.

The function signature for the Tier Solver is:

```c
int GetCanonicalParentPositions(
    TierPosition child,
    Tier parent_tier,
    Position parents[static kTierSolverNumParentPositionsMax]);
```

It fills `parents[]` with the canonical hashes (within `parent_tier`) of all
legal positions in `parent_tier` that have a legal move to `child`, and returns
the count. The function is allowed to return some unreachable positions. This
definition does not affect the correctness of the solver and greatly simplifies
the implementation of this function. The proof of correctness is outside the
scope of this tutorial. However, deduplication is the responsibility of the 
implementation.

> [!IMPORTANT]
> We **strongly recommend** implementing `GetCanonicalParentPositions`. It is
> not strictly required if you have sufficient RAM, but it is almost always
> necessary for games with more than a few million positions, and it is one of
> the best tools for catching correctness bugs during development.

---

## Step 5: Implement the Gameplay API

The **Gameplay API** drives the interactive TextUI mode and the automated test
runner. It must be implemented for all games regardless of the solver chosen.

### Structure

A [`GameplayApi`](../core/types/gameplay_api/gameplay_api.h)
contains a pointer to a common sub-struct and exactly one of two
solver-specific sub-structs:

```c
typedef struct GameplayApi {
    const GameplayApiCommon  *common;   // always required
    const GameplayApiRegular *regular;  // for non-tier games
    const GameplayApiTier    *tier;     // for tier games
} GameplayApi;
```

Set the one that does not apply to `NULL`.

### `GameplayApiCommon` (always required)

Defined in
[`gameplay_api_common.h`](../core/types/gameplay_api/gameplay_api_common.h).

| Member | Description |
|---|---|
| `GetInitialPosition` | Returns the initial position (or initial position within the initial tier for tier games). |
| `position_string_length_max` | Maximum number of characters in a rendered board string (excluding `'\0'`). |
| `move_string_length_max` | Maximum number of characters in a move string (excluding `'\0'`). |
| `MoveToString` | Converts a `Move` integer to its human-readable string representation. |
| `IsValidMoveString` | Validates a user-typed move string before it is parsed. |
| `StringToMove` | Converts a validated move string to a `Move` integer. |

> [!NOTE]
> The strings `"b"`, `"q"`, `"u"`, and `"v"` are reserved by the TextUI system.
> Make sure your move strings never collide with these.

### `GameplayApiRegular` (for non-tier games)

Defined in
[`gameplay_api_regular.h`](../core/types/gameplay_api/gameplay_api_regular.h).

| Member | Description |
|---|---|
| `PositionToString` | Renders a `Position` as a human-readable board into a pre-allocated buffer. |
| `GenerateMoves` | Returns a `MoveArray` of legal moves (heap-allocated; the caller frees it). |
| `DoMove` | Applies a move and returns the resulting `Position`. |
| `Primitive` | Returns the primitive value of the position, or `kUndecided`. |

### `GameplayApiTier` (for tier games)

Defined in
[`gameplay_api_tier.h`](../core/types/gameplay_api/gameplay_api_tier.h).

| Member | Description |
|---|---|
| `GetInitialTier` | Returns the initial tier. |
| `TierPositionToString` | Renders a `TierPosition` as a human-readable board into a pre-allocated buffer. |
| `GenerateMoves` | Returns a `MoveArray` of legal moves (heap-allocated; the caller frees it). |
| `DoMove` | Applies a move and returns the resulting `TierPosition`. |
| `Primitive` | Returns the primitive value of the tier position, or `kUndecided`. |

> [!NOTE]
> The `GenerateMoves` functions in the Gameplay API return a heap-allocated
> `MoveArray`, unlike their solver API counterparts which write into a
> stack-allocated array. In practice, most implementations delegate to the
> solver's `GenerateMoves` and copy the results into a `MoveArray`.

---

## Step 6: Implement the UWAPI (Optional — Web Frontend)

To launch the game on [GamesmanUni](https://github.com/GamesCrafters/GamesmanUni),
you must implement the **Universal Web API (UWAPI)**. The UWAPI is consumed by
[GamesCraftersUWAPI](https://github.com/GamesCrafters/GamesCraftersUWAPI),
an internal request-routing server that bridges GamesmanOne with the web GUI.

The top-level
[`Uwapi`](../core/types/uwapi/uwapi.h) struct
mirrors the `GameplayApi` structure: it holds pointers to one of two
solver-specific sub-structs depending on whether the game uses the Tier Solver
or the Regular Solver.

```c
typedef struct Uwapi {
    const UwapiRegular *regular;   // for non-tier games
    const UwapiTier    *tier;      // for tier games
} Uwapi;
```

### `UwapiRegular` and `UwapiTier`

Both are defined in
[`uwapi_regular.h`](../core/types/uwapi/uwapi_regular.h)
and
[`uwapi_tier.h`](../core/types/uwapi/uwapi_tier.h)
respectively. They share a common pattern. Most game-logic functions
(`GenerateMoves`, `DoMove`, `Primitive`, `GetInitialPosition`, and
`GetInitialTier` for tier games) are the same as those in the solver and
gameplay APIs. In addition, UWAPI requires:

| Function | Description |
|---|---|
| `IsLegalFormalPosition` | Validates a formal position string received from the web. This is a security boundary — the input is raw user data. |
| `FormalPositionToPosition` / `FormalPositionToTierPosition` | Converts a validated formal position string to the internal hash. |
| `PositionToFormalPosition` / `TierPositionToFormalPosition` | Converts an internal hash to a human-editable formal position string (e.g., a FEN-like string). |
| `PositionToAutoGuiPosition` / `TierPositionToAutoGuiPosition` | Converts an internal hash to an AutoGUI position string used by GamesmanUni for board rendering. |
| `MoveToFormalMove` | Converts a move to a human-readable formal move string. |
| `MoveToAutoGuiMove` | Converts a move to an AutoGUI move string that encodes rendering instructions. |
| `GetRandomLegalPosition` / `GetRandomLegalTierPosition` | (Optional) Returns a random legal position for the "I'm Feeling Lucky" feature. |
| `GeneratePartmoves` | (Optional) Returns part-moves for the multipart move subsystem (see below). |

> [!NOTE]
> **Formal positions** are human-editable strings meant to be used as URL
> parameters in database queries (analogous to FEN strings in chess).
> **AutoGUI positions** are a different, more opaque format that encodes
> additional rendering metadata for GamesmanUni's board animation system.

### AutoGUI and Multipart Moves

The AutoGUI system handles board animation in the web frontend, and the
multipart move subsystem supports games where a single logical move is
naturally broken into multiple interactive steps by the player 
(e.g., moving one piece and then another in the same turn).
Both systems have their own detailed specifications.
Refer to the documentation in `src/core/types/uwapi/README.md`
(coming soon) for formatting rules, string conventions, and worked examples.

> [!CAUTION]
> `IsLegalFormalPosition` receives **unsanitized user input** from UWAPI
> queries. If it returns `true`, the input is treated as trusted and passed
> directly to other functions. Implement this function defensively.

---

## Step 7: Test Your Implementation

### Manual play

Before solving, test that the game behaves correctly interactively by launching the 
`gamesman` binary in interactive mode:

```
/path/to/gamesman
```

This launches the TextUI and lets you step through the game. The board
rendering from `PositionToString` / `TierPositionToString`, move parsing, and
move application can all be verified here without running the solver.

### Automated testing

Run the built-in test suite with:

```
/path/to/gamesman test <game_name> [variant_index]
```

The test runner exercises a random sample of positions from each tier (or from
the entire position space for non-tier games) and checks for internal
consistency. Examples of what the tests detect include:

- Child positions reported by `DoMove` that are not recognized as legal.
- Results from `GetCanonicalChildPositions` that do not match those produced by
  calling `GenerateMoves` + `DoMove` + `GetCanonicalPosition`.
- Results from `GetCanonicalParentPositions` that are not consistent with the
  children reported by `GenerateMoves` + `DoMove` (a particularly valuable
  cross-check when both directions are independently implemented).
- Tier symmetry inconsistencies (applying symmetry twice should be an identity).

> [!NOTE]
> The tests only cover a randomly sampled *subset* of positions. Even a passing
> test run does not guarantee correctness for all positions. For large games,
> full-coverage testing is generally infeasible. A trick that might be helpful
> for validating large games is to implement and verify a smaller variant under
> the same rules.

Some tests are only available when the corresponding optional API functions are
implemented. For example, the child/parent consistency check requires both
`GetCanonicalChildPositions` (or `GenerateMoves` + `DoMove`) and
`GetCanonicalParentPositions`. See the `TierSolverTestErrors` enum in
[`tier_solver.h`](../core/solvers/tier_solver/tier_solver.h#L481-L511)
for the full list of detected error types.

---

## Step 8: Register the Game

### Create source files

Create a new subdirectory under `src/games/` for your game:

```
src/games/<game_name>/
    <game_name>.h
    <game_name>.c
    CMakeLists.txt
```

**`<game_name>.h`** — Declare and `extern` the `const Game` object:

```c
#ifndef GAMESMANONE_GAMES_<GAME_NAME>_<GAME_NAME>_H_
#define GAMESMANONE_GAMES_<GAME_NAME>_<GAME_NAME>_H_

#include "core/types/game/game.h"

extern const Game k<GameName>;

#endif  // GAMESMANONE_GAMES_<GAME_NAME>_<GAME_NAME>_H_
```

**`CMakeLists.txt`** — Add the sources to the `gamesman_core` target:

```cmake
set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/<game_name>.h)
set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/<game_name>.c)
target_sources(gamesman_core PRIVATE ${HEADERS} ${SOURCES})
```

### Add to the game list

Open
[`src/games/game_list.c`](./game_list.c)
and make two edits, following the inline comments:

1. **Include the game header** at the top of the file (step 1 in the file):

   ```c
   #include "games/<game_name>/<game_name>.h"
   ```

2. **Add the game object pointer** to the `kAllGames[]` array (step 2), before
   the terminating `NULL`:

   ```c
   const Game *kAllGames[] = {
       ...,
       &k<GameName>,
       NULL,
   };
   ```

> [!IMPORTANT]
> The `kAllGames` array must always be terminated by a `NULL` entry.

After recompiling, verify that your game appears in the menu:

```
/path/to/gamesman
```
