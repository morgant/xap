/*
 * gtk_dlg.c
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
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "i18n.h"
#include "gtk_dlg.h"
#include "gtk_dnd.h"
#include "icons/warning.xpm"
#include "icons/info.xpm"
#include "icons/question.xpm"
#include "icons/error.xpm"

#define B_WIDTH		80
#define B_HEIGHT	35
#define E_WIDTH		260

typedef struct {
	GtkWidget *top;
	GtkWidget *entry;
	void *data;
	int result;
	int type;
} dlg;

static dlg dl;

/*
 * called if user presses cancel button or ESC
 */
static void
on_cancel (GtkWidget *btn, gpointer *data)
{
	if ((int)data != DLG_RC_DESTROY) {
		gtk_widget_destroy (dl.top);
	}
	dl.result = DLG_RC_CANCEL;
	gtk_main_quit();
}

/*
 * called if user presses ok button
 */
static void
on_ok (GtkWidget *ok, gpointer *data)
{
#ifdef DEBUG
	printf ("on_ok()\n");
#endif
	if (dl.entry) {
		if (dl.data)
		sprintf (dl.data, "%s", gtk_entry_get_text(GTK_ENTRY(dl.entry)));
	}
	gtk_widget_destroy (dl.top);

	dl.result = (int)data;
	gtk_main_quit();
}

/*
 * call on_ok if user presses RETURN in entry widget
 */
static gint
on_key_press (GtkWidget *entry, GdkEventKey *event, gpointer cancel)
{
	if (event->keyval == GDK_Escape) {
		on_cancel ((GtkWidget *) cancel, (gpointer *)DLG_RC_CANCEL);
		return (TRUE);
	}
	return (FALSE);
}

/*
 * create a modal dialog and handle it
 */
