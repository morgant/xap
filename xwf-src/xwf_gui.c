/*
 * xwf_gui.c
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
#include <unistd.h>
#include <utime.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "config.h"
#include "xwf_gui.h"
#include "i18n.h"
#include "entry.h"
#include "xwf_icon.h"
#include "gtk_dlg.h"
#include "gtk_exec.h"
#include "gtk_dnd.h"
#include "xwf_cfg.h"
#include "xwf_dnd.h"
#include "gtk_util.h"
#include "uri.h"
#include "io.h"
#include "top.h"
#include "reg.h"
#include "mailcap.h"
#include "history.h"
#ifdef ADOUBLE
#include "adouble.h"
#endif

enum {
	MN_DIR,
	MN_FILE,
	MN_MIXED,
	MENUS,
};

enum {
	COL_NAME,
	COL_SIZE,
	COL_DATE,
	COLUMNS		/* number of columns */
};

#define SPACING 4
#define DEF_APP	"netscape"
#ifdef DEBUG
#define TIMERVAL	40000
#else
#define TIMERVAL	4000
#endif
#define XWF_RC "xwf.rc"

#define yes	1
#define no	0
#define ERROR -1

static GtkTargetEntry target_table[] = {
	/* target,			flags,					info	*/
    {"application/x-xwf-list", 0,			TARGET_XWF_LIST },
    {"text/uri-list",	0,  				TARGET_URI_LIST },
    {"text/plain", 		0,  				TARGET_PLAIN },
    {"STRING",     		0,  				TARGET_STRING },
    /* {"application/x-rootwin-drop",	0,	TARGET_ROOTWIN }, */
};
#define NUM_TARGETS (sizeof(target_table)/sizeof(GtkTargetEntry))

#define ACCEL	2
typedef struct {
	char *label;
	void *func;
	int data;
	gint key;
	guint8 mod;
} menu_entry;



static GtkWidget *new_top (char *p, char *x, char *trash, GList *reg,
							mc_mime_reg_t *mreg,
							int width, int height, int flags);

/*
 * gtk init function, must be called first
 */
void
gui_init (int *argc, char ***argv, char *user_rc)
{
	gtk_set_locale ();
	gtk_init (argc, argv);

	gtk_rc_add_default_file (XWF_RC);
	gtk_rc_parse (DATA_DIR"/xap/"XAP_RC);
	gtk_rc_parse (user_rc);
}

/*
 */
#define FATAL 1
#define alloc_error_fatal() alloc_error(__FILE__,__LINE__,FATAL)
static void
alloc_error (char *file, int num, int mode)
{
	fprintf (stderr, "malloc error in %s at line %d\n", file, num);
	if (mode == FATAL)
		exit (1);
}

/*
 * my own sort function
 * honor if an entry is a directory or a file
 */
static gint
my_compare (GtkCList *clist, gconstpointer ptr1, gconstpointer ptr2)
{
	GtkCTreeRow *row1 = (GtkCTreeRow *) ptr1;
	GtkCTreeRow *row2 = (GtkCTreeRow *) ptr2;
	entry_t *en1, *en2;
	int type1, type2;

	en1 = row1->row.data;
	en2 = row2->row.data;
	type1 = en1->type & FT_DIR ? FT_DIR : FT_FILE;
	type2 = en2->type & FT_DIR ? FT_DIR : FT_FILE;
	if (type1 != type2) {
		/* i want to have the directories at the top
		 */
		return (type1 < type2 ? -1 : 1);
	}
	if (clist->sort_column != COL_NAME) {
		/* use default compare function which we have saved before
		 */
		GtkCListCompareFunc compare;
		cfg_t *win;
		win = gtk_object_get_user_data (GTK_OBJECT(clist));
		compare = (GtkCListCompareFunc)win->compare;
		return compare(clist, ptr1, ptr2);
	}
	return strcmp (en1->label, en2->label);
}


/*
 * called if a node will be destroyed
 * so free all private data
 */
void
node_destroy (gpointer p)
{
	entry_t *en = (entry_t *)p;
#ifdef DEBUG_XWF
	printf ("node_destroy(0x%X) label=%s\n", (unsigned int)p, en->label);
#endif
	entry_free (en);
}

/*
 * count the number of  selected nodes, if there are no nodes selected
 * in "first" the root node is returned
 */
int
count_selection (GtkCTree *ctree, GtkCTreeNode **first)
{
	int num =0;
	GList *list;

	*first = GTK_CTREE_NODE(GTK_CLIST(ctree)->row_list);

	list = GTK_CLIST(ctree)->selection;
	num = g_list_length (list);
	if (num <= 0) {
#ifdef	DEBUG
		printf ("count_selection() ctree=0x%X list length=0\n", (int)ctree);
#endif
		return (0);
	}
	*first = GTK_CTREE_NODE(GTK_CLIST(ctree)->selection->data);
	return (num);
}


/*
 * on mouse click on the column headers the rows
 * are sorted accordingly
 */
void
on_click_column (GtkCList *clist, gint column, gpointer data)
{
	int num;
	GtkCTreeNode *node;
	GList *selection;

	if (column != clist->sort_column)
		gtk_clist_set_sort_column (clist, column);
	else {
		if (clist->sort_type == GTK_SORT_ASCENDING)
			clist->sort_type = GTK_SORT_DESCENDING;
		else
			clist->sort_type = GTK_SORT_ASCENDING;
	}
	num = count_selection (GTK_CTREE(clist), &node);
	if (num) {
		for (selection = GTK_CLIST(clist)->selection;
					selection; selection = selection->next) {
			node = selection->data;
			if (!GTK_CTREE_ROW(node)->children ||
					(!GTK_CTREE_ROW(node)->expanded)) {
				/* select parent node */
				node = GTK_CTREE_ROW(node)->parent;
			}
			gtk_ctree_sort_node (GTK_CTREE(clist), node);
		}
	} else {
		gtk_clist_sort (clist);
		/* workaround for bug in 1.2.8 */
		if (GTK_CLIST(clist)->row_list)
			GTK_CLIST(clist)->row_list->prev = NULL;
	}
}

/*
 * if the menu is detached
 */
void
menu_detach ()
{
#ifdef DEBUG_XWF
	printf ("menu_detach()\n");
#endif
}

/*
 */
void
cb_open_trash (GtkWidget *item, void *data)
{
	cfg_t *win = (cfg *)data;
	new_top (win->trash, win->xap, win->trash, win->reg,
				(mc_mime_reg_t*) win->mreg,
				win->width, win->height, 0);
}

/*
 * open new windows with the marked directories
 */
void
cb_new_window (GtkWidget *widget, GtkCTree *ctree)
{
	int num;
	GList *selection;
	GtkCTreeNode *node;
	entry_t *en = NULL;
	cfg *win;

	win = gtk_object_get_user_data (GTK_OBJECT(ctree));

	num = count_selection (ctree, &node);
	if (num) {
		for (selection = GTK_CLIST(ctree)->selection;
					selection; selection = selection->next) {
			node = selection->data;
			en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
			if (! EN_IS_DIR(en)) {
				node = GTK_CTREE_ROW(node)->parent;
				if (!node) {
					continue;
				}
			}
			en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
			new_top (uri_clear_path(en->path), win->xap, win->trash, win->reg,
						(mc_mime_reg_t *)win->mreg,
						win->width, win->height, en->flags);
		}
	} else {
		en = gtk_ctree_node_get_row_data (ctree, node);
		new_top (uri_clear_path(en->path), win->xap, win->trash, win->reg,
					(mc_mime_reg_t *)win->mreg,
					win->width, win->height, en->flags);
	}
}

/*
 */
static void
node_unselect_by_type (GtkCTree *ctree, GtkCTreeNode *node, void *data)
{
	entry_t *en;

	en = gtk_ctree_node_get_row_data (ctree, node);
#ifdef DEBUG_XWF
	printf ("node_unselect_by_type() %s viewable=%d\n", en->path,
				gtk_ctree_is_viewable(GTK_CTREE(ctree), node));
#endif
	if (en->type & (int)data) {
		gtk_ctree_unselect (ctree, node);
	}
}

/*
 */
static void
node_unselect_non_viewables (GtkCTree *ctree, GtkCTreeNode *node, void *data)
{
	entry_t *en;

	en = gtk_ctree_node_get_row_data (ctree, node);
	if (!gtk_ctree_is_viewable(GTK_CTREE(ctree), node)) {
		gtk_ctree_unselect (ctree, node);
	}
}

/*
 * select all entries in the current depth
 */
static void
cb_select (GtkWidget *item, GtkCTree *ctree)
{
	GtkCTreeNode *node;

	/* find top entry
	 */
	count_selection (ctree, &node);
	if (!GTK_CTREE_ROW(node)->expanded)
		node = GTK_CTREE_ROW(node)->parent;

	gtk_ctree_select_recursive (ctree, node);
	gtk_ctree_unselect (ctree, node);
	gtk_ctree_pre_recursive (ctree, node, node_unselect_non_viewables, NULL);
	gtk_ctree_pre_recursive (ctree, node, node_unselect_by_type,
			(void *)FT_DIR_UP);
}

/*
 * unselect all marked items
 */
