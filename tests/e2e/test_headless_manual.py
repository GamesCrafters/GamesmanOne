"""GamesmanOne Manual E2E Test Script
Script that tests game graph traversing on a list of large, pre-solved games.

The data path is hardcoded to "data" in the project root directory, where the
games are assumed to be already solved and stored. Solving and analyzing are
skipped as they take too long for these games.

Graph traversal is tested only when the game implements AutoGUI.
The test shall be done using only the provided "gamesman getstart" and "gamesman
query" CLI commands following a pseudorandom path generated with a fixed seed.

Usage in root project directory:
    # Run tests
    GAMESMAN_PRESET=<cmake_preset> pytest tests/e2e/test_headless_manual.py -v

    # Run tests and update baseline snapshot
    GAMESMAN_PRESET=<cmake_preset> pytest tests/e2e/test_headless_manual.py --snapshot-update
"""

import json
import os
import pytest
import random
import subprocess
from syrupy.assertion import SnapshotAssertion

# 1. Test matrix [(<game0>, <variant_id0>, <autogui0>), ...]
# Note: You can replace this list with the larger games you want to test manually.
GAMES_TO_TEST: list[tuple[str, int, bool]] = [
    ("mallqueenschess", 0, False),  # Regular
    ("dshogi", 0, True),  # Regular
    ("gates", 0, False),  # Tier
    ("gobbletg", 0, True),  # Tier
    ("mills", 0, True),  # Tier
    ("mills", 72, True),  # Tier
    ("mills", 144, True),  # Tier
    ("mills", 234, True),  # Tier
    ("mills", 236, True),  # Tier
    ("mills", 312, True),  # Tier
    ("mills", 314, True),  # Tier
    ("mills", 318, True),  # Tier
    ("mills", 320, True),  # Tier
    ("mills", 396, True),  # Tier
    ("mills", 450, True),  # Tier
    ("neutron", 0, True),  # Regular
    ("quixo", 0, True),  # Tier
    ("teeko", 0, True),  # Regular
    ("teeko", 1, True),  # Regular
    ("winkers", 0, True),  # Tier
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


@pytest.mark.parametrize("game,variant_id,autogui", GAMES_TO_TEST)
def test_gamesman_manual_e2e(
    game: str,
    variant_id: int,
    autogui: bool,
    snapshot: SnapshotAssertion,
    gamesman_config: tuple[str, dict[str, str]],
) -> None:
    """
    End-to-End manual test for a specific pre-solved game and variant.
    Uses 'snapshot' (from syrupy) to validate graph traversal against known good baselines.
    """
    bin_path, run_env = gamesman_config
    variant = str(variant_id)

    # Hardcoded data path for pre-solved games
    data_path = "data"

    # Skip this test if AutoGUI is not available
    if not autogui:
        pytest.skip(f"AutoGUI not enabled for {game} variant {variant}")
        return

    # ==========================================
    # GRAPH TRAVERSAL (RANDOM WALK)
    # ==========================================

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
            f"--data-path={data_path}",
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
