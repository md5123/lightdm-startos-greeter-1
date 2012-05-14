#include "config.h"
#include <gdk/gdkx.h>
#include <lightdm.h>
#include "backend.h"
#include "ui.h"


LightDMGreeter *global_greeter;
static GKeyFile * global_conf_keyfile = NULL;

static void show_prompt_cb (LightDMGreeter *greeter, const gchar *prompt, LightDMPromptType type);
static void show_message_cb (LightDMGreeter *greeter, const gchar *message, LightDMMessageType type);
static void authentication_complete_cb (LightDMGreeter *greeter);


static void show_prompt_cb (LightDMGreeter *greeter, const gchar *prompt, LightDMPromptType type)
{
	g_warning ("Prompt --- %s, T = %d\n", prompt, type);
    ui_set_prompt_text (prompt, type);
}

static void show_message_cb (LightDMGreeter *greeter, const gchar *message, LightDMMessageType type)
{
	g_warning ("Message --- %s, T = %d\n", message, type);
	ui_set_prompt_text (message, type);
}

static void authentication_complete_cb (LightDMGreeter *greeter)
{
	GError *error;
	ui_set_prompt_text ("", 0);

	if (lightdm_greeter_get_is_authenticated (global_greeter))
	{
		if (!lightdm_greeter_start_session_sync (global_greeter, NULL, &error))
		{
			ui_set_prompt_text (_("Failed to starting session"), 0);
			g_warning ("Failed to starting session: %s\n", error->message);
		}
	}
	else /* Authentication failed, Re-authenticate the current user */
	{
		ui_set_prompt_text (_("Authenticated Failed"), 1);
		backend_authenticate_process (lightdm_greeter_get_authentication_user (global_greeter));
	}
}

void backend_authenticate_username_only (const gchar *username)
{
	lightdm_greeter_authenticate (global_greeter, username); 
	/* add user to ~/.cache/ni/state lastuser */
}

void backend_authenticate_process (const gchar *text)
{
	GError *error;

	if (lightdm_greeter_get_is_authenticated (global_greeter))
	{
		if (!lightdm_greeter_start_session_sync (global_greeter, NULL, &error))
		{
			ui_set_prompt_text ("Failed to starting session", 0);
			g_warning ("Starting session: %s\n", error->message);
            g_clear_error (&error);
		}
	}
	else if (lightdm_greeter_get_in_authentication (global_greeter))
	{
		lightdm_greeter_respond (global_greeter, text); /* password */
	}
	else
	{
        backend_authenticate_username_only (text); /* username */
	}
}

gboolean backend_init_greeter ()
{	
	GError *error;

	global_greeter = lightdm_greeter_new ();
	if (!lightdm_greeter_connect_sync (global_greeter, &error))
	{
		g_debug ("Greeter connect Fail: %s\n", error->message);
		g_clear_error (&error);
		return FALSE;
	}
	g_signal_connect (G_OBJECT(global_greeter), "show-prompt", G_CALLBACK(show_prompt_cb), NULL); 
    g_signal_connect (G_OBJECT(global_greeter), "show-message", G_CALLBACK (show_message_cb), NULL); 
    g_signal_connect (G_OBJECT(global_greeter), "authentication-complete", G_CALLBACK (authentication_complete_cb), NULL);
	
	return TRUE;
}

void backend_init_config ()
{
	GError *error;

	global_conf_keyfile = g_key_file_new ();
	if (!g_key_file_load_from_file (global_conf_keyfile, GREETER_CONF_DIR"nq-greeter.conf", G_KEY_FILE_NONE, &error))
	{
		g_warning ("Failed to Loading Config file \"%s\" : %s\n", GREETER_CONF_DIR"nq-greeter.conf", error->message);
		g_clear_error (&error);
		g_key_file_free (global_conf_keyfile);
		global_conf_keyfile = NULL;
	}
}

void backend_set_background (GKeyFile *configfile)
{
	gchar *value;
	GError *error;
	GdkRGBA  bg_color;
	GdkPixbuf *bg_pixbuf = NULL;
    GdkRectangle monitor_geometry;

	value = g_key_file_get_value (global_conf_keyfile, "greeter", "background", NULL);
backend_reset_bg:
	if (!value)
	{
		value = g_strdup ("#000000");
	}
	if (gdk_rgba_parse (&bg_color, value))
	{
		g_warning ("Backgroud color %s\n", value);
		g_free (value);
	}
	else
	{
		gchar *path;

		if (g_path_is_absolute (value))
            path = g_strdup (value);
        else
            path = g_build_filename (GREETER_DATA_DIR, value, NULL);
		bg_pixbuf = gdk_pixbuf_new_from_file (path, &error);
		g_free (path);
		g_free (value);
		if(!bg_pixbuf)
		{
			g_warning ("Failed to load background: %s -- %s\n", path, error->message);
			g_clear_error (&error);
			goto backend_reset_bg;
		}
	}
	
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
	
/* ----- copy from lightdm-gtk-greeter ---- */
cairo_surface_t *
backend_create_root_surface (GdkScreen *screen)
{
    gint number, width, height;
    Display *display;
    Pixmap pixmap;
    cairo_surface_t *surface;

    number = gdk_screen_get_number (screen);
    width  = gdk_screen_get_width (screen);
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
