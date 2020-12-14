/*
 * xpg_gui.c
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
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gtk/gtk.h>
/* #include <gdk/gdk.h> */
#include <gdk/gdkkeysyms.h>
#include "i18n.h"
#include "xpg_gui.h"
#include "gtk_exec.h"
#include "gtk_dnd.h"
#include "uri.h"
#include "io.h"
#include "gtk_dlg.h"
#include "gtk_util.h"
#include "xpg_opt.h"
#include "xap_fs.h"
#include "icons/xpg_encrypt.xpm"
#include "icons/xpg_decrypt.xpm"
#include "icons/xpg_save.xpm"
#include "icons/xpg_cut.xpm"
#include "icons/xpg_sign.xpm"
#include "icons/xpg_open.xpm"

#define LINESIZE 1024
#define CBORDER 5

static GtkTargetEntry target_table[] = {
	{ "text/uri-list",	0,	TARGET_URI_LIST },
	{ "STRING",			0,	TARGET_STRING },
	{ "text/plain",		0,	TARGET_PLAIN },
};
#define NUM_TARGETS (sizeof(target_table)/sizeof(GtkTargetEntry))

typedef struct {
	GtkWidget *notebook;
	GtkWidget *error;
	GdkColormap *cmap;
	opt_t *opt;
} cfg_t;

typedef struct {
	GtkWidget *w_label;
	GtkWidget *w_box;
	GtkWidget *w_list;	/* key list */
	GtkWidget *w_text;	/* text widget */
	GtkWidget *w_file;	/* file list */
	cfg_t *app;
} page_t;

#define ACCEL 1
typedef struct {
	gchar *label;
	void *func;
	void *data;
	int flags;
	int key;
	int mod;
} menu_entry;

/*
 * gtk initializing
 */
void
gui_init (int *argc, char ***argv, char *user_rc)
{
	gtk_set_locale ();
	gtk_rc_add_default_file ("xpg.rc");
	gtk_init (argc, argv);

	gtk_rc_parse (DATA_DIR"/xap/"XAP_RC);
	gtk_rc_parse (user_rc);
}

/*
 * compare the end of "haystack" with "needle", if found
 * return the starting point in "haystack"
 */
char *
strrstr (char *haystack, char *needle)
{
	int len_h, len_n;

	if (!haystack)
		return NULL;
	if (!needle)
		return NULL;

	len_h = strlen (haystack);
	len_n = strlen (needle);

	if (len_h < len_n)
		return NULL;

	if (strncmp (haystack + (len_h - len_n), needle, len_n) == 0)
		return haystack + (len_h - len_n);
	return NULL;
}

/*
 * called on drag_motion event - needed for tooltips
 */
gboolean
button_drag_motion (GtkWidget *btn, GdkDragContext *context, gint x, gint y,
					guint time, void *data)
{
	GdkEventCrossing ev;

	ev.type = GDK_ENTER_NOTIFY;
	ev.window = btn->window;
	ev.send_event = TRUE;
	ev.subwindow = 0;
	ev.time = gdk_time_get ();
	ev.mode = GDK_CROSSING_GRAB;

	gdk_event_put ((GdkEvent *)&ev);
	return (TRUE);
}

/*
 * called on drag_leave event - needed for tooltips
 */
void
button_drag_leave (GtkWidget *btn, GdkDragContext *cnt, guint time, void *data)
{
	GdkEventCrossing ev;

	ev.type = GDK_LEAVE_NOTIFY;
	ev.window = btn->window;
	ev.send_event = TRUE;
	ev.subwindow = 0;
	ev.time = gdk_time_get ();
	ev.mode = GDK_CROSSING_GRAB;

	gdk_event_put ((GdkEvent *)&ev);
}

/*
 * what is that for?
 */
void
menu_detach ()
{
}

/*
 * make a icon widget from the named icon data
 */
static GtkWidget *
icon_widget (GdkWindow *window, GdkColor *bg, GdkColormap *cmap, char **icon)
{
	GtkWidget *w_pix = NULL;
	GdkPixmap *pixmap;
	GdkBitmap *mask = NULL;

#ifdef DEBUG
	printf ("icon_widget()\n");
#endif
	pixmap = gdk_pixmap_colormap_create_from_xpm_d (window, cmap,
				&mask, bg, icon);
	if (pixmap) {
		w_pix = gtk_pixmap_new (pixmap, mask);
		gdk_pixmap_unref (pixmap);
		gdk_pixmap_unref (mask);
	}
	return (w_pix);
}

/*
 * called if a drop is at the text notebook-page label or at the 
 * text widget itself
 */
void
text_drop_data (GtkWidget *w, GdkDragContext *context, gint x, gint y,
		GtkSelectionData *data, guint info, guint time, page_t *pg)
{
	char *arg, *f;
	int len, num;
	GList *list;
	uri_t *uri;
	FILE *fp;
	char line[LINESIZE];

	if ((data->length > 0) && (data->format == 8)) {
		gu_cursor_wait (w->parent);
		len = data->length;
		arg = g_malloc (len + 1);
		arg[len] = '\0';
		memcpy (arg, data->data, len);
		if (info == TARGET_URI_LIST) {

			num = uri_parse_list (arg, &list);
			g_free (arg);
			if (!num) {
				gtk_drag_finish (context, FALSE, TRUE, time);
				return;
			} else {
				gtk_drag_finish (context, TRUE, TRUE, time);
			}
			uri_remove_file_prefix_from_list (list);
			while (num) {
				num--;
				uri = (uri_t *) (g_list_nth (list, num))->data;
				if (uri) {
					f = uri->url;
					fp = fopen (f, "r");
					if (fp) {
						gtk_text_freeze (GTK_TEXT(pg->w_text));
						while (fgets (line, LINESIZE, fp)) {
							gtk_text_insert (GTK_TEXT(pg->w_text),
								NULL, NULL, NULL, line, strlen(line));
						}
						gtk_text_thaw (GTK_TEXT(pg->w_text));
						fclose (fp);
					} else {
						gtk_text_insert (GTK_TEXT(pg->w_text),
							NULL, NULL, NULL, f, uri->len);
					}
				}
			}
			uri_free_list (list);
		} else {
			gtk_drag_finish (context, TRUE, TRUE, time);
			gtk_text_insert (GTK_TEXT(pg->w_text),
					NULL, NULL, NULL, arg, len);
			g_free (arg);
		}
		gu_cursor_reset (w->parent);
		return;
	}
	gtk_drag_finish (context, FALSE, TRUE, time);
}


