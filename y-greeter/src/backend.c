#include <glib/gi18n.h>
#include "backend.h"
#include "ui.h"
#include <gdk/gdkx.h>
#include <lightdm.h>
#include <stdlib.h>


typedef struct _backend_componet backend_componet;

backend_componet back;
gboolean has_prompted = FALSE;

static void backend_open_config (void);
static void backend_open_state (void);

static void show_prompt_cb (LightDMGreeter *greeter, const gchar *prompt, LightDMPromptType type);
static void show_message_cb (LightDMGreeter *greeter, const gchar *message, LightDMMessageType type);
static void authentication_complete_cb (LightDMGreeter *greeter);
static void start_session (void);

static cairo_surface_t * backend_create_root_surface (GdkScreen *screen);

static void show_prompt_cb (LightDMGreeter *greeter, const gchar *prompt, LightDMPromptType type)
{

    /* 
     * FIXME: It must to do so & here, or login box can't 
     *        re-SENSITIVE. Any suggestion ?
     */
    ui_set_login_box_sensitive (TRUE);

    /*
    if (!has_prompted)
    {
        ui_set_prompt_text (dgettext("Linux-PAM", prompt), type);
    }
    */
}

static void show_message_cb (LightDMGreeter *greeter, const gchar *message, LightDMMessageType type)
{
	ui_set_prompt_text (dgettext("Linux-PAM", message), type);
}

static void start_session ()
{
	GError *error = NULL;
    const char  * session = NULL;
    const char  * language = NULL;

    session = ui_get_session ();
    language = ui_get_language ();

    backend_state_file_set_language (language);
    backend_state_file_set_session (session);
    

    language ? lightdm_greeter_set_language (back.greeter, language) : g_warning ("Get language NULL, use default"); 
    if (!session)
    {
        g_warning ("Get session NULL, use default");
    }
    g_warning ("lang = %s, session = %s", language, session);

    if (!lightdm_greeter_start_session_sync (back.greeter, NULL, &error))
    {
        g_warning ("Starting session: %s\n", error->message);
        g_clear_error (&error);
    }
}

void backend_state_file_set_session (const char * session)
{
    gsize length;
    char * data;
    g_return_if_fail (session);
    g_key_file_set_value (back.statekeyfile, "greeter", "last-session", session);
    data = g_key_file_to_data (back.statekeyfile, &length, NULL);
    g_warning ("Put session to state file: %s\n", session);
    g_file_set_contents (back.statefile, data, length, NULL);
    g_free (data);
}

void backend_state_file_set_language (const char * lang)
{
    gsize length;
    char * data;
    g_return_if_fail (lang);
    g_key_file_set_value (back.statekeyfile, "greeter", "last-language", lang);
    data = g_key_file_to_data (back.statekeyfile, &length, NULL);
    g_warning ("Put language to state file: %s\n", lang);
    g_file_set_contents (back.statefile, data, length, NULL);
    g_free (data);
}


void backend_state_file_set_keyboard (const char * kb)
{
    char * data;
    gsize length;
    g_return_if_fail (kb);
    g_key_file_set_value (back.statekeyfile, "greeter", "last-keyboard", kb);
    data = g_key_file_to_data (back.statekeyfile, &length, NULL);
    g_warning ("Put keyboard to state file: %s\n", kb);
    g_file_set_contents (back.statefile, data, length, NULL);
    g_free (data);
}

gchar * backend_state_file_get_session  ()
{
    return g_key_file_get_value (back.statekeyfile, "greeter", "last-session", NULL);
}

gchar * backend_state_file_get_language (void)
{
    return g_key_file_get_value (back.statekeyfile, "greeter", "last-language", NULL);
}

gchar * backend_state_file_get_keyboard (void)
{
    return g_key_file_get_value (back.statekeyfile, "greeter", "last-keyboard", NULL);
}

gchar * backend_state_file_get_user (void)
{
    return g_key_file_get_value (back.statekeyfile, "greeter", "last-user", NULL);
}


static void authentication_complete_cb (LightDMGreeter *greeter)
{
	//ui_set_prompt_text (NULL, 0);

	if (lightdm_greeter_get_is_authenticated (back.greeter))
	{
        start_session ();
	}
	else
	{
		ui_set_prompt_text (_("Authenticated Failed, Try Again"), 1);
        ui_set_prompt_show (TRUE);
        has_prompted = TRUE;
		backend_authenticate_process (lightdm_greeter_get_authentication_user (back.greeter));
	}
}

