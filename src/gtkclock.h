#ifndef __GTK_CLOCK_H_

#define __GTK_CLOCK_H_

#include <gtk/gtk.h>

typedef struct _GtkClock GtkClock;
typedef struct _GtkClockClass GtkClockClass;
typedef struct _GtkClockPrivate GtkClockPrivate;


#define GTK_TYPE_CLOCK  (gtk_clock_get_type ())
#define GTK_CLOCK(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_CLOCK, GtkClock))
#define GTK_CLOCK_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_CLOCK, GtkClockClass))
#define GTK_IS_CLOCK(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_CLOCK, GtkClock))
#define GTK_IS_CLOCK_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_CLOCK, GtkClock))
#define GTK_CLOCK_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_CLOCK, GtkClockClass))
#define GTK_CLOCK_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_CLOCK, GtkClockPrivate))

struct _GtkClock
{
	GtkBox box;
    GtkClockPrivate *priv;
};

struct _GtkClockClass
{
	GtkBoxClass parent_class;
};


GType gtk_clock_get_type (void);
GtkWidget * gtk_clock_new (void);

#endif /* __GTK_CLOCK_H__ */
