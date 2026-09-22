/* test_daemon_core.c — Phase 3 daemon core unit tests.
 *
 * Groups:
 *   STATE  S01–S06  Atomic WM state cache
 *   WORKER W01–W05  pthread pool + eventfd
 *   REACT  R01–R04  Handler responses via socketpair()
 *   PATH   P01–P02  Socket path resolution
 *
 * Zero dynamic allocations. No X11 required.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/eventfd.h>

#include "../include/oomaya_ipc.h"
#include "../include/oomaya_state.h"
#include "../include/oomaya_worker.h"
#include "../include/oomaya_rate.h"

/* ── Harness ─────────────────────────────────────────────────────────────── */
static int g_pass = 0, g_fail = 0;

#define ASSERT_EQ(lbl, got, exp) \
    do { \
        long long _g=(long long)(got), _e=(long long)(exp); \
        if (_g==_e){printf("  [PASS] %s\n",(lbl));++g_pass;} \
        else{printf("  [FAIL] %s got=%lld exp=%lld\n",(lbl),_g,_e);++g_fail;} \
    } while(0)
#define ASSERT_GT(lbl, got, floor) \
    do { \
        long long _g=(long long)(got),_f=(long long)(floor); \
        if(_g>_f){printf("  [PASS] %s (%lld > %lld)\n",(lbl),_g,_f);++g_pass;} \
        else{printf("  [FAIL] %s got=%lld not>%lld\n",(lbl),_g,_f);++g_fail;} \
    } while(0)
#define TEST(n) printf("\n[TEST] " n "\n")

/* ── Helpers ─────────────────────────────────────────────────────────────── */

static int
recv_timeout_ms(int fd, void *buf, size_t len, int ms)
{
    fd_set rfds; struct timeval tv;
    FD_ZERO(&rfds); FD_SET(fd, &rfds);
    tv.tv_sec = ms/1000; tv.tv_usec = (ms%1000)*1000;
    if (select(fd+1, &rfds, NULL, NULL, &tv) <= 0) return -1;
    return (int)recv(fd, buf, len, 0);
}

static void
sock_writev2(int fd, const void *a, size_t alen, const void *b, size_t blen)
{
    struct iovec iov[2]; ssize_t _r;
    iov[0].iov_base=(void*)a; iov[0].iov_len=alen;
    iov[1].iov_base=(void*)b; iov[1].iov_len=blen;
    _r=writev(fd, iov, 2); (void)_r;
}

