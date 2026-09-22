/* oomaya_reactor.h — epoll reactor public API. */
#ifndef OOMAYA_REACTOR_H
#define OOMAYA_REACTOR_H

#include "oomaya_worker.h"
#include "oomaya_state.h"

/* Opaque reactor context — defined in reactor.c */
struct oomaya_reactor;

int  oomaya_reactor_init(struct oomaya_reactor *r,
                          int                   listen_fd,
                          oomaya_worker_pool_t *workers,
                          oomaya_wm_state_t    *state);
int  oomaya_reactor_add_x11(struct oomaya_reactor *r, int x11_fd);
void oomaya_reactor_run(struct oomaya_reactor *r);
void oomaya_reactor_stop(struct oomaya_reactor *r);
void oomaya_reactor_destroy(struct oomaya_reactor *r);

#endif /* OOMAYA_REACTOR_H */
