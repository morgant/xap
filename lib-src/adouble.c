/*
 * adouble.c
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
#include <limits.h>
#include "i18n.h"
#include "adouble.h"

/*
 * create the filename for the appledouble file
 * return value must be free'd with free()
 * /dir/file -> /dir/.AppleDouble/file
 */
char *
ad_file (char *file)
{
	char *afile, *p;
	int len;

	if (!file)
		return NULL;
	if (!*file)
		return NULL;

	len = strlen (file) + sizeof (APPLEDOUBLE) + 3;
	afile = malloc (len);
	if (!afile)
		return NULL;

	p = strrchr (file, G_DIR_SEPARATOR);
	if (!p) {
		sprintf (afile, "%s%c%s", APPLEDOUBLE, G_DIR_SEPARATOR, file);
	} else {
		if (p == file) {
			/* root directory */
			sprintf (afile, "%c%s%c%s", G_DIR_SEPARATOR, APPLEDOUBLE,
				G_DIR_SEPARATOR, file);
		} else {
			len = p - file;
			strncpy (afile, file, len);
			sprintf (afile+len, "%c%s%c%s", G_DIR_SEPARATOR,
				APPLEDOUBLE, G_DIR_SEPARATOR, p+1);
		}
	}
	return afile;
}

/*
 * create the directoryname for the appledouble directory
 * /dir/dir2 -> /dir/.AppleDouble
 * return value must be free()ed
 */
char *
ad_dir (char *file)
{
	char *afile, *p;
	int len;

	if (!file)
		return NULL;
	if (!*file)
		return NULL;

	len = strlen (file) + sizeof (APPLEDOUBLE) + 2;
	afile = malloc (len);
	if (!afile)
		return NULL;

	p = strrchr (file, G_DIR_SEPARATOR);
	if (!p) {
		sprintf (afile, "%s", APPLEDOUBLE);
	} else {
		if (p == file) {
			/* root directory */
			sprintf (afile, "%c%s", G_DIR_SEPARATOR, APPLEDOUBLE);
		} else {
			len = p - file;
			strncpy (afile, file, len);
			sprintf (afile+len, "%c%s", G_DIR_SEPARATOR, APPLEDOUBLE);
		}
	}
	return afile;
}

/*
 * create the filename for the appledouble directory
 *  /usr/bin -> /usr/bin/.AppleDouble
 * returned value must be free'ed with free()
 */
char *
ad_subdir (char *file)
{
	char *afile;
	int len;

	if (!file)
		return NULL;
	if (!*file)
		return NULL;

	len = strlen (file);
	if (file[len-1] != G_DIR_SEPARATOR) {
		len += 1 + sizeof (APPLEDOUBLE) + 1;
		afile = malloc (len);
		sprintf (afile, "%s%c%s", file, G_DIR_SEPARATOR, APPLEDOUBLE);
	} else {
		afile = malloc (len);
		len += sizeof (APPLEDOUBLE) + 1;
		sprintf (afile, "%s%s", file, APPLEDOUBLE);
	}
	return afile;
}

