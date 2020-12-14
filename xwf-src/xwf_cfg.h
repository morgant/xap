/*
 * xwf_cfg.h
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

#ifndef __XWF_CFG_H__
#define __XWF_CFG_H__

#define IGNORE_HIDDEN 1

typedef struct {
	void *compare;
	char *trash;	/* xwf trash dir */
	char *xap;		/* xap home-dir, default: $HOME/.xap */
	GList *reg;		/* registered programs */
	void *mreg;
	int dnd_row;
	void *dnd_menu;
	char *dnd_data;
	int dnd_has_drag;
	int timer;
	GList *history;
	void *scroll_w;
	/* geometry */
	int width;
	int height;
} cfg, cfg_t;

#endif

