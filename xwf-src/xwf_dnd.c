/*
 * xwf_dnd.c
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
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <time.h>
#include <gtk/gtk.h>
#include "xwf_cfg.h"
#include "xwf_dnd.h"
#include "xwf_misc.h"
#include "entry.h"
#include "uri.h"
#include "io.h"
#include "i18n.h"
#include "gtk_dlg.h"
#include "gtk_get.h"
#include "gtk_dnd.h"

#ifdef DEBUG_DND
#define DEBUG_DNDp(x) x
#else
#define DEBUG_DNDp(x)
#endif

static int dnd_file_ask (GtkWidget *parent, int time);
static int dnd_exec_ask (GtkWidget *parent, int time);

/*
 * DND source: called form drag source at startup of DND
 * not used until now..
 */
void
on_drag_src_begin (GtkWidget *tree, GdkDragContext *context, void *data)
{
	DEBUG_DNDp(
    printf ("on_drag_src_begin (w=0x%X, .. data=0x%X) as=%d sa=%d a=%d\n",
			(int)tree, (int)data,
            context->actions, context->suggested_action, context->action);)
	/* gdk_window_get_pointer (tree->window, &x, &y, 0); */
}

/*
 * what's that for?
 */
gboolean
on_drag_drop (GtkWidget *w, GdkDragContext *context, gint x, gint y,
				guint time, gpointer p)
{
	DEBUG_DNDp(
	printf ("on_drag_drop() targets=%s p=%d\n", context->targets?"yes":"no",
			*(int *)p);)

	return (FALSE);
}

/*
 * draw a frame arround the named row
 */
static void
draw_rectangle (GtkCList *clist, int row)
{
	gdk_draw_rectangle (clist->clist_window,
			clist->xor_gc, FALSE,
			0, ROW_TOP_YPIXEL(clist, row), 
			clist->clist_window_width - 1,
			clist->row_height - 1);
	DEBUG_DNDp(printf ("draw_rectangle (0x%X, row=%d)\n", (int)clist, row);)
}

/*
 * DND receiver: called if the receiver gets a drag_leave-event
 */
void
on_drag_rcv_leave (GtkWidget *tree, GdkDragContext *context, guint time,void *p)
{
    GtkCList *clist = GTK_CLIST(tree);
	cfg_t *win = (cfg_t *)p;

	DEBUG_DNDp(
     printf ("on_drag_rcv_leave() w=0x%X, p=0x%X\n", (int)tree, win->dnd_row);)
	if (win->dnd_row >=0) {
		/* remove old mark */
		DEBUG_DNDp(printf (" ** removing rectangle ..\n");)
		draw_rectangle (clist, win->dnd_row);
	}
	win->dnd_has_drag = 0;
}

/*
 * DND receiver: called during motion
 * return value?
 */
