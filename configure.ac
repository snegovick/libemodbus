dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT([libemodbus], [1.0.0], [snegovick@gmail.com])

AC_CONFIG_AUX_DIR([./config])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.9 foreign])

AC_MSG_CHECKING([whether to build with debug information])
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
        [enable debug data generation (def=no)])],
    [debugit="$enableval"],
    [debugit=no])
AC_MSG_RESULT([$debugit])

if test x"$debugit" = x"yes"; then
    AC_DEFINE([DEBUG],[],[Debug Mode])
    AM_CFLAGS="$AM_CFLAGS -g -Wall -Werror -Wno-uninitialized -O0"
    CFLAGS="-g -O0 -Wall -Werror -Wno-uninitialized $CFLAGS"
else
    AC_DEFINE([NDEBUG],[],[No-debug Mode])
    AM_CFLAGS="$AM_CFLAGS -O3"
fi

PKG_CHECK_MODULES(CCONTAINER, libccontainer >= 1.0.1)


AC_PROG_CC
AC_PROG_LIBTOOL

dnl default-enabled tests feature
AC_ARG_ENABLE([tests],
    AS_HELP_STRING([--disable-tests], [Disable tests building]))

AM_CONDITIONAL([BUILD_TESTS], [test "x$enable_tests" != "xno"])
AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile])



#src/Makefile

AC_OUTPUT

