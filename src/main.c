/* vim: ts=4 sw=4 expandtab smartindent cindent */

/*
 * License: GPLv3
 * Copyright (C) Dongguan Vali Network Technology Co., Ltd.
 * Author: chen-qx@live.cn
 * Date: 2012-05
 * Description: A LightDM greeter for StartOS
 */

#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include "backend.h"
#include "ui.h"
#include <stdlib.h>

void set_numlock (void);


void set_numlock ()
{
    unsigned char machine_type = 2;

    machine_type = chk_machine_type ();
    if (machine_type == 0x0a || machine_type == 0x09)
    {
        system("numlockx off");
    }
    else
    {
        system("numlockx on");
    }
}

int main (int argc, char *argv[])
{
	GtkWidget *rootwin;

    setlocale (LC_ALL, "");
    bindtextdomain (TEXTDOMAIN, LOCALEDIR);
    textdomain (TEXTDOMAIN);

    set_numlock ();

	gtk_init (&argc, &argv);

	if (!backend_init_greeter ())
		return 1;
    backend_set_screen_background (); /* set X Screen background, use it optionally at present. For future use. */
    backend_set_config (gtk_settings_get_default ());
	rootwin = ui_make_root_win (); 
	gdk_window_set_cursor (gdk_get_default_root_window (), gdk_cursor_new (GDK_LEFT_PTR));
	gtk_widget_show_all (rootwin);
    gtk_window_present (GTK_WINDOW(rootwin));
	gtk_main ();

    ui_finalize ();
    backend_finalize (); 

	return 0;
}
