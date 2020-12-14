/*
 * callbacks.c
 *
 * Copyright (C) 2000 Rasca, Berlin
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "callbacks.h"
#include "gui.h"
#include "io.h"
#include "support.h"
#include "gtk_dnd.h"
#include "uri.h"
#include "entry.h"
#include "reg.h"

GtkWidget *wAbout;
GtkWidget *wFilesel;
GtkWidget *wDirsel;
GtkWidget *wTop;
GtkWidget *wPopup;
GList *sReg;


/*
 * change cursor to indicate process
 */
static void
cursor_wait(GtkWidget *w)
{
	GdkCursor *cursor;
	cursor = gdk_cursor_new (GDK_WATCH);
	gdk_window_set_cursor (w->window, cursor);
	gdk_flush();
	gdk_cursor_destroy (cursor);
}

/* release the wait cursor
 */
#define cursor_reset(w) gdk_window_set_cursor(w->window,NULL);


/*
 * if quit in the menu was pressed
 */
void
on_mquit_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	exit(0);
}

/*
 * if about in the menu was pressed
 */
void
on_mabout_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gtk_widget_show (wAbout);
}

/*
 * if save in the menu was pressed
 */
void
on_msave_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gtk_widget_show (wFilesel);
}


#define CANCEL 0
#define OK 1
static int gState = OK;

/*
 * catch escape key
 */
gboolean
on_btn_find_key_press                  (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
	if (event->keyval == GDK_Escape) {
		gState = CANCEL;
		return TRUE;
	}
	return FALSE;
}

/*
 * if button "find .." was pressed
 */
void
on_btn_find_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *pattern, *root, *list, *item;
	GtkWidget *radio_file, *radio_dir, *radio_all, *pbar, *sbar;
	GtkWidget *opt_this_fs, *opt_follow, *opt_nouser, *opt_nogroup;
	char *val_pattern, *val_root;
	char cmd[PATH_MAX*2], status[64];
	char opt[512] = { "" };
	FILE *pipe;
	char line[PATH_MAX];
	int len;
	int num;
	gfloat pos;

	gState = OK;

	pbar = lookup_widget (GTK_WIDGET(button), "progressbar");
	if (!pbar)
		return;
	sbar = lookup_widget (GTK_WIDGET(button), "statusbar");
	if (!sbar)
		return;
	list = lookup_widget (GTK_WIDGET(button), "lst_result");
	if (!list)
		return;
	radio_file = lookup_widget (GTK_WIDGET(button), "radio_file");
	if (!radio_file)
		return;
	radio_dir = lookup_widget (GTK_WIDGET(button), "radio_dir");
	if (!radio_dir)
		return;
	radio_all = lookup_widget (GTK_WIDGET(button), "radio_all");
	if (!radio_all)
		return;
	opt_this_fs = lookup_widget (GTK_WIDGET(button), "cb_this_fs");
	if (!opt_this_fs)
		return;
	opt_follow = lookup_widget (GTK_WIDGET(button), "cb_symlinks");
	if (!opt_follow)
		return;
	opt_nouser = lookup_widget (GTK_WIDGET(button), "cb_no_user");
	if (!opt_nouser)
		return;
	opt_nogroup = lookup_widget (GTK_WIDGET(button), "cb_no_group");
	if (!opt_nogroup)
		return;

	pattern = lookup_widget (GTK_WIDGET(button), "coe_pattern");
	if (!pattern)
		return;
	val_pattern = gtk_entry_get_text (GTK_ENTRY(pattern));

	root = lookup_widget (GTK_WIDGET(button), "coe_root");
	if (!root)
		return;
	val_root = gtk_entry_get_text (GTK_ENTRY(root));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(opt_this_fs)))
		strcat (opt, " -xdev");
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(opt_follow)))
		strcat (opt, " -follow");
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(opt_nouser)))
		strcat (opt, " -nouser");
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(opt_nogroup)))
		strcat (opt, " -nogroup");

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(radio_file)))
		sprintf (cmd,"find '%s' %s -name '%s' -a -type f -printf \"%%p\\n\\c\"",
				val_root, opt, val_pattern);
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_dir)))
		sprintf (cmd,"find '%s' %s -name '%s' -a -type d -printf \"%%p\\n\\c\"",
				val_root, opt, val_pattern);
	else /* file and directory */
		sprintf (cmd, "find '%s' %s -name '%s' -printf \"%%p\\n\\c\"",
				val_root, opt, val_pattern);
	pipe = popen (cmd, "r");
	if (!pipe)
		return;

	cursor_wait(gtk_widget_get_toplevel(list));
	sprintf (status, "Preparing ..");
	gtk_statusbar_push (GTK_STATUSBAR(sbar), 1, status);

	while (gtk_events_pending())
		gtk_main_iteration();

	gtk_list_clear_items (GTK_LIST(list), 0, -1);

	gtk_progress_bar_set_activity_step (GTK_PROGRESS_BAR(pbar), 1);

	num = 0;
	pos = 0;
	while (fgets (line, PATH_MAX, pipe) != NULL) {
		len = strlen (line);
		if (len > 0) {
			line[len-1] = '\0';
			item = gtk_list_item_new_with_label (line);
			gtk_widget_show (item);
			gtk_container_add (GTK_CONTAINER(list), item);
			sprintf (status, "%d items", ++num);
			gtk_statusbar_push (GTK_STATUSBAR(sbar), 1, status);
			pos += 0.1;
			if (pos > 1) pos = 0;
			gtk_progress_bar_update (GTK_PROGRESS_BAR(pbar), pos);
		}
		while (gtk_events_pending()) {
			gtk_main_iteration();
			if (!gState) {
				sprintf (status, "Search canceled .. please wait");
				gtk_statusbar_push (GTK_STATUSBAR(sbar), 1, status);
				while (gtk_events_pending())
					gtk_main_iteration();
				goto END;
			}
		}
	}
