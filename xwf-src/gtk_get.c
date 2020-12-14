/*
 * gtk_get.c
 *
 * Copyright (C) 1998 Rasca, Berlin
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
#include <errno.h>
#include <time.h>
#include <gtk/gtk.h>
#include "uri.h"
#include "io.h"
#include "gtk_get.h"
#include "gtk_dlg.h"

#ifndef URLFETCH
#define URLFETCH "curl"
#endif

#define X_OPT GTK_FILL|GTK_EXPAND|GTK_SHRINK

#define ST_CANCEL	2
#define ST_OK		1

static int status;

/*
 */
static void
cb_cancel (GtkWidget *w, void *data)
{
#ifdef DEBUG
	printf ("%s: cb_cancel() data=0x%X\n", __FILE__, (int)data);
#endif
	if (data) {
		gtk_widget_destroy ((GtkWidget *)data);
	}
	status = ST_CANCEL;
}

/*
 */
int
download (uri_t *u, char *path)
{
#define BUFLEN 8192
	GtkWidget *dialog, *cancel, *label[5], *box, *table;
	char target[PATH_MAX+1];
	char cmd[URI_MAX+(PATH_MAX*2)+1];
	char buf[BUFLEN], received[32];
	int num, len, buflen, bps;
	FILE *fp, *pipe;
	time_t start_time, elapsed;

	status = ST_OK;
	if (io_is_directory (path)) {
		sprintf (target, "%s/%s", path, uri_basename(u->url));
		if (io_is_file (target)) {
			if (dlg_question("Override file?", target) != DLG_RC_OK) {
				return 0;
			}
		}
	} else {
		sprintf (target, "%s", path);
	}

	sprintf (cmd, "%s %s", URLFETCH, u->url);
#ifdef DEBUG
	printf ("%s: download() cmd='%s' target='%s'\n", __FILE__, cmd, target);
#endif
	fp = fopen (target, "wb");
	if (!fp) {
		dlg_error (target, strerror(errno));
		return 0;
	}
	pipe = popen (cmd, "r");
	if (!pipe) {
		dlg_error (target, strerror(errno));
		fclose (fp);
		return 0;
	}

	dialog = gtk_dialog_new ();
	gtk_signal_connect (GTK_OBJECT(dialog), "destroy",
			GTK_SIGNAL_FUNC(cb_cancel), NULL);

	box = gtk_vbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), box, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER(box), 5);

	table = gtk_table_new (2, 2, FALSE);
	gtk_box_pack_start (GTK_BOX(box), table ,TRUE, TRUE,0);

	label[0] = gtk_label_new ("Source: ");
	gtk_table_attach(GTK_TABLE(table), label[0], 0, 1, 0, 1, X_OPT,0,0,0);
	gtk_label_set_justify (GTK_LABEL(label[0]), GTK_JUSTIFY_RIGHT);
	label[1] = gtk_label_new (u->url);
	gtk_table_attach(GTK_TABLE(table), label[1], 1, 2, 0, 1, X_OPT,0,0,0);
	gtk_label_set_justify (GTK_LABEL(label[1]), GTK_JUSTIFY_LEFT);

	label[2] = gtk_label_new ("Target: ");
	gtk_table_attach(GTK_TABLE(table), label[2], 0, 1, 1, 2, X_OPT,0,0,0);
	gtk_label_set_justify (GTK_LABEL(label[2]), GTK_JUSTIFY_RIGHT);
	label[3] = gtk_label_new (target);
	gtk_table_attach(GTK_TABLE(table), label[3], 1, 2, 1, 2, X_OPT,0,0,0);
	gtk_label_set_justify (GTK_LABEL(label[3]), GTK_JUSTIFY_LEFT);

	label[4] = gtk_label_new ("0 bytes received");
	gtk_box_pack_start (GTK_BOX(box), label[4] ,TRUE, TRUE,0);

	cancel = gtk_button_new_with_label ("Cancel");
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->action_area),
			cancel,TRUE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
			GTK_SIGNAL_FUNC(cb_cancel), dialog);

	gtk_window_set_title (GTK_WINDOW(dialog), "Xwf: Download");
	gtk_widget_show_all (dialog);

	while (gtk_events_pending())
		gtk_main_iteration();

	/* read the data from the pipe */
	len = 0;
	start_time = time(NULL);
	buflen = 16;
	while ((num = fread (buf, 1, buflen, pipe)) > 0) {
		fwrite (buf, 1, num, fp);
		len += num;
		while (gtk_events_pending())
			gtk_main_iteration();
		if (status == ST_CANCEL) {
			fclose (pipe);
			fclose (fp);
			return (0);
		}
		elapsed = time(NULL) - start_time;
		bps = len/(elapsed > 0 ? elapsed : 1);
		buflen = BUFLEN < bps ? BUFLEN : bps;
		buflen = buflen < 32 ? 32 : buflen;
		sprintf (received, "%d bytes received (%d bps)", len, bps);
		gtk_label_set_text (GTK_LABEL(label[4]), received);
	}
	fclose (fp);
	fclose (pipe);

	gtk_widget_destroy (dialog);
	return (ST_OK);
}

