/*
 * lnk.h
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

#ifndef __LNK_H__
#define __LNK_H__

#define LNK_LINE_MAX 2048

typedef struct {
	char *self;	/* file name of the desktop entry file */
	char *name;
	char *exec;
	char *icon;
	char *path;		/* use this as the default path if set */
	char *cmnt;		/* default comment (LC_LANG=C) */
	int type;		/* Application, .. */
	int terminal;	/* start in a terminal window? */
	GList *names;	/* for names in other languages */
	GList *cmnts;	/* for comments in other languages */
} lnk_t;

typedef struct {
	unsigned char *key;
	unsigned char *val;
} keyval_t;

#define LT_EXEC	1
#define LT_FILE	2
#define LT_DIR	3
#define LT_LINK	4
#define LT_MAX	5

lnk_t *lnk_read (char *file);
lnk_t *lnk_new (void);
int lnk_write (char *file, lnk_t *lnk);
void lnk_free (lnk_t *lnk);
char *lnk_get_name (lnk_t *lnk);
char *lnk_get_comment (lnk_t *lnk);
void lnk_set_name (lnk_t *lnk, char *name);
void lnk_set_comment (lnk_t *lnk, char *cmnt);
void lnk_set_exec (lnk_t *lnk, char *exec);
void lnk_set_icon (lnk_t *lnk, char *icon);

#endif /* __LNK_H__ */