/*
 * called if a drop is at the file notebook-page label
 */
void
file_drop_data (GtkWidget *label, GdkDragContext *context, gint x, gint y,
		GtkSelectionData *data, guint info, guint time, page_t *pg)
{
	char *arg, *f;
	int len, num;
	GList *list;
	uri_t *uri;

	if ((data->length > 0) && (data->format == 8)) {
		len = data->length;
		arg = g_malloc (len + 1);
		arg[len] = '\0';
		memcpy (arg, data->data, len);

		num = uri_parse_list (arg, &list);
		g_free (arg);
		if (!num) {
			gtk_drag_finish (context, FALSE, TRUE, time);
			return;
		} else {
			gtk_drag_finish (context, TRUE, TRUE, time);
		}
		uri_remove_file_prefix_from_list (list);
		while (num) {
			num--;
			uri = (uri_t *) (g_list_nth (list, num))->data;
			if (uri) {
				f = uri->url;
				gtk_clist_append (GTK_CLIST(pg->w_file), &f);
			}
		}
		uri_free_list (list);
		return;
	}
	gtk_drag_finish (context, FALSE, TRUE, time);
}

/*
 * put page to the front if drag comes over
 */
gboolean
page_drag_motion (GtkWidget *label,
		GdkDragContext *context, gint x, gint y, guint time, int i)
{
	GtkNotebook *notebook = GTK_NOTEBOOK(label->parent);
	if (i != gtk_notebook_get_current_page (notebook)) {
		gtk_notebook_set_page (notebook, i);
	}
	return (1);
}

/*
 */
static void
sync_error_widget (char *file, GtkWidget *text)
{
	FILE *fp;
	char line[LINESIZE];

	/* read error file */
	fp = fopen (file, "r");
	if (fp) {
		while (fgets (line, LINESIZE, fp))
			gtk_text_insert (GTK_TEXT(text), NULL, NULL, NULL,
				line, strlen(line));
		fclose (fp);
	}
}

/*
 * encrypt text in the text widget and paste the result back
 */
static void
cb_text_encrypt (GtkWidget *w, gpointer *data)
{
	FILE *fp;
	char *fname;
	char *buff;
	char *cmd;
	char line[LINESIZE];
	int len, num, cmd_len;
	page_t *pg = (page_t *) data;
	GtkWidget *text;
	GList *list;
#define COLS 3
#define CSIZE 256
	char *email;
	int rc;
	opt_t *opt;

	text = pg->w_text;
	opt  = pg->app->opt;

	len = gtk_text_get_length (GTK_TEXT(text));
	if (!len) {
		dlg_info (_("Nothing to encrypt!"));
		return;
	}

	fname = tmpnam (NULL);
	if (!fname) {
		return;
	}
#ifdef DEBUG
	printf ("cb_text_encrypt() fname=%s\n", fname);
#endif
	gu_cursor_wait (w->parent);
	list = GTK_CLIST(pg->w_list)->selection;
	cmd_len = 0;
	for (list = GTK_CLIST(pg->w_list)->selection; list; list = list->next) {
		num = (int)list->data;
		gtk_clist_get_text (GTK_CLIST(pg->w_list), num, 2, &email);
		cmd_len += strlen (email) + 4;
	}
	if (!cmd_len) {
		gu_cursor_reset (w->parent);
		dlg_error(_("Error"),
			_("You have to mark at least one person in the key list!"));
		return;
	}
	cmd_len += strlen (fname) + 120;
	cmd = g_malloc (cmd_len);
	if (!cmd) {
		gu_cursor_reset (w->parent);
		return;
	}
	strcpy (cmd, "gpg --yes --batch -a --encrypt -o -");
	for (list = GTK_CLIST(pg->w_list)->selection; list; list = list->next) {
		num = (int)list->data;
		gtk_clist_get_text (GTK_CLIST(pg->w_list), num, 2, &email);
		strcat (cmd, " -r ");
		strcat (cmd, email);
	}
	if (opt->quiet)
		strcat (cmd, " -q");
	strcat (cmd, " ");
	strcat (cmd, fname);

#ifdef DEBUG
	printf ("** cmd=%s\n", cmd);
#endif
	fp = fopen (fname, "w");
	if (!fp) {
		gu_cursor_reset (w->parent);
		dlg_error (fname, strerror (errno));
		g_free (cmd);
		return;
	}

	/* read the text widget content and write it to a file
	 */
	buff = gtk_editable_get_chars (GTK_EDITABLE(text), 0, -1);
	fwrite (buff, 1, strlen(buff), fp);
	fclose (fp);
	g_free (buff);

	gtk_text_freeze (GTK_TEXT(text));
	gtk_text_set_point (GTK_TEXT(text), 0);
	gtk_text_forward_delete (GTK_TEXT(text), len);
	gtk_text_thaw (GTK_TEXT(text));

	/* redirect stderr */
	freopen (opt->err_file, "w", stderr);

	fp = popen (cmd, "r");
	if (!fp) {
		gu_cursor_reset (w->parent);
		dlg_error (_("Error"), _("Failed to open pipe"));
		unlink (fname);
		g_free (cmd);
		return;
	}
	/* read back the encrypted data */
	gtk_text_freeze (GTK_TEXT(text));
	while (fgets (line, LINESIZE, fp)) {
		gtk_text_insert (GTK_TEXT(text), NULL, NULL, NULL, line, strlen(line));
	}
	gtk_text_thaw (GTK_TEXT(text));
	rc = pclose (fp);

	sync_error_widget (opt->err_file, pg->app->error);
	/* check for errors in the pipe */
	if (rc != 0) {
		dlg_error (_("Error"), _("GPG encryption failed!"));
		/* restore the original data */
		fp = fopen (fname, "r");
		gtk_text_freeze (GTK_TEXT(text));
		while (fgets (line, LINESIZE, fp)) {
			gtk_text_insert (GTK_TEXT(text),NULL,NULL,NULL,line, strlen(line));
		}
		gtk_text_thaw (GTK_TEXT(text));
		fclose (fp);
	}
	unlink (fname);
	g_free (cmd);
	gu_cursor_reset (w->parent);
}

