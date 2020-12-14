/*
 * history.c
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
#include <unistd.h>
#include <utime.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <glib.h>
#include "history.h"
#include "uri.h"

/*
 */
GList *
history_init (char *path)
{
	GList *list = NULL;
	char *s, *p;
	char file[PATH_MAX+1];
#define LINESIZE PATH_MAX
	char line[LINESIZE];
	FILE *fp;
	int len;

	sprintf (file, "%s/.xap/xwf.his", getenv ("HOME"));
	fp = fopen (file, "r");
	if (fp) {
		while (fgets (line, LINESIZE, fp) != NULL) {
			len = strlen (line);
			if (len) {
				line[len-1] = '\0';
				list = g_list_append (list, g_strdup (line));
			}
		}
		fclose (fp);
	} else {
		if (!path)
			return NULL;
		s = p = g_strdup (path);
		list = g_list_append (list, g_strdup (s));

		while (( p = strrchr (s, G_DIR_SEPARATOR)) != NULL) {
			if (p == s) {
				*(p+1) = '\0';
				list = g_list_append (list, g_strdup (s));
				break;
			}
			*p = '\0';
			list = g_list_append (list, g_strdup (s));
		}
		g_free (s);
	}
	return list;
}

/*
 */
GList *
history_add (GList *list, char *path)
{
	GList *lptr, *lptr2, *ltmp;
	int num;

	if (!path) {
		return list;
	}
	if (!*path) {
		return list;
	}
	list = g_list_prepend (list, g_strdup(uri_clear_path(path)));

	/* remove dupes from the list
	 */
	lptr = list;
	num = 0;
	while (lptr) {
		lptr2 = lptr->next;
		while (lptr2) {
			if (strcmp (lptr->data, lptr2->data) == 0) {
				ltmp = lptr2;
				lptr2 = lptr2->next;
				g_free (ltmp->data);
				g_list_remove_link (list, ltmp);
				g_list_free_1 (ltmp);
			} else {
				lptr2 = lptr2->next;
			}
		}
		lptr = lptr->next;
		num++;
	}
	/* check the length
	 */
	if (num > HISTORY_MAX) {
		/* remove last entry */
		ltmp = g_list_last (list);
		g_free (ltmp->data);
		g_list_remove_link (list, ltmp);
		g_list_free_1 (ltmp);
	}
	return list;
}

/*
 */
int
history_save (GList *list)
{
	char path[PATH_MAX+1];
	FILE *fp;

	sprintf (path, "%s/.xap/xwf.his", getenv ("HOME"));
	fp = fopen (path, "w");
	if (!fp)
		return 0;
	while (list) {
		fprintf (fp, "%s\n", (char *)list->data);
		list = list->next;
	}
	fclose (fp);
	return 1;
}