void
cb_unselect (GtkWidget *widget, GtkCTree *ctree)
{
	gtk_ctree_unselect_recursive (ctree, NULL);
}



/*
 * popup context menu
 */
static gint
on_button_press (GtkWidget *widget, GdkEventButton *event, void *data)
{
	GtkCTree *ctree = GTK_CTREE(widget);
	GtkWidget **menu = (GtkWidget **)data;
	GtkCTreeNode *node;
	int num, row, column, mn_no = MN_DIR;
	entry_t *en;

#ifdef DEBUG
	fprintf (stderr, "on_button_press (%p, %p, %p)\n", widget, event, data);
#endif
	if (event->button == 3) {
		num = count_selection (ctree, &node);
		if (!num) {
			row = -1;
			gtk_clist_get_selection_info (GTK_CLIST(widget),
				event->x, event->y,
				&row, &column);
#ifdef DEBUG
			fprintf (stderr, " row=%d, col=%d\n", row, column);
#endif
			if (row > -1) {
				gtk_clist_select_row (GTK_CLIST(ctree), row, 0);
				/* arghh.. should ever be.. but is not while pressing shift
				 * at the same time */
				if (GTK_CLIST(ctree)->selection)
					num = 1;
			}
		}
#ifdef DEBUG
		printf (" on_button_press() type=%d selected=%d\n", event->type, num);
#endif
		if (num == 1) {
			node = GTK_CLIST(ctree)->selection->data;
			en = gtk_ctree_node_get_row_data (ctree, node);
			if (EN_IS_DIR(en))
				mn_no = MN_DIR;
			else
				mn_no = MN_FILE;
		} else if (num > 1) {
			mn_no = MN_MIXED;
		}
		/* fprintf (stderr, "menu:%d\n", mn_no); */
		gtk_menu_popup (GTK_MENU(menu[mn_no]), NULL, NULL, NULL, NULL,
				3, event->time);
		return TRUE;
	}
	return FALSE;
}

/*
 * freeze the tree - so we don't waste time for updates
 */
static void
ctree_freeze (GtkCTree *ctree)
{
	gu_cursor_wait (GTK_WIDGET(ctree));
	gtk_clist_freeze (GTK_CLIST(ctree));
}

/*
 * now refresh the new list on the screen
 */
static void
ctree_thaw (GtkCTree *ctree)
{
	gtk_clist_thaw (GTK_CLIST(ctree));
	gu_cursor_reset (GTK_WIDGET(ctree));
}

/*
 * create and add a node to the tree
 */
static GtkCTreeNode *
add_node (GtkCTree *ctree, GtkCTreeNode *parent, GtkCTreeNode *sibling,
			char *label, char *path, int *type, int flags)
{
	entry_t *en;
	GtkCTreeNode *item;
	gchar *text[COLUMNS];
	gchar size[16] = {""};
	gchar date[18] = {""};
	GdkPixmap *icon[4];

	if (!label || !path) {
		fprintf (stderr, "Fatal error %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}
	if (*type & FT_DUMMY) {
#ifdef DEBUG_XWF
		printf (" adding dummy for %s\n", path);
#endif
		en = entry_new ();
		en->label = g_strdup (label);
		en->path  = g_strdup (path);
		en->type = FT_DIR | FT_DUMMY;
	} else {
		en = entry_new_by_path_and_label (path, label);
		if (!en) {
			fprintf (stderr, "Fatal error, %s[%d] %s: %s\n",
						__FILE__, __LINE__, label, strerror(errno));
			return 0;
		}
		en->flags = flags;

		sprintf (date, "%04d-%02d-%02d %02d:%02d",
						en->date.year, en->date.month, en->date.day,
						en->date.hour, en->date.min);
		if (en->size < 0) {
			sprintf (size, "?(ERR %d)", -en->size);
		} else {
			sprintf (size, "%10lu", (long unsigned) en->size);
		}
	}
	text[COL_NAME] = en->label;
	text[COL_DATE] = date;
	text[COL_SIZE] = size;

#ifdef DEBUG_XWF
	printf ("add_node() label=%s, path=%s type=%d\n",
				en->label, en->path, en->type);
#endif
	icon_find (en, getuid(), icon);

	if (EN_IS_DIR(en)) {
		item = gtk_ctree_insert_node (ctree, parent, sibling, text, SPACING,
						icon[0], icon[1], icon[2], icon[3], FALSE, FALSE);
	} else if (en->type & FT_DIR_PD) {
		item = gtk_ctree_insert_node (ctree, parent, NULL, text, SPACING,
						icon[0], icon[1], icon[2], icon[3], FALSE, FALSE);
	} else {
		item = gtk_ctree_insert_node (ctree, parent, NULL, text, SPACING,
						icon[0], icon[1], icon[2], icon[3], TRUE, FALSE);
	}

	gtk_ctree_node_set_row_data_full (ctree, item, en, node_destroy);
	*type = en->type;
	return (item);
}

/*
 */
static void
add_subtree (GtkCTree *ctree, GtkCTreeNode *root, char *path, int depth,
			int flags)
{
	DIR *dir;
	struct dirent *de;
	GtkCTreeNode *item = NULL, *first= NULL;
	char *base;
	gchar complete[PATH_MAX+NAME_MAX+1];
	gchar label[NAME_MAX+1];
	int add_slash = no, len;
	int type = 0;
	int count, count_max = 200;

	if (depth == 0)
		return;

	if (!path)
		return;
	len = strlen (path);
	if (!len)
		return;
	if (path[len-1] != G_DIR_SEPARATOR) {
		add_slash = yes;
		len++;
	}
	base = g_malloc (len+1);
	if (!base)
		alloc_error_fatal();
	strcpy (base, path);
	if (add_slash)
		strcat (base, G_DIR_SEPARATOR_S);

#ifdef DEBUG
	printf ("add_subtree() depth=%d, path=%s flags=%d\n", depth, path, flags);
#endif
	if (depth == 1) {
		/* create dummy entry */
		sprintf (complete, "%s.", base);
		type = FT_DUMMY;
		add_node (GTK_CTREE(ctree), root, NULL, ".", complete, &type, flags);
		g_free (base);
		return;
	}
	dir = opendir (path);
	if (!dir) {
		g_free (base);
		return;
	}
	count = 0;
	while ((de = readdir (dir)) != NULL) {
		type = 0;
		if (de->d_name[0] == '.') {
			if ((de->d_name[1] == '.') && (de->d_name[2] == '\0')) {
				type |= FT_DIR_UP;
			} else {
				if (((flags & IGNORE_HIDDEN) && (de->d_name[1] != '\0'))
						|| (de->d_name[1] == '\0'))
				continue;
			}
		}
		sprintf (complete, "%s%s", base, de->d_name);
		strcpy  (label, de->d_name);
		item = add_node (GTK_CTREE(ctree),
				root, first, label, complete, &type, flags);
		if ((type & FT_DIR) && (!(type & FT_DIR_UP))) {
			add_subtree (ctree, item, complete, depth-1, flags);
		} else {
			if (!first)
				first = item;
		}
		count++;
		if (count > count_max) {
			count = 0;
			count_max *= 3;
			gtk_ctree_sort_node (ctree, root);
			gtk_clist_thaw (GTK_CLIST(ctree));
			while (gtk_events_pending ())
				gtk_main_iteration ();
			gtk_clist_freeze (GTK_CLIST(ctree));
		}
	}
	g_free (base);
	closedir (dir);
	gtk_ctree_sort_node (ctree, root);
	gtk_clist_thaw (GTK_CLIST(ctree));
}

/*
 */
void
on_dotfiles (GtkWidget *item, GtkCTree *ctree)
{
	GtkCTreeNode *node, *child;
	entry_t *en;

	gtk_clist_freeze (GTK_CLIST(ctree));

	/* use first selection if available
	 */
	count_selection (ctree, &node);
	en = gtk_ctree_node_get_row_data (ctree, node);
	if (!en)
		printf ("FIXME!\n");
	if (!(en->type & FT_DIR)) {
		/* select parent node */
		node = GTK_CTREE_ROW(node)->parent;
		en = gtk_ctree_node_get_row_data (ctree, node);
	}
	child = GTK_CTREE_ROW(node)->children;
	while (child) {
		gtk_ctree_remove_node (ctree, child);
		child = GTK_CTREE_ROW(node)->children;
	}
	if (en->flags & IGNORE_HIDDEN)
		en->flags &= ~IGNORE_HIDDEN;
	else
		en->flags |= IGNORE_HIDDEN;
	add_subtree (ctree, node, en->path, 2, en->flags);
	gtk_ctree_expand (ctree, node);
	gtk_clist_thaw (GTK_CLIST(ctree));
}


/*
 */
void
on_expand (GtkCTree *ctree, GtkCTreeNode *node, char *path)
{
	GtkCTreeNode *child;
	entry_t *en;

	ctree_freeze (ctree);
	child = GTK_CTREE_ROW(node)->children;
	while (child) {
		gtk_ctree_remove_node (ctree, child);
		child = GTK_CTREE_ROW(node)->children;
	}
	en = gtk_ctree_node_get_row_data (ctree, node); 
	add_subtree (ctree, node, en->path, 2, en->flags);
	ctree_thaw (ctree);
}

/*
 * unmark all marked on collapsion
 */
void
on_collapse (GtkCTree *ctree, GtkCTreeNode *node, char *path)
{
	GtkCTreeNode *child;
#ifdef DEBUG2
	entry_t *en;
	en = gtk_ctree_node_get_row_data (ctree, node); 
	printf ("on_collapse() path=%s is_leaf=%d childs=%s\n",
			en->path, GTK_CTREE_ROW(node)->is_leaf,
			GTK_CTREE_ROW(node)->children?"yes":"no");
#endif
	/* unselect all children */
	child = GTK_CTREE_NODE(GTK_CTREE_ROW(node)->children);
	while (child) {
		gtk_ctree_unselect (ctree, child);
		child = GTK_CTREE_ROW(child)->sibling;
	}
}

/*
 * realy delete files incl. subs
 */
void
delete_files (char *path)
{
	struct stat st;
	DIR *dir;
	struct dirent *de;
	char complete[PATH_MAX+NAME_MAX+1];

	if (lstat (path, &st) == -1) {
		perror (path);
		return ;
	}
	if (S_ISDIR (st.st_mode) && (!S_ISLNK (st.st_mode))) {
		if (access (path, R_OK|W_OK) == -1) {
			return;
		}
		dir = opendir (path);
		if (!dir) {
			return;
		}
		while ((de = readdir(dir)) != NULL) {
			if (io_is_current (de->d_name))
				continue;
			if (io_is_dirup (de->d_name))
				continue;
			sprintf (complete, "%s/%s", path, de->d_name);
			delete_files (complete);
		}
		closedir (dir);
		rmdir (path);
	} else {
		unlink (path);
	}
}

/*
 * find a node and check if it is expanded
 */
void
node_is_open (GtkCTree *ctree, GtkCTreeNode *node, void *data)
{
	GtkCTreeRow *row;
	entry_t *check = (entry_t *) data;
	entry_t *en = gtk_ctree_node_get_row_data (ctree, node);

	if (strcmp (en->path, check->path) == 0) {
		row = GTK_CTREE_ROW(node);
		if (row->expanded) {
			check->label = (char *)node;
			check->flags = TRUE;
		}
	}
}

/*
 */
int
compare_node_path (gconstpointer ptr1, gconstpointer ptr2)
{
	entry_t *en1 = (entry_t *)ptr1, *en2 = (entry_t *) ptr2;

	return strcmp (en1->path, en2->path);
}

/*
 * empty trash folder
 */
void
cb_empty_trash (GtkWidget *widget, GtkCTree *ctree)
{
	GtkCTreeNode *node;
	cfg *win;
	DIR *dir;
	struct dirent *de;
	char complete[PATH_MAX+1];
	entry_t check;

	win = gtk_object_get_user_data (GTK_OBJECT(ctree));
	check.path = win->trash;
	check.flags = FALSE;
#ifdef DEBUG
	printf ("cb_empty_trash() trash=%s\n", win->trash);
#endif
	if (!win)
		return;
	/* check if the trash dir is open, so we have to update */
	gtk_ctree_pre_recursive (ctree,
		GTK_CTREE_NODE(GTK_CLIST(ctree)->row_list), node_is_open, &check);
	dir = opendir (win->trash);
	if (!dir)
		return;
	gu_cursor_wait (GTK_WIDGET(ctree));
	while ((de = readdir(dir)) != NULL) {
		if (io_is_current (de->d_name))
			continue;
		if (io_is_dirup (de->d_name))
			continue;
		sprintf (complete, "%s/%s", win->trash, de->d_name);
		delete_files (complete);
		if (check.flags) {
			/* remove node */
			check.path = complete;
			node = gtk_ctree_find_by_row_data_custom (ctree,
						GTK_CTREE_NODE(GTK_CLIST(ctree)->row_list), &check,
						compare_node_path);
			if (node) {
				gtk_ctree_remove_node (ctree, node);
			}
		}
	}
	closedir (dir);
	gu_cursor_reset (GTK_WIDGET(ctree));
}

/*
 * check if a node has a child with the label <label>
 */
static int
node_has_child (GtkCTree *ctree, GtkCTreeNode *node, char *label)
{
	GtkCTreeNode *child;
	entry_t *en;

	child = GTK_CTREE_ROW(node)->children;
	while (child) {
		en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), child);
		if (strcmp (en->label, label) == 0) {
			return (1);
		}
		child = GTK_CTREE_ROW(child)->sibling;
	}
	return (0);
}

