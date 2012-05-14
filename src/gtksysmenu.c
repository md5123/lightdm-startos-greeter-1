
#include <gtk/gtk.h>
#include "gtksysmenu.h"

#define SYS_MENU_ITEM_FONT_SIZE 20
#define SYS_MENU_ITEM_WIDTH     80
#define SYS_MENU_ITEM_HEIGHT    20
#define SCREEN_HEIGHT           900

enum 
{
    USER_SELECTED,
    LAST_SIGNAL
};

struct _GtkSysMenuPrivate
{
    GdkWindow * event_window;
    GtkAllocation allocation;
    gint        start, end; /* view-area start item and end item */
    GList     * children;
    GtkSysMenuItem ** childindex;
    gint        selected;
	guint       amount;        /* items */ 
    gboolean    exceed;    /* Mark whether the amount of items exceed menu */
};

G_DEFINE_TYPE (GtkSysMenu, gtk_sys_menu, GTK_TYPE_WIDGET);

static void gtk_sys_menu_class_init (GtkSysMenuClass *klass);
static void gtk_sys_menu_init (GtkSysMenu *sysmenu);
static void gtk_sys_menu_realize (GtkWidget *widget);
static void gtk_sys_menu_unrealize (GtkWidget *widget);
static void gtk_sys_menu_map (GtkWidget *widget);
static void gtk_sys_menu_unmap (GtkWidget *widget);
static void gtk_sys_menu_size_allocate (GtkWidget *widget, GtkAllocation *allocation);

static gboolean gtk_sys_menu_draw (GtkWidget *widget, cairo_t *ctx);
static gboolean gtk_sys_menu_leave_notify (GtkWidget *widget, GdkEventCrossing *event);
static gboolean gtk_sys_menu_motion_notify_event (GtkWidget *widget, GdkEventMotion *event);
static gboolean gtk_sys_menu_release_event (GtkWidget *widget, GdkEventButton *event);


