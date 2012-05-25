#include <gtk/gtk.h>
#include "gtkclock.h"
#include <time.h>
#include <libintl.h>

gchar * weekday[7];

struct _GtkClockPrivate
{
	gint x, y, w, h;
	guint timeout_id;
    gboolean secflash;
    gchar *time; 
    gchar *week; 
};

static void gtk_clock_init (GtkClock *clock);
static void gtk_clock_class_init (GtkClockClass *klass);
static void gtk_clock_realize (GtkWidget *widget);
static gboolean gtk_clock_draw (GtkWidget *widget, cairo_t *ctx);
static gboolean update_time (GtkClock *clock);


G_DEFINE_TYPE (GtkClock, gtk_clock, GTK_TYPE_BOX);


static void gtk_clock_class_init (GtkClockClass *klass)
{
	g_type_class_add_private (klass, sizeof (GtkClockPrivate));
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->draw = gtk_clock_draw;
	widget_class->realize = gtk_clock_realize;
}

static void gtk_clock_init (GtkClock *clock)
{
	clock->priv = GTK_CLOCK_GET_PRIVATE (clock);
    clock->priv->secflash = TRUE;
    clock->priv->time = NULL;
    clock->priv->week = NULL;
    gtk_widget_set_has_window (GTK_WIDGET(clock), FALSE);

    weekday[0] = gettext("Sunday");
    weekday[1] = gettext("Monday");
    weekday[2] = gettext("Tuesday");
    weekday[3] = gettext("Wednesday");
    weekday[4] = gettext("Thursday");
    weekday[5] = gettext("Friday");
    weekday[6] = gettext("Saturday");
}

static void gtk_clock_realize (GtkWidget *widget)
{
    GtkClock *clock = GTK_CLOCK(widget);
	GTK_WIDGET_CLASS(gtk_clock_parent_class)->realize (widget); 
    clock->priv->timeout_id   = g_timeout_add(60000, (GSourceFunc)update_time, clock);
}

static gboolean update_time (GtkClock *clock)
{
    if (!gtk_widget_get_realized (GTK_WIDGET(clock))) 
        return TRUE;
    gtk_widget_queue_draw (GTK_WIDGET(clock));
    //clock->priv->secflash = !clock->priv->secflash;
    return TRUE;
}

static gboolean gtk_clock_draw (GtkWidget *widget, cairo_t *ctx)
{

    struct tm *tms;
    time_t t;
	GtkClockPrivate *priv = GTK_CLOCK(widget)->priv;

    g_free(priv->time);
    t = time (NULL);
    tms = localtime(&t);
    priv->time = 
        g_strdup_printf ("%.2d%c%.2d", tms->tm_hour, priv->secflash ? ':' : ' ', tms->tm_min);
    priv->week = weekday[tms->tm_wday];

    cairo_set_source_rgba (ctx, 0.0, 0.0, 0.0, 0.4);
    cairo_paint (ctx);

    cairo_move_to (ctx, 4, 36);
    cairo_select_font_face (ctx, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size (ctx, 36);
    cairo_set_source_rgb (ctx, 1.0, 1.0, 1.0);
    cairo_show_text (ctx, priv->time);

    cairo_select_font_face (ctx, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (ctx, 24);
    cairo_move_to (ctx, 4, 84);
    cairo_show_text (ctx, priv->week);

	return FALSE;
}

GtkWidget * gtk_clock_new ()
{
	return (GtkWidget *)g_object_new (GTK_TYPE_CLOCK, NULL); 
}

