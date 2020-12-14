/*
 * gtk_exec.c
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
#include <unistd.h>	/* readlink() */
#include <dirent.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif
#include "i18n.h"
#include "gtk_exec.h"
#include "gtk_dlg.h"
#include "io.h"
#include "lnk.h"
#include "entry.h"
#include "gtk_dnd.h"


typedef struct {
	GtkWidget *top;
	GtkWidget *combo;
	GtkWidget *check;
	GtkWidget *cancel;
	GtkWidget *nice;
	int result;
	int in_terminal;
	int nice_val;
	char *cmd;
} dlg_t;

#ifndef TERMINAL
#define TERMINAL "rxvt"
#endif

/*
 */
static gint
entry_compare (gconstpointer ptr1, gconstpointer ptr2)
{
	return strcmp (((entry_t *)ptr1)->label, ((entry_t *)ptr2)->label);
}


/*
 * create an sorted item list
 */
static void
create_item_list (char *path, GtkWidget *combo)
{
	GList *list = NULL, *tmp_list;
	DIR *xap, *app;
	struct dirent *de, *deApp;
	char compl[PATH_MAX+1];
	char file[PATH_MAX+1];
	char nfile[PATH_MAX+1];
	lnk_t *lnk;
	entry_t *entry;
	int len;
	char *label;
	GtkWidget *li; /* list item */

#ifdef DEBUG
	printf ("create_item_list() path=%s\n", path);
#endif
	xap = opendir (path);
	if (!xap)
		return ;
	while ((de = readdir(xap)) != NULL) {
		if (*de->d_name == '.')
			continue;
		sprintf (compl, "%s%c%s", path, G_DIR_SEPARATOR, de->d_name);
		app = opendir(compl);
		if (!app)
			continue;
		while ((deApp = readdir(app)) != NULL) {
			if (*deApp->d_name == '.')
				continue;
			/* check if it is a desktop entry or resolve the link
			 */
			entry = NULL;
			sprintf (file, "%s%c%s", compl, G_DIR_SEPARATOR, deApp->d_name);
			if (io_is_link (file)) {
				/* symbolic link, find out where it points to
				 */
				len = readlink (file, nfile, PATH_MAX);
				nfile[len] = '\0';
				entry = entry_new_by_path (nfile);
			} else if (io_is_file (file)) {
				/* may be a desktop entry file
				 */
				lnk = lnk_read (file);
				if (lnk && lnk->exec) {
					entry = entry_new_by_type (lnk->exec, FT_EXE);
					label = lnk_get_name(lnk);
					if (!label)
						label = lnk->exec;
					g_free (entry->label);
					entry->label = g_strdup (label);
					lnk_free (lnk);
				}
			}
			if (entry) {
				list = g_list_append (list, entry);
			}
		}
		closedir(app);
	}
	closedir (xap);

	if (list) {
		gtk_list_clear_items (GTK_LIST(GTK_COMBO(combo)->list), 0, -1);
		list = g_list_sort (list, entry_compare);
		tmp_list = list;
		while (tmp_list) {
			li = gtk_list_item_new_with_label (
				(char *) ((entry_t *)tmp_list->data)->label);
			gtk_widget_show (li);
			gtk_container_add (GTK_CONTAINER(GTK_COMBO(combo)->list), li);
			gtk_combo_set_item_string (GTK_COMBO(combo), GTK_ITEM(li),
				((entry_t *)tmp_list->data)->path);
			tmp_list = tmp_list->next;
		}
		tmp_list = list;
		while (tmp_list) {
			entry_free ((entry_t *)tmp_list->data);
			tmp_list = tmp_list->next;
		}
		g_list_free (list);
	}
	return ;
}

/*
 */
static void
on_btn_cancel_clicked (GtkWidget *btn, gpointer *data)
{
	dlg_t *dlg = (dlg_t *) data;

	dlg->result = DLG_RC_CANCEL;
	gtk_main_quit();
}

/*
 */
static void
on_btn_ok_clicked (GtkWidget *ok, gpointer data)
{
	dlg_t *dlg = (dlg_t *)data;

	dlg->cmd = g_strdup (gtk_entry_get_text
				(GTK_ENTRY(GTK_COMBO(dlg->combo)->entry)));
	dlg->in_terminal = GTK_TOGGLE_BUTTON(dlg->check)->active;
	dlg->result = DLG_RC_OK;
	dlg->nice_val =gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dlg->nice));
	gtk_main_quit();
}

