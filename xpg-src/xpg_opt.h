/*
 * xpg_opt.h
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

#ifndef __XPG_OPT_H__
#define __XPG_OPT_H__

typedef struct {
	int file_enc_armor;
	int file_enc_remove_org;
	int file_dec_remove_org;
	int quiet;
	char *file;
	char *err_file;
} opt_t;

opt_t *opt_read (char *);
int opt_save (opt_t *);

#endif
