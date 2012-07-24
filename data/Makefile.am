greeterdatadir = $(datadir)/lightdm-startos-greeter

imagedir = $(greeterdatadir)
dist_image_DATA = \
	defaultface.png \
	background.jpg \
	keyboard.png \
	keyboard-hl.png \
	language.png \
	language-hl.png \
	power.png \
	power-hl.png \
	session.png \
	session-hl.png \
	go-normal.png \
	go-hover.png \
    currentlogin.png


configdir = $(sysconfdir)/lightdm
dist_config_DATA = lightdm-startos-greeter.conf

xgreetersdir = $(datadir)/xgreeters

lightdm-startos-greeter.desktop: lightdm-startos-greeter.desktop.in
	sed -e 's,[@]BINDIR[@],$(sbindir),g' lightdm-startos-greeter.desktop.in > lightdm-startos-greeter.desktop

dist_xgreeters_DATA = lightdm-startos-greeter.desktop

DISTCLEANFILES = \
	Makefile.in \
	lightdm-startos-greeter.desktop
