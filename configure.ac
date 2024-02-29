#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([GamesmanOne], [0.6.1], [robertyishi@berkeley.edu])

AM_INIT_AUTOMAKE([-Wall -Werror subdir-objects foreign])
AC_CONFIG_SRCDIR([src/core/main.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AS_IF([test "x$CC" = "xmpicc"], [CFLAGS="$CFLAGS -DUSE_MPI"])

# Enables OpenMP if possible.
# Pass "--disable-openmp" to configure to force disable.
AC_OPENMP

# Checks for libraries.
AC_CHECK_LIB([z], [deflate], [], [AC_MSG_ERROR([zlib library not found])])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h float.h inttypes.h limits.h locale.h stddef.h stdint.h stdlib.h string.h strings.h sys/param.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([dup2 memmove memset mkdir setlocale strcasecmp strchr strcspn strdup strerror strncasecmp strrchr strstr strtoull])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
