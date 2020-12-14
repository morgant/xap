/*
 * xap_gui.c
 *
 * Copyright (C) 1999 - 2000 Rasca, Berlin
 * EMail: thron@gmx.de
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "config.h"
#include "i18n.h"
#include "xap_gui.h"
#include "gtk_exec.h"
#include "gtk_dnd.h"
#include "uri.h"
#include "lnk.h"
#include "io.h"
#include "gtk_dlg.h"
#include "gtk_util.h"
#include "lnk_opt.h"

static GtkTargetEntry target_table[] = {
	{ "text/uri-list",	0,	TARGET_URI_LIST },
	{ "STRING",			0,	TARGET_STRING },
	{ "text/plain",		0,	TARGET_PLAIN },
};
#define NUM_TARGETS (sizeof(target_table)/sizeof(GtkTargetEntry))

static GtkTooltips *Tips;

typedef struct {
	char *path;
	GtkWidget *w_label;
	GtkWidget *w_box;
	GtkWidget *w_box_menu;
	GtkWidget *w_btn_menu;
} page_t;

#define ACCEL 1
typedef struct {
	gchar *label;
	void *func;
	void *data;
	int flags;
	int key;
	int mod;
} menu_entry;

typedef struct {
	GtkWidget *notebook;
	GtkWidget *btn_menu;
	char *path;
	int last;
	int box_o;		/* box orientation */
	GdkColormap *cmap;
} cfg_t;

/*
 * gtk initializing
 */
void
gui_init (int *argc, char ***argv, char *user_rc)
{
	gtk_set_locale();
	gtk_init (argc, argv);

	gtk_rc_add_default_file (XAP_RC);
	gtk_rc_parse (DATA_DIR"/xap/"XAP_RC);
	gtk_rc_parse (user_rc);
}

/*
 * callback if button was pressed (signal: "clicked")
 */
static void
button_clicked (GtkWidget *button, lnk_t *lnk)
{
	char *args[3];
	int len, use_shell = 0;
	GdkEvent *event;

	if (!lnk || !lnk->exec)
		return;
	event = gtk_get_current_event ();

	if ((*lnk->exec != '/') || (strchr (lnk->exec, ' ') != NULL))
		use_shell = 1;
	if ((event && ((GdkEventButton *)event)->state & GDK_MOD1_MASK)
		|| (lnk->terminal)) {
		args[0] = TERMINAL;
		args[1] = "-e";
		args[2] = lnk->exec;
		len = 3;
	} else {
		if (use_shell) {
			args[0] = SHELL;
			args[1] = "-c";
			args[2] = lnk->exec;
			len = 3;
		} else {
			args[0] = lnk->exec;
			len = 1;
		}
	}
	gdk_event_free (event);
	gu_cursor_wait (button->parent);
	if (io_system_var (args, len) == -1)
		dlg_error (_("Can't execute"), lnk->exec);
	gu_cursor_reset (button->parent);
}

/*
 * called on drag_motion event - needed for tooltips
 */
gboolean
button_drag_motion (GtkWidget *btn, GdkDragContext *context, gint x, gint y,
					guint time, void *data)
{
	GdkEventCrossing ev;

	ev.type = GDK_ENTER_NOTIFY;
	ev.window = btn->window;
	ev.send_event = TRUE;
	ev.subwindow = 0;
	ev.time = gdk_time_get ();
	ev.mode = GDK_CROSSING_GRAB;

	gdk_event_put ((GdkEvent *)&ev);
	return (TRUE);
}

/*
 * called on drag_leave event - needed for tooltips
 */
void
button_drag_leave (GtkWidget *btn, GdkDragContext *cnt, guint time, void *data)
{
	GdkEventCrossing ev;

	ev.type = GDK_LEAVE_NOTIFY;
	ev.window = btn->window;
	ev.send_event = TRUE;
	ev.subwindow = 0;
	ev.time = gdk_time_get ();
	ev.mode = GDK_CROSSING_GRAB;
	gdk_event_put ((GdkEvent *)&ev);
}


/*
 * called if a drop is on the program button
 */