/*
 * update a node and its childs if visible
 * return 1 if some nodes have removed or added, else 0
 *
 */
static int
update_tree (GtkCTree *ctree, GtkCTreeNode *node)
{
	GtkCTreeNode *child, *new_child;
	entry_t *en, *child_en;
	struct dirent *de;
	DIR *dir;
	char compl[PATH_MAX+1];
	char label[NAME_MAX+1];
	int type, p_len, changed, tree_updated, root_changed;
	gchar size[16];
	gchar date[18];

	tree_updated = FALSE;
	en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
	if ((root_changed = entry_update (en)) == ERROR) {
		/* can't stat() the directory, is removed? */
		gtk_ctree_remove_node (ctree, node);
		return TRUE;
	}
#ifdef DEBUG
	printf ("update_tree() path=%s\n", en->path);
#endif
	child = GTK_CTREE_ROW(node)->children;
	while (child) {
/*		if (!gtk_ctree_node_is_visible (ctree, node)) {
			child = GTK_CTREE_ROW(child)->sibling);
			continue;
		}*/
		child_en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), child);
		if ((changed = entry_update (child_en)) == ERROR) {
			gtk_ctree_remove_node (ctree, child);
			/* realign the list */
			child = GTK_CTREE_ROW(node)->children;
			tree_updated = TRUE;
			continue;
		} else if (changed == TRUE) {
			/* update the labels */
#ifdef		DEBUG2
			printf (" updating %s size=%d ..\n",child_en->path, child_en->size);
#endif
			sprintf (date, "%02d-%02d-%02d %02d:%02d",
				child_en->date.year, child_en->date.month, child_en->date.day,
				child_en->date.hour, child_en->date.min);
			sprintf (size, "%10lu", (unsigned long)child_en->size);
			gtk_ctree_node_set_text (ctree, child, COL_DATE, date);
			gtk_ctree_node_set_text (ctree, child, COL_SIZE, size);
		}
		child = GTK_CTREE_ROW(child)->sibling;
	}
	if ((root_changed == TRUE) && (GTK_CTREE_ROW(node)->expanded)) {
		/* may be there are new files */
		dir = opendir (en->path);
		if (!dir) {
			perror (en->path);
			return (TRUE);
		}
		p_len = strlen (en->path);
		while ((de = readdir (dir)) != NULL) {
			if (io_is_hidden(de->d_name) && (en->flags & IGNORE_HIDDEN))
				continue;
			if (io_is_current(de->d_name))
				continue;
			/* it seems not to be a good idea to use de->d_name, so
			 * we copy it before */
			strcpy (label, de->d_name);
#ifdef		DEBUG_XWF
			printf (" looking for (%s)\n", label);
#endif
			if (!node_has_child (ctree, node, label)) {
				if (io_is_root(en->path))
					/* at the root directory we don't have to add a '/'
					 */
					sprintf (compl, "%s%s", en->path, label);
				else
					sprintf (compl, "%s/%s", en->path, label);
				type = 0;
				new_child = add_node (ctree, node,  NULL, label,
										compl, &type, en->flags);
				if ((type & FT_DIR) && (!(type & FT_DIR_UP)))
					add_subtree (ctree, new_child, compl, 1, en->flags);
				tree_updated = TRUE;
			}
		}
		closedir (dir);
		if (tree_updated) {
			gtk_ctree_sort_node (GTK_CTREE(ctree), node);
		}
	}
	return (tree_updated);
}

/*
 * check if a node is a directory and is visible and expanded
 * will be called for every node
 */
static void
get_visible_or_parent (GtkCTree *ctree, GtkCTreeNode *node, gpointer data)
{
	GtkCTreeNode *child;
	GList **list = (GList **)data;

	if (GTK_CTREE_ROW(node)->is_leaf)
		return;

	if (gtk_ctree_node_is_visible (ctree, node) &&
		GTK_CTREE_ROW(node)->expanded) {
		/* we can't remove a node or something else here,
		 * that would break the calling gtk_ctree_pre_recursive()
		 * so we remember the node in a linked list
		 */
		*list = g_list_append (*list, node);
		return;
	}

	/* check if at least one child is visible
	 */
	child = GTK_CTREE_ROW(node)->children;
	while (child) {
		if (gtk_ctree_node_is_visible(GTK_CTREE(ctree), child)) {
			*list = g_list_append (*list, node);
			return;
		}
		child = GTK_CTREE_ROW(child)->sibling;
	}
}
/*
 * timer function to update the view
 */
