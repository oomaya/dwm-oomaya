/* reactor.c — epoll-based non-blocking I/O event reactor.
 *
 * Phase 3 Core. C99 + Linux epoll + POSIX UDS.
 *
 * Event loop:
 *   [UDS listener fd]  → accept4() new client, add to epoll, rate_client_add()
 *   [client fd]        → recv header+body, validate, rate_check, classify:
 *                         fast-path: dispatch inline, writev() response
 *                         offload:   worker_pool_submit(), async completion
 *   [eventfd]          → worker completed: drains resp_ring, writev() to client
 *   [x11 fd] (opt.)    → XPending() flush, PropertyNotify → emit signals
 *
 * Zero malloc. All client state in a fixed client array.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/eventfd.h>

#include "../../include/oomaya_ipc.h"
#include "../../include/oomaya_rate.h"
#include "../../include/oomaya_state.h"
#include "../../include/oomaya_worker.h"
#include "../../include/oomaya_reactor.h"
#include "../../include/oomaya_reactor_impl.h"
#include "../../include/oomaya_x11_bridge.h"

/* ── Constants ──────────────────────────────────────────────────────────── */
#define EPOLL_MAX_EVENTS  64
#define REACTOR_BACKLOG   16
#define BODY_BUF_MAX      OOMAYA_FRAME_BODY_MAX

/* ── Fast-path response body ─────────────────────────────────────────────── */
typedef struct __attribute__((packed)) {
    int32_t  status;
    uint32_t tag_mask;
    uint32_t layout_idx;
    uint32_t focused_win;
    uint32_t monitor_count;
} oomaya_state_resp_t;

/* ── Helpers ─────────────────────────────────────────────────────────────── */

static reactor_client_t *
client_slot_alloc(struct oomaya_reactor *r, int fd, int rate_slot)
{
    int i;
    for (i = 0; i < OOMAYA_RATE_MAX_CLIENTS; ++i) {
        if (r->clients[i].fd == -1) {
            /* A new generation retires completions owed to the previous client. */
            uint32_t generation = r->clients[i].generation + 1u;
            memset(&r->clients[i], 0, sizeof(r->clients[i]));
            r->clients[i].fd         = fd;
            r->clients[i].rate_slot  = rate_slot;
            r->clients[i].cstate     = CS_WANT_HDR;
            r->clients[i].generation = generation;
            return &r->clients[i];
        }
    }
    return NULL;
}

static void
client_slot_free(struct oomaya_reactor *r, reactor_client_t *c)
{
    if (c == NULL || c->fd < 0) return;
    if (c->subscribed) {
        c->subscribed = 0;
        --r->subscribers;
    }
    epoll_ctl(r->epfd, EPOLL_CTL_DEL, c->fd, NULL);
    oomaya_rate_client_remove(&r->rate, c->rate_slot);
    close(c->fd);
    c->fd = -1;
}

/* Send one whole frame, or report failure. A client socket never blocks the
 * reactor, and a short write would leave half a frame in the stream, so the
 * only safe answer to either is to drop that client. */
static int
send_frame(int fd, const oomaya_ipc_header_t *hdr,
           const void *body, uint32_t body_len)
{
    struct iovec  iov[2];
    struct msghdr msg;
    size_t        total = sizeof(*hdr) + (size_t)body_len;
    ssize_t       n;

    iov[0].iov_base = (void *)hdr;
    iov[0].iov_len  = sizeof(*hdr);
    iov[1].iov_base = (void *)body;
    iov[1].iov_len  = (size_t)body_len;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov    = iov;
    msg.msg_iovlen = (body_len > 0) ? 2 : 1;

    do {
        n = sendmsg(fd, &msg, MSG_DONTWAIT | MSG_NOSIGNAL);
    } while (n < 0 && errno == EINTR);

    return (n == (ssize_t)total) ? 0 : -1;
}

