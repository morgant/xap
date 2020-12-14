/*
 * callbacks.c
 *
 * Copyright (C) 2000 Rasca, Berlin
 * EMail: thron at gmx.de
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <stdio.h>	/* sprintf() */
#include <time.h>	/* strftime() */
#include <string.h>	/* strstr() */
#include <sys/stat.h>	/* mode_t macros */
#include <pwd.h>	/* getpwuid() */
#include <grp.h>	/* getgrgid() */
#include <unistd.h>	/* getuid() */
#include <dirent.h>	/* opendir(), .. */
#include <errno.h>	/* errno */
#include <stdlib.h>	/* free() */

#include <gtk/gtk.h>

#include "callbacks.h"
#include "gui.h"
#include "support.h"
#include "entry.h"
#include "mailcap.h"
#include "i18n.h"
#include "gtk_dlg.h"
#include "io.h"
#include "adouble.h"
#include "gtk_util.h"

extern GList *gList; /* file list */
extern mc_mime_reg_t *gMreg; /* registry */

#define CH_OWNER	1
#define CH_GROUP	2
#define CH_USERS	(CH_OWNER | CH_GROUP)
#define PERMS_SET	4
#define PERMS_ADD	8
#define PERMS_REM	16
#define CH_PERMS	(PERMS_SET | PERMS_ADD | PERMS_REM)
#define NESTED		32
#define INTERACTIVE	64

/* globals for this module
 */
static int gmStatus = 1;
static GtkWidget *gmNotebook = NULL;
static GtkWidget *gmStatusBar = NULL;

/*
 * user clicked on check button "change permissions"
 */
void
on_cb_changeperms_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *w;
	int toggle;

	toggle = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(button));

	w = lookup_widget (GTK_WIDGET(button), "frame_op");
	gtk_widget_set_sensitive (w, toggle);
	w = lookup_widget (GTK_WIDGET(button), "lbl_i_owner");
	gtk_widget_set_sensitive (w, toggle);
	w = lookup_widget (GTK_WIDGET(button), "lbl_i_group");
	gtk_widget_set_sensitive (w, toggle);
	w = lookup_widget (GTK_WIDGET(button), "lbl_i_other");
	gtk_widget_set_sensitive (w, toggle);
}

/*
 */
static int
process_file (entry_t *entry, int flags, mode_t mode, uid_t uid, gid_t gid)
{
#ifdef DEBUG_XAT
	printf ("process_file(%s)\n", entry->path);
#endif
	if (EN_IS_LINK(entry)) /* skip symbolic links */
		return 1;
	gtk_statusbar_push (GTK_STATUSBAR(gmStatusBar), 1, entry->path);
	while (gtk_events_pending())
		gtk_main_iteration();
	if (!gmStatus)
		return 0;
	
	/* first change owner */
	if (flags & CH_USERS) {
		if (chown (entry->path, uid, gid) != 0) {
			if (dlg_continue(entry->path, strerror (errno)) == DLG_RC_CANCEL) {
				return 0;
			}
		}
	}
	if (flags & CH_PERMS) {
		int rc;
		if (flags & PERMS_SET)
			rc = chmod (entry->path, mode);
		else if (flags & PERMS_ADD)
			rc = chmod (entry->path, entry->mode | mode);
		else
			rc = chmod (entry->path, entry->mode & ~mode);
		if (rc != 0) {
			if (dlg_continue(entry->path, strerror (errno)) == DLG_RC_CANCEL) {
				return 0;
			}
		}
	}
	return 1;
}

/*
 * this is also called recursive if requested!
 */
