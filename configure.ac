#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(ktech, 2.0)
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([no-define foreign subdir-objects])
AC_CONFIG_MACRO_DIRS([m4])

AC_CONFIG_SRCDIR([src/ktech.cpp])

AH_TOP([#ifndef KTECH_CONFIG_H
#define KTECH_CONFIG_H])
AC_CONFIG_HEADERS([config.h])
AH_BOTTOM([#endif])


AC_ARG_ENABLE([cpuextensions],
	AS_HELP_STRING([--disable-cpuextensions], [Disables use of CPU extensions.]))


# Checks for packages
PKG_CHECK_MODULES([DEPS], [Magick++])

# Checks for programs.
OLD_CXXFLAGS=$CXXFLAGS
AC_PROG_CXX
CXXFLAGS=$OLD_CXXFLAGS

AC_PROG_RANLIB
AC_PROG_INSTALL


# Checks for libraries.
AC_CHECK_LIB([m], [floor])

# Checks for header files.
AC_LANG_PUSH([C])
AC_CHECK_HEADERS([stdint.h inttypes.h sys/types.h],
		 [ktech_has_int_header=yes; break;])
AS_IF([test "x$ktech_has_int_header" != "xyes"],
      [AC_MSG_ERROR([Unable to find headers defining fixed-length integer types.])])
AC_CHECK_HEADERS([sys/stat.h stat.h],
		 [ktech_has_stat_header=yes; break;])
AS_IF([test "x$ktech_has_stat_header" != "xyes"],
      [AC_MSG_ERROR([Unable to find a stat.h header.])])
AC_CHECK_HEADERS([libgen.h],
		 [ktech_has_libgen_header=yes; break;])
AS_IF([test "x$ktech_has_libgen_header" != "xyes"],
      [AC_MSG_ERROR([Unable to find the libgen.h header.])])
AC_LANG_POP

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

AX_CXXFLAGS_WARN_ALL
AX_APPEND_COMPILE_FLAGS([-pedantic -Wshadow -Wextra -Wno-long-long], [CXXFLAGS])
AS_IF([! test "x$enable_cpuextensions" = "xno" && test "x$OLD_CXXFLAGS" = "x"],
	[AX_APPEND_COMPILE_FLAGS([-O3], [CXXFLAGS])])

AS_IF([! test "x$enable_cpuextensions" = "xno" && test "x$ac_cv_cxx_compiler_gnu" = "xyes"], [AX_EXT])
AM_CONDITIONAL([HAVE_SSE2_EXTENSION], [test "x$ax_cv_have_sse2_ext" = "xyes"])
AM_CONDITIONAL([HAVE_ALTIVEC_EXTENSION], [test "x$ax_cv_have_altivec_ext" = "xyes"])

################
# tclap stuff
################

AC_CXX_HAVE_SSTREAM
AC_CXX_HAVE_STRSTREAM
AC_CXX_HAVE_LONG_LONG


AC_CONFIG_FILES([Makefile m4/Makefile src/Makefile \
		 lib/Makefile \
		 	lib/boost/Makefile \
			\
		 	lib/squish/Makefile \
			\
			lib/tclap/Makefile \
		])

AC_OUTPUT
