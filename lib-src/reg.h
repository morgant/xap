/*
 * reg.h
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

#ifndef __REG_H__
#define __REG_H__

typedef struct {
	char *app;	/* program name */
	char *arg;	/* arguments/options */
	char *sfx;	/* suffix */
	int len;	/* suffix length */
} reg_t;

GList *reg_build_list(char *file);
void reg_destroy_list(GList *list);
GList *reg_add_suffix (GList *list, char *sfx, char *prog, char *args);
char *reg_app_by_file (GList *list, char *file);
char *reg_app_by_suffix(GList *list, char *sfx);
reg_t *reg_prog_by_file (GList *list, char *file);
reg_t *reg_prog_by_suffix (GList *list, char *sfx);
GList *reg_app_list(GList *list);
int reg_save (GList *list);

#endif
