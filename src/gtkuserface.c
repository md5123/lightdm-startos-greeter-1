/* vim: ts=4 sw=4 expandtab smartindent cindent */

/*
 * License: GPLv3
 * Copyright (C) Dongguan Vali Network Technology Co., Ltd.
 * Author: chen-qx@live.cn
 * Date: 2012-05
 * Description: A LightDM greeter for StartOS
 */

#include "gtkuserface.h"
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <utmpx.h>

#define FACE_SIZE 64
#define LOGIN_INDICATOR_SIZE 20

struct _GtkUserfacePrivate
{
	GdkWindow * event_window;
    GFile     * utmp_file;
    GFileMonitor * utmp_monitor;
	gint x, y;
    const GdkPixbuf *facepixbuf;
    const GdkPixbuf *facepixbuf_scale;
    const GdkPixbuf *login_indicator;
	const gchar * facepath;
    const gchar * username;
    gboolean long_name;
    gint name_x, name_w;
    PangoLayout *namelabel;
	gboolean hover;
	gboolean current_login;
    guint timeout_id;
};



static void gtk_userface_class_init (GtkUserfaceClass *klass);
static void gtk_userface_init (GtkUserface *userface);
static void gtk_userface_realize (GtkWidget *widget);
static void gtk_userface_unrealize (GtkWidget *widget);
static void gtk_userface_map (GtkWidget *widget);
static void gtk_userface_unmap (GtkWidget *widget);
static void gtk_userface_size_allocate (GtkWidget *widget, GtkAllocation *allocation);

static gboolean gtk_userface_draw (GtkWidget *widget, cairo_t *ctx);
static gboolean gtk_userface_focus (GtkWidget *widget, GtkDirectionType direction);

static gboolean gtk_userface_enter_notify (GtkWidget *widget, GdkEventCrossing *event);
static gboolean gtk_userface_leave_notify (GtkWidget *widget, GdkEventCrossing *event);
static gboolean username_slide_cb (GtkUserface * userface);

G_DEFINE_TYPE (GtkUserface, gtk_userface, GTK_TYPE_WIDGET);

static void check_current_user (GtkUserface *userface);
static void utmp_file_changed_cb (GFileMonitor * monitor,
        GFile * file,
        GFile * otherfile,
        GFileMonitorEvent event_type,
        GtkUserface * userface);

static void gtk_userface_class_init (GtkUserfaceClass *klass)
{
	g_type_class_add_private (klass, sizeof (GtkUserfacePrivate));
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->draw = gtk_userface_draw;
	widget_class->focus = gtk_userface_focus; /* Either leaving or comming, it will be emitted */
	widget_class->enter_notify_event = gtk_userface_enter_notify;
	widget_class->leave_notify_event = gtk_userface_leave_notify;
	widget_class->realize = gtk_userface_realize;
	widget_class->unrealize = gtk_userface_unrealize;
	widget_class->map = gtk_userface_map;
	widget_class->unmap = gtk_userface_unmap;
	widget_class->size_allocate = gtk_userface_size_allocate;
}

static void gtk_userface_init (GtkUserface *userface)
{
	userface->priv = GTK_USERFACE_GET_PRIVATE (userface);
	userface->priv->facepath = NULL;
	userface->priv->username = NULL;
	userface->priv->facepixbuf = NULL;
	userface->priv->hover = FALSE;
	userface->priv->x = 0;
	userface->priv->y = 0;
	userface->priv->name_x = 0;
	userface->priv->name_w = 0;
	userface->priv->long_name = FALSE;
	userface->priv->current_login = FALSE;
	userface->priv->timeout_id = 0;
    gtk_widget_set_has_window (GTK_WIDGET(userface), FALSE); 
    gtk_widget_set_can_focus (GTK_WIDGET(userface), TRUE);
    gtk_widget_set_can_default (GTK_WIDGET(userface), TRUE);
}

