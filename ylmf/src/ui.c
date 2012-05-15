#include "config.h"
#include <gtk/gtk.h>
#include <lightdm.h>
#include "ui.h"
#include "gtkloginbox.h"
#include "gtkuserface.h"
#include "backend.h"
#include "gtksysmenu.h"
#include "gtkclock.h"

#define LDM_SHUTDOWN  "shutdown"
#define LDM_RESTART   "restart"
#define LDM_SUSPEND   "suspend"
#define LDM_HIBERNATE "hibernate"

#define LOGIN_BOX_WIDTH 680
#define LOGIN_BOX_HEIGHT 120
#define USER_FACE_SPACING 6
#define USER_FACE_WIDTH 36
#define USER_FACE_HEIGHT 50
#define SYS_BUTTON_WIDTH 36
#define SYS_BUTTON_HEIGHT 36


static GdkRectangle monitor_geometry;
static _ui_widgets 	ui_widgets;



static GtkWidget * ui_init_win (void);
static void ui_monitors_changed_cb (GdkScreen *screen, gpointer data);
static gboolean ui_draw_cb (GtkWindow *window, cairo_t *ctx, gpointer data);

static void init_ui_widget(void);

static void install_login_box (GtkFixed *fixed);
static void install_users_table (GtkFixed *fixed);
static void install_power_button (GtkFixed *fixed);
static void install_lang_button (GtkFixed *fixed);
static void install_keyboard_button (GtkFixed *fixed);
static void install_session_button (GtkFixed *fixed);
static void install_clock_label (GtkFixed *fixed);

static GtkWidget * make_power_menu (void);
static GtkWidget * make_lang_menu (void);
static GtkWidget * make_session_menu (void);
static GtkWidget * make_keyboard_menu (void);

static void power_control_panel (GtkSysMenuItem *selected_item, const gchar *op);
static void keyboard_changed_cb (GtkSysMenuItem * item, gpointer data);
static gboolean popup_sys_menu (GtkWidget *widget, GdkEvent *event, gpointer data);
static GtkWidget * make_user_item (LightDMUser *user);
static void user_added_cb (LightDMUserList * list, LightDMUser *user);
static void user_changed_cb (LightDMUserList * list, LightDMUser *user);
static void user_removed_cb (LightDMUserList * list, LightDMUser *user);

static void login_box_input_ready_cb (GtkLoginBox *box, const gchar *text, gpointer data);
static void userface_release_cb (GtkWidget * widget, gpointer data);
static GtkFixed * make_users_table (GList *userlist);
static GtkWidget * make_sys_button (const gchar *imgpath, _sys_button *bt);

static void update_userface_size_allocation (void);


GtkWidget * ui_make_root_win ()
{
	GtkWidget *win;
	GtkWidget *fixed;

	win = ui_init_win ();
	fixed = gtk_fixed_new ();
	gtk_container_add (GTK_CONTAINER(win), fixed);

	init_ui_widget ();

	install_login_box (GTK_FIXED(fixed));
 	install_users_table (GTK_FIXED(fixed));
 	install_power_button (GTK_FIXED(fixed));
 	install_lang_button (GTK_FIXED(fixed));
 	install_keyboard_button (GTK_FIXED(fixed));
 	install_session_button (GTK_FIXED(fixed));
    install_clock_label (GTK_FIXED(fixed));
    /* don't forgot free memory for 4 g_list  */

	return win;
}

static GtkWidget * ui_init_win ()
{
	GtkWidget *win;
	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (G_OBJECT(gtk_window_get_screen(GTK_WINDOW(win))), 
                               "monitors-changed", G_CALLBACK(ui_monitors_changed_cb), win);
    g_signal_connect (G_OBJECT(win), "draw", G_CALLBACK(ui_draw_cb), NULL);
	ui_monitors_changed_cb (gtk_window_get_screen(GTK_WINDOW(win)), win);
	gtk_window_set_decorated (GTK_WINDOW(win), FALSE);
	gtk_window_set_has_resize_grip (GTK_WINDOW(win), FALSE);
    gtk_widget_set_app_paintable (win, TRUE);
	
	return win;
}


static void ui_monitors_changed_cb (GdkScreen *screen, gpointer data)
{
	GtkWindow * win = GTK_WINDOW (data);

	gdk_screen_get_monitor_geometry (screen, 
									gdk_screen_get_primary_monitor (screen),
									&monitor_geometry);
	gtk_window_move (win, 0, 0);
	gtk_window_resize (win, monitor_geometry.width, monitor_geometry.height);
}

