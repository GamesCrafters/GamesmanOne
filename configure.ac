#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([GamesmanOne], [0.4], [robertyishi@berkeley.edu])

AM_INIT_AUTOMAKE([-Wall -Werror subdir-objects foreign])
AC_CONFIG_SRCDIR([src/core/main.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Enables OpenMP if possible.
# Pass "--disable-openmp" to configure to force disable.
AC_OPENMP

# Checks for libraries.
AC_CHECK_LIB([z], [deflate], [], [AC_MSG_ERROR([zlib library not found])])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h stdint.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset mkdir strcspn])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
