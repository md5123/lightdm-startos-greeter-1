/* vim: ts=4 sw=4 expandtab smartindent cindent */

/*
 * License: GPLv3
 * Copyright: vali 
 * Author: chen-qx@live.cn
 * Date: 2012-05
 * Description: A developing LightDM greeter for YLMF OS 5
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "gtkloginbox.h"
#include "gtkprompt.h"

#define UFACE_W  150
#define UFACE_H  UFACE_W
#define UNAME_W  250
#define UNAME_H  40
#define SPACING  2
#define PROMPT_W 244
#define PROMPT_H 40
#define INPUT_W  250
#define INPUT_H  50
#define BT_W     80
#define BT_H     45


enum 
{
    INPUT_READY_SIGNAL, 
    UPDATE_FACE_NAME_SIGNAL,
    REBORN_SIGNAL,
    LAST_SIGNAL
};

static guint signals_id[LAST_SIGNAL] = { 0 };

struct _GtkLoginBoxPrivate
{
    GList       * children;
    GtkEntry    * input;
    GtkImage    * userface;
    PangoLayout * username;
    GtkEventBox * loginbutton;
    GtkPrompt 	* prompt;
    gint          username_x, username_y;
};

static void gtk_login_box_class_init (GtkLoginBoxClass *klass);
static void gtk_login_box_init (GtkLoginBox *box);
static void gtk_login_box_realize (GtkWidget *widget);
static void gtk_login_box_add (GtkContainer *container, GtkWidget *widget);
static gboolean gtk_login_box_draw (GtkWidget *widget, cairo_t * ctx);
static gboolean gtk_login_box_key_press_event (GtkWidget * widget, GdkEventKey * event);
static void gtk_login_box_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static void gtk_login_box_forall (GtkContainer *container, gboolean include_internals, 
                                    GtkCallback callback, gpointer callback_data);
static void gtk_login_box_remove (GtkContainer *container, GtkWidget *widget);

G_DEFINE_TYPE (GtkLoginBox, gtk_login_box, GTK_TYPE_CONTAINER);

static void entry_ready_cb (GtkWidget * widget, gpointer data);
static gboolean login_button_enter_cb (GtkWidget * widget, GdkEvent * event, gpointer data);
static gboolean login_button_leave_cb (GtkWidget * widget, GdkEvent * event, gpointer data);
static gboolean login_button_release_cb (GtkWidget * widget, GdkEvent * event, gpointer data);


static gboolean gtk_login_box_key_press_event (GtkWidget * widget, GdkEventKey * event)
{
    if (event->hardware_keycode == 9) /* ESC */
        g_signal_emit (GTK_LOGIN_BOX(widget), signals_id[REBORN_SIGNAL], 0);

    return FALSE;
}


static void gtk_login_box_class_init (GtkLoginBoxClass *klass)
{
    g_type_class_add_private (klass, sizeof (GtkLoginBoxPrivate));
    GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;
    GtkContainerClass *container_class = (GtkContainerClass *)klass;

    widget_class->size_allocate = gtk_login_box_size_allocate;
    widget_class->realize = gtk_login_box_realize;
    widget_class->draw = gtk_login_box_draw;
	widget_class->key_press_event = gtk_login_box_key_press_event;

    container_class->add = gtk_login_box_add;
    container_class->forall = gtk_login_box_forall;
    container_class->remove = gtk_login_box_remove;
	/* 
     * proto type:
     * void (*input_ready_cb) (GtkLoginBox * box, const gchar *text, gpointer data); 
     */ 
    signals_id[INPUT_READY_SIGNAL] = 
		g_signal_new ("input-ready", 
						GTK_TYPE_LOGIN_BOX, 
                        G_SIGNAL_RUN_LAST, 0, NULL, NULL, 
                        g_cclosure_marshal_VOID__STRING, 
                        G_TYPE_NONE, 1, G_TYPE_STRING);

    signals_id[UPDATE_FACE_NAME_SIGNAL] = 
       g_signal_new ("update-face-name", 
						GTK_TYPE_LOGIN_BOX, 
                        G_SIGNAL_RUN_LAST, 0, NULL, NULL, 
                        g_cclosure_marshal_VOID__STRING, 
                        G_TYPE_NONE, 1, G_TYPE_STRING);

    signals_id[REBORN_SIGNAL] = 
       g_signal_new ("reborn", 
						GTK_TYPE_LOGIN_BOX, 
                        G_SIGNAL_RUN_LAST, 0, NULL, NULL, 
                        g_cclosure_marshal_VOID__VOID, 
                        G_TYPE_NONE, 0);
}

