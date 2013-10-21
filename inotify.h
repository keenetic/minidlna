#ifndef __INOTIFY_H__
#define __INOTIFY_H__

int
inotify_insert_directory(int fd, char *name, const char * path);

int
inotify_remove_file(const char * path);

void *
start_inotify();

int
inotify_remove_directory(int fd, const char * path);

#endif	// __INOTIFY_H__
