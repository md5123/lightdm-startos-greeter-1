#include <gtk/gtk.h>
#include <glib/gi18n.h>
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

#define LOGIN_BOX_WIDTH     643
#define LOGIN_BOX_HEIGHT    207
#define USER_FACE_SPACING   14
#define USER_FACE_WIDTH     80
#define USER_FACE_HEIGHT    104
#define SYS_BUTTON_WIDTH    32
#define SYS_BUTTON_HEIGHT   32


GdkRectangle    monitor_geometry;
_ui_widgets     ui_widgets;

GdkRGBA bg_color;
GdkPixbuf * bg_pixbuf = NULL;
GdkPixbuf * bg_scale_pixbuf = NULL;

static GtkWidget * ui_init_win (void);
static void ui_monitors_changed_cb (GdkScreen *screen, gpointer data);
static gboolean ui_draw_cb (GtkWindow *window, cairo_t *ctx, gpointer data);

static void init_ui_widget(void);

static void install_login_box (GtkFixed *fixed);
static void install_users_table (GtkFixed *fixed);
static void install_buttons_bg (GtkFixed *fixed);
static void install_power_button (GtkFixed *fixed);
static void install_lang_button (GtkFixed *fixed);
static void install_keyboard_button (GtkFixed *fixed);
static void install_session_button (GtkFixed *fixed);
static void install_clock_label (GtkFixed *fixed);