static gboolean ui_draw_cb (GtkWindow *window, cairo_t *ctx, gpointer data)
{
    GdkRGBA bg_color;
    GdkPixbuf * bg_pixbuf = NULL;
    backend_get_conf_background (&bg_pixbuf, &bg_color);

    if (bg_pixbuf)
    {
        GdkPixbuf *pixbuf = gdk_pixbuf_scale_simple (bg_pixbuf, monitor_geometry.width, monitor_geometry.height, GDK_INTERP_BILINEAR);
        gdk_cairo_set_source_pixbuf (ctx, pixbuf, 0.0, 0.0);
        g_object_unref (pixbuf);
    }
    else
        gdk_cairo_set_source_rgba (ctx, &bg_color);
    cairo_paint(ctx);

    return FALSE;
}

static void install_login_box  (GtkFixed *fixed)
{
	GtkWidget *box = gtk_login_box_new ();
	g_signal_connect (G_OBJECT(box), "input-ready", G_CALLBACK(login_box_input_ready_cb), NULL);
	gtk_widget_set_size_request (box, ui_widgets.loginbox.w, ui_widgets.loginbox.h);
	ui_widgets.loginbox.loginbox = GTK_LOGIN_BOX(box);
	gtk_fixed_put (fixed, box, ui_widgets.loginbox.x, ui_widgets.loginbox.y);
}

static void login_box_input_ready_cb (GtkLoginBox *box, const gchar *text, gpointer data)
{
	backend_authenticate_process (text);
	gtk_login_box_set_input (box, "");
}


void ui_set_prompt_text(const char *prompt, int type)
{
	gtk_login_box_set_prompt (ui_widgets.loginbox.loginbox, prompt);
    gtk_login_box_set_input_visible (ui_widgets.loginbox.loginbox, type == 0);
}


static void install_users_table (GtkFixed *fixed)
{
    GList     * userlist;
    LightDMUserList * ldm_userlist;

	ldm_userlist = lightdm_user_list_get_instance ();
	g_signal_connect (G_OBJECT(ldm_userlist), "user-added", G_CALLBACK(user_added_cb), NULL);
	g_signal_connect (G_OBJECT(ldm_userlist), "user-changed", G_CALLBACK(user_changed_cb), NULL);
	g_signal_connect (G_OBJECT(ldm_userlist), "user-removed", G_CALLBACK(user_removed_cb), NULL);
	userlist = lightdm_user_list_get_users (ldm_userlist);
	for (; userlist; userlist = userlist->next)
	{
        ui_widgets.userstable.userlist = 
            g_list_append (ui_widgets.userstable.userlist, 
                    make_user_item ((LightDMUser *)(userlist->data)) );
	}
    ui_widgets.userstable.table  = make_users_table (ui_widgets.userstable.userlist);
    gtk_fixed_put (fixed, GTK_WIDGET(ui_widgets.userstable.table), ui_widgets.userstable.x, ui_widgets.userstable.y);
}

static GtkWidget * make_user_item (LightDMUser *user)
{
    GtkWidget * userface;
    const gchar * username = lightdm_user_get_name (user);
    const gchar * facepath = lightdm_user_get_image (user);

    userface = gtk_userface_new (facepath ? facepath : GREETER_DATA_DIR "defaultface.png",
            username);
    gtk_widget_set_size_request (userface, USER_FACE_WIDTH, USER_FACE_HEIGHT);
    g_signal_connect (G_OBJECT(userface), "button-release-event", 
            G_CALLBACK(userface_release_cb), NULL); 
    gtk_widget_show (userface);
    return userface;
}

static void user_added_cb (LightDMUserList * list, LightDMUser *user)
{
    GtkWidget * widget = make_user_item (user);
    gtk_widget_show (widget);
    gint index;
    ui_widgets.userstable.userlist = g_list_append (ui_widgets.userstable.userlist, widget);
    index = g_list_length (ui_widgets.userstable.userlist) - 1;
    gtk_fixed_put (ui_widgets.userstable.table, GTK_WIDGET(widget), 
            (index % 2 + 1) * USER_FACE_SPACING + (index % 2) * USER_FACE_WIDTH, 
            (index / 2 + 1) * USER_FACE_SPACING + (index / 2) * USER_FACE_HEIGHT); 
}

static void user_changed_cb (LightDMUserList * list, LightDMUser *user)
{
    g_warning ("I'm sorry, I don't know When you be emited ??\n");
    /* If Not change user name , then I am not care about it */
}

