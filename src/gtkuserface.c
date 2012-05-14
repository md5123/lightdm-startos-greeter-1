#include "gtkuserface.h"
#include <math.h>

struct _GtkUserfacePrivate
{
	GdkWindow *event_window;
	gint x, y;
    const GdkPixbuf *facepixbuf;
	const char * facepath;
    const char * username;
    gboolean long_name;
    gint name_x;
    PangoLayout *namelabel;
	gboolean hover;
};



static void gtk_userface_class_init (GtkUserfaceClass *klass);
static void gtk_userface_init (GtkUserface *userface);
static void gtk_userface_realize (GtkWidget *widget);
static void gtk_userface_unrealize (GtkWidget *widget);
static void gtk_userface_map (GtkWidget *widget);
static void gtk_userface_unmap (GtkWidget *widget);
static void gtk_userface_size_allocate (GtkWidget *widget, GtkAllocation *allocation);

static gboolean gtk_userface_draw (GtkWidget *widget, cairo_t *ctx);
static gboolean gtk_userface_enter_notify (GtkWidget *widget, GdkEventCrossing *event);
static gboolean gtk_userface_leave_notify (GtkWidget *widget, GdkEventCrossing *event);

G_DEFINE_TYPE (GtkUserface, gtk_userface, GTK_TYPE_WIDGET);

static void gtk_userface_class_init (GtkUserfaceClass *klass)
{
	g_type_class_add_private (klass, sizeof (GtkUserfacePrivate));
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->draw = gtk_userface_draw;
    /*
	widget_class->enter_notify_event = gtk_userface_enter_notify;
	widget_class->leave_notify_event = gtk_userface_leave_notify;
    */
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
    gtk_widget_set_has_window (GTK_WIDGET(userface), FALSE); 
	userface->priv->x = 0;
	userface->priv->y = 0;
	userface->priv->name_x = 0;
	userface->priv->long_name = FALSE;
}

static gboolean gtk_userface_draw (GtkWidget *widget, cairo_t *ctx)
{
	GtkUserfacePrivate *priv = GTK_USERFACE(widget)->priv;

	cairo_save (ctx);
    if (priv->facepixbuf)
	{
        cairo_save (ctx);
        cairo_rectangle (ctx, 0, 0, 36, 36);
        cairo_clip (ctx);
		gdk_cairo_set_source_pixbuf (ctx, gdk_pixbuf_scale_simple (priv->facepixbuf, 36, 36, GDK_INTERP_BILINEAR), 0, 0);
		cairo_paint (ctx);
        cairo_restore (ctx);
	}
	cairo_move_to (ctx, priv->name_x, 36);
	pango_cairo_show_layout (ctx, priv->namelabel);

	cairo_restore (ctx);

	return FALSE;
}

GtkWidget * gtk_userface_new (const char *face_path, const char *username)
{
	GtkUserface *userface;
	userface = g_object_new (GTK_TYPE_USERFACE, NULL);
	userface->priv->facepath = face_path;
	userface->priv->username = username;

	return  GTK_WIDGET(userface);
}

static gboolean gtk_userface_enter_notify (GtkWidget *widget, GdkEventCrossing *event)
{
	GTK_USERFACE(widget)->priv->hover = TRUE;
	gtk_widget_queue_draw (widget);
	return FALSE;
}

static gboolean gtk_userface_leave_notify (GtkWidget *widget, GdkEventCrossing *event)
{
	GTK_USERFACE(widget)->priv->hover = FALSE;
	gtk_widget_queue_draw (widget);
	return FALSE;
}


static void gtk_userface_realize (GtkWidget *widget)
{
	GtkAllocation allocation;
	GdkWindow *parent_window;
	GdkWindowAttr attributes;
	gint attributes_mask;
    gint name_w;
	GtkUserfacePrivate *priv = GTK_USERFACE (widget)->priv;

	gtk_widget_set_realized (widget, TRUE);
	gtk_widget_get_allocation (widget, &allocation);

	if (priv->facepath)
	{
		if (!(priv->facepixbuf = gdk_pixbuf_new_from_file(priv->facepath, NULL)))
        {
            g_debug("Open face image \"%s\"faild !\n", priv->facepath);
            priv->facepath = NULL;
        }
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
	pango_layout_set_font_description (priv->namelabel, pango_font_description_from_string ("Sans 10"));
    pango_layout_get_pixel_size(priv->namelabel, &name_w, NULL);
    if (name_w < 37)
    {
        priv->name_x = (36 - name_w) / 2;
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
	GTK_WIDGET_CLASS(gtk_userface_parent_class)->unrealize (widget);
}

static void gtk_userface_map (GtkWidget *widget)
{
	GTK_WIDGET_CLASS(gtk_userface_parent_class)->map (widget);
	gdk_window_show (GTK_USERFACE(widget)->priv->event_window); /* instead widget receive event */
}

static void gtk_userface_unmap (GtkWidget *widget)
{
	gdk_window_hide (GTK_USERFACE(widget)->priv->event_window); /* instead widget receive event */
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

const gchar * gtk_userface_get_name (GtkUserface * userface)
{
    g_return_val_if_fail (GTK_IS_USERFACE(userface), NULL);
    return GTK_USERFACE(userface)->priv->username;
}

const GdkPixbuf * gtk_userface_get_facepixbuf(GtkUserface * userface)
{
    g_return_val_if_fail (GTK_IS_USERFACE(userface), NULL);
    return GTK_USERFACE(userface)->priv->facepixbuf;
}