static GtkWidget * make_power_menu (void);
static GtkWidget * make_lang_menu (void);
static GtkWidget * make_session_menu (void);
static GtkWidget * make_keyboard_menu (void);
static gboolean popup_sys_menu (GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean sys_button_change_image (GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean buttons_bg_draw_cb (GtkWidget * label, cairo_t * ctx, gpointer data);
static GtkWidget * make_user_item (LightDMUser *user);

static void power_control_panel (GtkSysMenuItem *selected_item, const gchar *op);
static void keyboard_changed_cb (GtkSysMenuItem * item, gpointer data);
static void user_added_cb (LightDMUserList * list, LightDMUser *user);
static void user_changed_cb (LightDMUserList * list, LightDMUser *user);
static void user_removed_cb (LightDMUserList * list, LightDMUser *user);

static void login_box_input_ready_cb (GtkLoginBox *box, const gchar *text, gpointer data);
static void login_box_update_face_cb (GtkLoginBox *box, const gchar *username, gpointer data);
static void login_box_reborn_cb (GtkLoginBox *box, gpointer data);
static void userface_release_cb (GtkWidget * widget, gpointer data);
static GtkFixed * make_users_table (GList *userlist);
static void make_sys_button (_sys_button *bt);

static void update_userface_size_allocation (void);

static void set_last_user (void);


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
    install_buttons_bg (GTK_FIXED(fixed));
 	install_power_button (GTK_FIXED(fixed));
 	install_lang_button (GTK_FIXED(fixed));
 	install_keyboard_button (GTK_FIXED(fixed));
 	install_session_button (GTK_FIXED(fixed));
    install_clock_label (GTK_FIXED(fixed));
    set_last_user ();

	return win;
}

static GtkWidget * ui_init_win ()
{
	GtkWidget *win;

    backend_get_conf_background (&bg_pixbuf, &bg_color);

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

    if (bg_pixbuf)
    {
        bg_scale_pixbuf = gdk_pixbuf_scale_simple (bg_pixbuf, monitor_geometry.width, monitor_geometry.height, GDK_INTERP_BILINEAR);
    }
}

static gboolean ui_draw_cb (GtkWindow *window, cairo_t *ctx, gpointer data)
{

    if (bg_scale_pixbuf)
    {
        gdk_cairo_set_source_pixbuf (ctx, bg_scale_pixbuf, 0.0, 0.0);
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
	g_signal_connect (G_OBJECT(box), "update-face", G_CALLBACK(login_box_update_face_cb), NULL);
	g_signal_connect (G_OBJECT(box), "reborn", G_CALLBACK(login_box_reborn_cb), NULL);
	gtk_widget_set_size_request (box, ui_widgets.loginbox.w, ui_widgets.loginbox.h);
	ui_widgets.loginbox.loginbox = GTK_LOGIN_BOX(box);
	gtk_fixed_put (fixed, box, ui_widgets.loginbox.x, ui_widgets.loginbox.y);
}

static void login_box_input_ready_cb (GtkLoginBox *box, const gchar *text, gpointer data)
{
	backend_authenticate_process (text);
	gtk_login_box_set_input (box, "");
    gtk_widget_set_sensitive (GTK_WIDGET(box), FALSE);
}

static void login_box_update_face_cb (GtkLoginBox *box, const gchar *username, gpointer data)
{
    GList * item = ui_widgets.userstable.userlist;
    const gchar * name;

    g_return_if_fail (username);

    for (; item; item = item->next)
    {
        name = gtk_userface_get_name (GTK_USERFACE(item->data));
        if (!g_strcmp0 (username, name))
            break ;
    }
    if (!item)
        return ;
    gtk_login_box_update_face_name (box, 
            (GdkPixbuf *)gtk_userface_get_facepixbuf (item->data),
            NULL);
}

static void login_box_reborn_cb (GtkLoginBox *box, gpointer data)
{
	gtk_login_box_set_input (box, "");
    gtk_login_box_update_face_name (box, NULL, "YLMF OS");
    gtk_login_box_set_input_visible (box, TRUE);
    backend_authenticate_username_only (NULL);
}

void ui_set_prompt_text(const char *prompt, int type)
{
	gtk_login_box_set_prompt (ui_widgets.loginbox.loginbox, prompt);
    gtk_login_box_set_input_visible (ui_widgets.loginbox.loginbox, type == 0);
    gtk_widget_set_sensitive (GTK_WIDGET(ui_widgets.loginbox.loginbox), TRUE);
    gtk_login_box_set_input_focus (ui_widgets.loginbox.loginbox);
}

void ui_set_prompt_show (gboolean setting)
{
    gtk_login_box_set_prompt_show (ui_widgets.loginbox.loginbox, setting);
}

void ui_set_login_box_sensitive (gboolean setting)
{
    gtk_widget_set_sensitive (GTK_WIDGET(ui_widgets.loginbox.loginbox), setting);
    gtk_login_box_set_input_focus (ui_widgets.loginbox.loginbox);
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
    ui_widgets.userstable.table = make_users_table (ui_widgets.userstable.userlist);
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
     * I am not sure weather the list and its item address will change or not
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
    GtkWidget   * label;
    guint i = 0;

    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL(label), g_strdup_printf ("<span color=\"white\" font=\"Sans 15\">%s</span>", _("Other Users")));
    fixed = (GtkFixed *)gtk_fixed_new ();
    gtk_fixed_put (fixed, label, 5, 2); 

    for (i = 0; userlist; userlist = userlist->next, i++)
    {
        user = userlist->data;
        gtk_fixed_put (fixed, GTK_WIDGET(user), 
                (i % 2) * (USER_FACE_WIDTH + USER_FACE_SPACING), 
                (i / 2 + 1) * USER_FACE_SPACING + (i / 2) * USER_FACE_HEIGHT + 30); 
    }
    return fixed;
}

static gboolean buttons_bg_draw_cb (GtkWidget * label, cairo_t * ctx, gpointer data)
{
    cairo_set_source_rgba (ctx, 0.0, 0.0, 0.0, 0.8);
    cairo_rectangle (ctx, 0, 0, ui_widgets.buttons_bg.w, ui_widgets.buttons_bg.h);
    cairo_fill (ctx);
    return FALSE;
}

static void install_buttons_bg (GtkFixed *fixed)
{
    GtkWidget * label;
    label = gtk_label_new (NULL);
    g_signal_connect (label, "draw", G_CALLBACK(buttons_bg_draw_cb), NULL);
    gtk_widget_set_size_request (label, ui_widgets.buttons_bg.w, ui_widgets.buttons_bg.h);
    gtk_widget_set_app_paintable (label, TRUE);
    gtk_fixed_put (fixed, label, ui_widgets.buttons_bg.x, ui_widgets.buttons_bg.y);
    gtk_widget_show (label);
}

static void make_sys_button (_sys_button *bt)
{
	GtkWidget * widget;
	GtkWidget * img;
    
    widget = gtk_event_box_new ();
    img = gtk_image_new ();
	g_signal_connect (widget, "enter-notify-event", G_CALLBACK(popup_sys_menu), bt);
	g_signal_connect (widget, "leave-notify-event", G_CALLBACK(sys_button_change_image), bt);
	gtk_event_box_set_visible_window (GTK_EVENT_BOX(widget), FALSE);

    gtk_image_set_from_file (GTK_IMAGE(img), bt->img_nor);
    gtk_widget_set_size_request (img, SYS_BUTTON_WIDTH, SYS_BUTTON_HEIGHT);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER(widget), img);
    bt->img_empty = (GtkImage *)img;
    bt->button = widget;
    gtk_widget_show (widget);
}

