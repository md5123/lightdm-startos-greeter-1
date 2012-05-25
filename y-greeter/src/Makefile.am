sbin_PROGRAMS = y-greeter

y_greeter_SOURCES = \
	main.c \
	backend.c \
	backend.h \
	gtkclock.c \
	gtkclock.h \
	gtkloginbox.c \
	gtkloginbox.h \
	gtkprompt.c \
	gtkprompt.h \
	gtksysmenu.c \
	gtksysmenu.h \
	gtkuserface.c \
	gtkuserface.h \
	ui.c \
	ui.h 

y_greeter_CFLAGS = \
	-DGETTEXT_PACKAGE=\""$(GETTEXT_PACKAGE)\"" \
	-DLOCALEDIR=\""$(localedir)"\" \
	-DGREETER_DATA_DIR=\""$(pkgdatadir)/"\" \
	-DGREETER_CONF_FILE=\""$(sysconfdir)/lightdm/y-greeter.conf"\" \
	-DTEXTDOMAIN=GETTEXT_PACKAGE \
	-DAPP_NAME=GETTEXT_PACKAGE \
	-Wall \
	$(GREETER_CFLAGS)

y_greeter_LDADD = \
	$(GREETER_LIBS)

DISTCLEANFILES = \
	Makefile.in
