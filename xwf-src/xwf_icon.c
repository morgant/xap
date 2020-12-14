/*
 * xwf_icon.c
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

#include <sys/stat.h>
#include <gtk/gtk.h>
#include "entry.h"
#include "xwf_icon.h"
#include "icons/i_page.xpm"
#include "icons/i_page_owner.xpm"
#include "icons/i_page_lnk.xpm"
#include "icons/i_page_write.xpm"
#include "icons/i_dir_pd.xpm"
#include "icons/i_dir_close.xpm"
#include "icons/i_dir_close_write.xpm"
#include "icons/i_dir_close_lnk.xpm"
#include "icons/i_dir_open.xpm"
#include "icons/i_dir_open_write.xpm"
#include "icons/i_dir_open_lnk.xpm"
#include "icons/i_dir_close_owner.xpm"
#include "icons/i_dir_close_lnk_owner.xpm"
#include "icons/i_dir_open_owner.xpm"
#include "icons/i_dir_open_lnk_owner.xpm"
#include "icons/i_dir_up.xpm"
#include "icons/i_dir_up_owner.xpm"
#include "icons/i_exe.xpm"
#include "icons/i_exe_owner.xpm"
#include "icons/i_exe_suid.xpm"
#include "icons/i_exe_suid_owner.xpm"
#include "icons/i_exe_sgid.xpm"
#include "icons/i_exe_sgid_owner.xpm"
#include "icons/i_exe_sugd.xpm"
#include "icons/i_exe_lnk.xpm"
#include "icons/i_exe_lnk_owner.xpm"
#include "icons/i_char_dev.xpm"
#include "icons/i_char_dev_owner.xpm"
#include "icons/i_fifo.xpm"
#include "icons/i_socket.xpm"
#include "icons/i_block_dev.xpm"
#include "icons/i_stale_lnk.xpm"

/* globals */
static GdkPixmap
	*gPIX_page,
	*gPIM_page,
	*gPIX_page_owner,
	*gPIM_page_owner,
	*gPIX_page_lnk,
	*gPIM_page_lnk,
	*gPIX_page_write,
	*gPIM_page_write,
	*gPIX_dir_pd,
	*gPIM_dir_pd,
	*gPIX_dir_close,
	*gPIM_dir_close,
	*gPIX_dir_close_lnk,
	*gPIM_dir_close_lnk,
	*gPIX_dir_close_write,
	*gPIM_dir_close_write,
	*gPIX_dir_open_lnk,
	*gPIM_dir_open_lnk,
	*gPIX_dir_open,
	*gPIM_dir_open,
	*gPIX_dir_open_write,
	*gPIM_dir_open_write,
	*gPIX_dir_close_owner,
	*gPIM_dir_close_owner,
	*gPIX_dir_close_lnk_owner,
	*gPIM_dir_close_lnk_owner,
	*gPIX_dir_open_lnk_owner,
	*gPIM_dir_open_lnk_owner,
	*gPIX_dir_open_owner,
	*gPIM_dir_open_owner,
	*gPIX_dir_up,
	*gPIM_dir_up,
	*gPIX_dir_up_owner,
	*gPIM_dir_up_owner,
	*gPIX_char_dev,
	*gPIM_char_dev,
	*gPIX_char_dev_owner,
	*gPIM_char_dev_owner,
	*gPIX_fifo,
	*gPIM_fifo,
	*gPIX_socket,
	*gPIM_socket,
	*gPIX_block_dev,
	*gPIM_block_dev,
	*gPIX_exe,
	*gPIM_exe,
	*gPIX_exe_owner,
	*gPIM_exe_owner,
	*gPIX_exe_suid,
	*gPIM_exe_suid,
	*gPIX_exe_suid_owner,
	*gPIM_exe_suid_owner,
	*gPIX_exe_sgid,
	*gPIM_exe_sgid,
	*gPIX_exe_sgid_owner,
	*gPIM_exe_sgid_owner,
	*gPIX_exe_sugd,
	*gPIM_exe_sugd,
	*gPIX_stale_lnk,
	*gPIM_stale_lnk,
	*gPIX_exe_lnk_owner,
	*gPIM_exe_lnk_owner,
	*gPIX_exe_lnk,
	*gPIM_exe_lnk;


/*
 * find the correct icon
 */
