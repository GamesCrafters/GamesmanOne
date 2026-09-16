# GamesmanOne: The Finite, Two-Person Perfect-Information Game Generator

GamesmanOne is a highly efficient, parallel two-player abstract strategy game generator and strong solver developed by the [GamesCrafters Research Group](https://gamescrafters.berkeley.edu/) at UC Berkeley (supervised by Teaching Professor [Dan Garcia](https://people.eecs.berkeley.edu/~ddgarcia/)). 

The project was inspired by and based heavily on the [**GamesmanClassic**](https://github.com/GamesCrafters/GamesmanClassic) project. Parallelized using **OpenMP** multithreading for shared-memory parallelism and **MPI** for distributed-memory computing, GamesmanOne is built to provide an extensible, modular platform capable of strongly solving and analyzing combinatorial games with state spaces on the **trillion-position scale**.

---

## Table of Contents

- [Overview](#overview)
  - [Goals & Core Capabilities](#goals--core-capabilities)
  - [Architecture & Solvers](#architecture--solvers)
  - [Supported Games](#supported-games)
- [Developer Setup Guide](#developer-setup-guide)
  - [Prerequisites & System Dependencies](#prerequisites--system-dependencies)
    - [Automated Setup (Linux & macOS)](#automated-setup-linux--macos)
    - [Manual Dependency Installation](#manual-dependency-installation)
  - [Python Environment](#python-environment)
  - [Building from Source](#building-from-source)
    - [Available CMake Presets](#available-cmake-presets)
  - [Running GamesmanOne](#running-gamesmanone)
    - [Interactive Mode](#interactive-mode)
    - [Headless CLI Mode](#headless-cli-mode)
    - [Web Service Interface](#web-service-interface)
  - [Running Tests](#running-tests)
    - [Unit Tests](#unit-tests)
    - [End-to-End (E2E) Tests](#end-to-end-e2e-tests)
- [Links & Documentation](#links--documentation)
- [License](#license)

---

## Overview

### Goals & Core Capabilities

In game theory, **strongly solving** a game means determining the exact game-theoretic outcome (**Win**, **Lose**, **Tie**, or **Draw**) and the **remoteness** (the number of plies to the optimal outcome under perfect play) for every legal, reachable board position.

GamesmanOne provides:
- **Exhaustive Retrograde Analysis**: Solves both loop-free (DAG) state spaces and loopy games containing cycles and draw conditions.
- **Extreme Scale**: Optimizations including bit-packed atomic transposition table, SIMD hardware acceleration, and custom random-access compression algorithms designed for massive game graphs.
- **HPC Cluster Integration**: Distributed solving support via MPI and automated SLURM job script generation for supercomputing environments such as UC Berkeley's Savio cluster.
- **Compressed On-Disk Database**: Employs **XZRA** (*XZ Random Access*), a custom block-indexed XZ/LZMA compression format allowing multi-gigabyte solved databases to remain compressed on disk while supporting fast arbitrary position queries.
- **Web Frontend Integration**: Connects seamlessly to [GamesCraftersUWAPI](https://github.com/GamesCrafters/GamesCraftersUWAPI) (Universal Web API), which relays position evaluations and game graph data to [GamesmanUni](https://github.com/GamesCrafters/GamesmanUni), the universal web frontend.

### Architecture & Solvers

GamesmanOne features two primary solver engines:
1. **Tier Solver** (`kTierSolver`): Stratified solver that partitions large, structured games into a directed acyclic graph (DAG) of "tiers" (e.g., by piece count, phase, or board occupancy), solving layers backwards and handling intra-tier loops and inter-tier transitions.
2. **Regular Solver** (`kRegularSolver`): Retrograde solver for single-tier games.

### Supported Games

The engine currently includes implementations for the following 13 games:

| Name | CLI Identifier | Solver | AutoGUI Support |
| :--- | :--- | :--- | :---: |
| All Queens Chess | `mallqueenschess` | Regular | Yes |
| Dobutsu Shogi | `dshogi` | Regular | Yes |
| Fair Shares and Varied Pairs | `fsvp` | Regular | No |
| Gates | `gates` | Tier | No |
| Gobblet Gobblers | `gobbletg` | Tier | Yes |
| Kaooa | `mkaooa` | Regular | Yes |
| Mills (Nine Men's Morris & variants) | `mills` | Tier | Yes |
| Neutron | `neutron` | Regular | Yes |
| Quixo | `quixo` | Tier | Yes |
| Teeko | `teeko` | Regular | Yes |
| Tic-Tac-Toe Tier | `mtttier` | Tier | Yes |
| Tic-Tac-Toe | `mttt` | Regular | No |
| Winkers | `winkers` | Tier | Yes |

---

## Setup Guide

> [!NOTE]
> GamesmanOne is intended to be **built from source**. We do not distribute pre-compiled release binaries.

### Prerequisites & System Dependencies

GamesmanOne requires:
- **C Compiler**: Clang (recommended) or GCC supporting C11 / C17.
- **C++ Compiler**: Clang++ or G++ supporting C++17 (for GoogleTest test harnesses).
- **CMake**: Version 3.25 or later.
- **Build System**: [Ninja](https://ninja-build.org/) (required by CMake presets).
- **Compression & System Libraries**: `zlib`, `libomp` (OpenMP runtime), `openmpi` (optional, for MPI builds), `jq`.
- **Tools**: `ccache` (recommended), `lcov` (for code coverage).

#### Automated Setup (Linux & macOS)

A setup script is provided to install all necessary system packages automatically:

```bash
chmod +x gamesman_setup.sh
sudo ./gamesman_setup.sh --install-deps
```

> [!IMPORTANT]
> Running `./gamesman_setup.sh --install-deps` requires `sudo` (or administrator privileges) to invoke the system package manager.

#### Manual Dependency Installation

If you prefer to install dependencies manually:

- **Ubuntu / Debian**:
  ```bash
  sudo apt update
  sudo apt install -y cmake ninja-build clang clang-tidy libomp-dev openmpi-bin libopenmpi-dev zlib1g zlib1g-dev jq ccache lcov
  ```

- **Fedora / RHEL**:
  ```bash
  sudo dnf update
  sudo dnf install -y cmake ninja-build clang clang-tools-extra libomp-devel openmpi openmpi-devel zlib zlib-devel jq ccache lcov
  ```

- **Apple Silicon (M-Chip) macOS**:
  Install dependencies via [Homebrew](https://brew.sh/):
  ```bash
  brew install cmake ninja llvm libomp open-mpi zlib jq ccache lcov
  ```
  *(CMake automatically detects Homebrew's LLVM toolchain and OpenMP headers at `/opt/homebrew`.)*

### Python Environment

End-to-End (E2E) testing and web services require Python 3.11+. Install the Python dependencies:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt pytest
```

### Building from Source

GamesmanOne uses **CMake Presets** (`CMakePresets.json`) with Ninja. To build the project:

```bash
# 1. Configure the build
cmake --preset release

# 2. Compile the executable
cmake --build --preset release-build
```

The compiled binary will be placed at `./bin/gamesman`.

> [!NOTE]
> Binaries are placed under `./bin/` only when compiled using the `release` or `benchmark` presets. For all other presets, the resulting binaries are located under `./build/<preset>/src/` (e.g., `./build/ci-asan/src/gamesman`). For more details, refer to the [Developer's Guide](docs/developers_guide.md).

#### Available CMake Presets

| Configure Preset | Build Preset | Description |
| :--- | :--- | :--- |
| `release` | `release-build` | Optimized build with animations enabled. Outputs to `./bin/gamesman`. |
| `ci-asan` | `ci-asan-build` | `RelWithDebInfo` + AddressSanitizer (ASan) & UndefinedBehaviorSanitizer (UBSan). |
| `ci-asan-st` | `ci-asan-st-build` | Single-threaded build with ASan and UBSan (OpenMP disabled). |
| `ci-coverage` | `ci-coverage-build` | `Debug` build with line coverage instrumentation (`--coverage`). |
| `ci-coverage-st` | `ci-coverage-st-build` | Single-threaded `Debug` build with line coverage. |
| `benchmark` | `benchmark-build` | Release build targeting Google Benchmark executables (`gamesman_benchmark`). |

You can also use the helper script to configure and build presets in one step:
```bash
./gamesman_setup.sh --build release
```

---

## Running GamesmanOne

### Interactive Mode

To launch the interactive text-based terminal interface, run the binary without arguments:

```bash
./bin/gamesman
```

From the interactive menu, you can select games, configure rule variants, solve boards in memory, view analytical distributions, or play against human/computer opponents.

### Headless CLI Mode

GamesmanOne provides subcommands for automated batch execution, scripting, and database queries.

#### Subcommand Reference

- `solve <game> <variant_id> [--data-path=PATH] [-M MEM_LIMIT_GIB] [-f]`: Solves a game variant and saves the compressed database records.
- `analyze <game> <variant_id> [--data-path=PATH]`: Analyzes solved game databases and prints outcome distributions, remoteness tables, and longest paths.
- `getstart <game> <variant_id>`: Returns the initial game state in JSON format (supported on games with AutoGUI enabled).
- `query <game> <variant_id> <position_string>`: Evaluates a board position of a solved game in JSON format, returning its game-theoretic value, remoteness, and legal next moves.
- `test <game> <variant_id> [--seed=SEED]`: Executes built-in game module verification tests.

#### Executable Examples

- **Solve a game variant** (e.g., Quixo variant 2):
  ```bash
  ./bin/gamesman solve quixo 2
  ```

- **Analyze statistics of a solved game** (e.g., Tic-Tac-Toe):
  ```bash
  ./bin/gamesman solve mttt 0
  ./bin/gamesman analyze mttt 0
  ```

- **Retrieve the initial game position in JSON** (e.g., Kaooa):
  ```bash
  ./bin/gamesman getstart mkaooa 0
  ```

- **Query a board position evaluation of a solved game**:
  ```bash
  ./bin/gamesman solve mkaooa 0
  ./bin/gamesman query mkaooa 0 "1_----------0"
  ```

- **Run built-in game implementation tests**:
  ```bash
  ./bin/gamesman test mttt 0 --seed=42
  ```

### Web Service Interface

To run the lightweight REST API microservice for [GamesCraftersUWAPI](https://github.com/GamesCrafters/GamesCraftersUWAPI) and [GamesmanUni](https://github.com/GamesCrafters/GamesmanUni):

```bash
python3 server.py
```
This serves endpoints at `http://localhost:8084/<game_name>/<variant_id>/` and `http://localhost:8084/<game_name>/<variant_id>/<position>`.

---

## Running Tests

### Unit Tests

Unit tests are written with **GoogleTest** and integrated into CTest and CMake workflow presets.

```bash
# Run unit tests via CMake workflow preset
cmake --workflow --preset ci-asan

# Or run via CTest directly
ctest --preset ci-asan-test
```

### End-to-End (E2E) Tests

GamesmanOne contains a comprehensive Python E2E test suite built with `pytest`, `pexpect`, and `syrupy` snapshot assertion testing:

- **Headless Core System Tests** (Solve $\rightarrow$ Hash Verification $\rightarrow$ Analyze $\rightarrow$ Graph Traversal):
  ```bash
  GAMESMAN_PRESET=ci-asan pytest tests/e2e/test_headless.py -v
  ```

- **Interactive Gameplay Tests** (Simulates terminal TTY interactions):
  ```bash
  GAMESMAN_PRESET=ci-asan pytest tests/e2e/test_interactive.py -v
  ```

- **Game Implementation Presubmit Tests**:
  ```bash
  ./build/ci-asan/src/gamesman test mttt 0 --seed=42
  ```

---

## Links & Documentation

Additional documentation and developer guides:
- [Developer's Guide](docs/developers_guide.md)
- [Doxygen & Code Documentation Conventions](docs/doxygen_conventions.md)
- [Unit Testing Conventions](docs/unit_testing_conventions.md)
- [Style Guide & Code Conventions](docs/style_guide.md)
- [VS Code Setup Guide](docs/vscode_setup.md)

---

## License

GamesmanOne is open-source software released under the [GNU General Public License v3.0 (GPLv3)](COPYING).