/*
 * called if "decrypt" button on the text page is pressed
 */
static void
cb_text_decrypt (GtkWidget *w, gpointer *data)
{
	int fd;
	FILE *fp;
	int len, rc;
	char pw[DLG_MAX];
	char enfile[1024];
	char pwfile[1024];
	char cmd[2048];
	char line[LINESIZE];
	char *str;
	page_t *pg = (page_t *) data;
	opt_t *opt;

	opt = pg->app->opt;
	len = gtk_text_get_length (GTK_TEXT(pg->w_text));
	if (!len) {
		dlg_info (_("Nothing to decrypt!"));
		return;
	}
	pw[0] = '\0';
	if (dlg_password (_("Passphrase: "), pw) != DLG_OK) {
		return;
	}
	gu_cursor_wait (w->parent);
	tmpnam (pwfile);
#ifdef DEBUG
	printf ("cb_text_decrypt() pwfile=%s\n", pwfile);
#endif
	fd = open (pwfile, O_RDWR|O_CREAT);
	if (fd < 0) {
		gu_cursor_reset (w->parent);
		dlg_error (pwfile, strerror(errno));
		return;
	}
	chmod (pwfile, S_IRUSR|S_IWUSR);
	len = strlen (pw);
	write (fd, pw, len);
	lseek (fd, 0, SEEK_SET);
	/* now we have the password saved */

	tmpnam (enfile);
#ifdef DEBUG
	printf ("cb_text_decrypt() enfile=%s\n", enfile);
#endif
	fp = fopen (enfile, "w");
	if (!fp) {
		gu_cursor_reset (w->parent);
		dlg_error (enfile, strerror(errno));
		close (fd);
		return;
	}
	chmod (enfile, S_IRUSR|S_IWUSR);
	/* write text to the tempfile */
	len = gtk_text_get_length (GTK_TEXT(pg->w_text));
	str = gtk_editable_get_chars (GTK_EDITABLE(pg->w_text), 0, -1);
	fwrite (str, 1, len, fp);
	fclose (fp);

	/* prepare command */
	sprintf (cmd,
		"gpg --batch --yes --passphrase-fd=%d --decrypt", fd);
	if (opt->quiet)
		strcat (cmd, " -q");
	strcat (cmd, " '");
	strcat (cmd, enfile);
	strcat (cmd, "'");

	/* redirect stderr */
	freopen (pg->app->opt->err_file, "w", stderr);

	/* decrypt */
	fp = popen (cmd, "r");
	if (!fp) {
		gu_cursor_reset (w->parent);
		dlg_error (_("Can't open pipe"), cmd);
		close (fd);
		return;
	}
	/* read the decrypted text back */
	gtk_text_freeze (GTK_TEXT(pg->w_text));
	gtk_text_set_point (GTK_TEXT(pg->w_text), 0);
	gtk_text_forward_delete (GTK_TEXT(pg->w_text), len);
	while (fgets (line, LINESIZE, fp)) {
		gtk_text_insert (GTK_TEXT(pg->w_text),NULL,NULL,NULL,line,strlen(line));
	}
	gtk_text_thaw (GTK_TEXT(pg->w_text));
	/* clean up */
	rc = pclose (fp);
	close (fd);
	unlink (pwfile);

	sync_error_widget (pg->app->opt->err_file, pg->app->error);
	if (rc != 0) {
		dlg_error (_("Error"), _("GPG decryption failed!"));
		/* read back the original text */
		fp = fopen (enfile, "r");
		while (fgets (line, LINESIZE, fp)) {
			gtk_text_insert (GTK_TEXT(pg->w_text),
				NULL,NULL,NULL,line, strlen(line));
		}
		fclose (fp);
	}
	unlink (enfile);
	gu_cursor_reset (w->parent);
}

/*
 * called if button "sign" on the text page is pressed
 */
