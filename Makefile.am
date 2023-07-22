AM_CFLAGS = -I$(srcdir)/src
AM_CFLAGS += -Wall -Wextra -std=gnu99
AM_CFLAGS += ${OPENMP_CFLAGS}

noinst_PROGRAMS = bin/gamesman

bin_gamesman_SOURCES = src/core/main.c

bin_gamesman_SOURCES += src/core/analysis.c
bin_gamesman_SOURCES += src/core/data_structures/int64_array.c
bin_gamesman_SOURCES += src/core/data_structures/int64_hash_map.c
bin_gamesman_SOURCES += src/core/data_structures/int64_queue.c
bin_gamesman_SOURCES += src/core/gamesman.c
bin_gamesman_SOURCES += src/core/misc.c
bin_gamesman_SOURCES += src/core/naivedb.c
bin_gamesman_SOURCES += src/core/solver.c
bin_gamesman_SOURCES += src/core/tier_solver/frontier.c
bin_gamesman_SOURCES += src/core/tier_solver/reverse_graph.c
bin_gamesman_SOURCES += src/core/tier_solver/tier_solver.c

bin_gamesman_SOURCES += src/games/mttt/mttt.c
