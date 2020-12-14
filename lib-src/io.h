/*
 * io.h
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

#ifndef __IO_H__
#define __IO_H__

int io_is_exec (char *file);
int io_can_exec (char *file);
int io_can_write_to_parent (char *file);
int io_system_nice (char *cmd, int n);
#define io_system(cmd) io_system_nice(cmd,0);
int io_system_var (char **cmd, int len);
int io_is_directory (char *path);
int io_is_file (char *file);
int io_is_link (char *file);
int io_item_exists (char *file);

#define io_is_root(s)	((s[0] == '/') && (s[1] == '\0'))
#define io_is_hidden(s)	(s[0] == '.'? TRUE : FALSE)
#define io_is_current(s) ((s[0]=='.') && (s[1]=='\0') ? TRUE : FALSE)
#define io_is_dirup(s)	((s[0]=='.') && (s[1]=='.') && (s[2]=='\0')? TRUE:FALSE)

#endif