END:
	pclose (pipe);
	/* gtk_widget_show_all(list); */

	sprintf (status, "%d items found.", num);
	gtk_statusbar_push (GTK_STATUSBAR(sbar), 1, status);
	while (gtk_events_pending()) {
		gtk_main_iteration();
	}
	cursor_reset(gtk_widget_get_toplevel(list));
}

static GtkTargetEntry target_table[] = {
    {"text/uri-list",	0,  				TARGET_URI_LIST },
    {"text/plain", 		0,  				TARGET_PLAIN },
    {"STRING",     		0,  				TARGET_STRING },	
};
#define NUM_TARGETS (sizeof(target_table)/sizeof(GtkTargetEntry))

/*
 */
void
on_coe_root_realize                    (GtkWidget       *widget,
                                        gpointer         user_data)
{
	char *base = getenv ("XFI_DIR");
	if (!base)
		base = getenv ("HOME");
	gtk_entry_set_text (GTK_ENTRY(widget), base);
	gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_ALL,
		target_table, NUM_TARGETS, GDK_ACTION_COPY);
}

/*
 */
void
on_coe_drag_data_received         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{
	int nitems;
	GList *list;

	if ((data->length <= 0) || (data->format != 8)) {
		gtk_drag_finish (drag_context, FALSE, TRUE, time);
		return;
	}
	nitems = uri_parse_list (data->data, &list);
	if (nitems <= 0) {
		gtk_drag_finish (drag_context, FALSE, TRUE, time);
		return;
	}
	uri_remove_file_prefix_from_list (list);
	gtk_entry_set_text (GTK_ENTRY(widget), ((uri_t *)(list->data))->url);
}

void
on_btn_about_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_hide (wAbout);
}


/*
 * save search result in a text file
 */
void
on_ok_button_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	char *file, *str;
	FILE *fp;
	GList *child;
	GtkWidget *list;

	gtk_widget_hide (wFilesel);
	file = gtk_file_selection_get_filename (GTK_FILE_SELECTION(wFilesel));
	list = lookup_widget (GTK_WIDGET(wTop), "lst_result");
	if (file && *file) {
		fp = fopen (file, "w");
		if (!fp)
			return;
		child = GTK_LIST(list)->children;
		while (child) {
			gtk_label_get (GTK_LABEL(GTK_BIN(child->data)->child), &str);
			fprintf (fp, "%s\n", str);
			child = child->next;
		}
		fclose (fp);
	}
}


