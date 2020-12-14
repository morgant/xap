/*
 * xap_fs.c
 *
 * Copyright (C) 2000 Rasca, Berlin
 * EMail: thron at gmx.de
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

#include <stdlib.h>		/* free() */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "xap_fs.h"
#include "gtk_dnd.h"


static GtkWidget *gFilesel = NULL;
static int gStatus = 1;

/*
 */
static void
on_fileselection_show                  (GtkWidget       *widget,
                                        gpointer         user_data)
{
	/* not used until now */
}

/*
 */
static void
on_btn_ok_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_hide (gFilesel);
	gtk_main_quit();
}

/*
 */
static void
on_btn_cancel_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	gStatus = 0;
	gtk_widget_hide (gFilesel);
	gtk_main_quit();
}


/*
 */
gboolean
on_fileselection_delete_event          (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	gStatus = 0;
	gtk_widget_hide (gFilesel);
	gtk_main_quit();
	return TRUE;
}

/*
 */
static GtkWidget *
create_fileselection (void)
{
	GtkWidget *fileselection;
	GtkWidget *ok_btn;
	GtkWidget *cancel_btn;

	fileselection = gtk_file_selection_new ("Select File");
	gtk_container_set_border_width (GTK_CONTAINER (fileselection), 8);
	gtk_window_set_modal (GTK_WINDOW (fileselection), TRUE);
	gtk_window_set_position (GTK_WINDOW(fileselection), GTK_WIN_POS_MOUSE);

	ok_btn = GTK_FILE_SELECTION (fileselection)->ok_button;
	gtk_widget_show (ok_btn);
	GTK_WIDGET_SET_FLAGS (ok_btn, GTK_CAN_DEFAULT);

	cancel_btn = GTK_FILE_SELECTION (fileselection)->cancel_button;
	gtk_widget_show (cancel_btn);
	GTK_WIDGET_SET_FLAGS (cancel_btn, GTK_CAN_DEFAULT);

	gtk_signal_connect (GTK_OBJECT (fileselection), "show",
                      GTK_SIGNAL_FUNC (on_fileselection_show),
                      NULL);
	gtk_signal_connect (GTK_OBJECT (fileselection), "delete_event",
                      GTK_SIGNAL_FUNC (on_fileselection_delete_event),
                      NULL);
	gtk_signal_connect (GTK_OBJECT (ok_btn), "clicked",
                      GTK_SIGNAL_FUNC (on_btn_ok_clicked),
                      NULL);
	gtk_signal_connect (GTK_OBJECT (cancel_btn), "clicked",
                      GTK_SIGNAL_FUNC (on_btn_cancel_clicked),
                      NULL);
	dnd_entry_init(GTK_FILE_SELECTION(fileselection)->selection_entry, NULL);

	return fileselection;
}

/*
 * return value must be g_free()ed
 */
char *
fs_get_file (char *dir)
{
	char *file = NULL;
	char *olddir = NULL;

	if (dir) {
		olddir = getcwd (NULL, -1);
		chdir (dir);
		if (gFilesel)
			gtk_widget_destroy (gFilesel);
		gFilesel = NULL;
	}

	if (!gFilesel)
		gFilesel = create_fileselection();
	gtk_widget_show (gFilesel);
	gtk_main();
	if (gStatus) {
		/* read the file name */
		file = gtk_file_selection_get_filename (GTK_FILE_SELECTION(gFilesel));
	}
	if (olddir) {
		chdir(olddir);
		free (olddir);
	}
	return g_strdup (file);
}

