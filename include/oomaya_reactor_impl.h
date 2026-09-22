/* oomaya_reactor_impl.h — Full reactor struct layout for stack allocation.
 * Include ONLY from main.c and test files that need to stack-allocate the reactor.
 * All other code uses the opaque pointer via oomaya_reactor.h. */
#ifndef OOMAYA_REACTOR_IMPL_H
#define OOMAYA_REACTOR_IMPL_H

#include <stdint.h>
#include "oomaya_ipc.h"
#include "oomaya_rate.h"
#include "oomaya_worker.h"
#include "oomaya_state.h"

typedef enum { CS_WANT_HDR = 0, CS_WANT_BODY, CS_READY } client_state_t;

typedef struct {
    int               fd;
    int               rate_slot;
    client_state_t    cstate;
    oomaya_ipc_header_t hdr;
    uint8_t           body[OOMAYA_FRAME_BODY_MAX];
    uint32_t          body_read;
} reactor_client_t;

struct oomaya_reactor {
    int                  epfd;
    int                  listen_fd;
    int                  x11_fd;
    oomaya_rate_pool_t   rate;
    reactor_client_t     clients[OOMAYA_RATE_MAX_CLIENTS];
    oomaya_worker_pool_t *workers;
    oomaya_wm_state_t    *state;
    int                   running;
};

#endif /* OOMAYA_REACTOR_IMPL_H */
