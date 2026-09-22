/* test_integration_live.c — Full live reactor + worker pool integration test. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

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

int main(void)
{
    char sock_path[100];
    int listen_fd, client_fd;
    struct sockaddr_un addr;
    oomaya_wm_state_t state;
    oomaya_worker_pool_t workers;
    struct oomaya_reactor reactor;
    pthread_t r_tid;
    ssize_t rw;

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
