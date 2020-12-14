/*
 * xcp_gui.c
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>	/* utime() */
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_SENDFILE
#include <sys/sendfile.h>
#endif
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "i18n.h"
#include "xcp_gui.h"
#include "gtk_exec.h"
#include "gtk_dnd.h"
#include "uri.h"
#include "io.h"
#include "gtk_dlg.h"
#include "entry.h"
#include "adouble.h"

#define XOPT (GTK_FILL|GTK_EXPAND|GTK_SHRINK)
#define LOPT (GTK_FILL)

#define ST_OK		1
#define ST_CANCEL	2

#define RC_TRUE		DLG_RC_OK
#define RC_FALSE	DLG_RC_CANCEL

static int gStatus = ST_OK;
static int gCurrent;
static int gNum;

typedef struct {
	GtkWidget *source;
	GtkWidget *target;
	GtkWidget *progress;
	GtkWidget *label;
	GtkWidget *cancel;
	time_t start;
	unsigned int bytes;
	int mode;
} cfg_t;


/*
 * gtk initializing
 */
void
gui_init (int *argc, char ***argv, char *user_rc)
{
	gtk_set_locale ();
	gtk_rc_add_default_file ("xcp.rc");
	gtk_init (argc, argv);
	gtk_rc_parse (user_rc);
}

/*
 * call on exit
 */
void
cb_exit (GtkWidget *w, gpointer data)
{
	cfg_t *cfg;

	cfg = (cfg_t *) data;
#ifdef DEBUG_XCP
	printf ("cb_exit()\n");
#endif
	exit(0);
}


/*
 * callback for the cancel button
 */
static void
cb_cancel (GtkWidget *cancel, void *data)
{
	gStatus = ST_CANCEL;
}


/*
 * move / copy / link a file
 */
