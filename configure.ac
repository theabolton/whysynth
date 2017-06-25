dnl Process this file with autoconf to produce a configure script.

AC_INIT(WhySynth, 20120903, whysynth-at-smbolton-dot-com)

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
AM_PATH_GTK_2_0(2.24.0, with_gtk='yes', AC_MSG_WARN([GUI will not be built; GTK+ 2.24 or later needed]))
AM_CONDITIONAL(BUILD_GUI, test "x${with_gtk}" != 'xno')

dnl developer functions
AC_ARG_ENABLE(developer, AC_HELP_STRING([--enable-developer], [enable developer functions, default=no]),
              [ if test $enableval = "yes"; then
                AC_DEFINE(DEVELOPER, 1, [Define to 1 to enable developer-only functions.])
                fi ])

dnl OS specific checks
case "${host_os}" in
darwin*)
  darwin=yes
  PKG_CHECK_MODULES(ALSA, libdssialsacompat)
  ;;
freebsd*)
  darwin=no
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
      #define _GNU_SOURCE
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

CFLAGS="$CFLAGS $ALSA_CFLAGS $COMMON_CFLAGS"
LDFLAGS="$LDFLAGS $COMMON_LIBS"
PLUGIN_LIBS="$PLUGIN_LIBS $ALSA_LIBS"
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
    *)  PLUGIN_CFLAGS="$PLUGIN_CFLAGS -funroll-loops -ffast-math"
      if "$CC" --version | grep -q gcc ; then
        PLUGIN_CFLAGS="$PLUGIN_CFLAGS -finline-functions"
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

