/*
 * lnk_opt.c
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "i18n.h"
#include "lnk.h"
#include "xap_fs.h"
#include "gtk_dnd.h"

static int gState = 0;
static GtkWidget *gOpt = NULL;

static GtkWidget *gEn_label;
static GtkWidget *gEn_cmnt;
static GtkWidget *gEn_exec;
static GtkWidget *gEn_icon;
static GtkWidget *gCb_terminal;

/*
 */
static void
on_btn_browse_exec_clicked (GtkButton *button, gpointer user_data)
{
	char *file;
	char *dir = NULL, *p;

	dir = gtk_entry_get_text (GTK_ENTRY(gEn_exec));
	if (dir) {
		dir = g_strdup (dir);
		p = strrchr (dir, G_DIR_SEPARATOR);
		if (p) {
			*p = '\0';
		}
	}
	file = fs_get_file (dir);
	if (file) {
		gtk_entry_set_text (GTK_ENTRY(gEn_exec), file);
		g_free (file);
	}
	if (dir)
		g_free (dir);
}

/*
 */
static void
on_btn_browse_icon_clicked (GtkButton *button, gpointer user_data)
{
	char *file;
	char *dir = NULL, *p;

	dir = gtk_entry_get_text (GTK_ENTRY(gEn_icon));
	if (dir) {
		dir = g_strdup (dir);
		p = strrchr (dir, G_DIR_SEPARATOR);
		if (p) {
			*p = '\0';
		}
	}
	file = fs_get_file (dir);
	if (file) {
		gtk_entry_set_text (GTK_ENTRY(gEn_icon), file);
		g_free (file);
	}
	if (dir)
		g_free (dir);
}

/*
 */
static gboolean
on_lnk_opt_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	gtk_widget_hide (gOpt);
	gtk_main_quit();
	return TRUE;
}

/*
 */
static void
on_btn_ok_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	gState = 1;
	gtk_widget_hide (gOpt);
	gtk_main_quit();
}

/*
 */
static void
on_btn_cancel_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_hide (gOpt);
	gtk_main_quit();
}

/*
 * create modale dialog window
 */
