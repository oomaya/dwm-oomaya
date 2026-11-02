/* test_integration_live.c — Full live reactor + worker pool integration test. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <errno.h>
#include <poll.h>
#include <time.h>

#include "../include/oomaya_ipc.h"
#include "../include/oomaya_state.h"
#include "../include/oomaya_worker.h"
#include "../include/oomaya_reactor.h"
#include "../include/oomaya_reactor_impl.h"

static int g_pass = 0, g_fail = 0;
#define ASSERT_EQ(lbl, got, exp) \
    do { \
        long long _g=(long long)(got), _e=(long long)(exp); \
        if (_g==_e){printf("  [PASS] %s\n",(lbl));++g_pass;} \
        else{printf("  [FAIL] %s got=%lld exp=%lld\n",(lbl),_g,_e);++g_fail;} \
    } while(0)

static void
test_dispatch(oomaya_task_t *task, oomaya_wm_state_t *state)
{
    (void)state;
    /* A slow offload keeps a completion pending across a client's exit. */
    if (task->opcode == IPC_CMD_SYS_SET_POWER) {
        struct timespec ts = { .tv_sec = 0, .tv_nsec = 3000000 };
        nanosleep(&ts, NULL);
    }
    /* Simulate worker processing SYS_GET_POWER (battery % = 95) */
    int32_t val = 95;
    task->status = 0;
    task->resp_len = sizeof(val);
    memcpy(task->resp_data, &val, sizeof(val));
}

static void *
reactor_thread(void *arg)
{
    struct oomaya_reactor *r = (struct oomaya_reactor *)arg;
    oomaya_reactor_run(r);
    return NULL;
}

/* Frame-contract test helpers */

#define TEST_BODY_MAX 1024u
/* Wire layout: status, tag_mask, layout_idx, focused_win, monitor_count. */
#define TEST_STATE_RESP_LEN 20u

/* Connect with a receive timeout so a missing reply fails instead of hanging. */
static int
client_connect(const struct sockaddr_un *addr)
{
    struct timeval tv = { .tv_sec = 2, .tv_usec = 0 };
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd < 0) return -1;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0 ||
        connect(fd, (const struct sockaddr *)addr, sizeof(*addr)) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

/* Header and body go out in one call so the reactor sees them queued together. */
static int
send_frame(int fd, const oomaya_ipc_header_t *hdr,
           const void *body, size_t body_len)
{
    struct iovec iov[2];
    size_t total = sizeof(*hdr) + body_len;

    iov[0].iov_base = (void *)hdr;
    iov[0].iov_len  = sizeof(*hdr);
    iov[1].iov_base = (void *)body;
    iov[1].iov_len  = body_len;
    return writev(fd, iov, body_len > 0 ? 2 : 1) == (ssize_t)total ? 0 : -1;
}

static int
recv_exact(int fd, void *buf, size_t len)
{
    uint8_t *p = buf;
    size_t got = 0;

    while (got < len) {
        ssize_t n = recv(fd, p + got, len - got, 0);
        if (n <= 0) return -1;
        got += (size_t)n;
    }
    return 0;
}

/*
 * A server close reads as EOF, or as ECONNRESET when the daemon closed with
 * body bytes unread. Timeouts and further frames are not a close.
 */
static int
peer_closed(int fd)
{
    uint8_t b;
    ssize_t n = recv(fd, &b, sizeof(b), 0);

    return n == 0 || (n < 0 && errno == ECONNRESET);
}

/* Read a WM_GET_STATE response; returns tag_mask, or -1 on any mismatch. */
static long long
recv_state_mask(int fd, uint16_t msg_id)
{
    oomaya_ipc_header_t resp;
    uint8_t body[TEST_STATE_RESP_LEN];
    uint32_t mask;

    if (recv_exact(fd, &resp, sizeof(resp)) != 0) return -1;
    if (resp.magic != OOMAYA_IPC_MAGIC ||
        resp.msg_type != (uint16_t)OOMAYA_MSG_RESPONSE ||
        resp.msg_id != msg_id ||
        resp.payload_len != sizeof(body)) return -1;
    if (recv_exact(fd, body, sizeof(body)) != 0) return -1;
    memcpy(&mask, body + 4, sizeof(mask));
    return (long long)mask;
}