static void
send_response(struct oomaya_reactor *r, reactor_client_t *c,
              const oomaya_ipc_header_t *hdr,
              const void *body, uint32_t body_len)
{
    if (c->fd < 0) return;
    if (send_frame(c->fd, hdr, body, body_len) != 0)
        client_slot_free(r, c);
}

/* ── Signals ─────────────────────────────────────────────────────────────── */

static void
send_signal(struct oomaya_reactor *r, reactor_client_t *c, uint16_t sig,
            const int32_t *values, uint32_t nvalues)
{
    uint8_t             body[2 + 2 * sizeof(int32_t)];
    uint32_t            len = 2 + nvalues * (uint32_t)sizeof(int32_t);
    oomaya_ipc_header_t hdr;

    memcpy(body, &sig, sizeof(sig));
    memcpy(body + 2, values, nvalues * sizeof(int32_t));

    memset(&hdr, 0, sizeof(hdr));
    hdr.magic       = OOMAYA_IPC_MAGIC;
    hdr.client_ver  = OOMAYA_IPC_CURRENT_VER;
    hdr.target_ver  = OOMAYA_IPC_CURRENT_VER;
    hdr.msg_type    = (uint16_t)OOMAYA_MSG_SIGNAL;
    hdr.payload_len = len;
    send_response(r, c, &hdr, body, len);
}

/* A subscriber that cannot keep up is dropped by send_response, so one stalled
 * reader never delays the others. */
static void
broadcast(struct oomaya_reactor *r, uint16_t sig,
          const int32_t *values, uint32_t nvalues)
{
    int i;

    for (i = 0; i < OOMAYA_RATE_MAX_CLIENTS; ++i) {
        reactor_client_t *c = &r->clients[i];
        if (c->fd >= 0 && c->subscribed)
            send_signal(r, c, sig, values, nvalues);
    }
}

static void
broadcast_state_changes(struct oomaya_reactor *r)
{
    uint32_t tag, layout, focused;
    int32_t  value;

    if (r->subscribers == 0) return;

    tag     = oomaya_state_get_tag_mask(r->state);
    layout  = oomaya_state_get_layout(r->state);
    focused = oomaya_state_get_focused(r->state);

    if (tag != r->last_tag_mask) {
        r->last_tag_mask = tag;
        value = (int32_t)tag;
        broadcast(r, IPC_SIG_WM_TAG_CHANGED, &value, 1);
    }
    if (layout != r->last_layout) {
        r->last_layout = layout;
        value = (int32_t)layout;
        broadcast(r, IPC_SIG_WM_LAYOUT_CHANGED, &value, 1);
    }
    if (focused != r->last_focused) {
        r->last_focused = focused;
        value = (int32_t)focused;
        broadcast(r, IPC_SIG_WM_FOCUS_CHANGED, &value, 1);
    }
}

/* ── Fast-path handlers ──────────────────────────────────────────────────── */

static void
handle_subscribe(struct oomaya_reactor *r, reactor_client_t *c)
{
    oomaya_ipc_header_t resp;
    int32_t             st = 0;

    oomaya_frame_init_response(&resp, &c->hdr, sizeof(st));
    send_response(r, c, &resp, &st, sizeof(st));
    if (c->fd < 0 || c->subscribed) return;

    c->subscribed = 1;
    if (r->subscribers++ == 0) {
        r->last_tag_mask = oomaya_state_get_tag_mask(r->state);
        r->last_layout   = oomaya_state_get_layout(r->state);
        r->last_focused  = oomaya_state_get_focused(r->state);
    }
}

static void
handle_wm_get_state(struct oomaya_reactor *r, reactor_client_t *c)
{
    const oomaya_ipc_header_t *req = &c->hdr;
    oomaya_wm_state_t *state = r->state;
    oomaya_ipc_header_t resp;
    oomaya_state_resp_t body;

    body.status        = 0;
    body.tag_mask      = oomaya_state_get_tag_mask(state);
    body.layout_idx    = oomaya_state_get_layout(state);
    body.focused_win   = oomaya_state_get_focused(state);
    body.monitor_count = oomaya_state_get_monitor_count(state);

    oomaya_frame_init_response(&resp, req, (uint32_t)sizeof(body));
    send_response(r, c, &resp, &body, (uint32_t)sizeof(body));
}