gboolean
on_drag_rcv_motion (GtkWidget *dest, GdkDragContext *context,
				gint x, gint y, guint etime, void *data)
{
	GtkCList *clist = GTK_CLIST(dest);
	gint row, col;
	GtkCTreeNode *node;
	entry_t *entry;
	cfg_t *win = (cfg_t *)data;


	row = col = -1;

	if (y <= clist->column_title_area.height) {
		/* scroll up the "scrolled window"
		 */
		/* doesn't work! */
		GdkEventButton event;
		event.window =
			GTK_RANGE(
			GTK_SCROLLED_WINDOW(
				win->scroll_w)->vscrollbar) ->step_back;
		event.x = event.y = 1;
		event.send_event = TRUE;
		event.button = 1;
		event.state = 0;

		event.type = GDK_BUTTON_PRESS;
		event.time = gdk_time_get();
		gdk_event_put ((GdkEvent *)&event);

		event.type = GDK_BUTTON_RELEASE;
		event.time = gdk_time_get();
		/* gdk_event_put ((GdkEvent *)&event); */
		return (TRUE);
	}
	y -= clist->column_title_area.height;
	gtk_clist_get_selection_info (clist, x, y, &row, &col);

	DEBUG_DNDp(
		printf ("on_drag_rcv_motion (0x%X .. x=%d y=%d .. 0x%X) "
			"row=%d col=%d sa=%d)\n",
			(int)dest, x, y, (int)data, row, col, context->suggested_action);)

	/* initialize row pointer */
	if (!win->dnd_has_drag) {
		win->dnd_row = -1;
	}

	DEBUG_DNDp(printf ("  ** row=%d dnd_row=%d\n", row, win->dnd_row);)

	/* check for the target row
	 */
	if (row < 0)
		row = 0;
	do {
		node = gtk_ctree_node_nth (GTK_CTREE(dest), row);
		entry= gtk_ctree_node_get_row_data (GTK_CTREE(dest), node);
		/* check if target is ok
		 */
		if (!(
			(EN_IS_DIR(entry) && access (entry->path, W_OK|X_OK) == 0) ||
			(EN_IS_EXE(entry) && access (entry->path, X_OK) == 0)
			)) {
			/* target is not ok -> find next parent node
			 */
			node = GTK_CTREE_ROW (node)->parent;
			entry= gtk_ctree_node_get_row_data (GTK_CTREE(dest), node);
			row = g_list_position (GTK_CLIST(dest)->row_list, (GList *)node);
		} else {
			/* destination found
			 */
			break;
		}
	} while (row > 0);

	if (row == win->dnd_row) {
		/* moved inside the same row, nothing to do
		 */
		return (TRUE);
	}
	DEBUG_DNDp(
		printf("  -- drop target-path=%s, win->dnd_row=%d\n",
		entry->path,win->dnd_row);)

	/* remove old mark
	 */
	if (win->dnd_row >= 0 && (win->dnd_row != row)) {
		DEBUG_DNDp(
			printf ("  ** removing rect around row %d (focus=%d)\n",
						win->dnd_row, clist->focus_row);)
		if (clist->focus_row == win->dnd_row) {
			clist->focus_row = row;
			draw_rectangle (clist, win->dnd_row);
		}
		/* still buggy */
	}
	/*
	 */
	win->dnd_row = row;
	clist->focus_row = row;
	DEBUG_DNDp (printf ("  ** drawing rect around row %d\n", win->dnd_row);)
	draw_rectangle (clist, win->dnd_row);
	win->dnd_has_drag = 1;
	/* gdk_drag_status (context, context->suggested_action, etime); */
	return (TRUE);
}


/*
 * DND receiver: called if drop data will be received
 * signal: drag_data_received
 */