gint
update_timer (GtkCTree *ctree)
{
	GList *list = NULL, *tmp;
	GtkCTreeNode *node;
	GdkEvent *event;

	if (gtk_events_pending()) {
		event = gtk_get_current_event();
		if (event) {
			/* no time to update right now .. */
			gdk_event_free (event);
			return TRUE;
		}
	}
	/* get a list of directories we have to check
	 */
	gtk_ctree_post_recursive (ctree,
		GTK_CTREE_NODE(GTK_CLIST(ctree)->row_list),get_visible_or_parent,&list);

	tmp = list;
#ifdef DEBUG2
		printf ("update_timer() listsize=%d\n", g_list_length(list));
#endif
	while (tmp) {
		node = tmp->data;
		if (update_tree (ctree, node) == TRUE) {
			break;
		}
		tmp = tmp->next;
	}
	g_list_free (list);
	return (TRUE);
}

/*
 * menu callback for deleting files
 */
static void
cb_delete (GtkWidget *widget, GtkCTree *ctree)
{
	int num, i, pos = 0;
	entry_t *en;
	int result;
	GtkCTreeNode *node;
	int ask = TRUE;
	cfg *win;
	GList *list;
	char **argv;

	/* with alt+backspace we don't ask
	 */
	if (widget == GTK_WIDGET(ctree))
		ask = FALSE;
	win = gtk_object_get_user_data (GTK_OBJECT(ctree));

	list = GTK_CLIST(ctree)->selection;
	while (list) {
		en = gtk_ctree_node_get_row_data (ctree, GTK_CTREE_NODE(list->data));
		if (EN_IS_DIRUP(en)) {
			/* we do not process ".." */
			gtk_ctree_unselect (ctree, GTK_CTREE_NODE(list->data));
			list = GTK_CLIST(ctree)->selection;
		} else
			list = list->next;
	}
	num = count_selection (ctree, &node);
	if (!num) {
		/* nothing to delete */
		dlg_warning (_("No files marked!"));
		return;
	}
	list = GTK_CLIST(ctree)->selection;
	if (ask) {
		char buf[256];
		sprintf (buf, "[%d item(s) marked]", num);
		if (dlg_question(_("Delete selected item(s)?"),buf) == DLG_RC_CANCEL)
			return;
	}
	argv = (char **) malloc (sizeof (char *) * (2 + num + 1));
	if (!argv)
		return;
	argv[pos++] = PLUGINDIR"/xcp";
	argv[pos++] = "-mt"; /* trash */

	for (i = 0; i < num; i++) {
		if (!list) {
			printf ("FIXME!\n");
			continue;
		}
		en = gtk_ctree_node_get_row_data (ctree, GTK_CTREE_NODE(list->data));
		argv[pos++] = en->path;
#ifdef DEBUG_XWF
		printf ("cb_delete() (%s)\n", en->path);
#endif
		list = list->next;
	}
	argv[pos] = NULL;
	result = io_system_var (argv, pos);

	list = GTK_CLIST(ctree)->selection;
	while (list) {
		gtk_ctree_unselect (ctree, GTK_CTREE_NODE(list->data));
		list = GTK_CLIST(ctree)->selection;
	}
	while (gtk_events_pending ())
		gtk_main_iteration ();
	update_timer (GTK_CTREE(ctree));
}

/*
 * open find dialog
 */
void
cb_find (GtkWidget *item, GtkWidget *ctree)
{
	GtkCTreeNode *node;
	char cmd[PATH_MAX+1];
	entry_t *en;

	count_selection (GTK_CTREE(ctree), &node);
	en = gtk_ctree_node_get_row_data(GTK_CTREE(ctree), node);
	if (en && EN_IS_DIR(en))
		sprintf (cmd, "%s/xfi '%s' &", PLUGINDIR, en->path);
	else
		sprintf (cmd, "%s/xfi &", PLUGINDIR);
	io_system (cmd);
}

/*
 */
void
cb_about (GtkWidget *item, GtkWidget *ctree)
{
	dlg_info("This is xwf version "VERSION
				"\n(c) by Rasca\nPublished under GNU GPL"
				"\nhttp://home.pages.de/~rasca/xap/");
}

/*
 * set the title of the current window
 */
static void
set_title (GtkWidget *w, const char *path)
{
	char title[PATH_MAX+1+20];
	sprintf (title, "Xwf: %s", path);
	gtk_window_set_title (GTK_WINDOW(gtk_widget_get_toplevel(w)), title);
}

/*
 */
static void
go_to (GtkCTree *ctree, GtkCTreeNode *root, char *path, int flags)
{
	entry_t *en;
	int i;
	char *label[COLUMNS];
	GdkPixmap *icon[4];

#ifdef DEBUG
	printf ("go_to() path=%s\n", path);
#endif
	en = entry_new_by_path_and_label (path, path);
	if (!en) {
		printf (_("Can't find row data\n"));
		return;
	}
	if (!(en->type & FT_DIR))
		return;
	en->flags= flags;

	for (i = 0; i < COLUMNS; i++) {
		if (i == COL_NAME)
			label[i] = en->path;
		else
			label[i] = "";
	}
	ctree_freeze (ctree);
	gtk_ctree_remove_node (ctree, root);

	icon_find (en, getuid(), icon);
	root = gtk_ctree_insert_node (ctree, NULL, NULL, label, 8,
				icon[0], icon[1], icon[2], icon[3], FALSE, TRUE);
	gtk_ctree_node_set_row_data_full (ctree, root, en, node_destroy);
	add_subtree (ctree, root, en->path, 2, en->flags);
	ctree_thaw (ctree);
	set_title (GTK_WIDGET(ctree), en->path);
}

/*
 */
void
cb_go_to (GtkWidget *item, GtkCTree *ctree)
{
	GtkCTreeNode *node, *root;
	entry_t *en;
	char path[PATH_MAX+1], *p;
	int count;
	cfg_t *win;

	root = GTK_CTREE_NODE(GTK_CLIST(ctree)->row_list);
	count = count_selection (ctree, &node);
	en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
	win = gtk_object_get_user_data (GTK_OBJECT(ctree));

	if ((!count) || (count > 1) || (en && (!(en->type & FT_DIR)))) {
		if (en) {
			strcpy (path, en->path);
			if (!(en->type & FT_DIR)) {
				p = strrchr (en->path, G_DIR_SEPARATOR);
				if (p) {
					strncpy (path, en->path, p - en->path);
					path[p - en->path] = '\0';
				}
			}
		} else {
			strcpy (path, "/");
		}
		if (dlg_combo(_("Go to"), path, win->history) != DLG_RC_OK) {
			return;
		}
	} else {
		if (en)
			strcpy (path, en->path);
	}
#ifdef DEBUG
	printf ("cb_go_to() path=%s\n", path);
#endif
	if (en) {
		go_to (ctree, root, path, en->flags);
		win->history = history_add (win->history, path);
	}
}

/*
 */
void
cb_go_home (GtkWidget *item, GtkCTree *ctree)
{
	GtkCTreeNode *root;
	root = GTK_CTREE_NODE(GTK_CLIST(ctree)->row_list);
	go_to (ctree, root, getenv ("HOME"), IGNORE_HIDDEN);
}


/*
 * change root and go one directory up
 */
void
cb_go_up (GtkWidget *item, GtkCTree *ctree)
{
	entry_t *en;
	char path[PATH_MAX+1], *p;
	GtkCTreeNode *root;
	
	root = GTK_CTREE_NODE(GTK_CLIST(ctree)->row_list);
	en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), root);
	if (!en) {
		printf ("%s(%d): fatal error\n", __FILE__, __LINE__);
		return;
	}

	strcpy (path, en->path);
	p = strrchr (path, G_DIR_SEPARATOR);
	if (p == path) {
		if (!*(p+1))
			return;
		*(p+1) = '\0';
	} else
		*p = '\0';
#ifdef DEBUG
	printf ("new path=%s\n", path);
#endif
	go_to (ctree, root, path, en->flags);
}

#ifdef DEBUG_XWF2
#define return(x) printf(" return at line %d\n", __LINE__); return(x)
#endif
/*
 * start the marked program on double click
 */