static void
button_drop_data (GtkWidget *button, GdkDragContext *context, gint x, gint y,
		GtkSelectionData *data, guint info, guint time, lnk_t *lnk)
{
	char *arg, **argv, *cmd = NULL, *p;
	int num, i, term = 0, shell = 0, len, arg_pos = 0;
	GList *arg_list = NULL, *list;

	if ((data->length <= 0) || (data->format != 8)) {
		gtk_drag_finish (context, FALSE, TRUE, time);
		return;
	}

#ifdef	DEBUG
	fprintf (stderr, "%s: button_drop_data() info=%d, len=%d\n", __FILE__,
			info, data->length);
#endif
	arg = g_malloc (data->length + 1);
	if (!arg) {
		gtk_drag_finish (context, FALSE, TRUE, time);
		return;
	}
		/* may be the sender didn't add a terminating null byte
		 */
	arg[data->length] = '\0';
	memcpy (arg, data->data, data->length);

	num = uri_parse_list (arg, &arg_list);
	g_free (arg);
	if (!num) {
		gtk_drag_finish (context, FALSE, TRUE, time);
		return;
	}
	gtk_drag_finish (context, TRUE, TRUE, time);

	/* assume the most apps don't want a "file:/"-prefix
	 */
	uri_remove_file_prefix_from_list (arg_list);

	if ((context->action == GDK_ACTION_ASK) || (lnk->terminal)) {
		/* startup in terminal window */
		term = 2;
	}
	if ((*lnk->exec != G_DIR_SEPARATOR) ||
			(strchr (lnk->exec, ' ') != NULL)) {
		/* we assume a space means there are arguemnts for the command
		 */
		shell = 2;
	}

	argv = (char **) g_malloc (sizeof(char *) * 
				(term + (shell ? shell + 1 : 1 + num) + 1));
	if (!argv) {
		uri_free_list (arg_list);
		return;
	}
	if (term) {
		argv[arg_pos++] = TERMINAL;
		argv[arg_pos++] = "-e";
	}

	if (shell) {
		/* we have to cook the command string :(
		 */
		len = strlen(lnk->exec) + 3 * num + 1;
		list = arg_list;
		while (list) {
			len += ((uri_t *)list->data)->len;
			list = list->next;
		}
		cmd = (char *) malloc (len);
		if (cmd) {
			sprintf (cmd, "%s", lnk->exec);
			p = cmd + strlen(lnk->exec);
			list = arg_list;
			while (list) {
				if (strchr (((uri_t *)list->data)->url, '\''))
					sprintf (p, " \"%s\"", ((uri_t *)list->data)->url);
				else
					sprintf (p, " '%s'", ((uri_t *)list->data)->url);
				p += 1 + 2 + ((uri_t *)list->data)->len;
				list = list->next;
			}
			argv[arg_pos++] = SHELL;
			argv[arg_pos++] = "-c";
			argv[arg_pos++] = cmd;
			argv[arg_pos] = NULL;
		}
	} else {
		argv[arg_pos++] = lnk->exec;
		list = arg_list;
		for (i = 0; i < num; i++) {
			argv[arg_pos++] = (((uri_t *)(list->data))->url);
			list = list->next;
		}
		argv[arg_pos] = NULL;
	}

	if (io_system_var (argv, arg_pos) == -1)
		dlg_error (_("Can't execute"), lnk->exec);
	g_free (argv);
	uri_free_list (arg_list);
	if (cmd)
		free (cmd);
}

/*
 * prepare data for the drop target
 */
static void
button_drag_data_get (GtkWidget *btn, GdkDragContext *context,
		GtkSelectionData *data, guint info, guint time, gpointer mydata)
{
	int len;
	static char orig[PATH_MAX+3];
	lnk_t *lnk;

	lnk = gtk_object_get_user_data(GTK_OBJECT(btn));
	if (!lnk)
		return;
#ifdef DEBUG
	printf ("button_drag_data_get(): ->%s\n", lnk->exec);
#endif
	switch (info) {
		case TARGET_URI_LIST:
		case TARGET_STRING:
		case TARGET_PLAIN:
			/* text, uri-list, .. */
			strcpy (orig, lnk->exec);
			len = strlen (orig);
			if (info == TARGET_URI_LIST) {
				orig[len+0] = 0x0D;
				orig[len+1] = 0x0A;
				orig[len+2] = 0x00;
				len += 2;
			}
			gtk_selection_data_set (data, data->target, 8, orig, len+1);
			break;
		default:
			fprintf (stderr, "Unknown target: %d\n", info);
			break;
	}
}