void
on_drag_data (GtkWidget *ctree, GdkDragContext *context, gint x, gint y,
				GtkSelectionData *data, guint info, guint dndtime, void *client)
{
	cfg_t *win = (cfg_t *)client;
	GList *list = NULL, *t;
	entry_t *target;
	GtkCTreeNode *node;
	uri_t *u;
	int action, i;
	unsigned long nitems = 0;
	char msg[DLG_MAX], *string = NULL;
	char **args, *xcp_opt = NULL;

#ifdef DEBUG_DND
	printf ("on_drag_data() data received, info=%d a=%d as=%d sa=%d row=%d\n",
			info, context->action, context->actions,
			context->suggested_action, win->dnd_row);
#endif
	win->dnd_has_drag = 0;

	if ((data->length <= 0) || (data->format != 8)) {
		gtk_drag_finish (context, FALSE, TRUE, dndtime);
		return;
	}
	if (y <= GTK_CLIST(ctree)->column_title_area.height) {
		/* cancel DNDs on the column headers
		 */
		gtk_drag_finish (context, FALSE, TRUE, dndtime);
		return;
	}
	/* find target row
	 */
	if (win->dnd_row < 0) {
		win->dnd_row = 0;
	}
	action = context->action <= GDK_ACTION_DEFAULT ?
				GDK_ACTION_COPY : context->action;
	/* find target
	 */
	node = gtk_ctree_node_nth (GTK_CTREE(ctree), win->dnd_row);
	target = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
	if (!target ) {
		perror ("Fatal Error, entry not found");
		return;
	}

	switch (info) {
		case TARGET_XWF_LIST:
		case TARGET_URI_LIST:
		case TARGET_PLAIN:
		case TARGET_STRING:
			nitems = uri_parse_list (data->data, &list);
			/* remove the "file://" stuff
			 */
			uri_remove_file_prefix_from_list (list);
			DEBUG_DNDp(printf (" items=%ld\n",nitems);)
			break;
		default:
			break;
	}

	if (nitems == 0) {
		gtk_drag_finish (context, FALSE, TRUE, dndtime);
		return;
	} else if ((nitems > 1) &&
			(! (EN_IS_DIR(target) || EN_IS_EXE(target)) )) {
			dlg_error (_("Target must be a directory!"), NULL);
			gtk_drag_finish (context, FALSE, TRUE, dndtime);
			uri_free_list (list);
			return ;
	}

	if (action == GDK_ACTION_ASK) {
		if (EN_IS_EXE(target))
			action = dnd_exec_ask (ctree, time(NULL));
		else
			action = dnd_file_ask (ctree, time(NULL));
	}

	switch (action) {
		case GDK_ACTION_COPY:
			string = _("copy");
			xcp_opt = "-mc";
			break;
		case GDK_ACTION_PRIVATE:
			string = _("copy");
			xcp_opt = "-p";
			break;
		case GDK_ACTION_MOVE:
			string = _("move");
			xcp_opt = "-mm";
			break;
		case GDK_ACTION_LINK:
			string = _("link");
			xcp_opt = "-ml";
			break;
		default:
			fprintf (stderr,
				"on_drag_data() action not supported (=%d)\n",
				action);
			gtk_drag_finish (context, FALSE, TRUE, dndtime);
			uri_free_list (list);
			return;
			break;
	}
	gtk_drag_finish (context, TRUE, TRUE, dndtime);

	switch (info) {
		case TARGET_XWF_LIST:
			args = g_malloc (sizeof(char*) * (nitems + 1 + 2));
			if (EN_IS_EXE(target)) {
				args[0] = target->path;
				for (i = 0; i < nitems; i++) {
					args[i+1] = ((uri_t *)(g_list_nth (list, i)->data))->url;
				}
				args[i+1] = target->path;
				if (io_system_var (args, nitems+1) != 0)
					perror (target->path);
				uri_free_list (list);
			} else {
				args[0] = PLUGINDIR"/xcp";
				args[1] = xcp_opt;
				for (i = 0; i < nitems; i++) {
					args[i+2] = ((uri_t *)(g_list_nth (list, i)->data))->url;
				}
				args[i+2] = target->path;
				if (io_system_var (args, nitems+3) != 0)
					perror ("xcp");
				uri_free_list (list);
			}
			g_free (args);
			break;

		case TARGET_PLAIN:
		case TARGET_STRING:
		case TARGET_URI_LIST:
			if (context->action != GDK_ACTION_ASK) {
				sprintf (msg, _("Do you want to %s the item(s) to?"), string);
				if (dlg_question (msg, target->path) != DLG_RC_OK) {
					uri_free_list (list);
					return;
				}
			}
			t = list;
			while (t) {
				u = t->data;
			DEBUG_DNDp( printf (" wish: %s -> %s (type=%d)\n",
					u->url, target->path, u->type);)

				if ((u->type & URI_FTP) || (u->type & URI_HTTP)) {
					if (!download (u, target->path))
						break;
				} else if (u->type == URI_LOCAL) {
					/* printf (" %s -> %s\n", u->url, en->path); */
					args = g_malloc (sizeof(char*) * (1 + 1 + 2));
					args[0] = PLUGINDIR"/xcp";
					args[1] = xcp_opt;
					args[2] = ((uri_t *)t->data)->url;
					args[3] = target->path;
					if (io_system_var (args, 1+3) != 0)
						perror ("xcp");
					g_free (args);
				} else {
					fprintf (stderr, "type not supported.. (%d)\n", u->type);
				}
				t = t->next;
			}
			uri_free_list (list);
			break;
	}
}

