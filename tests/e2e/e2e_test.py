"""GamesmanOne E2E Test Script
Script that tests end-to-end solving, analyzing, and (optionally) game graph
traversing on a list of small games.

A random data path shall be used each time the script is run and it shall not
overwrite the default data path in the project directory.

The tests shall capture the terminal output and compare it with a previously
captured snapshot to find discrepancies. Failure shall be reported if any of
the test steps fails to execute or the terminal output is different from the
previous snapshot.

Graph traversal is optional and is only tested when the game implements AutoGUI.
The test shall be done using only the provided "gamesman getstart" and "gamesman
query" CLI commands following a pseudorandom path generated with a fixed seed.

Note:
    TSan does not play well with OpenMP and may report false positives. As of
    2026, TSan should not be required in E2E test for CI. However, it is
    recommended to run TSan E2E test locally from time to time and manually
    check for multithreading issues. Known false positives include "SEGV on
    unknown address" issue that happens when the program runs exit handlers,
    and data race warnings on omp_aligned_alloc'ed spaces.

Usage in root project directory:
    # Run tests
    GAMESMAN_PRESET=<cmake_preset> pytest tests/e2e/e2e_test.py -v

    # Run tests and update baseline snapshot
    GAMESMAN_PRESET=<cmake_preset> pytest tests/e2e/e2e_test.py --snapshot-update
"""

import os
from pathlib import Path
from syrupy.assertion import SnapshotAssertion
import hashlib
import json
import pytest
import random
import re
import subprocess

# 1. Test matrix [(<game0>, <variant_id0>, <autogui0>), ...]
GAMES_TO_TEST: list[tuple[str, int, bool]] = [
    ("fsvp", 10, False),  # Regular
    ("mttt", 0, False),  # Regular
    ("mkaooa", 0, True),  # Regular
    ("mills", 72, True),  # Tier
    ("mtttier", 0, True),  # Tier
    ("quixo", 2, True),  # Tier
]


@pytest.fixture(scope="session")
def gamesman_config() -> tuple[str, dict[str, str]]:
    """
    Determines the binary path and environment variables based on the GAMESMAN_PRESET.
    Defaults to release if the environment variable is not set.
    """
    preset = os.environ.get("GAMESMAN_PRESET", "release")

    # 1. Determine binary path
    if preset == "release":
        bin_path = "./bin/gamesman"
    else:
        bin_path = f"./build/{preset}/src/gamesman"

    # 2. Configure environment variables (e.g., TSAN_OPTIONS)
    env = os.environ.copy()
    if "tsan" in preset.lower():
        existing_tsan = env.get("TSAN_OPTIONS", "")
        # Prepend ignore_noninstrumented_modules=1 to prevent OpenMP false positives
        env["TSAN_OPTIONS"] = (
            f"ignore_noninstrumented_modules=1 {existing_tsan}".strip()
        )

    return bin_path, env


@pytest.fixture
def sandbox_dir(tmp_path: Path) -> Path:
    """
    Creates a temporary, isolated sandbox directory for each test run.
    This guarantees a clean state and eliminates the need for the -f flag.
    """
    return tmp_path / "data"


