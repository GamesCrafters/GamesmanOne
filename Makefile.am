AM_CFLAGS = -I$(srcdir)/src
AM_CFLAGS += -Wall -Wextra -std=gnu99
AM_CFLAGS += ${OPENMP_CFLAGS}

noinst_PROGRAMS = bin/gamesman

# Source file containing the main() function. 
bin_gamesman_SOURCES = src/core/main.c

# src/core/data_structures
bin_gamesman_SOURCES += src/core/data_structures/int64_array.c
bin_gamesman_SOURCES += src/core/data_structures/int64_hash_map.c
bin_gamesman_SOURCES += src/core/data_structures/int64_queue.c

# src/core/db
bin_gamesman_SOURCES += src/core/db/naivedb/naivedb.c
bin_gamesman_SOURCES += src/core/db/db_manager.c

# src/core/generic_hash
bin_gamesman_SOURCES += src/core/generic_hash/context.c
bin_gamesman_SOURCES += src/core/generic_hash/generic_hash.c

# src/core/interactive
bin_gamesman_SOURCES += src/core/interactive/credits/credits.c
bin_gamesman_SOURCES += src/core/interactive/games/presolve/options/options.c
bin_gamesman_SOURCES += src/core/interactive/games/presolve/postsolve/analyze/analyze.c
bin_gamesman_SOURCES += src/core/interactive/games/presolve/postsolve/configure/configure.c
bin_gamesman_SOURCES += src/core/interactive/games/presolve/postsolve/help/game_help.c
bin_gamesman_SOURCES += src/core/interactive/games/presolve/postsolve/play/play.c
bin_gamesman_SOURCES += src/core/interactive/games/presolve/postsolve/postsolve.c
bin_gamesman_SOURCES += src/core/interactive/games/presolve/solver_options/solver_options.c
bin_gamesman_SOURCES += src/core/interactive/games/presolve/match.c
bin_gamesman_SOURCES += src/core/interactive/games/presolve/presolve.c
bin_gamesman_SOURCES += src/core/interactive/games/games.c
bin_gamesman_SOURCES += src/core/interactive/help/help.c
bin_gamesman_SOURCES += src/core/interactive/automenu.c
bin_gamesman_SOURCES += src/core/interactive/main_menu.c

# src/core/solvers
bin_gamesman_SOURCES += src/core/solvers/regular_solver/regular_solver.c
bin_gamesman_SOURCES += src/core/solvers/tier_solver/frontier.c
bin_gamesman_SOURCES += src/core/solvers/tier_solver/reverse_graph.c
bin_gamesman_SOURCES += src/core/solvers/tier_solver/tier_manager.c
bin_gamesman_SOURCES += src/core/solvers/tier_solver/tier_solver.c
bin_gamesman_SOURCES += src/core/solvers/tier_solver/tier_worker.c
bin_gamesman_SOURCES += src/core/solvers/solver_manager.c

# src/core
bin_gamesman_SOURCES += src/core/analysis.c
bin_gamesman_SOURCES += src/core/gamesman_headless.c
bin_gamesman_SOURCES += src/core/gamesman_interactive.c
bin_gamesman_SOURCES += src/core/gamesman_types.c
bin_gamesman_SOURCES += src/core/misc.c

# src/games
bin_gamesman_SOURCES += src/games/game_manager.c

# All Games
bin_gamesman_SOURCES += src/games/mttt/mttt.c
bin_gamesman_SOURCES += src/games/mtttier/mtttier.c