void
on_cancel_button_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_hide (wFilesel);

}


/*
 */
gboolean
on_lst_result_key_press_event          (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
	printf ("event state=%d\n", event->state);
  	return FALSE;
}


void
on_m_select_all		                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *list;

	list = lookup_widget (GTK_WIDGET(wTop), "lst_result");
	gtk_list_select_all (GTK_LIST(list));
}


void
on_m_unselect_all                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *list;

	list = lookup_widget (GTK_WIDGET(wTop), "lst_result");
	gtk_list_unselect_all (GTK_LIST(list));
}

/*
 */
void
on_pm_delete                           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *list;
	GList *selection, *remove;
	char **cmd;
	int num, i;

	list = lookup_widget (GTK_WIDGET(wTop), "lst_result");

	num = 0;
	selection = GTK_LIST(list)->selection;
	while (selection) {
		selection = selection->next;
		num++;
	}
	cmd = malloc (sizeof(char*) * (num + 3));
	cmd[0] = PLUGINDIR"/xcp";
	cmd[1] = "-mt";
	i = 2;
	selection = GTK_LIST(list)->selection;
	while (selection) {
		gtk_label_get (GTK_LABEL(GTK_BIN(selection->data)->child), &cmd[i++]);
		selection = selection->next;
	}
	cmd[i] = NULL;
	io_system_var (cmd, num + 2);
	free (cmd);
	remove = g_list_copy (GTK_LIST(list)->selection);
	gtk_list_remove_items (GTK_LIST(list), remove);
	g_list_free (remove);
}

/*
 */
void
on_pm_attribute_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *list;
	GList *selection;
	char **cmd;
	int num, i;

	list = lookup_widget (GTK_WIDGET(wTop), "lst_result");

	num = 0;
	selection = GTK_LIST(list)->selection;
	while (selection) {
		selection = selection->next;
		num++;
	}
	if (num < 1) {
		return;
	}
	cmd = malloc (sizeof(char*) * (num + 2));
	cmd[0] = PLUGINDIR"/xat";
	i = 1;
	selection = GTK_LIST(list)->selection;
	while (selection) {
		gtk_label_get (GTK_LABEL(GTK_BIN(selection->data)->child), &cmd[i++]);
		selection = selection->next;
	}
	cmd[i] = NULL;
	io_system_var (cmd, num + 1);
	free (cmd);
}


/*
 */
void
on_pm_opendir_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *list;
	char *file;
	GList *selection;
	char *cmd[2], *p;
	int len;

	list = lookup_widget (GTK_WIDGET(wTop), "lst_result");
	selection = GTK_LIST(list)->selection;
	if (selection) {
		gtk_label_get (GTK_LABEL(GTK_BIN(selection->data)->child), &file);
		len = strlen (file);
		if (file[len-1] != G_DIR_SEPARATOR) {
			file = strdup (file);
			p = strrchr (file, G_DIR_SEPARATOR);
			if (p) {
				*p = '\0';
			}
		}
		cmd[0] = INSTDIR"/bin/xwf";
		cmd[1] = file;
		io_system_var (cmd, 2);
	}
}

/*
 * pop up context menu
 */
gboolean
on_lst_result_button_press_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	GtkWidget *item;
	char *str = NULL;
	entry_t *entry;
	char path[PATH_MAX*2], *exec;

	if (event->button == 3) {
		/* popup context menu
		 */
		gtk_menu_popup (GTK_MENU(wPopup),NULL,NULL,NULL,NULL, 3, event->time);
		return TRUE;
	} else if ((event->button == 1) && (event->type == GDK_2BUTTON_PRESS)) {
		cursor_wait (gtk_widget_get_toplevel(widget));
		/* double clicked -> start application or open file
		 */
		item = gtk_get_event_widget ((GdkEvent *) event);
		gtk_label_get (GTK_LABEL(GTK_BIN(item)->child), &str);
		entry = entry_new_by_path (str);
		if (!str || !entry) {
			cursor_reset (gtk_widget_get_toplevel(widget));
			return TRUE;
		}

		if (EN_IS_EXE(entry)) {
			/* run the application
			 */
			sprintf (path, "'%s' &", entry->path);
			io_system (path);
		} else if (EN_IS_DIR(entry)) {
			sprintf (path, "xwf '%s' &", entry->path);
			io_system (path);
		} else {
			/* open file if we have an application registered
			 */
			exec = reg_app_by_file (sReg, entry->path);
			if (exec) {
				sprintf (path, "%s '%s'&", exec, entry->path);
				io_system (path);
			}
		}
		entry_free (entry);
		usleep (200000);
		cursor_reset (gtk_widget_get_toplevel(widget));
		return TRUE;
	}
	return FALSE;
}