static int
process_dir (entry_t *entry, int flags, mode_t mode, uid_t uid, gid_t gid)
{
	DIR *dir;
	struct dirent *de;
	char *file;
	int len;
	entry_t *new_entry;
#ifdef DEBUG_XAT
	printf ("process_dir (%s, %d, ..)\n", entry->path, flags);
#endif
	if (EN_IS_LINK(entry)) /* skip symbolic links */
		return 1;
	gtk_statusbar_push (GTK_STATUSBAR(gmStatusBar), 1, entry->path);
	while (gtk_events_pending())
		gtk_main_iteration();
	if (!gmStatus)
		return 0;

	if (flags & NESTED) {
		dir = opendir (entry->path);
		if (!dir) {
			dlg_error (entry->path, strerror(errno));
		} else {
			len = strlen (entry->path);
			while ((de = readdir(dir)) != NULL) {
				if (io_is_current(de->d_name) || io_is_dirup(de->d_name))
					continue;
				file = malloc (len + strlen (de->d_name) + 2);
				sprintf (file,"%s%c%s",entry->path,G_DIR_SEPARATOR, de->d_name);
				new_entry = entry_new_by_path_and_label (file, de->d_name);
				if (new_entry) {
					/* */
					if (EN_IS_DIR(new_entry) && (!EN_IS_LINK(new_entry))) {
						if (!process_dir(new_entry, flags, mode, uid, gid))
							return 0;
					} else {
						if (!process_file(new_entry, flags, mode, uid, gid))
							return 0;
					}
					entry_free (new_entry);
				} else {
					perror (file);
				}
				free (file);
			}
			closedir(dir);
		}
	}
	/* first change owner */
	if (flags & CH_USERS) {
		if (chown (entry->path, uid, gid) != 0) {
			if (dlg_continue(entry->path, strerror (errno)) == DLG_RC_CANCEL) {
				return 0;
			}
		}
	}
	/* change mode bits */
	if (flags & CH_PERMS) {
		int rc;
		if (flags & PERMS_SET)
			rc = chmod (entry->path, mode);
		else if (flags & PERMS_ADD)
			rc = chmod (entry->path, entry->mode | mode);
		else
			rc = chmod (entry->path, entry->mode & ~mode);
		if (rc != 0) {
			if (dlg_continue(entry->path, strerror (errno)) == DLG_RC_CANCEL) {
				return 0;
			}
		}
	}
	return 1;
}

/*
 * process on of the files or directories
 */
static int
process_entry (GtkWidget *w, entry_t *entry)
{
	GtkWidget *rb_op;
	GtkWidget *cb_changeowner, *cb_changegroup, *cb_changeperms;
	GtkWidget *combo_owner, *combo_group, *bit, *cb_subdirs;
	char *owner, *group;
	int flags = 0;
	uid_t uid = -1;
	gid_t gid = -1;
	struct passwd *pw;
	struct group *gr;
	mode_t mode = 0;
#ifdef ADOUBLE
	char *adfile;
	entry_t *adentry;
#endif

	gtk_statusbar_push (GTK_STATUSBAR(gmStatusBar), 1, entry->path);
	while (gtk_events_pending())
		gtk_main_iteration();

	cb_changeperms = lookup_widget (GTK_WIDGET(w), "cb_changeperms");
	cb_changeowner = lookup_widget (GTK_WIDGET(w), "cb_changeowner");
	cb_changegroup = lookup_widget (GTK_WIDGET(w), "cb_changegroup");
	cb_subdirs = lookup_widget (GTK_WIDGET(w), "cbtn_subdirs");

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(cb_changeperms))) {
		/* check for the permissions, but first for the operator
		 */
		rb_op= lookup_widget (GTK_WIDGET(w), "rb_set");
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(rb_op))) {
			flags |= PERMS_SET;
		} else {
			rb_op= lookup_widget (GTK_WIDGET(w), "rb_add");
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(rb_op))) {
				flags |= PERMS_ADD;
			} else {
				flags |= PERMS_REM;
			}
		}
		/* collect the bits
		 */
		mode = 0;
		bit = lookup_widget (GTK_WIDGET(w), "cb_u_read");
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(bit)))
			mode |= S_IRUSR;
		bit = lookup_widget (GTK_WIDGET(w), "cb_u_write");
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(bit)))
			mode |= S_IWUSR;
		bit = lookup_widget (GTK_WIDGET(w), "cb_u_exec");
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(bit)))
			mode |= S_IXUSR;
		bit = lookup_widget (GTK_WIDGET(w), "cb_set_uid");
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(bit)))
			mode |= S_ISUID;
		bit = lookup_widget (GTK_WIDGET(w), "cb_g_read");
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(bit)))
			mode |= S_IRGRP;
		bit = lookup_widget (GTK_WIDGET(w), "cb_g_write");
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(bit)))
			mode |= S_IWGRP;
		bit = lookup_widget (GTK_WIDGET(w), "cb_g_exec");
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(bit)))
			mode |= S_IXGRP;
		bit = lookup_widget (GTK_WIDGET(w), "cb_set_gid");
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(bit)))
			mode |= S_ISGID;
		bit = lookup_widget (GTK_WIDGET(w), "cb_o_read");
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(bit)))
			mode |= S_IROTH;
		bit = lookup_widget (GTK_WIDGET(w), "cb_o_write");
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(bit)))
			mode |= S_IWOTH;
		bit = lookup_widget (GTK_WIDGET(w), "cb_o_exec");
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(bit)))
			mode |= S_IXOTH;
		bit = lookup_widget (GTK_WIDGET(w), "cb_set_sticky");
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(bit)))
			mode |= S_ISVTX;

	}
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(cb_changeowner))) {
		combo_owner = lookup_widget (GTK_WIDGET(w), "combo_owner");
		owner = gtk_entry_get_text (GTK_ENTRY(GTK_COMBO(combo_owner)->entry));
		pw = getpwnam (owner);
		if (pw) {
			flags |= CH_OWNER;
			uid = pw->pw_uid;
		} else {
			/* error */
			dlg_error (owner, _("Unknown user"));
			return 0;
		}
	}
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(cb_changegroup))) {
		combo_group = lookup_widget (GTK_WIDGET(w), "combo_group");
		group = gtk_entry_get_text (GTK_ENTRY(GTK_COMBO(combo_group)->entry));
		gr = getgrnam (group);
		if (gr) {
			flags |= CH_GROUP;
			gid = gr->gr_gid;
		} else {
			/* error */
			dlg_error (group, _("Unknown group"));
			return 0;
		}
	}
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(cb_subdirs))) {
		flags |= NESTED;
	}
	if ((flags & CH_USERS) || (flags & CH_PERMS)) {
		/* only go on if we want to change owner or mode */
		if (EN_IS_DIR(entry) && (!EN_IS_LINK(entry))) {
			if (!process_dir (entry, flags, mode, uid, gid))
				return 0;
		} else {
			if (!process_file (entry, flags, mode, uid, gid))
				return 0;
		}
#ifdef ADOUBLE
		adfile = ad_file (entry->path);
		if (adfile) {
			adentry = entry_new_by_path_and_label (adfile, entry->label);
			free (adfile);
			if (adentry) {
				process_file (adentry, flags & ~INTERACTIVE, mode, uid, gid);
			}
		}
#endif
	}
	return 1;
}