/*
 */
void
menu_detach ()
{
}

/*
 * popup button menu
 */
static gint
pop_btn_menu (GtkWidget *button, GdkEventButton *event, GtkWidget *menu)
{
	if (event->button != 3)
		return (FALSE);
	gtk_object_set_user_data (GTK_OBJECT(menu), button);
	gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, event->time);
	return (TRUE);
}

/*
 */
static GtkWidget *
new_icon (char *file, GdkWindow *window, GdkColor *bg, GdkColormap *cmap)
{
	GtkWidget *w_pix = NULL;
	GdkPixmap *pixmap;
	GdkBitmap *mask = NULL;

#ifdef DEBUG
	printf ("new_icon() file=%s\n", file);
#endif
	pixmap = gdk_pixmap_colormap_create_from_xpm (NULL, cmap,
				&mask, bg, file);
	if (pixmap) {
		w_pix = gtk_pixmap_new (pixmap, mask);
		gdk_pixmap_unref (pixmap);
		gdk_pixmap_unref (mask);
	}
	return (w_pix);
}

/*
 * add program button to the page (box)
 */
static void
add_program_button (lnk_t *lnk, GtkWidget *box, GtkWidget *menu,
		GtkTooltips *tips, GdkColormap *cmap)
{
	GtkWidget *button, *icon = NULL;

	if (lnk->icon) {
		icon = new_icon (lnk->icon, box->window,
			&box->style->bg[GTK_STATE_NORMAL],cmap);
	}

	if (!icon) {
		button = gtk_button_new_with_label (lnk_get_name(lnk));
		if (lnk_get_comment(lnk))
			gtk_tooltips_set_tip (tips,button,lnk_get_comment(lnk),"ToolTips/");
	} else {
		button = gtk_button_new ();
		gtk_container_add (GTK_CONTAINER(button), icon);
		if (lnk_get_comment(lnk))
			gtk_tooltips_set_tip (tips,button,lnk_get_comment(lnk),"ToolTips/");
		else
			gtk_tooltips_set_tip (tips, button, lnk_get_name(lnk), "ToolTips/");
		/* we have to send the buttons some events to get tooltips to work
		 */
		gtk_signal_connect (GTK_OBJECT(button), "drag_motion",
				GTK_SIGNAL_FUNC(button_drag_motion), NULL);
		gtk_signal_connect (GTK_OBJECT(button), "drag_leave",
				GTK_SIGNAL_FUNC(button_drag_leave), NULL);
	}
	gtk_signal_connect (GTK_OBJECT(button), "drag_data_get",
			GTK_SIGNAL_FUNC(button_drag_data_get), icon);

	gtk_object_set_user_data (GTK_OBJECT(button), lnk);
	gtk_box_pack_start (GTK_BOX(box), button, FALSE, TRUE, 1);
	gtk_drag_dest_set (button, GTK_DEST_DEFAULT_ALL,
		target_table, NUM_TARGETS, GDK_ACTION_COPY|GDK_ACTION_ASK);
	gtk_drag_source_set (button, GDK_BUTTON1_MASK,
		target_table, NUM_TARGETS, GDK_ACTION_COPY|GDK_ACTION_MOVE);
	gtk_signal_connect (GTK_OBJECT(button), "clicked",
		GTK_SIGNAL_FUNC(button_clicked), (void *) lnk);
	gtk_signal_connect (GTK_OBJECT(button), "drag_data_received",
		GTK_SIGNAL_FUNC (button_drop_data), (void *) lnk);
	gtk_signal_connect (GTK_OBJECT(button), "button_press_event",
		GTK_SIGNAL_FUNC (pop_btn_menu), (void *) menu);
	gtk_widget_show_all (button);
}

/*
 * check for an icon file and copy the string
 */
char *
find_icon (char *prog)
{
	char file[PATH_MAX+1];
	struct stat st;

	sprintf (file, "%s/%s/.icons/mini-%s.xpm",
				getenv("HOME"), XAP_PATH, prog);
	if (stat (file, &st) == 0)
		return strdup (file);
	/* try global icon directory */
	sprintf (file, "%s/mini-%s.xpm", ICONDIR, prog);
	if (stat (file, &st) == 0)
		return strdup (file);
	return NULL;
}

