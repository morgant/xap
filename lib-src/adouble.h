/*
 * adouble.h
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

#ifndef __ADOUBLE_H__
#define __ADOUBLE_H__
#include <glib.h>

#define APPLEDOUBLE ".AppleDouble"
/*
 * create the filename for the appledouble file
 */
char * ad_file (char *file);
char * ad_dir (char *file);
char * ad_subdir (char *file);

#endif