static GtkWidget*
create_lnk_opt (void)
{
	GtkWidget *lnk_opt;
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *lbl_cmnt;
	GtkWidget *lbl_exec;
	GtkWidget *lbl_icon;
	GtkWidget *lbl_label;
	GtkWidget *en_icon;
	GtkWidget *en_cmnt;
	GtkWidget *en_label;
	GtkWidget *en_exec;
	GtkWidget *cb_terminal;
	GtkWidget *btn_browse_exec;
	GtkWidget *btn_browse_icon;
	GtkWidget *hbox;
	GtkWidget *btn_ok;
	GtkWidget *btn_cancel;

	lnk_opt = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width (GTK_CONTAINER (lnk_opt), 2);
	gtk_window_set_title (GTK_WINDOW (lnk_opt), _("Options"));
	gtk_window_set_position (GTK_WINDOW(lnk_opt), GTK_WIN_POS_MOUSE);
	gtk_window_set_modal (GTK_WINDOW (lnk_opt), TRUE);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (lnk_opt), vbox);

	table = gtk_table_new (5, 3, FALSE);
	gtk_widget_show (table);

	gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (table), 4);
	gtk_table_set_row_spacings (GTK_TABLE (table), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table), 4);

	/* table elements
	 */
	lbl_label = gtk_label_new (_("Label:"));
	gtk_widget_show (lbl_label);
	gtk_table_attach (GTK_TABLE (table), lbl_label, 0, 1, 0, 1,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify (GTK_LABEL (lbl_label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment (GTK_MISC(lbl_label), 1, 0.5);

	lbl_cmnt = gtk_label_new (_("Comment:"));
	gtk_widget_show (lbl_cmnt);
	gtk_table_attach (GTK_TABLE (table), lbl_cmnt, 0, 1, 1, 2,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify (GTK_LABEL (lbl_cmnt), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment (GTK_MISC(lbl_cmnt), 1, 0.5);

	lbl_exec = gtk_label_new (_("Execute:"));
	gtk_widget_show (lbl_exec);
	gtk_table_attach (GTK_TABLE (table), lbl_exec, 0, 1, 2, 3,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify (GTK_LABEL (lbl_exec), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment (GTK_MISC(lbl_exec), 1, 0.5);

	lbl_icon = gtk_label_new (_("Icon (XPM):"));
	gtk_widget_show (lbl_icon);
	gtk_table_attach (GTK_TABLE (table), lbl_icon, 0, 1, 3, 4,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify (GTK_LABEL (lbl_icon), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment (GTK_MISC(lbl_icon), 1, 0.5);

	btn_browse_exec = gtk_button_new_with_label ("...");
	gtk_widget_show (btn_browse_exec);
	gtk_table_attach (GTK_TABLE (table), btn_browse_exec, 2, 3, 2, 3,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

	en_icon = gtk_entry_new ();
	gtk_widget_show (en_icon);
	gtk_table_attach (GTK_TABLE (table), en_icon, 1, 2, 3, 4,
		(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	gEn_icon = en_icon;
	dnd_entry_init (en_icon, NULL);

	btn_browse_icon = gtk_button_new_with_label ("...");
	gtk_widget_show (btn_browse_icon);
	gtk_table_attach (GTK_TABLE (table), btn_browse_icon, 2, 3, 3, 4,
		(GtkAttachOptions) (0),
		(GtkAttachOptions) (0), 0, 0);

	en_cmnt = gtk_entry_new ();
	gtk_widget_show (en_cmnt);
	gtk_table_attach (GTK_TABLE (table), en_cmnt, 1, 3, 1, 2,
		(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	gEn_cmnt = en_cmnt;
	dnd_entry_init (en_cmnt, NULL);

	en_label = gtk_entry_new ();
	gtk_widget_show (en_label);
	gtk_table_attach (GTK_TABLE (table), en_label, 1, 3, 0, 1,
		(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	gEn_label = en_label;
	dnd_entry_init (en_label, NULL);

	en_exec = gtk_entry_new ();
	gtk_widget_show (en_exec);
	gtk_table_attach (GTK_TABLE (table), en_exec, 1, 2, 2, 3,
		(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	gEn_exec = en_exec;
	dnd_entry_init (en_exec, NULL);

	cb_terminal = gtk_check_button_new_with_label
				(_("Start in Terminal Window"));
	gtk_widget_show (cb_terminal);
	gtk_table_attach (GTK_TABLE (table), cb_terminal, 1, 2, 4, 5,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	gCb_terminal = cb_terminal;

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);

	btn_ok = gtk_button_new_with_label ("Ok");
	gtk_widget_show (btn_ok);
	gtk_box_pack_start (GTK_BOX (hbox), btn_ok, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (btn_ok), 2);
	GTK_WIDGET_SET_FLAGS (btn_ok, GTK_CAN_DEFAULT);

	btn_cancel = gtk_button_new_with_label (_("Cancel"));
	gtk_widget_show (btn_cancel);
	gtk_box_pack_start (GTK_BOX (hbox), btn_cancel, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (btn_cancel), 2);
	GTK_WIDGET_SET_FLAGS (btn_cancel, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (lnk_opt), "delete_event",
                      GTK_SIGNAL_FUNC (on_lnk_opt_delete_event),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (btn_browse_exec), "clicked",
                      GTK_SIGNAL_FUNC (on_btn_browse_exec_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (btn_browse_icon), "clicked",
                      GTK_SIGNAL_FUNC (on_btn_browse_icon_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (btn_ok), "clicked",
                      GTK_SIGNAL_FUNC (on_btn_ok_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (btn_cancel), "clicked",
                      GTK_SIGNAL_FUNC (on_btn_cancel_clicked),
                      NULL);

	gtk_widget_grab_default (btn_ok);
	return lnk_opt;
}

/*
 */
int
lnk_opt_change (lnk_t *lnk)
{
	char *str;
	if (!lnk)
		return 0;

	if (!gOpt)
		gOpt = create_lnk_opt ();

	/* fill or clear
	 */
	if (lnk_get_name(lnk))
		gtk_entry_set_text (GTK_ENTRY(gEn_label), lnk_get_name(lnk));
	else
		gtk_entry_set_text (GTK_ENTRY(gEn_label), "");

	if (lnk_get_comment(lnk))
		gtk_entry_set_text (GTK_ENTRY(gEn_cmnt), lnk_get_comment(lnk));
	else
		gtk_entry_set_text (GTK_ENTRY(gEn_cmnt), "");

	if (lnk->exec)
		gtk_entry_set_text (GTK_ENTRY(gEn_exec), lnk->exec);
	else
		gtk_entry_set_text (GTK_ENTRY(gEn_exec), "");

	if (lnk->icon)
		gtk_entry_set_text (GTK_ENTRY(gEn_icon), lnk->icon);
	else
		gtk_entry_set_text (GTK_ENTRY(gEn_icon), "");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gCb_terminal),lnk->terminal);

	gtk_widget_show (gOpt);
	gtk_main();
	/* set new values
	 */
	if (gState) {
		str = gtk_entry_get_text (GTK_ENTRY(gEn_label));
		lnk_set_name (lnk, str);

		str = gtk_entry_get_text (GTK_ENTRY(gEn_cmnt));
		lnk_set_comment (lnk, str);

		str = gtk_entry_get_text (GTK_ENTRY(gEn_exec));
		lnk_set_exec (lnk, str);

		str = gtk_entry_get_text (GTK_ENTRY(gEn_icon));
		lnk_set_icon (lnk, str);

		lnk->terminal = 	
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gCb_terminal));
	}
	return gState;
}

