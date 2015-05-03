#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([svx2vdb], [0.01], [david@flypig.co.uk])
AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_SRCDIR([src/svx2vdb.cc])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AM_PROG_CC_C_O

# Checks for libraries.
PKG_CHECK_MODULES([OpenEXR], [OpenEXR, tbb])
PKG_CHECK_MODULES([SVX2VDB], [libpng, libzip, libxml-2.0])
#AC_CHECK_LIB([openvdb], [openvdb::initialize], [], [AC_MSG_ERROR([The openvdb library is missing.])])
#AC_CHECK_LIB([symbolic], [CreateInteger], [], [AC_MSG_ERROR([The symbolic library is missing.])])

# See http://nerdland.net/2009/07/detecting-c-libraries-with-autotools/
AC_MSG_CHECKING([for OpenVDB])
AC_LANG(C++)
SAVED_LIBS=$LIBS
LIBS="$LIBS -lopenvdb"
AC_LINK_IFELSE(
	[AC_LANG_PROGRAM([[#include <openvdb/openvdb.h>]], [openvdb::initialize();])],
	[AC_MSG_RESULT([yes]); SVX2VDB_LIBS="$SVX2VDB_LIBS -lopenvdb"] [HAVE_OPENVDB=1],
	[AC_MSG_ERROR([The openvdb library is missing.])])
LIBS=$LIBS

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h png.h zip.h openvdb/openvdb.h])

# Checks for compiler characteristics
AC_C_BIGENDIAN

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE

# Checks for library functions.
AC_FUNC_STRTOD
#AC_CHECK_FUNCS([floor ftime pow sqrt])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