static gint
on_double_click (GtkWidget *ctree, GdkEventButton *event, void *menu)
{
	GtkCTreeNode *node;
	entry_t *en, *parent;
	char cmd[(PATH_MAX+3)*2];
	char *wd;
	cfg_t *win;
	reg_t *prg;
	gint row, col;
#ifdef DEBUG_XWF
	printf ("on_double_click() type=%d\n", event->type);
#endif
	win = gtk_object_get_user_data (GTK_OBJECT(ctree));

	if ((event->type != GDK_2BUTTON_PRESS) || (event->button != 1)) {
		return (FALSE);
	}
	/* ok, double_click with left mouse button. let's go on.
	 */

	if ((event->x == 0) && (event->y == 0)) {
		/* user pressed RETURN, we don't have x,y so we
		 * use the current focus
		 */
		row = GTK_CLIST(ctree)->focus_row;
	} else {
		row = -1;
		gtk_clist_get_selection_info (GTK_CLIST(ctree), event->x,event->y,
					&row, &col);
	}
	/* check if the double click was over a directory
	 */
	if (row > -1) {
		node = gtk_ctree_node_nth (GTK_CTREE(ctree), row);
		en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
		if (EN_IS_DIR(en)) {
			/* Alt button is pressed it's the same as _go_to()..
			 */
			if ( (event->state & GDK_MOD1_MASK) ||
					( event->state & GDK_CONTROL_MASK) ||
					( event->x == 0 && event->y == 0) || EN_IS_DIR_UP(en) ) {
				win->history = history_add (win->history, en->path);
				go_to (GTK_CTREE(ctree),
					GTK_CTREE_NODE(GTK_CLIST(ctree)->row_list),
					uri_clear_path (en->path), en->flags);
				return (TRUE);
			} else {
				/* let the ctree take over this event
				 */
				return (FALSE);
			}
		}
	}
	if (! count_selection (GTK_CTREE(ctree), &node)) {
			printf ("no selection? FIXME?\n");
			return (TRUE);
		}
		if (! node) {
			printf ("FIXME?\n");
			return (TRUE);
		}
		en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);

		if (!(en->type & FT_FILE)) {
			return (FALSE);
		}
		/* call executable or open the file
		 */
		parent = gtk_ctree_node_get_row_data (GTK_CTREE(ctree),
				GTK_CTREE_ROW(node)->parent);
		gu_cursor_wait (GTK_WIDGET(ctree));

		wd = getcwd (NULL, PATH_MAX);
		chdir (parent->path);
		if (en->type & FT_EXE) /*io_can_exec (en->path))*/ {
			if (event->state & GDK_MOD1_MASK)
				sprintf (cmd, "%s -e \"%s\" &", TERMINAL, en->path);
			else
				sprintf (cmd, "\"%s\" &", en->path);
			io_system (cmd);
		} else {
			/* call open with dialog */
#ifdef DEBUG_XWF
			printf (" path=%s\n", en->path);
#endif
			win = gtk_object_get_user_data (GTK_OBJECT(ctree));
			prg = reg_prog_by_file (win->reg, en->path);
			if (prg) {
				if (prg->arg)
					sprintf (cmd, "\"%s\" %s \"%s\" &",
								prg->app, prg->arg, en->path);
				else
					sprintf (cmd, "\"%s\" \"%s\" &", prg->app, en->path);
				io_system (cmd);
			} else {
				char *p;
				p = mc_app_by_file (win->mreg, en->path);
				dlg_open_with (win->xap, p ? p : DEF_APP, en->path);
			}
		}
		chdir (wd);
		free (wd);
		gu_cursor_reset (GTK_WIDGET(ctree));
	return (TRUE);
}

/*
 */
int
get_number (GtkCList *ctree, GtkCTreeNode *node)
{
	int num = 0;
	GList *gl;
	gl = ctree->row_list;
	while (gl) {
		if (gl == &node->list)
			break;
		gl = gl->next;
		num++;
	}
	return num;
}

/*
 * handle keyboard short cuts
 */
static gint
on_key_press (GtkWidget *ctree, GdkEventKey *event, void *menu)
{
	int num, i, pos;
	entry_t *en;
	GtkCTreeNode *node;
	GdkEventButton evbtn;
	char cmd[PATH_MAX];

#ifdef DEBUG_XWF
	printf ("on_key_press(): %d state=%d\n", event->keyval, event->state);
	printf (" window %d", (int)event->window);
	printf (" send_event %d", (char)event->send_event);
	printf (" time %u\n", event->time);
#endif
	switch (event->keyval) {
		case GDK_Return:
			evbtn.type = GDK_2BUTTON_PRESS;
			evbtn.button = 1;
			evbtn.state = event->state;
			evbtn.x = 0;
			evbtn.y = 0;
			on_double_click (ctree, &evbtn, menu);
			return (TRUE);
			break;
		case GDK_BackSpace:
			if (event->state == GDK_MOD1_MASK) {
				cb_delete (ctree, GTK_CTREE(ctree));
			} else {
				/* go up */
				cb_go_up (ctree, GTK_CTREE(ctree));
			}
			return (TRUE);
			break;
		case GDK_F1:
			/* help */
			sprintf (cmd, "%s -e man xwf &", TERMINAL);
			system (cmd);
			return (TRUE);
			break;
		case GDK_Delete:
			cb_delete (gtk_widget_get_toplevel(ctree), GTK_CTREE(ctree));
			return (TRUE);
			break;
		default:
			if ((event->keyval >= GDK_space) && (event->keyval <= GDK_z) &&
				(event->state <= GDK_SHIFT_MASK)) {
				/* find next entry which starts with the key which was
				 * pressed
				 */
				int current_no = 0;

				if (GTK_CLIST(ctree)->selection) {
					node = GTK_CTREE_NODE(GTK_CLIST(ctree)->selection->data);
					current_no = get_number (GTK_CLIST(ctree), node);
					/* printf ("key: %d, state=%d num=%d\n",
							event->keyval, event->state, current_no); */
					current_no += 1;
				}
				num = g_list_length (GTK_CLIST(ctree)->row_list);
				for (i = 0 ; i < num; i++) {
					if ((num - i) <= current_no)
						pos = i - (num - current_no);
					else
						pos = i + current_no;
					node = gtk_ctree_node_nth (GTK_CTREE(ctree), pos);
					en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
					if (en->label && (*en->label == (char)event->keyval) &&
						gtk_ctree_is_viewable(GTK_CTREE(ctree), node)) {
#ifdef					DEBUG_XWF
						printf (" focus_row=%d\n", GTK_CLIST(ctree)->focus_row);
#endif
						GTK_CLIST(ctree)->focus_row = pos;
						gtk_ctree_unselect_recursive (GTK_CTREE(ctree), NULL);
						gtk_clist_moveto (GTK_CLIST(ctree), pos, COL_NAME, 0,0);
						gtk_clist_select_row (GTK_CLIST(ctree), pos, COL_NAME);
						break;
					}
				}
				return (TRUE);
			}
			break;
	}
	return (FALSE);
}

/*
 * create a new folder in the current
 */
void
cb_new_subdir (GtkWidget *item, GtkWidget *ctree)
{
	entry_t *en;
	GtkCTreeNode *node;
	char path[PATH_MAX+1];
	char label[PATH_MAX+1];
	char compl[PATH_MAX+1];

	count_selection (GTK_CTREE(ctree), &node);
	en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
	if (!(en->type & FT_DIR)) {
		node = GTK_CTREE_ROW(node)->parent;
		en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
	}

	if (!GTK_CTREE_ROW(node)->expanded)
		gtk_ctree_expand (GTK_CTREE(ctree), node);

	if (en->path[strlen(en->path)-1] == G_DIR_SEPARATOR)
		sprintf (path, "%s", en->path);
	else
		sprintf (path, "%s/", en->path);
	strcpy (label, _("New_Folder"));
	if (dlg_string(path, label) == DLG_RC_OK) {
		sprintf (compl, "%s%s", path, label);
		if (mkdir (compl, 0xFFFFFFFF) != -1)
			update_tree (GTK_CTREE(ctree), node);
		else dlg_error (compl, strerror(errno));
	}
}

/*
 * new file
 */
void
cb_new_file (GtkWidget *item, GtkWidget *ctree)
{
	entry_t *en;
	GtkCTreeNode *node;
	char path[PATH_MAX+1];
	char label[PATH_MAX+1];
	char compl[PATH_MAX+1];
	int tmp = 0, exists = 0;
	struct stat st;
	FILE *fp;

	count_selection (GTK_CTREE(ctree), &node);
	en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);

	if (!(en->type & FT_DIR)) {
		node = GTK_CTREE_ROW(node)->parent;
		en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
	}

	if (!GTK_CTREE_ROW(node)->expanded)
		gtk_ctree_expand (GTK_CTREE(ctree), node);

	if (en->path[strlen(en->path)-1] == G_DIR_SEPARATOR)
		sprintf (path, "%s", en->path);
	else
		sprintf (path, "%s/", en->path);
	strcpy (label, "New_File.c");
	if (dlg_string(path, label) == DLG_RC_OK) {
		sprintf (compl, "%s%s", path, label);
		if (stat (compl, &st) != -1) {
			if (dlg_question (_("File exists! Override?"),compl) !=DLG_RC_OK) {
				return;
			}
			exists = 1;
		}
		fp = fopen (compl, "w");
		if (!fp) {
			dlg_error(_("Can't create: "), compl);
			return;
		}
		fclose (fp);
		if (!exists)
		add_node (GTK_CTREE(ctree), node, NULL, label, compl,&tmp,en->flags);
	}
}

/*
 * create a new symbolic link
 */
void
cb_new_link (GtkWidget *item, GtkWidget *ctree)
{
	entry_t *en;
	GtkCTreeNode *node;
	char path[PATH_MAX+1];
	char label[PATH_MAX+1];
	char compl[PATH_MAX+1];
	char *curr, *p;
	int num, len;

	num = count_selection (GTK_CTREE(ctree), &node);
	if (!node || !num)
		return;
	en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
	if (!en)
		return;
	sprintf (label, "%s.alias", en->label);
	sprintf(path, _("Link name for '%s'"), en->label);
	if (dlg_string(path, label) == DLG_RC_OK) {
		/* go to base directory, cause we create relative links */
		curr = getcwd (NULL, 0);
		if (!curr)
			return;
		p = strrchr (en->path, G_DIR_SEPARATOR);
		if (!p) {
			fprintf (stderr, "error in %s:%d\n", __FILE__, __LINE__);
			return;
		}
		len = p - en->path + 1;
		strncpy (compl, en->path, len);
		compl[len] = '\0';
		chdir (compl);
		if (symlink (en->label, label) != 0) {
			dlg_error (_("Failed to create the link"), strerror(errno));
		}
		chdir (curr);
		free (curr);
	}
}

