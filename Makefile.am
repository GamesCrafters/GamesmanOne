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

# src/core/generic_hash
bin_gamesman_SOURCES += src/core/generic_hash/context.c
bin_gamesman_SOURCES += src/core/generic_hash/generic_hash.c

# src/core/tier_solver
bin_gamesman_SOURCES += src/core/tier_solver/frontier.c
bin_gamesman_SOURCES += src/core/tier_solver/reverse_graph.c
bin_gamesman_SOURCES += src/core/tier_solver/tier_solver.c

# src/core
bin_gamesman_SOURCES += src/core/analysis.c
bin_gamesman_SOURCES += src/core/gamesman.c
bin_gamesman_SOURCES += src/core/gamesman_math.c
bin_gamesman_SOURCES += src/core/misc.c
bin_gamesman_SOURCES += src/core/naivedb.c
bin_gamesman_SOURCES += src/core/solver.c

# src/games
bin_gamesman_SOURCES += src/games/mttt/mttt.c
bin_gamesman_SOURCES += src/games/mtttier/mtttier.c