/*
 * ok button was pressed
 */
void
on_btn_ok_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	entry_t *entry;

	gu_cursor_wait (gmNotebook);
	gtk_widget_set_sensitive (GTK_WIDGET(button), FALSE);
	entry = gList->data;
	/* process entry
	 */
	process_entry (GTK_WIDGET(button), entry);
	gu_cursor_reset (gmNotebook);

	/* prepare for the next
	 */
	gList = gList->next;
	if (!gList) {
		gtk_main_quit();
		return;
	}
	cb_fill_wmain (GTK_WIDGET(button), gList->data);
}

/*
 * process all files
 */
void
on_btn_all_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	entry_t *entry;

	gu_cursor_wait (gmNotebook);
	gtk_widget_set_sensitive (GTK_WIDGET(button), FALSE);
	while (gList) {
		/* process entry
		 */
		entry = gList->data;
		process_entry (GTK_WIDGET(button), entry);
		/* next */
		gList = gList->next;
		if (!gmStatus)
			break;
	}
	gu_cursor_reset (gmNotebook);
	gtk_main_quit();
}

/*
 * skip current file
 */
void
on_btn_skip_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	if (gList)
		gList = gList->next;
	if (!gList) {
		gtk_main_quit();
		return;
	}
	gu_cursor_wait (gmNotebook);
	cb_fill_wmain (GTK_WIDGET(button), gList->data);
	gu_cursor_reset (gmNotebook);
}

/*
 * cancel / quit the application
 */
void
on_btn_cancel_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	gmStatus = 0; /* break recursive directory changes as well */
	gtk_main_quit();
}

/*
 * fill user and group dropdown lists
 */
