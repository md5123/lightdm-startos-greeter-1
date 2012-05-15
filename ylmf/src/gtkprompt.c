#include <gtk/gtk.h>
#include "gtkprompt.h"

struct _GtkPromptPrivate
{
    GtkAllocation allocation;
};

G_DEFINE_TYPE (GtkPrompt, gtk_prompt, GTK_TYPE_LABEL);


static void gtk_prompt_class_init (GtkPromptClass * klass);
static void gtk_prompt_init (GtkPrompt * prompt);
static gboolean gtk_prompt_draw (GtkWidget * widget, cairo_t * ctx);
static void gtk_prompt_realize (GtkWidget * widget);

static void gtk_prompt_class_init (GtkPromptClass * klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    g_type_class_add_private (klass, sizeof (GtkPromptPrivate));
    widget_class->draw = gtk_prompt_draw;
    widget_class->realize = gtk_prompt_realize;
}

static void gtk_prompt_init (GtkPrompt * prompt)
{
    prompt->priv = GTK_PROMPT_GET_PRIVATE(prompt);
    gtk_widget_set_has_window (GTK_WIDGET(prompt), FALSE);
}

static gboolean gtk_prompt_draw (GtkWidget * widget, cairo_t * ctx)
{
    GtkPromptPrivate * priv = GTK_PROMPT(widget)->priv;

    cairo_save (ctx);

    cairo_move_to (ctx, 0, 5);
    cairo_line_to (ctx, 16, 5);
    cairo_line_to (ctx, 19, 0);
    cairo_line_to (ctx, 22, 5);
    cairo_line_to (ctx, priv->allocation.width, 5);
    cairo_line_to (ctx, priv->allocation.width, priv->allocation.height);
    cairo_line_to (ctx, 0, priv->allocation.height);
    cairo_line_to (ctx, 0, 5);
    cairo_set_line_width (ctx, 1.0);
	cairo_set_source_rgb (ctx, 104 / 255.0, 108 / 255.0, 77 / 255.0);
    cairo_stroke_preserve (ctx);
	cairo_set_source_rgb (ctx, 245 / 255.0, 207 / 255.0, 72 / 255.0);
	cairo_fill (ctx);

    cairo_translate (ctx, 0, 3); 
    GTK_WIDGET_CLASS(gtk_prompt_parent_class)->draw (widget, ctx);

    cairo_restore (ctx);

    return FALSE;
}

static void gtk_prompt_realize (GtkWidget * widget)
{
    GtkPromptPrivate * priv = GTK_PROMPT(widget)->priv;

    GTK_WIDGET_CLASS(gtk_prompt_parent_class)->realize (widget);
    gtk_widget_get_allocation (widget, &priv->allocation);
}

GtkWidget * gtk_prompt_new(const gchar *text)
{
    GtkPrompt *prompt = (GtkPrompt *)g_object_new (GTK_TYPE_PROMPT, NULL);
    if (text) 
        gtk_label_set_text (&prompt->label, text);

    return  GTK_WIDGET(prompt);
}

void gtk_prompt_set_text (GtkPrompt * prompt, const gchar *text)
{
    gtk_label_set_text (&prompt->label, text);
}
