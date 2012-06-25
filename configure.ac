dnl Process this file with autoconf to produce a configure script.

AC_INIT([lightdm-ylmfos-greeter], [0.00])
AC_CONFIG_MACRO_DIR(m4)

dnl AC_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
LT_INIT
AM_PROG_CC_C_O
AM_MAINTAINER_MODE
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES(yes)])

GOBJECT_INTROSPECTION_CHECK(0.9.5)
GNOME_COMPILE_WARNINGS(maximum)

PKG_CHECK_MODULES(GREETER, [
    gtk+-3.0
    x11
    liblightdm-gobject-1
])

AC_PROG_SED
IT_PROG_INTLTOOL
AC_SUBST(GETTEXT_PACKAGE, lightdm-ylmfos-greeter)

dnl set shell variable $"GETTEXT_PACKAGE" assign to GETTEXT_PACKAGE
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext package]) 

AC_PREFIX_DEFAULT([/usr])
AC_CONFIG_FILES([
	Makefile
	data/Makefile
	po/Makefile.in
	src/Makefile
])
AC_OUTPUT