static gboolean gtk_userface_draw (GtkWidget *widget, cairo_t *ctx)
{
	GtkUserfacePrivate *priv = GTK_USERFACE(widget)->priv;
    GtkStyleContext * context = gtk_widget_get_style_context (widget);
    gtk_style_context_save (context);

	cairo_save (ctx);
    if (priv->facepixbuf_scale)
	{
        cairo_save (ctx);
        cairo_rectangle (ctx, 0, 0, FACE_SIZE, FACE_SIZE);
        cairo_clip (ctx);
		gdk_cairo_set_source_pixbuf (ctx, priv->facepixbuf_scale, 0, 0);
		cairo_paint (ctx);
        cairo_rectangle (ctx, 0, 0, FACE_SIZE, FACE_SIZE);
        cairo_set_line_width (ctx, 2.0);
        cairo_set_source_rgb (ctx, 1.0, 1.0, 1.0);
        cairo_stroke (ctx);
        cairo_restore (ctx);
	}
	gtk_render_layout (context, ctx, priv->name_x, FACE_SIZE, priv->namelabel);

    if (priv->hover)
    {
        cairo_rectangle (ctx, 0, 0, FACE_SIZE, FACE_SIZE);
        cairo_set_line_width (ctx, 4.0);
        cairo_set_source_rgb (ctx, 0xff / 255.0, 0x8d / 255.0, 0x27 / 255.0);
        cairo_stroke (ctx);
    }

    if (priv->current_login && priv->login_indicator)
    {
        cairo_save (ctx);
        cairo_translate (ctx, FACE_SIZE - LOGIN_INDICATOR_SIZE, FACE_SIZE - LOGIN_INDICATOR_SIZE);
        cairo_rectangle (ctx, 0, 0, LOGIN_INDICATOR_SIZE, LOGIN_INDICATOR_SIZE);
        cairo_clip (ctx);
		gdk_cairo_set_source_pixbuf (ctx, priv->login_indicator, 0, 0);
		cairo_paint (ctx);
        cairo_restore (ctx);
    }
    gtk_style_context_restore (context);
	cairo_restore (ctx);

	return FALSE;
}

static gboolean 
gtk_userface_focus (GtkWidget *widget, GtkDirectionType direction)
{
    if (!gtk_widget_is_focus (widget))
    {
        GTK_USERFACE(widget)->priv->hover = TRUE;
        gtk_widget_grab_focus (widget);
        return TRUE;
    }
    else
    {
        GTK_USERFACE(widget)->priv->hover = FALSE;
    }
    return FALSE;
}

GtkWidget * gtk_userface_new (const char *face_path, const char *username)
{
	GtkUserface *userface;

	userface = g_object_new (GTK_TYPE_USERFACE, NULL);
	userface->priv->facepath = face_path;
	userface->priv->username = username;
    g_thread_create ((GThreadFunc)check_current_user, userface, FALSE, NULL);
	return  GTK_WIDGET(userface);
}

static gboolean gtk_userface_enter_notify (GtkWidget *widget, GdkEventCrossing *event)
{
	GtkUserfacePrivate * priv = GTK_USERFACE(widget)->priv; 
    priv->hover = TRUE;
    if (priv->long_name)
        priv->timeout_id = g_timeout_add (40, (GSourceFunc)username_slide_cb, GTK_USERFACE(widget));
	gtk_widget_queue_draw (widget);
	return FALSE;
}

static gboolean gtk_userface_leave_notify (GtkWidget *widget, GdkEventCrossing *event)
{
	GtkUserfacePrivate * priv = GTK_USERFACE(widget)->priv;
    priv->hover = FALSE;
    if (priv->long_name)
    {
        priv->name_x = 0;
        g_source_remove (priv->timeout_id);
        priv->timeout_id = 0;
    }
	gtk_widget_queue_draw (widget);
	return FALSE;
}


static void gtk_userface_realize (GtkWidget *widget)
{
	GtkAllocation allocation;
	GdkWindow *parent_window;
	GdkWindowAttr attributes;
	gint attributes_mask;
	GtkUserfacePrivate *priv = GTK_USERFACE (widget)->priv;

    PangoAttrList  * pattrs;
    PangoAttribute * pattr;

	gtk_widget_set_realized (widget, TRUE);
	gtk_widget_get_allocation (widget, &allocation);

	if (priv->facepath)
	{
		if (!(priv->facepixbuf = gdk_pixbuf_new_from_file(priv->facepath, NULL)))
        {
            g_warning ("Open face image \"%s\"faild !\n", priv->facepath);
            priv->facepath = NULL;
        }
        priv->facepixbuf_scale = gdk_pixbuf_scale_simple (priv->facepixbuf, FACE_SIZE, FACE_SIZE, GDK_INTERP_BILINEAR); 
	}

    if (!(priv->login_indicator = gdk_pixbuf_new_from_file(GREETER_DATA_DIR "currentlogin.png", NULL)))
    {
        g_warning ("Open login indicator image \"%s\"faild !\n", GREETER_DATA_DIR "currentlogin.png");
    }

	attributes.x = allocation.x;
	attributes.y = allocation.y;
	attributes.width = allocation.width;
	attributes.height = allocation.height;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.wclass = GDK_INPUT_ONLY;
	attributes.event_mask = gtk_widget_get_events (widget) | 
                            GDK_BUTTON_PRESS_MASK |
                            GDK_BUTTON_RELEASE_MASK |
                            GDK_EXPOSURE_MASK |
                            GDK_ENTER_NOTIFY_MASK |
                            GDK_LEAVE_NOTIFY_MASK;
    attributes_mask = GDK_WA_X | GDK_WA_Y;

    parent_window = gtk_widget_get_parent_window (widget);

  	g_object_ref (parent_window);
    gtk_widget_set_window (widget, parent_window);
    priv->event_window = gdk_window_new (parent_window, &attributes, attributes_mask);
    gdk_window_set_user_data (priv->event_window, widget);

    /* Don't  change below code to other position in this function */
    priv->namelabel = gtk_widget_create_pango_layout (widget, priv->username);

    pattrs = pango_attr_list_new ();
    pattr  = pango_attr_foreground_new (0xffff, 0xffff, 0xffff);
    pango_attr_list_change (pattrs, pattr);
    pango_layout_set_attributes (priv->namelabel, pattrs);

	pango_layout_set_font_description (priv->namelabel, pango_font_description_from_string ("Sans 10"));
    pango_layout_get_pixel_size(priv->namelabel, &priv->name_w, NULL);
    if (priv->name_w <= FACE_SIZE)
    {
        priv->name_x = (FACE_SIZE - priv->name_w) / 2;
    }
    else
    {
        priv->long_name = TRUE;
    }
} 