/*
 * duplicate a file or a directory (alt+d)
 */
void
cb_duplicate (GtkWidget *item, GtkCTree *ctree)
{
	entry_t *en;
	GtkCTreeNode *node;
	GList *selection;
	char nfile[PATH_MAX+1];
	struct stat s;
	char cmd[PATH_MAX*2];
	int num;

	gu_cursor_wait (GTK_WIDGET(ctree));

	for (selection = GTK_CLIST(ctree)->selection;
				selection; selection = selection->next) {

		node = selection->data;
		en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
		if (! (EN_IS_FILE(en) || EN_IS_DIR(en))) {
			printf ("%s is not a directory nor a file - skipped\n", en->path);
			continue;
		}
		/* find a new file name */
		num = 0;
		sprintf (nfile, "%s-%d", en->path, num++);
		while (stat(nfile, &s) != -1) {
			sprintf (nfile, "%s-%d", en->path, num++);
		}
		sprintf (cmd, "%s/xcp '%s' '%s' &", PLUGINDIR, en->path, nfile);
		system (cmd);
#ifdef DEBUG_XWF
		printf ("cb_duplicate() file=%s -> %s\n", en->path, nfile);
#endif
	}
	gu_cursor_reset(GTK_WIDGET(ctree));
}

/*
 * rename a file / directory
 */
void
cb_rename (GtkWidget *item, GtkCTree *ctree)
{
	entry_t *en;
	GtkCTreeNode *node;
	GdkPixmap *pix, *pim;
	guint8 spacing;
	char ofile[PATH_MAX+NAME_MAX+1];
	char nfile[PATH_MAX+NAME_MAX+1];
	char *p;
	struct stat st;

	if (!count_selection (ctree, &node)) {
		dlg_warning ("No item marked!");
		return;
	}
	en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
	if (en->type & FT_DIR_UP)
		return;
	sprintf (nfile, "%s", en->label);

	if (dlg_string(_("Rename to: "), nfile) == DLG_RC_OK) {
		if ((p=strchr (nfile, G_DIR_SEPARATOR)) != NULL) {
			p[1] = '\0';
			dlg_error (_("Character not allowed in filename"), p);
			return;
		}
#ifdef ADOUBLE
		if (strlen (nfile) > 31) {
			if (dlg_continue (_("Warning"), _("name has more then 31 characters!"))
				== DLG_RC_CANCEL) {
				return;
			}
		}
#endif
		sprintf (ofile, "%s", en->path);
		p = strrchr (ofile, G_DIR_SEPARATOR);
		p++;
		sprintf (p, "%s", nfile);
		strcpy (nfile, ofile);
		strcpy (ofile, en->path);
		if (lstat (nfile, &st) != ERROR) {
			if (dlg_question ("Override?", ofile) != DLG_RC_OK) {
				return;
			}
		}
		if (rename (ofile, nfile) == -1) {
			dlg_error(nfile, strerror(errno));
		} else {
#ifdef ADOUBLE
			char *adfileold = ad_file (en->path);
#endif
			g_free (en->path);
			g_free (en->label);
			en->path = g_strdup (nfile);
			p = strrchr (nfile, G_DIR_SEPARATOR);
			p++;
			en->label = g_strdup (p);
			gtk_ctree_get_node_info (ctree, node, NULL, &spacing, &pix, &pim,
					NULL, NULL, NULL, NULL);
			gtk_ctree_node_set_pixtext (ctree, node, 0, p, spacing, pix, pim);
#ifdef ADOUBLE
			if (stat (adfileold, &st) == 0) {
				/* rename .AppleDouble/file as well
				 */
				char *adfile = ad_file (en->path);
				if (adfile) {
					rename (adfileold, adfile);
					free (adfile);
				}
			}
			if (adfileold)
				free (adfileold);
#endif
		}
	}
}

/*
 * call the dialog "open with"
 */
void
cb_open_with (GtkWidget *item, GtkCTree *ctree)
{
	entry_t *en;
	cfg_t *win;
	GtkCTreeNode *node;
	GList *selection;
	char *prg;

	if (!count_selection (ctree, &node)) {
		dlg_warning ("No files marked!");
		return;
	}
	for (selection = GTK_CLIST(ctree)->selection;
				selection; selection = selection->next) {
		node = selection->data;
		en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
		win = gtk_object_get_user_data (GTK_OBJECT(ctree));
		if (EN_IS_DIR(en)) {
			new_top (uri_clear_path(en->path), win->xap, win->trash, win->reg,
						(mc_mime_reg_t *)win->mreg,
						win->width, win->height, en->flags);
		} else {
			prg = reg_app_by_file (win->reg, en->path);
			if (!prg)
				prg = mc_app_by_file (win->mreg, en->path);
			dlg_open_with (win->xap, prg ? prg : DEF_APP, en->path);
		}
	}
}

/*
 * call the dialog "properties"
 */
void
cb_props (GtkWidget *item, GtkCTree *ctree)
{
	entry_t *en;
	GtkCTreeNode *node;
	GList *selection;
	cfg_t *win;
	int num, i;
	char **cmd;

	if (!(num = count_selection (ctree, &node))) {
		dlg_warning ("No item marked!");
		return;
	}
	cmd = malloc (sizeof(char *) * (num + 2));
	if (!cmd) /* todo */
		return;
	cmd[0] = PLUGINDIR"/xat";
	i = 1;
	selection = GTK_CLIST(ctree)->selection;
	win = gtk_object_get_user_data (GTK_OBJECT(ctree));

	while (selection) {
		node = selection->data;
		en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
#ifdef DEBUG
		printf ("cp_props() entry=%s\n", en->path);
#endif
		cmd[i++] = en->path;
		selection = selection->next;
	}
	cmd[i] = NULL;
	io_system_var (cmd, num +1);
	free (cmd);
}

/*
 * ask user if he want to register a named suffix
 */
void
cb_register (GtkWidget *item, GtkWidget *ctree)
{
	GtkCTreeNode *node;
	char label[PATH_MAX+1];
	char path[PATH_MAX+1];
	char *sfx, *arg, *app;
	entry_t *en;
	cfg_t *win;
	GList *apps;
	reg_t *prog;

	if (!GTK_CLIST(ctree)->selection)
		return;
	node = GTK_CLIST(ctree)->selection->data;
	en = gtk_ctree_node_get_row_data(GTK_CTREE(ctree), node);
	win = gtk_object_get_user_data (GTK_OBJECT(ctree));

	sfx = strrchr (en->label, '.');
	if (!sfx) {
		if (dlg_continue(
			"Can't find suffix in filename, using complete filename",
				en->label) != DLG_RC_OK)
			return;
		sfx = en->label;
		sprintf (label, "Register program for file \"%s\"", sfx);
	} else {
		sprintf (label, "Register program for suffix \"%s\"", sfx);
	}
	prog = reg_prog_by_suffix (win->reg, sfx);
	if (!prog) {
		app = mc_app_by_file (win->mreg, en->label);
		arg = NULL;
	} else {
		app = prog->app;
		arg = prog->arg;
	}
	if (app) {
		if (arg) {
			sprintf (path, "%s %s", app, arg);
		} else {
			strcpy (path, app);
		}
	} else
		strcpy (path, DEF_APP);
	apps = reg_app_list (win->reg);
	if (dlg_combo (label, path, apps) == DLG_RC_OK) {
		if (*path) {
			if ((arg = strchr (path, ' ')) != NULL) {
				*arg++ = '\0';
				if (!*arg)
					arg = NULL;
			}
			win->reg = reg_add_suffix (win->reg, sfx, path, arg);
			reg_save (win->reg);
		}
	}
	g_list_free (apps);
}

/*
 * call xpg with the marked files as arguments
 */
void
cb_encrypt (GtkWidget *item, GtkWidget *ctree)
{
	GtkCTreeNode *node;
	int num, i;
	char **files;
	GList *selection;
	entry_t *en;

	if (!GTK_CLIST(ctree)->selection)
		return;
	num = count_selection (GTK_CTREE(ctree), &node);
	files = g_malloc (sizeof (char *) * num + 1);
	if (!files)
		return;
	i = 0;
	files[i++] = INSTDIR"/lib/xap/xpg";
	for (selection = GTK_CLIST(ctree)->selection;
				selection; selection = selection->next) {
		node = selection->data;
		en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
		files[i++] = en->path;
	}
	io_system_var (files, num+1);
	g_free (files);
}

/*
 */
void
on_destroy (GtkWidget *top, cfg_t *win)
{
#ifdef DEBUG
	printf ("on_destroy() trash=%s\n", win->trash);
#endif
	top_delete (top);
	gtk_timeout_remove (win->timer);
	history_save (win->history);
	g_free (win->trash);
	g_free (win->xap);
	g_free (win);

	if (!top_has_more ()) {
		gtk_main_quit ();
	}
}

/*
 * if window manager send delete event
 */