static void
handle_wm_view_tag(struct oomaya_reactor *r, reactor_client_t *c)
{
    const oomaya_ipc_header_t *req = &c->hdr;
    const uint8_t *body_bytes = c->body;
    oomaya_wm_state_t *state = r->state;
    uint32_t            new_mask;
    oomaya_ipc_header_t resp;
    int32_t             status = 0;

    /* body: [opcode:2][new_tag_mask:4] */
    if (req->payload_len >= 6) {
        memcpy(&new_mask, body_bytes + 2, sizeof(new_mask));
        oomaya_state_set_tag_mask(state, new_mask);
        oomaya_x11_view_tag(new_mask);
        oomaya_x11_sync_state();
    } else {
        status = -1;
    }

    oomaya_frame_init_response(&resp, req, sizeof(status));
    send_response(r, c, &resp, &status, sizeof(status));
}

static void
dispatch_fast_path(struct oomaya_reactor *r,
                   reactor_client_t *c,
                   uint16_t opcode)
{
    switch (opcode) {
    case IPC_CMD_WM_GET_STATE:
        handle_wm_get_state(r, c);
        break;
    case IPC_CMD_WM_VIEW_TAG:
        handle_wm_view_tag(r, c);
        break;
    case IPC_CMD_SUBSCRIBE:
        handle_subscribe(r, c);
        break;
    case IPC_CMD_WM_FOCUS_CLIENT: {
        uint32_t xid = 0;
        if (c->hdr.payload_len >= 6) memcpy(&xid, c->body + 2, 4);
        oomaya_state_set_focused(r->state, xid);
        oomaya_x11_focus_window(xid);
        oomaya_ipc_header_t resp;
        int32_t st = 0;
        oomaya_frame_init_response(&resp, &c->hdr, sizeof(st));
        send_response(r, c, &resp, &st, sizeof(st));
        break;
    }
    case IPC_CMD_WM_KILL_CLIENT: {
        uint32_t focused = oomaya_state_get_focused(r->state);
        if (focused != 0) oomaya_x11_kill_window(focused);
        oomaya_ipc_header_t resp;
        int32_t st = 0;
        oomaya_frame_init_response(&resp, &c->hdr, sizeof(st));
        send_response(r, c, &resp, &st, sizeof(st));
        break;
    }
    /* IPC_CMD_WM_TOGGLE_TAG and IPC_CMD_WM_SET_LAYOUT land here on purpose. dwm
     * has no message for either yet: EWMH carries one desktop index, and layout
     * has no inbound path. Refusing lets callers use the key binding. */
    default: {
        oomaya_ipc_header_t err;
        oomaya_frame_init_error(&err, &c->hdr, OOMAYA_IPC_ERR_OPCODE);
        send_response(r, c, &err, NULL, 0);
        break;
    }
    }
}

/* ── Frame dispatch ──────────────────────────────────────────────────────── */