/* Send a minimal WM_GET_STATE; returns tag_mask, or -1 on any mismatch. */
static long long
get_state_mask(int fd, uint16_t msg_id)
{
    oomaya_ipc_header_t req;
    uint16_t op = IPC_CMD_WM_GET_STATE;

    oomaya_frame_init_request(&req, op, msg_id, sizeof(op));
    if (send_frame(fd, &req, &op, sizeof(op)) != 0) return -1;
    return recv_state_mask(fd, msg_id);
}

/* Build a request header that a test then corrupts as needed. */
static oomaya_ipc_header_t
violation_req(uint16_t msg_type, uint32_t payload_len, uint16_t msg_id)
{
    oomaya_ipc_header_t req;

    oomaya_frame_init_request(&req, 0, msg_id, payload_len);
    req.msg_type = msg_type;
    return req;
}

/* A frame-contract violation gets one error frame, then a closed connection. */
static void
check_violation(const char *name, const struct sockaddr_un *addr,
                const oomaya_ipc_header_t *req, size_t body_len,
                int32_t exp_code)
{
    static uint8_t body[TEST_BODY_MAX];
    oomaya_ipc_header_t err;
    char lbl[96];
    int fd = client_connect(addr);

    snprintf(lbl, sizeof(lbl), "%s: connect", name);
    ASSERT_EQ(lbl, fd >= 0, 1);
    if (fd < 0) return;

    memset(body, 0xAB, sizeof(body));
    memset(&err, 0, sizeof(err));

    snprintf(lbl, sizeof(lbl), "%s: send", name);
    ASSERT_EQ(lbl, send_frame(fd, req, body, body_len), 0);
    snprintf(lbl, sizeof(lbl), "%s: error frame received", name);
    ASSERT_EQ(lbl, recv_exact(fd, &err, sizeof(err)), 0);
    snprintf(lbl, sizeof(lbl), "%s: msg_type", name);
    ASSERT_EQ(lbl, err.msg_type, (uint16_t)OOMAYA_MSG_ERROR);
    snprintf(lbl, sizeof(lbl), "%s: msg_id echoed", name);
    ASSERT_EQ(lbl, err.msg_id, req->msg_id);
    snprintf(lbl, sizeof(lbl), "%s: error code", name);
    ASSERT_EQ(lbl, (int32_t)err.payload_len, exp_code);
    snprintf(lbl, sizeof(lbl), "%s: connection closed", name);
    ASSERT_EQ(lbl, peer_closed(fd), 1);
    close(fd);
}

/* T01: exactly OOMAYA_FRAME_BODY_MAX bytes is accepted and stays in sync. */
static void
test_getstate_512_boundary_ok(const struct sockaddr_un *addr)
{
    static uint8_t body[OOMAYA_FRAME_BODY_MAX];
    oomaya_ipc_header_t req;
    uint16_t op = IPC_CMD_WM_GET_STATE;
    int fd = client_connect(addr);

    ASSERT_EQ("T01 connect", fd >= 0, 1);
    if (fd < 0) return;

    memset(body, 0xAB, sizeof(body));
    memcpy(body, &op, sizeof(op));
    oomaya_frame_init_request(&req, op, 0x3001, sizeof(body));

    ASSERT_EQ("T01 send", send_frame(fd, &req, body, sizeof(body)), 0);
    ASSERT_EQ("T01 512-byte frame answered", recv_state_mask(fd, 0x3001), 0x1234);
    ASSERT_EQ("T01 next frame stays aligned", get_state_mask(fd, 0x3002), 0x1234);
    close(fd);
}

/* T03: a fresh connection after a rejected frame inherits no desync. */
static void
test_reconnect_after_oversize_is_healthy(const struct sockaddr_un *addr)
{
    int fd = client_connect(addr);

    ASSERT_EQ("T03 reconnect", fd >= 0, 1);
    if (fd < 0) return;
    ASSERT_EQ("T03 WM_GET_STATE tag_mask", get_state_mask(fd, 0x3003), 0x1234);
    close(fd);
}

