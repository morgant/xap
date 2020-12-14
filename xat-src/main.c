/*
 * main.c
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

#include <gtk/gtk.h>
#ifdef HAVE_GETOPT_H
#	include <getopt.h>
#endif
#include <stdio.h>	/* stderr */
#include <unistd.h>	/* exit() */
#include <stdlib.h>	/* free() */
#include "gui.h"
#include "support.h"
#include "entry.h"
#include "mailcap.h"

#define XAT_RC "xat.rc"
/* globals
 */
GList *gList = NULL;
mc_mime_reg_t *gMreg = NULL;

/* prototypes
 */
void cb_fill_wmain (GtkWidget *, void *);
void cb_fill_combo (GtkWidget *);

/*
 */
void
usage (char *pname)
{
	fprintf (stderr, "Usage: %s file [more files]\n", pname);
	exit (1);
}

/*
 */
int
main (int argc, char *argv[])
{
	GtkWidget *wmain;
	int c, nfiles, i;
	GList *list = NULL;
	entry_t *entry;
	uid_t uid;
	char *home = getenv ("HOME");
	char user_rc[PATH_MAX+1];

	gtk_set_locale ();
	gtk_init (&argc, &argv);

	sprintf (user_rc, "%s/.xap/%s", home, XAT_RC);
	gtk_rc_parse (DATA_DIR"/xap/"XAP_RC);
	/* gtk_rc_parse (user_rc); */

	while ((c = getopt (argc, argv, "")) != EOF) {
		switch (c) {
			default:
				usage (argv[0]);
				break;
		}
	}

	/*
	 * create the main window
	 */
	wmain = create_xat ();
	gtk_widget_set_name (wmain, "xat");
	gtk_widget_show (wmain);

	nfiles = argc - optind;
	if (!nfiles)
		usage (argv[0]);
	for (i = 0; i < nfiles; i++) {
		entry = entry_new_by_path (argv[optind+i]);
		if (entry) {
			list = g_list_append(list, entry);
		}
	}
	if (!list)
		usage (argv[0]);
	/* we need the list in callbacks.c
	 */
	gList = list;
	gMreg = mc_build();

	/* first fill the combo boxes for user/group
	 */
	cb_fill_combo (wmain);

	/* now the information for the first file
	 */
	cb_fill_wmain (wmain, list->data);

	uid = getuid();
	if (uid != 0)
		gtk_widget_set_sensitive (
			(GtkWidget *)lookup_widget(wmain, "cb_changeowner"), 0);

	gtk_main ();
	return 0;
}

