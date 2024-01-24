# Include headers from the src directory.
AM_CFLAGS = -I$(srcdir)/src

# Local resources directory.
RES_DIR = $(srcdir)/res
AM_CFLAGS += -I$(RES_DIR)/include
AM_LDFLAGS = -L$(RES_DIR)/lib
AM_LDFLAGS += -L$(RES_DIR)/lib64

# Configure OpenMP.
AM_CFLAGS += ${OPENMP_CFLAGS}

# Build Gamesman as a non-installable program.
noinst_PROGRAMS = bin/gamesman

# Add linker directives.
bin_gamesman_LDADD = -ljson-c

# Source file containing the main() function. 
bin_gamesman_SOURCES = src/core/main.c

# src/core/analysis
bin_gamesman_SOURCES += src/core/analysis/analysis.c
bin_gamesman_SOURCES += src/core/analysis/stat_manager.c

# src/core/data_structures
bin_gamesman_SOURCES += src/core/data_structures/bitstream.c
bin_gamesman_SOURCES += src/core/data_structures/cstring.c
bin_gamesman_SOURCES += src/core/data_structures/int64_array.c
bin_gamesman_SOURCES += src/core/data_structures/int64_hash_map.c
bin_gamesman_SOURCES += src/core/data_structures/int64_queue.c

# src/core/db
bin_gamesman_SOURCES += src/core/db/bpdb/bparray.c
bin_gamesman_SOURCES += src/core/db/bpdb/bpdb_file.c
bin_gamesman_SOURCES += src/core/db/bpdb/bpdb_lite.c
bin_gamesman_SOURCES += src/core/db/bpdb/bpdb_probe.c
bin_gamesman_SOURCES += src/core/db/bpdb/bpdict.c
bin_gamesman_SOURCES += src/core/db/naivedb/naivedb.c
bin_gamesman_SOURCES += src/core/db/db_manager.c

# src/core/generic_hash
bin_gamesman_SOURCES += src/core/generic_hash/context.c
bin_gamesman_SOURCES += src/core/generic_hash/generic_hash.c

# src/core/headless
bin_gamesman_SOURCES += src/core/headless/hanalyze.c
bin_gamesman_SOURCES += src/core/headless/hjson.c
bin_gamesman_SOURCES += src/core/headless/hparser.c
bin_gamesman_SOURCES += src/core/headless/hquery.c
bin_gamesman_SOURCES += src/core/headless/hsolve.c
bin_gamesman_SOURCES += src/core/headless/hutils.c

# src/core/interactive
bin_gamesman_SOURCES += src/core/interactive/credits/credits.c
bin_gamesman_SOURCES += src/core/interactive/games/presolve/options/choices/choices.c
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
bin_gamesman_SOURCES += src/core/solvers/tier_solver/reverse_tier_graph.c
bin_gamesman_SOURCES += src/core/solvers/tier_solver/tier_analyzer.c
bin_gamesman_SOURCES += src/core/solvers/tier_solver/tier_manager.c
bin_gamesman_SOURCES += src/core/solvers/tier_solver/tier_solver.c
bin_gamesman_SOURCES += src/core/solvers/tier_solver/tier_worker.c
bin_gamesman_SOURCES += src/core/solvers/solver_manager.c

# src/core/types
bin_gamesman_SOURCES += src/core/types/game/game_variant.c
bin_gamesman_SOURCES += src/core/types/move_array.c
bin_gamesman_SOURCES += src/core/types/position_array.c
bin_gamesman_SOURCES += src/core/types/position_hash_set.c
bin_gamesman_SOURCES += src/core/types/tier_array.c
bin_gamesman_SOURCES += src/core/types/tier_hash_map.c
bin_gamesman_SOURCES += src/core/types/tier_hash_set.c
bin_gamesman_SOURCES += src/core/types/tier_position_array.c
bin_gamesman_SOURCES += src/core/types/tier_position_hash_set.c
bin_gamesman_SOURCES += src/core/types/tier_queue.c
bin_gamesman_SOURCES += src/core/types/tier_stack.c

# src/core
bin_gamesman_SOURCES += src/core/constants.c
bin_gamesman_SOURCES += src/core/game_manager.c
bin_gamesman_SOURCES += src/core/gamesman_headless.c
bin_gamesman_SOURCES += src/core/gamesman_interactive.c
bin_gamesman_SOURCES += src/core/misc.c

# src/games
bin_gamesman_SOURCES += src/games/game_list.c

# src/libs
bin_gamesman_SOURCES += src/libs/mgz/gz64.c
bin_gamesman_SOURCES += src/libs/mgz/mgz.c

# All Games
bin_gamesman_SOURCES += src/games/fsvp/fsvp.c
bin_gamesman_SOURCES += src/games/mallqueenschess/mallqueenschess.c
bin_gamesman_SOURCES += src/games/mttt/mttt.c
bin_gamesman_SOURCES += src/games/mtttier/mtttier.c
