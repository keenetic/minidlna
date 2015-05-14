#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include "dirent.h"

/* Calculate the required buffer size (in bytes) for directory       *
 * entries read from the given directory handle.  Return -1 if this  *
 * this cannot be done.                                              *
 *                                                                   *
 * This code does not trust values of NAME_MAX that are less than    *
 * 255, since some systems (including at least HP-UX) incorrectly    *
 * define it to be a smaller value.                                  *
 *                                                                   *
 * If you use autoconf, include fpathconf and dirfd in your          *
 * AC_CHECK_FUNCS list.  Otherwise use some other method to detect   *
 * and use them where available.                                     *
 * For details see                                                   *
 * http://womble.decadent.org.uk/readdir_r-advisory.html             */

struct dirent *
dirent_allocate(DIR *dirp)
{
	long name_max;
	size_t name_end;
#   if defined(HAVE_FPATHCONF) && defined(HAVE_DIRFD) \
	&& defined(_PC_NAME_MAX)
	name_max = fpathconf(dirfd(dirp), _PC_NAME_MAX);
	if (name_max == -1)
#if defined(NAME_MAX)
		name_max = (NAME_MAX > 255) ? NAME_MAX : 255;
#else
	return NULL;

#endif
#else
#if defined(NAME_MAX)
	name_max = (NAME_MAX > 255) ? NAME_MAX : 255;
#else
#error "buffer size for readdir_r cannot be determined"
#endif
#endif
	name_end = (size_t)offsetof(struct dirent, d_name) + name_max + 1;

	return (struct dirent*) malloc(
		name_end > sizeof(struct dirent) ?
		name_end : sizeof(struct dirent));
}

void
dirent_free(struct dirent *dentry)
{
	free(dentry);
}