/*
 * register target formats
 */
void
on_coe_pattern_realize                 (GtkWidget       *widget,
                                        gpointer         user_data)
{
	gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_ALL,
		target_table, NUM_TARGETS, GDK_ACTION_COPY);
}


void
on_btn_browse_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_show (wDirsel);
}


void
on_btn_dir_ok_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	char *file;
	GtkWidget *coe_root;

	file = gtk_file_selection_get_filename (GTK_FILE_SELECTION(wDirsel));
	gtk_widget_hide (wDirsel);
	if (!file || !*file)
		return;
	coe_root = lookup_widget (GTK_WIDGET(wTop), "coe_root");
	if (!coe_root)
		return;
	gtk_entry_set_text (GTK_ENTRY(coe_root), file);
}


void
on_btn_dir_cancel_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_hide (wDirsel);
}

/*
 * hide file list
 */
void
on_wdirsel_realize                     (GtkWidget       *widget,
                                        gpointer         user_data)
{
	gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION(widget));
	gtk_widget_hide (GTK_WIDGET(GTK_FILE_SELECTION(widget)->file_list)->parent);
}

/*
 * DND source
 * define source types for DND
 */
void
on_lst_result_realize                  (GtkWidget       *widget,
                                        gpointer         user_data)
{
	gtk_drag_source_set (widget, GDK_BUTTON1_MASK|GDK_BUTTON2_MASK,
		target_table, NUM_TARGETS, GDK_ACTION_COPY|GDK_ACTION_ASK);
}

static char *gDnd_data = NULL;

/*
 * DND source
 */
void
on_lst_result_drag_data_get            (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{
	GList *selection;
	int num, len = 0, slen;
	char *file, *p;

	num = g_list_length (GTK_LIST(widget)->selection);
	if (num < 1)
		return;
	selection = GTK_LIST(widget)->selection;
	switch (info) {
		case TARGET_URI_LIST:
			while (selection) {
				gtk_label_get(GTK_LABEL(GTK_BIN(selection->data)->child),&file);
				len += strlen (file) + 5 + 2;
				selection = selection->next;
			}
			break;
		case TARGET_STRING:
		case TARGET_PLAIN:
			while (selection) {
				gtk_label_get(GTK_LABEL(GTK_BIN(selection->data)->child),&file);
				len += strlen (file) + 1;
				selection = selection->next;
			}
			break;
		default:
			return;
			break;
	}
	p = gDnd_data = malloc (len + 1);
	gDnd_data[0] = '\0';
	selection = GTK_LIST(widget)->selection;
	switch (info) {
		case TARGET_URI_LIST:
			while (selection) {
				gtk_label_get(GTK_LABEL(GTK_BIN(selection->data)->child),&file);
				slen = strlen (file);
				sprintf (p, "file:%s\x0D\x0A", file);
				p += slen + 5 + 2;
				selection = selection->next;
			}
			break;
		case TARGET_STRING:
		case TARGET_PLAIN:
			while (selection) {
				gtk_label_get(GTK_LABEL(GTK_BIN(selection->data)->child),&file);
				strcat (gDnd_data, file);
				if ((info != TARGET_STRING) || (num > 1))
					strcat (gDnd_data, "\n");
				selection = selection->next;
			}
			break;
		default:
			return;
			break;
	}
	gtk_selection_data_set (data, data->target, 8, gDnd_data, len);
}

/*
 * DND source
 */
void
on_lst_result_drag_data_delete         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data)
{
	if (gDnd_data) {
		free (gDnd_data);
		gDnd_data = NULL;
	}
}