/*
 * DND source: prepare data for the receiver.
 * event: drag_data_get
 */
void
on_drag_src_data_get (GtkWidget *widget, GdkDragContext *context,
					GtkSelectionData *selection_data,
					guint info,
					guint time,
					gpointer data)
{
	GtkCTreeNode *node;
	GtkCTree *ctree = GTK_CTREE(widget);
	GList *selection;
	entry_t *en;
	unsigned long num;
	int i, len, slen, hostname_len;
	const char *hostname;
	gchar *files;
	cfg_t *win = (cfg_t *)data;


	num = g_list_length (GTK_CLIST(ctree)->selection);
	DEBUG_DNDp(
		printf ("on_drag_src_data_get (w=0x%X) info=%d num=%ld\n",
			(int)widget, info, num);)
	if (!num) {
		node = GTK_CTREE_NODE(GTK_CLIST(ctree)->row_list);
	} else {
		node = GTK_CTREE_NODE(GTK_CLIST(ctree)->selection->data);
	}

	/* prepare data for the receiver
	 */
	switch (info) {
		case TARGET_ROOTWIN:
			/* not implemented */
			printf ("root drop not handled until now\n");
			win->dnd_data = NULL;
			break;
		case TARGET_STRING:
		case TARGET_PLAIN:
			selection = GTK_CLIST(ctree)->selection;
			for (len = 0, i = 0; i < num; i++) {
				node = selection->data;
				en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
				len += strlen (en->path) + 1;
				selection = selection->next;
			}
			win->dnd_data = files = g_malloc (len+1);
			files[0] = '\0';
			selection = GTK_CLIST(ctree)->selection;
			for (i = 0; i < num; i++) {
				node = selection->data;
				en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
				strcat (files, en->path);
				if ((info != TARGET_STRING) || (num > 1))
					strcat (files, "\n");
				selection = selection->next;
			}
			gtk_selection_data_set (selection_data,
					selection_data->target, 8, files, len);
			break;
		case TARGET_XWF_LIST:
			hostname = uri_hostname();
			hostname_len = strlen (hostname);

			selection = GTK_CLIST(ctree)->selection;
			for (len = 0, i = 0; i < num; i++) {
				node = selection->data;
				en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
				len += strlen (en->path) + 7 + 2 + hostname_len;
				selection = selection->next;
			}
			win->dnd_data = files = g_malloc (len+1);
			files[0] = '\0';
			selection = GTK_CLIST(ctree)->selection;
			for (i = 0; i < num; i++) {
				node = selection->data;
				en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
				slen = strlen (en->path);
				sprintf (files, "file://%s%s\x0D\x0A", hostname, en->path);
				files += slen + 7 + 2 + hostname_len;
				selection = selection->next;
			}
			gtk_selection_data_set (selection_data,
					selection_data->target, 8, win->dnd_data, len);
			break;
		case TARGET_URI_LIST:
			selection = GTK_CLIST(ctree)->selection;
			for (len = 0, i = 0; i < num; i++) {
				node = selection->data;
				en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
				len += strlen (en->path) + 5 + 2;
				selection = selection->next;
			}
			win->dnd_data = files = g_malloc (len+1);
			files[0] = '\0';
			selection = GTK_CLIST(ctree)->selection;
			for (i = 0; i < num; i++) {
				node = selection->data;
				en = gtk_ctree_node_get_row_data (GTK_CTREE(ctree), node);
				slen = strlen (en->path);
				sprintf (files, "file:%s\x0D\x0A", en->path);
				DEBUG_DNDp(printf (" prepared: %s", files);)
				files += slen + 5 + 2;
				selection = selection->next;
			}
			gtk_selection_data_set (selection_data,
					selection_data->target, 8, win->dnd_data, len);
			break;
		default:
			printf ("%s:%d TARGET=%d: FIXME!\n", __FILE__, __LINE__, info);
			break;
	}
#ifdef DEBUG_DND
	printf (" saving data at 0x%X\n", (int)win->dnd_data);
#endif
}

