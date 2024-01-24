#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([GamesmanOne], [0.6.0], [robertyishi@berkeley.edu])

AM_INIT_AUTOMAKE([-Wall -Werror subdir-objects foreign])
AC_CONFIG_SRCDIR([src/core/main.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# If --with-mpi=auto is used, try to find MPI, but use standard C compiler if it is not found.
# If --with-mpi=yes is used, try to find MPI and fail if it isn't found.
# If --with-mpi=no is used, use a standard C compiler instead.
AC_ARG_WITH(mpi, [AS_HELP_STRING([--with-mpi],
    [compile with MPI (parallelization) support. If none is found,
    MPI is not used. Default: auto])], , [with_mpi=auto])

AX_PROG_CC_MPI([test x"$with_mpi" != xno], [use_mpi=yes], [
    use_mpi=no
    if test x"$with_mpi" = xyes; then
        AC_MSG_FAILURE([MPI compiler requested, but couldn't use MPI.])
    else
        AC_MSG_WARN([No MPI compiler found, won't use MPI.])
    fi
])

# Enables OpenMP if possible.
# Pass "--disable-openmp" to configure to force disable.
AC_OPENMP

# Checks for libraries.
AC_CHECK_LIB([z], [deflate], [], [AC_MSG_ERROR([zlib library not found])])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h stdint.h strings.h sys/param.h sys/time.h syslog.h unistd.h])

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
