"""Central game definitions for all E2E tests.

Each game entry contains a CLI internal name and variant ID, which are the
identifiers used by the gamesman binary. Future game implementation tests
will also use these identifiers.

NOTE: The headless game list is mirrored in .github/workflows/ci.yml for
per-game CI fan-out. Keep the two in sync when adding/removing games.
"""


# ---------------------------------------------------------------------------
# CLI internal game names — single source of truth for all test lists
# ---------------------------------------------------------------------------


class Game:
    DSHOGI = "dshogi"
    FSVP = "fsvp"
    GATES = "gates"
    GOBBLETG = "gobbletg"
    MALLQUEENSCHESS = "mallqueenschess"
    MILLS = "mills"
    MKAOOA = "mkaooa"
    MTTT = "mttt"
    MTTTIER = "mtttier"
    NEUTRON = "neutron"
    QUIXO = "quixo"
    TEEKO = "teeko"
    WINKERS = "winkers"


# ---------------------------------------------------------------------------
# Headless (automated) tests — small games that can be solved in CI
# (game_cli_name, variant_id, has_autogui)
# ---------------------------------------------------------------------------
HEADLESS_GAMES: list[tuple[str, int, bool]] = [
    (Game.FSVP, 10, False),  # Regular
    (Game.MTTT, 0, False),  # Regular
    (Game.MKAOOA, 0, True),  # Regular
    (Game.MILLS, 72, True),  # Tier
    (Game.MTTTIER, 0, True),  # Tier
    (Game.QUIXO, 1, True),  # Tier
    (Game.QUIXO, 2, True),  # Tier
]

# ---------------------------------------------------------------------------
# Headless manual tests — large pre-solved games (require local database)
# (game_cli_name, variant_id, has_autogui)
# ---------------------------------------------------------------------------
HEADLESS_MANUAL_GAMES: list[tuple[str, int, bool]] = [
    (Game.DSHOGI, 0, True),  # Regular
    (Game.GATES, 0, False),  # Tier
    (Game.GOBBLETG, 0, True),  # Tier
    (Game.MILLS, 0, True),  # Tier
    (Game.MILLS, 72, True),  # Tier
    (Game.MILLS, 144, True),  # Tier
    (Game.MILLS, 234, True),  # Tier
    (Game.MILLS, 236, True),  # Tier
    (Game.MILLS, 312, True),  # Tier
    (Game.MILLS, 314, True),  # Tier
    (Game.MILLS, 318, True),  # Tier
    (Game.MILLS, 320, True),  # Tier
    (Game.MILLS, 396, True),  # Tier
    (Game.MILLS, 450, True),  # Tier
    (Game.NEUTRON, 0, True),  # Regular
    (Game.QUIXO, 0, True),  # Tier
    (Game.TEEKO, 0, True),  # Regular
    (Game.TEEKO, 1, True),  # Regular
    (Game.WINKERS, 0, True),  # Tier
]

# ---------------------------------------------------------------------------
# Interactive tests
# ---------------------------------------------------------------------------

# Ordered list matching the interactive menu (sorted by formal name).
# Used to compute menu selection indices.
GAMES: list[str] = [
    Game.MALLQUEENSCHESS,
    Game.DSHOGI,
    Game.FSVP,
    Game.GATES,
    Game.GOBBLETG,
    Game.MKAOOA,
    Game.MILLS,
    Game.NEUTRON,
    Game.QUIXO,
    Game.TEEKO,
    Game.MTTTIER,
    Game.MTTT,
    Game.WINKERS,
]

# Interactive test matrix: (game_name, game_options)
INTERACTIVE_GAMES: list[tuple[str, list[int]]] = [
    (Game.MALLQUEENSCHESS, []),
    (Game.DSHOGI, []),
    (Game.FSVP, [0]),
    (Game.FSVP, [6]),
    (Game.GATES, []),
    (Game.GOBBLETG, []),
    (Game.MKAOOA, []),
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
    (Game.MTTTIER, []),
    (Game.MTTT, []),
    (Game.WINKERS, []),
]