static void
dispatch_frame(struct oomaya_reactor *r, reactor_client_t *c)
{
    uint16_t      opcode;
    oomaya_task_t task;

    /* Validate the frame */
    if (oomaya_frame_validate(&c->hdr) != OOMAYA_IPC_OK) {
        oomaya_ipc_header_t err;
        oomaya_frame_init_error(&err, &c->hdr, OOMAYA_IPC_ERR_MALFORMED);
        send_response(r, c, &err, NULL, 0);
        return;
    }

    /* Rate check — fail-closed */
    if (!oomaya_rate_check(&r->rate, c->rate_slot)) {
        oomaya_ipc_header_t err;
        oomaya_frame_init_error(&err, &c->hdr, OOMAYA_IPC_ERR_MALFORMED);
        send_response(r, c, &err, NULL, 0);
        return;
    }

    /* Extract opcode from first 2 bytes of body */
    if (c->hdr.payload_len >= 2) {
        memcpy(&opcode, c->body, sizeof(opcode));
    } else {
        opcode = 0xFFFFu;
    }

    /* Classify fast path vs worker pool */
    oomaya_ipc_header_t query_hdr = c->hdr;
    query_hdr.msg_id = opcode;
    int cls = oomaya_neg_classify(&query_hdr);

    if (cls == 0 && oomaya_neg_opcode_is_fast_path(opcode)) {
        dispatch_fast_path(r, c, opcode);
    } else if (cls == 1) {
        /* Offload path: submit task to worker pool */
        memset(&task, 0, sizeof(task));
        task.client_fd   = (uint32_t)c->fd;
        task.client_slot = (uint32_t)(c - r->clients);
        task.client_gen  = c->generation;
        task.msg_id      = c->hdr.msg_id;
        task.target_ver  = c->hdr.target_ver;
        task.opcode      = opcode;
        task.payload_len = c->hdr.payload_len;
        if (task.payload_len > sizeof(task.payload))
            task.payload_len = sizeof(task.payload);
        memcpy(task.payload, c->body, task.payload_len);

        if (oomaya_worker_pool_submit(r->workers, &task) != OOMAYA_RING_OK) {
            oomaya_ipc_header_t err;
            oomaya_frame_init_error(&err, &c->hdr, OOMAYA_IPC_ERR_INTERNAL);
            send_response(r, c, &err, NULL, 0);
        }
        /* Response will arrive asynchronously via eventfd wakeup */
    } else {
        oomaya_ipc_header_t err;
        oomaya_frame_init_error(&err, &c->hdr, OOMAYA_IPC_ERR_OPCODE);
        send_response(r, c, &err, NULL, 0);
    }
}

/* ── Client read ─────────────────────────────────────────────────────────── */

static void
handle_client_readable(struct oomaya_reactor *r, reactor_client_t *c)
{
    ssize_t n;

    if (c->cstate == CS_WANT_HDR) {
        /* Read up to 16 bytes of header */
        uint8_t *dst = (uint8_t *)&c->hdr;
        n = recv(c->fd, dst, sizeof(c->hdr), MSG_DONTWAIT);
        if (n <= 0) {
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK ||
                          errno == EINTR)) return;
            client_slot_free(r, c);
            return;
        }
        if ((size_t)n < sizeof(c->hdr)) {
            /* Partial header — close; reactor doesn't buffer partial headers */
            client_slot_free(r, c);
            return;
        }

        /*
         * v1 disconnects clients whose header violates the frame contract;
         * rejected-frame body bytes may still be in flight and would desync
         * the stream. validate() exempts ERROR frames from the payload cap
         * for outbound err-in-payload_len semantics, so inbound enforces it
         * here: a probe's 600 B ERROR frame left 88 residual bytes as a header.
         */
        int vr = oomaya_frame_validate(&c->hdr);
        if (vr == OOMAYA_IPC_OK && c->hdr.payload_len > OOMAYA_MAX_PAYLOAD_LEN)
            vr = OOMAYA_IPC_ERR_PAYLOAD;
        if (vr != OOMAYA_IPC_OK) {
            oomaya_ipc_header_t err;
            oomaya_frame_init_error(&err, &c->hdr, vr);
            send_response(r, c, &err, NULL, 0);
            client_slot_free(r, c);
            return;
        }

        c->body_read = 0;
        if (c->hdr.payload_len == 0) {
            dispatch_frame(r, c);
            c->cstate = CS_WANT_HDR;
        } else {
            c->cstate = CS_WANT_BODY;
        }
    }

    if (c->cstate == CS_WANT_BODY) {
        uint32_t want = c->hdr.payload_len > BODY_BUF_MAX
                        ? BODY_BUF_MAX : c->hdr.payload_len;
        n = recv(c->fd, c->body + c->body_read,
                 want - c->body_read, MSG_DONTWAIT);
        if (n <= 0) {
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK ||
                          errno == EINTR)) return;
            client_slot_free(r, c);
            return;
        }
        c->body_read += (uint32_t)n;
        if (c->body_read >= want) {
            dispatch_frame(r, c);
            c->cstate    = CS_WANT_HDR;
            c->body_read = 0;
        }
    }
}