void backend_authenticate_username_only (const gchar *username)
{
    char * data;
    gsize length;
    gboolean truename;
    
    lightdm_greeter_authenticate (back.greeter, username);

    truename = username && *username;

    if (truename)
    {
        if (!has_prompted)
        {
            ui_set_prompt_text (NULL, 1); /* password */
            ui_set_prompt_show (FALSE);
        }
    }
    else
    {
        has_prompted = FALSE;
        ui_set_prompt_text (_("Login"), 0);
        ui_set_prompt_show (TRUE);
    }
    

    if (!truename)
        return ;

    g_key_file_set_value (back.statekeyfile, "greeter", "last-user", username);
    data = g_key_file_to_data (back.statekeyfile, &length, NULL);
    g_debug ("Put username to state file: %s\n", username);
    g_file_set_contents (back.statefile, data, length, NULL);
}

void backend_authenticate_process (const gchar *text)
{
	if (lightdm_greeter_get_is_authenticated (back.greeter))
	{
        start_session ();
	}
	else if (lightdm_greeter_get_in_authentication (back.greeter))
	{
        //has_prompted = TRUE;
        ui_set_prompt_show (FALSE);
        ui_set_prompt_text ("", 1); /* entry invisible */

		lightdm_greeter_respond (back.greeter, text); /* password */
	}
	else
	{
        backend_authenticate_username_only (text); /* username */
	}
}

gboolean backend_init_greeter ()
{	
	GError *error;

	back.greeter = lightdm_greeter_new ();
	if (!lightdm_greeter_connect_sync (back.greeter, &error))
	{
		g_critical ("Greeter connect Fail: %s\n", error->message);
		g_clear_error (&error);
		return FALSE;
	}
	g_signal_connect (back.greeter, "show-prompt", G_CALLBACK(show_prompt_cb), NULL); 
    g_signal_connect (back.greeter, "show-message", G_CALLBACK (show_message_cb), NULL); 
    g_signal_connect (back.greeter, "authentication-complete", G_CALLBACK (authentication_complete_cb), NULL);
	
    backend_open_config ();
    backend_open_state ();
	return TRUE;
}

static void backend_open_config ()
{
	back.conffile = open_key_file (GREETER_CONF_FILE);
}

static void backend_open_state ()
{
    char *state_dir;
    state_dir = g_build_filename (g_get_user_cache_dir (), APP_NAME, NULL);
    if (g_mkdir_with_parents (state_dir, 0755) < 0)
    {
        back.statefile = NULL;
        back.statekeyfile = NULL;
        g_free (state_dir);
        return ;
    }
    back.statefile = g_build_filename (state_dir, "state", NULL);
	back.statekeyfile = open_key_file (back.statefile);
    g_free (state_dir);
}

GKeyFile * open_key_file (const char *filepath)
{
	GError *error = NULL;
    GKeyFile *keyfile;
    g_return_val_if_fail (filepath, NULL);

	keyfile = g_key_file_new ();
	if (!g_key_file_load_from_file (keyfile, filepath, G_KEY_FILE_NONE, &error))
	{
		g_warning ("Failed to Loading file \"%s\" : %s", filepath, error->message);
		g_clear_error (&error);
	}
    return keyfile;
}

void backend_set_config (GtkSettings * settings)
{
    char * value = NULL;

    if (!back.conffile)
        return ;
    
    g_debug ("Set Configuration");
    if ((value = g_key_file_get_value (back.conffile, "greeter", "theme-name", NULL)))
    {
        g_object_set (settings, "gtk-theme-name", value, NULL);
        g_free (value);
    }

    if ((value = g_key_file_get_value (back.conffile, "greeter", "font-name", NULL)))
    {
        g_object_set (settings, "gtk-font-name", value, NULL);
        g_free (value);
    }

    if ((value = g_key_file_get_value (back.conffile, "greeter", "xft-antialias", NULL)))
    {
        g_object_set (settings, "gtk-xft-antialias", g_strcmp0 (value, "true") == 0, NULL);
        g_free (value);
    }

    if ((value = g_key_file_get_value (back.conffile, "greeter", "xft-dpi", NULL)))
    {
        g_object_set (settings, "gtk-xft-dpi", (int) (1024 * atof (value)), NULL);
        g_free (value);
    }

    if ((value = g_key_file_get_value (back.conffile, "greeter", "xft-hintstyle", NULL)))
    {
        g_object_set (settings, "gtk-xft-hintstyle", value, NULL);
        g_free (value);
    }

    if ((value = g_key_file_get_value (back.conffile, "greeter", "xft-rgba", NULL)))
    {
        g_object_set (settings, "gtk-xft-rgba", value, NULL);
        g_free (value);
    }
}