static void
cb_text_sign (GtkWidget *w, gpointer *data)
{
	int fd;
	FILE *fp;
	int len, rc;
	char pw[DLG_MAX];
	char enfile[1024];
	char pwfile[1024];
	char cmd[2048];
	char line[LINESIZE];
	char *str;
	page_t *pg = (page_t *) data;
	opt_t *opt;

	opt = pg->app->opt;
	len = gtk_text_get_length (GTK_TEXT(pg->w_text));
	if (!len) {
		dlg_info (_("Nothing to sign!"));
		return;
	}
	pw[0] = '\0';
	if (dlg_password (_("Passphrase: "), pw) != DLG_OK) {
		return;
	}
	gu_cursor_wait (w->parent);
	tmpnam (pwfile);
#ifdef DEBUG
	printf ("cb_text_sign() pwfile=%s\n", pwfile);
#endif
	fd = open (pwfile, O_RDWR|O_CREAT);
	if (fd < 0) {
		gu_cursor_reset (w->parent);
		dlg_error (pwfile, strerror(errno));
		return;
	}
	chmod (pwfile, S_IRUSR|S_IWUSR);
	len = strlen (pw);
	write (fd, pw, len);
	lseek (fd, 0, SEEK_SET);
	/* now we have the password saved */

	tmpnam (enfile);
#ifdef DEBUG
	printf ("cb_text_sign() enfile=%s\n", enfile);
#endif
	fp = fopen (enfile, "w");
	if (!fp) {
		gu_cursor_reset (w->parent);
		dlg_error (enfile, strerror(errno));
		close (fd);
		return;
	}
	chmod (enfile, S_IRUSR|S_IWUSR);
	/* write text to the tempfile */
	len = gtk_text_get_length (GTK_TEXT(pg->w_text));
	str = gtk_editable_get_chars (GTK_EDITABLE(pg->w_text), 0, -1);
	fwrite (str, 1, len, fp);
	fclose (fp);

	/* prepare command */
	sprintf (cmd,
		"gpg --batch --yes --passphrase-fd=%d --clearsign", fd);
	if (opt->quiet)
		strcat (cmd, " -q");
	strcat (cmd, " -o - '");
	strcat (cmd, enfile);
	strcat (cmd, "'");

	/* redirect stderr */
	freopen (pg->app->opt->err_file, "w", stderr);

	/* sign */
	fp = popen (cmd, "r");
	if (!fp) {
		gu_cursor_reset (w->parent);
		dlg_error (_("Can't open pipe"), cmd);
		close (fd);
		return;
	}
	/* read the decrypted text back */
	gtk_text_freeze (GTK_TEXT(pg->w_text));
	gtk_text_set_point (GTK_TEXT(pg->w_text), 0);
	gtk_text_forward_delete (GTK_TEXT(pg->w_text), len);
	while (fgets (line, LINESIZE, fp)) {
		gtk_text_insert (GTK_TEXT(pg->w_text),NULL,NULL,NULL,line,strlen(line));
	}
	gtk_text_thaw (GTK_TEXT(pg->w_text));
	/* clean up */
	rc = pclose (fp);
	close (fd);
	unlink (pwfile);

	sync_error_widget (pg->app->opt->err_file, pg->app->error);
	if (rc != 0) {
		dlg_error (_("Error"), _("GPG signing failed!"));
		/* read back the original text */
		fp = fopen (enfile, "r");
		while (fgets (line, LINESIZE, fp)) {
			gtk_text_insert (GTK_TEXT(pg->w_text),
				NULL,NULL,NULL,line, strlen(line));
		}
		fclose (fp);
	}
	unlink (enfile);
	gu_cursor_reset (w->parent);
}

/*
 * called if button "save" on the text page is pressed
 */
static void
cb_text_save (GtkWidget *w, gpointer *data)
{
	char *file;
	FILE *fp;
	int len;
	char *str;
	page_t *pg = (page_t *)data;

	len = gtk_text_get_length (GTK_TEXT(pg->w_text));
	if (!len) {
		dlg_info (_("Nothing to save!"));
		return;
	}
	/* file name */
	file = fs_get_file(NULL);
	if (!file)
		return;
	/* override */
	if (io_is_file(file)) {
		if (dlg_question (_("Override file"), file) != DLG_OK)
			return;
	}
	fp = fopen (file, "w");
	if (!fp) {
		dlg_error (file, strerror(errno));
		return;
	}
	/* save */
	gu_cursor_wait (w->parent);
	str = gtk_editable_get_chars (GTK_EDITABLE(pg->w_text), 0, -1);
	fwrite (str, 1, len, fp);
	fclose (fp);
	gu_cursor_reset (w->parent);
}
	
/*
 * called if button "open" on the text page is pressed
 */
static void
cb_text_open (GtkWidget *w, gpointer *data)
{
	char *file;
	FILE *fp;
	int len;
	char str[LINESIZE+1];
	page_t *pg = (page_t *)data;
	GtkWidget *text = pg->w_text;

	file = fs_get_file(NULL);

	if (!file)
		return;

	fp = fopen (file, "r");
	if (!fp) {
		dlg_error (file, strerror(errno));
		g_free (file);
		return;
	}
	/* clear */
	len = gtk_text_get_length (GTK_TEXT(pg->w_text));
	gu_cursor_wait (w->parent);
	gtk_text_freeze (GTK_TEXT(text));
	gtk_text_set_point (GTK_TEXT(text), 0);
	gtk_text_forward_delete (GTK_TEXT(text), len);
	/* read */
	while (fgets (str, LINESIZE, fp)) {
			len = strlen(str);
			gtk_text_insert (GTK_TEXT(text),
					NULL, NULL, NULL, str, len);
	}
	gtk_text_thaw (GTK_TEXT(text));
	fclose (fp);
	gu_cursor_reset (w->parent);
	g_free (file);
}

/*
 * encrypt all files from the file list
 */
static void
cb_file_encrypt (GtkWidget *w, gpointer *data)
{
	page_t *pg = (page_t *) data;
	int numfiles, cmd_len, i, file_len, rc, row;
	char *cmd, *fname, *email, *fname_new;
	GList *list;
	GtkCList *clist = GTK_CLIST(pg->w_file);
	opt_t *opt;

	opt = pg->app->opt;
	/* check if the file list is empty */
	numfiles = clist->rows;
	if (!numfiles) {
		dlg_info (_("Nothing to encrypt!"));
		return;
	}
	gu_cursor_wait (w->parent);
	list = GTK_CLIST(pg->w_list)->selection;
	cmd_len = 0;
	for (list = GTK_CLIST(pg->w_list)->selection; list; list = list->next) {
		row = (int)list->data;
		gtk_clist_get_text (GTK_CLIST(pg->w_list), row, 2, &email);
		cmd_len += strlen (email) + 4;
	}
	if (!cmd_len) {
		gu_cursor_reset (w->parent);
		dlg_error(_("Error"),
			_("You have to mark at least one person in the key list!"));
		return;
	}
	cmd_len += 120;

	/* encrypt every file in the list
	 */
	for (i = 0; i < numfiles; i++) {
		gtk_clist_get_text (GTK_CLIST (pg->w_file), i, 0, &fname);
		file_len = strlen (fname);
		cmd = g_malloc (cmd_len + file_len);
		if (!cmd) {
			gu_cursor_reset (w->parent);
			return;
		}
		strcpy (cmd, "gpg --yes --batch --encrypt");
		if (opt->file_enc_armor)
			strcat (cmd, " -a");
		if (opt->quiet)
			strcat (cmd, " -q");
		for (list = GTK_CLIST(pg->w_list)->selection; list; list = list->next) {
			row = (int)list->data;
			gtk_clist_get_text (GTK_CLIST(pg->w_list), row, 2, &email);
			strcat (cmd, " -r ");
			strcat (cmd, email);
		}
		strcat (cmd, " ");
		strcat (cmd, fname);
		/* redirect stderr */
		freopen (pg->app->opt->err_file, "w", stderr);
		rc = system (cmd);
		sync_error_widget (pg->app->opt->err_file, pg->app->error);
		if (rc != 0) {
			dlg_error (fname, _("gnupg returned an error!"));
		} else {
			if (opt->file_enc_remove_org)
				unlink (fname);
			fname_new = g_malloc (file_len + 5);
			if (opt->file_enc_armor)
				sprintf (fname_new, "%s.asc", fname);
			else
				sprintf (fname_new, "%s.gpg", fname);
			gtk_clist_set_text (GTK_CLIST(pg->w_file), i, 0, fname_new);
			g_free (fname_new);
		}
		g_free (cmd);
	}
	gu_cursor_reset (w->parent);
}

