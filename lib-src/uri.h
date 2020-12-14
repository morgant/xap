/*
 * uri.h
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

#ifndef __URI_H__
#define __URI_H__

#define URI_MAX 1024

enum {
	URI_LOCAL,
	URI_FILE,
	URI_HTTP,
	URI_HTTPS,
	URI_FTP,
	URI_SMB,
} UriType;

typedef struct {
	char *url;
	short len;
	short type;
} uri_t;

char *uri_clear_path (const char *);
char *uri_to_quoted_list (GList *);
int uri_parse_list (const char *, GList **);
int uri_remove_file_prefix(char *, int);
int uri_remove_file_prefix_from_list(GList *);
void uri_free_list (GList *);
const char *uri_basename (const char *);
const char *uri_hostname (void);
#endif