gint
on_delete (GtkWidget *w, GdkEvent *event, gpointer data)
{
#ifdef DEBUG
	printf ("on_delete()\n");
#endif
	if (top_length () == 1) {
		if (dlg_ask(_("Do you want to exit the Xwf program?")) == DLG_RC_OK)
			return (FALSE);
		return (TRUE);
	}
	return (FALSE);
}


/*
 * called if Alt+w was pressed
 */
void
cb_destroy (GtkWidget *top, void *data)
{
#ifdef DEBUG
	printf ("cb_destroy()\n");
#endif
	if (top_length () == 1) {
		if (dlg_ask(_("Do you want to exit the Xwf program?")) == DLG_RC_OK)
			gtk_widget_destroy ((GtkWidget *)data);
		return;
	}
	gtk_widget_destroy ((GtkWidget *)data);
}



/*
 */
static void
cb_exec (GtkWidget *top, gpointer data)
{
	cfg_t *win = (cfg_t *) data;
	dlg_execute (win->xap, NULL);
}

/*
 * create a new toplevel window
 */
static GtkWidget *
new_top (char *path, char *xap, char *trash, GList *reg, mc_mime_reg_t *mreg,
			int width, int height, int flags)
{
	GtkWidget *top, *box, *scrolled, *ctree, **menu, *menu_item;
	GtkCTreeNode *root;
	gchar *label[COLUMNS];
	gchar *titles[COLUMNS];
	entry_t *en;
	cfg_t *win;
	GtkAccelGroup *accel;
	GdkPixmap *icon[4];
	int i;
#define WINCFG	1
#define TOPWIN	2

	menu_entry dir_mlist[] = {
		{ NULL,				NULL,			0},
		{ _("Open in new"),	cb_new_window,	0, GDK_o, GDK_MOD1_MASK},
		{ NULL,				NULL,			0},
		{ _("New Folder .."),cb_new_subdir,	0, GDK_n, GDK_MOD1_MASK},
		{ _("New file .."),	cb_new_file,	0, GDK_k, GDK_MOD1_MASK},
		{ NULL,				NULL,			0},
		{ _("Rename .."),	cb_rename,		0, GDK_r, GDK_MOD1_MASK},
		{ _("Delete .."),	cb_delete,		0, GDK_x, GDK_CONTROL_MASK},
		{ NULL,				NULL,			0},
		{ _("Go to marked"),cb_go_to,		0, GDK_g, GDK_MOD1_MASK},
		{ _("Go up"),		cb_go_up,		0, GDK_u, GDK_MOD1_MASK},
		{ _("Unselect"),	cb_unselect,0,GDK_a, GDK_SHIFT_MASK|GDK_MOD1_MASK},
		{ _("Find .."),		cb_find,		0, GDK_f, GDK_MOD1_MASK},
		{_("Toggle Dotfiles"),on_dotfiles,0,GDK_D,GDK_SHIFT_MASK|GDK_MOD1_MASK},
		{ NULL,				NULL,			0},
		{ _("Properties .."),cb_props, 		0, GDK_i, GDK_MOD1_MASK},
		{ NULL,				NULL,			0},
		{ _("Execute .."),	cb_exec, 		WINCFG, GDK_r, GDK_CONTROL_MASK},
		{ _("Quit program"),gtk_main_quit, 	0, GDK_q, GDK_MOD1_MASK},
	};
	#define LAST_DIR_MENU_ENTRY (sizeof(dir_mlist)/sizeof(menu_entry))

	menu_entry file_mlist[] = {
		{ NULL,					NULL,			0},
		{ _("New window"),		cb_new_window,	0},
		{ NULL,					NULL,			0},
		{ _("Open with .."),	cb_open_with,	0, GDK_o, GDK_MOD1_MASK},
		{ _("Make Alias .."),	cb_new_link,	0, GDK_m, GDK_MOD1_MASK},
		{ NULL,					NULL,			0},
		{ _("Rename .."),		cb_rename,		0},
		{ _("Delete .."),		cb_delete,		0, GDK_Delete},
		{ _("Duplicate"),		cb_duplicate,	0, GDK_d, GDK_MOD1_MASK},
		{ NULL,					NULL,			0},
		{ _("Go up"),			cb_go_up,		0},
		{ _("Go home"),			cb_go_home,		0, GDK_h, GDK_MOD1_MASK},
		{ _("Unselect"),		cb_unselect,	0},
		{ NULL,					NULL,			0},
		{ _("Properties .."),	cb_props,		0},
		{ _("Register  .."),	cb_register,	0, GDK_j, GDK_MOD1_MASK},
		{ _("En-/Decrypt .."),	cb_encrypt,		0},
		{ NULL,					NULL,			0},
		{ _("Execute .."),		cb_exec, 		WINCFG},
		{ _("Close window"),	cb_destroy,TOPWIN, GDK_w, GDK_MOD1_MASK},
		{ _("Quit program"),	gtk_main_quit,	0, GDK_q, GDK_CONTROL_MASK},
	};
	#define LAST_FILE_MENU_ENTRY (sizeof(file_mlist)/sizeof(menu_entry))

	menu_entry mixed_mlist[] = {
		{ NULL,					NULL,			0},
		{ _("New windows"),		cb_new_window,	0},
		{ NULL,					NULL,			0},
		{ _("Delete .."),			cb_delete,	0},
		{ NULL,					NULL,			0},
		{ _("Go up"),				cb_go_up,	0},
		{ _("Go home"),			cb_go_home,		0},
		{ _("Unselect all"),		cb_unselect,0},
		{ _("Select all"),		cb_select,		0, GDK_a, GDK_MOD1_MASK},
		{ NULL,					NULL,			0},
		{ _("Empty Trash"),		cb_empty_trash,	0, GDK_e, GDK_MOD1_MASK},
		{ _("Open Trash"),			cb_open_trash,WINCFG, GDK_t, GDK_MOD1_MASK},
		{ NULL,					NULL,			0},
		{ _("Properties .."),		cb_props,	0},
		{ NULL,					NULL,			0},
		{ _("Execute .."),			cb_exec, 	WINCFG},
		{ _("About .."),			cb_about,	0, GDK_a, GDK_CONTROL_MASK},
		{ _("Close window"),		cb_destroy, 0},
		{ _("Quit program"),		gtk_main_quit,	0},
	};
	#define LAST_MIXED_MENU_ENTRY (sizeof(mixed_mlist)/sizeof(menu_entry))

	win = g_malloc (sizeof(cfg_t));
	win->mreg = mreg;
	win->dnd_row = -1;
	win->dnd_has_drag =0;
	win->history = history_init(path);
	menu = g_malloc (sizeof (GtkWidget) * MENUS);
	titles[COL_NAME] = _("Name");
	titles[COL_SIZE] = _("Size (byte)");
	titles[COL_DATE] = _("Last changed (D/T)");
	label[COL_NAME] = path;	/* ? */
	label[COL_SIZE] = "";
	label[COL_DATE] = "";

	top = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_name (top, "xwf");
	gtk_signal_connect (GTK_OBJECT(top), "destroy",
			GTK_SIGNAL_FUNC(on_destroy), (void *)win);
	gtk_signal_connect (GTK_OBJECT(top), "delete_event",
			GTK_SIGNAL_FUNC(on_delete), (void *)win);
	top_register (top);

	box = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER(top), box);

	win->scroll_w = scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX(box), scrolled, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolled),
			GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);

	ctree = gtk_ctree_new_with_titles (COLUMNS, 0, titles);
	gtk_clist_set_auto_sort (GTK_CLIST(ctree), FALSE);
#ifdef DEBUG
	printf ("new_top() ctree=0x%X\n", (int)ctree);