static void user_removed_cb (LightDMUserList * list, LightDMUser *user)
{
    GList * item = ui_widgets.userstable.userlist;
    const gchar * username = lightdm_user_get_name (user);
    
    while (item)
    {
        if (!g_strcmp0 (username, gtk_userface_get_name (GTK_USERFACE(item->data))))
        {
            goto USER_ITEM_FOUND;
        }
        item = item->next;
    }
    g_warning ("Failed to remomved-user: %s\n", username);
    return ;

USER_ITEM_FOUND:
    gtk_container_remove (GTK_CONTAINER(ui_widgets.userstable.table), GTK_WIDGET(item->data));
    gtk_widget_destroy (GTK_WIDGET(item->data));
    ui_widgets.userstable.userlist = g_list_remove (ui_widgets.userstable.userlist, item);
    /* FIXME:
     * I am not sure if the list and its item address will change or net,
     * when remove one of its item. 
     * SO, I update all the remaining USERFACE
     */
    update_userface_size_allocation ();
}

static void update_userface_size_allocation ()
{
    GList * children = gtk_container_get_children (GTK_CONTAINER(ui_widgets.userstable.table));
    gint i = 0;
    gint x, y;

    for (; children; children = children->next, i++)
    {
        x = (i % 2 + 1) * USER_FACE_SPACING + (i % 2) * USER_FACE_WIDTH, 
        y = (i / 2 + 1) * USER_FACE_SPACING + (i / 2) * USER_FACE_HEIGHT; 
        gtk_fixed_move (GTK_FIXED(ui_widgets.userstable.table),
                GTK_WIDGET(children->data), x, y);
    }
}

static void userface_release_cb (GtkWidget * widget, gpointer data)
{
    GtkUserface * user = GTK_USERFACE(widget);
    const gchar * name = gtk_userface_get_name (user);
    backend_authenticate_username_only (name);
    gtk_login_box_update_face_name (ui_widgets.loginbox.loginbox, 
                                    (GdkPixbuf *)gtk_userface_get_facepixbuf (user),
                                    name);
    gtk_login_box_set_input (ui_widgets.loginbox.loginbox, "");
}

static GtkFixed * make_users_table (GList *userlist)
{
    GtkUserface * user;
    GtkFixed    * fixed;
    guint i = 0;

    fixed = (GtkFixed *)gtk_fixed_new ();
	gtk_container_set_reallocate_redraws (GTK_CONTAINER(fixed), TRUE);
    for (i = 0; userlist; userlist = userlist->next, i++)
    {
        user = userlist->data;
        gtk_fixed_put (fixed, GTK_WIDGET(user), 
                (i % 2 + 1) * USER_FACE_SPACING + (i % 2) * USER_FACE_WIDTH, 
                (i / 2 + 1) * USER_FACE_SPACING + (i / 2) * USER_FACE_HEIGHT); 
    }
    return fixed;
}

static GtkWidget * make_sys_button (const gchar *imgpath, _sys_button *bt)
{
	GtkWidget * widget;
    GtkWidget * img;
    
    widget = gtk_event_box_new ();
	g_signal_connect (widget, "enter-notify-event", G_CALLBACK(popup_sys_menu), bt);
	gtk_event_box_set_visible_window (GTK_EVENT_BOX(widget), FALSE);
    img = gtk_image_new_from_file (imgpath);
    gtk_widget_set_size_request (img, SYS_BUTTON_WIDTH, SYS_BUTTON_HEIGHT);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER(widget), img);
    gtk_widget_show (widget);

	return widget;
}

static gboolean popup_sys_menu (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	GtkWidget *fixed;
	_sys_button *bt = (_sys_button *)data;

	fixed = gtk_widget_get_parent (GTK_WIDGET(bt->button));
	if (!fixed)
	{
		g_warning ("Fail to get parent\n");
		return FALSE;
	}
	gtk_widget_show_all (GTK_WIDGET(bt->menu));
	if (!gtk_widget_get_parent (GTK_WIDGET(bt->menu))) 
		gtk_fixed_put (GTK_FIXED(fixed), GTK_WIDGET(bt->menu), bt->menu_x, bt->menu_y);

	gtk_widget_grab_focus (GTK_WIDGET(bt->menu));

	return FALSE;
}


static void install_power_button (GtkFixed *fixed)
{
    GtkWidget *pw; 

    ui_widgets.power.menu = make_power_menu ();
    gtk_widget_set_size_request (ui_widgets.power.menu, 80, 0); /* when y set to 0, sys menu will adjusts it-self */
    pw = make_sys_button (GREETER_DATA_DIR"power.png", &ui_widgets.power);
	ui_widgets.power.button = GTK_EVENT_BOX(pw);
    gtk_fixed_put (fixed, pw, ui_widgets.power.x, ui_widgets.power.y);
}

