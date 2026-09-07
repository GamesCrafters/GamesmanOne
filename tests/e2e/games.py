"""Central game definitions for all E2E tests.

Each game entry contains a CLI internal name and variant ID, which are the
identifiers used by the gamesman binary. Future game implementation tests
will also use these identifiers.

NOTE: The headless game list is mirrored in .github/workflows/ci.yml for
per-game CI fan-out. Keep the two in sync when adding/removing games.
"""

# ---------------------------------------------------------------------------
# Headless (automated) tests — small games that can be solved in CI
# (game_cli_name, variant_id, has_autogui)
# ---------------------------------------------------------------------------
HEADLESS_GAMES: list[tuple[str, int, bool]] = [
    ("fsvp", 10, False),  # Regular
    ("mttt", 0, False),  # Regular
    ("mkaooa", 0, True),  # Regular
    ("mills", 72, True),  # Tier
    ("mtttier", 0, True),  # Tier
    ("quixo", 1, True),  # Tier
    ("quixo", 2, True),  # Tier
]

# ---------------------------------------------------------------------------
# Headless manual tests — large pre-solved games (require local database)
# (game_cli_name, variant_id, has_autogui)
# ---------------------------------------------------------------------------
HEADLESS_MANUAL_GAMES: list[tuple[str, int, bool]] = [
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

# ---------------------------------------------------------------------------
# Interactive tests — game names as shown in the interactive menu
#
# NOTE: These names (e.g. "fair_shares_and_varied_pairs") do NOT match the
# CLI internal game names (e.g. "fsvp"). They are used for snapshot IDs and
# menu index lookup. A manual refactoring pass to align them is planned.
# ---------------------------------------------------------------------------


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


# Ordered list matching the interactive menu (sorted by formal name).
# Used to compute menu selection indices.
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

# Interactive test matrix: (game_name, game_options)
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