/* T04: an unknown opcode is not a frame-contract violation. */
static void
test_unknown_opcode_stays_connected(const struct sockaddr_un *addr)
{
    oomaya_ipc_header_t req, err;
    uint16_t op = 0x01FF; /* WM namespace, no such command */
    int fd = client_connect(addr);

    ASSERT_EQ("T04 connect", fd >= 0, 1);
    if (fd < 0) return;

    memset(&err, 0, sizeof(err));
    oomaya_frame_init_request(&req, op, 0x3004, sizeof(op));
    ASSERT_EQ("T04 send", send_frame(fd, &req, &op, sizeof(op)), 0);
    ASSERT_EQ("T04 error frame received", recv_exact(fd, &err, sizeof(err)), 0);
    ASSERT_EQ("T04 msg_type", err.msg_type, (uint16_t)OOMAYA_MSG_ERROR);
    ASSERT_EQ("T04 msg_id echoed", err.msg_id, 0x3004);
    ASSERT_EQ("T04 ERR_OPCODE", (int32_t)err.payload_len, OOMAYA_IPC_ERR_OPCODE);
    ASSERT_EQ("T04 connection still usable", get_state_mask(fd, 0x3005), 0x1234);
    close(fd);
}

/*
 * T08: good, oversize, and good frames queued in one write. The first is
 * answered, the second ends the connection, and the third is never parsed.
 */
static void
test_pipeline_oversize_never_desyncs(const struct sockaddr_un *addr)
{
    static uint8_t rest[TEST_BODY_MAX];
    oomaya_ipc_header_t first, bad, last, err;
    uint16_t op = IPC_CMD_WM_GET_STATE;
    uint32_t oversize = OOMAYA_FRAME_BODY_MAX + 1u;
    size_t len = 0;
    int fd = client_connect(addr);

    ASSERT_EQ("T08 connect", fd >= 0, 1);
    if (fd < 0) return;

    oomaya_frame_init_request(&first, op, 0x5001, sizeof(op));
    oomaya_frame_init_request(&last, op, 0x5003, sizeof(op));
    bad = violation_req(OOMAYA_MSG_REQUEST, oversize, 0x5002);

    memcpy(rest + len, &op, sizeof(op));
    len += sizeof(op);
    memcpy(rest + len, &bad, sizeof(bad));
    len += sizeof(bad);
    memset(rest + len, 0xAB, oversize);
    len += oversize;
    memcpy(rest + len, &last, sizeof(last));
    len += sizeof(last);
    memcpy(rest + len, &op, sizeof(op));
    len += sizeof(op);

    memset(&err, 0, sizeof(err));
    ASSERT_EQ("T08 send", send_frame(fd, &first, rest, len), 0);
    ASSERT_EQ("T08 first frame answered", recv_state_mask(fd, 0x5001), 0x1234);
    ASSERT_EQ("T08 error frame received", recv_exact(fd, &err, sizeof(err)), 0);
    ASSERT_EQ("T08 msg_type", err.msg_type, (uint16_t)OOMAYA_MSG_ERROR);
    ASSERT_EQ("T08 msg_id echoed", err.msg_id, 0x5002);
    ASSERT_EQ("T08 ERR_PAYLOAD", (int32_t)err.payload_len, OOMAYA_IPC_ERR_PAYLOAD);
    ASSERT_EQ("T08 third frame dropped, connection closed", peer_closed(fd), 1);
    close(fd);
}

/*
 * C4: a client exits with an offload pending, and the next client reuses its
 * fd number. The completion belongs to the first client and must not reach
 * the second.
 */
static void
test_completion_not_delivered_to_reused_fd(const struct sockaddr_un *addr)
{
    int misdelivered = 0, reused = 0, i;

    for (i = 0; i < 300; ++i) {
        uint16_t op = IPC_CMD_SYS_SET_POWER;
        oomaya_ipc_header_t req;
        struct pollfd pfd;
        int first = client_connect(addr), second;

        if (first < 0) break;
        oomaya_frame_init_request(&req, op, 0x5000, sizeof(op));
        if (send_frame(first, &req, &op, sizeof(op)) != 0) {
            close(first);
            break;
        }
        close(first);
        second = client_connect(addr);
        if (second < 0) break;
        ++reused;

        pfd.fd = second;
        pfd.events = POLLIN;
        pfd.revents = 0;
        if (poll(&pfd, 1, 6) > 0 && (pfd.revents & POLLIN)) ++misdelivered;
        close(second);
    }
    ASSERT_EQ("T09 ran every cycle", reused, 300);
    ASSERT_EQ("T09 no frame reached an unrelated client", misdelivered, 0);
}