static void gtk_login_box_init (GtkLoginBox *box)
{
    GtkWidget *widget;
    GtkWidget *image;
    GtkBorder border;

    box->priv = GTK_LOGIN_BOX_GET_PRIVATE (box);
    gtk_widget_set_has_window(GTK_WIDGET(box), FALSE);
    gtk_widget_set_can_focus (GTK_WIDGET(box), FALSE);

    widget = gtk_entry_new ();
    border.left = 12;
    border.right = 12;
    border.top = 16;
    border.bottom = 16;
    gtk_entry_set_inner_border (GTK_ENTRY(widget), &border);
    gtk_widget_set_can_default (widget, TRUE);
    gtk_widget_set_can_focus (widget, TRUE);
    gtk_widget_set_receives_default (widget, TRUE);
    gtk_widget_grab_focus (widget);
	g_signal_connect (widget, "activate", G_CALLBACK(entry_ready_cb), box);
    gtk_entry_set_has_frame (GTK_ENTRY(widget), FALSE);
    gtk_widget_show (widget);
    box->priv->input = (GtkEntry *)widget;
    gtk_container_add (GTK_CONTAINER(box), widget);

    widget = gtk_image_new_from_file (GREETER_DATA_DIR"defaultface.png");
    gtk_widget_set_can_focus (widget, FALSE);
    gtk_widget_show (widget);
    box->priv->userface = (GtkImage *)widget;
    gtk_container_add (GTK_CONTAINER(box), widget);

    widget = gtk_event_box_new ();
    image = gtk_image_new ();
    gtk_widget_set_size_request (image, BT_W, BT_H);
    gtk_container_add (GTK_CONTAINER(widget), image);
    gtk_image_set_from_file (GTK_IMAGE(image), GREETER_DATA_DIR"go-normal.png");
    gtk_event_box_set_visible_window (GTK_EVENT_BOX(widget), FALSE);
	g_signal_connect (widget, "button-release-event", G_CALLBACK(login_button_release_cb), box);
    g_signal_connect (widget, "enter-notify-event", G_CALLBACK(login_button_enter_cb), image);
    g_signal_connect (widget, "leave-notify-event", G_CALLBACK(login_button_leave_cb), image);

    gtk_widget_show (widget);
    box->priv->loginbutton = GTK_EVENT_BOX(widget);
    gtk_container_add (GTK_CONTAINER(box), widget);
    
    widget = gtk_prompt_new (_("User Name"));
    box->priv->prompt = (GtkPrompt *)widget;
    gtk_widget_set_visible (widget, FALSE);
    gtk_container_add (GTK_CONTAINER(box), widget);

    PangoFontDescription * pfd;
    PangoAttrList  * pattrs;
    PangoAttribute * pattr;


    pfd = pango_font_description_from_string ("Sans 32");
    box->priv->username = gtk_widget_create_pango_layout (widget, "");
    pango_layout_set_font_description (box->priv->username, pfd);
    pango_font_description_free (pfd);
    /*
    pango_layout_set_width (box->priv->username, 237568);
    pango_layout_set_height (box->priv->username, 20480);
    */
    pango_layout_set_width (box->priv->username, 300000);
    pango_layout_set_height (box->priv->username, 50);
    pango_layout_set_alignment (box->priv->username, PANGO_ALIGN_LEFT);
    pattrs = pango_attr_list_new ();
    pattr  = pango_attr_foreground_new (0xffff, 0xffff, 0xffff); 
    pango_attr_list_change (pattrs, pattr);
    pango_layout_set_attributes (box->priv->username, pattrs);

    //pango_attribute_destroy (pattr);
    //pango_attr_list_unref (pattrs);

    box->priv->username_x = 0;
    box->priv->username_y = 0;
}

static gboolean login_button_leave_cb (GtkWidget * widget, GdkEvent * event, gpointer data)
{
    gtk_image_set_from_file (GTK_IMAGE(data), GREETER_DATA_DIR"go-normal.png");
    return TRUE;
}

