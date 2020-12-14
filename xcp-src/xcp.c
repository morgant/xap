/*
 * xcp.c
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
#include <string.h>
#include <sys/stat.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <glib.h>
#include <X11/Xlib.h>
#include "i18n.h"
#ifdef HAVE_LIBGTK
#include "xcp_gui.h"
#else
#error HAVE_LIBGTK not defined
#endif

#ifndef VERSION
#define VERSION "?.?.?"
#endif
#ifndef TRASH_DIR
#define TRASH_DIR ".xap/.trash"
#endif

/*
 * short help how to use it
 */
void
usage (const char *pname)
{
	fprintf (stderr,
		"Usage: %s [-v] [-g <x-geometry>] [-m<c|m|l>] "
		"[-p] <file> [files ..] <target>\n", pname);
	exit (1);
}

/*
 * main - program starting point
 */
int
main (int argc, char *argv[])
{
	int c, i;
#ifdef DEBUG_XCP
	int verbose = 1;
#else
	int verbose = 0;
#endif
	int nfiles = 0;
	char path[PATH_MAX+1];
	char rc[PATH_MAX+sizeof(XCP_RC)+1];
	char *geometry = NULL;
	wgeo_t geo = { -1, -1, 420, 0 };
	char **files = NULL;
	char *target = NULL;
	char trash[PATH_MAX];
	int mode = MODE_COPY, result;
#ifdef ADOUBLE
	int flags = FL_APPLEDOUBLE;
#else
	int flags = FL_NONE;
#endif

	setlocale (LC_ALL, "");
	bindtextdomain ("xcp", LOCALEDIR);
	textdomain ("xcp");

	if (nice (12) != 0)
		printf ("Warning: nice() failed!\n");
	sprintf (path, "%s/%s", getenv("HOME"), XAP_PATH);
	sprintf (rc, "%s/%s", path, XCP_RC);
	gui_init (&argc, &argv, rc);

	while ((c = getopt (argc, argv, "g:vm:pa")) != EOF) {
		switch (c) {
			case 'a':
				flags |= FL_APPLEDOUBLE;
				break;
			case 'g':
				geometry = optarg;
				break;
			case 'v':
				verbose++;
				break;
			case 'm':
				mode = *optarg;
				break;
			case 'p':
				flags |= FL_PRESERVE;
				break;
			default:
				usage (argv[0]);
				break;
		}
	}

	nfiles = argc - optind;
#ifdef DEBUG_XCP
	printf ("main(): behind getopt(): argc=%d, optind=%d\n", argc, optind);
#endif
	if ((nfiles < 2) && (mode != MODE_TRASH)) {
		usage (argv[0]);
	}
	if (mode != MODE_TRASH)
		nfiles--;
	files = malloc (sizeof(char *) * nfiles);
	if (!files)
		return 1;
	for (i = 0; i < nfiles; i++) {
		files[i] = argv[optind+i];
	}
	if (mode == MODE_TRASH) {
		sprintf (trash, "%s%c%s", getenv ("HOME"), G_DIR_SEPARATOR, TRASH_DIR);
		target = trash;
	} else
		target = argv[optind+i];
	if (verbose) {
		printf ("XCP, Version %s (pid=%d)\n", VERSION, getpid());
		printf ("Using rc-file: %s\n", rc);
		printf ("%s .. -> %s\n", files[0], target);
	}
	if (geometry) {
		XParseGeometry (geometry, &geo.x, &geo.y, &geo.width, &geo.height);
		if (verbose)
			printf("geometry = %dx%d+%d+%d\n",geo.width,geo.height,geo.x,geo.y);
	}
	result = gui_main (&geo, mode, files, target, nfiles, flags);
	free (files);
#ifdef DEBUG_XCP
	fprintf (stderr, "return = %d\n", result);
#endif
	return result;
}

