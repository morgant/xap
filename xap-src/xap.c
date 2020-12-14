/*
 * xap.c
 *
 * Copyright (C) 1999 Rasca, Berlin
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
#include <sys/stat.h>
#include "config.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
extern char *optarg;
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <X11/Xlib.h>
#include "i18n.h"
#ifdef HAVE_LIBGTK
#include "xap_gui.h"
#else
#error HAVE_LIBGTK not defined
#endif

#ifndef VERSION
#define VERSION "?.?.?"
#endif

/*
 */
void
usage (const char *pname)
{
	fprintf (stderr, "Usage: %s [-v] [-t] [-g <x-geometry>]\n", pname);
	exit (1);
}

/*
 * main program start point
 */
int
main (int argc, char *argv[])
{
	int c;
	int transient = 0;
	int verbose = 0;
	int nb_labelpos = NONE;	/* default is top, see below */
	int nb_boxlayout = HORIZONTAL;
	char path[PATH_MAX+1];
	char apps[PATH_MAX+1];
	char icon[PATH_MAX+1];
	char rc[PATH_MAX+sizeof(XAP_RC)+1];
	struct stat dir;
	char *geometry = NULL;
	wgeo_t geo = { -1, -1, 0, 0 };

	setlocale (LC_ALL, "");
	bindtextdomain ("xap", LOCALEDIR);
	textdomain ("xap");

	sprintf (path, "%s/%s", getenv("HOME"), XAP_PATH);
	sprintf (rc, "%s/%s", path, XAP_RC);
	gui_init (&argc, &argv, rc);

	while ((c = getopt (argc, argv, "g:tvl:n:")) != EOF) {
		switch (c) {
			case 'l':
				if (*optarg == 'h')
					nb_boxlayout = HORIZONTAL;
				else {
					nb_boxlayout = VERTICAL;
					if (nb_labelpos == NONE)
						nb_labelpos = 4;
				}
				break;
			case 'n':
				nb_labelpos = atoi (optarg);
				break;
			case 'g':
				geometry = optarg;
				break;
			case 't':
				transient = 1;
				break;
			case 'v':
				verbose++;
				break;
			default:
				usage (argv[0]);
				break;
		}
	}

	if (nb_labelpos == NONE)
		nb_labelpos = 2;

	if (verbose) {
		printf ("XAP, Version %s (pid=%d)\n", VERSION, getpid());
		printf ("using rc file: %s\n", rc);
	}
	c = stat (path, &dir);
	if (c == -1) {
		if (verbose)
			printf ("creating directory %s\n", path);
		mkdir (path, 0700);

		sprintf (apps, "%s/apps", path);
		if (verbose)
			printf ("creating directory %s\n", apps);
		mkdir (apps, 0700);
		sprintf (apps, "%s/tools", path);
		if (verbose)
			printf ("creating directory %s\n", apps);
		mkdir (apps, 0700);

		sprintf (apps, "%s/apps/xwf", path);
		symlink (INSTDIR"/bin/xwf", apps);
		sprintf (apps, "%s/apps/xpg", path);
		symlink (PLUGINDIR"/xpg", apps);
		sprintf (apps, "%s/apps/xfi", path);
		symlink (PLUGINDIR"/xfi", apps);
		sprintf (apps, "%s/apps/xat", path);
		symlink (PLUGINDIR"/xat", apps);

		/* check for some common applications
		 */
#ifdef MOZILLA
		sprintf (apps, "%s/apps/netscape", path);
		symlink (MOZILLA, apps);
#endif
#ifdef RXVT
		sprintf (apps, "%s/apps/rxvt", path);
		symlink (RXVT, apps);
#endif
#ifdef GVIM
		sprintf (apps, "%s/apps/gvim", path);
		symlink (GVIM, apps);
#endif
#ifdef SOFFICE
		sprintf (apps, "%s/apps/soffice", path);
		symlink (SOFFICE, apps);
#endif
	}
	sprintf (icon, "%s/.icons", path);
	if (stat (icon, &dir) == -1) {
		/* create icon directory */
		if (verbose) {
			printf ("creating directory %s\n", icon);
		}
		mkdir (icon, 0700);
	}
	if (verbose)
		printf ("xap path: %s\n", path);
	if (geometry) {
		XParseGeometry (geometry, &geo.x, &geo.y, &geo.width, &geo.height);
		if (verbose)
			printf("geometry = %dx%d+%d+%d\n",geo.width,geo.height,geo.x,geo.y);
	}
	gui_main (path, transient, &geo, nb_labelpos, nb_boxlayout);
	return 0;
}