/*
 */
static GtkWidget *
create_w_exec (dlg_t *dlg, char *file)
{
	GtkWidget *w_exec;
	GtkWidget *dialog_vbox;
	GtkWidget *hbox_entry;
	GtkWidget *label;
	GtkWidget *combo;
	GtkWidget *combo_entry;
	guint cb_terminal_key;
	GtkWidget *cb_terminal;
	GtkWidget *hbox_nice;
	GtkObject *sb_nice_adj;
	GtkWidget *sb_nice;
	GtkWidget *lbl_nice;
	GtkWidget *dialog_action_area;
	GtkWidget *hbox;
	guint btn_ok_key;
	GtkWidget *btn_ok;
	guint btn_cancel_key;
	GtkWidget *btn_cancel;
	GtkAccelGroup *accel_group;
	char *title;

	if (file) {
		title = _("Open with ..");
	} else {
		title = _("Execute ..");
	}

	accel_group = gtk_accel_group_new ();

	w_exec = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (w_exec), title);
	gtk_object_set_data (GTK_OBJECT (w_exec), "w_exec", w_exec);
	gtk_window_set_policy (GTK_WINDOW (w_exec), TRUE, TRUE, FALSE);
	gtk_window_position (GTK_WINDOW(w_exec), GTK_WIN_POS_MOUSE);
	gtk_window_set_modal (GTK_WINDOW(w_exec), TRUE);

	dialog_vbox = GTK_DIALOG (w_exec)->vbox;
	gtk_object_set_data (GTK_OBJECT (w_exec), "dialog_vbox", dialog_vbox);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_vbox), 6);

	hbox_entry = gtk_hbox_new (FALSE, 4);
	gtk_object_set_data (GTK_OBJECT (w_exec), "hbox_entry", hbox_entry);
	gtk_widget_show (hbox_entry);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox_entry, TRUE, TRUE, 0);

	combo = gtk_combo_new ();
	gtk_object_set_data (GTK_OBJECT (w_exec), "combo", combo);
	gtk_widget_show (combo);
	gtk_box_pack_start (GTK_BOX (hbox_entry), combo, TRUE, TRUE, 0);
	dlg->combo = combo;

	combo_entry = GTK_COMBO (combo)->entry;
	gtk_object_set_data (GTK_OBJECT (w_exec), "combo_entry", combo_entry);
	gtk_widget_show (combo_entry);

	if (file) {
		label = gtk_label_new (file);
		gtk_box_pack_start (GTK_BOX(hbox_entry), label, FALSE, TRUE, 0);
	}

	cb_terminal = gtk_check_button_new_with_label ("");
	cb_terminal_key = gtk_label_parse_uline (
						GTK_LABEL (GTK_BIN (cb_terminal)->child),
						_("S_tart application in a terminal"));
	gtk_widget_add_accelerator (cb_terminal, "clicked", accel_group,
	                            cb_terminal_key, GDK_MOD1_MASK, 0);
	dlg->check = cb_terminal;
	gtk_widget_show (cb_terminal);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), cb_terminal, FALSE, FALSE, 0);

	hbox_nice = gtk_hbox_new (FALSE, 4);
	gtk_widget_show (hbox_nice);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox_nice, FALSE, FALSE, 0);

	sb_nice_adj = gtk_adjustment_new (0, 0, 19, 1, 5, 5);
	sb_nice = gtk_spin_button_new (GTK_ADJUSTMENT (sb_nice_adj), 1, 0);
	gtk_widget_show (sb_nice);
	dlg->nice = sb_nice;
	gtk_box_pack_start (GTK_BOX (hbox_nice), sb_nice, FALSE, FALSE, 0);
	gtk_spin_button_set_update_policy (
					GTK_SPIN_BUTTON (sb_nice), GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (sb_nice), TRUE);

	lbl_nice = gtk_label_new (_("Nice value"));
	gtk_widget_show (lbl_nice);
	gtk_box_pack_start (GTK_BOX (hbox_nice), lbl_nice, FALSE, TRUE, 0);

	dialog_action_area = GTK_DIALOG (w_exec)->action_area;
	gtk_object_set_data (GTK_OBJECT (w_exec),
				"dialog_action_area", dialog_action_area);
	gtk_widget_show (dialog_action_area);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_action_area), 10);

	hbox = gtk_hbox_new (TRUE, 100);
	gtk_object_set_data (GTK_OBJECT (w_exec), "hbox", hbox);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (dialog_action_area), hbox, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 1);

	btn_ok = gtk_button_new_with_label ("");
	btn_ok_key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (btn_ok)->child),
					_("_OK"));
	gtk_widget_add_accelerator (btn_ok, "clicked", accel_group,
	                            btn_ok_key, GDK_MOD1_MASK, 0);
	gtk_object_set_data (GTK_OBJECT (w_exec), "btn_ok", btn_ok);
	gtk_widget_show (btn_ok);
	gtk_box_pack_start (GTK_BOX (hbox), btn_ok, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (btn_ok), 2);
	GTK_WIDGET_SET_FLAGS (btn_ok, GTK_CAN_DEFAULT);

	btn_cancel = gtk_button_new_with_label ("");
	btn_cancel_key = gtk_label_parse_uline (
						GTK_LABEL (GTK_BIN (btn_cancel)->child),
						_("_Cancel"));
	gtk_widget_add_accelerator (btn_cancel, "clicked", accel_group,
	                            btn_cancel_key, GDK_MOD1_MASK, 0);
	gtk_object_set_data (GTK_OBJECT (w_exec), "btn_cancel", btn_cancel);
	dlg->cancel = btn_cancel;
	gtk_widget_show (btn_cancel);
	gtk_box_pack_start (GTK_BOX (hbox), btn_cancel, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (btn_cancel), 2);
	GTK_WIDGET_SET_FLAGS (btn_cancel, GTK_CAN_DEFAULT);

	gtk_widget_add_accelerator (btn_cancel, "clicked", accel_group,
	                            GDK_Escape, 0,
	                            GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator (btn_cancel, "clicked", accel_group,
	                            GDK_W, GDK_MOD1_MASK,
	                            GTK_ACCEL_VISIBLE);

	gtk_signal_connect (GTK_OBJECT (btn_ok), "clicked",
	                    GTK_SIGNAL_FUNC (on_btn_ok_clicked),
	                    (void *)dlg);
	gtk_signal_connect (GTK_OBJECT (btn_cancel), "clicked",
	                    GTK_SIGNAL_FUNC (on_btn_cancel_clicked),
	                    (void *)dlg);

	gtk_window_add_accel_group (GTK_WINDOW (w_exec), accel_group);

	return w_exec;
}

