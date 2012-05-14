
#ifndef __BACKEND_H__
#define __BACKEND_H__


#include <gtk/gtk.h>
#include <cairo-xlib.h>

gboolean backend_init_greeter (void);

void backend_set_background (GKeyFile *configfile);
cairo_surface_t * backend_create_root_surface (GdkScreen *screen);
void backend_init_config (void);

void backend_authenticate_username_only (const gchar *username);
void backend_authenticate_process (const gchar *text);



#endif /* __BACKEND_H__ */