/*
 * called if a drop is at the program-page label
 * this will create a new entry/button by linking the program
 * globals: Tips
 */
void
page_drop_data (GtkWidget *label, GdkDragContext *context, gint x, gint y,
		GtkSelectionData *data, guint info, guint time, page_t *pg)
{
	char *arg, *f, *tf;
	char path[PATH_MAX+1];
	int len, num;
	GList *list;
	uri_t *uri;
	lnk_t *lnk;

	if ((data->length > 0) && (data->format == 8)) {
		len = data->length;
		arg = g_malloc (len + 1);
		arg[len] = '\0';
		memcpy (arg, data->data, len);

		num = uri_parse_list (arg, &list);
		g_free (arg);
		if (!num) {
			gtk_drag_finish (context, FALSE, TRUE, time);
			return;
		} else {
			gtk_drag_finish (context, TRUE, TRUE, time);
		}
		uri_remove_file_prefix_from_list (list);
		while (num) {
			num--;
			uri = (uri_t *) (g_list_nth (list, num))->data;
			if (uri) {
				f = uri->url;
				if (!io_is_exec (f)) {
					fprintf (stderr, "%s: not an executable!\n", f);
				} else {
					/* create link file and add to page */
					tf = strrchr (f, '/');
					if (!tf)
						tf = f;
					else
						tf++;
					sprintf (path, "%s/%s", pg->path, tf);
					/* printf ("%s -> %s\n", f, lnk); */
					lnk = lnk_new ();
					lnk->self = strdup (path);
					lnk->exec = strdup (f);
					lnk->name = strdup (tf);
					lnk->type = LT_EXEC;
					lnk->icon = find_icon (tf);
					unlink (path);
					lnk_write (path, lnk);
					add_program_button (lnk, pg->w_box,
					 pg->w_btn_menu, Tips,
					 gtk_widget_get_colormap(gtk_widget_get_toplevel(label)));
				}
			}
		}
		uri_free_list (list);
		return;
	}
	gtk_drag_finish (context, FALSE, TRUE, time);
}



/*
 * put page to the front if drag comes over
 */
gboolean
page_drag_motion (GtkWidget *label,
		GdkDragContext *context, gint x, gint y, guint time, int i)
{
	GtkNotebook *notebook = GTK_NOTEBOOK(label->parent);
	if (i != gtk_notebook_get_current_page (notebook)) {
		gtk_notebook_set_page (notebook, i);
	}
	return (1);
}

/*
 * if file is a entry destop file read it else create a
 * structure in memory
 */
static lnk_t *
entry_or_link (char *file, char *label)
{
	lnk_t *lnk = NULL;
	struct stat st;
	char path[PATH_MAX+1];
	int len;

#ifdef DEBUG_XAP2
	printf ("entry_or_link (%s, %s)\n", file, label);
#endif
	if (lstat (file, &st) == 0) {
		if (!(S_ISLNK(st.st_mode))) {
			/* should be a desktop entry file
			 */
			lnk = lnk_read (file);
		}
	}
	if (!lnk) {
		/* create a new one
		 */
		lnk = lnk_new ();
		if (!lnk)
			return NULL;
		lnk->name = strdup (label);
		lnk->type = LT_EXEC;
		len = readlink(file, path, PATH_MAX);
		path[len] = '\0';
		lnk->exec = strdup (path);
		lnk->icon = find_icon (label);
		lnk->self = strdup (file);
	}
	return lnk;
}

/*
 * process application subdirectory
 */
void
process_dir (GtkWidget *page, char *path, GtkWidget *menu, GtkTooltips *tips,
				GdkColormap *cmap)
{
	DIR *dir;
	struct dirent *de;
	int len;
	char *name;
	lnk_t *lnk;

	dir = opendir (path);
	if (!dir) {
		perror (path);
		return;
	}
	while ((de = readdir(dir)) != NULL) {
		if (*de->d_name == '.') {
			/* ignore hidden files incl. "." and ".."
			 */
			continue;
		}
		len = strlen (path) + strlen (de->d_name) + 2;
		name = g_malloc (len);
		sprintf (name, "%s/%s", path, de->d_name);
		lnk = entry_or_link (name, de->d_name);
		add_program_button (lnk, page, menu, tips, cmap);
	}
	closedir (dir);
}

