#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "gtkclock.h"
#include <time.h>
#include <libintl.h>

gchar * weekday[7];
gchar * month[12];
gchar * day[32];



struct _GtkClockPrivate
{
	gint x, y, w, h;
	guint timeout_id;
    gboolean secflash;
    gchar *time; 
    gchar *date; 
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
    clock->priv->date = NULL;
    gtk_widget_set_has_window (GTK_WIDGET(clock), FALSE);

    weekday[0] = _("Sunday");
    weekday[1] = _("Monday");
    weekday[2] = _("Tuesday");
    weekday[3] = _("Wednesday");
    weekday[4] = _("Thursday");
    weekday[5] = _("Friday");
    weekday[6] = _("Saturday");

    month[0] = _("Jan");
    month[1] = _("Feb");
    month[2] = _("Mar");
    month[3] = _("Apr");
    month[4] = _("May");
    month[5] = _("Jun");
    month[6] = _("Jul");
    month[7] = _("Aug");
    month[8] = _("Sep");
    month[9] = _("Oct");
    month[10] = _("Nov");
    month[11] = _("Dec");

    day[1] = _("1");
    day[2] = _("2");
    day[3] = _("3");
    day[4] = _("4");
    day[5] = _("5");
    day[6] = _("6");
    day[7] = _("7");
    day[8] = _("8");
    day[9] = _("9");
    day[10] = _("10");
    day[11] = _("11");
    day[12] = _("12");
    day[13] = _("13");
    day[14] = _("14");
    day[15] = _("15");
    day[16] = _("16");
    day[17] = _("17");
    day[18] = _("18");
    day[19] = _("19");
    day[20] = _("20");
    day[21] = _("21");
    day[22] = _("22");
    day[23] = _("23");
    day[24] = _("24");
    day[25] = _("25");
    day[26] = _("26");
    day[27] = _("27");
    day[28] = _("28");
    day[29] = _("29");
    day[30] = _("30");
    day[31] = _("31");
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
    priv->date = 
        g_strdup_printf ("%s %s , %s", month[tms->tm_mon], day[tms->tm_mday], weekday[tms->tm_wday]);

    cairo_set_source_rgba (ctx, 0.0, 0.0, 0.0, 0.6);
    cairo_paint (ctx);

    cairo_move_to (ctx, 20, 50);
    cairo_select_font_face (ctx, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size (ctx, 45);
    cairo_set_source_rgb (ctx, 1.0, 1.0, 1.0);
    cairo_show_text (ctx, priv->time);

    cairo_select_font_face (ctx, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (ctx, 24);
    cairo_move_to (ctx, 20, 84);
    cairo_show_text (ctx, priv->date);

	return FALSE;
}

GtkWidget * gtk_clock_new ()
{
	return (GtkWidget *)g_object_new (GTK_TYPE_CLOCK, NULL); 
}

