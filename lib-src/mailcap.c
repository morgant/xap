/*
 * mailcap.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mailcap.h"

/*
 * find first white char
 */
static char *
white_char (char *s)
{
	char *p;
	if (!s)
		return NULL;
	p = s;
	while (*p && (*p != ' ' && *p != '\t'))
		p++;
	if (*p)
		return p;
	return NULL;
}

/*
 * find first occurence of " ", "," or "\t"
 */
static char *
cvs_char (char *s)
{
	char *p;
	if (!s)
		return NULL;
	p = s;
	while (*p && (*p != ' ' && *p != '\t' && *p != ','))
		p++;
	if (*p)
		return p;
	return NULL;
}

/*
 * read a line and build with \ continued lines
 * skip comment lines (#) and empty lines
 * remove \n at end of line
 */
int
mc_readline (FILE *fp, char **retp)
{
	static char buf[LINESIZE+1];
	char *pbuf, *p;
	int len, tlen, rest;

	*retp = buf;
NEXT_LINE:
	len = 0;
	rest= LINESIZE;
	pbuf = buf;
	while  (pbuf < (buf+LINESIZE)) {
		p = fgets (pbuf, rest, fp);
		if (!p) {
			if (!len)
				return 0;
			break;
		}
		tlen = strlen (pbuf);
		/* remove \n */
		tlen--;
		pbuf[tlen] = '\0';

		if (tlen > 0 && pbuf[tlen-1] == '\\') {
			tlen--;
			pbuf[tlen] = '\0';
			len += tlen;
			rest = LINESIZE - len;
			pbuf = pbuf+tlen;
		} else {
			/* ok, done */
			len += tlen;

			break;
		}
	}
	/* remove trailing spaces */
	p = buf;
	tlen = 0;
	while (*p && p < buf+LINESIZE) {
		if (*p == ' ' || *p == '\t') {
			tlen++;
		} else {
			break;
		}
		p++;
	}
	if (tlen)
		memmove (buf, buf+tlen, len-tlen+1);
	if (*buf == '#' || !*buf)
		goto NEXT_LINE;
	return len - tlen;
}

/*
 */
mc_mime_type_t *
mc_mt_new (const char *type)
{
	char *p, *mtype, *stype;
	int len;
	const char *cp;
	mc_mime_type_t *mt;

	if (!type)
		return NULL;
	p = strchr (type, '/');
	if (!p)
		return NULL;
	len = p++ - type;
	if ((!*p) || (!len))
		return NULL;
	mt = (mc_mime_type_t *) malloc (sizeof (mc_mime_type_t));
	if (!mt) {
		return NULL;
	}
	mt->mimetype = strdup (type);
	mtype = (char *)malloc (len+1);
	if (!mtype)
		return NULL;
	mtype[len] = '\0';
	memcpy (mtype, type, len);

	cp = type+len+1;
	len = strlen (cp);
	stype = (char *)malloc (len+1);
	if (!stype) {
		free (mtype);
		return NULL;
	}
	stype[len] = '\0';
	memcpy (stype, cp, len);

	mt->maintype = mtype;
	mt->subtype = stype;
	mt->mc = NULL;
	mt->next = mt->prev = NULL;

	if (strcasecmp (mtype, "text") == 0) {
		mt->type = MT_TEXT;
	} else if (strcasecmp (mtype, "image") == 0) {
		mt->type = MT_IMAGE;
	} else if (strcasecmp (mtype, "audio") == 0) {
		mt->type = MT_AUDIO;
	} else if (strcasecmp (mtype, "video") == 0) {
		mt->type = MT_VIDEO;
	} else if (strcasecmp (mtype, "application") == 0) {
		mt->type = MT_APP;
	} else if (strcasecmp (mtype, "multipart") == 0) {
		mt->type = MT_MULTI;
	} else {
		mt->type = MT_EXT;
	}
#if 0
	printf ("type=%s %s %d\n", mt->maintype, mt->subtype, mt->type);
#endif
	return mt;
}

/*
 */
void
mc_mt_free  (mc_mime_type_t *mt)
{
	if (!mt)
		return;
	if (mt->mimetype)
		free (mt->mimetype);
	if (mt->maintype)
		free (mt->maintype);
	if (mt->subtype)
		free (mt->subtype);
	free (mt);
}

/*
 */
mc_suffix_t *
mc_suffix_new (const char *sfx)
{
	mc_suffix_t *entry;
	entry = malloc (sizeof (mc_suffix_t));
	entry->next = entry->prev = NULL;
	entry->mt = NULL;
	entry->sfx = strdup (sfx);
	return (entry);
}