#endif

	accel = gtk_accel_group_new ();
	gtk_accel_group_attach (accel, GTK_OBJECT(top));

	gtk_widget_add_accelerator (GTK_WIDGET(
			GTK_CLIST(ctree)->column[COL_NAME].button), "clicked", accel,
			GDK_n, GDK_CONTROL_MASK|GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator (GTK_WIDGET(
			GTK_CLIST(ctree)->column[COL_DATE].button), "clicked", accel,
			GDK_d, GDK_CONTROL_MASK|GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator (GTK_WIDGET(
			GTK_CLIST(ctree)->column[COL_SIZE].button), "clicked", accel,
			GDK_s, GDK_CONTROL_MASK|GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

	win->compare = GTK_CLIST(ctree)->compare;
	win->trash = g_strdup (trash);
	win->xap = g_strdup (xap);
	win->reg = reg;
	win->width = width;
	win->height= height;
	gtk_object_set_user_data (GTK_OBJECT(ctree), win);

	gtk_clist_set_compare_func (GTK_CLIST(ctree), my_compare);
	gtk_clist_set_column_auto_resize (GTK_CLIST(ctree), 0, TRUE);
	gtk_clist_set_selection_mode (GTK_CLIST(ctree), GTK_SELECTION_EXTENDED);
	gtk_ctree_set_spacing (GTK_CTREE(ctree), 8);
	gtk_ctree_set_line_style (GTK_CTREE(ctree), GTK_CTREE_LINES_NONE);
	gtk_ctree_set_expander_style (GTK_CTREE(ctree),GTK_CTREE_EXPANDER_TRIANGLE);
	gtk_clist_set_row_height (GTK_CLIST(ctree), 16);
	gtk_container_add (GTK_CONTAINER(scrolled), ctree);
	gtk_widget_grab_focus (ctree);
	win->timer =gtk_timeout_add (TIMERVAL, (GtkFunction) update_timer, ctree);

	menu[MN_DIR] = gtk_menu_new ();
	gtk_menu_set_accel_group (GTK_MENU(menu[MN_DIR]), accel);

	for (i = 0; i < LAST_DIR_MENU_ENTRY; i++) {
		if (dir_mlist[i].label) {
			menu_item = gtk_menu_item_new_with_label (dir_mlist[i].label);
		} else
			menu_item = gtk_menu_item_new ();
		if (dir_mlist[i].func) {
			if (dir_mlist[i].data == WINCFG)
				gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
					GTK_SIGNAL_FUNC(dir_mlist[i].func), win);
			else
				gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
					GTK_SIGNAL_FUNC(dir_mlist[i].func), (void*)ctree);
		}
		if (dir_mlist[i].key) {
			gtk_widget_add_accelerator (menu_item, "activate", accel,
				dir_mlist[i].key, dir_mlist[i].mod, GTK_ACCEL_VISIBLE);
		}
		gtk_menu_append (GTK_MENU(menu[MN_DIR]), menu_item);
		gtk_widget_show (menu_item);
	}
	gtk_menu_attach_to_widget(GTK_MENU(menu[MN_DIR]),ctree,menu_detach);

	menu[MN_FILE] = gtk_menu_new ();
	for (i = 0; i < LAST_FILE_MENU_ENTRY; i++) {
		if (file_mlist[i].label)
			menu_item = gtk_menu_item_new_with_label (file_mlist[i].label);
		else
			menu_item = gtk_menu_item_new ();
		gtk_menu_append (GTK_MENU(menu[MN_FILE]), menu_item);
		if (file_mlist[i].func) {
			if (file_mlist[i].data == WINCFG)
				gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
					GTK_SIGNAL_FUNC(file_mlist[i].func), win);
			else if (file_mlist[i].data == TOPWIN)
				gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
					GTK_SIGNAL_FUNC(file_mlist[i].func), top);
			else
				gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
					GTK_SIGNAL_FUNC(file_mlist[i].func), (void*)ctree);
		}
		if (file_mlist[i].key) {
			gtk_widget_add_accelerator (menu_item, "activate", accel,
				file_mlist[i].key, file_mlist[i].mod, GTK_ACCEL_VISIBLE);
		}
		gtk_widget_show (menu_item);
	}
	gtk_menu_attach_to_widget(GTK_MENU(menu[MN_FILE]),ctree,menu_detach);

	menu[MN_MIXED] = gtk_menu_new ();
	for (i = 0; i < LAST_MIXED_MENU_ENTRY; i++) {
		if (mixed_mlist[i].label)
			menu_item = gtk_menu_item_new_with_label (mixed_mlist[i].label);
		else
			menu_item = gtk_menu_item_new ();
		gtk_menu_append (GTK_MENU(menu[MN_MIXED]), menu_item);
		if (mixed_mlist[i].func) {
			if (mixed_mlist[i].data == WINCFG)
				gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
					GTK_SIGNAL_FUNC(mixed_mlist[i].func), win);
			else
				gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
					GTK_SIGNAL_FUNC(mixed_mlist[i].func), (void*)ctree);
		}
		if (mixed_mlist[i].key) {
			gtk_widget_add_accelerator (menu_item, "activate", accel,
				mixed_mlist[i].key, mixed_mlist[i].mod, GTK_ACCEL_VISIBLE);
		}
		gtk_widget_show (menu_item);
	}
	gtk_menu_attach_to_widget(GTK_MENU(menu[MN_MIXED]),ctree,menu_detach);

	en = entry_new_by_path_and_label (path, path);
	if (!en) {
		fprintf (stderr, "Fatal error, %s:%d\n", __FILE__, __LINE__);
		exit (1);
	}
	en->flags= flags;
	icon_find (en, getuid(), icon);
	root = gtk_ctree_insert_node (GTK_CTREE(ctree), NULL, NULL, label, 8,
				icon[0], icon[1], icon[2], icon[3], FALSE, TRUE);

	gtk_ctree_node_set_row_data_full (GTK_CTREE(ctree), root, en, node_destroy);
	add_subtree (GTK_CTREE(ctree), root, path, 2, flags);

	gtk_drag_source_set (ctree, GDK_BUTTON1_MASK|GDK_BUTTON2_MASK,
			target_table, NUM_TARGETS,
			GDK_ACTION_COPY|GDK_ACTION_MOVE|GDK_ACTION_ASK|GDK_ACTION_PRIVATE);
	gtk_drag_dest_set (ctree, GTK_DEST_DEFAULT_ALL,
			target_table, NUM_TARGETS,
			GDK_ACTION_COPY|GDK_ACTION_MOVE|GDK_ACTION_ASK|GDK_ACTION_PRIVATE);

	/* DND target */
	gtk_signal_connect (GTK_OBJECT(ctree), "drag_data_received",
			GTK_SIGNAL_FUNC(on_drag_data), win);
	gtk_signal_connect (GTK_OBJECT(ctree), "drag_motion",
			GTK_SIGNAL_FUNC(on_drag_rcv_motion), win);
	gtk_signal_connect (GTK_OBJECT(ctree), "drag_leave",
			GTK_SIGNAL_FUNC(on_drag_rcv_leave), win);
/*	gtk_signal_connect (GTK_OBJECT(ctree), "drag_drop",
			GTK_SIGNAL_FUNC(on_drag_drop), &win->dnd_row);*/

	/* DND source */
	gtk_signal_connect (GTK_OBJECT(ctree), "drag_begin",
			GTK_SIGNAL_FUNC(on_drag_src_begin), win);
	gtk_signal_connect (GTK_OBJECT(ctree), "drag_data_get",
			GTK_SIGNAL_FUNC(on_drag_src_data_get), win);
	gtk_signal_connect (GTK_OBJECT(ctree), "drag_data_delete",
			GTK_SIGNAL_FUNC(on_drag_data_delete), win);

	gtk_signal_connect (GTK_OBJECT(ctree), "tree_expand",
			GTK_SIGNAL_FUNC(on_expand), path);
	gtk_signal_connect (GTK_OBJECT(ctree), "tree_collapse",
			GTK_SIGNAL_FUNC(on_collapse), path);
	gtk_signal_connect (GTK_OBJECT(ctree), "click_column",
			GTK_SIGNAL_FUNC(on_click_column), en);
	gtk_signal_connect_after (GTK_OBJECT(ctree), "button_press_event",
			GTK_SIGNAL_FUNC(on_double_click), root);
	gtk_signal_connect (GTK_OBJECT(ctree), "button_press_event",
			GTK_SIGNAL_FUNC(on_button_press), menu);
	gtk_signal_connect (GTK_OBJECT(ctree), "key_press_event",
			GTK_SIGNAL_FUNC(on_key_press), menu);

	set_title (top, en->path);
	if (win->width > 0 && win->height > 0) {
		gtk_window_set_default_size (GTK_WINDOW(top), width, height);
	}

	GTK_WIDGET_SET_FLAGS(ctree, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (ctree);
	gtk_widget_show_all (top);
	return (top);
}

/*
 * create pixmaps and create a new toplevel tree widget
 */
void
gui_main (char *path, char *xap_path, char *trash, char *reg_file, wgeo_t *geo,
			int flags)
{
	GList *reg;
	GtkWidget *top, *label, *new_win;
	mc_mime_reg_t *mime_reg;

	top = gtk_window_new (GTK_WINDOW_DIALOG);

	label = gtk_label_new (" Xwf is coming up ..");
	gtk_container_add (GTK_CONTAINER(top), label);
	gtk_window_position (GTK_WINDOW(top), GTK_WIN_POS_CENTER);
	gtk_widget_show_all (top);

	/* ensure it's up there .. we need this window for the
		pixmaps .. */
	while (gtk_events_pending ())
		gtk_main_iteration ();

	reg = reg_build_list (reg_file);
	mime_reg = mc_build ();
#ifdef DEBUG_MIME
	{
	mc_suffix_t *sfx;
	sfx = mime_reg->sfx;
	while (sfx) {
		if (sfx->mt) {
			if (sfx->mt->mc) {
				printf ("sfx=%s %s ->%s\n", sfx->sfx, sfx->mt->mimetype, sfx->mt->mc->view);
			} else {
				printf ("sfx=%s %s\n", sfx->sfx, sfx->mt->mimetype);
			}
		} else {
			printf ("sfx=%s\n", sfx->sfx);
		}
		sfx = sfx->next;
	}
	}
#endif

	icon_init (top);

	if (!io_is_directory(path)) {
		dlg_error(path, strerror(errno));
		return;
	}
	new_win = new_top (path, xap_path, trash, reg, mime_reg, geo->width,
				geo->height, flags);
	if (geo->x > -1 && geo->y > -1) {
		gtk_widget_set_uposition (new_win, geo->x, geo->y);
	}
	gtk_widget_unrealize (top);
	gtk_main ();
}

