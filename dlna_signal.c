#include <signal.h>
#include <string.h>
#include "dlna_signal.h"

void
dlna_signal_block(int sig)
{
	sigset_t blocked;

	sigemptyset(&blocked);
	sigaddset(&blocked, sig);
	pthread_sigmask(SIG_BLOCK, &blocked, NULL);
}

void
dlna_signal_unblock(int sig)
{
	sigset_t unblocked;

	sigemptyset(&unblocked);
	sigaddset(&unblocked, sig);
	pthread_sigmask(SIG_UNBLOCK, &unblocked, NULL);
}