/*
 * add new entry to the top of the suffixes list
 */
mc_suffix_t *
mc_add_suffix (mc_suffix_t *sfx, const char *suffix, mc_mime_type_t *mt)
{
	mc_suffix_t *entry;
#if 0
	fprintf (stderr, "-> %s\n", suffix);
#endif
	if (!suffix)
		return sfx;
	entry = mc_suffix_new (suffix);
	/* */
	if (sfx)
		sfx->prev = entry;
	entry->next = sfx;
	if (mt) {
		entry->mt = mt;
	}
	return entry;
}

/*
 */
mc_mime_type_t *
mc_add_mt_entry (mc_mime_type_t *list, mc_mime_type_t *mt, mc_suffix_t *sfx)
{
	mc_mime_type_t *t, *next;
	mc_suffix_t *tsfx;
	if (!list)
		return mt;
	if (!mt)
		return list;
	t = list;
	while (t) {
		/* remove old one if exists */
		if (strcasecmp (t->mimetype, mt->mimetype) == 0) {
			if (t->prev)
				t->prev->next = t->next;
			if (t->next)
				t->next->prev = t->prev;
			next = t->next;
			/* adjust references */
			tsfx = sfx;
			while (tsfx) {
				if (tsfx->mt == t)
					tsfx->mt = mt;
				tsfx = tsfx->next;
			}
			mc_mt_free (t);
			t = next;
		} else
			t = t->next;
	}
	list->prev = mt;
	mt->next = list;
	/* */
	return mt;
}

/*
 */
mc_mime_type_t *
mc_mt_find (mc_mime_type_t *list, char *mt)
{
	mc_mime_type_t *t;
	t = list;
	while (t) {
		if (strcasecmp (t->mimetype, mt) == 0)
			return t;
		t = t->next;
	}
	return NULL;
}

/*
 * parse a mime.types file
 */
mc_mime_type_t *
mc_parse_mime_types (const char *file, mc_mime_type_t *list,
						mc_suffix_t **retsfx)
{
	FILE *fp;
	mc_mime_type_t *mt, *mt_list;
	char buf[LINESIZE+1], *pbuf, *p, *type, *w;
	int len, nsf;
	mc_suffix_t *sfx = *retsfx;

	mt_list = list;

	if (!file)
		return mt_list;
	fp = fopen (file, "r");
	if (!fp)
		return mt_list;

	buf[LINESIZE] = '\0';
	while ((len = mc_readline (fp, &pbuf)) > 0) {
		if (len <= 2)
			continue;

		p = pbuf;
		type = p;
		p = white_char (p);
		if (!p)
			continue;
		*p++ = '\0';
		while (*p && (*p == ' ' || *p == '\t'))
			p++;
		if (!*p)
			continue;

		mt = mc_mt_new (type);
		if (!mt)
			continue;
		mt_list = mc_add_mt_entry (mt_list, mt, sfx);
		/* parse suffixes
		 */
		nsf = 0;
		while (p && *p) {
			nsf++;
			w = cvs_char (p);
			if (w)
				*w = '\0';
#if 0
			fprintf (stderr, "%s(%d) %s\n", __FILE__, __LINE__, p);
#endif
			if (!strchr (p, '='))
				sfx = mc_add_suffix (sfx, p, mt);
			else
				break;
			if (!w) {
				p = NULL;
			} else {
				p = w+1;
				while (*p && (*p == ' ' || *p == ',' || *p == '\t'))
					p++;
			}
		}
	}
	fclose (fp);
	*retsfx = sfx;
	return mt_list;
}

/*
 */
mc_mailcap_t *
mc_mc_new (char *pname)
{
	mc_mailcap_t *mc;

	if (!pname)
		return NULL;
	mc = malloc (sizeof (mc_mailcap_t));
	if (!mc)
		return NULL;
	mc->view = malloc (strlen (pname)+1);
	if (!mc->view) {
		free (mc);
		return NULL;
	}
	strcpy (mc->view, pname);
	mc->prev = mc->next = NULL;
	return mc;
}

/*
 */
mc_mailcap_t *
mc_mc_add (mc_mailcap_t *list, mc_mailcap_t *mc)
{
	if (!mc)
		return list;
	if (!list)
		return mc;
	list->prev = mc;
	mc->next = list;
	return mc;
}


/*
 * parse a mailcap file
 */