void
icon_find (entry_t *en, uid_t uid, GdkPixmap **icon)
{
	if (!icon)
		return;
#ifdef DEBUG_XWF2
	printf ("%s: icon_find()\n", __FILE__);
#endif
	icon[2] = icon[3] = NULL;
	if (EN_IS_DUMMY(en)) {
		icon[0] = icon[1] = NULL;
		return;
	}

	if (en->uid == uid) {
		/* we are the owner, so use other icon colors
		 */
		if (en->type & FT_EXE) {
			if (EN_IS_LINK(en)) {
				icon[0] = gPIX_exe_lnk_owner;
				icon[1] = gPIM_exe_lnk_owner;
			} else {
				if ((S_ISUID & en->mode) && (S_ISGID & en->mode)) {
					icon[0] = gPIX_exe_sugd;
					icon[1] = gPIM_exe_sugd;
				} else if (S_ISUID & en->mode) {
					icon[0] = gPIX_exe_suid_owner;
					icon[1] = gPIM_exe_suid_owner;
				} else if (S_ISGID & en->mode) {
					icon[0] = gPIX_exe_sgid_owner;
					icon[1] = gPIM_exe_sgid_owner;
				} else {
					icon[0] = gPIX_exe_owner;
					icon[1] = gPIM_exe_owner;
				}
			}
		} else if (en->type & FT_FILE) {
			if (en->type & FT_LINK) {
				icon[0] = gPIX_page_lnk;
				icon[1] = gPIM_page_lnk;
			} else {
				icon[0] = gPIX_page_owner;
				icon[1] = gPIM_page_owner;
			}
		} else if (en->type & FT_DIR_UP) {
			icon[0] = gPIX_dir_up_owner;
			icon[1] = gPIM_dir_up_owner;
		} else if (en->type & FT_DIR_PD) { /* private dir */
			icon[0] = gPIX_dir_pd;
			icon[1] = gPIM_dir_pd;
			icon[2] = gPIX_dir_pd;
			icon[3] = gPIM_dir_pd;
		} else if (en->type & FT_DIR) {
			if (en->type & FT_LINK) {
				icon[0] = gPIX_dir_close_lnk_owner;
				icon[1] = gPIM_dir_close_lnk_owner;
				icon[2] = gPIX_dir_open_lnk_owner;
				icon[3] = gPIM_dir_open_lnk_owner;
			} else {
				icon[0] = gPIX_dir_close_owner;
				icon[1] = gPIM_dir_close_owner;
				icon[2] = gPIX_dir_open_owner;
				icon[3] = gPIM_dir_open_owner;
			}
		} else if (en->type & FT_CHAR_DEV) {
			icon[0] = gPIX_char_dev_owner;
			icon[1] = gPIM_char_dev_owner;
		} else if (en->type & FT_BLOCK_DEV) {
			icon[0] = gPIX_block_dev;
			icon[1] = gPIM_block_dev;
		} else if (en->type & FT_FIFO) {
			icon[0] = gPIX_fifo;
			icon[1] = gPIM_fifo;
		} else if (en->type & FT_SOCKET) {
			icon[0] = gPIX_socket;
			icon[1] = gPIM_socket;
		} else {
			icon[0] = gPIX_stale_lnk;
			icon[1] = gPIM_stale_lnk;
		}
	} else {
		/* we are not the owner */
		if (en->type & FT_EXE) {
			if (EN_IS_LINK(en)) {
				icon[0] = gPIX_exe_lnk;
				icon[1] = gPIM_exe_lnk;
			} else {
				if ((S_ISUID & en->mode) && (S_ISGID & en->mode)) {
					icon[0] = gPIX_exe_sugd;
					icon[1] = gPIM_exe_sugd;
				} else if (S_ISUID & en->mode) {
					icon[0] = gPIX_exe_suid;
					icon[1] = gPIM_exe_suid;
				} else if (S_ISGID & en->mode) {
					icon[0] = gPIX_exe_sgid;
					icon[1] = gPIM_exe_sgid;
				} else {
					icon[0] = gPIX_exe;
					icon[1] = gPIM_exe;
				}
			}
		} else if (en->type & FT_FILE) {
			if (en->type & FT_LINK) {
				icon[0] = gPIX_page_lnk;
				icon[1] = gPIM_page_lnk;
			} else {
				if (en->type & FT_WRITE) {
					icon[0] = gPIX_page_write;
					icon[1] = gPIM_page_write;
				} else {
					icon[0] = gPIX_page;
					icon[1] = gPIM_page;
				}
			}
		} else if (en->type & FT_DIR_UP) {
			icon[0] = gPIX_dir_up;
			icon[1] = gPIM_dir_up;
		} else if (en->type & FT_DIR_PD) {
			icon[0] = gPIX_dir_pd;
			icon[1] = gPIM_dir_pd;
			icon[2] = gPIX_dir_pd;
			icon[3] = gPIM_dir_pd;
		} else if (en->type & FT_DIR) {
			if (en->type & FT_LINK) {
				icon[0] = gPIX_dir_close_lnk;
				icon[1] = gPIM_dir_close_lnk;
				icon[2] = gPIX_dir_open_lnk;
				icon[3] = gPIM_dir_open_lnk;
			} else {
				if (en->type & FT_WRITE) {
					icon[0] = gPIX_dir_close_write;
					icon[1] = gPIM_dir_close_write;
					icon[2] = gPIX_dir_open_write;
					icon[3] = gPIM_dir_open_write;
				} else {
					icon[0] = gPIX_dir_close;
					icon[1] = gPIM_dir_close;
					icon[2] = gPIX_dir_open;
					icon[3] = gPIM_dir_open;
				}
			}
		} else if (en->type & FT_CHAR_DEV) {
			icon[0] = gPIX_char_dev;
			icon[1] = gPIM_char_dev;
		} else if (en->type & FT_BLOCK_DEV) {
			icon[0] = gPIX_block_dev;
			icon[1] = gPIM_block_dev;
		} else if (en->type & FT_FIFO) {
			icon[0] = gPIX_fifo;
			icon[1] = gPIM_fifo;
		} else if (en->type & FT_SOCKET) {
			icon[0] = gPIX_socket;
			icon[1] = gPIM_socket;
		} else {
			icon[0] = gPIX_stale_lnk;
			icon[1] = gPIM_stale_lnk;
		}
	}
}