static void gtk_userface_unrealize (GtkWidget *widget)
{
	GtkUserfacePrivate *priv = GTK_USERFACE(widget)->priv;
	gdk_window_set_user_data (priv->event_window, NULL);
	gdk_window_destroy (priv->event_window);
	priv->event_window = NULL;
    g_object_unref (priv->utmp_file);
    if (priv->utmp_monitor)
        g_object_unref (priv->utmp_monitor);
	GTK_WIDGET_CLASS(gtk_userface_parent_class)->unrealize (widget);
}

static void gtk_userface_map (GtkWidget *widget)
{
	GTK_WIDGET_CLASS(gtk_userface_parent_class)->map (widget);
	gdk_window_show (GTK_USERFACE(widget)->priv->event_window); /* instead widget receive event */
}

static void gtk_userface_unmap (GtkWidget *widget)
{
	gdk_window_hide (GTK_USERFACE(widget)->priv->event_window); 
	GTK_WIDGET_CLASS(gtk_userface_parent_class)->unmap (widget);
}

static void gtk_userface_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
    gtk_widget_set_allocation (widget, allocation);
    if (gtk_widget_get_realized (widget))
        gdk_window_move_resize (GTK_USERFACE(widget)->priv->event_window,
                                allocation->x, 
                                allocation->y, 
                                allocation->width, 
                                allocation->height);
}

static gboolean username_slide_cb (GtkUserface * userface)
{
    userface->priv->name_x -= 2;
    if (userface->priv->name_x < - (FACE_SIZE + userface->priv->name_w)) 
        userface->priv->name_x = FACE_SIZE;
	gtk_widget_queue_draw (GTK_WIDGET(userface));
    return TRUE;
}

static void utmp_file_changed_cb (GFileMonitor * monitor,
        GFile * file,
        GFile * otherfile,
        GFileMonitorEvent event_type,
        GtkUserface * userface)
{
    check_current_user (userface);
}


#define _PATH_UTMP  "/var/run/utmp"

static void 
check_current_user (GtkUserface *userface)
{
    static gboolean  firstrun = TRUE;
    GtkUserfacePrivate * priv = userface->priv;
    gboolean prev_val  = priv->current_login;
    struct utmpx *u;

    priv->current_login = FALSE;
    while ( (u = getutxent ()) != NULL)
    {
        if (u->ut_type == USER_PROCESS)
        {
            if (!g_strcmp0 (priv->username, u->ut_user))
            {
                priv->current_login = TRUE;
                break ;
            }
        }
    }
    endutxent ();
    if ((prev_val != priv->current_login) && gtk_widget_get_realized (GTK_WIDGET(userface)))
        gtk_widget_queue_draw (GTK_WIDGET(userface));

    if (firstrun)
    {
        GError * err = NULL;
        firstrun = FALSE;
        priv->utmp_file = g_file_new_for_path (_PATH_UTMP);
        priv->utmp_monitor =
            g_file_monitor_file (priv->utmp_file,
                    G_FILE_MONITOR_NONE,
                    NULL,
                    &err);
        if (err)
        {
            g_warning ("Monitor utmp file: %s", err->message);
            g_clear_error (&err);
        }
        else 
        {
            g_signal_connect (priv->utmp_monitor, "changed",  
                    G_CALLBACK(utmp_file_changed_cb), userface);
        }
    }
}

const gchar * gtk_userface_get_name (GtkUserface * userface)
{
    g_return_val_if_fail (GTK_IS_USERFACE(userface), NULL);
    return userface->priv->username;
}

const GdkPixbuf * gtk_userface_get_facepixbuf (GtkUserface * userface)
{
    g_return_val_if_fail (GTK_IS_USERFACE(userface), NULL);
    return userface->priv->facepixbuf;
}

const gchar * gtk_userface_get_facepath (GtkUserface * userface)
{
    g_return_val_if_fail (GTK_IS_USERFACE(userface), NULL);
    return userface->priv->facepath;
}