static gboolean login_button_enter_cb (GtkWidget * widget, GdkEvent * event, gpointer data)
{
    gtk_image_set_from_file (GTK_IMAGE(data), GREETER_DATA_DIR"go-hover.png");
    return TRUE;
}

static gboolean login_button_release_cb (GtkWidget * widget, GdkEvent * event, gpointer data)
{
    entry_ready_cb (widget, data);
    return TRUE;
}

static void entry_ready_cb (GtkWidget * widget, gpointer data)
{
	GtkLoginBox *box = GTK_LOGIN_BOX(data);
    gchar *text = g_strdup(gtk_entry_get_text(box->priv->input));
    if (!(text && *text))
        return ;
    if (gtk_entry_get_visibility (box->priv->input)) 
    {
        g_signal_emit (box, signals_id[UPDATE_FACE_NAME_SIGNAL], 0, text);
    }
    gtk_widget_queue_draw (widget);
	g_signal_emit (box, signals_id[INPUT_READY_SIGNAL], 0, text);
    g_free (text);
}

static gboolean gtk_login_box_draw (GtkWidget *widget, cairo_t * ctx)
{
    cairo_save (ctx);
    GtkLoginBoxPrivate * priv = GTK_LOGIN_BOX(widget)->priv;
    GTK_WIDGET_CLASS(gtk_login_box_parent_class)->draw (widget, ctx);
    cairo_rectangle (ctx, 2, 2, UFACE_W, UFACE_H);
    cairo_set_line_width (ctx, 2);
    cairo_set_source_rgb (ctx, 1.0, 1.0, 1.0);
    cairo_stroke (ctx);
    cairo_restore (ctx);

    cairo_move_to (ctx, priv->username_x, priv->username_y);
    pango_cairo_show_layout (ctx, priv->username);

    return FALSE;
}

static void gtk_login_box_realize (GtkWidget *widget)
{
    GdkWindow * parent_window ;
    GtkLoginBoxPrivate * priv = GTK_LOGIN_BOX(widget)->priv;
    guint event_mask = 0;
    GList *item;

    GTK_WIDGET_CLASS(gtk_login_box_parent_class)->realize (widget);
    gtk_widget_set_realized (widget, TRUE);
    event_mask = GDK_EXPOSURE_MASK | 
                 GDK_POINTER_MOTION_MASK | 
                 GDK_BUTTON_PRESS_MASK | 
                 GDK_BUTTON_RELEASE_MASK | 
                 GDK_SCROLL_MASK | 
                 GDK_ENTER_NOTIFY_MASK | 
                 GDK_LEAVE_NOTIFY_MASK;
    gtk_widget_add_events (widget, event_mask);
    parent_window = gtk_widget_get_parent_window (widget);
    gtk_widget_set_window (widget, parent_window);

    parent_window = gtk_widget_get_window (widget);
    for (item = priv->children; item; item = item->next)
    {
        gtk_widget_set_parent_window (GTK_WIDGET(item->data), parent_window);
    }
}

static void 
gtk_login_box_size_allocate (GtkWidget *widget, GtkAllocation * allocation)
{
    GtkAllocation  child_allocation;
    GtkLoginBox  *box = GTK_LOGIN_BOX(widget);
    gtk_widget_set_allocation (widget, allocation);

    child_allocation.x = allocation->x + SPACING;
    child_allocation.y = allocation->y + SPACING;
    child_allocation.width = UFACE_W;
    child_allocation.height = UFACE_H;
    gtk_widget_size_allocate (GTK_WIDGET(box->priv->userface), &child_allocation);

    box->priv->username_x = SPACING + UFACE_W + 15;
    box->priv->username_y = SPACING;

    child_allocation.x = allocation->x + SPACING + UFACE_W + 15;
    child_allocation.y = allocation->y + SPACING + UNAME_H + 15;
    child_allocation.width  = INPUT_W;
    child_allocation.height = INPUT_H;
    gtk_widget_size_allocate (GTK_WIDGET(box->priv->input), &child_allocation);

    child_allocation.x = allocation->x + SPACING + UFACE_W + 15;
    child_allocation.y = allocation->y + SPACING + UNAME_H + 15 + INPUT_H + SPACING;
    child_allocation.width = PROMPT_W;
    child_allocation.height = PROMPT_H;
    gtk_widget_size_allocate (GTK_WIDGET(box->priv->prompt), &child_allocation);

    child_allocation.x = allocation->x + SPACING + UFACE_W + 15 + INPUT_W + 15;
    child_allocation.y = allocation->y + SPACING + UNAME_H + 15;
    child_allocation.width = BT_W;
    child_allocation.height = BT_H;
    gtk_widget_size_allocate (GTK_WIDGET(box->priv->loginbutton), &child_allocation);
}