gint
dlg_new (char *labelval, char *defval, void *data, int type)
{
	GtkWidget	*ok = NULL,
				*cancel = NULL,
				*all = NULL,
				*skip = NULL,
				*close = NULL,
				*icon = NULL,
				*combo= NULL,
				*label, *box;
	char title[DLG_MAX];
	char *longlabel = NULL;
	GdkPixmap *pix = NULL;
	GdkBitmap *pim = NULL;
	GdkColor transparent;
	GdkColormap *cmap;

	dl.result = 0;
	dl.type = type;
	dl.entry = NULL;
	dl.data = defval;

	dl.top = gtk_dialog_new ();
	gtk_window_position (GTK_WINDOW(dl.top), GTK_WIN_POS_MOUSE);
	gtk_window_set_modal (GTK_WINDOW(dl.top), TRUE);
	box = gtk_hbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER(box), 5);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dl.top)->vbox), box, TRUE, TRUE, 0);
	gtk_widget_realize (dl.top);
	cmap = gtk_widget_get_colormap (dl.top);

	/* what kind of pixmap do we want to use..?
	 */
	if ((type & DLG_QUESTION) || (type & DLG_CONTINUE)) {
		pix = gdk_pixmap_colormap_create_from_xpm_d (NULL, cmap, &pim,
						&transparent, question_xpm);
		sprintf (title, _("Question"));
	} else if (type & DLG_INFO) {
		pix = gdk_pixmap_colormap_create_from_xpm_d (NULL, cmap, &pim,
						&transparent, info_xpm);
		sprintf (title, _("Information"));
	} else if (type & DLG_ERROR) {
		pix = gdk_pixmap_colormap_create_from_xpm_d (NULL, cmap, &pim,
						&transparent, error_xpm);
		sprintf (title, _("Error"));
	} else if (type & DLG_WARN) {
		pix = gdk_pixmap_colormap_create_from_xpm_d (NULL, cmap, &pim,
						&transparent, warning_xpm);
		sprintf (title, _("Warning"));
	} else {
		sprintf (title, _("Dialog"));
	}
	if (pix) {
		icon = gtk_pixmap_new (pix, pim);
		gdk_pixmap_unref (pix);
		gdk_pixmap_unref (pim);
		gtk_box_pack_start (GTK_BOX(box), icon, FALSE, FALSE, 0);
	}
	gtk_window_set_title (GTK_WINDOW(dl.top), title);

	/* create the requested buttons..
 	 */
	if (type & (DLG_OK|DLG_YES|DLG_CONTINUE)) {
		if (type & DLG_OK) {
			ok = gtk_button_new_with_label (_("Ok"));
		} else if (type & DLG_YES) {
			ok = gtk_button_new_with_label (_("Yes"));
		} else {
			ok = gtk_button_new_with_label (_("Continue"));
		}
		GTK_WIDGET_SET_FLAGS (ok, GTK_CAN_DEFAULT);
		gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dl.top)->action_area),
				ok, TRUE, FALSE, 0);
		gtk_signal_connect (GTK_OBJECT(ok), "clicked",
				GTK_SIGNAL_FUNC(on_ok), (void *)DLG_RC_OK);
		gtk_widget_set_usize (ok, B_WIDTH, B_HEIGHT);
		gtk_signal_connect (GTK_OBJECT(ok), "key_press_event",
				GTK_SIGNAL_FUNC(on_key_press), (void *)cancel);
		gtk_widget_grab_default (ok);
	}
	if (type & DLG_SKIP) {
		skip = gtk_button_new_with_label (_("Skip"));
		GTK_WIDGET_SET_FLAGS (skip, GTK_CAN_DEFAULT);
		gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dl.top)->action_area),
				skip, TRUE, FALSE, 0);
		gtk_signal_connect (GTK_OBJECT(skip), "clicked",
				GTK_SIGNAL_FUNC(on_ok), (void *)DLG_RC_SKIP);
		gtk_widget_set_usize (skip, B_WIDTH, B_HEIGHT);
	}
	if (type & (DLG_CANCEL|DLG_NO)) {
		if (type & DLG_CANCEL) {
			cancel = gtk_button_new_with_label (_("Cancel"));
		} else
			cancel = gtk_button_new_with_label (_("No"));
		GTK_WIDGET_SET_FLAGS (cancel, GTK_CAN_DEFAULT);
		gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dl.top)->action_area),
				cancel, TRUE, FALSE, 0);
		gtk_signal_connect (GTK_OBJECT(cancel), "clicked",
				GTK_SIGNAL_FUNC(on_cancel), (void *)DLG_RC_CANCEL);
		gtk_widget_set_usize (cancel, B_WIDTH, B_HEIGHT);
	}
	if (type & DLG_ALL) {
		all = gtk_button_new_with_label (_("All"));
		GTK_WIDGET_SET_FLAGS (all, GTK_CAN_DEFAULT);
		gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dl.top)->action_area),
				all, TRUE, FALSE, 0);
		gtk_signal_connect (GTK_OBJECT(all), "clicked",
				GTK_SIGNAL_FUNC(on_ok), (void *)DLG_RC_ALL);
		gtk_widget_set_usize (all, B_WIDTH, B_HEIGHT);
	}
	if (type & DLG_CLOSE) {
		close = gtk_button_new_with_label (_("Close"));
		GTK_WIDGET_SET_FLAGS (close, GTK_CAN_DEFAULT);
		gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dl.top)->action_area),
				close, TRUE, FALSE, 0);
		gtk_signal_connect (GTK_OBJECT(close), "clicked",
				GTK_SIGNAL_FUNC(on_ok), (void *)DLG_RC_OK);
		gtk_widget_grab_default (close);
	}

	if (type & DLG_ENTRY_VIEW) {
			dl.entry = gtk_entry_new_with_max_length (DLG_MAX);
			gtk_widget_set_usize (dl.entry, E_WIDTH, -1);
			gtk_entry_set_editable (GTK_ENTRY(dl.entry), FALSE);
			gtk_widget_set_sensitive (dl.entry, FALSE);
	} else if ((type & DLG_ENTRY_EDIT) || (type & DLG_ENTRY_PW)) {
			dl.entry = gtk_entry_new_with_max_length (DLG_MAX);
			gtk_widget_set_usize (dl.entry, E_WIDTH, -1);
			GTK_WIDGET_SET_FLAGS (dl.entry, GTK_CAN_DEFAULT);
			dnd_entry_init (dl.entry, NULL);
			if (type & DLG_ENTRY_PW)
				gtk_entry_set_visibility (GTK_ENTRY(dl.entry), FALSE);
	} else if (type & DLG_COMBO) {
			combo = gtk_combo_new ();
			dl.entry = GTK_COMBO(combo)->entry;
			dnd_entry_init (dl.entry, NULL);
	} else {
		if (defval) {
			longlabel = g_malloc (strlen (labelval)+strlen(defval) + 5);
			sprintf (longlabel, "%s: %s", labelval, (char *)defval);
			labelval = longlabel;
		}
	}

	label = gtk_label_new (labelval);
	gtk_box_pack_start (GTK_BOX(box), label, FALSE, FALSE, 0);

	if (dl.entry) {
		if (type & DLG_COMBO) {
			if (data) {
				gtk_combo_set_popdown_strings (GTK_COMBO(combo), (GList*)data);
			}
			gtk_box_pack_start (GTK_BOX(box), combo, TRUE, TRUE, 0);
			gtk_signal_connect (GTK_OBJECT(dl.entry), "key_press_event",
				GTK_SIGNAL_FUNC(on_key_press), (void *)cancel);
		} else {
			gtk_box_pack_start (GTK_BOX(box), dl.entry, TRUE, TRUE, 0);
			gtk_signal_connect_object (GTK_OBJECT(dl.entry), "activate",
				GTK_SIGNAL_FUNC(gtk_button_clicked), GTK_OBJECT(ok));
			gtk_signal_connect (GTK_OBJECT(dl.entry), "key_press_event",
				GTK_SIGNAL_FUNC(on_key_press), (void *)cancel);
			if (type & DLG_ENTRY_EDIT) {
				gtk_widget_grab_focus (dl.entry);
			}
		}
		if (defval)
			gtk_entry_set_text (GTK_ENTRY(dl.entry), defval);
		if (type & DLG_ENTRY_EDIT)
			gtk_entry_select_region (GTK_ENTRY(dl.entry), 0, -1);
	}
	gtk_signal_connect (GTK_OBJECT(dl.top), "destroy",
			GTK_SIGNAL_FUNC(on_cancel), (void*)DLG_RC_DESTROY);
	gtk_signal_connect (GTK_OBJECT(dl.top), "key_press_event",
				GTK_SIGNAL_FUNC(on_key_press), (void *)cancel);
	gtk_widget_show_all (dl.top);
	gtk_main ();
	if (longlabel)
		g_free (longlabel);
	return (dl.result);
}