/*
 */
static void
cb_file_decrypt (GtkWidget *w, gpointer *data)
{
	page_t *pg = (page_t *) data;
	int numfiles, rc, fd, len, i;
	char pw[DLG_MAX];
	char pwfile[1024];
	char cmd[2048];
	char *fname, *fname_new, *p;
	GtkCList *clist = GTK_CLIST(pg->w_file);
	opt_t *opt;

	opt = pg->app->opt;

	/* check if the file list is empty */
	numfiles = clist->rows;
	if (!numfiles) {
		dlg_info (_("Nothing to encrypt!"));
		return;
	}
	pw[0] = '\0';
	if (dlg_password (_("Passphrase: "), pw) != DLG_OK) {
		return;
	}
	gu_cursor_wait (w->parent);

	tmpnam (pwfile);
#ifdef DEBUG
	printf ("cb_file_decrypt() pwfile=%s\n", pwfile);
#endif
	fd = open (pwfile, O_RDWR|O_CREAT);
	if (fd < 0) {
		gu_cursor_reset (w->parent);
		dlg_error (pwfile, strerror(errno));
		return;
	}
	chmod (pwfile, S_IRUSR|S_IWUSR);
	len = strlen (pw);
	write (fd, pw, len);

	for (i = 0; i < numfiles; i++) {
		/* decrypt every file */
		lseek (fd, 0, SEEK_SET);
		gtk_clist_get_text (GTK_CLIST (pg->w_file), i, 0, &fname);
		fname_new = g_malloc (strlen(fname) + 10 + 1);
		strcpy (fname_new, fname);
		if (opt->file_enc_armor) {
			p = strrstr (fname_new, ".asc");
			if (p) *p = '\0';
			else strcat (fname_new, ".decrypted");
		} else {
			p = strrstr (fname_new, ".gpg");
			if (p) *p = '\0';
			else strcat (fname_new, ".decrypted");
		}
		sprintf (cmd,
				"gpg --batch --yes --passphrase-fd=%d --decrypt -o '%s' '%s'",
				fd, fname_new, fname);
		freopen (opt->err_file, "w", stderr);
		rc = system (cmd);
		sync_error_widget (opt->err_file, pg->app->error);
		if (rc != 0) {
			dlg_error (_("Error"), _("GnuPG returned an error!"));
		} else {
			if (opt->file_dec_remove_org)
				unlink (fname);
			gtk_clist_set_text (GTK_CLIST(pg->w_file), i, 0, fname_new);
		}
		g_free (fname_new);
	}
	close (fd);
	unlink (pwfile);
	gu_cursor_reset (w->parent);
}


/*
 */
static void
cb_file_cut (GtkWidget *w, gpointer *data)
{
	page_t *pg = (page_t *) data;
	GList *list;
	int num;

	list = GTK_CLIST(pg->w_file)->selection;
	while (list) {
		num = (int)list->data;
		gtk_clist_remove (GTK_CLIST(pg->w_file), num);
		list = GTK_CLIST(pg->w_file)->selection;
	}
}

/*
 */
static void
cb_check_btn (GtkWidget *w, gpointer *data)
{
	int *val = (int *) data;

	*val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
}



/*
 */
GtkWidget *
mk_key_list (GtkWidget *box)
{
	GtkWidget *scroll, *clist;
	FILE *fp;
	char buf[CSIZE], date[CSIZE], name[CSIZE], email[CSIZE], *p, *e;
	char *text[COLS];
	int i;

	text[0] = _("Date");
	text[1] = _("Name");
	text[2] = _("eMail");
	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX(box), scroll, TRUE, TRUE, 0);
	clist = gtk_clist_new_with_titles (COLS, text);
	gtk_container_add (GTK_CONTAINER(scroll), clist);
	for (i = 0; i < COLS; i++)
		gtk_clist_set_column_auto_resize (GTK_CLIST(clist), i, TRUE);
	gtk_clist_set_selection_mode (GTK_CLIST(clist), GTK_SELECTION_MULTIPLE);

	fp = popen ("gpg --list-keys | grep ^pub", "r");
	if (!fp)
		return NULL;
	text[0] = date;
	text[1] = name;
	text[2] = email;
	while (fgets (buf, CSIZE, fp)) {
		p = strchr (buf+8, ' ');
		if (p) {
			*date = *name = *email = '\0';
			e = strchr (p+1, ' ');
			if (!e)
				continue;
			*e = '\0';
			strcpy (date, p+1);
			p = e+1;

			e = strchr (p+1, '<');
			if (!e)
				continue;
			e--;
			*e = '\0';
			strcpy (name, p);
			p = e+1;

			e = strchr (p+1, '>');
			if (!e)
				continue;
			*e = '\0';
			strcpy (email, p+1);
			p = e+1;
			gtk_clist_append (GTK_CLIST(clist), text);
		}
	}
	pclose (fp);
	return clist;
}

