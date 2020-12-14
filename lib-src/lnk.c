/*
 * lnk.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>		/* gethostname() */
#include <string.h>
#include <limits.h>
#include <glib.h>
#include "config.h"
#include "lnk.h"

static char *lt_str[LT_MAX] = {
	"Unknown",
	"Application",
	"URL",
	"Directory",
	"Link"
};

/*
 * allocate a new lnk structure
 */
lnk_t *
lnk_new (void)
{
	lnk_t *lnk;

	lnk = (lnk_t *)malloc (sizeof (lnk_t));
	lnk->self = lnk->name = lnk->exec = lnk->icon = lnk->path = lnk->cmnt 
		= NULL;
	lnk->terminal = lnk->type = 0;
	lnk->names = lnk->cmnts = NULL;
	return lnk;
}

/*
 */
char *
lnk_lang (void)
{
	static char *lang = NULL;
	if (lang)
		return lang;
	
	lang = getenv ("LC_LANG");
	if (!lang) {
		lang = getenv ("LC_ALL");
		if (!lang)
			lang = "C";
	}
	return lang;
}

/*
 * free a lnk structure
 */
void
lnk_free (lnk_t *lnk)
{
	keyval_t *kv;
	GList *list;

	if (lnk) {
		if (lnk->self)
			free (lnk->self);
		if (lnk->name)
			free (lnk->name);
		if (lnk->exec)
			free (lnk->exec);
		if (lnk->icon)
			free (lnk->icon);
		if (lnk->path)
			free (lnk->path);
		if (lnk->cmnt)
			free (lnk->cmnt);
		list = lnk->names;
		if (list) {
			while (list) {
				kv = list->data;
				if (list->data) {
					kv = (keyval_t *)list->data;
					if (kv->key)
						free (kv->key);
					if (kv->val)
						free (kv->val);
					free (kv);
				}
				list = list->next;
			}
			g_list_free (lnk->names);
		}
		list = lnk->cmnts;
		if (list) {
			while (list) {
				kv = list->data;
				if (list->data) {
					kv = (keyval_t *)list->data;
					if (kv->key)
						free (kv->key);
					if (kv->val)
						free (kv->val);
					free (kv);
				}
				list = list->next;
			}
			g_list_free (lnk->cmnts);
		}
		free (lnk);
	}
}

/*
 */
const char *
lnk_get_type_str (int type)
{
	if ((type > 0) && (type < LT_MAX))
		return lt_str[type];
	return "";
}

/*
 */
char *
lnk_get_name (lnk_t *lnk)
{
	GList *list;
	keyval_t *kv;
	char *lang = lnk_lang();

	list = lnk->names;
	while (list) {
		kv = list->data;
		if (kv) {
			if (strcmp (kv->key, lang) == 0)
				return kv->val;
		}
		list = list->next;
	}
	return lnk->name;
}

/*
 */
void
lnk_set_name (lnk_t *lnk, char *name)
{
	GList *list;
	keyval_t *kv;
	char *lang = lnk_lang();

	list = lnk->names;
	while (list) {
		kv = list->data;
		if (kv) {
			if (strcmp (kv->key, lang) == 0) {
				free (kv->val);
				kv->val = strdup (name);
				return;
			}
		}
		list = list->next;
	}
	if (name && *name) {
		kv = malloc (sizeof (keyval_t));
		if (kv) {
			kv->key = strdup (lang);
			kv->val = strdup (name);
			lnk->names = g_list_append (lnk->names, kv);
		}
	}
}

/*
 */
char *
lnk_get_comment (lnk_t *lnk)
{
	GList *list;
	keyval_t *kv;
	char *lang = lnk_lang();

	list = lnk->cmnts;
	while (list) {
		kv = list->data;
		if (kv) {
			if (strcmp (kv->key, lang) == 0)
				return kv->val;
		}
		list = list->next;
	}
	return lnk->cmnt;
}

/*
 */
void
lnk_set_comment (lnk_t *lnk, char *cmnt)
{
	GList *list;
	keyval_t *kv;
	char *lang = lnk_lang();

	list = lnk->cmnts;
	while (list) {
		kv = list->data;
		if (kv) {
			if (strcmp (kv->key, lang) == 0) {
				free (kv->val);
				kv->val = strdup (cmnt);
				return;
			}
		}
		list = list->next;
	}
	if (cmnt && *cmnt) {
		kv = malloc (sizeof (keyval_t));
		if (kv) {
			kv->key = strdup (lang);
			kv->val = strdup (cmnt);
			lnk->cmnts = g_list_append (lnk->cmnts, kv);
		}
	}
}

/*
 */
void
lnk_set_exec (lnk_t *lnk, char *exec)
{
	if ((!exec) || (!*exec)) {
		if (lnk->exec) {
			free (lnk->exec);
			lnk->exec = NULL;
		}
	} else {
		if (lnk->exec) {
			free (lnk->exec);
		}
		lnk->exec = strdup (exec);
	}
}

/*
 */
