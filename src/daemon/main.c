/* main.c — dwm-oomayad daemon entry point.
 *
 * Phase 3. C99 + POSIX.1-2008.
 *
 * Lifecycle:
 *   1. Resolve socket path (XDG_RUNTIME_DIR → UID fallback).
 *   2. mkdir -p the socket directory (no hardcoded paths).
 *   3. Create + bind AF_UNIX SOCK_STREAM listener (SOCK_CLOEXEC).
 *   4. Init state cache, worker pool, reactor.
 *   5. Install signal handlers (SIGTERM/SIGINT → graceful stop; SIGPIPE → SIG_IGN).
 *   6. Block in reactor_run().
 *   7. Drain workers, destroy reactor, unlink socket, exit 0.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "../../include/oomaya_ipc.h"
#include "../../include/oomaya_state.h"
#include "../../include/oomaya_worker.h"
#include "../../include/oomaya_reactor_impl.h"
#include "../../include/oomaya_reactor.h"
#include "../../include/oomaya_x11_bridge.h"
#include "../../include/oomaya_dispatch.h"

/* ── Signal handling ─────────────────────────────────────────────────────── */

static volatile sig_atomic_t g_stop = 0;
static struct oomaya_reactor *g_reactor_ptr = NULL;

static void
sig_stop(int signo)
{
    (void)signo;
    g_stop = 1;
    if (g_reactor_ptr)
        oomaya_reactor_stop(g_reactor_ptr);
}

/* ── Socket path resolution ─────────────────────────────────────────────── */

/*
 * resolve_socket_path() — Write the canonical socket path into buf.
 *
 * Primary:  $XDG_RUNTIME_DIR/dwm-oomaya/ipc.sock
 * Fallback: /tmp/dwm-oomaya-{UID}/ipc.sock
 *
 * Returns 0 on success, -1 if buf is too small.
 * NEVER hardcodes /home or any username.
 */
static int
resolve_socket_path(char *buf, size_t buflen)
{
    const char *xdg;
    int         n;

    xdg = getenv("XDG_RUNTIME_DIR");
    if (xdg != NULL && xdg[0] != '\0') {
        n = snprintf(buf, buflen, "%s/dwm-oomaya/ipc.sock", xdg);
    } else {
        n = snprintf(buf, buflen, "/tmp/dwm-oomaya-%u/ipc.sock",
                     (unsigned)getuid());
    }

    return (n > 0 && (size_t)n < buflen) ? 0 : -1;
}

/*
 * mkdir_p() — Create a directory and any missing parents.
 * Accepts a full file path; creates up to the last '/' component.
 */
static int
mkdir_p_for_path(const char *filepath)
{
    char  dir[256];
    char *p;
    int   n;

    n = snprintf(dir, sizeof(dir), "%s", filepath);
    if (n <= 0 || (size_t)n >= sizeof(dir)) return -1;

    /* Truncate to directory component */
    p = strrchr(dir, '/');
    if (p == NULL) return 0;
    *p = '\0';

    /* Walk and create each component */
    for (p = dir + 1; *p != '\0'; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(dir, 0700) < 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    return (mkdir(dir, 0700) < 0 && errno != EEXIST) ? -1 : 0;
}

/* ── Listener socket creation ────────────────────────────────────────────── */

static int
create_listener(const char *path)
{
    int               fd;
    struct sockaddr_un addr;
    socklen_t         addrlen;

    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) return -1;

    /* Remove stale socket file if present */
    unlink(path);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path)
            >= (int)sizeof(addr.sun_path)) {
        close(fd); return -1;
    }
    addrlen = (socklen_t)(offsetof(struct sockaddr_un, sun_path) +
                          strlen(addr.sun_path) + 1);

    if (bind(fd, (struct sockaddr *)&addr, addrlen) < 0) {
        close(fd); return -1;
    }
    if (listen(fd, 16) < 0) {
        close(fd); return -1;
    }

    return fd;
}

/* ── Worker dispatch ───────────────────────────────────────────────────────
 * Offload-path opcodes (power, audio volume, monitor cycling) are handled
 * by oomaya_default_dispatch() in worker_dispatch.c.
 */

/* ── Entry point ─────────────────────────────────────────────────────────── */

int
main(void)
{
    char                  sock_path[256];
    int                   listen_fd;
    long                  ncpus;
    uint32_t              nworkers;
    oomaya_wm_state_t     state;
    oomaya_worker_pool_t  workers;
    struct oomaya_reactor  reactor;
    struct sigaction       sa;

    /* --- Signal handlers --- */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_stop;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);

    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);

    /* --- Socket path --- */
    if (resolve_socket_path(sock_path, sizeof(sock_path)) < 0) {
        fprintf(stderr, "dwm-oomayad: socket path too long\n");
        return EXIT_FAILURE;
    }
    if (mkdir_p_for_path(sock_path) < 0) {
        fprintf(stderr, "dwm-oomayad: cannot create socket directory: %s\n",
                sock_path);
        return EXIT_FAILURE;
    }

    /* --- Listener --- */
    listen_fd = create_listener(sock_path);
    if (listen_fd < 0) {
        fprintf(stderr, "dwm-oomayad: cannot bind %s: %s\n",
                sock_path, strerror(errno));
        return EXIT_FAILURE;
    }

    fprintf(stdout, "dwm-oomayad: listening on %s\n", sock_path);

    /* --- State cache --- */
    oomaya_state_init(&state);

    /* --- Worker pool (2–4 threads based on nproc, capped at 4) --- */
    ncpus    = sysconf(_SC_NPROCESSORS_ONLN);
    nworkers = (ncpus >= 4) ? 4u : (ncpus >= 2 ? 2u : 1u);
    if (oomaya_worker_pool_init(&workers, nworkers, &state, oomaya_default_dispatch) < 0) {
        fprintf(stderr, "dwm-oomayad: worker pool init failed\n");
        close(listen_fd); unlink(sock_path);
        return EXIT_FAILURE;
    }
    fprintf(stdout, "dwm-oomayad: worker pool started (%u threads)\n", nworkers);

    /* --- Reactor --- */
    if (oomaya_reactor_init(&reactor, listen_fd, &workers, &state) < 0) {
        fprintf(stderr, "dwm-oomayad: reactor init failed\n");
        oomaya_worker_pool_shutdown(&workers);
        close(listen_fd); unlink(sock_path);
        return EXIT_FAILURE;
    }

    /* --- X11 Bridge (The Shield) --- */
    int x11_fd = -1;
    if (oomaya_x11_bridge_init(&state, &x11_fd) == 0 && x11_fd >= 0) {
        oomaya_reactor_add_x11(&reactor, x11_fd);
        fprintf(stdout, "dwm-oomayad: X11 bridge active (fd %d)\n", x11_fd);
    }

    /* --- Main event loop --- */
    g_reactor_ptr = &reactor;
    while (!g_stop)
        oomaya_reactor_run(&reactor);
    g_reactor_ptr = NULL;

    /* --- Graceful shutdown --- */
    fprintf(stdout, "dwm-oomayad: shutting down\n");
    oomaya_reactor_stop(&reactor);
    oomaya_reactor_destroy(&reactor);
    oomaya_x11_bridge_destroy();
    oomaya_worker_pool_shutdown(&workers);
    close(listen_fd);
    unlink(sock_path);

    return EXIT_SUCCESS;
}
