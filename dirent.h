#ifndef __READDIR_H__
#define __READDIR_H__

#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

struct dirent *
dirent_allocate(DIR *dirp);

void
dirent_free(struct dirent *dentry);

#endif

