/*
 * gtk_dnd.c
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
#include "gtk_dnd.h"

static GtkTargetEntry target_table[] = {
	{ "STRING",	0,	TARGET_STRING},
#	define NUM_TARGETS (sizeof(target_table)/sizeof(GtkTargetEntry))
};

/*
 * user drops some data on the entry
 */
static void
dnd_entry_recv_data (GtkWidget *entry, GdkDragContext *context,
			gint x, gint y,
			GtkSelectionData *data, guint info, guint time, void *user_data)
{
	int len;
	char *text;

	if ((data->length < 1) || (data->length > DND_MAX) ||
		(data->format != 8) || (info != TARGET_STRING)) {
		gtk_drag_finish (context, FALSE, TRUE, time);
	}
	/* allocate private memory
	 */
	len = data->length;
	text = g_malloc (len+1);
	if (!text) {
		perror ("out of memory");
		gtk_drag_finish (context, FALSE, TRUE, time);
		return;
	}
	text[len] = '\0'; /* who knows if the data area has a terminating \0 */
	memcpy (text, data->data, len);

	/* check length again, their meight be null bytes..
	 */
	len = strlen (text);

	if (len == 0) {
		gtk_drag_finish (context, FALSE, TRUE, time);
	} else {
		/* len > 0 ..
		 * release DND context with an OK */
		gtk_drag_finish (context, TRUE, TRUE, time);

		/* remove \n and \r at the end
	 	 */
		if ((text[len-1] == '\n') || (text[len-1] == '\r')) {
			text[len-1] = '\0';
		}
		if ((len > 1) && (text[len-2] == '\r')) {
			text[len-2] = '\0';
		}
		gtk_entry_set_text (GTK_ENTRY(entry), text);
	}
	g_free (text);
}

/*
 * register entry widget as DND target
 */
void
dnd_entry_init (GtkWidget *entry, void *data)
{
	gtk_drag_dest_set (entry, GTK_DEST_DEFAULT_ALL,
			target_table, NUM_TARGETS, GDK_ACTION_COPY);
	gtk_signal_connect (GTK_OBJECT(entry), "drag_data_received",
			GTK_SIGNAL_FUNC(dnd_entry_recv_data), data);
}