/* M11: a client that stops reading is dropped; the reactor never blocks on it
 * and never leaves half a frame in its stream. */
static void
test_client_that_stops_reading_is_dropped(const struct sockaddr_un *addr)
{
    uint8_t buf[256];
    uint16_t op = IPC_CMD_WM_GET_STATE;
    oomaya_ipc_header_t req;
    int fd = client_connect(addr), i, closed = 0;
    long frames = 0;

    if (fd < 0) { ASSERT_EQ("T10 connect", fd >= 0, 1); return; }
    oomaya_frame_init_request(&req, op, 0x5100, sizeof(op));
    memcpy(buf, &req, sizeof(req));
    memcpy(buf + sizeof(req), &op, sizeof(op));
    for (i = 0; i < 4000; ++i) {
        if (send(fd, buf, sizeof(req) + sizeof(op), MSG_NOSIGNAL) < 0) break;
    }
    for (;;) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n == 0 || (n < 0 && errno == ECONNRESET)) { closed = 1; break; }
        if (n < 0) break;
        frames += n;
    }
    ASSERT_EQ("T10 stalled client disconnected", closed, 1);
    ASSERT_EQ("T10 stream ended on a frame boundary",
              frames % 16 == 0 || frames % 36 == 0 || frames % 52 == 0, 1);
    close(fd);

    fd = client_connect(addr);
    ASSERT_EQ("T10 reactor still accepts", fd >= 0, 1);
    if (fd >= 0) {
        oomaya_ipc_header_t resp;
        oomaya_frame_init_request(&req, op, 0x5101, sizeof(op));
        ASSERT_EQ("T10 send", send_frame(fd, &req, &op, sizeof(op)), 0);
        ASSERT_EQ("T10 reply", recv_exact(fd, &resp, sizeof(resp)), 0);
        close(fd);
    }
}

/* Send one fast-path request and consume its reply. */
static int
request_view(int fd, uint32_t mask, uint16_t msg_id)
{
    uint8_t body[6], reply[4];
    uint16_t op = IPC_CMD_WM_VIEW_TAG;
    oomaya_ipc_header_t req, resp;

    memcpy(body, &op, 2);
    memcpy(body + 2, &mask, 4);
    oomaya_frame_init_request(&req, op, msg_id, sizeof(body));
    if (send_frame(fd, &req, body, sizeof(body)) != 0) return -1;
    if (recv_exact(fd, &resp, sizeof(resp)) != 0) return -1;
    return recv_exact(fd, reply, sizeof(reply));
}

static int
subscribe(int fd)
{
    uint16_t op = IPC_CMD_SUBSCRIBE;
    int32_t st = -1;
    oomaya_ipc_header_t req, resp;

    oomaya_frame_init_request(&req, op, 0x5200, sizeof(op));
    if (send_frame(fd, &req, &op, sizeof(op)) != 0) return -1;
    if (recv_exact(fd, &resp, sizeof(resp)) != 0) return -1;
    if (resp.msg_type != OOMAYA_MSG_RESPONSE) return -1;
    if (recv_exact(fd, &st, sizeof(st)) != 0) return -1;
    return st;
}

