/*
 * mailcap.h
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

#ifndef __MAILCAP_H__
#define __MAILCAP_H__

#define BASE_DIR		"/etc"
#define MIME_FILE		"mime.types"
#define BASE_MIME_FILE	(BASE_DIR "/" MIME_FILE)
#define MAILCAP_FILE	"mailcap"
#define BASE_MAILCAP_FILE	(BASE_DIR "/" MAILCAP_FILE)
#define LINESIZE 2048

typedef struct _mailcap_t {
	char *view;		/* view command */
	char *edit; 	/* not used */
	struct _mailcap_t *prev;
	struct _mailcap_t *next;
} mc_mailcap_t;

/* */
typedef struct _mime_type {
	int type;			/* MT_AUDIO, .. */
	int flags;
	char *mimetype;
	char *maintype;
	char *subtype;		/* image/png, .. */
	mc_mailcap_t *mc;
	struct _mime_type *prev;
	struct _mime_type *next;
} mc_mime_type_t;

typedef struct _suffix_t {
	char *sfx;
	mc_mime_type_t *mt;
	struct _suffix_t *prev;
	struct _suffix_t *next;
} mc_suffix_t;

/*
 * hold all lists we need
 */
typedef struct {
	mc_mime_type_t *mt;
	mc_mailcap_t *mc;
	mc_suffix_t *sfx;
} mc_mime_reg_t;

enum {
	MT_NONE,
	MT_TEXT,
	MT_IMAGE,
	MT_AUDIO,
	MT_VIDEO,
	MT_APP,
	MT_MULTI,
	MT_EXT
};

mc_mime_type_t *mc_parse_mime_types (const char *file, mc_mime_type_t *list,
							mc_suffix_t **sfx);
mc_mime_reg_t *mc_build (void);
char *mc_app_by_file (mc_mime_reg_t *reg, char *path);
char *mc_get_mime_type (mc_mime_reg_t *reg, char *path);

#endif
