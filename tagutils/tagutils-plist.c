//=========================================================================
// FILENAME	: playlist.c
// DESCRIPTION	: Playlist
//=========================================================================
// Copyright (c) 2008- NETGEAR, Inc. All Rights Reserved.
//=========================================================================

/*
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "misc.h"
#include "tagutils.h"
#include "textutils.h"
// #include "log.h"

#include <nconv.h>


#define MAX_BUF 4096

static FILE *fp = 0;
static int _utf8bom = 0;
static int _trackno;

static int (*_next_track)(struct song_metadata*, struct stat*, char*, char*);
static int _m3u_pls_next_track(struct song_metadata*, struct stat*, char*, char*);

int
start_plist(const char *path, struct song_metadata *psong, struct stat *stat, char *lang, char *type)
{
	char *fname, *suffix;

	_next_track = 0;
	_utf8bom = 0;
	_trackno = 0;

	if(strcasecmp(type, "m3u") == 0)
		_next_track = _m3u_pls_next_track;
	else if(strcasecmp(type, "pls") == 0)
		_next_track = _m3u_pls_next_track;

	if(!_next_track)
	{
		DPRINTF(E_ERROR, L_SCANNER, "Unsupported playlist type <%s> (%s)\n", type, path);
		return -1;
	}

	if(!(fp = fopen(path, "rb")))
	{
		DPRINTF(E_ERROR, L_SCANNER, "Cannot open %s\n", path);
		return -1;
	}

	if(!psong)
		return 0;

	memset((void*)psong, 0, sizeof(struct song_metadata));
	psong->is_plist = 1;
	psong->path = strdup(path);
	psong->type = type;

	fname = strrchr(psong->path, '/');
	psong->basename = fname ? fname + 1 : psong->path;

	psong->title = strdup(psong->basename);
	suffix = strrchr(psong->title, '.');
	if(suffix) *suffix = '\0';

	if(stat)
	{
		if(!psong->time_modified)
			psong->time_modified = stat->st_mtime;
		psong->file_size = stat->st_size;
	}

	return 0;
}

char *utf8_fgets(char *pbuf, int isize, FILE *fin) {
	char ctb[MAX_BUF / 4];
	nconv_t cnv;
	size_t inbytes, outbytes;
	char 	*in, *out;
	
	/* no conversion needed */
	if( _utf8bom ) return fgets(pbuf, isize, fin);
	
	if( isize <= 0 || !(fgets(ctb, sizeof(ctb) - 1, fin)) ) return NULL;
	ctb[sizeof(ctb) - 1] = 0;
	
	/* test conversion need */
	if( !_utf8bom ) _utf8bom = is_utf8(ctb);
	
	/* fallback */
	if( _utf8bom || (cnv = nconv_open("utf-8", "cp1251")) == (nconv_t)-1 ) {
		strncpy(pbuf, ctb, isize - 1);
		pbuf[isize - 1] = 0;
		return pbuf;
	}
	
	in = ctb;
	out = pbuf;
	
	inbytes = strlen(ctb) + 1;
	outbytes = isize;
	nconv(cnv, &in, &inbytes, &out, &outbytes);
	pbuf[isize - 1] = 0;
	
	nconv_close(cnv);
	return pbuf;
}

int
_m3u_pls_next_track(struct song_metadata *psong, struct stat *stat, char *lang, char *type)
{
	char buf[MAX_BUF], *p;
	int len;

	memset((void*)psong, 0, sizeof(struct song_metadata));

	// read first line
	p = utf8_fgets(buf, MAX_BUF, fp);
	if(!p)
	{
		fclose(fp);
		return 1;
	}

	while(p)
	{
		while(isspace(*p)) p++;

		if(!(*p) || *p == '#')
			goto next_line;

		if(!isprint(*p))
		{
			DPRINTF(E_ERROR, L_SCANNER, "Playlist looks bad (unprintable characters)\n");
			fclose(fp);
			return 2;
		}

		if(strcasecmp(type, "pls") == 0)
		{
			// verify that it's a valid pls playlist
			if(!_trackno)
			{
				if(strncmp(p, "[playlist]", 10))
					break;
				_trackno++;
				goto next_line;
			}

			if(strncmp(p, "File", 4) != 0)
				goto next_line;

			psong->track = strtol(p+4, &p, 10);
			if(!psong->track || !p++)
				goto next_line;
			_trackno = psong->track;
		}
		else if(strcasecmp(type, "m3u") == 0)
			psong->track = ++_trackno;

		len = strlen(p);
		while(p[len-1] == '\r' || p[len-1] == '\n')
			p[--len] = '\0';
		psong->path = strdup(p);
		return 0;
next_line:
		p = utf8_fgets(buf, MAX_BUF, fp);
	}

	fclose(fp);
	return 1;
}

int
next_plist_track(struct song_metadata *psong, struct stat *stat, char *lang, char *type)
{
	if(_next_track)
		return _next_track(psong, stat, lang, type);
	return -1;
}