void backend_finalize ()
{
    g_key_file_free (back.conffile);
    g_key_file_free (back.statekeyfile);
    g_free (back.statefile);
}

void backend_get_conf_background (GdkPixbuf ** bg_pixbuf, GdkRGBA *bg_color)
{
	GError *error = NULL;
	gchar *value;

    value = g_key_file_get_value (back.conffile, "greeter", "background", NULL);
BACKEND_RESET_BG:
	if (!value)
	{
		value = g_strdup ("#1F6492");
	}
	if (gdk_rgba_parse (bg_color, value))
	{
		g_debug ("Backgroud color %s\n", value);
	    g_free (value);
	}
	else
	{
		gchar *path;

		if (g_path_is_absolute (value))
            path = g_strdup (value);
        else
            path = g_build_filename (GREETER_DATA_DIR, value, NULL);
		*bg_pixbuf = gdk_pixbuf_new_from_file (path, &error);
		g_debug ("Backgroud picture %s\n", path);
		g_free (path);
		g_free (value);
        value = NULL;
		if(!(*bg_pixbuf))
		{
			g_warning ("Failed to load background: %s -- %s\n", path, error->message);
			g_clear_error (&error);
			goto BACKEND_RESET_BG;
		}
	}
}

void backend_set_screen_background ()
{
	GdkRGBA  bg_color;
    GdkPixbuf * bg_pixbuf = NULL;
    GdkRectangle monitor_geometry;

	
    backend_get_conf_background (&bg_pixbuf, &bg_color);
	/* Set the background */
	int i;
    for (i = 0; i < gdk_display_get_n_screens (gdk_display_get_default ()); i++)
    {
        GdkScreen *screen;
        cairo_surface_t *surface;
        cairo_t *c;
        int monitor;

        screen = gdk_display_get_screen (gdk_display_get_default (), i);
        surface = backend_create_root_surface (screen);
        c = cairo_create (surface);

        for (monitor = 0; monitor < gdk_screen_get_n_monitors (screen); monitor++)
        {
            gdk_screen_get_monitor_geometry (screen, monitor, &monitor_geometry);

            if (bg_pixbuf)
            {
                GdkPixbuf *pixbuf = gdk_pixbuf_scale_simple (bg_pixbuf, monitor_geometry.width, monitor_geometry.height, GDK_INTERP_BILINEAR);
                gdk_cairo_set_source_pixbuf (c, pixbuf, monitor_geometry.x, monitor_geometry.y);
                g_object_unref (pixbuf);
            }
            else
                gdk_cairo_set_source_rgba (c, &bg_color);
            cairo_paint (c);
        }

        cairo_destroy (c);

        /* Refresh background */
        gdk_flush ();
        XClearWindow (GDK_SCREEN_XDISPLAY (screen), RootWindow (GDK_SCREEN_XDISPLAY (screen), i));
    }
    if (bg_pixbuf)
        g_object_unref (bg_pixbuf);
}
	
/* 
 * ----- copy from lightdm-gtk-greeter ---- 
 * Many Thanks to its author: Robert Ancell
 */
static cairo_surface_t *
backend_create_root_surface (GdkScreen *screen)
{
    gint number, width, height;
    Display *display;
    Pixmap pixmap;
    cairo_surface_t *surface;

    number = gdk_screen_get_number (screen);
    width  = gdk_screen_get_width  (screen);
    height = gdk_screen_get_height (screen);

    /* Open a new connection so with Retain Permanent so the pixmap remains when the greeter quits */
    gdk_flush ();
    display = XOpenDisplay (gdk_display_get_name (gdk_screen_get_display (screen)));
    if (!display)
    {
        g_warning ("Failed to create root pixmap");
        return NULL;
    }
    XSetCloseDownMode (display, RetainPermanent);
    pixmap = XCreatePixmap (display, RootWindow (display, number), width, height, DefaultDepth (display, number));
    XCloseDisplay (display);

    /* Convert into a Cairo surface */
    surface = cairo_xlib_surface_create (GDK_SCREEN_XDISPLAY (screen),
                                         pixmap,
                                         GDK_VISUAL_XVISUAL (gdk_screen_get_system_visual (screen)),
                                         width, height);

    /* Use this pixmap for the background */
    XSetWindowBackgroundPixmap (GDK_SCREEN_XDISPLAY (screen),
                                RootWindow (GDK_SCREEN_XDISPLAY (screen), number),
                                cairo_xlib_surface_get_drawable (surface));

    return surface;  
}