static void
sock_writev1(int fd, const void *a, size_t alen)
{
    struct iovec iov[1]; ssize_t _r;
    iov[0].iov_base=(void*)a; iov[0].iov_len=alen;
    _r=writev(fd, iov, 1); (void)_r;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* STATE CACHE                                                                */
/* ══════════════════════════════════════════════════════════════════════════ */

static void s01_init(void)
{
    oomaya_wm_state_t s;
    TEST("S01: State cache init");
    oomaya_state_init(&s);
    ASSERT_EQ("tag_mask==0",      oomaya_state_get_tag_mask(&s),     0u);
    ASSERT_EQ("layout_idx==0",    oomaya_state_get_layout(&s),        0u);
    ASSERT_EQ("focused_win==0",   oomaya_state_get_focused(&s),       0u);
    ASSERT_EQ("monitor_count==1", oomaya_state_get_monitor_count(&s), 1u);
}

static void s02_tag_mask(void)
{
    oomaya_wm_state_t s;
    TEST("S02: tag_mask get/set round-trip");
    oomaya_state_init(&s);
    oomaya_state_set_tag_mask(&s, 0xDEADu);
    ASSERT_EQ("round-trip", oomaya_state_get_tag_mask(&s), 0xDEADu);
}

static void s03_layout(void)
{
    oomaya_wm_state_t s;
    TEST("S03: layout_idx get/set round-trip");
    oomaya_state_init(&s);
    oomaya_state_set_layout(&s, 7u);
    ASSERT_EQ("round-trip", oomaya_state_get_layout(&s), 7u);
}

static void s04_focused(void)
{
    oomaya_wm_state_t s;
    TEST("S04: focused_win get/set round-trip");
    oomaya_state_init(&s);
    oomaya_state_set_focused(&s, 0xCAFEBABEu);
    ASSERT_EQ("round-trip", oomaya_state_get_focused(&s), 0xCAFEBABEu);
}

static void s05_monitor_count(void)
{
    oomaya_wm_state_t s;
    TEST("S05: monitor_count get/set round-trip");
    oomaya_state_init(&s);
    oomaya_state_set_monitor_count(&s, 3u);
    ASSERT_EQ("round-trip", oomaya_state_get_monitor_count(&s), 3u);
}

static void s06_monitor_tags(void)
{
    oomaya_wm_state_t s;
    TEST("S06: per-monitor tags + out-of-range clamp");
    oomaya_state_init(&s);
    oomaya_state_set_monitor_tag(&s, 0u, 0xAAu);
    oomaya_state_set_monitor_tag(&s, 3u, 0xBBu);
    ASSERT_EQ("mon[0]", oomaya_state_get_monitor_tag(&s, 0u), 0xAAu);
    ASSERT_EQ("mon[3]", oomaya_state_get_monitor_tag(&s, 3u), 0xBBu);
    ASSERT_EQ("mon[4] OOB→0", oomaya_state_get_monitor_tag(&s, 4u), 0u);
    oomaya_state_set_monitor_tag(&s, 99u, 0xFFu); /* must not crash */
    printf("  [PASS] OOB set no crash\n"); ++g_pass;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* WORKER POOL                                                                */
/* ══════════════════════════════════════════════════════════════════════════ */

static void
echo_dispatch(oomaya_task_t *task, oomaya_wm_state_t *state)
{
    (void)state;
    task->status   = 0;
    task->resp_len = 2;
    memcpy(task->resp_data, &task->opcode, 2);
}

static void w01_pool_init(void)
{
    oomaya_wm_state_t s; oomaya_worker_pool_t p;
    TEST("W01: Worker pool init (2 threads)");
    oomaya_state_init(&s);
    ASSERT_EQ("init==0", oomaya_worker_pool_init(&p, 2u, &s, echo_dispatch), 0);
    ASSERT_EQ("count==2", p.thread_count, 2u);
    oomaya_worker_pool_shutdown(&p);
}

static void w02_eventfd_valid(void)
{
    oomaya_wm_state_t s; oomaya_worker_pool_t p;
    TEST("W02: eventfd >= 0 after init");
    oomaya_state_init(&s);
    oomaya_worker_pool_init(&p, 2u, &s, echo_dispatch);
    ASSERT_GT("eventfd>=0", p.eventfd, -1);
    oomaya_worker_pool_shutdown(&p);
}

static void w03_dispatch_fires(void)
{
    oomaya_wm_state_t s; oomaya_worker_pool_t p;
    oomaya_task_t task;
    fd_set rfds; struct timeval tv;
    uint64_t val = 0;
    int rc;

    TEST("W03: Submit task → dispatch fires → eventfd readable");
    oomaya_state_init(&s);
    oomaya_worker_pool_init(&p, 2u, &s, echo_dispatch);

    memset(&task, 0, sizeof(task));
    task.opcode = IPC_CMD_SYS_GET_POWER;

    ASSERT_EQ("submit OK", oomaya_worker_pool_submit(&p, &task), OOMAYA_RING_OK);

    FD_ZERO(&rfds); FD_SET(p.eventfd, &rfds);
    tv.tv_sec = 0; tv.tv_usec = 500000;
    rc = select(p.eventfd+1, &rfds, NULL, NULL, &tv);
    ASSERT_GT("eventfd readable", rc, 0);
    if (rc > 0) {
        rc = (int)read(p.eventfd, &val, sizeof(val));
        (void)rc;
        ASSERT_GT("counter>=1", (long long)val, 0LL);
    }

    oomaya_worker_pool_shutdown(&p);
}

static void w04_shutdown_clean(void)
{
    oomaya_wm_state_t s; oomaya_worker_pool_t p;
    TEST("W04: Shutdown returns and closes eventfd");
    oomaya_state_init(&s);
    oomaya_worker_pool_init(&p, 4u, &s, echo_dispatch);
    oomaya_worker_pool_shutdown(&p);
    printf("  [PASS] shutdown returned\n"); ++g_pass;
    ASSERT_EQ("eventfd==-1 after shutdown", p.eventfd, -1);
}

static void w05_null_submit(void)
{
    oomaya_task_t t;
    TEST("W05: Submit to NULL pool → RING_FULL");
    memset(&t, 0, sizeof(t));
    ASSERT_EQ("NULL pool→FULL", oomaya_worker_pool_submit(NULL, &t), OOMAYA_RING_FULL);
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* REACTOR HANDLER TESTS (inline via socketpair)                             */
/* ══════════════════════════════════════════════════════════════════════════ */

/* Miniature handler shims — mirror exactly what reactor.c does. */

typedef struct __attribute__((packed)) {
    int32_t  status;
    uint32_t tag_mask;
    uint32_t layout_idx;
    uint32_t focused_win;
    uint32_t monitor_count;
} state_resp_t;

static void
shim_wm_get_state(int client_fd,
                  const oomaya_ipc_header_t *req,
                  oomaya_wm_state_t *st)
{
    oomaya_ipc_header_t resp;
    state_resp_t        body;
    body.status        = 0;
    body.tag_mask      = oomaya_state_get_tag_mask(st);
    body.layout_idx    = oomaya_state_get_layout(st);
    body.focused_win   = oomaya_state_get_focused(st);
    body.monitor_count = oomaya_state_get_monitor_count(st);
    oomaya_frame_init_response(&resp, req, (uint32_t)sizeof(body));
    sock_writev2(client_fd, &resp, sizeof(resp), &body, sizeof(body));
}

static void
shim_wm_view_tag(int client_fd,
                 const oomaya_ipc_header_t *req,
                 const uint8_t *body_bytes,
                 oomaya_wm_state_t *st)
{
    oomaya_ipc_header_t resp;
    int32_t st_code = 0;
    uint32_t mask = 0;
    if (req->payload_len >= 6) { memcpy(&mask, body_bytes+2, 4); oomaya_state_set_tag_mask(st, mask); }
    else st_code = -1;
    oomaya_frame_init_response(&resp, req, (uint32_t)sizeof(st_code));
    sock_writev2(client_fd, &resp, sizeof(resp), &st_code, sizeof(st_code));
}

static void
shim_wm_set_layout(int client_fd,
                   const oomaya_ipc_header_t *req,
                   const uint8_t *body_bytes,
                   oomaya_wm_state_t *st)
{
    oomaya_ipc_header_t resp;
    int32_t st_code = 0;
    uint32_t idx = 0;
    if (req->payload_len >= 6) { memcpy(&idx, body_bytes+2, 4); oomaya_state_set_layout(st, idx); }
    else st_code = -1;
    oomaya_frame_init_response(&resp, req, (uint32_t)sizeof(st_code));
    sock_writev2(client_fd, &resp, sizeof(resp), &st_code, sizeof(st_code));
}

static void r01_get_state(void)
{
    int sv[2]; oomaya_wm_state_t st; oomaya_ipc_header_t req, resp;
    uint16_t op = IPC_CMD_WM_GET_STATE;
    TEST("R01: WM_GET_STATE → valid response header");
    oomaya_state_init(&st);
    oomaya_state_set_tag_mask(&st, 0xABCDu);
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) { printf("  [SKIP]\n"); return; }
    oomaya_frame_init_request(&req, op, 0x0042u, 2u);
    req.payload_len = 2;
    shim_wm_get_state(sv[0], &req, &st);
    memset(&resp, 0, sizeof(resp));
    recv_timeout_ms(sv[1], &resp, sizeof(resp), 300);
    ASSERT_EQ("resp magic",    (uint32_t)resp.magic,    (uint32_t)OOMAYA_IPC_MAGIC);
    ASSERT_EQ("resp RESPONSE", resp.msg_type,            (uint16_t)OOMAYA_MSG_RESPONSE);
    ASSERT_EQ("resp msg_id",   resp.msg_id,              req.msg_id);
    close(sv[0]); close(sv[1]);
}

static void r02_view_tag(void)
{
    int sv[2]; oomaya_wm_state_t st; oomaya_ipc_header_t req;
    uint8_t body[6]; uint16_t op = IPC_CMD_WM_VIEW_TAG; uint32_t new_mask = 0x00FFu;
    TEST("R02: WM_VIEW_TAG → state tag_mask updated");
    oomaya_state_init(&st);
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) { printf("  [SKIP]\n"); return; }
    memcpy(body, &op, 2); memcpy(body+2, &new_mask, 4);
    oomaya_frame_init_request(&req, op, 0x0001u, sizeof(body));
    req.payload_len = sizeof(body);
    shim_wm_view_tag(sv[0], &req, body, &st);
    ASSERT_EQ("tag_mask updated", oomaya_state_get_tag_mask(&st), new_mask);
    close(sv[0]); close(sv[1]);
}

static void r03_set_layout(void)
{
    int sv[2]; oomaya_wm_state_t st; oomaya_ipc_header_t req;
    uint8_t body[6]; uint16_t op = IPC_CMD_WM_SET_LAYOUT; uint32_t new_idx = 3u;
    TEST("R03: WM_SET_LAYOUT → state layout_idx updated");
    oomaya_state_init(&st);
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) { printf("  [SKIP]\n"); return; }
    memcpy(body, &op, 2); memcpy(body+2, &new_idx, 4);
    oomaya_frame_init_request(&req, op, 0x0002u, sizeof(body));
    req.payload_len = sizeof(body);
    shim_wm_set_layout(sv[0], &req, body, &st);
    ASSERT_EQ("layout updated", oomaya_state_get_layout(&st), new_idx);
    close(sv[0]); close(sv[1]);
}