/*
 * save options
 */
static void
cb_save (GtkWidget *btn, void *data)
{
	opt_save ((opt_t *)data);
}

/*
 * create the text en-/decrypt page
 */
static void
mk_text_notebook (cfg_t *app, GtkWidget *menu, GdkColormap *cmap)
{
	page_t *pg;
	GtkWidget *box, *scroll, *toolbar, *icon;

	pg = malloc (sizeof (page_t));
	if (!pg)
		return;
	pg->app = app;
	pg->w_box = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER(pg->w_box), CBORDER);
	pg->w_label = gtk_label_new (_("Text En-/Decryption"));

	gtk_drag_dest_set (pg->w_label, GTK_DEST_DEFAULT_ALL,
			target_table, NUM_TARGETS, GDK_ACTION_COPY);
	gtk_signal_connect (GTK_OBJECT(pg->w_label), "drag_motion",
			GTK_SIGNAL_FUNC(page_drag_motion), (void *) PG_TEXT_EN);
	gtk_signal_connect (GTK_OBJECT(pg->w_label), "drag_data_received",
			GTK_SIGNAL_FUNC(text_drop_data), (void *) pg);

	gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook),pg->w_box,pg->w_label);
	gtk_object_set_user_data (GTK_OBJECT(pg->w_label), pg);

	pg->w_text = gtk_text_new (NULL, NULL);

	toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
	gtk_box_pack_start (GTK_BOX(pg->w_box), toolbar, FALSE, FALSE, 0);

	box = pg->w_box;
	icon = icon_widget (box->window, &box->style->bg[GTK_STATE_NORMAL],
			cmap, xpg_encrypt_xpm);
	gtk_toolbar_append_item (GTK_TOOLBAR(toolbar),
		_("Encrypt"), _("Encrypt text with GnuPG"), "Toolbar/Encrypt",
		icon, (GtkSignalFunc)cb_text_encrypt, pg);
	icon = icon_widget (box->window, &box->style->bg[GTK_STATE_NORMAL],
			cmap, xpg_decrypt_xpm);
	gtk_toolbar_append_item (GTK_TOOLBAR(toolbar),
		_("Decrypt"), _("Decrypt text with GnuPG"), "Toolbar/Decrypt",
		icon, (GtkSignalFunc)cb_text_decrypt, pg);
	icon = icon_widget (box->window, &box->style->bg[GTK_STATE_NORMAL],
			cmap, xpg_sign_xpm);
	gtk_toolbar_append_item (GTK_TOOLBAR(toolbar),
		_("Sign"), _("Sign Text with GnuPG"), "Toolbar/Sign",
		icon, (GtkSignalFunc)cb_text_sign, pg);
	icon = icon_widget (box->window, &box->style->bg[GTK_STATE_NORMAL],
			cmap, xpg_save_xpm);
	gtk_toolbar_append_item (GTK_TOOLBAR(toolbar),
		_("Save"), _("Save text to a file"), "Toolbar/Save",
		icon, (GtkSignalFunc)cb_text_save, pg);
	icon = icon_widget (box->window, &box->style->bg[GTK_STATE_NORMAL],
			cmap, xpg_open_xpm);
	gtk_toolbar_append_item (GTK_TOOLBAR(toolbar),
		_("Open"), _("Open and read a file"), "Toolbar/Open",
		icon, (GtkSignalFunc)cb_text_open, pg);

	pg->w_list = mk_key_list (box);

	box = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX(pg->w_box), box, TRUE, TRUE, 0);

	gtk_text_set_editable (GTK_TEXT(pg->w_text), TRUE);
	gtk_drag_dest_set (pg->w_text, GTK_DEST_DEFAULT_ALL,
			target_table, NUM_TARGETS, GDK_ACTION_COPY);
	gtk_signal_connect (GTK_OBJECT(pg->w_text), "drag_data_received",
			GTK_SIGNAL_FUNC(text_drop_data), (void *) pg);

	gtk_box_pack_start (GTK_BOX(box), pg->w_text, TRUE, TRUE, 0);
	scroll = gtk_vscrollbar_new (GTK_TEXT(pg->w_text)->vadj);
	gtk_box_pack_start (GTK_BOX(box), scroll, FALSE, FALSE, 0);
	gtk_widget_show_all (GTK_WIDGET(app->notebook));
}

/*
 * make file en-/decrypt page
 */
