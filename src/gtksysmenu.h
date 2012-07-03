#ifndef __GTK_SYS_MENU_H__

#define __GTK_SYS_MENU_H__

#include <gtk/gtk.h>

typedef struct _GtkSysMenuItem          GtkSysMenuItem;

typedef struct _GtkSysMenu              GtkSysMenu;
typedef struct _GtkSysMenuClass         GtkSysMenuClass;
typedef struct _GtkSysMenuPrivate       GtkSysMenuPrivate;

#define GTK_TYPE_SYS_MENU         (gtk_sys_menu_get_type ())
#define GTK_SYS_MENU(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_SYS_MENU, GtkSysMenu))
#define GTK_SYS_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_SYS_MENU, GtkSysMenuClass))
#define GTK_IS_SYS_MENU(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_SYS_MENU))
#define GTK_IS_SYS_MENU_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_SYS_MENU))
#define GTK_SYS_MENU_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_SYS_MENU, GtkSysMenuClass))
#define GTK_SYS_MENU_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_SYS_MENU, GtkSysMenuPrivate))

struct _GtkSysMenuItem
{
    const char  *text;
    const char  *code;
    PangoLayout *layout;
    void (* func) (GtkSysMenuItem *item, ...);
    gpointer func_data;
	gint x, y;
    gboolean  selected;
    gpointer data;
};

struct _GtkSysMenu
{
	GtkWidget   menu;
	GtkSysMenuPrivate * priv;
};

struct _GtkSysMenuClass
{
	GtkWidgetClass   parent_class;
};


GType gtk_sys_menu_get_type (void);

GtkWidget * gtk_sys_menu_new (GList * itemlist);
GtkSysMenuItem * gtk_sys_menu_item_new (const char *text, gpointer data, void *func, gpointer f_user_data, gboolean selected);
const char * gtk_sys_menu_get_select_text (GtkSysMenu * menu);
gpointer gtk_sys_menu_get_select_data (GtkSysMenu * menu);

#endif /* __GTK_SYS_MENU_H__ */
