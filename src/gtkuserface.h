#ifndef __GTK_USERFACE_H__

#define __GTK_USERFACE_H__

#include <gtk/gtk.h>

typedef struct _GtkUserface GtkUserface;
typedef struct _GtkUserfaceClass GtkUserfaceClass;
typedef struct _GtkUserfacePrivate GtkUserfacePrivate;


struct _GtkUserface
{
	GtkWidget widget;
	GtkUserfacePrivate *priv;
};

struct _GtkUserfaceClass
{
	GtkWidgetClass parent_class;
};

GType gtk_userface_get_type ();

#define GTK_TYPE_USERFACE (gtk_userface_get_type ())
#define GTK_USERFACE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_USERFACE, GtkUserface))
#define GTK_USERFACE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_USERFACE, GtkUserfaceClass))
#define GTK_IS_USERFACE(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_USERFACE))
#define GTK_IS_USERFACE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_USERFACE))
#define GTK_USERFACE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_USERFACE, GtkUserfaceClass))
#define GTK_USERFACE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_TYPE_USERFACE, GtkUserfacePrivate))



GtkWidget * gtk_userface_new (const char *face_path, const char *username);

const gchar * gtk_userface_get_name (GtkUserface * userface);
const GdkPixbuf * gtk_userface_get_facepixbuf (GtkUserface * userface);
#endif /* __GTK_USERFACE_H__ */