static void
test_subscribers_receive_tag_signal(const struct sockaddr_un *addr)
{
    int sub = client_connect(addr), other = client_connect(addr);
    oomaya_ipc_header_t hdr;
    uint8_t body[6];
    uint16_t sig = 0;
    uint32_t mask = 0;

    ASSERT_EQ("T11 subscribe", subscribe(sub), 0);
    ASSERT_EQ("T11 view", request_view(other, 1u << 4, 0x5201), 0);
    ASSERT_EQ("T11 frame", recv_exact(sub, &hdr, sizeof(hdr)), 0);
    ASSERT_EQ("T11 type", hdr.msg_type, OOMAYA_MSG_SIGNAL);
    ASSERT_EQ("T11 length", hdr.payload_len, sizeof(body));
    ASSERT_EQ("T11 body", recv_exact(sub, body, sizeof(body)), 0);
    memcpy(&sig, body, 2);
    memcpy(&mask, body + 2, 4);
    ASSERT_EQ("T11 opcode", sig, IPC_SIG_WM_TAG_CHANGED);
    ASSERT_EQ("T11 mask", mask, 1u << 4);

    /* The requester did not subscribe, so its stream stays request/response. */
    {
        struct pollfd pfd = { .fd = other, .events = POLLIN, .revents = 0 };
        ASSERT_EQ("T11 no signal to a non-subscriber", poll(&pfd, 1, 20), 0);
    }
    close(sub);
    close(other);
}

/* M11: a stalled subscriber is dropped while a healthy one keeps receiving. */
static void
test_slow_subscriber_is_dropped(const struct sockaddr_un *addr)
{
    int stalled = client_connect(addr), healthy = client_connect(addr);
    int i, j, received = 0, closed = 0;
    uint32_t mask = 1u;
    oomaya_ipc_header_t hdr;
    uint8_t body[16];

    ASSERT_EQ("T12 subscribe stalled", subscribe(stalled), 0);
    ASSERT_EQ("T12 subscribe healthy", subscribe(healthy), 0);

    /* One connection stays inside the rate limit, so spread the load. */
    for (i = 0; i < 40 && !closed; ++i) {
        int sender = client_connect(addr);
        for (j = 0; j < 90; ++j) {
            mask = (mask == 1u) ? 2u : 1u;
            if (request_view(sender, mask, 0x5300) != 0) break;
            if (recv_exact(healthy, &hdr, sizeof(hdr)) != 0 ||
                recv_exact(healthy, body, hdr.payload_len) != 0) break;
            ++received;
        }
        close(sender);
        {
            struct pollfd pfd = { .fd = stalled, .events = 0, .revents = 0 };
            if (poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLHUP)) closed = 1;
        }
    }
    ASSERT_EQ("T12 stalled subscriber dropped", closed, 1);
    ASSERT_EQ("T12 healthy subscriber kept up", received > 100, 1);
    close(stalled);
    close(healthy);
}