page_t *
mk_file_notebook (cfg_t *app, GtkWidget *menu, GdkColormap *cmap)
{
	page_t *pg;
	GtkWidget *toolbar, *box, *icon, *scroll, *clist;
	char *text[1];

	pg = malloc (sizeof (page_t));
	if (!pg)
		return NULL;
	pg->app = app;
	pg->w_box = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER(pg->w_box), CBORDER);
	pg->w_label = gtk_label_new (_("File En-/Decryption"));

	gtk_drag_dest_set (pg->w_label, GTK_DEST_DEFAULT_ALL,
			target_table, NUM_TARGETS, GDK_ACTION_COPY);
	gtk_signal_connect (GTK_OBJECT(pg->w_label), "drag_motion",
			GTK_SIGNAL_FUNC(page_drag_motion), (void *) PG_FILE_EN);
	gtk_signal_connect (GTK_OBJECT(pg->w_label), "drag_data_received",
			GTK_SIGNAL_FUNC(file_drop_data), (void *) pg);

	gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook),pg->w_box,pg->w_label);
	gtk_object_set_user_data (GTK_OBJECT(pg->w_label), pg);

	toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
	gtk_box_pack_start (GTK_BOX(pg->w_box), toolbar, FALSE, FALSE, 0);

	box = pg->w_box;
	icon = icon_widget (box->window, &box->style->bg[GTK_STATE_NORMAL],
			cmap, xpg_encrypt_xpm);
	gtk_toolbar_append_item (GTK_TOOLBAR(toolbar),
		_("Encrypt"), _("Encrypt listed file(s) with GnuPG"), "Toolbar/Encrypt",
		icon, (GtkSignalFunc)cb_file_encrypt, pg);

	icon = icon_widget (box->window, &box->style->bg[GTK_STATE_NORMAL],
			cmap, xpg_decrypt_xpm);
	gtk_toolbar_append_item (GTK_TOOLBAR(toolbar),
		_("Decrypt"), _("Decrypt listed file(s) with GnuPG"), "Toolbar/Decrypt",
		icon, (GtkSignalFunc)cb_file_decrypt, pg);

	icon = icon_widget (box->window, &box->style->bg[GTK_STATE_NORMAL],
			cmap, xpg_cut_xpm);
	gtk_toolbar_append_item (GTK_TOOLBAR(toolbar),
		_("Cut"), _("Cut marked files from the list"), "Toolbar/Cut",
		icon, (GtkSignalFunc)cb_file_cut, pg);


	/* key list */
	pg->w_list = mk_key_list (pg->w_box);

	/* file list */
	text[0] = _("Files");
	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX(box), scroll, TRUE, TRUE, 0);
	pg->w_file = clist = gtk_clist_new_with_titles (1, text);
	gtk_container_add (GTK_CONTAINER(scroll), clist);
	gtk_clist_set_column_auto_resize (GTK_CLIST(clist), 0, TRUE);
	gtk_clist_set_selection_mode (GTK_CLIST(clist), GTK_SELECTION_MULTIPLE);

	gtk_drag_dest_set (pg->w_file, GTK_DEST_DEFAULT_ALL,
			target_table, NUM_TARGETS, GDK_ACTION_COPY);
	gtk_signal_connect (GTK_OBJECT(pg->w_file), "drag_data_received",
			GTK_SIGNAL_FUNC(file_drop_data), (void *) pg);

	gtk_widget_show_all (GTK_WIDGET(app->notebook));
	return pg;
}

/*
 *
 */
void
mk_options_notebook (cfg_t *app, GtkWidget *menu, char *path)
{
	page_t *pg;
	int len;
	char *file;
	GtkWidget *box, *frame, *check, *obox, *btn;

	pg = malloc (sizeof (page_t));
	if (!pg)
		return;
	pg->app = app;
	pg->w_box = box = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER(pg->w_box), CBORDER);
	pg->w_label = gtk_label_new (_("Options"));


	gtk_drag_dest_set (pg->w_label, GTK_DEST_DEFAULT_ALL,
			target_table, NUM_TARGETS, GDK_ACTION_COPY);
	gtk_signal_connect (GTK_OBJECT(pg->w_label), "drag_motion",
			GTK_SIGNAL_FUNC(page_drag_motion), (void *) PG_OPTIONS);

	gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook),pg->w_box,pg->w_label);
	gtk_object_set_user_data (GTK_OBJECT(pg->w_label), pg);

	/* read the config file */
	len = strlen (path);
	file = malloc (len + 20);
	if (!file) {
		perror ("malloc()");
		return;
	}
	sprintf (file, "%s/xpg.scf", path);
	app->opt = opt_read (file);
	if (!app->opt) {
		/* too less memory -> exit */
		return;
	}

	/* build up the gui elements */
	frame = gtk_frame_new (_("General"));
	gtk_box_pack_start (GTK_BOX(box), frame, FALSE, TRUE, 2);
	obox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER(frame), obox);

	check = gtk_check_button_new_with_label (
				_("Quiet mode"));
	if (app->opt->quiet)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check), 1);
	gtk_signal_connect (GTK_OBJECT(check), "clicked",
			GTK_SIGNAL_FUNC(cb_check_btn), (void *) &app->opt->quiet);
	gtk_box_pack_start (GTK_BOX(obox), check, FALSE, TRUE, 2);


	frame = gtk_frame_new (_("Encryption"));
	gtk_box_pack_start (GTK_BOX(box), frame, FALSE, TRUE, 2);
	obox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER(frame), obox);

	check = gtk_check_button_new_with_label (
				_("Create ASCII armored output when encrypting files"));
	if (app->opt->file_enc_armor)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check), 1);
	gtk_signal_connect (GTK_OBJECT(check), "clicked",
			GTK_SIGNAL_FUNC(cb_check_btn), (void *) &app->opt->file_enc_armor);
	gtk_box_pack_start (GTK_BOX(obox), check, FALSE, TRUE, 2);

	check = gtk_check_button_new_with_label (
				_("Delete original file after encryption"));
	if (app->opt->file_enc_remove_org)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check), 1);
	gtk_signal_connect (GTK_OBJECT(check), "clicked",
		GTK_SIGNAL_FUNC(cb_check_btn), (void *) &app->opt->file_enc_remove_org);
	gtk_box_pack_start (GTK_BOX(obox), check, FALSE, TRUE, 2);

	frame = gtk_frame_new (_("Decryption"));
	gtk_box_pack_start (GTK_BOX(box), frame, FALSE, TRUE, 2);
	obox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER(frame), obox);

	check = gtk_check_button_new_with_label (
				_("Delete original file after decryption"));
	if (app->opt->file_dec_remove_org)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check), 1);
	gtk_signal_connect (GTK_OBJECT(check), "clicked",
		GTK_SIGNAL_FUNC(cb_check_btn),(void *)&app->opt->file_dec_remove_org);
	gtk_box_pack_start (GTK_BOX(obox), check, FALSE, TRUE, 2);

	obox = gtk_hbox_new (FALSE, 10);
	gtk_box_pack_start (GTK_BOX(box), obox, FALSE, FALSE, 10);
	btn = gtk_button_new_with_label (_("Save options"));
	gtk_box_pack_start (GTK_BOX(obox), btn, FALSE, FALSE, 1);
	gtk_signal_connect (GTK_OBJECT(btn), "clicked",
		GTK_SIGNAL_FUNC(cb_save),(void *)app->opt);

	gtk_widget_show_all (GTK_WIDGET(app->notebook));
}

/*
 *
 */