GtkWidget * gtk_login_box_new ()
{
    return (GtkWidget *)g_object_new (GTK_TYPE_LOGIN_BOX, NULL);
}

static void gtk_login_box_add (GtkContainer *container, GtkWidget *widget)
{
    GtkLoginBoxPrivate *priv = GTK_LOGIN_BOX(container)->priv;
    priv->children = g_list_append (priv->children, widget);
    if (gtk_widget_get_realized (GTK_WIDGET(container)))
        gtk_widget_set_parent_window (widget, gtk_widget_get_window (GTK_WIDGET(container)));
    gtk_widget_set_parent (widget, GTK_WIDGET(container));
}

static void gtk_login_box_forall (GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data)
{
  GtkWidget * child;
  GList *children;

  children = GTK_LOGIN_BOX(container)->priv->children;
  while (children)
  {
      child = GTK_WIDGET(children->data);
      children = children->next;

      (* callback) (child, callback_data);
  }
}

static void gtk_login_box_remove (GtkContainer *container, GtkWidget *widget)
{
    gtk_widget_unparent (widget);
    GTK_LOGIN_BOX(container)->priv->children = g_list_remove (GTK_LOGIN_BOX(container)->priv->children, widget);
}

void gtk_login_box_update_face_name (GtkLoginBox *box, GdkPixbuf *facepixbuf, const gchar *name)
{
    GtkLoginBoxPrivate * priv = NULL; 
    g_return_if_fail (GTK_IS_LOGIN_BOX(box));
    priv = GTK_LOGIN_BOX(box)->priv;
    if (facepixbuf)   
    {
        gtk_image_set_from_pixbuf (GTK_IMAGE(priv->userface), 
                 gdk_pixbuf_scale_simple (facepixbuf, UFACE_W, UFACE_H, GDK_INTERP_BILINEAR));
    }
    if (name && *name)
    {
        pango_layout_set_text (priv->username, name, -1);
        gtk_widget_hide (GTK_WIDGET(priv->prompt));
    }
    else
    {
        pango_layout_set_text (priv->username, _("User Name"), -1);
    }
    gtk_widget_queue_draw (GTK_WIDGET(box));
}

const gchar * gtk_login_box_get_input (GtkLoginBox *box)
{
    g_return_val_if_fail (GTK_IS_LOGIN_BOX(box), "");
    return gtk_entry_get_text (GTK_LOGIN_BOX(box)->priv->input);
}

void  gtk_login_box_set_input (GtkLoginBox *box, const char *text)
{
    g_return_if_fail (GTK_IS_LOGIN_BOX(box));
    gtk_entry_set_text (GTK_LOGIN_BOX(box)->priv->input, text);
}

void  gtk_login_box_set_prompt (GtkLoginBox *box, const char *text)
{
    g_return_if_fail (GTK_IS_LOGIN_BOX(box));
    gtk_prompt_set_text (box->priv->prompt, text);
}

void  gtk_login_box_set_input_visible (GtkLoginBox *box, gboolean setting)
{
    g_return_if_fail (GTK_IS_LOGIN_BOX(box));
    gtk_entry_set_visibility (box->priv->input, setting);
}

void gtk_login_box_set_input_focus (GtkLoginBox *box)
{
    g_return_if_fail (GTK_IS_LOGIN_BOX(box));
    gtk_widget_grab_focus (GTK_WIDGET(box->priv->input));
}

void gtk_login_box_set_prompt_show (GtkLoginBox *box, gboolean setting)
{
    g_return_if_fail (GTK_IS_LOGIN_BOX(box));
    if (setting)
        gtk_widget_show (GTK_WIDGET(box->priv->prompt));
    else 
        gtk_widget_hide (GTK_WIDGET(box->priv->prompt));
}