/*
 * add a page to the notebook and append the menu
 */
page_t *
add_page (GtkNotebook *notebook, char *label, char *base, void *btn_mn,
			int num, int bo)
{
	page_t *pg;

#ifdef DEBUG
	printf ("add_page() label=%s base=%s\n", label, base);
#endif
	pg = g_malloc (sizeof(page_t));
	if (!pg)
		return NULL;
	pg->path = g_malloc (strlen(base) + strlen(label) + 2);
	sprintf (pg->path, "%s/%s", base, label);

	pg->w_btn_menu = btn_mn;
	/* mark */
	if (bo == HORIZONTAL)
		pg->w_box = gtk_hbox_new (FALSE, 0);
	else
		pg->w_box = gtk_vbox_new (FALSE, 0);
	pg->w_label = gtk_label_new (label);

	gtk_drag_dest_set (pg->w_label, GTK_DEST_DEFAULT_ALL,
			target_table, NUM_TARGETS, GDK_ACTION_COPY);
	gtk_signal_connect (GTK_OBJECT(pg->w_label), "drag_motion",
			GTK_SIGNAL_FUNC(page_drag_motion), (void *) num);
	gtk_signal_connect (GTK_OBJECT(pg->w_label), "drag_data_received",
			GTK_SIGNAL_FUNC(page_drop_data), (void *) pg);

	gtk_notebook_append_page (notebook, pg->w_box, pg->w_label);
	gtk_object_set_user_data (GTK_OBJECT(pg->w_label), pg);
	gtk_widget_show_all (GTK_WIDGET(notebook));
	return (pg);
}

/*
 * globals: Tips
 */
void
build_pages (cfg_t *app, char *path, GtkWidget *menu)
{
	DIR *dir;
	struct dirent *de;
	char complete[PATH_MAX+1];
	page_t *pg;
	struct stat st;

	app->last = -1;
	dir = opendir (path);
	if (!dir) {
		perror (path);
		exit (1);
	}
	while ((de = readdir (dir)) != NULL) {
		if (io_is_hidden (de->d_name)) {
			/* skip ".", "..", ".trash", and so on
			 */
			continue;
		}
		sprintf (complete, "%s/%s", path, de->d_name);
		stat (complete, &st);
		if (!S_ISDIR(st.st_mode))
			continue;
		app->last++;
		pg = add_page (GTK_NOTEBOOK(app->notebook), de->d_name,
				path, menu, app->last, app->box_o);
		if (pg)
			process_dir (pg->w_box, complete, menu, Tips, app->cmap);
	}
	closedir (dir);
}

/*
 * callback for menu entry "delete"
 */
static void
cb_delete (GtkWidget *item, gpointer *data)
{
	lnk_t *lnk;
	GtkWidget *button = gtk_object_get_user_data(GTK_OBJECT(item->parent));

	lnk = gtk_object_get_user_data (GTK_OBJECT(button));
	if (lnk) {
#ifdef DEBUG
		printf ("deleting: %s\n", lnk->exec);
#endif
		if (lnk->self) {
			unlink (lnk->self);
		}
		lnk_free (lnk);
		gtk_widget_destroy (button);
	}
}


/*
 * callback for menu entry "options"
 */
static void
cb_options (GtkWidget *item, gpointer *data)
{
	lnk_t *lnk;
	GtkWidget *button, *label, *icon;
	cfg_t *app = (cfg_t *)data;
	GList *child_list;

	button = gtk_object_get_user_data(GTK_OBJECT(item->parent));

	lnk = gtk_object_get_user_data (GTK_OBJECT(button));
	if (!lnk)
		return;
	if (lnk_opt_change (lnk)) {
		unlink (lnk->self);
		lnk_write (lnk->self, lnk);
		icon = NULL;

		child_list = gtk_container_children (GTK_CONTAINER(button));
		while (child_list) {
			gtk_container_remove (GTK_CONTAINER(button),
				GTK_WIDGET(child_list->data));
			child_list = child_list->next;
		}
		if (lnk_get_comment(lnk))
			gtk_tooltips_set_tip (Tips,button,lnk_get_comment(lnk),"ToolTips/");
		else
			gtk_tooltips_set_tip (Tips, button, lnk_get_name(lnk), "ToolTips/");

		if (lnk->icon) {
			icon = new_icon (lnk->icon, button->window,
						&button->style->bg[GTK_STATE_NORMAL], app->cmap);
			if (icon) {
				gtk_container_add (GTK_CONTAINER(button), icon);
				gtk_widget_show (icon);
			} else {
				dlg_error (_("Can't create icon from"), lnk->icon);
			}
		}
		if ((!lnk->icon) || (!icon)) {
			label = gtk_label_new (lnk_get_name(lnk) ?
				lnk_get_name(lnk) : lnk->exec);
			if (label) {
				gtk_container_add (GTK_CONTAINER(button), label);
				gtk_widget_show (label);
			}
		}
	}
}

