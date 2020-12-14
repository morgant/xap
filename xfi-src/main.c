/*
 * main.c
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

#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#ifdef HAVE_LIBGTK
#	include <gtk/gtk.h>
#endif

#ifdef HAVE_GETOPT_H
#	include <getopt.h>
#endif

#include "gui.h"
#include "support.h"
#include "reg.h"

extern GtkWidget *wAbout;
extern GtkWidget *wFilesel;
extern GtkWidget *wDirsel;
extern GtkWidget *wTop;
extern GtkWidget *wPopup;
extern GList *sReg;

#ifndef REG_FILE
#define REG_FILE ".xap/xwf.reg"
#endif
#define XFI_RC "xfi.rc"

/*
 */
int
main (int argc, char *argv[])
{
	int c;
	char *dir = NULL;
	char reg[PATH_MAX+1];
	char *home = getenv ("HOME");
	char user_rc[PATH_MAX+1];

	gtk_set_locale ();
	gtk_init (&argc, &argv);

	sprintf (user_rc, "%s/.xap/%s", home, XFI_RC);
	gtk_rc_parse (DATA_DIR"/xap/"XAP_RC);
	gtk_rc_parse (user_rc);

	sprintf (reg, "%s/%s", getenv ("HOME"), REG_FILE);
	sReg = reg_build_list (reg);

	while ((c = getopt (argc, argv, "")) != EOF) {
	}
	if (argc - optind == 1) {
		dir = argv[optind];
#ifdef HAVE_SETENV
		setenv ("XFI_DIR", dir, 1);
#else
		{ char *str; int len;
			len = sizeof ("XFI_DIR") + 1 + strlen (dir) + 1;
			str = malloc (len);
			if (str) {
				sprintf (str, "XFI_DIR=%s", dir);
				putenv (str);
				free (str);
			}
		}
#endif
	}
	wTop = create_xfi ();
	gtk_widget_set_name (wTop, "xfi");
	gtk_widget_show (wTop);

	wAbout = create_wabout ();
	wFilesel = create_wfilesel();
	wDirsel = create_wdirsel();
	wPopup = create_pmenu();

	gtk_main ();
	return 0;
}

