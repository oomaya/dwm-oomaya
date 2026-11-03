/* oomaya_audio_watch.h — one `pactl subscribe` child feeding the reactor. */
#ifndef OOMAYA_AUDIO_WATCH_H
#define OOMAYA_AUDIO_WATCH_H

#include <sys/types.h>

#define OOMAYA_AUDIO_WATCH_CARRY 16

/* Start the child. Returns a non-blocking read fd and sets *pid, or -1. */
int oomaya_audio_watch_start(pid_t *pid);

/* Close fd, then kill and reap the child. Safe with fd -1 or pid 0. */
void oomaya_audio_watch_stop(int fd, pid_t pid);

/* Drain fd. Returns 1 if an audio change was seen, 0 if not, -1 at EOF. */
int oomaya_audio_watch_read(int fd, char *carry);

#endif /* OOMAYA_AUDIO_WATCH_H */
