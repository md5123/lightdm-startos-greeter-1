
#include <gtk/gtk.h>
#include "gtksysmenu.h"

#define SYS_MENU_ITEM_FONT_SIZE 20
#define SYS_MENU_ITEM_WIDTH     80
#define SYS_MENU_ITEM_HEIGHT    20

#if !defined(SYS_BUTTON_HEIGHT)
#define SYS_BUTTON_HEIGHT    60
#endif

enum 
{
    USER_SELECTED,
    LAST_SIGNAL
};

struct _GtkSysMenuPrivate
{
    GdkWindow * event_window;
    GtkAllocation allocation;
    GtkSysMenuItem ** childindex;
    gint        start, end; /* view-area start item and end item */
    GList     * children;
    gint        selected;
    gint        hover;
	guint       amount;        /* items */ 
    gboolean    exceed;    /* Mark whether the amount of items exceed menu */
    gint        monitor_width, monitor_height;
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

    /*
    g_signal_new ("user-selected", GTK_TYPE_SYS_MENU,
            G_SIGNAL_RUN_LAST, 0, NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 1, G_TYPE_POINTER);
    */
}

static void gtk_sys_menu_init (GtkSysMenu *menu)
{
	menu->priv = GTK_SYS_MENU_GET_PRIVATE (menu);
    gtk_widget_set_has_window (GTK_WIDGET(menu), FALSE); 
	menu->priv->start  = 0;
	menu->priv->end    = 0;
	menu->priv->amount = 0;
	menu->priv->hover  = 0;
	menu->priv->exceed = FALSE;
	menu->priv->children = NULL;
	menu->priv->selected = -1;
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
	GTK_SYS_MENU(widget)->priv->hover = -1; 
	GTK_WIDGET_CLASS(gtk_sys_menu_parent_class)->unmap (widget);
}

/* 
 * if user not SIZE-REQUEST or unreasonable,  the sysmenu have to adjust it 
 * ::: x, y, width, height 
 */
static void gtk_sys_menu_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
    GtkSysMenuPrivate * priv = GTK_SYS_MENU(widget)->priv;
    guint height;
    GdkRectangle rectangle;
    GdkScreen  * screen;

    if (gtk_widget_get_realized (widget))
    {
        screen = gdk_window_get_screen (priv->event_window);
        gdk_screen_get_monitor_geometry (screen, gdk_screen_get_primary_monitor (screen), &rectangle);
        priv->monitor_height = rectangle.height;
        priv->monitor_width  = rectangle.width;
        priv->allocation.width  = MAX (allocation->width, priv->allocation.width); 
        priv->allocation.height = MAX (allocation->height, priv->amount * SYS_MENU_ITEM_HEIGHT);
        
        if (priv->allocation.height > priv->monitor_height)
        {
            priv->allocation.height = priv->monitor_height - SYS_BUTTON_HEIGHT;
            priv->exceed = TRUE;
        }
        priv->allocation.y = priv->monitor_height - priv->allocation.height - SYS_BUTTON_HEIGHT;
        if (allocation->x + priv->allocation.width > priv->monitor_width && allocation->x < priv->allocation.width)
            priv->allocation.x = allocation->x - priv->allocation.width;
        else
            priv->allocation.x = allocation->x;

        priv->start = 0;
        height = priv->amount * SYS_MENU_ITEM_HEIGHT;
        priv->end = ((height < priv->allocation.height) ? height : priv->allocation.height) / SYS_MENU_ITEM_HEIGHT;

        gtk_widget_set_allocation (widget, &priv->allocation);
        gdk_window_move_resize (priv->event_window,
                                priv->allocation.x, 
                                priv->allocation.y, 
                                priv->allocation.width, 
                                priv->allocation.height);
    }
    else
    {
        gtk_widget_set_allocation (widget, allocation);
    }
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


    if (priv->selected >= priv->start && priv->selected <= priv->end)
    {
        cairo_save (ctx);
        cairo_rectangle (ctx, 0, (*priv->childindex[priv->selected]).y, priv->allocation.width, SYS_MENU_ITEM_HEIGHT);
        cairo_clip (ctx);
        cairo_set_source_rgba (ctx, 0.0, 0.0, 0.0, 0.9);
        cairo_paint (ctx);
        cairo_restore (ctx);

        cairo_move_to (ctx, (*priv->childindex[priv->selected]).x, (*priv->childindex[priv->selected]).y);
        cairo_set_source_rgb (ctx, 1.0, 1.0, 1.0);
        pango_cairo_show_layout (ctx, (*priv->childindex[priv->selected]).layout);
    }

    if (priv->hover != -1)
    {
        cairo_save (ctx);
        cairo_rectangle (ctx, 0, (*priv->childindex[priv->hover]).y, priv->allocation.width, SYS_MENU_ITEM_HEIGHT);
        cairo_clip (ctx);
        cairo_set_source_rgba (ctx, 0.0, 0.0, 0.0, 0.4);
        cairo_paint (ctx);
        cairo_restore (ctx);

        cairo_move_to (ctx, (*priv->childindex[priv->hover]).x, (*priv->childindex[priv->hover]).y);
        cairo_set_source_rgb (ctx, 1.0, 1.0, 1.0);
        pango_cairo_show_layout (ctx, (*priv->childindex[priv->hover]).layout);
    }

    cairo_restore (ctx);
    
    return FALSE;
}