void
mk_about_notebook (cfg_t *app, GtkWidget *menu)
{
	page_t *pg;
	GtkWidget *label;

	pg = malloc (sizeof (page_t));
	pg->w_box = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER(pg->w_box), CBORDER);
	pg->w_label = gtk_label_new (_("About"));

	gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook),pg->w_box,pg->w_label);
	gtk_object_set_user_data (GTK_OBJECT(pg->w_label), pg);
	label = gtk_label_new (
	_("\nX Privacy Guard\n(c) Rasca, Berlin 2000\nrasca.home.pages.de/xap/\n"));
	gtk_box_pack_start (GTK_BOX(pg->w_box), label, FALSE, FALSE, 0);
	label = gtk_label_new (
	_("XPG is part of the XAP package and is published under the GNU GPL.\n"));
	gtk_box_pack_start (GTK_BOX(pg->w_box), label, FALSE, FALSE, 0);
	label = gtk_label_new (
	_("To use this program you need a running GnuPG installation!"));
	gtk_box_pack_start (GTK_BOX(pg->w_box), label, FALSE, FALSE, 0);
	label = gtk_label_new (
	_("For more information about GnuPG see www.gnupg.org."));
	gtk_box_pack_start (GTK_BOX(pg->w_box), label, FALSE, FALSE, 0);
	gtk_widget_show_all (GTK_WIDGET(app->notebook));

}

/*
 * execute a named program
 */
static void
cb_exec (GtkWidget *w, gpointer data)
{
	dlg_execute ((char *)data, NULL);
}

/*
 * call on exit
 */
void
cb_exit (GtkWidget *w, gpointer data)
{
	cfg_t *app;

	app = (cfg_t *) data;
#ifdef DEBUG
	printf ("cb_exit()\n");
#endif
	exit(0);
}

/*
 * path: default value = "$HOME/.xap"
 */
void
gui_main (char *path, wgeo_t *geo, int nb_tabpos, int active,
	char **files, int num)
{
	GtkWidget *top, *page_menu, *menu_item;
	GtkWidget *notebook, *paned;
	GtkAccelGroup *accel;
	page_t *file_page;
	char err_file[1024];

	int i;
	cfg_t app;

#define LAST_BUTTON_MENU (sizeof(button_me)/sizeof(menu_entry))
	menu_entry page_me[] = {
		{ NULL,		NULL,	NULL },
		{ _("Execute .."),		cb_exec, 		path },
		{ NULL,		NULL, 	NULL },
		{ _("Quit"),		gtk_main_quit,	NULL, ACCEL, GDK_q, GDK_MOD1_MASK },
	};
#define LAST_PAGE_MENU (sizeof(page_me)/sizeof(menu_entry))

	/* create toplevel etc..
	 */
	top = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_name (top, "xpg");
	gtk_signal_connect (GTK_OBJECT(top), "destroy",
		GTK_SIGNAL_FUNC (cb_exit), (void *) &app);

	accel = gtk_accel_group_new();
	gtk_accel_group_attach (accel, GTK_OBJECT(top));

	paned = gtk_vpaned_new ();
	gtk_container_add (GTK_CONTAINER(top), paned);
	notebook = gtk_notebook_new ();
	gtk_notebook_set_homogeneous_tabs ((GtkNotebook *)notebook, 0);
	gtk_notebook_set_tab_hborder ((GtkNotebook *)notebook, 0);
	gtk_notebook_set_tab_vborder ((GtkNotebook *)notebook, 0);
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK(notebook), nb_tabpos);
	gtk_container_set_border_width (GTK_CONTAINER(notebook), 0);
	gtk_paned_add1 (GTK_PANED(paned), notebook);

	app.notebook = notebook;

	page_menu = gtk_menu_new ();
	gtk_menu_set_accel_group (GTK_MENU(page_menu), accel);
	for (i = 0; i < LAST_PAGE_MENU; i++) {
		if (page_me[i].label)
			menu_item = gtk_menu_item_new_with_label (page_me[i].label);
		else
			menu_item = gtk_menu_item_new ();
		if (page_me[i].func) {
			if (page_me[i].data) {
				gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
					GTK_SIGNAL_FUNC(page_me[i].func), page_me[i].data);
			} else {
				gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
					GTK_SIGNAL_FUNC(page_me[i].func), (void *)&app);
			}
		}
		if (page_me[i].flags & ACCEL) {
			gtk_widget_add_accelerator(menu_item, "activate", accel,
				page_me[i].key, page_me[i].mod, GTK_ACCEL_VISIBLE);
		}
		gtk_menu_append (GTK_MENU(page_menu), menu_item);
		gtk_widget_show (menu_item);
	}
	gtk_menu_attach_to_widget (GTK_MENU(page_menu), GTK_WIDGET(notebook),
			menu_detach);

	gtk_notebook_popup_enable (GTK_NOTEBOOK(notebook));
	GTK_NOTEBOOK(notebook)->menu = page_menu;

	/* */
	app.error = gtk_text_new (NULL, NULL);
	gtk_paned_add2 (GTK_PANED(paned), app.error);

	gtk_widget_realize (top);
	app.cmap = gtk_widget_get_colormap (top);

	mk_text_notebook (&app, page_menu, app.cmap);
	file_page = mk_file_notebook (&app, page_menu, app.cmap);
	mk_options_notebook (&app, page_menu, path);
	mk_about_notebook (&app, page_menu);

	gtk_notebook_set_page (GTK_NOTEBOOK(notebook), active);
	if (file_page) {
		for (i = 0; i < num; i++) {
			gtk_clist_append (GTK_CLIST(file_page->w_file), files+i);
		}
	}
	/* for the standard error output of the called gpg application
	 */
	tmpnam (err_file);
	app.opt->err_file = err_file;
#ifdef DEBUG
	printf ("gui_main() err_file=%s\n", err_file);
#endif

	gtk_widget_show_all (top);
	gtk_window_set_default_size (GTK_WINDOW(top), geo->width, geo->height);
	gtk_main ();
	unlink (err_file);
}