/*
 * create pixmaps
 */
void
icon_init (GtkWidget *top)
{
	GdkColor transparent;

	gPIX_page = gdk_pixmap_create_from_xpm_d (top->window, &gPIM_page,
					&transparent, i_page_xpm);
	gPIX_page_owner = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_page_owner, &transparent, i_page_owner_xpm);
	gPIX_page_lnk = gdk_pixmap_create_from_xpm_d (top->window, &gPIM_page_lnk,
					&transparent, i_page_lnk_xpm);
	gPIX_page_write = gdk_pixmap_create_from_xpm_d (top->window, &gPIM_page_write,
					&transparent, i_page_write_xpm);
	gPIX_dir_pd = gdk_pixmap_create_from_xpm_d (top->window, &gPIM_dir_pd,
					&transparent, i_dir_pd_xpm);
	gPIX_dir_open = gdk_pixmap_create_from_xpm_d (top->window, &gPIM_dir_open,
					&transparent, i_dir_open_xpm);
	gPIX_dir_open_lnk=gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_dir_open_lnk, &transparent, i_dir_open_lnk_xpm);
	gPIX_dir_open_write=gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_dir_open_write, &transparent, i_dir_open_write_xpm);
	gPIX_dir_close= gdk_pixmap_create_from_xpm_d (top->window, &gPIM_dir_close,
					&transparent, i_dir_close_xpm);
	gPIX_dir_close_lnk=gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_dir_close_lnk, &transparent, i_dir_close_lnk_xpm);
	gPIX_dir_open_owner = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_dir_open_owner,
					&transparent, i_dir_open_owner_xpm);
	gPIX_dir_open_lnk_owner=gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_dir_open_lnk_owner, &transparent,
					i_dir_open_lnk_owner_xpm);
	gPIX_dir_close_owner= gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_dir_close_owner,
					&transparent, i_dir_close_owner_xpm);
	gPIX_dir_close_lnk_owner=gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_dir_close_lnk_owner, &transparent,
					i_dir_close_lnk_owner_xpm);
	gPIX_dir_close_write=gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_dir_close_write, &transparent,
					i_dir_close_write_xpm);
	gPIX_dir_up = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_dir_up, &transparent, i_dir_up_xpm);
	gPIX_dir_up_owner = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_dir_up_owner, &transparent, i_dir_up_owner_xpm);
	gPIX_exe = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_exe, &transparent, i_exe_xpm);
	gPIX_exe_owner = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_exe_owner, &transparent, i_exe_owner_xpm);
	gPIX_exe_suid = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_exe_suid, &transparent, i_exe_suid_xpm);
	gPIX_exe_suid_owner = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_exe_suid_owner, &transparent, i_exe_suid_owner_xpm);
	gPIX_exe_sgid = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_exe_sgid, &transparent, i_exe_sgid_xpm);
	gPIX_exe_sgid_owner = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_exe_sgid_owner, &transparent, i_exe_sgid_owner_xpm);
	gPIX_exe_sugd = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_exe_sugd, &transparent, i_exe_sugd_xpm);
	gPIX_exe_lnk = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_exe_lnk, &transparent, i_exe_lnk_xpm);
	gPIX_exe_lnk_owner = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_exe_lnk_owner, &transparent, i_exe_lnk_owner_xpm);
	gPIX_char_dev = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_char_dev, &transparent, i_char_dev_xpm);
	gPIX_char_dev_owner = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_char_dev_owner, &transparent, i_char_dev_owner_xpm);
	gPIX_block_dev = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_block_dev, &transparent, i_block_dev_xpm);
	gPIX_fifo = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_fifo, &transparent, i_fifo_xpm);
	gPIX_socket = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_socket, &transparent, i_socket_xpm);
	gPIX_stale_lnk = gdk_pixmap_create_from_xpm_d (top->window,
					&gPIM_stale_lnk, &transparent, i_stale_lnk_xpm);
}

