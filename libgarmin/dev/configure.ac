#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(libgarmin, 0.1, libgarmin@gmail.com)
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
# Checks for programs.
AC_PROG_CC
if eval "test x$GCC = xyes"; then
        CFLAGS="$CFLAGS -Wall -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith -Wreturn-type"
fi

AC_PROG_RANLIB
# Checks for libraries.
# FIXME: Replace `main' with a function in `-lgarmin':
#AC_CHECK_LIB([garmin], [main])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h malloc.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_OFF_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([strchr strdup strerror strrchr])

AC_ARG_ENABLE([debug],
[  --enable-debug    Turn on debugging (default:true)],
[case "${enableval}" in
yes) debug=true ;;
no)  debug=false ;;
*) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],[debug=true])
AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])
if eval "test x$GCC = xyes"; then
        CFLAGS="$CFLAGS -DDEBUG"
fi

extras=false
if test -f "src/extras.c";
then
	extras=true
	CFLAGS="$CFLAGS -DEXTRAS"
fi
AM_CONDITIONAL([HAVEEXTRAS], [test x$extras = xtrue])

AC_CANONICAL_HOST
win32=no
case $host_os in
wince)
	win32=yes
	AC_DEFINE(TARGET_WIN32CE, 1, [Building for WINCE])
	;;
esac
AM_CONDITIONAL(TARGET_WIN32CE, [test "x$win32" = "xyes"])


AC_CONFIG_FILES([Makefile
                 src/Makefile
                 data/Makefile
                 utils/Makefile
		 libgarmin.pc])
AC_OUTPUT
