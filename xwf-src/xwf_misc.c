/*
 * xwf_misc.c
 *
 * Copyright (C) 1999 Rasca, Berlin
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
#include <gtk/gtk.h>
#include "xwf_misc.h"
#include "entry.h"

/*
 * return a list of the files which are selected
 */
int
list_from_selection (GtkCTree *ctree, GList **list)
{
	GList *sel;
	int num = 0;
	entry_t *en;
	GtkCTreeNode *node;

	*list = NULL;
	sel = GTK_CLIST(ctree)->selection;

	while (sel) {
		num++;
		node = sel->data;
		en = gtk_ctree_node_get_row_data (ctree, node);
#ifdef DEBUG
		printf ("%s: list_from_selection() path=%s\n", __FILE__, en->path);
#endif
		*list = g_list_append (*list, g_strdup(en->path));
		sel = sel->next;
	}
#ifdef DEBUG
	printf ("%s: list_from_selection() num=%d\n", __FILE__, num);
#endif
	return (num);
}

/*
 * build a list of entries by the selection
 */
unsigned long
entry_list_from_selection (GtkCTree *ctree, GList **list, int *state)
{
	GList *sel;
	unsigned long num = 0;
	entry_t *en, *new_en;

	*list = NULL;
	sel = GTK_CLIST(ctree)->selection;

	while (sel && state) {
		num++;
		en = gtk_ctree_node_get_row_data (ctree, GTK_CTREE_NODE(sel->data));
#ifdef DEBUG
		printf ("%s: entry_list_from_selection() path=%s\n",__FILE__, en->path);
#endif
		new_en = entry_dupe (en);
		*list = g_list_append (*list, new_en);
		sel = sel->next;
	}
#ifdef DEBUG
	printf ("%s: entry_list_from_selection() num=%ul\n", __FILE__, num);
#endif
	return (num);
}

/*
 */
int
entry_list_free (GList *list)
{
	GList *t = list;
	if (!list)
		return (0);
	while (t) {
		entry_free ((entry_t *)t->data);
		t = t->next;
	}
	g_list_free (list);
	return (1);
}

