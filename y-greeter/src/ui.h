#ifndef __UI_H__

#define __UI_H__

#include <gtk/gtk.h>
#include "gtkloginbox.h"
#include "gtkclock.h"

typedef struct _ui_widgets   _ui_widgets;
typedef struct _login_box    _login_box;
typedef struct _users_table  _users_table;
typedef struct _sys_button   _sys_button;
typedef struct _clock 	     _clock;

struct _login_box
{
    GtkLoginBox * loginbox;
    gint x, y, w, h;
};

struct _users_table
{ 
	GtkFixed  * table;
	GList 	  * userlist;
	guint 		column;
	gint 		x, y;
};

struct _sys_button
{ 
	GtkEventBox * button;
    GtkWidget   * menu;
	gint 	      x, y;
    gint    	menu_x, menu_y;
};

struct _clock
{ 
	GtkClock 	* clock;
	gint 	      w, h, x, y;
};

struct  _ui_widgets 
{
    _login_box   loginbox;
    _users_table userstable;
    _sys_button  power;
    _sys_button  language;
    _sys_button  keyboard;
    _sys_button  session;
	_clock 		 clock;
};

GtkWidget * ui_make_root_win (void);
void ui_finalize (void);
void ui_set_prompt_text(const char *prompt, int type);
gpointer ui_get_session (void);
gpointer ui_get_language (void);
gpointer ui_get_keyboard_layout (void);









#endif /* __UI_H__ */