static void r04_bad_magic_error(void)
{
    int sv[2]; oomaya_ipc_header_t bad, resp, err;
    TEST("R04: Bad magic → OOMAYA_MSG_ERROR response");
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) { printf("  [SKIP]\n"); return; }
    memset(&bad, 0, sizeof(bad));
    bad.magic = 0xDEADBEEFu;
    bad.client_ver = OOMAYA_IPC_VERSION_1_0;
    bad.target_ver = OOMAYA_IPC_VERSION_1_0;
    if (oomaya_frame_validate(&bad) != OOMAYA_IPC_OK) {
        oomaya_frame_init_error(&err, NULL, OOMAYA_IPC_ERR_MALFORMED);
        sock_writev1(sv[0], &err, sizeof(err));
    }
    memset(&resp, 0, sizeof(resp));
    recv_timeout_ms(sv[1], &resp, sizeof(resp), 300);
    ASSERT_EQ("resp magic",    (uint32_t)resp.magic, (uint32_t)OOMAYA_IPC_MAGIC);
    ASSERT_EQ("resp is ERROR", resp.msg_type, (uint16_t)OOMAYA_MSG_ERROR);
    close(sv[0]); close(sv[1]);
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* SOCKET PATH RESOLUTION                                                    */
/* ══════════════════════════════════════════════════════════════════════════ */