/*
 * create a modal dialog and handle it
 */
gint
dlg_open_with (char *xap, char *defval, char *file)
{
	char cmd[PATH_MAX*3 + 6];
	dlg_t *dlg;
	int result;

	dlg = g_malloc (sizeof (dlg_t));
	if (!dlg)
		return DLG_RC_CANCEL;

	dlg->result = 0;
	dlg->in_terminal = 0;
	dlg->top = create_w_exec (dlg, file);

	/* gtk_widget_grab_default (ok); */

	create_item_list (xap, dlg->combo);
	if (defval) {
		gtk_entry_set_text (GTK_ENTRY(GTK_COMBO(dlg->combo)->entry), defval);
		gtk_editable_select_region(GTK_EDITABLE(GTK_COMBO(dlg->combo)->entry),0,-1);
	} else {
		gtk_entry_set_text (GTK_ENTRY(GTK_COMBO(dlg->combo)->entry), "");
	}

	dnd_entry_init (GTK_COMBO(dlg->combo)->entry, NULL);

	gtk_widget_show_all (dlg->top);
	gtk_main ();
	gtk_widget_destroy (dlg->top);

	/* */
	if (dlg->result == DLG_RC_OK) {
		if (!dlg->cmd || !*dlg->cmd) {
			/* empty entry field
			 */
			return 0;
		}
		if (dlg->in_terminal) {
			/* start in terminal window */
			if (file) {
				sprintf (cmd, "%s -e %s \"%s\" &", TERMINAL, dlg->cmd, file);
			} else {
				sprintf (cmd, "%s -e %s &", TERMINAL, dlg->cmd);
			}
		} else {
			if (file) {
				sprintf (cmd, "%s \"%s\" &", dlg->cmd, file);
			} else {
				sprintf (cmd, "%s &", dlg->cmd);
			}
		}
		g_free (dlg->cmd);
#ifdef DEBUG
		printf ("dlg_open_with() cmd=%s\n", cmd);
#endif
		io_system_nice (cmd, dlg->nice_val);
	}
 	/* free the program list
	 */
	result = dlg->result;
	g_free (dlg);
	return (result);
}