static gboolean sys_button_change_image (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	_sys_button *bt = (_sys_button *)data;
    gtk_image_set_from_file (bt->img_empty, bt->img_nor);
    return FALSE;
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
    gtk_image_set_from_file (bt->img_empty, bt->img_hl);

	return FALSE;
}


static void install_power_button (GtkFixed *fixed)
{
    ui_widgets.power.menu = make_power_menu ();
    ui_widgets.power.img_nor = GREETER_DATA_DIR "power.png";
    ui_widgets.power.img_hl  = GREETER_DATA_DIR "power-hl.png";
    make_sys_button (&ui_widgets.power);
    gtk_fixed_put (fixed, GTK_WIDGET(ui_widgets.power.button), 
            ui_widgets.power.x, ui_widgets.power.y);
}

static void install_lang_button (GtkFixed *fixed)
{
    ui_widgets.language.menu = make_lang_menu ();
    ui_widgets.language.img_nor = GREETER_DATA_DIR "language.png"; 
    ui_widgets.language.img_hl  = GREETER_DATA_DIR "language-hl.png"; 
    make_sys_button (&ui_widgets.language);
    gtk_fixed_put (fixed, ui_widgets.language.button, 
            ui_widgets.language.x, ui_widgets.language.y);
}

static void install_keyboard_button (GtkFixed *fixed)
{
    ui_widgets.keyboard.menu = make_keyboard_menu ();
    ui_widgets.keyboard.img_nor = GREETER_DATA_DIR "keyboard.png"; 
    ui_widgets.keyboard.img_hl  = GREETER_DATA_DIR "keyboard-hl.png"; 
    make_sys_button (&ui_widgets.keyboard);
    gtk_fixed_put (fixed, ui_widgets.keyboard.button, 
            ui_widgets.keyboard.x, ui_widgets.keyboard.y);
}

static void install_session_button (GtkFixed *fixed)
{
    ui_widgets.session.menu = make_session_menu ();
    ui_widgets.session.img_nor = GREETER_DATA_DIR "session.png"; 
    ui_widgets.session.img_hl  = GREETER_DATA_DIR "session-hl.png"; 
    make_sys_button (&ui_widgets.session);
    gtk_fixed_put (fixed, ui_widgets.session.button, 
            ui_widgets.session.x, ui_widgets.session.y);
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
    
    ui_widgets.power.menu_x = 20;
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
    gchar * last_lang_key = backend_state_file_get_language ();
    

    langlist = lightdm_get_languages ();
    for (; langlist; langlist = langlist->next)
    {
        LightDMLanguage *lang = (LightDMLanguage *)(langlist->data);
        const gchar * lang_key = lightdm_language_get_code (lang);

        item = gtk_sys_menu_item_new (g_strdup_printf("%s(%s)", lightdm_language_get_name (lang),
                                        lightdm_language_get_territory (lang)),
                                        (gpointer)lang_key,
                                        NULL, NULL, 
                                        g_strcmp0(lang_key, last_lang_key) == 0);

        itemlist = g_list_append (itemlist, item);
    }
    
    g_free (last_lang_key);
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
    gchar * last_session_key = backend_state_file_get_session ();

    sessionlist = lightdm_get_sessions ();
    for (; sessionlist; sessionlist = sessionlist->next)
    {
        LightDMSession *session = (LightDMSession *)(sessionlist->data);
        const gchar * session_key = lightdm_session_get_key (session);
        item = gtk_sys_menu_item_new (lightdm_session_get_name (session),
                                      (gpointer)session_key,
                                      NULL, NULL, 
                                      g_strcmp0(session_key, last_session_key) == 0);
        itemlist = g_list_append (itemlist, item);
    }
    
    g_free (last_session_key);
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
    gchar * last_kb_name = backend_state_file_get_keyboard ();

    keyboardlist = lightdm_get_layouts ();
    for (; keyboardlist; keyboardlist = keyboardlist->next)
    {
        LightDMLayout * keyboard = (LightDMLayout *)(keyboardlist->data);
        const gchar * kb_name = lightdm_layout_get_name (keyboard);
        item = gtk_sys_menu_item_new (kb_name, 
                                      keyboard, 
                                      keyboard_changed_cb, 
                                      NULL, 
                                      g_strcmp0(kb_name, last_kb_name) == 0);
        itemlist = g_list_append (itemlist, item);
    }
    
    g_free (last_kb_name);
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
    backend_state_file_set_keyboard (lightdm_layout_get_name ((LightDMLayout *)(item->data)));
}