/* ── eventfd drain (worker completions) ──────────────────────────────────── */

static void
handle_eventfd(struct oomaya_reactor *r, int evfd)
{
    uint64_t count = 0;
    ssize_t  s;
    oomaya_task_t completed;

    s = read(evfd, &count, sizeof(count)); /* drain the counter */
    (void)s;

    if (r == NULL || r->workers == NULL) return;

    /* Drain all completed responses from the completion ring */
    while (oomaya_ring_dequeue(&r->workers->resp_ring, &completed) == OOMAYA_RING_OK) {
        reactor_client_t *c;

        /* The slot and generation, not the fd number, name the client: a closed
         * client's fd can be reused by an unrelated one before this drains. */
        if (completed.client_slot >= OOMAYA_RATE_MAX_CLIENTS) continue;
        c = &r->clients[completed.client_slot];
        if (c->fd < 0 || c->generation != completed.client_gen) continue;

        oomaya_ipc_header_t resp;
        resp.magic       = OOMAYA_IPC_MAGIC;
        resp.client_ver  = OOMAYA_IPC_CURRENT_VER;
        resp.target_ver  = completed.target_ver;
        resp.msg_id      = completed.msg_id;
        if (completed.status != 0) {
            /* Wire rule: an error frame carries its oomaya_ipc_err_t in
             * payload_len and has no body. */
            resp.msg_type    = (uint16_t)OOMAYA_MSG_ERROR;
            resp.payload_len = (uint32_t)completed.status;
            send_response(r, c, &resp, NULL, 0);
            continue;
        }
        resp.msg_type    = (uint16_t)OOMAYA_MSG_RESPONSE;
        resp.payload_len = completed.resp_len;

        send_response(r, c, &resp, completed.resp_data, completed.resp_len);
    }
}

/* ── Accept new client ───────────────────────────────────────────────────── */

