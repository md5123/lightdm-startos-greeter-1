#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include "backend.h"
#include "ui.h"



int main (int argc, char *argv[])
{
	GtkWidget *rootwin;

    setlocale (LC_ALL, "");
    bindtextdomain (TEXTDOMAIN, LOCALEDIR);
    textdomain (TEXTDOMAIN);

	gtk_init (&argc, &argv);

	if (!backend_init_greeter ())
		return 1;
	gdk_window_set_cursor (gdk_get_default_root_window (), gdk_cursor_new (GDK_LEFT_PTR));
    backend_set_screen_background (); /* set X Screen background, use it optionally at present. For future use. */
    backend_set_config (gtk_settings_get_default ());
	rootwin = ui_make_root_win (); 
	gtk_widget_show_all (rootwin);
    gtk_window_present (GTK_WINDOW(rootwin));
	gtk_main ();

    ui_finalize ();
    backend_finalize (); 

	return 0;
}