static int
resolve_sock(char *buf, size_t len)
{
    const char *xdg = getenv("XDG_RUNTIME_DIR");
    int n;
    if (xdg != NULL && xdg[0] != '\0')
        n = snprintf(buf, len, "%s/dwm-oomaya/ipc.sock", xdg);
    else
        n = snprintf(buf, len, "/tmp/dwm-oomaya-%u/ipc.sock", (unsigned)getuid());
    return (n > 0 && (size_t)n < len) ? 0 : -1;
}

static void p01_xdg_path(void)
{
    char buf[256];
    TEST("P01: XDG_RUNTIME_DIR set → XDG path used");
    setenv("XDG_RUNTIME_DIR", "/run/user/9999", 1);
    resolve_sock(buf, sizeof(buf));
    ASSERT_EQ("starts with XDG", strncmp(buf, "/run/user/9999", 14u), 0);
    unsetenv("XDG_RUNTIME_DIR");
}

static void p02_uid_fallback(void)
{
    char buf[256];
    TEST("P02: XDG unset → /tmp/dwm-oomaya-{UID} fallback");
    unsetenv("XDG_RUNTIME_DIR");
    resolve_sock(buf, sizeof(buf));
    ASSERT_EQ("starts with /tmp", strncmp(buf, "/tmp/dwm-oomaya-", 16u), 0);
}

/* ══════════════════════════════════════════════════════════════════════════ */

int main(void)
{
    printf("=== oomaya Phase 3 daemon core tests ===\n");

    printf("\n─── State Cache ───\n");
    s01_init(); s02_tag_mask(); s03_layout();
    s04_focused(); s05_monitor_count(); s06_monitor_tags();

    printf("\n─── Worker Pool ───\n");
    w01_pool_init(); w02_eventfd_valid(); w03_dispatch_fires();
    w04_shutdown_clean(); w05_null_submit();

    printf("\n─── Reactor Handlers (socketpair) ───\n");
    r01_get_state(); r02_view_tag(); r03_set_layout(); r04_bad_magic_error();

    printf("\n─── Socket Path Resolution ───\n");
    p01_xdg_path(); p02_uid_fallback();

    printf("\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
    return (g_fail == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
