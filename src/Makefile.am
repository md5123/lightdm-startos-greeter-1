sbin_PROGRAMS = lightdm-ylmfos-greeter

lightdm_ylmfos_greeter_SOURCES = \
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

lightdm_ylmfos_greeter_CFLAGS = \
	-DGETTEXT_PACKAGE=\""$(GETTEXT_PACKAGE)\"" \
	-DLOCALEDIR=\""$(localedir)"\" \
	-DGREETER_DATA_DIR=\""$(pkgdatadir)/"\" \
	-DGREETER_CONF_FILE=\""$(sysconfdir)/lightdm/lightdm-ylmfos-greeter.conf"\" \
	-DTEXTDOMAIN=GETTEXT_PACKAGE \
	-DAPP_NAME=GETTEXT_PACKAGE \
	-Wall \
	$(GREETER_CFLAGS)

lightdm_ylmfos_greeter_LDADD = \
	$(GREETER_LIBS)

DISTCLEANFILES = \
	Makefile.in