int
main(void)
{
    char sock_path[100];
    int listen_fd, client_fd;
    struct sockaddr_un addr;
    oomaya_wm_state_t state;
    oomaya_worker_pool_t workers;
    struct oomaya_reactor reactor;
    pthread_t r_tid;
    oomaya_ipc_header_t bad;
    ssize_t rw;

    /* The daemon ignores SIGPIPE in main(); the in-process reactor must too. */
    signal(SIGPIPE, SIG_IGN);

    snprintf(sock_path, sizeof(sock_path), "/tmp/test_oomaya_live_%d.sock", (int)getpid());
    unlink(sock_path);

    listen_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sock_path);
    bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(listen_fd, 4);

    oomaya_state_init(&state);
    oomaya_state_set_tag_mask(&state, 0x1234);

    ASSERT_EQ("worker pool init", oomaya_worker_pool_init(&workers, 2, &state, test_dispatch), 0);
    ASSERT_EQ("reactor init", oomaya_reactor_init(&reactor, listen_fd, &workers, &state), 0);

    pthread_create(&r_tid, NULL, reactor_thread, &reactor);

    /* Client connects */
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_EQ("client connect", connect(client_fd, (struct sockaddr *)&addr, sizeof(addr)), 0);

    /* 1. Send Fast-Path (WM_GET_STATE) */
    {
        oomaya_ipc_header_t req, resp;
        uint16_t op = IPC_CMD_WM_GET_STATE;
        uint8_t resp_body[64];

        oomaya_frame_init_request(&req, op, 0x1010, 2);
        rw = write(client_fd, &req, sizeof(req)); (void)rw;
        rw = write(client_fd, &op, sizeof(op)); (void)rw;

        /* Read response */
        rw = read(client_fd, &resp, sizeof(resp)); (void)rw;
        ASSERT_EQ("fast-path magic", resp.magic, OOMAYA_IPC_MAGIC);
        ASSERT_EQ("fast-path type", resp.msg_type, (uint16_t)OOMAYA_MSG_RESPONSE);
        ASSERT_EQ("fast-path msg_id", resp.msg_id, 0x1010);

        rw = read(client_fd, resp_body, resp.payload_len); (void)rw;
        /* resp_body layout: status:4, tag_mask:4 */
        uint32_t got_mask;
        memcpy(&got_mask, resp_body + 4, 4);
        ASSERT_EQ("fast-path tag_mask", got_mask, 0x1234);
    }

    /* 2. Send Worker Offload (SYS_GET_POWER) */
    {
        oomaya_ipc_header_t req, resp;
        uint16_t op = IPC_CMD_SYS_GET_POWER;
        int32_t val = 0;

        oomaya_frame_init_request(&req, op, 0x2020, 2);
        rw = write(client_fd, &req, sizeof(req)); (void)rw;
        rw = write(client_fd, &op, sizeof(op)); (void)rw;

        /* Read response from worker via eventfd! */
        rw = read(client_fd, &resp, sizeof(resp)); (void)rw;
        ASSERT_EQ("offload magic", resp.magic, OOMAYA_IPC_MAGIC);
        ASSERT_EQ("offload type", resp.msg_type, (uint16_t)OOMAYA_MSG_RESPONSE);
        ASSERT_EQ("offload msg_id", resp.msg_id, 0x2020);
        ASSERT_EQ("offload payload_len", resp.payload_len, sizeof(val));

        rw = read(client_fd, &val, sizeof(val)); (void)rw;
        ASSERT_EQ("offload result value", val, 95);
    }

    close(client_fd);

    /* 3. Frame contract: boundary, oversize disconnects, no inherited desync */
    test_getstate_512_boundary_ok(&addr);
    bad = violation_req(OOMAYA_MSG_REQUEST, OOMAYA_FRAME_BODY_MAX + 1u, 0x4001);
    check_violation("T02 oversize header only", &addr, &bad, 0,
                    OOMAYA_IPC_ERR_PAYLOAD);
    bad.msg_id = 0x4002;
    check_violation("T02 oversize with body", &addr, &bad, bad.payload_len,
                    OOMAYA_IPC_ERR_PAYLOAD);
    test_reconnect_after_oversize_is_healthy(&addr);
    test_unknown_opcode_stays_connected(&addr);
    bad = violation_req(OOMAYA_MSG_ERROR, 600, 0x4003);
    check_violation("T05 oversize ERROR frame", &addr, &bad, bad.payload_len,
                    OOMAYA_IPC_ERR_PAYLOAD);
    bad = violation_req(OOMAYA_MSG_REQUEST, 2, 0x4004);
    bad.magic ^= 1u;
    check_violation("T06 bad magic", &addr, &bad, bad.payload_len,
                    OOMAYA_IPC_ERR_MALFORMED);
    bad = violation_req(OOMAYA_MSG_REQUEST, 2, 0x4005);
    bad.client_ver = 0;
    check_violation("T07 unknown client_ver", &addr, &bad, bad.payload_len,
                    OOMAYA_IPC_ERR_VERSION);
    bad = violation_req(OOMAYA_MSG_REQUEST, 2, 0x4006);
    bad.target_ver = 0;
    check_violation("T07 unknown target_ver", &addr, &bad, bad.payload_len,
                    OOMAYA_IPC_ERR_VERSION);
    test_pipeline_oversize_never_desyncs(&addr);
    test_completion_not_delivered_to_reused_fd(&addr);
    test_client_that_stops_reading_is_dropped(&addr);
    test_subscribers_receive_tag_signal(&addr);
    test_slow_subscriber_is_dropped(&addr);

    /* Shutdown */
    oomaya_reactor_stop(&reactor);
    pthread_join(r_tid, NULL);
    oomaya_reactor_destroy(&reactor);
    oomaya_worker_pool_shutdown(&workers);
    close(listen_fd);
    unlink(sock_path);

    printf("\n=== Live Integration: %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
