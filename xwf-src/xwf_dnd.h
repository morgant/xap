/*
 * xwf_dnd.h
 *
 * Copyright (C) 1998 Rasca, Berlin
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

#ifndef __XWF_DND_H__
#define __XWF_DND_H__

#define ROW_TOP_YPIXEL(clist,row) (((clist)->row_height*(row))+\
	(((row))+1)*1+(clist)->voffset)

void on_drag_rcv_leave (GtkWidget *tree,GdkDragContext *con,guint time,void *p);
gboolean on_drag_drop (GtkWidget *, GdkDragContext *,gint, gint, guint,
			gpointer);
gboolean on_drag_rcv_motion (GtkWidget *w, GdkDragContext *c, gint x, gint y,
			guint time, void *data);
void on_drag_data (GtkWidget *ctree, GdkDragContext *context, gint x, gint y,
			GtkSelectionData *data, guint info, guint itme, void *client);

void on_drag_src_begin (GtkWidget *ctree, GdkDragContext *context, void *data);
void on_drag_src_data_get (GtkWidget *widget, GdkDragContext *context,
			GtkSelectionData *selection_data, guint info,
			guint time, gpointer data);
void on_drag_data_delete (GtkWidget *widget, GdkDragContext *context,
			gpointer *data);

#endif
