/* MiniDLNA media server
 * Copyright (C) 2008-2010 NETGEAR, Inc. All Rights Reserved.
 *
 * This file is part of MiniDLNA.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/syslog.h>

#include "log.h"

#if 0
static FILE *log_fp = NULL;
#endif
static int default_log_level = E_INFO;
int log_level[L_MAX];

char *facility_name[] = {
	"general",
	"artwork",
	"database",
	"inotify",
	"scanner",
	"metadata",
	"http",
	"ssdp",
	"tivo",
	0
};

char *level_name[] = {
	"off",					// E_OFF
	"fatal",				// E_FATAL
	"error",				// E_ERROR
	"warn",					// E_WARN
	"info",					// E_INFO
	"debug",				// E_DEBUG
	0
};

#if 0
int
log_init(const char *fname, const char *debug)
{
	int i;
	FILE *fp;
	short int log_level_set[L_MAX];
	int openlog_option;

	if (debug)
	{
		const char *rhs, *lhs, *nlhs, *p;
		int n;
		int level, facility;
		memset(&log_level_set, 0, sizeof(log_level_set));
		rhs = nlhs = debug;
		while (rhs && (rhs = strchr(rhs, '='))) {
			rhs++;
			p = strchr(rhs, ',');
			n = p ? p - rhs : strlen(rhs);
			for (level=0; level_name[level]; level++) {
				if (!(strncasecmp(level_name[level], rhs, n)))
					break;
			}
			lhs = nlhs;
			rhs = nlhs = p;
			if (!(level_name[level])) {
				// unknown level
				continue;
			}
			do {
				if (*lhs==',') lhs++;
				p = strpbrk(lhs, ",=");
				n = p ? p - lhs : strlen(lhs);
				for (facility=0; facility_name[facility]; facility++) {
					if (!(strncasecmp(facility_name[facility], lhs, n)))
						break;
				}
				if ((facility_name[facility])) {
					log_level[facility] = level;
					log_level_set[facility] = 1;
				}
				lhs = p;
			} while (*lhs && *lhs==',');
		}
		for (i=0; i<L_MAX; i++)
		{
			if( !log_level_set[i] )
			{
				log_level[i] = default_log_level;
			}
		}
	}
	else {
		for (i=0; i<L_MAX; i++)
			log_level[i] = default_log_level;
	}

	if (!fname)					// use default i.e. stdout
		return 0;

	if (!(fp = fopen(fname, "a")))
		return 1;
	log_fp = fp;

	return 0;
}

void
log_err(int level, enum _log_facility facility, char *fname, int lineno, char *fmt, ...)
{
	//char errbuf[1024];
	char * errbuf;
	va_list ap;
	time_t t;
	struct tm *tm;

	if (level && level>log_level[facility] && level>E_FATAL)
		return;

	// user log
	va_start(ap, fmt);

	if (vasprintf(&errbuf, fmt, ap) == -1)
	{
		va_end(ap);
		return;
	}
	va_end(ap);

	// timestamp
	t = time(NULL);
	tm = localtime(&t);
	fprintf(log_fp, "[%04d/%02d/%02d %02d:%02d:%02d] ",
	        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
	        tm->tm_hour, tm->tm_min, tm->tm_sec);

	if (level)
		fprintf(log_fp, "%s:%d: %s: %s\n", fname, lineno, level_name[level], errbuf);
	else
		fprintf(log_fp, "%s:%d: %s\n", fname, lineno, errbuf);
	fflush(log_fp);
	free(errbuf);

	if (level==E_FATAL)
		exit(-1);

	return;
}
#else

int
log_init(const char *fname, const char *debug)
{
	int i;
//	FILE *fp;
//	short int log_level_set[L_MAX];
	int openlog_option;

	for (i=0; i<L_MAX; i++)
		log_level[i] = default_log_level;

	openlog_option = LOG_CONS;
	if(debug)
	{
//		openlog_option |= LOG_PID;		/* also add pid in log */
		openlog_option |= LOG_PERROR;	/* also log on stderr */
	}

	openlog("minidlna", openlog_option, LOG_DAEMON);

	if(!debug)
	{
		/* speed things up and ignore LOG_INFO and LOG_DEBUG */
		setlogmask(LOG_UPTO(LOG_NOTICE));
	}

	return 0;
}

void
log_err(int level, enum _log_facility facility, char *fname, int lineno, char *fmt, ...)
{
	//char errbuf[1024];
	char * errbuf;
	va_list ap;
	int syslog_level = LOG_DEBUG;

	if (level && level>log_level[facility] && level>E_FATAL)
		return;

	// user log
	va_start(ap, fmt);
	if (vasprintf(&errbuf, fmt, ap) == -1)
	{
		va_end(ap);
		return;
	}
	va_end(ap);

	switch (level) {
		case E_OFF:
		case E_FATAL:
		case E_ERROR:
			syslog_level = LOG_ERR;
		break;
		case E_WARN:
		case E_INFO:
			syslog_level = LOG_INFO;
		break;
		default:
			syslog_level = LOG_DEBUG;
	};

	if (level == E_DEBUG)
		syslog(syslog_level, "%s:%d: %s: %s", fname, lineno, level_name[level], errbuf);
	else
		syslog(syslog_level, "%s", errbuf);

	free(errbuf);

	if (level==E_FATAL)
		exit(-1);

	return;
}

#endif