/*
 */
void
cb_page_new (GtkWidget *item, gpointer *data)
{
	cfg_t *app = (cfg_t *)data;
	char name[DLG_MAX];
	char path[PATH_MAX+1];

#ifdef DEBUG
	printf ("cb_page_new() path=%s\n", app->path);
#endif
	strcpy (name, "tools");
	if (dlg_string ("New page:", name) == DLG_RC_OK) {
		sprintf (path, "%s/%s", app->path, name);
		if (mkdir (path, 0xFFFFFFFF) != -1) {
			app->last++;
			add_page (GTK_NOTEBOOK(app->notebook), name, app->path,
						app->btn_menu, app->last, app->box_o);
		}
	}

}

/*
 * execute a named program
 */
void
cb_exec (GtkWidget *w, gpointer data)
{
	dlg_execute ((char *)data, NULL);
}

/*
 * rename a page
 * globals: Tips
 */
void
cb_page_rename (GtkWidget *item, gpointer data)
{
	cfg_t *app = (cfg_t *)data;
	char name[DLG_MAX], *p;
	char path[PATH_MAX+1];
	GtkWidget *nth, *label;
	int current;
	page_t *pg, *npg;

	current = gtk_notebook_get_current_page (GTK_NOTEBOOK(app->notebook));
	nth = gtk_notebook_get_nth_page (GTK_NOTEBOOK(app->notebook), current);
	label = gtk_notebook_get_tab_label (GTK_NOTEBOOK(app->notebook), nth);
	if (!label)
		return;
	p = NULL;
	gtk_label_get (GTK_LABEL(label), &p);
	if (!p)
		return;
	strcpy (name, p);
	pg = gtk_object_get_user_data (GTK_OBJECT(label));
	
	if (dlg_string (_("Rename notebook-page to:"), name) != DLG_RC_OK) {
		return;
	}
	if ((!*name) || strcmp (name, p) == 0)
		return;

	strcpy (path, pg->path);
	p = strrchr (path, '/');
	if (p)
		*(p+1) = '\0';
	strcat (path, name);

	if (rename (pg->path, path) == -1) {
		dlg_error (path, strerror(errno));
		return;
	}
	npg = add_page (GTK_NOTEBOOK(app->notebook),
				name, app->path, pg->w_btn_menu, current, app->box_o);
	if (npg)
		process_dir (npg->w_box, path, pg->w_btn_menu, Tips, app->cmap);
	gtk_notebook_remove_page (GTK_NOTEBOOK(app->notebook), current);
}

/*
 */
void
on_page_del (GtkWidget *item, gpointer *data)
{
}

/*
 * path: default value = "$HOME/.xap"
 * transient: boolean, should program run as transient window?
 */
