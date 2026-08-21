"""Gamesman Interactive E2E Test Script
Tests the interactive CLI mode of gamesman using pexpect to simulate user input.
Terminal output is captured and verified against a syrupy baseline snapshot.

Usage in root project directory:
    # Run interactive tests
    GAMESMAN_PRESET=<cmake_preset> pytest tests/e2e/test_interactive.py -v

    # Run tests and update baseline snapshot
    GAMESMAN_PRESET=<cmake_preset> pytest tests/e2e/test_interactive.py --snapshot-update
"""

import io
import os
import random
import re
import pexpect
import pytest
from syrupy.assertion import SnapshotAssertion


class Game:
    ALL_QUEENS_CHESS = "all_queens_chess"
    DOBUTSU_SHOGI = "dobutsu_shogi"
    FAIR_SHARES_AND_VARIED_PAIRS = "fair_shares_and_varied_pairs"
    GATES = "gates"
    GOBBLET_GOBBLERS = "gobblet_gobblers"
    KAOOA = "kaooa"
    MILLS = "mills"
    NEUTRON = "neutron"
    QUIXO = "quixo"
    TEEKO = "teeko"
    TTTIER = "tttier"
    TTT = "ttt"
    WINKERS = "winkers"


GAMES: list[str] = [
    Game.ALL_QUEENS_CHESS,
    Game.DOBUTSU_SHOGI,
    Game.FAIR_SHARES_AND_VARIED_PAIRS,
    Game.GATES,
    Game.GOBBLET_GOBBLERS,
    Game.KAOOA,
    Game.MILLS,
    Game.NEUTRON,
    Game.QUIXO,
    Game.TEEKO,
    Game.TTTIER,
    Game.TTT,
    Game.WINKERS,
]

# Test matrix: [(<game_name>, <game_options>), ...]
INTERACTIVE_GAMES: list[tuple[str, list[int]]] = [
    (Game.ALL_QUEENS_CHESS, []),
    (Game.DOBUTSU_SHOGI, []),
    (Game.FAIR_SHARES_AND_VARIED_PAIRS, [0]),
    (Game.FAIR_SHARES_AND_VARIED_PAIRS, [6]),
    (Game.GATES, []),
    (Game.GOBBLET_GOBBLERS, []),
    (Game.KAOOA, []),
    (Game.MILLS, [0, 1, 0, 0, 0]),
    (Game.MILLS, [1, 1, 0, 0, 0]),
    (Game.MILLS, [2, 1, 0, 0, 0]),
    (Game.MILLS, [3, 1, 0, 0, 0]),
    (Game.MILLS, [4, 1, 0, 0, 0]),
    (Game.MILLS, [5, 1, 0, 0, 0]),
    (Game.MILLS, [6, 1, 0, 0, 0]),
    (Game.MILLS, [7, 1, 0, 0, 0]),
    (Game.MILLS, [3, 0, 0, 0, 0]),
    (Game.MILLS, [3, 1, 1, 0, 0]),
    (Game.MILLS, [3, 1, 0, 1, 0]),
    (Game.MILLS, [3, 1, 0, 2, 0]),
    (Game.MILLS, [3, 1, 0, 0, 1]),
    (Game.NEUTRON, []),
    (Game.QUIXO, [0]),
    (Game.QUIXO, [1]),
    (Game.QUIXO, [2]),
    (Game.TEEKO, []),
    (Game.TTTIER, []),
    (Game.TTT, []),
    (Game.WINKERS, []),
]


@pytest.fixture(scope="session")
def gamesman_bin() -> str:
    """
    Determines the binary path based on the GAMESMAN_PRESET.
    Defaults to release if the environment variable is not set.
    """
    preset = os.environ.get("GAMESMAN_PRESET", "release")
    if preset == "release":
        return "./bin/gamesman"
    return f"./build/{preset}/src/gamesman"


@pytest.mark.parametrize("game_name,game_options", INTERACTIVE_GAMES)
def test_gamesman_interactive(
    game_name: str,
    game_options: list[int],
    gamesman_bin: str,
    snapshot: SnapshotAssertion,
) -> None:
    """
    Interactive End-to-End test for gameplay loop.
    Uses 'snapshot' (from syrupy) to validate the terminal stdout.
    """
    # Automatically resolve the game's menu selection index
    game_choice = GAMES.index(game_name)

    max_moves = 50
    random.seed(42)
    output_log = io.StringIO()

    # Spawn the process
    child = pexpect.spawn(gamesman_bin, encoding="utf-8", timeout=5)

    # Enforce a large, static terminal size to prevent unpredictable line-wrapping
    child.setwinsize(200, 200)

    # Log all stdout from the application to our output_log variable
    child.logfile_read = output_log

    try:
        # -- WELCOME SCREEN --
        child.expect(r"press <return> to continue")
        child.sendline("")
        child.expect(r"=>")

        # -- MAIN MENU: Select Game --
        child.sendline("g")
        child.expect(r"=>")

        # -- GAMES LIST: Select the specific game --
        child.sendline(str(game_choice))
        child.expect(r"=>")

        # -- PRE-SOLVED MENU: Select Options --

        # Configure game options if provided
        if game_options:
            child.sendline("g")

            for option_index, option_value in enumerate(game_options):
                # Select the option index to change
                child.expect(r"=>")
                child.sendline(str(option_index))

                # Select the specific value for that option
                child.expect(r"=>")
                child.sendline(str(option_value))

            # Go back to Pre-Solved Menu
            child.expect(r"=>")
            child.sendline("b")
            child.expect(r"=>")

        # Start without solving
        child.sendline("w")
        child.expect(r"=>")

        # Play new game
        child.sendline("p")

        # -- GAMEPLAY LOOP --
        move_regex = re.compile(r"Player \d+'s move \[(.*)\]:")
        game_ended = False

        for _ in range(max_moves):
            # We expect either the next move prompt OR the menu prompt (if game ended)
            index = child.expect([move_regex, r"=>"])

            if index == 0:
                # Game is still going, parse moves and play
                raw_moves_string = child.match.group(1)
                available_moves = re.findall(r"\[([^\]]+)\]", raw_moves_string)
                chosen_move = random.choice(available_moves)
                child.sendline(chosen_move)
            elif index == 1:
                # Game ended early and dropped us back at the menu
                game_ended = True
                break

        # -- ABORT & QUIT --
        if not game_ended:
            # We hit max_moves. Check if the last move ended the game or if it's asking for another
            index = child.expect([move_regex, r"=>"])
            if index == 0:
                # Still in a game, we need to abort
                child.sendline("a")
                child.expect(r"=>")

        # Now we are definitively at the Pre/Post-Solved menu, so we can quit
        child.sendline("q")

        # Wait for the process to exit cleanly
        child.expect(pexpect.EOF)

    except pexpect.TIMEOUT:
        pytest.fail(
            f"Test timed out for {game_name}. The application did not prompt as expected.\n"
            f"Current Output Log:\n{output_log.getvalue()}"
        )
    except ValueError:
        pytest.fail(f"'{game_name}' was not found in the GAMES list.")

    # ==========================================
    # SNAPSHOT ASSERTION
    # ==========================================
    actual_output = output_log.getvalue()

    # Strip trailing whitespace/carriage returns that might differ between OS environments
    actual_output = "\n".join([line.rstrip() for line in actual_output.splitlines()])

    # Validate the entire string log against the snapshot
    assert actual_output == snapshot(name=f"{game_name}_interactive_stdout")
