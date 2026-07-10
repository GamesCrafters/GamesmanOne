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


@pytest.fixture
def sandbox_dir(tmp_path: Path) -> Path:
    """
    Creates a temporary, isolated sandbox directory for each test run.
    This guarantees a clean state and eliminates the need for the -f flag.
    """
    # tmp_path is a built-in pytest fixture providing a unique temp pathlib.Path
    return tmp_path / "data"


@pytest.mark.parametrize("game,variant_id,autogui", GAMES_TO_TEST)
def test_gamesman_e2e(
    game: str,
    variant_id: int,
    autogui: bool,
    sandbox_dir: Path,
    snapshot: SnapshotAssertion,
) -> None:
    """
    End-to-End test for a specific game and variant.
    Uses 'snapshot' (from syrupy) to validate output against known good baselines.
    """
    bin_path = "./bin/gamesman"
    variant = str(variant_id)

    # ==========================================
    # PHASE 1: SOLVE & HASH VERIFICATION
    # ==========================================

    solve_cmd = [bin_path, "solve", game, variant, f"--data-path={sandbox_dir}"]
    subprocess.run(solve_cmd, check=True, capture_output=True, text=True)

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

    analyze_cmd = [bin_path, "analyze", game, variant, f"--data-path={sandbox_dir}"]
    analyze_result = subprocess.run(
        analyze_cmd, check=True, capture_output=True, text=True
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
    # The (?:win|lose|tie) acts as an "OR" match without creating an extra capture group
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
    start_cmd = [bin_path, "getstart", game, variant]
    start_result = subprocess.run(start_cmd, check=True, capture_output=True, text=True)
    start_data = json.loads(start_result.stdout.strip())

    current_pos = start_data["position"]
    walk_history = []

    # Seed random for a deterministic walk
    random.seed(42)
    MAX_DEPTH = 100  # Prevent infinite loops in cyclic games

    for _ in range(MAX_DEPTH):
        query_cmd = [bin_path, "query", "--", game, variant, current_pos]
        query_result = subprocess.run(
            query_cmd, check=True, capture_output=True, text=True
        )
        query_data = json.loads(query_result.stdout.strip())

        walk_history.append(query_data)

        moves = query_data.get("moves", [])
        if not moves:
            break  # Reached a terminal state

        # Sort moves by the 'move' string to guarantee identical behavior
        # even if the C-solver iterates through a hash map and alters JSON order.
        moves.sort(key=lambda m: m["move"])

        next_move = random.choice(moves)
        current_pos = next_move["position"]

    # Validate the entire JSON walk path against the snapshot
    assert walk_history == snapshot(name="random_walk")