int
transfer_file (entry_t *source, entry_t *target, int flags, cfg_t *cfg, int lvl)
{
#define BUFLEN 131072
	char targetfile [PATH_MAX+1], link [PATH_MAX+1];
	char buf[BUFLEN], str_bps[64];
	struct stat t_stat;
	int rc, len, i;
	unsigned long written, bps;
	FILE *ofp, *nfp;

#ifdef DEBUG_XCP
	printf ("transfer_file() %s -> %s\n", source->path, target->path);
#endif
	/* create target filename if needed
	 */
	if (EN_IS_DIR(target)) {
		if (io_is_root (target->path)) {
			/* don't add a slash
			 */
			sprintf (targetfile, "%s%s", target->path, source->label);
		} else {
			sprintf (targetfile, "%s%c%s",
				target->path, G_DIR_SEPARATOR, source->label);
		}
	} else {
		strcpy (targetfile, target->path);
	}
	if (cfg->mode == MODE_TRASH) {
		/* check if we have to use an other filename
		 */
		i = 0;
		while (++i) {
			if (lstat (targetfile, &t_stat) != ERROR) {
				sprintf (targetfile, "%s%c%s;%d",
					target->path, G_DIR_SEPARATOR, source->label, i);
			} else
				break;
		}
	}
	/* update labels
	 */
	gtk_entry_set_text (GTK_ENTRY(cfg->source), source->path);
	gtk_entry_set_text (GTK_ENTRY(cfg->target), targetfile);
	gtk_progress_set_percentage (GTK_PROGRESS(cfg->progress), 0.0);
	while (gtk_events_pending())
		gtk_main_iteration();

	/* does the file still exist?
	 */
	if (lstat (targetfile, &t_stat) != ERROR) {
		if (t_stat.st_ino == source->inode) {
			return dlg_continue (_("Same file!"), source->path);
		}
		if (!(flags & FL_QUIET)) {
			if ((gCurrent < (gNum - 1)) || (lvl > 0)) {
				rc = dlg_ok_all_skip (_("Overwrite file?"), targetfile);
			} else {
				rc = dlg_ok_skip (_("Overwrite file?"), targetfile);
			}
			if (rc == DLG_RC_SKIP)
				return (RC_TRUE);
			if (rc == DLG_RC_CANCEL)
				return (RC_FALSE);
			if (rc == DLG_RC_ALL)
				flags |= FL_QUIET;
			/* RC_OK -> we can go on
			 */
		}
	} else if (EN_IS_FILE(target)) {
		/* target file does not exist, so we have to stat() the
		 * parent directory
		 */
		char *parent;
		parent = entry_get_parent (target);
		if (parent) {
			lstat (parent, &t_stat);
			g_free (parent);
		}
	} else if (EN_IS_DIR(target)) {
		lstat (target->path, &t_stat);
	}
	/* first check if the files are on the same device
	 */
	if ((cfg->mode == MODE_MOVE) || (cfg->mode == MODE_TRASH)) {
#ifdef DEBUG_XCP
		printf (" s_dev=%d t_dev=%d\n", (int)source->device,(int)t_stat.st_dev);
#endif
		if (source->device == t_stat.st_dev) {
			/* both are on the same device, so we have just to rename
			 */
#ifdef DEBUG_XCP
			fprintf (stderr, "%s[%d] %s -> %s\n",
				__FILE__, __LINE__, source->path, targetfile);
#endif
			if (rename (source->path, targetfile) == ERROR) {
				return dlg_continue (source->path, strerror(errno));
			}
			gtk_progress_set_percentage (GTK_PROGRESS(cfg->progress), 1);
			return (RC_TRUE);
		}
	}
	/* symbilic links are copied as they are
	 */
	if ((cfg->mode == MODE_LINK) || (EN_IS_LINK(source))) {
		if (cfg->mode != MODE_LINK) {
			len = readlink (source->path, link, PATH_MAX);
			if (len <= 0) {
				return dlg_continue (source->path, strerror(errno));
			}
			link[len] = '\0';
		} else {
			strcpy (link, source->path);
		}
		if (symlink (link, targetfile) == ERROR) {
			return dlg_continue (targetfile, strerror(errno));
		}
		if ((cfg->mode == MODE_MOVE) || (cfg->mode == MODE_TRASH)) {
			if (unlink (source->path) == ERROR) {
				return dlg_continue (source->path, strerror(errno));
			}
		}
		gtk_progress_set_percentage (GTK_PROGRESS(cfg->progress), 1);
		return (RC_TRUE);
	}
	/* we can't copy device files */
    if (EN_IS_DEVICE(source)) {
        return dlg_continue (source->path, _("Can't copy device file"));
    }
    /* we can't copy fifo files */
    if (EN_IS_FIFO(source)) {
        return dlg_continue (source->path, _("Can't copy FIFO"));
    }
    /* we can't copy socket files */
    if (EN_IS_SOCKET(source)) {
        return dlg_continue (source->path, _("Can't copy SOCKET"));
    }

	/* we have to copy the file by reading/writing the data
	 */
	ofp = fopen (source->path, "rb");
	if (!ofp) {
		return dlg_error_continue (source->path, strerror(errno));
	}
	nfp = fopen (targetfile, "wb");
	if (!nfp) {
		if ((flags & FL_APPLEDOUBLE) &&(strstr(target->path,APPLEDOUBLE))){
			/* may be we miss a appledouble directory ..
			 */
			mkdir (target->path, 0xFFFFFFFF);
			nfp = fopen (targetfile, "wb");
			if (!nfp) {
				fclose (ofp);
				return dlg_error_continue (targetfile, strerror(errno));
			}
		} else {
			fclose (ofp);
			return dlg_error_continue (targetfile, strerror(errno));
		}
	}
	written = 0;
	while ((len = fread (buf, 1, BUFLEN, ofp)) > 0) {
		if (fwrite (buf, 1, len, nfp) != len) {
			fclose (ofp);
			fclose (nfp);
			dlg_error (targetfile, strerror(errno));
			break;
		}
		written += len;
		cfg->bytes += len;
		bps = cfg->bytes / (time(NULL) - cfg->start +1);
		sprintf (str_bps, "%lu Kbytes/sec.", bps >> 10);
		gtk_label_set_text (GTK_LABEL(cfg->label), str_bps);
		gtk_progress_set_percentage (GTK_PROGRESS(cfg->progress),
				written / (gfloat)source->size );
		while (gtk_events_pending())
			gtk_main_iteration();
		if (gStatus == ST_CANCEL) {
			fclose (nfp);
			fclose (ofp);
			unlink (targetfile);
			return (RC_FALSE);
		}
	}
	
	fclose (nfp);
	fclose (ofp);

	if (written < source->size) {
		dlg_error (_("Too less bytes transfered! Device full?"), targetfile);
	} else if (written > source->size) {
		dlg_error (_("Too much bytes transfered!?"), targetfile);
	}
	if ((flags & FL_PRESERVE) || (cfg->mode == MODE_MOVE)) {
		/* chown(), chmod() and utime()
		 */
#ifdef DEBUG_XCP
		printf (" chown() etc.. of %s\n", targetfile);
#endif
		rc = lchown (targetfile, source->uid, source->gid);
		if ((rc == 0) && (!EN_IS_LINK(source))) {
			struct utimbuf utb;
			chmod (targetfile, source->mode);
			utb.actime = source->atime;
			utb.modtime = source->mtime;
			utime (targetfile, &utb);
		} else {
			/* at least set execute bits */
			if (source->mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
				stat (targetfile, &t_stat);
				chmod (targetfile,
					t_stat.st_mode | (source->mode &(S_IXUSR|S_IXGRP|S_IXOTH)));
			}
		}
	} else {
		/* only set execute bits */
		if (source->mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
			stat (targetfile, &t_stat);
			chmod (targetfile,
				t_stat.st_mode | (source->mode &(S_IXUSR|S_IXGRP|S_IXOTH)));
		}
	}
	if ((cfg->mode == MODE_MOVE) || (cfg->mode == MODE_TRASH)) {
		if (unlink (source->path) == ERROR) {
			return dlg_error_continue (source->path, strerror (errno));
		}
	}
	return RC_TRUE;
}

