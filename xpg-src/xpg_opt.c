/*
 * xpg_opt.c
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
#include <string.h>
#include "xpg_opt.h"

#define LINESIZE 1024

/*
 * read configuration file
 */
opt_t *
opt_read (char *file)
{
	FILE *fp;
	opt_t *opt = NULL;
	char line[LINESIZE];
	char *p, *key, *val;
	int length;

	if (!file || !*file)
		return NULL;
	opt = malloc (sizeof (opt_t));
	if (!opt)
		return NULL;

	/* default values */
	opt->quiet = 0;
	opt->file_enc_armor = 0;
	opt->file_enc_remove_org = 0;
	opt->file_dec_remove_org = 0;
	opt->file = file;

	fp = fopen (file, "r");
	if (!fp) {
		/* perror (file); */
		return opt;
	}
	while (fgets(line, LINESIZE, fp)) {
		if (*line == '#')
			continue;
		if (*line == '\n')
			continue;
		p = strchr (line, '=');
		if (!p)
			continue;
		length = strlen (line);
		line[length-1] = '\0';
		key = line;
		val = p+1;
		*p = '\0';
		if (strcasecmp (key, "quiet") == 0)
			opt->quiet = atoi(val);
		else
		if (strcasecmp (key, "file_enc_armor") == 0)
			opt->file_enc_armor = atoi(val);
		else
		if (strcasecmp (key, "file_enc_remove_org") == 0)
			opt->file_enc_remove_org = atoi(val);
		else
		if (strcasecmp (key, "file_dec_remove_org") == 0)
			opt->file_dec_remove_org = atoi(val);
	}
	fclose (fp);
	return opt;
}

/*
 * save options to the configuration file
 */
int
opt_save (opt_t *opt)
{
	FILE *fp;

	if (!opt)
		return -1;
	fp = fopen (opt->file, "w");
	if (!fp)
		return -1;
	fprintf (fp, "# don't edit!\n");
	fprintf (fp, "quiet=%d\n", opt->quiet);
	fprintf (fp, "file_enc_armor=%d\n", opt->file_enc_armor);
	fprintf (fp, "file_enc_remove_org=%d\n", opt->file_enc_remove_org);
	fprintf (fp, "file_dec_remove_org=%d\n", opt->file_dec_remove_org);
	fclose (fp);
	return 0;
}