void
lnk_set_icon (lnk_t *lnk, char *icon)
{
	if ((!icon) || (!*icon)) {
		if (lnk->icon) {
			free (lnk->icon);
			lnk->icon = NULL;
		}
	} else {
		if (lnk->icon) {
			free (lnk->icon);
		}
		lnk->icon = strdup (icon);
	}
}

/*
 */
void
lnk_add_name (lnk_t *lnk, char *lang, char *val)
{
	keyval_t *kv;

	kv = malloc (sizeof (keyval_t));
	if (kv) {
		kv->key = strdup (lang);
		kv->val = strdup (val);
		lnk->names = g_list_append (lnk->names, kv);
	}
}

/*
 */
void
lnk_add_comment (lnk_t *lnk, char *lang, char *val)
{
	keyval_t *kv;

	kv = malloc (sizeof (keyval_t));
	if (kv) {
		kv->key = strdup (lang);
		kv->val = strdup (val);
		lnk->cmnts = g_list_append (lnk->cmnts, kv);
	}
}

/*
 * read the content of a lnk file and save it in
 * a new allocated lnk structure
 */
lnk_t *
lnk_read (char *file)
{
	FILE *fp;
	lnk_t *lnk;
	char line[LNK_LINE_MAX+1];
	int len;
	char *ep, *val;

	if (!file || !*file)
		return NULL;

	fp = fopen (file, "r");
	if (!fp)
		return NULL;
	lnk = lnk_new();
	if (!lnk) {
		fclose (fp);
		return NULL;
	}
	line[LNK_LINE_MAX] = '\0';
	while (fgets (line, LNK_LINE_MAX, fp) != NULL) {
		len = strlen (line);
		if (len <= 2)
			continue;
		if ((line[len-1] != '\r') && (line[len-1] != '\n'))
			continue;
		ep = strchr (line, '\r');
		if (!ep)
			ep = strchr (line, '\n');
		*ep-- = '\0';
		val = strchr (line, '=');
		if (!val)
			continue;
		*val++ = '\0';
		if (!*val)
			continue;

		len = strlen (line);

		if (strcasecmp (line, "Name") == 0) {
			lnk->name = strdup (val);
		} else if (strcasecmp (line, "Exec") == 0) {
			lnk->exec = strdup (val);
			lnk->type = LT_EXEC;
		} else if (strcasecmp (line, "Icon") == 0) {
			lnk->icon = strdup (val);
		} else if (strcasecmp (line, "Terminal") == 0) {
			lnk->terminal = atoi (val);
		} else if (strcasecmp (line, "Comment") == 0) {
			lnk->cmnt = strdup (val);
		} else if ((len > 6) && (strncasecmp (line, "Name[", 5) == 0)) {
			ep = strchr (line, ']');
			if (ep) {
				*ep = '\0';
				lnk_add_name (lnk, line+5, val);
			}
		} else if ((len > 9) && (strncasecmp (line, "Comment[", 8) == 0)) {
			ep = strchr (line, ']');
			if (ep) {
				*ep = '\0';
				lnk_add_comment (lnk, line+8, val);
			}
		}
	}
	fclose (fp);

	if (!lnk->name) {
		/* use the link filename as the name
		 */
		lnk->name = strdup (file);
	}
	lnk->self = strdup (file);
	return lnk;
}

/*
 * write the settings to a desktop entry file
 */
int
lnk_write (char *file, lnk_t *lnk)
{
	FILE *fp;
	GList *list;
	keyval_t *kv;

	if (!lnk)
		return 0;

	fp = fopen (file, "w");
	if (!fp)
		return 0;
	fprintf (fp, "[Desktop Entry]\n");
	if (lnk->icon)
		fprintf (fp, "Icon=%s\n", lnk->icon);
	if (lnk->type) {
		fprintf (fp, "Type=%s\n", lnk_get_type_str (lnk->type));
	}
	if (lnk->exec)
		fprintf (fp, "Exec=%s\n", lnk->exec);
	if (lnk->terminal)
		fprintf (fp, "Terminal=%d\n", lnk->terminal);
	if (lnk->name)
		fprintf (fp, "Name=%s\n", lnk->name);
	list = lnk->names;
	while (list) {
		kv = list->data;
		fprintf (fp, "Name[%s]=%s\n", kv->key, kv->val);
		list = list->next;
	}
	if (lnk->cmnt)
		fprintf (fp, "Comment=%s\n", lnk->cmnt);
	list = lnk->cmnts;
	while (list) {
		kv = list->data;
		fprintf (fp, "Comment[%s]=%s\n", kv->key, kv->val);
		list = list->next;
	}
	fclose (fp);
	return 1;
}

#ifdef TEST
int
main (int argc, char **argv)
{
	lnk_t *lnk;

	lnk = lnk_read (argv[1]);
	printf ("link = %s\n", argv[1]);
	if (lnk) {
		printf ("exec=%s\n", lnk->exec);
		printf ("icon=%s\n", lnk->icon);
		printf ("name=%s\n", lnk->name);
		lnk_write ("foo", lnk);
	}
	return 0;
}
#endif
