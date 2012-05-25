/* vim: ts=4 sw=4 expandtab smartindent cindent */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "gtkloginbox.h"
#include "gtkprompt.h"

enum 
{
    INPUT_READY_SIGNAL, 
    UPDATE_FACE_SIGNAL,
    LAST_SIGNAL
};

static guint signals_id[LAST_SIGNAL] = { 0 };

struct _GtkLoginBoxPrivate
{
    GList       * children;
    GtkEntry    * input;
    GtkImage    * userface;
    GtkLabel    * username;
    GtkButton   * loginbutton;
    GtkPrompt 	* prompt;
};

static void gtk_login_box_init (GtkLoginBox *box);
static void gtk_login_box_realize (GtkWidget *widget);
static gboolean gtk_login_box_draw (GtkWidget *widget, cairo_t * ctx);
static void gtk_login_box_add (GtkContainer *container, GtkWidget *widget);
static void gtk_login_box_class_init (GtkLoginBoxClass *klass);
static void gtk_login_box_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static void gtk_login_box_forall (GtkContainer *container, gboolean include_internals, 
                                    GtkCallback callback, gpointer callback_data);
static void gtk_login_box_remove (GtkContainer *container, GtkWidget *widget);

G_DEFINE_TYPE (GtkLoginBox, gtk_login_box, GTK_TYPE_CONTAINER);

static void entry_ready_cb (GtkWidget * widget, gpointer data);



static void gtk_login_box_class_init (GtkLoginBoxClass *klass)
{
    g_type_class_add_private (klass, sizeof (GtkLoginBoxPrivate));
    GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;
    GtkContainerClass *container_class = (GtkContainerClass *)klass;

    widget_class->size_allocate = gtk_login_box_size_allocate;
    widget_class->realize = gtk_login_box_realize;
    widget_class->draw = gtk_login_box_draw;

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

   signals_id[UPDATE_FACE_SIGNAL] = 
       g_signal_new ("update-face", 
						GTK_TYPE_LOGIN_BOX, 
                        G_SIGNAL_RUN_LAST, 0, NULL, NULL, 
                        g_cclosure_marshal_VOID__STRING, 
                        G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void gtk_login_box_init (GtkLoginBox *box)
{
    GtkWidget *widget;
    GtkBorder border;

    box->priv = GTK_LOGIN_BOX_GET_PRIVATE (box);
    gtk_widget_set_has_window(GTK_WIDGET(box), FALSE);

    widget = gtk_entry_new ();
    gtk_widget_set_size_request (widget, 200, 150); /* HELP: I have to do it, then I can change it's height 
                                                     * in _size_allocate(). If it is a bug of gtk+ ??? */
    border.left = 10;
    border.right = 10;
    border.top = 15;
    border.bottom = 15;
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
    gtk_widget_show (widget);
    box->priv->userface = (GtkImage *)widget;
    gtk_container_add (GTK_CONTAINER(box), widget);

    widget = gtk_label_new ("YLMF OS");
    box->priv->username = (GtkLabel *)widget;
    gtk_widget_show (widget);
    gtk_container_add (GTK_CONTAINER(box), widget);

    widget = gtk_button_new_with_label (_("Login"));
    gtk_button_set_relief (GTK_BUTTON(widget), GTK_RELIEF_NONE);
    gtk_widget_set_can_default (widget, FALSE);
    gtk_widget_set_can_focus (widget, FALSE);
	gtk_button_set_focus_on_click (GTK_BUTTON(widget), FALSE);
	g_signal_connect (widget, "released", G_CALLBACK(entry_ready_cb), box);
    gtk_widget_show (widget);
    box->priv->loginbutton = GTK_BUTTON(widget);
    gtk_container_add (GTK_CONTAINER(box), widget);
    
    widget = gtk_prompt_new (_("User Name"));
    gtk_widget_set_can_default (widget, FALSE);
    gtk_widget_set_can_focus (widget, FALSE);
    box->priv->prompt = (GtkPrompt *)widget;
    gtk_widget_set_visible (widget, FALSE);
    gtk_container_add (GTK_CONTAINER(box), widget);

}

static void entry_ready_cb (GtkWidget * widget, gpointer data)
{
	GtkLoginBox *box = GTK_LOGIN_BOX(data);
    gchar *text = g_strdup(gtk_entry_get_text(box->priv->input));
	g_signal_emit (box, signals_id[INPUT_READY_SIGNAL], 0, text);
    if (gtk_entry_get_visibility (box->priv->input)) 
    {
        gtk_label_set_label (box->priv->username, text);
        g_signal_emit (box, signals_id[UPDATE_FACE_SIGNAL], 0, text);
    }
    g_free (text);
}

static gboolean gtk_login_box_draw (GtkWidget *widget, cairo_t * ctx)
{
    GTK_WIDGET_CLASS(gtk_login_box_parent_class)->draw (widget, ctx);
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
    GtkAllocation  child_allocation; // = *allocation;
    GtkLoginBox  *box = GTK_LOGIN_BOX(widget);
    gtk_widget_set_allocation (widget, allocation);

    child_allocation.x = allocation->x + 2;
    child_allocation.y = allocation->y + 2;
    child_allocation.width = 96;
    child_allocation.height = 96;
    gtk_widget_size_allocate (GTK_WIDGET(box->priv->userface), &child_allocation);
        
    child_allocation.x = allocation->x + 2 + 96 + 10;
    child_allocation.y = allocation->y + 2;
    child_allocation.width = 150;
    child_allocation.height = 40;
    gtk_widget_size_allocate (GTK_WIDGET(box->priv->username), &child_allocation);

    child_allocation.x = allocation->x + 2 + 96 + 10;
    child_allocation.y = allocation->y + 2 + 40 + 2;
    child_allocation.width = 150;
    child_allocation.height = 30;
    gtk_widget_size_allocate (GTK_WIDGET(box->priv->input), &child_allocation);

    child_allocation.y = allocation->y + 2 + 40 + 2 + 30 + 4;
    child_allocation.width = 150;
    child_allocation.height = 25;
    gtk_widget_size_allocate (GTK_WIDGET(box->priv->prompt), &child_allocation);

    child_allocation.x += 150 + 10;
    child_allocation.y = allocation->y + 22;
    child_allocation.width = 60;
    child_allocation.height = 35;
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
                 gdk_pixbuf_scale_simple (facepixbuf, 96, 96, GDK_INTERP_BILINEAR));
    }
    name ? gtk_label_set_text (GTK_LABEL(priv->username), name) : NULL;
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
    gtk_prompt_set_text (GTK_LOGIN_BOX(box)->priv->prompt, text);
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