/*
 */
void
on_drag_data_delete (GtkWidget *widget, GdkDragContext *context, gpointer *data)
{
	cfg_t *win = (cfg_t *)data;

#ifdef DEBUG_DND
	printf ("on_drag_data_delete() deleting memory at 0x%X\n",
			(int)win->dnd_data);
#endif
	g_free (win->dnd_data);
}



/* dnd dialog
 */
static int result;
/*
 */
static void
menu_detach()
{
#ifdef DEBUG_XWF
	printf ("menu_detach()\n");
#endif
}

/*
 */
static void
cb_done (GtkWidget *w, void *data)
{
	result = (int)data;
	gtk_main_quit ();
}

/*
 */
static int
dnd_file_ask (GtkWidget *parent, int dndtime)
{
	GtkWidget *menu, *menu_item;

	result = DND_NONE;
	menu = gtk_menu_new();
	gtk_signal_connect (GTK_OBJECT(menu),
				"hide", GTK_SIGNAL_FUNC(cb_done), (void *)0);

	menu_item = gtk_menu_item_new_with_label (_("Move"));
	gtk_signal_connect (GTK_OBJECT(menu_item),
				"activate", GTK_SIGNAL_FUNC(cb_done), (void *)GDK_ACTION_MOVE);
	gtk_menu_append (GTK_MENU(menu), menu_item);

	menu_item = gtk_menu_item_new_with_label (_("Copy"));
	gtk_signal_connect (GTK_OBJECT(menu_item),
				"activate", GTK_SIGNAL_FUNC(cb_done), (void *)GDK_ACTION_COPY);
	gtk_menu_append (GTK_MENU(menu), menu_item);

	menu_item = gtk_menu_item_new_with_label (_("Copy (preserve)"));
	gtk_signal_connect (GTK_OBJECT(menu_item),
				"activate",GTK_SIGNAL_FUNC(cb_done),(void *)GDK_ACTION_PRIVATE);
	gtk_menu_append (GTK_MENU(menu), menu_item);

	menu_item = gtk_menu_item_new_with_label (_("Link"));
	gtk_signal_connect (GTK_OBJECT(menu_item),
				"activate", GTK_SIGNAL_FUNC(cb_done), (void *)GDK_ACTION_LINK);
	gtk_menu_append (GTK_MENU(menu), menu_item);

	gtk_menu_attach_to_widget (GTK_MENU(menu), parent, menu_detach);

	gtk_widget_show_all (menu);
	gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, 1, dndtime);
	gtk_main();
	gtk_widget_destroy (menu);
	return (result);
}

/*
 */
static int
dnd_exec_ask (GtkWidget *parent, int dndtime)
{
	GtkWidget *menu, *menu_item;

	result = DND_NONE;
	menu = gtk_menu_new();
	gtk_signal_connect (GTK_OBJECT(menu),
				"hide", GTK_SIGNAL_FUNC(cb_done), (void *)0);

	menu_item = gtk_menu_item_new_with_label (_("Execute"));
	gtk_signal_connect (GTK_OBJECT(menu_item),
				"activate", GTK_SIGNAL_FUNC(cb_done), (void *)GDK_ACTION_COPY);
	gtk_menu_append (GTK_MENU(menu), menu_item);

	gtk_menu_attach_to_widget (GTK_MENU(menu), parent, menu_detach);

	gtk_widget_show_all (menu);
	gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, 1, dndtime);
	gtk_main();
	gtk_widget_destroy (menu);
	return (result);
}