static void install_lang_button (GtkFixed *fixed)
{
    GtkWidget *lang; 

    ui_widgets.language.menu = make_lang_menu ();
    gtk_widget_set_size_request (ui_widgets.language.menu, 80, 0);
    lang = make_sys_button (GREETER_DATA_DIR"language.png", &ui_widgets.language);
	ui_widgets.language.button = GTK_EVENT_BOX(lang);
    gtk_fixed_put (fixed, lang, ui_widgets.language.x, ui_widgets.language.y);
}

static void install_keyboard_button (GtkFixed *fixed)
{
    GtkWidget *kb; 

    ui_widgets.keyboard.menu = make_keyboard_menu ();
    kb = make_sys_button (GREETER_DATA_DIR"keyboard.png", &ui_widgets.keyboard);
	ui_widgets.keyboard.button = GTK_EVENT_BOX(kb);
    gtk_fixed_put (fixed, kb, ui_widgets.keyboard.x, ui_widgets.keyboard.y);
}

static void install_session_button (GtkFixed *fixed)
{
    GtkWidget *session; 

    ui_widgets.session.menu = make_session_menu ();
    gtk_widget_set_size_request (ui_widgets.session.menu, 80, 0);
    session = make_sys_button (GREETER_DATA_DIR"session.png", &ui_widgets.session);
	ui_widgets.session.button = GTK_EVENT_BOX(session);
    gtk_fixed_put (fixed, session, ui_widgets.session.x, ui_widgets.session.y);
}

static void install_clock_label (GtkFixed *fixed)
{
    GtkWidget * widget;
    widget = gtk_clock_new ();
    ui_widgets.clock.clock = GTK_CLOCK(widget);
    gtk_widget_set_size_request (widget, ui_widgets.clock.w, ui_widgets.clock.h);
    gtk_widget_show (widget);
    gtk_fixed_put (fixed, widget, ui_widgets.clock.x, ui_widgets.clock.y);
}


static GtkWidget * make_power_menu ()
{
	GList * list = NULL;
	GtkSysMenuItem * item;
	
	if (lightdm_get_can_shutdown ())
	{
		item = gtk_sys_menu_item_new(_("shutdown"), NULL, power_control_panel, LDM_SHUTDOWN, FALSE);
		list = g_list_append (list, item);
	}
	if (lightdm_get_can_restart ())
	{
		item = gtk_sys_menu_item_new(_("restart"), NULL, power_control_panel, LDM_RESTART, FALSE);
		list = g_list_append (list, item);
	}
	if (lightdm_get_can_suspend())
	{
		item = gtk_sys_menu_item_new(_("suspend"), NULL, power_control_panel, LDM_SUSPEND, FALSE);
		list = g_list_append (list, item);
	}
	if (lightdm_get_can_hibernate ())
	{
		item = gtk_sys_menu_item_new(_("hibernate"), NULL, power_control_panel, LDM_HIBERNATE, FALSE);
		list = g_list_append (list, item);
	}
    
    /* Position menu (ui_widgets.power.menu_x, ui_widgets.power.menu_y) */

    /* 
    gint x = 0, y = 0;
    x = (ui_widgets.power.x + 80 > monitor_geometry.width) ?  monitor_geometry.width - 85 : 
                                ui_widgets.power.x;
    y = 10;
    */
    ui_widgets.power.menu_x = 10;
    ui_widgets.power.menu_y = 10;

	return gtk_sys_menu_new(list);
}

static void power_control_panel (GtkSysMenuItem *selected_item, const gchar *op)
{
    GError *error = NULL;

    if (!g_strcmp0(op, LDM_SHUTDOWN))
    {
        if (!lightdm_shutdown (&error))
        {
            g_warning ("Fail to shutdown: %s", error->message);
            g_clear_error (&error);
        }
    }
    else if (!g_strcmp0(op, LDM_RESTART))
    {
        if (!lightdm_restart (&error))
        {
            g_warning ("Fail to restart: %s", error->message);
            g_clear_error (&error);
        }
    }
    if (!g_strcmp0(op, LDM_SUSPEND))
    {
        if (!lightdm_suspend (&error))
        {
            g_warning ("Fail to suspend: %s", error->message);
            g_clear_error (&error);
        }
    }
    else if (!g_strcmp0(op, LDM_HIBERNATE))
    {
        if (!lightdm_hibernate (&error))
        {
            g_warning ("Fail to hibernate: %s", error->message);
            g_clear_error (&error);
        }
    }
    /*
    else 
    {
        g_warning ("Unknow Operation");
    }
    */
}

