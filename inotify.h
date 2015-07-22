#ifndef __INOTIFY_H__
#define __INOTIFY_H__

#include <signal.h>

#ifdef HAVE_INOTIFY
void start_inotify_thread(pthread_t *inotify_thread);
void stop_inotify_thread(pthread_t *inotify_thread);
#else
static inline void start_inotify_thread(pthread_t *inotify_thread){};
static inline void stop_inotify_thread(pthread_t *inotify_thread){};
#endif

#endif //__INOTIFY_H__
