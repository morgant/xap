/*
 * reg.c
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

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "reg.h"

static char *regfile = NULL;

/*
 */
static int
compare_sfx (reg_t *reg1, reg_t *reg2)
{
	return strcmp (reg1->sfx, reg2->sfx);
}

void
reg_free_reg (reg_t *app)
{
	if (!app)
		return;
	if (app->app)
		g_free (app->app);
	if (app->arg)
		g_free (app->arg);
	if (app->sfx)
		g_free (app->sfx);
	g_free (app);
}

/*
 * on startup we build a complete list
 */
GList *
reg_build_list (char *file)
{
#define BUFSIZE 1024
	FILE *fp;
	GList *g_reg = NULL;
	char buf[BUFSIZE], *p, *ep, *buf_end;
	char suffix[256];
	char cmd[PATH_MAX+1];
	char arg[PATH_MAX+1];
	reg_t *prg;
	int inner;
	int line;

	regfile = g_strdup (file);
	fp = fopen (file, "r");
	if (!fp) {
		printf ("Warning, can't find %s\n", file);
		return (NULL);
	}
	buf_end = buf+BUFSIZE-1;
	line = 0;
	while (fgets(buf, 1024, fp) != NULL) {
		line++;
		p = buf;
		while ((*p == ' ') || (*p == '\t'))
			p++;
		if (*p == '#')
			continue;
		prg = g_malloc (sizeof(reg_t));
		if (!prg) {
			perror ("malloc()");
			fclose (fp);
			return (g_reg);
		}
		prg->app = prg->sfx = prg->arg = NULL;
		if (*p != '(') {
			/* old style */
			*arg = '\0';
			sscanf (buf, "%s %[^ \n] %[^\n]", suffix, cmd, arg);
#ifdef DEBUG2
			printf ("%s:%s\n", suffix, cmd);
#endif
			prg->sfx = g_strdup (suffix);
			prg->app = g_strdup (cmd);
			if (*arg)
				prg->arg = g_strdup (arg);
			prg->len = strlen(suffix);
			g_reg = g_list_append (g_reg, prg);
		} else {
			/* new style:
			 * (suffix)(program)(arguments)
			 */
			inner = 0;
			p++;
			ep = p;
			while (*ep && (ep < buf_end)) {
				if (*ep == '(') inner++;
				else if (*ep == ')') {
					if (inner) inner--;
					 else break;
				}
				ep++;
			}
			if (*ep != ')') {
				/* parse error */
				fprintf (stderr, "parse error at line %d\n", line);
				reg_free_reg (prg);
				continue;
			}
			*ep = '\0';
			prg->sfx = g_strdup (p);
			prg->len = strlen(prg->sfx);

			/* find program */
			p = ep+1;
			while (*p && (p < buf_end)) {
				if (*p == '(')
					break;
				p++;
			}
			if (*p != '(') {
				/* parse error */
				fprintf (stderr, "parse error at line %d\n", line);
				reg_free_reg (prg);
				continue;
			}
			inner = 0;
			p++;
			ep = p;
			while (*ep && (ep < buf_end)) {
				if (*ep == '(') inner++;
				else if (*ep == ')') {
					if (inner) inner--;
					 else break;
				}
				ep++;
			}
			if (*ep != ')') {
				/* parse error */
				fprintf (stderr, "parse error at line %d\n", line);
				reg_free_reg (prg);
				continue;
			}
			*ep = '\0';
			prg->app = g_strdup (p);

			/* find args */
			p = ep+1;
			while (*p && (p < buf_end)) {
				if (*p == '(')
					break;
				p++;
			}
			if (*p == '(') {
				inner = 0;
				p++;
				ep = p;
				while (*ep && (ep < buf_end)) {
					if (*ep == '(') inner++;
					else if (*ep == ')') {
						if (inner) inner--;
						 else break;
					}
					ep++;
				}
				if (*ep != ')') {
					/* parse error */
					fprintf (stderr, "parse error at line %d\n", line);
					reg_free_reg (prg);
					continue;
				}
				*ep = '\0';
				if (p != ep)
					prg->arg = g_strdup (p);
			}
			g_reg = g_list_append (g_reg, prg);
		}
#ifdef DEBUG
		fprintf (stderr, "%s->%s->%s\n", prg->sfx, prg->app, prg->arg);
#endif
	}
	fclose (fp);
	return (g_reg);
}

/*
 */
char *
reg_app_by_suffix (GList *g_reg, char *sfx)
{
	reg_t *prg;
	while (g_reg) {
		prg = g_reg->data;
		if (strcmp (prg->sfx, sfx) == 0) {
#ifdef		DEBUG2
			printf ("reg_app_by_sfx () ->%s\n", prg->app);
#endif
			return (prg->app);
		}
		g_reg = g_reg->next;
	}
	return (NULL);
}

/*
 */
reg_t *
reg_prog_by_suffix (GList *g_reg, char *sfx)
{
	reg_t *prg;
	while (g_reg) {
		prg = g_reg->data;
		if (strcmp (prg->sfx, sfx) == 0) {
#ifdef		DEBUG2
			printf ("reg_app_by_sfx () ->%s\n", prg->app);
#endif
			return (prg);
		}
		g_reg = g_reg->next;
	}
	return (NULL);
}

