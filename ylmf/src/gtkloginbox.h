#ifndef __GTK_MYBOX_H_

#define __GTK_MYBOX_H_

#include <gtk/gtk.h>

typedef struct _GtkLoginBox GtkLoginBox;
typedef struct _GtkLoginBoxClass GtkLoginBoxClass;
typedef struct _GtkLoginBoxPrivate GtkLoginBoxPrivate;

#define GTK_TYPE_LOGIN_BOX  (gtk_login_box_get_type ())
#define GTK_LOGIN_BOX(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_LOGIN_BOX, GtkLoginBox)) 
#define GTK_LOGIN_BOX_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_LOGIN_BOX, GtkLoginBoxClass)) 
#define GTK_IS_LOGIN_BOX(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_LOGIN_BOX)) 
#define GTK_IS_LOGIN_BOX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_LOGIN_BOX)) 
#define GTK_LOGIN_BOX_GET_CLASS(o)      (G_TYPE_INSTANCE_GET_CLASS ((o), GTK_TYPE_LOGIN_BOX, GtkLoginBoxClass))
#define GTK_LOGIN_BOX_GET_PRIVATE(o)    (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTK_TYPE_LOGIN_BOX, GtkLoginBoxPrivate))


struct _GtkLoginBox
{
	GtkContainer container;

    GtkLoginBoxPrivate *priv;
};

struct _GtkLoginBoxClass
{
	GtkContainerClass parent_class;
};


GType gtk_login_box_get_type (void);
GtkWidget * gtk_login_box_new (void);
void gtk_login_box_update_face_name (GtkLoginBox *box, GdkPixbuf *facepixbuf, const gchar *name);

const gchar * gtk_login_box_get_input (GtkLoginBox *box);
void  gtk_login_box_set_input (GtkLoginBox *box, const char *text);
void  gtk_login_box_set_prompt (GtkLoginBox *box, const char *text);
void  gtk_login_box_set_input_visible (GtkLoginBox *box, gboolean setting);
#endif /* __GTK_MYBOX_H__ */
