/* audio_watch.c — Audio change source for IPC_SIG_SYS_AUDIO_CHANGED.
 *
 * Reads `pactl subscribe` through a pipe, so the reactor can watch it with
 * epoll and no audio library is linked. The child runs only while a client is
 * subscribed.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "../../include/oomaya_audio_watch.h"

extern char **environ;

int
oomaya_audio_watch_start(pid_t *pid)
{
    /* LC_ALL=C keeps the event text matchable. env execs pactl, so the pid
     * stays valid for stop. */
    char *argv[] = { "env", "LC_ALL=C", "pactl", "subscribe", NULL };
    posix_spawn_file_actions_t actions;
    int pipefd[2];
    int rc;

    if (pipe2(pipefd, O_CLOEXEC | O_NONBLOCK) != 0) return -1;

    /* The child's end must block, so clear O_NONBLOCK on the write side. */
    if (fcntl(pipefd[1], F_SETFL, 0) != 0 ||
        posix_spawn_file_actions_init(&actions) != 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
    posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null",
                                     O_WRONLY, 0);
    rc = posix_spawnp(pid, "env", &actions, NULL, argv, environ);
    posix_spawn_file_actions_destroy(&actions);
    close(pipefd[1]);
    if (rc != 0) {
        close(pipefd[0]);
        return -1;
    }
    return pipefd[0];
}

void
oomaya_audio_watch_stop(int fd, pid_t pid)
{
    if (fd >= 0) close(fd);
    if (pid > 0) {
        kill(pid, SIGKILL);
        while (waitpid(pid, NULL, 0) < 0 && errno == EINTR) { }
    }
}

int
oomaya_audio_watch_read(int fd)
{
    char    buf[1024];
    int     changed = 0;
    ssize_t n;

    for (;;) {
        n = read(fd, buf, sizeof(buf) - 1);
        if (n == 0) return -1;
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return changed;
            return -1;
        }
        buf[n] = '\0';
        if (strstr(buf, " on sink") != NULL ||
            strstr(buf, " on server") != NULL ||
            strstr(buf, " on card") != NULL) {
            changed = 1;
        }
    }
}