static void gtk_sys_menu_class_init (GtkSysMenuClass *klass)
{
	g_type_class_add_private (klass, sizeof (GtkSysMenuPrivate));
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->draw = gtk_sys_menu_draw;
	widget_class->leave_notify_event = gtk_sys_menu_leave_notify;
	widget_class->realize = gtk_sys_menu_realize;
	widget_class->motion_notify_event = gtk_sys_menu_motion_notify_event;
	widget_class->unrealize = gtk_sys_menu_unrealize;
	widget_class->map = gtk_sys_menu_map;
	widget_class->unmap = gtk_sys_menu_unmap;
	widget_class->size_allocate = gtk_sys_menu_size_allocate;
	widget_class->button_release_event = gtk_sys_menu_release_event;

    g_signal_new ("user-selected", GTK_TYPE_SYS_MENU,
            G_SIGNAL_RUN_LAST, 0, NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void gtk_sys_menu_init (GtkSysMenu *menu)
{
	menu->priv = GTK_SYS_MENU_GET_PRIVATE (menu);
    gtk_widget_set_has_window (GTK_WIDGET(menu), FALSE); 
	menu->priv->start  = 0;
	menu->priv->end    = 0;
	menu->priv->amount = 0;
	menu->priv->exceed = FALSE;
	menu->priv->children = NULL;
	menu->priv->selected = 0;
}

static gboolean gtk_sys_menu_leave_notify (GtkWidget *widget, GdkEventCrossing *event)
{
    gtk_widget_hide (widget);
	return FALSE;
}


static void gtk_sys_menu_realize (GtkWidget *widget)
{
    GtkAllocation allocation;
	GdkWindow *parent_window;
	GdkWindowAttr attributes;
	gint attributes_mask;
	GtkSysMenuPrivate *priv = GTK_SYS_MENU(widget)->priv;

	gtk_widget_set_realized (widget, TRUE);
	gtk_widget_get_allocation (widget, &allocation);

	attributes.x = allocation.x;
	attributes.y = allocation.y;
	attributes.width  = allocation.width;
	attributes.height = allocation.height;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.wclass      = GDK_INPUT_ONLY;
	attributes.event_mask  = gtk_widget_get_events (widget) | 
                             GDK_BUTTON_RELEASE_MASK |
                             GDK_BUTTON_PRESS_MASK |
                             GDK_EXPOSURE_MASK |
                             GDK_POINTER_MOTION_MASK |
                             GDK_POINTER_MOTION_HINT_MASK |
                             GDK_LEAVE_NOTIFY_MASK;

    attributes_mask = GDK_WA_X | GDK_WA_Y;
    parent_window = gtk_widget_get_parent_window (widget);

  	g_object_ref (parent_window);
    gtk_widget_set_window (widget, parent_window);
    priv->event_window = gdk_window_new (parent_window, &attributes, attributes_mask);
    gdk_window_set_user_data (priv->event_window, widget);

    /*
     * 使用指针数组,加快item定位速度 
     */

    GList *item;
    int i = 0;
    int w;

    priv->amount = g_list_length (priv->children);
    priv->childindex = (GtkSysMenuItem **)g_malloc(priv->amount * sizeof (GtkSysMenuItem *));
    for (item = priv->children; item; item = item->next, i++)
    {
        priv->childindex[i] = (GtkSysMenuItem *)(item->data);
        (*priv->childindex[i]).y = i * SYS_MENU_ITEM_HEIGHT; 
        (*priv->childindex[i]).x = 2; 
        (*priv->childindex[i]).layout = gtk_widget_create_pango_layout (widget, (*priv->childindex[i]).text);
        pango_layout_get_pixel_size ((*priv->childindex[i]).layout, &w, NULL);
        priv->allocation.width = MAX (priv->allocation.width, w);
        priv->selected = (*priv->childindex[i]).selected ? i : priv->selected;
    }
} 

static void gtk_sys_menu_unrealize (GtkWidget *widget)
{
	GtkSysMenuPrivate *priv = GTK_SYS_MENU(widget)->priv;
	gdk_window_set_user_data (priv->event_window, NULL);
	gdk_window_destroy (priv->event_window);
	priv->event_window = NULL;
    g_free (priv->childindex);
    priv->childindex = NULL;
	GTK_WIDGET_CLASS(gtk_sys_menu_parent_class)->unrealize (widget);
}

static void gtk_sys_menu_map (GtkWidget *widget)
{
	GTK_WIDGET_CLASS(gtk_sys_menu_parent_class)->map (widget);
	gdk_window_show (GTK_SYS_MENU(widget)->priv->event_window); // instead widget receive event 
}

static void gtk_sys_menu_unmap (GtkWidget *widget)
{
	gdk_window_hide (GTK_SYS_MENU(widget)->priv->event_window); 
	GTK_WIDGET_CLASS(gtk_sys_menu_parent_class)->unmap (widget);
}

static void gtk_sys_menu_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
    GtkSysMenuPrivate * priv = GTK_SYS_MENU(widget)->priv;
    guint height;

    priv->allocation.width = MAX (allocation->width, priv->allocation.width); 
    priv->allocation.height = MAX (allocation->height, priv->amount * SYS_MENU_ITEM_HEIGHT);
    if (priv->allocation.height > SCREEN_HEIGHT)
    {
        priv->allocation.height =  SCREEN_HEIGHT - 100;
        priv->exceed = TRUE;
    }
    priv->start = 0;
    height = priv->amount * SYS_MENU_ITEM_HEIGHT;
    priv->end = ((height < priv->allocation.height) ? height : priv->allocation.height) / SYS_MENU_ITEM_HEIGHT;
    priv->allocation.x = allocation->x;
    priv->allocation.y = SCREEN_HEIGHT - priv->allocation.height - 80;
    gtk_widget_set_allocation (widget, &priv->allocation);
    if (gtk_widget_get_realized (widget))
        gdk_window_move_resize (GTK_SYS_MENU(widget)->priv->event_window,
                                priv->allocation.x, 
                                priv->allocation.y, 
                                priv->allocation.width, 
                                priv->allocation.height);
    g_warning ("%d, %d, %d, %d",priv->allocation.x, priv->allocation.y, priv->allocation.width, priv->allocation.height);
}

static gboolean gtk_sys_menu_draw (GtkWidget *widget, cairo_t *ctx)
{
    gint i;
    cairo_save (ctx);
    GtkSysMenuPrivate * priv = GTK_SYS_MENU(widget)->priv;
    cairo_set_source_rgba (ctx, 0.0, 0.0, 0.0, 0.4);
    cairo_paint (ctx);

    cairo_select_font_face (ctx, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (ctx, SYS_MENU_ITEM_FONT_SIZE);
    cairo_set_source_rgb (ctx, 1.0, 1.0, 1.0);
    for (i = priv->start; i < priv->end; i++)
    {
        cairo_move_to (ctx, (*priv->childindex[i]).x, (*priv->childindex[i]).y);
        pango_cairo_show_layout (ctx, (*priv->childindex[i]).layout);
    }

    cairo_save (ctx);
    cairo_rectangle (ctx, 0, (*priv->childindex[priv->selected]).y, priv->allocation.width, SYS_MENU_ITEM_HEIGHT);
    cairo_clip (ctx);
    cairo_set_source_rgba (ctx, 0.0, 0.0, 0.0, 0.8);
    cairo_paint (ctx);
    cairo_restore (ctx);

    cairo_move_to (ctx, (*priv->childindex[priv->selected]).x, (*priv->childindex[priv->selected]).y);
    cairo_set_source_rgb (ctx, 1.0, 1.0, 1.0);
    pango_cairo_show_layout (ctx, (*priv->childindex[priv->selected]).layout);

    cairo_restore (ctx);
    
    return FALSE;
}

static gboolean gtk_sys_menu_motion_notify_event (GtkWidget *widget, GdkEventMotion *event)
{
    GtkSysMenuPrivate * priv = GTK_SYS_MENU(widget)->priv;

    if ((event->window != priv->event_window) || (event->y > (double)(priv->amount * SYS_MENU_ITEM_HEIGHT)))
        return FALSE;
    priv->selected = event->y / SYS_MENU_ITEM_HEIGHT + priv->start;
    (*priv->childindex[priv->selected]).selected = TRUE;
    gtk_widget_queue_draw (widget);
    //gtk_widget_queue_draw_area (widget, 2, (int)(event->y / SYS_MENU_ITEM_HEIGHT) * SYS_MENU_ITEM_HEIGHT, SYS_MENU_ITEM_WIDTH, SYS_MENU_ITEM_HEIGHT);

    return FALSE;
}

static gboolean gtk_sys_menu_release_event (GtkWidget *widget, GdkEventButton *event)
{
    GtkSysMenuPrivate * priv = GTK_SYS_MENU(widget)->priv;
    gtk_widget_hide (widget);
    GtkSysMenuItem selected_item;
    selected_item = *priv->childindex[priv->selected];

    g_debug("RELEASE-ITEM: %s\n", (*priv->childindex[priv->selected]).text);
    /* exec user's func, this help me forget the signal mechanism */
    if (selected_item.func) 
    {
        selected_item.func (&selected_item, selected_item.user_data);
    }
    return FALSE;
}

GtkWidget * gtk_sys_menu_new (GList * itemlist)
{
	GtkSysMenu *sysmenu;

    g_return_val_if_fail (itemlist, NULL);
	sysmenu = (GtkSysMenu *)g_object_new (GTK_TYPE_SYS_MENU, NULL);
    sysmenu->priv->children = itemlist;
    
	return  GTK_WIDGET(sysmenu);
}

GtkSysMenuItem * gtk_sys_menu_item_new (const char *text, void *func, gpointer user_data, gboolean selected)
{
    GtkSysMenuItem * item;

    item = (GtkSysMenuItem *)g_malloc0(sizeof(GtkSysMenuItem));
    item->text = text;
    item->func = func;
    item->user_data = user_data; 
    item->selected = selected;
    /* the others had initialized as 0 */
    return item;
}