/*
 * move/copy a directory (source) to an other directory (target)
 */
int
transfer_dir (entry_t *source, entry_t *target, int flags, cfg_t *cfg, int lvl)
{
	char targetdir[PATH_MAX+1], sourcedir[PATH_MAX+1];
	struct stat nt_stat, t_stat;
	DIR *dir;
	struct dirent *de;
	entry_t *new_source, *new_target;
	int rc, i;

#ifdef DEBUG_XCP
	printf ("transfer_dir() %s -> %s\n", source->path, target->path);
#endif
	/* update the labels
	 */
	gtk_entry_set_text (GTK_ENTRY(cfg->source), source->path);
	gtk_entry_set_text (GTK_ENTRY(cfg->target), target->path);
	while (gtk_events_pending())
		gtk_main_iteration();

	/* new directory name
	 */
	if (io_is_directory (target->path)) {
		sprintf (targetdir,"%s%c%s",target->path,G_DIR_SEPARATOR,source->label);
	} else {
		sprintf (targetdir,"%s",target->path);
	}

	if (cfg->mode == MODE_TRASH) {
		/* check if we have to use an other filename
		 */
		i = 0;
		while (++i) {
			if (lstat (targetdir, &t_stat) != ERROR) {
				sprintf (targetdir, "%s%c%s;%d",
					target->path, G_DIR_SEPARATOR, source->label, i);
			} else
				break;
		}
	}
	/* check if both are the same
	 */
#ifdef DEBUG_XCP
	printf ("  source_i=%lu target_i=%lu\n", source->inode, target->inode);
#endif
	if (source->inode == target->inode) {
		return dlg_error_continue (_("Error"), _("Same Inode!"));
	}
	/* check if rename() is enough
	 */
	if ((cfg->mode == MODE_MOVE) || (cfg->mode == MODE_TRASH)) {
		if (stat (target->path, &t_stat) == ERROR) {
			return dlg_error_continue (_("Can't stat() file"), target->path);
		}
		if (source->device == t_stat.st_dev) {
			if (stat (targetdir, &t_stat) == 0) {
				return dlg_skip (targetdir, _("still exists"));
			}
			if (rename (source->path, targetdir) == ERROR) {
				return dlg_error_continue (source->path, strerror(errno));
			}
			return (DLG_OK);
		}
	}
	/* read the source directory
	 */
	dir = opendir (source->path);
	if (!dir) {
		return dlg_error_continue (source->path, strerror(errno));
	}

	if (mkdir (targetdir, 0xFFFFFFFF) == ERROR) {
		if (errno == EEXIST) {
			if (!io_is_directory (targetdir)) {
				closedir (dir);
				return dlg_skip (targetdir, _("exists and is not directory"));
			}
			/* else: silently ignore this.. */
			if ( stat (targetdir, &nt_stat) == ERROR) {
				closedir (dir);
				return dlg_error_continue (_("Can't stat() file"), targetdir);
			}
			if (nt_stat.st_ino == source->inode) {
				closedir (dir);
				return dlg_error_continue (_("Same directory!"), source->path);
			}
			if (cfg->mode != MODE_COPY) {
				closedir (dir);
				return dlg_skip (targetdir, _("still exists"));
			}
		} else {
			closedir (dir);
			return dlg_error_continue (targetdir, strerror(errno));
		}
	}
	while ((de = readdir (dir)) != NULL) {
		while (gtk_events_pending())
			gtk_main_iteration();
		if (gStatus == ST_CANCEL) {
			closedir (dir);
			return (RC_FALSE);
		}
		if ((io_is_current(de->d_name)) || (io_is_dirup(de->d_name)))
			continue;
		sprintf (sourcedir, "%s%c%s", source->path, G_DIR_SEPARATOR,de->d_name);
		new_source = entry_new_by_path_and_label (sourcedir, de->d_name);
		new_target = entry_new_by_path_and_label (targetdir, source->label);

		if ((!new_source) || (!new_target)) {
			if (dlg_skip (_("Can't transfer file"), sourcedir) == RC_FALSE) {
				closedir (dir);
				return RC_FALSE;
			}
			if (new_source)
				entry_free (new_source);
			if (new_target)
				entry_free (new_target);
			continue;
		}
		if (EN_IS_DIR (new_source) && (!EN_IS_LINK(new_source))) {
			/* call this function recursive
			 */
			rc = transfer_dir (new_source, new_target, flags, cfg, lvl+1);
		} else {
			rc = transfer_file(new_source, new_target, flags, cfg, lvl+1);
		}
		entry_free (new_source);
		entry_free (new_target);
		if (rc == RC_FALSE) {
			closedir (dir);
			return RC_FALSE;
		}
	}
	closedir (dir);
	if ((flags & FL_PRESERVE) || (cfg->mode == MODE_MOVE)) {
		/* chown() and chmod()
		 */
#ifdef DEBUG_XCP
		printf (" chown() etc..\n");
#endif
		rc = lchown (targetdir, source->uid, source->gid);
		if ((rc == 0) && (!EN_IS_LINK(source))) {
			/* only if user/group are ok the setuid setgid bits
			 * should be set */
			chmod (targetdir, source->mode);
		}
	}
	if ((cfg->mode == MODE_MOVE) || (cfg->mode == MODE_TRASH)) {
		if (rmdir (source->path) == ERROR) {
            return dlg_error_continue (source->path, strerror(errno));
        }
	}
	return (RC_TRUE);
}

