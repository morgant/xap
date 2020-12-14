/*
 * xpg.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "config.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <X11/Xlib.h>
#include "i18n.h"
#ifdef HAVE_LIBGTK
#include "xpg_gui.h"
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
	fprintf (stderr, "Usage: %s [-v] [-g <x-geometry>] [files ..]\n", pname);
	exit (1);
}

/*
 * main program start point
 */
int
main (int argc, char *argv[])
{
	int c, i;
	int verbose = 0, nfiles = 0;
	int page = PG_TEXT_EN;
	int nb_labelpos = NONE;	/* default is top, see below */
	char path[PATH_MAX+1];
	char rc[PATH_MAX+sizeof(XPG_RC)+1];
	char *geometry = NULL;
	wgeo_t geo = { -1, -1, 510, 0 };
	char **files = NULL;

	setlocale (LC_ALL, "");
	bindtextdomain ("xpg", LOCALEDIR);
	textdomain ("xpg");

	sprintf (path, "%s/%s", getenv("HOME"), XAP_PATH);
	sprintf (rc, "%s/%s", path, XPG_RC);
	gui_init (&argc, &argv, rc);

	while ((c = getopt (argc, argv, "g:vn:")) != EOF) {
		switch (c) {
			case 'n':
				nb_labelpos = atoi (optarg);
				break;
			case 'g':
				geometry = optarg;
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

	nfiles = argc - optind;
	if (nfiles) {
		files = malloc (sizeof (char *) * nfiles);
		if (!files)
			return 1;
		for (i = 0; i < nfiles; i++) {
			files[i] = argv[optind+i];
		}
		page = PG_FILE_EN;
	}
	if (verbose) {
		printf ("XPG, Version %s (pid=%d)\n", VERSION, getpid());
		printf ("using rc file: %s\n", rc);
	}
	if (verbose)
		printf ("xpg path: %s\n", path);
	if (geometry) {
		XParseGeometry (geometry, &geo.x, &geo.y, &geo.width, &geo.height);
		if (verbose)
			printf("geometry = %dx%d+%d+%d\n",geo.width,geo.height,geo.x,geo.y);
	}
	gui_main (path, &geo, nb_labelpos, page, files, nfiles);
	return 0;
}