static void
handle_accept(struct oomaya_reactor *r)
{
    int              fd;
    int              rate_slot;
    reactor_client_t *c;
    struct epoll_event ev;

    for (;;) {
        fd = accept4(r->listen_fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            if (errno == EINTR) continue;
            break;
        }

        rate_slot = oomaya_rate_client_add(&r->rate, fd);
        if (rate_slot < 0) {
            /* Pool full — reject connection */
            close(fd);
            continue;
        }

        c = client_slot_alloc(r, fd, rate_slot);
        if (c == NULL) {
            oomaya_rate_client_remove(&r->rate, rate_slot);
            close(fd);
            continue;
        }

        /* Level-triggered client events prevent starvation on partial reads */
        memset(&ev, 0, sizeof(ev));
        ev.events   = EPOLLIN | EPOLLHUP | EPOLLERR;
        ev.data.ptr = c;
        if (epoll_ctl(r->epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            client_slot_free(r, c);
        }
    }
}

/* ── Public API ───────────────────────────────────────────────────────────── */

int
oomaya_reactor_init(struct oomaya_reactor   *r,
                    int                      listen_fd,
                    oomaya_worker_pool_t    *workers,
                    oomaya_wm_state_t       *state)
{
    int i;
    struct epoll_event ev;

    if (r == NULL || listen_fd < 0 || workers == NULL || state == NULL)
        return -1;

    memset(r, 0, sizeof(*r));
    r->listen_fd = listen_fd;
    r->x11_fd    = -1;
    r->workers   = workers;
    r->state     = state;
    r->running   = 1;
    r->stop_fd   = -1;

    for (i = 0; i < OOMAYA_RATE_MAX_CLIENTS; ++i)
        r->clients[i].fd = -1;

    oomaya_rate_pool_init(&r->rate);

    r->epfd = epoll_create1(EPOLL_CLOEXEC);
    if (r->epfd < 0) return -1;

    /* Add listener fd (uses address of listen_fd as unambiguous pointer) */
    memset(&ev, 0, sizeof(ev));
    ev.events   = EPOLLIN | EPOLLET;
    ev.data.ptr = &r->listen_fd;
    if (epoll_ctl(r->epfd, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
        close(r->epfd); return -1;
    }

    /* Add worker eventfd (uses address of eventfd as unambiguous pointer) */
    memset(&ev, 0, sizeof(ev));
    ev.events   = EPOLLIN | EPOLLET;
    ev.data.ptr = &workers->eventfd;
    if (epoll_ctl(r->epfd, EPOLL_CTL_ADD, workers->eventfd, &ev) < 0) {
        close(r->epfd); return -1;
    }

    /* The loop sleeps until something happens, so stop needs its own wakeup. */
    r->stop_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (r->stop_fd < 0) {
        close(r->epfd); return -1;
    }
    memset(&ev, 0, sizeof(ev));
    ev.events   = EPOLLIN;
    ev.data.ptr = &r->stop_fd;
    if (epoll_ctl(r->epfd, EPOLL_CTL_ADD, r->stop_fd, &ev) < 0) {
        close(r->stop_fd); close(r->epfd); return -1;
    }

    return 0;
}

int
oomaya_reactor_add_x11(struct oomaya_reactor *r, int x11_fd)
{
    struct epoll_event ev;
    if (r == NULL || x11_fd < 0) return -1;
    r->x11_fd = x11_fd;
    memset(&ev, 0, sizeof(ev));
    ev.events   = EPOLLIN;
    ev.data.ptr = &r->x11_fd;
    return epoll_ctl(r->epfd, EPOLL_CTL_ADD, x11_fd, &ev);
}

void
oomaya_reactor_run(struct oomaya_reactor *r)
{
    struct epoll_event events[EPOLL_MAX_EVENTS];
    int                nfds, i;

    while (r->running) {
        nfds = epoll_wait(r->epfd, events, EPOLL_MAX_EVENTS, -1);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            break;
        }

        for (i = 0; i < nfds; ++i) {
            struct epoll_event *ev = &events[i];

            if (ev->data.ptr == &r->listen_fd) {
                handle_accept(r);
            } else if (ev->data.ptr == &r->workers->eventfd) {
                handle_eventfd(r, r->workers->eventfd);
            } else if (r->x11_fd >= 0 && ev->data.ptr == &r->x11_fd) {
                oomaya_x11_process_events();
            } else if (ev->data.ptr == &r->stop_fd) {
                continue; /* running was cleared before the wakeup */
            } else {
                reactor_client_t *c = (reactor_client_t *)ev->data.ptr;
                /* Process readable data first even if peer signalled disconnect */
                if (ev->events & EPOLLIN) {
                    handle_client_readable(r, c);
                }
                if (ev->events & (EPOLLHUP | EPOLLERR)) {
                    if (c->fd >= 0) {
                        client_slot_free(r, c);
                    }
                }
            }
        }
        broadcast_state_changes(r);
    }
}

/* Called from signal handlers: only async-signal-safe work here. */
void
oomaya_reactor_stop(struct oomaya_reactor *r)
{
    uint64_t one = 1;
    ssize_t  s;

    if (r == NULL) return;
    r->running = 0;
    if (r->stop_fd >= 0) {
        s = write(r->stop_fd, &one, sizeof(one));
        (void)s;
    }
}

void
oomaya_reactor_destroy(struct oomaya_reactor *r)
{
    int i;
    if (r == NULL) return;
    for (i = 0; i < OOMAYA_RATE_MAX_CLIENTS; ++i) {
        if (r->clients[i].fd >= 0)
            client_slot_free(r, &r->clients[i]);
    }
    if (r->stop_fd >= 0) { close(r->stop_fd); r->stop_fd = -1; }
    if (r->epfd >= 0) { close(r->epfd); r->epfd = -1; }
}
