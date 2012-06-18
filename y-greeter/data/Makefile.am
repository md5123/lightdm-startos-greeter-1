greeterdatadir = $(datadir)/y-greeter

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
	go-hover.png


configdir = $(sysconfdir)/lightdm
dist_config_DATA = y-greeter.conf

xgreetersdir = $(datadir)/xgreeters

y-greeter.desktop: y-greeter.desktop.in
	sed -e 's,[@]BINDIR[@],$(sbindir),g' y-greeter.desktop.in > y-greeter.desktop

dist_xgreeters_DATA = y-greeter.desktop

DISTCLEANFILES = \
	Makefile.in \
	y-greeter.desktop