void
cb_fill_combo	(GtkWidget       *widget)
{
	GtkWidget *w_owner, *w_group;
	GList *glist = NULL, *ulist = NULL;
	uid_t uid;
	struct passwd *pw;
	struct group *gr;
	gid_t gidlist[NGROUPS_MAX];
	int groups, i;

#ifdef DEBUG_XAT
	printf ("on_combo_owner_realize()\n");
#endif
	w_owner = lookup_widget (GTK_WIDGET(widget), "combo_owner");
	w_group = lookup_widget (GTK_WIDGET(widget), "combo_group");
	/* prepare user and group list
	 */
	uid = getuid();
	if (uid == 0) {
		/* we need the complete lists only as root
		 */
		while ((pw = getpwent()) != NULL) {
			if (pw->pw_name && !(pw->pw_name[0]=='+' && pw->pw_name[1]=='\0'))
				ulist = g_list_append (ulist, g_strdup (pw->pw_name));
		}
		endpwent();
		while ((gr = getgrent()) != NULL) {
			if (gr->gr_name && !(gr->gr_name[0]=='+' && gr->gr_name[1]=='\0'))
				glist = g_list_append (glist, g_strdup (gr->gr_name));
		}
		endgrent();
	} else {
		/* normal user */
		pw = getpwuid (uid);
		ulist = g_list_append (ulist, g_strdup (pw->pw_name));

		/* use only groups to which the belongs to
		 */
		groups = getgroups (NGROUPS_MAX, gidlist);
		if (groups > 0) {
			for (i = 0; i < groups; i++) {
				if ((gr = getgrgid (gidlist[i])) != NULL)
					glist = g_list_append (glist, g_strdup (gr->gr_name));
			}
		}
	}
	ulist = g_list_sort (ulist, (GCompareFunc)strcmp);
	glist = g_list_sort (glist, (GCompareFunc)strcmp);
	gtk_combo_set_popdown_strings (GTK_COMBO(w_owner), ulist);
	gtk_combo_set_popdown_strings (GTK_COMBO(w_group), glist);
}

/*
 */
void
on_cb_changeowner_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *combo_owner;
	combo_owner = lookup_widget (GTK_WIDGET(button), "combo_owner");
	if (combo_owner)
		gtk_widget_set_sensitive (combo_owner,
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)));

}

/*
 */
void
on_cb_changegroup_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *combo_group;
	combo_group = lookup_widget (GTK_WIDGET(button), "combo_group");
	if (combo_group)
		gtk_widget_set_sensitive (combo_group,
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)));
}


/*
 */
gboolean
on_wmain_delete_event                  (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	gmStatus = 0;
	gtk_main_quit();
  	return TRUE;
}

/*
 */
void
on_wmain_realize                       (GtkWidget       *widget,
                                        gpointer         user_data)
{
	gmNotebook = lookup_widget (GTK_WIDGET(widget), "notebook");
	gmStatusBar = lookup_widget (GTK_WIDGET(widget), "statusbar");
}

/*
 * fill the widgets ..
 */
