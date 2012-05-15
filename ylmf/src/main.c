#include <gtk/gtk.h>
#include "backend.h"
#include "ui.h"



int main (int argc, char *argv[])
{
	GtkWidget *rootwin;

	gtk_init (&argc, &argv);

	if (!backend_init_greeter ())
		return 1;
	gdk_window_set_cursor (gdk_get_default_root_window (), gdk_cursor_new (GDK_LEFT_PTR));
    //backend_set_screen_background (); // set X window background, we not need at present, for future use.
    backend_set_config (gtk_settings_get_default ());
	rootwin = ui_make_root_win (); 
	gtk_widget_show_all (rootwin);
	gtk_main ();

	return 0;
}