mc_mailcap_t *
mc_parse_mailcap (const char *file, mc_mailcap_t *list, mc_mime_type_t **mtlist,
	mc_suffix_t *sfx)
{
	FILE *fp;
	mc_mailcap_t *mc, *mc_list;
	mc_mime_type_t *mt, *mt_list = NULL;
	int len;
	char *pbuf, *p, *ep, *tp;

	mc_list = list;
	if (mtlist)
		mt_list = *mtlist;
	fp = fopen (file, "r");
	if (!fp)
		return mc_list;
	while ((len = mc_readline (fp, &pbuf)) > 0) {
		p = strchr (pbuf, ';');
		if (!p)
			continue;
		*p++ = '\0';
		mt = mc_mt_find (mt_list, pbuf);
		if (!mt && mt_list) {
			mt = mc_mt_new (pbuf);
			if (!mt)
				continue;
			mt_list = mc_add_mt_entry (mt_list, mt, sfx);
		}
		while (*p == ' ' || *p == '\t')
			p++;
		if (!*p)
			continue;
		if (*p == ';')
			continue;
		ep = strchr (p, ';');
		if (ep == p)
			continue;
		if (ep)
			*ep = '\0';
		/* we use only entries with the sheme "<progname> %s"
		 * cause we remove the "%s" and keep only the "progname"
		 */
		tp = strchr (p, '%');
		if (! (tp && *(tp+1) == 's' && *(tp+2) == '\0'))
			continue;
		*(--tp) = '\0';
		
		mc = mc_mc_new (p);
		if (mc)
			mc_list = mc_mc_add (mc_list, mc);
		if (mt && mc)
			mt->mc = mc;
	}
	fclose (fp);
	if (mtlist)
		*mtlist = mt_list;
	return mc_list;
}

/*
 */
mc_mime_reg_t *
mc_build (void)
{
	mc_mime_reg_t *mime_reg;
	mc_suffix_t *sfx = NULL;
	char *private;
	char *home;
	int len;

	home = getenv ("HOME");
	len = 0;
	if (home) {
		len += strlen (home);
	}
	len += strlen (MIME_FILE) + 3;
	private = malloc (len);
	sprintf (private, "%s/.%s", home, MIME_FILE);

	mime_reg = malloc (sizeof (mc_mime_reg_t));
	if (!mime_reg)
		return NULL;
	mime_reg->mt = mc_parse_mime_types (private, NULL, &sfx);
	mime_reg->mt = mc_parse_mime_types (BASE_MIME_FILE, mime_reg->mt, &sfx);
	mime_reg->sfx= sfx;
	free (private);
	len = 0;
	if (home) {
		len += strlen (home);
	}
	len += strlen (MAILCAP_FILE) + 3;
	private = malloc (len);
	sprintf (private, "%s/.%s", home, MAILCAP_FILE);
	mime_reg->mc = mc_parse_mailcap (private, NULL, &mime_reg->mt, sfx);
	mime_reg->mc = mc_parse_mailcap (BASE_MAILCAP_FILE, mime_reg->mc,
					&mime_reg->mt, sfx);
	free (private);
	return mime_reg;
}

/*
 */
mc_mime_type_t *
mc_mt_by_file (mc_mime_reg_t *mreg, char *file)
{
	char *p;
	mc_suffix_t *sfx;

	if (!file)
		return NULL;
	if (!mreg)
		return NULL;
	if (!mreg->mt)
		return NULL;
	if (!mreg->sfx)
		return NULL;
	p = strrchr (file, '.');
	if (!p) p = file;
	else p++;

	sfx = mreg->sfx;
	while (sfx) {
		if (strcmp (sfx->sfx, p) == 0)
			return sfx->mt;
		sfx = sfx->next;
	}
	return NULL;
}

/*
 */
char *
mc_app_by_file (mc_mime_reg_t *mreg, char *file)
{
	mc_mime_type_t *mt;
	if (!file)
		return NULL;
	if (!mreg)
		return NULL;
	mt = mc_mt_by_file (mreg, file);
	if (!mt) {
		return NULL;
	}
	if (mt->mc)
		return mt->mc->view;
	return NULL;
}

/*
 */
char *
mc_get_mime_type (mc_mime_reg_t *mreg, char *file)
{
	mc_mime_type_t *mt;
	if (!file)
		return NULL;
	if (!mreg)
		return NULL;
	mt = mc_mt_by_file (mreg, file);
	if (!mt) {
		return NULL;
	}
	return mt->mimetype;
}