void
cb_fill_wmain (GtkWidget *widget, void *data)
{
	entry_t *entry = (entry_t *)data;
	GtkWidget *status, *w;
	GList *list = gList;
	char string[64], *p, *mime;
	char line[LINE_MAX];
	char cmd[(PATH_MAX+NAME_MAX)*2];
	FILE *pipe;
	int len;
	struct passwd *pw;
	struct group *gr;

#ifdef DEBUG_XAT
	printf ("cb_fill_wmain()\n");
#endif
	status = lookup_widget (GTK_WIDGET(widget), "statusbar");
	gtk_statusbar_push (GTK_STATUSBAR(status), 1, entry->path);
	if (!list->next) {
		/* disable "skip" and "all" buttons
		 */
		w = lookup_widget (GTK_WIDGET(widget), "btn_skip");
		gtk_widget_set_sensitive (w, 0);
		w = lookup_widget (GTK_WIDGET(widget), "btn_all");
		gtk_widget_set_sensitive (w, 0);
	}
	w = lookup_widget (GTK_WIDGET(widget), "en_iv_name");
	gtk_entry_set_text (GTK_ENTRY(w), entry->path);

	w = lookup_widget (GTK_WIDGET(widget), "lbl_iv_type");
	sprintf (cmd, "file '%s'", entry->path);
	pipe = popen(cmd, "r");
	if (pipe) {
		fgets (line, LINE_MAX, pipe);
		len = strlen(line);
		if (len > 1) {
			line[len-1] = '\0';
			if ((p = strstr (line, ": ")) != NULL) {
				gtk_label_set_text (GTK_LABEL(w), p+2);
			} else
				gtk_label_set_text (GTK_LABEL(w), _("<unknown>"));
		}
		pclose (pipe);
	}

	w = lookup_widget (GTK_WIDGET(widget), "lbl_iv_mime");
	mime = mc_get_mime_type (gMreg, entry->path);
	gtk_label_set_text (GTK_LABEL(w), mime ? mime : _("<unknown>"));

	w = lookup_widget (GTK_WIDGET(widget), "lbl_iv_size");
	sprintf (string, "%d", entry->size);
	gtk_label_set_text (GTK_LABEL(w), string);
	w = lookup_widget (GTK_WIDGET(widget), "btn_diskusage");
	if (EN_IS_DIR(entry) && !EN_IS_LINK(entry)) {
		gtk_label_set_text (GTK_LABEL(GTK_BIN(w)->child),
			"Calculate Disk Usage");
		gtk_widget_show (w);
	} else {
		gtk_widget_hide (w);
	}

	w = lookup_widget (GTK_WIDGET(widget), "lbl_iv_mtime");
	strftime (string, 64, "%Y/%m/%d %H:%M", localtime(&entry->mtime));
	gtk_label_set_text (GTK_LABEL(w), string);

	w = lookup_widget (GTK_WIDGET(widget), "lbl_iv_atime");
	strftime (string, 64, "%Y/%m/%d %H:%M", localtime(&entry->atime));
	gtk_label_set_text (GTK_LABEL(w), string);

	/* permissions
	 */
	w = lookup_widget (GTK_WIDGET(widget), "cb_u_read");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), entry->mode & S_IRUSR);
	w = lookup_widget (GTK_WIDGET(widget), "cb_u_write");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), entry->mode & S_IWUSR);
	w = lookup_widget (GTK_WIDGET(widget), "cb_u_exec");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), entry->mode & S_IXUSR);
	w = lookup_widget (GTK_WIDGET(widget), "cb_set_uid");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), entry->mode & S_ISUID);

	w = lookup_widget (GTK_WIDGET(widget), "cb_g_read");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), entry->mode & S_IRGRP);
	w = lookup_widget (GTK_WIDGET(widget), "cb_g_write");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), entry->mode & S_IWGRP);
	w = lookup_widget (GTK_WIDGET(widget), "cb_g_exec");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), entry->mode & S_IXGRP);
	w = lookup_widget (GTK_WIDGET(widget), "cb_set_gid");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), entry->mode & S_ISGID);

	w = lookup_widget (GTK_WIDGET(widget), "cb_o_read");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), entry->mode & S_IROTH);
	w = lookup_widget (GTK_WIDGET(widget), "cb_o_write");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), entry->mode & S_IWOTH);
	w = lookup_widget (GTK_WIDGET(widget), "cb_o_exec");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), entry->mode & S_IXOTH);
	w = lookup_widget (GTK_WIDGET(widget), "cb_set_sticky");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), entry->mode & S_ISVTX);

	/* group / owner
	 */
	w = lookup_widget (GTK_WIDGET(widget), "combo_owner");
	pw = getpwuid (entry->uid);
	w = GTK_COMBO(w)->entry;
	gtk_entry_set_text (GTK_ENTRY(w), pw ? pw->pw_name : "<unknown>");

	w = lookup_widget (GTK_WIDGET(widget), "combo_group");
	gr = getgrgid (entry->gid);
	w = GTK_COMBO(w)->entry;
	gtk_entry_set_text (GTK_ENTRY(w), gr ? gr->gr_name : "<unknown>");
}

/*
 */
static unsigned long
size_of_dir (char *path)
{
	int size = 0, tsize;
	char file[PATH_MAX+1];
	DIR *dir;
	struct stat st;
	struct dirent *de;

	if (!path)
		return 0;

	dir = opendir (path);
	if (!dir)
		return 0;
	while ((de = readdir(dir)) != NULL) {
		if (io_is_dirup (de->d_name))
			continue;
		sprintf (file, "%s%c%s", path, G_DIR_SEPARATOR, de->d_name);
		if (lstat (file, &st) != 0) {
			closedir (dir);
			return 0;
		}
		if ((!io_is_current(de->d_name)) && S_ISDIR(st.st_mode)) {
			tsize = size_of_dir (file);
			if (!tsize) {
				closedir (dir);
				return 0;
			}
			size += tsize;
		} else {
			size += st.st_size;
		}
	}
	closedir (dir);
	return size;
}

/*
 */
void
on_btn_diskusage_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *name;
	char *file;
	entry_t *entry;
	unsigned long size;
	char str_size[64];

	name = lookup_widget (GTK_WIDGET(button), "en_iv_name");
	if (!name)
		return;
	gu_cursor_wait (GTK_WIDGET(button)->parent);
	file = gtk_entry_get_text (GTK_ENTRY(name));
	entry = entry_new_by_path (file);
	if (entry) {
		size = size_of_dir (entry->path);
		entry_free (entry);
		if (size) {
			sprintf (str_size, "%lu KBytes", (size >> 10) + 1);
		} else
			sprintf (str_size, "%s", "<Error>");
		gtk_label_set_text (GTK_LABEL(GTK_BIN(button)->child), str_size);
	}
	gu_cursor_reset (GTK_WIDGET(button)->parent);
}