static void set_last_user ()
{
    char * last_user_name = backend_state_file_get_user ();
    GList * item = ui_widgets.userstable.userlist;
    const gchar * facepath = NULL;
    const gchar * name = NULL;
    GdkPixbuf * pixbuf = NULL;

    if (!last_user_name || !item)
        goto SET_ORIGINAL_FACE_NAME;

    while (item) 
    {
        name = gtk_userface_get_name (GTK_USERFACE(item->data));
        if (!g_strcmp0 (last_user_name, name))
        {
            break ;
        }
        item = item->next;
    }
    g_free (last_user_name);
    if (!item)
        goto SET_ORIGINAL_FACE_NAME;

    backend_authenticate_username_only (name);

    /*  
     * Before GtkUserface realize (shown), image not convet to pixbuf yet, 
     * so we have to open it .
     */
    if ((facepath = gtk_userface_get_facepath ((GTK_USERFACE(item->data)))))
	{
		if (!(pixbuf = gdk_pixbuf_new_from_file (facepath, NULL)))
        {
            g_debug("Open face image \"%s\"faild !\n", facepath);
        }
	}
    goto SET_FACE_NAME;

SET_ORIGINAL_FACE_NAME:
    name = NULL;
    pixbuf = NULL;
    ui_set_prompt_text (_("Login"), 0);
    ui_set_prompt_show (TRUE);

SET_FACE_NAME:
    gtk_login_box_update_face_name (ui_widgets.loginbox.loginbox, 
            pixbuf, name);
    gtk_login_box_set_input (ui_widgets.loginbox.loginbox, "");
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
	ui_widgets.loginbox.y = monitor_geometry.height / 6;	

	ui_widgets.userstable.x = ui_widgets.loginbox.x;
	ui_widgets.userstable.y = ui_widgets.loginbox.y + LOGIN_BOX_HEIGHT + 10;	

	ui_widgets.buttons_bg.w = 228;
	ui_widgets.buttons_bg.h = 52; 
	ui_widgets.buttons_bg.x = 20;
	ui_widgets.buttons_bg.y = monitor_geometry.height - 62;

	ui_widgets.power.x = 40;
	ui_widgets.power.y = ui_widgets.buttons_bg.y + 10;

	ui_widgets.language.x = 40 + SYS_BUTTON_WIDTH + 20;
	ui_widgets.language.y = ui_widgets.power.y;

	ui_widgets.keyboard.x = 40 + SYS_BUTTON_WIDTH + 20 + SYS_BUTTON_WIDTH + 20;
	ui_widgets.keyboard.y = ui_widgets.power.y;

	ui_widgets.session.x = 40 + SYS_BUTTON_WIDTH + 20 + SYS_BUTTON_WIDTH + 20 + SYS_BUTTON_WIDTH + 20;
	ui_widgets.session.y = ui_widgets.power.y;

    ui_widgets.clock.w = 280;
    ui_widgets.clock.h = 100;
    ui_widgets.clock.x = monitor_geometry.width - 300;
    ui_widgets.clock.y = monitor_geometry.height - 110;
}

void ui_finalize ()
{
    g_list_free (ui_widgets.userstable.userlist);
}

