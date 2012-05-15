
#ifndef __PROMPT_H__
#define __PROMPT_H__

#include <gtk/gtk.h>

typedef  struct _GtkPrompt GtkPrompt;
typedef  struct _GtkPromptClass GtkPromptClass;
typedef  struct _GtkPromptPrivate GtkPromptPrivate;


struct _GtkPrompt
{
	GtkLabel label;
    GtkPromptPrivate * priv;
};

struct _GtkPromptClass
{
	GtkLabelClass parent_class;
};

#define GTK_TYPE_PROMPT 	(gtk_prompt_get_type())
#define GTK_PROMPT(obj) 	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_PROMPT ,GtkPrompt))
#define GTK_PROMPT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_PROMPT, GtkPromptClass))
#define GTK_IS_PROMPT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_PROMPT))
#define GTK_IS_PROMPT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_PROMPT))
#define GTK_PROMPT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_PROMPT, GtkPromptClass))
#define GTK_PROMPT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_PROMPT, GtkPromptPrivate))

GType gtk_prompt_get_type (void);

GtkWidget * gtk_prompt_new(const gchar *text);
void gtk_prompt_set_text (GtkPrompt * prompt, const gchar *text);

#endif  /* __PROMPT_H__ */
