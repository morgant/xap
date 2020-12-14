/*
 * xcp_gui.h
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

#ifndef __XCP_GUI_H__
#define __XCP_GUI_H__

#define XAP_PATH ".xap"
#define XCP_RC	"xcp.rc"

typedef struct {
	int x;
	int y;
	int width;
	int height;
} wgeo_t;

#define VERTICAL	1
#define NONE		-1

#define MODE_COPY	'c'
#define MODE_MOVE	'm'
#define MODE_TRASH	't'
#define MODE_LINK	'l'

#define FL_NONE			0
#define FL_PRESERVE		1
#define FL_APPLEDOUBLE	2
#define FL_QUIET		4

void gui_init (int *argc, char ***argv, char *rc);
int  gui_main (wgeo_t *, int mode, char **, char *, int, int);
#endif