void
gui_main (char *path, int transient, wgeo_t *geo, int nb_tabpos, int nb_boxlayout)
{
	GtkWidget *top, *button_menu, *page_menu, *menu_item;
	GtkWidget *notebook;
	GtkAccelGroup *accel;

	int i;
	cfg_t app;

	menu_entry button_me[] = {
		{ NULL,		NULL,	NULL },
		{ _("Options .."),		cb_options,		&app },
		{ _("Delete button"),	cb_delete,		NULL },
		{ NULL,		NULL,	NULL },
		{ _("Execute .."),		cb_exec, 		path },
		{ NULL,		NULL, 	NULL },
		{ _("Quit"),			gtk_main_quit,	NULL },
	};
#define LAST_BUTTON_MENU (sizeof(button_me)/sizeof(menu_entry))
	menu_entry page_me[] = {
		{ NULL,		NULL,	NULL },
		{ _("New page .."),	cb_page_new,	NULL, ACCEL, GDK_n, GDK_MOD1_MASK },
		{ _("Rename page .."),	cb_page_rename,	NULL},
		{ NULL,		NULL, 	NULL },
		{ _("Execute .."),		cb_exec, 		path },
		{ NULL,		NULL, 	NULL },
		{ _("Quit"),		gtk_main_quit,	NULL, ACCEL, GDK_q, GDK_MOD1_MASK },
	};
#define LAST_PAGE_MENU (sizeof(page_me)/sizeof(menu_entry))


	top = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_name (top, "xap");
	gtk_signal_connect (GTK_OBJECT(top), "destroy",
		GTK_SIGNAL_FUNC (exit), NULL);

	accel = gtk_accel_group_new();
	gtk_accel_group_attach (accel, GTK_OBJECT(top));

	notebook = gtk_notebook_new ();
	gtk_notebook_set_homogeneous_tabs ((GtkNotebook *)notebook, 1);
	gtk_notebook_set_tab_hborder ((GtkNotebook *)notebook, 0);
	gtk_notebook_set_tab_vborder ((GtkNotebook *)notebook, 0);
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK(notebook), nb_tabpos);
	gtk_container_set_border_width (GTK_CONTAINER(notebook), 0);
	gtk_container_add (GTK_CONTAINER(top), notebook);

	app.path = g_strdup (path);
	app.notebook = notebook;
	app.box_o = nb_boxlayout;

	button_menu = gtk_menu_new ();
	app.btn_menu = button_menu;
	for (i = 0; i < LAST_BUTTON_MENU; i++) {
		if (button_me[i].label)
			menu_item = gtk_menu_item_new_with_label (button_me[i].label);
		else
			menu_item = gtk_menu_item_new ();
		if (button_me[i].func) {
			if (button_me[i].data) {
				gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
					GTK_SIGNAL_FUNC(button_me[i].func), button_me[i].data);
			} else {
				gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
					GTK_SIGNAL_FUNC(button_me[i].func), (void *)path);
			}
		}
		gtk_menu_append (GTK_MENU(button_menu), menu_item);
		gtk_widget_show (menu_item);
	}
	gtk_menu_attach_to_widget (GTK_MENU(button_menu), GTK_WIDGET(notebook),
			menu_detach);

	page_menu = gtk_menu_new ();
	gtk_menu_set_accel_group (GTK_MENU(page_menu), accel);
	for (i = 0; i < LAST_PAGE_MENU; i++) {
		if (page_me[i].label)
			menu_item = gtk_menu_item_new_with_label (page_me[i].label);
		else
			menu_item = gtk_menu_item_new ();
		if (page_me[i].func) {
			if (page_me[i].data) {
				gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
					GTK_SIGNAL_FUNC(page_me[i].func), page_me[i].data);
			} else {
				gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
					GTK_SIGNAL_FUNC(page_me[i].func), (void *)&app);
			}
		}
		if (page_me[i].flags & ACCEL) {
			gtk_widget_add_accelerator(menu_item, "activate", accel,
				page_me[i].key, page_me[i].mod, GTK_ACCEL_VISIBLE);
		}
		gtk_menu_append (GTK_MENU(page_menu), menu_item);
		gtk_widget_show (menu_item);
	}
	gtk_menu_attach_to_widget (GTK_MENU(page_menu), GTK_WIDGET(notebook),
			menu_detach);

	gtk_notebook_popup_enable (GTK_NOTEBOOK(notebook));
	GTK_NOTEBOOK(notebook)->menu = page_menu;

	Tips = gtk_tooltips_new ();
	/* gtk_widget_show_all (top); */
	gtk_widget_realize (top);
	app.cmap = gtk_widget_get_colormap (top);
	build_pages (&app, path, button_menu);

	if (transient)
		gtk_window_set_transient_for (GTK_WINDOW(top), GTK_WINDOW(top));

	if (geo) {
		if (geo->width > 0 && geo->height > 0) {
			gtk_widget_set_usize (top, geo->width,geo->height);
		}
		if (geo->x > -1 && geo->height > -1) {
			gtk_widget_set_uposition (GTK_WIDGET(top),geo->x,geo->y);
		}
	}
	gtk_widget_show_all (top);
	gtk_main ();
}