/*
 */
char *
reg_app_by_file (GList *g_reg, char *file)
{
	reg_t *prg;
	int len;

#ifdef DEBUG
	printf ("%s: reg_app_by_file(.., file=%s)\n", __FILE__, file);
#endif
	if (!file)
		return NULL;
	len = strlen (file);

	while (g_reg) {
		prg = g_reg->data;
		if ((!prg) || (prg->len > len)) {
			g_reg = g_reg->next;
			continue;
		}
		if (strcmp (file+(len - prg->len), prg->sfx) == 0) {
#ifdef		DEBUG2
			printf ("reg_app_by_sfx () ->%s\n", prg->app);
#endif
			return (prg->app);
		}
		g_reg = g_reg->next;
	}
	return (NULL);
}

/*
 */
reg_t *
reg_prog_by_file (GList *g_reg, char *file)
{
	reg_t *prg;
	int len;

#ifdef DEBUG
	printf ("%s: reg_app_by_file(.., file=%s)\n", __FILE__, file);
#endif
	if (!file)
		return NULL;
	len = strlen (file);

	while (g_reg) {
		prg = g_reg->data;
		if ((!prg) || (prg->len > len)) {
			g_reg = g_reg->next;
			continue;
		}
		if (strcmp (file+(len - prg->len), prg->sfx) == 0) {
#ifdef		DEBUG2
			printf ("reg_app_by_file () ->%s\n", prg->app);
#endif
			return (prg);
		}
		g_reg = g_reg->next;
	}
	return (NULL);
}

/*
 * register a new suffix
 */
GList *
reg_add_suffix (GList *g_reg, char *sfx, char *program, char *args)
{
	reg_t *prg = NULL;
	GList *g_tmp = g_reg;
	int found = 0;
#ifdef DEBUG_XWF
	printf ("reg_add_suffix() %s -> %s\n", sfx, program);
#endif
	/* is suffix still registered?
	 * check it out ..
	 */
	while (g_tmp) {
		prg = g_tmp->data;
		if (strcmp (prg->sfx, sfx) == 0) {
			found = 1;
			break;
		}
		g_tmp = g_tmp->next;
	}
	if (found) {
		g_free (prg->app);
		prg->app = g_strdup (program);
		g_free (prg->arg);
		prg->arg = NULL;
		if (args) {
			prg->arg = g_strdup (args);
		}
	} else {
		prg = g_malloc (sizeof(reg_t));
		prg->app = g_strdup (program);
		prg->sfx = g_strdup (sfx);
		prg->len = strlen(sfx);
		if (args) {
			prg->arg = g_strdup (args);
		} else {
			prg->arg = NULL;
		}
		g_reg = g_list_append (g_reg, prg);
	}
	g_reg = g_list_sort (g_reg, (GCompareFunc)compare_sfx);
	return g_reg;
}

/*
 * free the registry list
 */
void
reg_destroy_list (GList *list)
{
	GList *g_tmp = list;
	reg_t *prg;
	while (g_tmp) {
		prg = (reg_t *) g_tmp->data;
		g_free (prg->sfx);
		g_free (prg->app);
		if (prg->arg)
			g_free (prg->arg);
		g_free (prg);
		g_tmp = g_tmp->next;
	}
	g_list_free (list);
}


/*
 * save registry to named file
 */
int
reg_save (GList *g_reg)
{
	FILE *fp;
	reg_t *prg;

	if (!regfile)
		return (0);
	fp = fopen (regfile, "w");
	if (!fp) {
		perror (regfile);
		return (FALSE);
	}
	fprintf (fp, "# (suffix) (program) (args)\n");
	while (g_reg) {
		prg = g_reg->data;
		if (prg) {
			if (prg->arg) {
				fprintf (fp, "(%s) (%s) (%s)\n", prg->sfx, prg->app, prg->arg);
			} else {
				fprintf (fp, "(%s) (%s) ()\n", prg->sfx, prg->app);
			}
		}
		g_reg = g_reg->next;
	}
	fclose (fp);
	return (TRUE);
}

/*
 * return a list of all registered applications
 * the list does not have their own data values, just pointers!
 */
GList *
reg_app_list (GList *g_reg)
{
	GList *g_apps = NULL;
	GList *g_tmp;

	while (g_reg) {
		g_apps = g_list_append (g_apps, ((reg_t *)g_reg->data)->app);
		g_reg = g_reg->next;
	}
	g_apps = g_list_sort (g_apps, (GCompareFunc)strcmp);
	/* remove dupes */
	g_tmp = g_apps;
	while (g_tmp) {
		if (g_tmp->next) {
			if (strcmp ((char*)g_tmp->data, (char*)g_tmp->next->data) == 0) {
				g_tmp = g_apps = g_list_remove (g_apps, g_tmp->data);
				continue;
			}
		}
		g_tmp = g_tmp->next;
	}
	return (g_apps);
}

/*
 */
int
reg_execute (reg_t *app, char *file)
{
	return 0;
}

