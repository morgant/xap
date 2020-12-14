/*
 * xap_gui.h
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

#ifndef __XAP_GUI_H__
#define __XAP_GUI_H__

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define XAP_PATH ".xap"
#define XAP_RC	"xap.rc"

typedef struct {
	int x;
	int y;
	int width;
	int height;
} wgeo_t;

#define HORIZONTAL	0
#define VERTICAL	1
#define NONE		-1

void gui_init (int *argc, char ***argv, char *rc);
void gui_main (char *path, int transient, wgeo_t *, int , int);
#endif
