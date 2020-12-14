/*
 * gtk_util.c
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
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "i18n.h"
#include "gtk_util.h"

/*
 * change cursor to indicate busyness
 */
void
gu_cursor_wait (GtkWidget *widget)
{                              
    GdkCursor *cursor;
    cursor = gdk_cursor_new (GDK_WATCH);
    gdk_window_set_cursor (widget->window, cursor);
    gdk_flush ();
    gdk_cursor_destroy (cursor);
}

