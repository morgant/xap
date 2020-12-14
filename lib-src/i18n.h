/*
 * i18n.h
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

#ifndef __I18N_H__
#define __I18N_H__

#ifdef HAVE_LIBINTL
#include <libintl.h>
#include <locale.h>
#define _(str) gettext (str)
#else

#define _(str) (str)
#define N_(str) (str)
#define setlocale(lc, str)
#define textdomain(domain)
#define bindtextdomain(package, directory)

#endif
#endif

