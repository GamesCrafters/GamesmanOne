# GamesmanOne Developer's Guide

> [!NOTE]
> This guide is intended for contributors and developers working on GamesmanOne. For end-user setup instructions (prerequisites, installing dependencies, building the `release` binary, and running the program), please refer to the [root README](../README.md) first.

---

## Table of Contents

- [1. Build System](#1-build-system)
  - [How the Build System Is Organized](#how-the-build-system-is-organized)
  - [CMake Options Reference](#cmake-options-reference)
  - [Full Preset Reference](#full-preset-reference)
  - [Preset Design Philosophy](#preset-design-philosophy)
  - [Output Locations](#output-locations)
  - [The `gamesman_setup.sh` Helper Script](#the-gamesman_setupsh-helper-script)
  - [CMake Modules (`cmake/`)](#cmake-modules-cmake)
  - [Tips for Local Development](#tips-for-local-development)
- [2. Project Layout](#2-project-layout)
  - [Top-Level Directory Overview](#top-level-directory-overview)
  - [Source Tree: `src/`](#source-tree-src)
  - [Tests: `tests/`](#tests-tests)
  - [Other Notable Directories](#other-notable-directories)
- [3. Testing](#3-testing)
  - [Unit Tests](#unit-tests)
  - [End-to-End (E2E) Tests](#end-to-end-e2e-tests)
  - [Running Tests Locally](#running-tests-locally)
- [4. Continuous Integration](#4-continuous-integration)
  - [`presubmit.yml` — PR and Branch Validation](#presubmityml--pr-and-branch-validation)
  - [`postsubmit.yml` — Post-Merge Extended Coverage](#postsubmityml--post-merge-extended-coverage)
  - [`codeql.yml` — Security and Quality Analysis](#codeqlyml--security-and-quality-analysis)
  - [Maintaining the CI Matrix](#maintaining-the-ci-matrix)

---

## 1. Build System

### How the Build System Is Organized

GamesmanOne uses **CMake ≥ 3.25** with the **Ninja** build system and standardizes all build configurations through a [`CMakePresets.json`](../CMakePresets.json) file. This file is the single source of truth for every supported build mode — from the optimized release binary to sanitizer-instrumented CI builds. Using presets eliminates the need to remember long chains of `-D` flags and ensures that every developer and CI job uses identical, reproducible configurations.

The top-level [`CMakeLists.txt`](../CMakeLists.txt) orchestrates the build in several stages:

1. **Platform detection** — On macOS, the Homebrew LLVM toolchain and OpenMP headers are located *before* the `project()` call so the correct compilers are selected from the start.
2. **Language standards** — C17 and C++17 are enforced globally.
3. **CMake options** — Feature flags (sanitizers, coverage, benchmarks, etc.) are defined as CMake `option()` variables, which the presets then set declaratively via `cacheVariables`.
4. **Dependency resolution** — External libraries (zlib, OpenMP, MPI, GoogleTest, Google Benchmark) are located via `cmake/gamesman_dependencies.cmake`.
5. **Common compiler flags** — An INTERFACE library called `common_flags` is created and linked to every Gamesman target. It applies `-Wall -Wextra -Wpedantic -march=native` universally, and conditionally adds sanitizer, coverage, or static-analysis flags according to the active preset.
6. **Hardware feature detection** — The build system probes the host CPU for SIMD extensions (SSE2, SSSE3, SSE4.1, BMI1, BMI2) using `check_c_compiler_flag()`, and defines corresponding preprocessor macros (`GAMESMAN_HAS_SSE2`, etc.) so performance-critical code paths can be compiled with the best available instructions.
7. **System introspection** — A small C probe (`scripts/cacheline.c`) is compiled and run at configure time to detect the host CPU cache-line size. The processor count is also detected. Both values are baked into `src/config.h` via `config.h.in`.
8. **Link-Time Optimization (LTO/IPO)** — Enabled automatically for `Release` builds when the toolchain supports it, although it's gradually being replaced by header-only module implementations and will eventually be removed.
9. **Subdirectory targets** — `src/`, `tests/` (when testing is enabled), and `benchmarks/` (when benchmarks are enabled) are added.

### CMake Options Reference

The following options can be set manually via `-D` flags or — preferably — via a preset's `cacheVariables` block.

| Option | Default | Description |
| :--- | :---: | :--- |
| `GAMESMAN_ENABLE_OPENMP` | `ON` | Enable OpenMP multithreading. Disable for single-threaded builds (`-st` presets). |
| `GAMESMAN_ENABLE_MPI` | `OFF` | Enable MPI for distributed-memory solving (Savio cluster builds). |
| `GAMESMAN_ENABLE_STATIC_ANALYZER` | `OFF` | Enable compiler static analysis (GCC `-fanalyzer` or Clang-Tidy). |
| `GAMESMAN_ENABLE_ASAN_UBSAN` | `OFF` | Instrument with AddressSanitizer + UndefinedBehaviorSanitizer. |
| `GAMESMAN_ENABLE_TSAN` | `OFF` | Instrument with ThreadSanitizer (Clang only). |
| `GAMESMAN_ENABLE_LINE_COVERAGE` | `OFF` | Add `--coverage` instrumentation for `lcov` reports. |
| `GAMESMAN_ENABLE_BENCHMARKS` | `OFF` | Build the `gamesman_benchmark` target (requires Google Benchmark). |
| `GAMESMAN_ENABLE_TESTING` | `ON` | Build GoogleTest unit-test targets. Set `OFF` in the `release` preset. |
| `GAMESMAN_ENABLE_ANIMATION` | `OFF` | Enable terminal animations in interactive mode. Set `ON` in the `release` preset. |

### Full Preset Reference

All presets are defined in [`CMakePresets.json`](../CMakePresets.json). There is a hidden `base` configure preset that all others inherit from; it sets the binary output directory to `build/<presetName>/` and enables compile-command export (`CMAKE_EXPORT_COMPILE_COMMANDS=ON`, which powers editor tooling like `clangd`).

#### Configure & Build Presets

| Configure Preset | Build Preset | Build Type | Key Flags | Primary Use |
| :--- | :--- | :--- | :--- | :--- |
| `release` | `release-build` | `Release` | Animations ON, testing OFF, output → `./bin/` | **End-user / distribution binary** |
| `benchmark` | `benchmark-build` | `Release` | Inherits `release`; benchmarks ON | Microbenchmark development |
| `ci-asan` | `ci-asan-build` | `RelWithDebInfo` | ASan + UBSan + OpenMP | **Primary CI validation** |
| `ci-asan-st` | `ci-asan-st-build` | `RelWithDebInfo` | ASan + UBSan, **OpenMP disabled** | Verifying single-threaded correctness |
| `ci-tsan` | `ci-tsan-build` | `RelWithDebInfo` | TSan, Clang-only | Multithreading race detection (manual) |
| `ci-coverage` | `ci-coverage-build` | `Debug` | Line coverage (`--coverage`) + OpenMP | Coverage report generation |
| `ci-coverage-st` | `ci-coverage-st-build` | `Debug` | Line coverage, **OpenMP disabled** | Single-threaded coverage |
| `ci-analyze-gcc` | `ci-analyze-gcc-build` | `Debug` | GCC `-fanalyzer`, GCC toolchain | GCC static analysis |
| `ci-analyze-clang` | `ci-analyze-clang-build` | `Debug` | Clang-Tidy, Clang toolchain | Clang static analysis (target: `tidy`) |

#### Test Presets

Test presets are consumed by `ctest --preset <name>` and are paired with their corresponding configure preset. They all share the `base-test` configuration which prints full output on failure. The `ci-tsan-test` preset additionally sets `execution.jobs=1` to prevent concurrent test processes from over-subscribing the CPU.

| Test Preset | Paired Configure Preset |
| :--- | :--- |
| `ci-asan-test` | `ci-asan` |
| `ci-asan-st-test` | `ci-asan-st` |
| `ci-tsan-test` | `ci-tsan` |
| `ci-coverage-test` | `ci-coverage` |
| `ci-coverage-st-test` | `ci-coverage-st` |

#### Workflow Presets

Workflow presets chain configure → build → test into a single command. The CI workflows use these for unit tests:

```bash
cmake --workflow --preset ci-asan        # configure + build + ctest in one step
cmake --workflow --preset ci-asan-st
cmake --workflow --preset ci-coverage
cmake --workflow --preset ci-coverage-st
```

### Preset Design Philosophy

The preset hierarchy is structured around two principles:

1. **Inheritance over repetition.** The hidden `base` preset holds shared settings (Ninja generator, `build/<presetName>/` binary dir, compile-commands export). Sanitizer presets inherit `base`; single-threaded variants inherit their multi-threaded counterparts and flip only `GAMESMAN_ENABLE_OPENMP=OFF`. This keeps the JSON maintainable and prevents configuration drift.

2. **Every CI preset is also locally reproducible.** Because all configurations are expressed as named presets in a checked-in file, any developer can exactly replicate a CI failure locally with a single command (`cmake --workflow --preset ci-asan`), without needing to know which `-D` flags the CI job passed.

The `-st` (single-threaded) variants exist because certain classes of bugs — particularly race conditions in initialization code — are only reliably detected without OpenMP's thread pool active. Running ASan on both threaded and unthreaded builds catches a broader set of issues.

The `release` preset deliberately omits testing targets (`GAMESMAN_ENABLE_TESTING=OFF`) and enables animations to reflect the exact configuration shipped to end users.

### Output Locations

| Preset | Binary location |
| :--- | :--- |
| `release` | `./bin/gamesman` |
| `benchmark` | `./bin/<benchmark_binary>` |
| All other presets | `./build/<preset-name>/src/gamesman` |

The E2E test framework reads the `GAMESMAN_PRESET` environment variable to determine the binary path at runtime (see [`tests/e2e/conftest.py`](../tests/e2e/conftest.py)).

### The `gamesman_setup.sh` Helper Script

[`gamesman_setup.sh`](../gamesman_setup.sh) is a convenience wrapper around the two-step `cmake --preset` / `cmake --build --preset` workflow. It also handles OS-specific dependency installation when the `--install-deps` flag is passed.

```
Usage: gamesman_setup.sh [OPTIONS]

Options:
  --install-deps        Install required OS dependencies (requires sudo/root privileges for package manager).
  --build <preset>      Configure and build the specified CMake preset (assumes build preset is <preset>-build).
  -h, --help            Show this help message and list available CMake presets.
```

> [!IMPORTANT]
> Always run `gamesman_setup.sh` as a **normal user** (not root). The script internally invokes `sudo` only for the package-manager steps. Running as root can leave files with incorrect ownership that break later builds.

The `--install-deps` and `--build` flags are independent and can be combined:

```bash
# Install deps AND build the release preset in one invocation
./gamesman_setup.sh --install-deps --build release
```

After a successful build, the script prints a concise summary of useful follow-up commands (run, clean, wipe cache). When invoked with `--help` or no arguments, it uses `jq` to parse `CMakePresets.json` directly and pretty-print all available presets — so the script output is always in sync with the file.

### CMake Modules (`cmake/`)

Custom CMake modules live in the [`cmake/`](../cmake/) directory and are added to `CMAKE_MODULE_PATH` at the top of the root `CMakeLists.txt`.

| Module | Purpose |
| :--- | :--- |
| `gamesman_dependencies.cmake` | Locates and links all external library dependencies (zlib, OpenMP, MPI, GoogleTest, Google Benchmark). |
| `gamesman_macos_llvm_openmp.cmake` | Detects Homebrew's LLVM toolchain on Apple Silicon and configures compiler and OpenMP paths. Must run before `project()`. |
| `gamesman_lcov.cmake` | Defines CMake targets for generating `lcov`/HTML line coverage reports. |
| `gamesman_static_analyzer.cmake` | Configures GCC `-fanalyzer` or Clang-Tidy flags and the `tidy` build target. |

### Tips for Local Development

- **Use `ccache`**: If installed, CMake detects it automatically and uses it as the C and C++ compiler launcher. This dramatically speeds up incremental rebuilds after branch switches. Install it via your package manager (e.g., `apt install ccache` or `brew install ccache`).
- **Editor integration**: The `CMAKE_EXPORT_COMPILE_COMMANDS=ON` setting (present in all presets via `base`) generates a `compile_commands.json` in the build directory. Point your editor or `clangd` language server at this file for accurate cross-file navigation and diagnostics. Many editors accept a symlink at the project root: `ln -s build/ci-asan/compile_commands.json compile_commands.json`.
- **Incremental reconfiguration**: If you add or remove source files from a `CMakeLists.txt`, re-run only the configure step (`cmake --preset <name>`); Ninja will detect the dependency change on the next build.
- **Wiping a stale cache**: If CMake behavior seems wrong after changing options or switching branches, delete `build/<preset>/CMakeCache.txt` and reconfigure. To start completely fresh, remove the entire `build/<preset>/` directory.

---

## 2. Project Layout

### Top-Level Directory Overview

```
GamesmanOne/
├── .github/workflows/   # GitHub Actions CI definitions
├── benchmarks/          # Google Benchmark microbenchmark targets
├── bin/                 # Compiled release binary (release/benchmark presets only)
├── build/               # Per-preset build trees (gitignored)
├── cmake/               # Custom CMake module scripts
├── config.h.in          # Template for the generated src/config.h
├── CMakeLists.txt       # Root build definition
├── CMakePresets.json    # All named build/test configurations
├── data/                # Solved game databases (gitignored in practice)
├── docs/                # Project documentation (including this file)
├── gamesman_setup.sh    # Helper script for dependency install and builds
├── requirements.txt     # Python dependencies for E2E tests and web service
├── scripts/             # Developer utility scripts
├── server.py            # Lightweight web service / UWAPI relay
├── src/                 # All C source code
└── tests/               # All test code (unit and E2E)
```

Below is a description of each directory relevant to contributors.

| Directory | Description |
| :--- | :--- |
| `.github/workflows/` | GitHub Actions workflow YAML files. See [§ 4 Continuous Integration](#4-continuous-integration). |
| `benchmarks/` | Google Benchmark targets for profiling performance-critical subsystems. Compiled only when `GAMESMAN_ENABLE_BENCHMARKS=ON` (the `benchmark` preset). |
| `bin/` | Output directory for the end-user `gamesman` binary, written only by the `release` and `benchmark` presets. |
| `cmake/` | Project-specific CMake modules included by the root `CMakeLists.txt`. See the [CMake Modules](#cmake-modules-cmake) section above. |
| `docs/` | Project documentation. You are reading a file in this directory. |
| `scripts/` | Developer utility scripts: `cacheline.c` (run at configure time to detect CPU cache-line size), `header_guards.py` (linting helper for include guards), `hooks/` (optional git hooks). |

### Source Tree: `src/`

The `src/` directory is divided into three sub-trees, each targeting a different contributor audience.

```
src/
├── main.c        # Program entry point
├── config.h      # Configuration header automatically generated from config.h.in
├── core/         # Core system: solvers, database, memory, UI engines, etc.
├── games/        # Individual game implementations
└── libs/         # Self-contained utility libraries independent of the system
```

#### `src/core/` — Core System

> For core system developers. See [src/core/README.md](../src/core/README.md) *(coming soon)*.

This directory contains the engine that makes game solving, storing, and querying possible.

#### `src/games/` — Game Implementations

> For game developers. See [src/games/README.md](../src/games/README.md).

Each subdirectory under `src/games/` contains the implementation of a single game (or a family of closely related variants). 

The central files [`game_list.h`](../src/games/game_list.h) and [`game_list.c`](../src/games/game_list.c) serve as the game registry — every implemented game must be registered here to be accessible via the CLI. The game developer's guide (linked above, coming soon) covers the full interface a game implementation must provide.

#### `src/libs/` — Utility Libraries

> For core system developers. See [src/libs/README.md](../src/libs/README.md) *(coming soon)*.

`src/libs/` contains self-contained utility libraries that have no dependencies on the core system or on any game. They can be developed, tested, and reasoned about in isolation.

### Tests: `tests/`

```
tests/
├── unit/       # GoogleTest unit tests (mirrors src/ structure)
└── e2e/        # End-to-End Python/pytest tests
```

See [§ 3 Testing](#3-testing) for a full explanation of the test architecture.

### Other Notable Directories

| Directory | Description |
| :--- | :--- |
| `data/` | Runtime game database storage (game results, `.finish` sentinels, `.stat` analysis files). Created automatically when solving; not tracked in git. |
| `benchmarks/` | Google Benchmark source files for core-system microbenchmarks. Only built with the `benchmark` preset. |

---

## 3. Testing

GamesmanOne maintains two categories of automated tests that run in CI: **unit tests** and **end-to-end (E2E) tests**. They target different layers of the system and are written in different languages.

### Unit Tests

Unit tests exercise individual translation units (`.c`/`.h` files) or header-only libraries in isolation. They are written in **C++ using GoogleTest** and live under [`tests/unit/`](../tests/unit/), mirroring the source tree structure.

**Scope and requirements:**

- Unit testing is **required for all core system code** (`src/core/` and `src/libs/`).
- Game implementations in `src/games/` are **not required to have unit tests**. Game logic correctness is instead verified by the solver-driven game implementation tests (see the game tests described under E2E below). Refer to the game developer's guide for details on this approach.

Unit tests are compiled into separate test executables and registered with CTest. They are built whenever `GAMESMAN_ENABLE_TESTING=ON` (the default for all presets except `release`).

Running unit tests with CTest directly:
```bash
# Using a workflow preset (configure + build + test in one step)
cmake --workflow --preset ci-asan

# Or step by step with an existing build:
ctest --preset ci-asan-test
```

### End-to-End (E2E) Tests

E2E tests verify the behavior of the complete compiled `gamesman` binary, exercising the full pipeline from command invocation to output. They are written in **Python using pytest** and live under [`tests/e2e/`](../tests/e2e/).

The E2E test suite is divided into three categories:

#### 1. Headless Core System Tests (`test_headless.py`)

These tests drive the `gamesman` binary through a full lifecycle for a set of small, quickly solvable games:

- **Phase 1 — Solve & hash verification**: Runs `gamesman solve <game> <variant>` and SHA-256 hashes all generated compressed database archives. The sorted list of hashes is compared against a [syrupy](https://github.com/syrupy-project/syrupy) snapshot, ensuring the solver produces **bitwise-identical output** across runs and platforms.
- **Phase 2 — Analysis verification**: Runs `gamesman analyze <game> <variant>` and compares the printed statistics table against a snapshot (with non-deterministic fields like position indices masked by regex).
- **Phase 3 — Graph traversal (AutoGUI games only)**: Runs a pseudorandom walk through the game graph using `gamesman getstart` and repeated `gamesman query` calls with a fixed seed (`42`), then validates the entire JSON walk history against a snapshot.

The game list for these tests is defined in [`tests/e2e/games.py`](../tests/e2e/games.py) (`HEADLESS_GAMES`). Games are selected to be small enough to solve within CI time limits.

The binary path is resolved automatically by the shared `gamesman_config` fixture in [`conftest.py`](../tests/e2e/conftest.py) based on the `GAMESMAN_PRESET` environment variable (defaults to `release`).

#### 2. Game Implementation Tests (E2E Game Tests)

The `gamesman` binary includes a built-in `test` subcommand that the solver uses to **randomly verify the correctness of a game's own implementation** (position generation, move legality, hash/unhash consistency, etc.):

```bash
./build/ci-asan/src/gamesman test <game> <variant> --seed=<N>
```

A non-zero exit code indicates a detected inconsistency. These tests do not require a pre-solved database and exercise game logic directly, making them the primary correctness check for game implementations.

The full list of tested game/variant combinations is maintained in [`tests/e2e/games.py`](../tests/e2e/games.py) as `GAME_TESTS_PRESUBMIT` (run on every PR) and `GAME_TESTS_POSTSUBMIT` (extended set, run after merging). The CI matrix in the workflow YAML files is kept in sync with this Python file — see comments in both files.

#### 3. Interactive TextUI Tests (`test_interactive.py`)

These tests simulate a human user interacting with the `gamesman` TextUI over a pseudo-terminal (PTY), using the [`pexpect`](https://pexpect.readthedocs.io/) library. They verify that:

- Game selection menus render and respond correctly.
- Variant configuration options navigate as expected.
- The interactive game loop starts without errors.

Interactive tests run only on Ubuntu in CI (macOS is excluded due to terminal race conditions in the CI environment) and only against the `ci-asan` preset (not `release`, because the release build enables animated terminal output that is incompatible with `pexpect` pattern matching).

### Running Tests Locally

**Unit tests** (using a workflow preset):
```bash
cmake --workflow --preset ci-asan         # multi-threaded
cmake --workflow --preset ci-asan-st      # single-threaded
cmake --workflow --preset ci-coverage     # with coverage instrumentation
```

**E2E headless tests** (requires a Python virtual environment; see README):
```bash
# Build first:
cmake --preset ci-asan && cmake --build --preset ci-asan-build

# Run core system E2E tests:
GAMESMAN_PRESET=ci-asan pytest tests/e2e/test_headless.py -v

# Update snapshots after an intentional output change:
GAMESMAN_PRESET=ci-asan pytest tests/e2e/test_headless.py --snapshot-update
```

**E2E game implementation tests**:
```bash
./build/ci-asan/src/gamesman test mttt 0 --seed=42
./build/ci-asan/src/gamesman test mills 18 --seed=42
```

**E2E interactive tests** (Linux only):
```bash
GAMESMAN_PRESET=ci-asan pytest tests/e2e/test_interactive.py -v
```

---

## 4. Continuous Integration

GamesmanOne uses GitHub Actions with three workflow files under [`.github/workflows/`](../.github/workflows/). The CI matrix runs on both **`ubuntu-latest`** and **`macos-latest`** runners (except where noted), providing cross-platform validation on every change.

### `presubmit.yml` — PR and Branch Validation

**Triggers**: Pushes to `main` or `dev`, and all pull requests targeting those branches. Also supports manual dispatch.

This is the primary gating workflow. All jobs must pass before a PR can be merged. It runs four parallel job groups:

#### Job 1: Unit Tests (`unit-test`)

Runs the full GoogleTest suite using the CMake workflow presets. The matrix covers:

- **Platforms**: `ubuntu-latest`, `macos-latest`
- **Presets**: `ci-asan`, `ci-asan-st`, `ci-coverage`, `ci-coverage-st`

This produces 8 parallel jobs total (2 OS × 4 presets), each using `cmake --workflow --preset <preset>` to configure, build, and run all unit tests in one step.

#### Job 2: E2E Game Implementation Tests (`e2e-game-test`)

Runs the solver's built-in `test` subcommand for every game/variant pair in `GAME_TESTS_PRESUBMIT`. The matrix is **fanned out per game/variant** to maximize parallelism and to provide granular failure attribution.

- **Platforms**: `ubuntu-latest`, `macos-latest`
- **Build**: `ci-asan` preset
- **Seed**: fixed at `42` for determinism

> [!IMPORTANT]
> The game/variant matrix in `presubmit.yml` must be kept in sync with `GAME_TESTS_PRESUBMIT` in [`tests/e2e/games.py`](../tests/e2e/games.py). When adding or removing a game's test coverage, update **both** files.

#### Job 3: E2E Core System Tests (`e2e-core-system-test`)

Runs `pytest tests/e2e/test_headless.py` for a subset of small games, testing the full solve → analyze → query pipeline and validating output against syrupy snapshots.

- **Platforms**: `ubuntu-latest`, `macos-latest`
- **Presets**: `ci-asan` and `release` (verifies that the optimized release binary also produces correct output)
- **Games**: the subset defined as `HEADLESS_GAMES` in `games.py`

The job installs `pytest` and `syrupy` from PyPI (no virtual environment caching in CI — straightforward `pip install`).

#### Job 4: E2E Interactive Tests (`e2e-interactive-test`)

Runs `pytest tests/e2e/test_interactive.py` using `pexpect` to exercise the TextUI.

- **Platform**: `ubuntu-latest` only (macOS excluded due to terminal race conditions)
- **Preset**: `ci-asan` only (not `release` — animations break `pexpect` matching)

---

### `postsubmit.yml` — Post-Merge Extended Coverage

**Triggers**: Pushes to `main` or `dev` only (does **not** run on PRs). Also supports manual dispatch.

This workflow runs a second, larger batch of game implementation tests covering variants that are too slow or too large for presubmit CI. These are the **additional** entries in `GAME_TESTS_POSTSUBMIT` that are not already in `GAME_TESTS_PRESUBMIT`.

- **Platforms**: `ubuntu-latest`, `macos-latest`
- **Build**: `ci-asan` preset
- **Seed**: fixed at `42`

Currently this covers additional variants of Mills (Nine Men's Morris), Quixo, Neutron, and Gates.

> [!NOTE]
> Because presubmit already validated the fast variants on the same commit, postsubmit does not re-run them — it runs *only* the additional slower variants. The Python source of truth (`GAME_TESTS_POSTSUBMIT`) contains the full union for local reference.

---

### `codeql.yml` — Security and Quality Analysis

**Triggers**: Pushes to `main` or `dev`, pull requests targeting those branches, and a **weekly scheduled run** every Saturday at 20:00 UTC.

Uses GitHub's [CodeQL](https://codeql.github.com/) engine to perform deep static security analysis on two language targets simultaneously:

| Language | Build Mode | Queries |
| :--- | :--- | :--- |
| `c-cpp` | Manual (builds with `release` preset) | `security-extended`, `security-and-quality` |
| `python` | Automatic (no build needed) | `security-extended`, `security-and-quality` |

For the C/C++ analysis, the workflow installs and configures `ccache` with a commit-keyed cache to speed up repeated analyses. The cache key is derived from `github.sha` so each commit saves a fresh snapshot, while restoring from the most recent previous entry via `restore-keys`.

The `security-extended` and `security-and-quality` query suites are broader than CodeQL's defaults, enabling detection of additional CWE categories including buffer handling, integer overflow, and unsafe API usage patterns common in C codebases.

---

### Maintaining the CI Matrix

When making changes that affect the test suite, keep the following synchronization invariants:

1. **Game implementation test matrix**: `GAME_TESTS_PRESUBMIT` in [`tests/e2e/games.py`](../tests/e2e/games.py) ↔ `e2e-game-test.matrix.test_case` in [`presubmit.yml`](../.github/workflows/presubmit.yml).
2. **Postsubmit additional variants**: `GAME_TESTS_POSTSUBMIT` (excluding `GAME_TESTS_PRESUBMIT`) in `games.py` ↔ `e2e-game-test-postsubmit.matrix.test_case` in [`postsubmit.yml`](../.github/workflows/postsubmit.yml).
3. **Headless E2E test games**: `HEADLESS_GAMES` in `games.py` is the canonical list for `test_headless.py`; it does not need to be mirrored in the workflow files because that job runs the full list as a single pytest call.
4. **Snapshots**: After any intentional change to solver output, analysis output, or game graph query responses, regenerate the syrupy snapshots with `--snapshot-update` and commit the updated `tests/e2e/__snapshots__/` files alongside the code change.
