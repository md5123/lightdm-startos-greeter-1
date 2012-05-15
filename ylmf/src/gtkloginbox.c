/* vim: ts=4 sw=4 expandtab smartindent cindent */
#include "config.h"
#include <gtk/gtk.h>
#include "gtkloginbox.h"
#include "gtkprompt.h"

enum 
{
    INPUT_READY_SIGNAL,
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

    container_class->add = gtk_login_box_add;
    container_class->forall = gtk_login_box_forall;
    container_class->remove = gtk_login_box_remove;
	signals_id[INPUT_READY_SIGNAL] = 
		g_signal_new ("input-ready", 
						GTK_TYPE_LOGIN_BOX, 
                        G_SIGNAL_RUN_LAST, 0, NULL, NULL, 
                        g_cclosure_marshal_VOID__STRING, 
                        G_TYPE_NONE, 1, G_TYPE_STRING);
    /* 
     * this Signal's callback: 
     * void (*input_ready_cb) (GtkLoginBox * box, const gchar *text, gpointer data); 
     */ 
}

static void gtk_login_box_init (GtkLoginBox *box)
{
    GtkWidget *widget;

    box->priv = GTK_LOGIN_BOX_GET_PRIVATE (box);
    gtk_widget_set_has_window(GTK_WIDGET(box), FALSE);

    widget = gtk_entry_new ();
    gtk_widget_set_can_focus (widget, TRUE);
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

    widget = gtk_button_new_with_label ("GO");
	gtk_button_set_focus_on_click (GTK_BUTTON(widget), FALSE);
	g_signal_connect (widget, "released", G_CALLBACK(entry_ready_cb), box);
    gtk_widget_show (widget);
    box->priv->loginbutton = GTK_BUTTON(widget);
    gtk_container_add (GTK_CONTAINER(box), widget);
    
    widget = gtk_prompt_new ("PROMPT");
    box->priv->prompt = (GtkPrompt *)widget;
    gtk_widget_set_visible (widget, FALSE);
    gtk_container_add (GTK_CONTAINER(box), widget);

}

static void entry_ready_cb (GtkWidget * widget, gpointer data)
{
	GtkLoginBox *box = GTK_LOGIN_BOX(data);
	g_signal_emit (box, signals_id[INPUT_READY_SIGNAL], 0, 
			gtk_entry_get_text(box->priv->input));
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
    child_allocation.height = 60;
    gtk_widget_size_allocate (GTK_WIDGET(box->priv->input), &child_allocation);

    child_allocation.y += 40 + 4;
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
    facepixbuf ? gtk_image_set_from_pixbuf (GTK_IMAGE(priv->userface), facepixbuf) : NULL;
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