/*
 */
int
gui_main (wgeo_t *geo, int mode, char **files, char *target, int num, int flags)
{
	GtkWidget *top, *box, *table, *label, *btnbox;
	char *title, *file, *adfile, *adtarget;
	cfg_t cfg;
	entry_t *target_en, *src_en, *en_ad, *en_ad_target;
	int appledouble;

	cfg.start = time (NULL);
	cfg.bytes = 0;
	cfg.mode  = mode;

	appledouble = flags & FL_APPLEDOUBLE;
	flags = flags & ~FL_APPLEDOUBLE;

	gCurrent = 0;
	gNum = num;

	/* create toplevel etc..
	 */
	top = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect (GTK_OBJECT(top), "destroy",
		GTK_SIGNAL_FUNC (cb_exit), (void *) &cfg);

	box = gtk_vbox_new (FALSE, 4);
	gtk_container_add (GTK_CONTAINER(top), box);
	gtk_container_set_border_width (GTK_CONTAINER(box), 4);

	table = gtk_table_new (2, 4, FALSE);
	gtk_box_pack_start (GTK_BOX(box), table, TRUE, TRUE, 4);

	label = gtk_label_new (_("Source: "));
	gtk_table_attach (GTK_TABLE(table), label, 0, 1, 0, 1, 0, 0,0,0);

	cfg.source = gtk_entry_new ();
	gtk_table_attach (GTK_TABLE(table), cfg.source, 1, 2, 0, 1, XOPT, 0,0,0);

	label = gtk_label_new (_("Target: "));
	gtk_table_attach (GTK_TABLE(table), label, 0, 1, 1, 2, 0, 0,0,0);

	cfg.target = gtk_entry_new ();
	gtk_table_attach (GTK_TABLE(table), cfg.target, 1, 2, 1, 2, XOPT, 0,0,0);

	cfg.progress = gtk_progress_bar_new ();
	gtk_table_attach (GTK_TABLE(table), cfg.progress, 1, 2, 2, 3, LOPT, 0,0,0);
	gtk_progress_set_format_string (GTK_PROGRESS(cfg.progress), "%p%%");
	gtk_progress_set_show_text (GTK_PROGRESS(cfg.progress), TRUE);

	cfg.label = gtk_label_new (_("0 bytes/sec."));
	gtk_table_attach (GTK_TABLE(table), cfg.label, 1, 2, 3, 4, LOPT, 0,0,0);

	btnbox = gtk_hbox_new (FALSE, 1);
	gtk_box_pack_start (GTK_BOX(box), btnbox, FALSE, FALSE, 3);
	cfg.cancel = gtk_button_new_with_label (_(" Cancel "));
	gtk_box_pack_start (GTK_BOX(btnbox), cfg.cancel, TRUE, FALSE, 4);

	gtk_signal_connect (GTK_OBJECT(cfg.cancel), "clicked",
				GTK_SIGNAL_FUNC(cb_cancel), &cfg);

	gtk_widget_show_all (top);
	gtk_window_set_default_size (GTK_WINDOW(top), geo->width, geo->height);

	switch (mode) {
		case MODE_COPY:
			title = _("xcp: Copying ..");
			break;
		case MODE_MOVE:
			title = _("xcp: Moving ..");
			break;
		case MODE_TRASH:
			title = _("xcp: Trashing ..");
			break;
		case MODE_LINK:
			title = _("xcp: Linking ..");
			break;
		default:
			fprintf (stderr, "fatal error, unknown mode '%c'\n", mode);
			exit (1);
	}
	gtk_window_set_title (GTK_WINDOW(top), title);

	target_en = entry_new_by_path (target);
	if (!target_en) {
		if (num > 1) {
			dlg_error (_("Error getting info from"), target);
			/* todo */
			exit (1);
		} else {
			/* if we have only one source target could be
			 * a file.
			 */
			target_en = entry_new_by_type (target, FT_FILE);
		}
	}
	if ((num > 1) && (!(EN_IS_DIR(target_en)))) {
		dlg_error (_("Fatal error"),
			_("Can't transfer multiple files to a file"));
		exit (1);
	}

	gCurrent = 0;
	while (gCurrent < num) {
		while (gtk_events_pending())
			gtk_main_iteration();
		file = files[gCurrent];
#ifdef DEBUG_XCP
		printf ("gui_main() %s -> %s\n", file, target);
#endif
		if (gStatus == ST_CANCEL) {
			entry_free (target_en);
			return 0;
		}
		/* update labels */
		gtk_entry_set_text (GTK_ENTRY(cfg.source), file);
		gtk_entry_set_text (GTK_ENTRY(cfg.target), target);

		src_en = entry_new_by_path (file);
		if (!src_en) {
			if (dlg_continue (file, strerror(errno)) == DLG_RC_CANCEL)
				return 0;
			else {
				gCurrent++;
				continue;
			}
		}
		if (mode == MODE_LINK) {
			if (transfer_file (src_en, target_en, flags, &cfg, 0) == RC_FALSE)
				return 2;
		} else {
			if (EN_IS_DIR(src_en) && (!EN_IS_LINK(src_en))) {
				if (transfer_dir(src_en, target_en, flags, &cfg, 0) == RC_FALSE)
					return 3;
			} else {
				/* file, symbolic links ..
				 */
				if (transfer_file(src_en,target_en, flags, &cfg,0)==RC_FALSE) {
					return 2;
				}
				if ((appledouble) &&
					(!strstr(src_en->path, APPLEDOUBLE)) &&
					(!strstr(target_en->path, APPLEDOUBLE)) ) {
					/* copy /foo/.AppleDouble/bar as well
					 */
					adfile = ad_file (src_en->path);
					if (adfile) {
						en_ad = entry_new_by_path (adfile);
						free (adfile);
						if (en_ad) {
							/* AppleDouble file exists
							 */
							if (EN_IS_DIR(target_en)) {
								adtarget = ad_subdir (target_en->path);
							} else {
								adtarget = ad_dir (target_en->path);
							}
							en_ad_target = entry_new_by_path (adtarget);
							if (!en_ad_target) {
								/* create it */
								mkdir (adtarget, 0xFFFFFFFF);
								en_ad_target = entry_new_by_path (adtarget);
								/* todo: chmod, chown */
							}
							free (adtarget);
							if (transfer_file ( en_ad, en_ad_target, 
									flags | FL_APPLEDOUBLE | FL_QUIET, &cfg,0)
									== RC_FALSE)
								return 2;
							entry_free (en_ad);
							entry_free (en_ad_target);
						}
					}
				}
			}
			entry_free (src_en);
		}
		gCurrent++;
	}
	return 0;
}