static gboolean gtk_sys_menu_motion_notify_event (GtkWidget *widget, GdkEventMotion *event)
{
    GtkSysMenuPrivate * priv = GTK_SYS_MENU(widget)->priv;

    if ((event->window != priv->event_window) || (event->y > (double)(priv->amount * SYS_MENU_ITEM_HEIGHT)))
        return FALSE;
    priv->hover = event->y / SYS_MENU_ITEM_HEIGHT + priv->start;
    gtk_widget_queue_draw (widget);
    /*
     gtk_widget_queue_draw_area (widget, 2, 
                 (int)(event->y / SYS_MENU_ITEM_HEIGHT) * SYS_MENU_ITEM_HEIGHT, 
                 SYS_MENU_ITEM_WIDTH, SYS_MENU_ITEM_HEIGHT);
    */

    return FALSE;
}

static gboolean gtk_sys_menu_release_event (GtkWidget *widget, GdkEventButton *event)
{
    GtkSysMenuPrivate * priv = GTK_SYS_MENU(widget)->priv;
    gtk_widget_hide (widget);
    GtkSysMenuItem *selected_item;
    
    priv->selected = event->y / SYS_MENU_ITEM_HEIGHT + priv->start;
    (*priv->childindex[priv->selected]).selected = TRUE;
    selected_item = priv->childindex[priv->selected];
    g_debug("RELEASE-ITEM: %s\n", (*priv->childindex[priv->selected]).text);
    if (selected_item->func) 
    {
        selected_item->func (selected_item, selected_item->func_data);
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

GtkSysMenuItem * gtk_sys_menu_item_new (const char *text, gpointer data, void *func, gpointer func_data, gboolean selected)
{
    GtkSysMenuItem * item;

    item = (GtkSysMenuItem *)g_malloc0(sizeof(GtkSysMenuItem));
    item->text = text;
    item->data = data;
    item->func = func;
    item->func_data = func_data; 
    item->selected = selected;
    
    return item;
}

const char * gtk_sys_menu_get_select_text (GtkSysMenu * menu)
{
    g_return_val_if_fail (GTK_IS_SYS_MENU(menu), NULL);
    return (menu->priv->selected > -1 ) ? menu->priv->childindex[menu->priv->selected]->text: NULL;
}

gpointer gtk_sys_menu_get_select_data (GtkSysMenu * menu)
{
    g_return_val_if_fail (GTK_IS_SYS_MENU(menu), NULL);
    return (menu->priv->selected > -1 ) ? menu->priv->childindex[menu->priv->selected]->data : NULL;
}