static GtkWidget * make_lang_menu ()
{
	GList * itemlist = NULL;
	GList * langlist = NULL;
	GtkSysMenuItem * item;

    langlist = lightdm_get_languages ();
    for (; langlist; langlist = langlist->next)
    {
        LightDMLanguage *lang = (LightDMLanguage *)(langlist->data);

        item = gtk_sys_menu_item_new (lightdm_language_get_name (lang),
                (gpointer)lightdm_language_get_code (lang),
                NULL, NULL, FALSE);
        itemlist = g_list_append (itemlist, item);
    }
    
    gint x = 0, y = 0;
    x = (ui_widgets.language.x + 80 > monitor_geometry.width) ?  monitor_geometry.width - 85 : 
                                ui_widgets.language.x;
    y = 180;
    ui_widgets.language.menu_x = x;
    ui_widgets.language.menu_y = y;

	return gtk_sys_menu_new(itemlist);
}

static GtkWidget * make_session_menu ()
{
	GList * itemlist = NULL;
	GList * sessionlist = NULL;
	GtkSysMenuItem * item;

    sessionlist = lightdm_get_sessions ();
    for (; sessionlist; sessionlist = sessionlist->next)
    {
        LightDMSession *session = (LightDMSession *)(sessionlist->data);
        item = gtk_sys_menu_item_new (lightdm_session_get_name (session),
                                      (gpointer)lightdm_session_get_key (session),
                                      NULL, NULL, FALSE);
        itemlist = g_list_append (itemlist, item);
    }
    
    gint x = 0, y = 0;
    x = (ui_widgets.session.x + 80 > monitor_geometry.width) ?  monitor_geometry.width - 85 : 
                                ui_widgets.session.x;
    y = 180;
    ui_widgets.session.menu_x = x;
    ui_widgets.session.menu_y = y;

	return gtk_sys_menu_new(itemlist);
}

static GtkWidget * make_keyboard_menu ()
{
	GList * itemlist = NULL;
	GList * keyboardlist = NULL;
	GtkSysMenuItem * item;

    keyboardlist = lightdm_get_layouts ();
    for (; keyboardlist; keyboardlist = keyboardlist->next)
    {
        LightDMLayout *keyboard = (LightDMLayout *)(keyboardlist->data);
        item = gtk_sys_menu_item_new (lightdm_layout_get_name (keyboard), 
                                      keyboard, 
                                      keyboard_changed_cb, 
                                      NULL, FALSE);
        itemlist = g_list_append (itemlist, item);
    }
    
    gint x = 0, y = 0;
    x = (ui_widgets.keyboard.x + 80 > monitor_geometry.width) ?  monitor_geometry.width - 85 : 
                                ui_widgets.keyboard.x;
    y = 180;
    ui_widgets.keyboard.menu_x = x;
    ui_widgets.keyboard.menu_y = y;

	return gtk_sys_menu_new(itemlist);
}

static void keyboard_changed_cb (GtkSysMenuItem * item, gpointer data)
{
    lightdm_set_layout ((LightDMLayout *)(item->data));
}

gpointer ui_get_language ()
{
    return gtk_sys_menu_get_select_data (GTK_SYS_MENU(ui_widgets.language.menu));
}

gpointer ui_get_session ()
{
    return gtk_sys_menu_get_select_data (GTK_SYS_MENU(ui_widgets.session.menu));
}

gpointer ui_get_keyboard_layout (void)
{
    return gtk_sys_menu_get_select_data (GTK_SYS_MENU(ui_widgets.keyboard.menu));
}

static void init_ui_widget()
{
	ui_widgets.loginbox.w = LOGIN_BOX_WIDTH;
	ui_widgets.loginbox.h = LOGIN_BOX_HEIGHT; 
	ui_widgets.loginbox.x = monitor_geometry.width / 2;
	ui_widgets.loginbox.y = monitor_geometry.height / 4;	
	ui_widgets.userstable.x = ui_widgets.loginbox.x;
	ui_widgets.userstable.y = ui_widgets.loginbox.y + 100;	
	ui_widgets.power.x = 10;
	ui_widgets.power.y = monitor_geometry.height - 60;	
	ui_widgets.language.x = 60;
	ui_widgets.language.y = monitor_geometry.height - 60;
	ui_widgets.keyboard.x = 110;
	ui_widgets.keyboard.y = monitor_geometry.height - 60;	
	ui_widgets.session.x = 160;
	ui_widgets.session.y = monitor_geometry.height - 60;
    ui_widgets.clock.w = 200;
    ui_widgets.clock.h = 100;
    ui_widgets.clock.x = monitor_geometry.width - 250;
    ui_widgets.clock.y = monitor_geometry.height - 150;
}
