#ifndef __MONITOR_H__
#define __MONITOR_H__

#include <signal.h>
#include <pthread.h>

#ifdef HAVE_INOTIFY
void start_inotify_thread(pthread_t *inotify_thread);
void stop_inotify_thread(pthread_t *inotify_thread);
#else
static inline void start_inotify_thread(pthread_t *inotify_thread){};
static inline void stop_inotify_thread(pthread_t *inotify_thread){};
#endif

int monitor_insert_directory(int fd, char *name, const char * path);
int monitor_remove_file(const char * path);
int monitor_remove_directory(int fd, const char * path);

#if defined(HAVE_INOTIFY) && defined(HAVE_KQUEUE)
#define HAVE_WATCH 1
int add_watch(int, const char *);
#endif

#ifdef HAVE_KQUEUE
void kqueue_monitor_start(void);
#endif

#endif //__MONITOR_H__
