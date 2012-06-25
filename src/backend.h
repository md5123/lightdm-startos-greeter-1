
#ifndef __BACKEND_H__
#define __BACKEND_H__


#include <gtk/gtk.h>
#include <cairo-xlib.h>
#include <lightdm.h>


struct _backend_componet
{
    GKeyFile   * conffile;
    GKeyFile   * statekeyfile;
    char * statefile;
    LightDMGreeter * greeter;
};



gboolean backend_init_greeter (void);
void backend_finalize (void);
GKeyFile * open_key_file (const char *file);

void backend_set_screen_background (void);
void backend_authenticate_username_only (const gchar *username);
void backend_authenticate_process (const gchar *text);
void backend_set_config (GtkSettings * settings);
void backend_get_conf_background (GdkPixbuf ** bg_pixbuf, GdkRGBA *bg_color);
void backend_state_file_set_keyboard (const char * kb);
void backend_state_file_set_language (const char * lang);
void backend_state_file_set_session  (const char * session);

gchar * backend_state_file_get_session  (void);
gchar * backend_state_file_get_language (void);
gchar * backend_state_file_get_keyboard (void);
gchar * backend_state_file_get_user     (void);



#endif /* __BACKEND_H__ */
