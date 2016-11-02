AC_CONFIG_MACRO_DIRS([m4])
AC_INIT([Pi IO Service], [0.3.0], [bugs@miqra.nl], [piio-server], [http://www.miqra.nl/])
AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([1.11 no-define foreign subdir-objects])
AC_CONFIG_HEADERS([config.hpp])
AC_PROG_CXX
AC_PROG_CC
PKG_CHECK_MODULES([DEPS], [dbus-c++-1 >= 0.9.0, libconfig++])

AC_CHECK_PROG(DBUSXX_CHECK,dbusxx-xml2cpp,yes)
if test x"$DBUSXX_CHECK" != x"yes" ; then
    AC_MSG_ERROR([Please install libdbus-c++-bin before installing.])
fi

AC_PATH_PROG([DEBUILD], [dpkg-buildpackage], [Could not find dpkg-buildpackage])
AC_PATH_PROG([DHMAKE], [dh_make], [Could not find dh_make])

AX_BOOST_BASE([1.49.0]) 
AX_BOOST_PROGRAM_OPTIONS

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
