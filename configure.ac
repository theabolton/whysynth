dnl Process this file with autoconf to produce a configure script.

AC_INIT(WhySynth, devel, musound-at-jps-dot-net)

AC_CONFIG_SRCDIR(src/whysynth.h)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

AC_PROG_CC

AC_ENABLE_STATIC(no)
AC_ENABLE_SHARED(yes)
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

AC_C_BIGENDIAN

dnl Require DSSI, liblo, and FFTW3
AC_CHECK_HEADERS(ladspa.h)
PKG_CHECK_MODULES(COMMON, dssi >= 0.9)
PKG_CHECK_MODULES(PLUGIN, fftw3f >= 3.0.1)
PKG_CHECK_MODULES(GUI, liblo >= 0.12)

dnl Check for GTK+
with_gtk=no
AM_PATH_GTK_2_0(2.4.0, with_gtk='yes', AC_MSG_WARN([GUI will not be built; GTK+ 2.4 or later needed]))
AM_CONDITIONAL(BUILD_GUI, test "x${with_gtk}" != 'xno')

dnl developer functions
AC_ARG_ENABLE(developer, AC_HELP_STRING([--enable-developer], [enable developer functions, default=no]),
              [ if test $enableval = "yes"; then
                AC_DEFINE(DEVELOPER,, [Define to 1 to enable developer-only functions.])
                fi ])

dnl OS specific checks
case "${host_os}" in
darwin*)
  darwin=yes
  PKG_CHECK_MODULES(ALSA, libdssialsacompat)
  ;;
*)
  darwin=no
  PKG_CHECK_MODULES(ALSA, alsa)
  ;;
esac
AM_CONDITIONAL(DARWIN, test x$darwin = xyes)

dnl Check for newlocale() et al.
AC_MSG_CHECKING(for newlocale in xlocale.h)
AC_TRY_COMPILE([
    #include <xlocale.h>
  ], [newlocale(LC_ALL_MASK, "C", NULL)], use_xlocale=yes, use_xlocale=no)
if test "$use_xlocale" = "yes"; then
  AC_DEFINE(THREAD_LOCALE_XLOCALE_H, 1, [Define to 1 if newlocale() is defined in xlocale.h (e.g. on Mac OS X)])
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
  AC_MSG_CHECKING(for newlocale in locale.h)
  AC_TRY_COMPILE([
      #define _XOPEN_SOURCE 700
      #include <locale.h>
    ], [newlocale(LC_ALL_MASK, "C", NULL)], use_locale=yes, use_locale=no)
  if test "$use_locale" = "yes"; then
    AC_DEFINE(THREAD_LOCALE_LOCALE_H, 1, [Define to 1 if newlocale() is defined in locale.h (e.g. in glibc)])
    AC_MSG_RESULT(yes)
  else
    AC_MSG_WARN([newlocale()/uselocale() not found, patch loading may fail in non-C locales])
    AC_MSG_RESULT(no)
  fi
fi

dnl Use lotsa flags if we have gcc.
CFLAGS="$CFLAGS $ALSA_CFLAGS $COMMON_CFLAGS"
LDFLAGS="$LDFLAGS $ALSA_LIBS $COMMON_LIBS"
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
  case " $CFLAGS " in
  *[\ \	]-O[1-9][\ \	]*) ;;
  *) CFLAGS="$CFLAGS -O2" ;;
  esac
  case " $CFLAGS " in
  *[\ \	]-ffast-math[\ \	]*) ;;
  *)  PLUGIN_CFLAGS="$PLUGIN_CFLAGS -fomit-frame-pointer -funroll-loops -finline-functions -ffast-math"
      # add '-finline-limit=...' only for GCC 3.x and 4.x:
      case `$CC -dumpversion 2>/dev/null` in
      # these --params are also in 3.4.6, but not in 3.3.6
      4.*) PLUGIN_CFLAGS="$PLUGIN_CFLAGS -finline-limit=5000 --param large-function-growth=4900 --param inline-unit-growth=900" ;;
      3.*) PLUGIN_CFLAGS="$PLUGIN_CFLAGS -finline-limit=5000" ;;
      esac
      if test "x$enable_developer" = "xyes"; then
        PLUGIN_CFLAGS="$PLUGIN_CFLAGS -Winline"
      fi ;;
  esac
fi
changequote([,])dnl
AC_SUBST(PLUGIN_CFLAGS)
AC_SUBST(PLUGIN_LIBS)
AC_SUBST(GUI_CFLAGS)
AC_SUBST(GUI_LIBS)

AC_OUTPUT([
Makefile
src/Makefile
])

