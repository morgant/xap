/*
 * uri.c
 *
 * Copyright (C) 1998 - 2000 Rasca, Berlin
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
#include <unistd.h>		/* gethostname() */
#include <string.h>
#include <limits.h>
#include <glib.h>
#include "config.h"
#include "uri.h"

/*
 * remove "file:///", "file://host/" and "file:/"
 */
int
uri_remove_file_prefix (char *path, int len)
{
	char *p;
	int striped = 0;
#ifdef DEBUG_URI
	printf ("uri_remove_file_prefix (%s, %d)\n", path, len);
#endif

	if (!path)
		return (0);

	if ((len > 7) && (strncasecmp (path, "file:///", 8) == 0)) {
		striped = 7;
		len = len - striped;
		memmove (path, path + striped, len);
		path[len] = '\0';
	} else if ((len > 6) && ( strncasecmp (path, "file://", 7) == 0)) {
		p = strchr (path+7, '/');
		if (p) {
			/* remove hostname as well
			 */
			striped = p - path;
			len = len - striped;
			memmove (path, path+striped, len);
			path[len] = '\0';
		}
		return (striped);
	} else if ((len > 5) && ( strncasecmp (path, "file:/", 6) == 0)) {
		striped = 5;
		len = len - striped;
		memmove (path, path + striped, len);
		path[len] = '\0';
	}
	return (striped);
}

/*
 * go through the list and remove the file:// prefix
 * from every element.
 */
int
uri_remove_file_prefix_from_list (GList *list)
{
	int rc = 0;
	uri_t *u;

	while (list) {
		u = (uri_t *)list->data;
		if (u->type == URI_FILE) {
			rc = uri_remove_file_prefix (u->url, u->len);
			u->len -= rc;
			u->type = URI_LOCAL;
		}
		list = list->next;
	}
	return (rc);
}

/*
 * remove ".." and trailing "/" and "/." from a path
 */
char *
uri_clear_path (const char *org_path)
{
	static char path[PATH_MAX+1];
	char *p, *ld;
    int len;

    if (!path) {
        return (NULL);
    }
	strcpy (path, org_path);
    /* remove ".."
     */
    p = path+1;
    ld= path;
    while (*p) {
        if (*p == '/') {
            if (*(p+1) != '\0') {
                if (!((*(p+1) == '.') && (*(p+2) == '.'))) {
                    ld = p;
                }
            } else {
                break;
            }
        } else {
            if ((*(p-1) == '/') && (*p == '.') && (*(p+1) == '.')) {
                len = strlen (p+2);
                if (!len) {
                    *(ld+1) = '\0';
                } else {
                    memmove (ld, p+2, len+1);
                }
                ld = p = path;
            }
        }
        p++;
    }
    /* remove trailing '/' and '/.'
     */
    while (1) {
        len = strlen (path);
        if (len > 1) {
            if (path[len-1] == '/')
                path[len-1] = '\0';
            else if ((path[len-2] == '/') && (path[len-1] == '.')) {
                if (len == 2)
                    path[len-1] = '\0';
                else
                    path[len-2] = '\0';
            } else
                break;
        } else {
            break;
        }
    }
	return (path);
}

/*
 * return uri type: file, http or ftp
 */
int
uri_type (char *s)
{
	if (*s == '/')
		return (URI_LOCAL);
	if (strncmp (s, "file:/", 6) == 0)
		return (URI_FILE);
	if (strncmp (s, "http:/", 6) == 0)
		return (URI_HTTP);
	if (strncmp (s, "https:/",7) == 0)
		return (URI_HTTPS);
	if (strncmp (s, "ftp:/",  5) == 0)
		return (URI_FTP);
	if (strncmp (s, "smb:/",  5) == 0)
		return (URI_SMB);
	return (URI_LOCAL);
}

/*
 */
int
uri_parse_list (const char *text, GList **list)
{
	int num, org_len, len, tlen, i, no;
	const char *p, *end;
	uri_t *u;

	if (!text)
		return 0;
#ifdef DEBUG
	printf ("uri_parse_list() text=%s\n", text);
#endif
	*list = NULL;
	org_len = strlen (text);
	p = text;
	num = 0;
	while ((p = strchr (p, '\n')) != NULL) {
		p++;
		num++;
	}
	if ((!num) || (text[org_len-1] != '\n'))
		num++;
#ifdef DEBUG
	printf (" num=%d\n", num);
#endif
	p = text;
	no = num;
	for (i = 0; i < num; i++) {
		tlen = 2;	/* terminator length */
		end = strchr (p, '\r');
		if (!end) {
			end = strchr (p, '\n');
			tlen = 1;
		}
		if (end) end--;
		else end = text+org_len-1;
		len = end - p + 1;
#ifdef DEBUG2
		printf (" len=%d\n", len);
#endif
		if ((len > 0) && (*p != '#')) {
			u = g_malloc (sizeof(uri_t));
			if (!u)
				return (0);
			u->url = g_malloc (len+1);
			strncpy (u->url, p, len);
			u->url[len] = '\0';
			u->len = len;
			u->type= uri_type (u->url);
			if (u->len > URI_MAX) {
				u->len = URI_MAX;
				u->url[URI_MAX] = '\0';
			}
			*list = g_list_append (*list, u);
		} else {
			no--;
		}
		p = p+len+tlen;
		
	}
	return (no);
}

/*
 */
void
uri_free_list (GList *list)
{
	GList *t = list;
	while (t) {
		g_free(((uri_t *)t->data)->url);
		g_free ((uri_t *)(t->data));
		t = t->next;
	}
	g_list_free (list);
}

/*
 */
char *
uri_to_quoted_list (GList *list)
{
	int nitems =0;
	int len =0;
	char *p, *string, quote;
	GList *t = list;
	uri_t *u;

	/* count items and sum the lenght */
	while (t) {
		len += ((uri_t *)t->data)->len;
		nitems++;
		t = t->next;
	}
	string = p = g_malloc (len + (nitems * 3) + 1);
	p[len+(nitems*3)] = '\0';
	while (list) {
		u = (uri_t *)list->data;
		list = list->next;
		if (!u)
			continue;
		if (strchr (u->url, '\''))
			quote = '"';
		else
			quote = '\'';
		*p++ = quote;
		memcpy (p, u->url, u->len);
		p += u->len;
		*p++ = quote;
		*p++ = ' ';
	}
#ifdef DEBUG
	printf ("uri_to_quoted_list() len=%d string='%s'\n", len, string);
#endif
	return (string);
}

/*
 */
const char *
uri_basename (const char *path)
{
	char *p;

	if (!path)
		return (NULL);
	p = strrchr (path, '/');
	if (p) {
		p++;
		if (*p != '\0')
			return (p);
	}
	return (NULL);
}

/*
 * return hostname
 */
const char *
uri_hostname (void)
{
	static char host[64];

	host[63] = '\0';
	gethostname (host, 63);
	return host;
}

