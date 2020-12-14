/*
 * entry.h
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

#ifndef __ENTRY_H__
#define __ENTRY_H__

#include <sys/types.h>

typedef struct {
	short year;
	short month;
	short day;
	short hour;
	short min;
	short sec;
} mdate_t;

typedef struct {
	gchar *path;
	gchar *label;
	int type;
	int flags;
	size_t size;
	time_t atime;
	time_t mtime;
	ino_t inode;
	uid_t uid;
	gid_t gid;
	dev_t device;
	mode_t mode;
	mdate_t date;
} entry_t;


#define FT_LINK			(1<<0)
#define FT_DIR			(1<<1)
#define FT_FILE			(1<<2)
#define FT_CHAR_DEV		(1<<3)
#define FT_BLOCK_DEV	(1<<4)
#define FT_FIFO			(1<<5)
#define FT_SOCKET		(1<<6)
#define FT_EXE			(1<<7)
#define FT_HIDDEN		(1<<8)
#define FT_DIR_UP		(1<<9)
#define FT_DIR_PD		(1<<10)
#define FT_STALE_LINK	(1<<11)
#define FT_WRITE		(1<<12)	/* not a field type .. nevertheless .. */
#define FT_UNKNOWN		(1<<13)
#define FT_DUMMY		(1<<14)
#define FT_SORT_MASK (FT_DIR|FT_FILE|FT_CHAR_DEV|FT_BLOCK_DEV|FT_FIFO|FT_SOCKET)

#define EN_IS_DIR(en) (en->type & FT_DIR)
#define EN_IS_FILE(en) (en->type & FT_FILE)
#define EN_IS_DIR_UP(en) (en->type & FT_DIR_UP)
#define EN_IS_LINK(en) (en->type & FT_LINK)
#define EN_IS_DIRUP(en) (en->type & FT_DIR_UP)
#define EN_IS_DUMMY(en) (en->type & FT_DUMMY)
#define EN_IS_FIFO(en) (en->type & FT_FIFO)
#define EN_IS_SOCKET(en) (en->type & FT_SOCKET)
#define EN_IS_EXE(en) (en->type & FT_EXE)
#define EN_IS_DEVICE(en) ((en->type & FT_CHAR_DEV)||(en->type & FT_BLOCK_DEV))
#ifndef ERROR
#define ERROR -1
#endif

void entry_free (entry_t *);
entry_t *entry_new (void);
entry_t *entry_dupe (entry_t *);
entry_t *entry_new_by_path (char *path);
entry_t *entry_new_by_path_and_label (char *path, char *label);
entry_t *entry_new_by_type (char *path, int type);
int entry_update (entry_t *);
char *entry_get_parent (entry_t *);
#endif
