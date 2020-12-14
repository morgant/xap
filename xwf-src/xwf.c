/*
 * xwf.c
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
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "config.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
extern char *optarg;
extern int optind;
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <X11/Xlib.h>	/* XParseGeometry */
#include "config.h"
#ifdef HAVE_LIBGTK
#include <glib.h>
#include "i18n.h"
#include "xwf_gui.h"
#include "xwf_cfg.h"
#endif
#include "uri.h"

#ifndef VERSION
#define VERSION "(not defined)"
#endif
#define TRASH_DIR ".trash"
#define BASE_DIR ".xap"

int
main (int argc, char *argv[])
{
	int c;
	int verbose = 0;
	int flags = IGNORE_HIDDEN;
	char *path = getenv("HOME"), *geometry = NULL;
	char rc[PATH_MAX+NAME_MAX+1];
	char base[PATH_MAX+1];
	char tmp[PATH_MAX+1];
	char trash[PATH_MAX+1];
	char reg[PATH_MAX+1];
	struct stat st;
	wgeo_t geo = { -1, -1, 400, 480 };

	setlocale (LC_ALL, "");
	bindtextdomain ("xap", LOCALEDIR);
	textdomain ("xap");

	sprintf (rc, "%s/%s/%s", path, BASE_DIR, "xwf.rc");

	gui_init (&argc, &argv, rc);
	while ((c = getopt (argc, argv, "vg:i:")) != EOF) {
		switch (c) {
			case 'v':
				verbose++;
				break;
			case 'g':
				geometry = optarg;
				break;
			case 'i':
				if (atoi(optarg))
					flags |= IGNORE_HIDDEN;
				else
					flags &= ~IGNORE_HIDDEN;
				break;
			default:
				break;
		}
	}
	if (argc != optind) {
		path = argv[argc-1];
	}
	if (strcmp (path, ".") == 0) {
		path = getcwd (NULL, PATH_MAX);
	}
	if (verbose) {
		printf ("Xwf, Version %s\n", VERSION);
		printf ("directory: %s\n", path);
	}
	sprintf (base, "%s/%s", getenv("HOME"), BASE_DIR);
	if (stat(base, &st) == -1) {
		if (verbose) {
			printf ("creating directory: %s\n", base);
		}
		mkdir (base, 0700);
	}
	sprintf (reg, "%s/xwf.reg", base);
	sprintf (trash, "%s/%s", base, TRASH_DIR);
	if (stat (trash, &st) == -1) {
		if (verbose) {
			printf ("creating directory: %s\n", trash);
		}
		mkdir (trash, 0700);
	}
	if (strncmp (path, "..", 2) == 0) {
		sprintf (tmp, "%s/", getcwd(NULL, PATH_MAX));
		strcat (tmp, path);
		path = tmp;
	}
	path = strdup (uri_clear_path(path));
	if (geometry) {
		XParseGeometry (geometry, &geo.x, &geo.y, &geo.width, &geo.height);
		if (verbose) {
			printf ("geometry: %dx%d+%d+%d\n", geo.width, geo.height,
					geo.x, geo.y);
		}
	}
	gui_main (path, base, trash, reg, &geo, flags);
	return 0;
}