@pytest.mark.parametrize("game,variant_id,autogui", GAMES_TO_TEST)
def test_gamesman_e2e(
    game: str,
    variant_id: int,
    autogui: bool,
    sandbox_dir: Path,
    snapshot: SnapshotAssertion,
    gamesman_config: tuple[str, dict[str, str]],
) -> None:
    """
    End-to-End test for a specific game and variant.
    Uses 'snapshot' (from syrupy) to validate output against known good baselines.
    """
    bin_path, run_env = gamesman_config
    variant = str(variant_id)

    # ==========================================
    # PHASE 1: SOLVE & HASH VERIFICATION
    # ==========================================

    solve_cmd = [
        bin_path,
        "solve",
        game,
        variant,
        f"--data-path={sandbox_dir}",
    ]
    # Inject the environment variables into the subprocess
    subprocess.run(solve_cmd, env=run_env, check=True, capture_output=True, text=True)

    # Locate the variant directory in the sandbox
    variant_dir = sandbox_dir / game / variant
    assert variant_dir.exists(), "Variant directory was not created."

    # Because db_type is implementation-determined, we find it by looking for .finish
    finish_files = list(variant_dir.rglob(".finish"))
    assert len(finish_files) == 1, (
        f"Expected exactly one .finish file, found {len(finish_files)}"
    )

    db_dir = finish_files[0].parent

    # Gather and hash all compressed archives (ignoring the .finish file)
    archive_hashes = []
    archive_count = 0
    for file_path in db_dir.iterdir():
        if file_path.is_file() and file_path.name != ".finish":
            archive_count += 1
            file_hash = hashlib.sha256(file_path.read_bytes()).hexdigest()
            archive_hashes.append(file_hash)

    assert archive_count > 0, "No database archives were generated."

    # Sort hashes to ensure deterministic order regardless of OS file listing
    archive_hashes.sort()

    # Validate the hashes against the snapshot
    assert archive_hashes == snapshot(name="db_hashes")

    # ==========================================
    # PHASE 2: ANALYZE VERIFICATION
    # ==========================================

    analyze_cmd = [
        bin_path,
        "analyze",
        game,
        variant,
        f"--data-path={sandbox_dir}",
    ]
    analyze_result = subprocess.run(
        analyze_cmd, env=run_env, check=True, capture_output=True, text=True
    )

    # Sanitize the output to filter out non-deterministic output
    sanitized_stdout = analyze_result.stdout
    # 1. Mask position and tier for the largest available moves
    sanitized_stdout = re.sub(
        r"(Position )\d+( in tier )\d+( has the largest number of available moves:)",
        r"\g<1><POS>\g<2><TIER>\g<3>",
        sanitized_stdout,
    )
    # 2. Mask position and tier for longest win/lose/tie
    sanitized_stdout = re.sub(
        r"(One longest (?:win|lose|tie) starts from position )\d+( in tier )\d+(, which has remoteness)",
        r"\g<1><POS>\g<2><TIER>\g<3>",
        sanitized_stdout,
    )

    # Validate the analysis stdout table against the snapshot
    assert sanitized_stdout == snapshot(name="analyze_stdout")

    # Verify the number of .stat files matches the number of database archives
    analysis_dir = variant_dir / "analysis"
    assert analysis_dir.exists(), "Analysis directory was not created."

    stat_files = list(analysis_dir.glob("*.stat"))
    assert len(stat_files) == archive_count, (
        f"Mismatch: {len(stat_files)} .stat files vs {archive_count} archives."
    )

    # ==========================================
    # PHASE 3: GRAPH TRAVERSAL (RANDOM WALK)
    # ==========================================

    # Skip this phase if AutoGUI is not available
    if not autogui:
        return

    # Get initial position
    start_cmd = [
        bin_path,
        "getstart",
        game,
        variant,
    ]
    start_result = subprocess.run(
        start_cmd, env=run_env, check=True, capture_output=True, text=True
    )
    start_data = json.loads(start_result.stdout.strip())

    current_pos = start_data["position"]
    walk_history = []

    # Seed random for a deterministic walk
    random.seed(42)
    MAX_DEPTH = 100  # Prevent infinite loops in cyclic games

    for _ in range(MAX_DEPTH):
        query_cmd = [
            bin_path,
            f"--data-path={sandbox_dir}",
            "--",
            "query",
            game,
            variant,
            current_pos,
        ]
        query_result = subprocess.run(
            query_cmd, env=run_env, check=True, capture_output=True, text=True
        )
        query_data = json.loads(query_result.stdout.strip())

        walk_history.append(query_data)

        moves = query_data.get("moves", [])
        if not moves:
            break  # Reached a terminal state

        # Sort moves by the 'move' string to guarantee identical behavior
        moves.sort(key=lambda m: m["move"])

        next_move = random.choice(moves)
        current_pos = next_move["position"]

    # Validate the entire JSON walk path against the snapshot
    assert walk_history == snapshot(name="random_walk")
