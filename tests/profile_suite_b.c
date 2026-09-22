/* profile_suite_b.c — Nanosecond stage profiler for Suite B (Worker Offload).
 *
 * Breaks down each stage of the offload round-trip:
 *   Stage 1: Client writev -> Reactor reads frame & enqueues to ring
 *   Stage 2: sem_post -> Worker thread wakes from sem_wait
 *   Stage 3: Worker dispatch -> Worker enqueues to resp_ring & writes eventfd
 *   Stage 4: write(eventfd) -> Reactor wakes from epoll_wait
 *   Stage 5: Reactor writev -> Client read receives response
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>

#include "../include/oomaya_ipc.h"
#include "../include/oomaya_state.h"
#include "../include/oomaya_worker.h"
#include "../include/oomaya_reactor.h"
#include "../include/oomaya_reactor_impl.h"

#define ITERS 10000u

static uint64_t t_client_send[ITERS];
static uint64_t t_worker_wake[ITERS];
static uint64_t t_worker_done[ITERS];
static uint64_t t_client_recv[ITERS];


static inline uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

static int cmp_u64(const void *a, const void *b) {
    uint64_t va = *(const uint64_t *)a, vb = *(const uint64_t *)b;
    return (va > vb) - (va < vb);
}

static void print_percentiles(const char *label, uint64_t *arr, size_t n) {
    qsort(arr, n, sizeof(uint64_t), cmp_u64);
    double min = (double)arr[0] / 1000.0;
    double p50 = (double)arr[n * 50 / 100] / 1000.0;
    double p95 = (double)arr[n * 95 / 100] / 1000.0;
    double p99 = (double)arr[n * 99 / 100] / 1000.0;
    double max = (double)arr[n - 1] / 1000.0;
    printf("  %-42s  Min: %6.2f µs | P50: %6.2f µs | P95: %6.2f µs | P99: %6.2f µs | Max: %7.2f µs\n",
           label, min, p50, p95, p99, max);
}

/* Custom dispatch that records timestamps */
static void profile_dispatch(oomaya_task_t *task, oomaya_wm_state_t *state) {
    uint32_t it = task->msg_id;
    if (it < ITERS) {
        t_worker_wake[it] = now_ns();
    }
    int32_t val = 42;
    task->status = 0;
    task->resp_len = sizeof(val);
    memcpy(task->resp_data, &val, sizeof(val));
    if (it < ITERS) {
        t_worker_done[it] = now_ns();
    }
}

static void *reactor_th(void *arg) {
    oomaya_reactor_run((struct oomaya_reactor *)arg);
    return NULL;
}

int main(void) {
    char sock_path[108];
    int listen_fd, client_fd;
    struct sockaddr_un addr;
    oomaya_wm_state_t state;
    oomaya_worker_pool_t workers;
    struct oomaya_reactor reactor;
    pthread_t r_tid;
    ssize_t rw;

    snprintf(sock_path, sizeof(sock_path), "/tmp/prof_oomaya_%d.sock", (int)getpid());
    unlink(sock_path);

    listen_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sock_path);
    bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(listen_fd, 8);

    oomaya_state_init(&state);
    oomaya_worker_pool_init(&workers, 4, &state, profile_dispatch);
    oomaya_reactor_init(&reactor, listen_fd, &workers, &state);

    pthread_create(&r_tid, NULL, reactor_th, &reactor);

    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    connect(client_fd, (struct sockaddr *)&addr, sizeof(addr));

    printf("Profiling Suite B across %u iterations...\n", ITERS);

    for (uint32_t i = 0; i < ITERS; ++i) {
        oomaya_ipc_header_t req, resp;
        uint16_t op = IPC_CMD_SYS_GET_POWER;
        int32_t val = 0;
        struct iovec iov[2];

        oomaya_frame_init_request(&req, op, (uint16_t)i, 2);
        iov[0].iov_base = &req; iov[0].iov_len = sizeof(req);
        iov[1].iov_base = &op;  iov[1].iov_len = sizeof(op);

        t_client_send[i] = now_ns();
        rw = writev(client_fd, iov, 2); (void)rw;
        rw = read(client_fd, &resp, sizeof(resp)); (void)rw;
        if (resp.payload_len > 0) {
            rw = read(client_fd, &val, sizeof(val)); (void)rw;
        }
        t_client_recv[i] = now_ns();
    }

    /* Compute stage deltas */
    uint64_t total_roundtrip[ITERS];
    uint64_t send_to_wake[ITERS];
    uint64_t wake_to_done[ITERS];
    uint64_t done_to_recv[ITERS];

    for (uint32_t i = 0; i < ITERS; ++i) {
        total_roundtrip[i] = t_client_recv[i] - t_client_send[i];
        send_to_wake[i]    = t_worker_wake[i] > t_client_send[i] ? t_worker_wake[i] - t_client_send[i] : 0;
        wake_to_done[i]    = t_worker_done[i] > t_worker_wake[i] ? t_worker_done[i] - t_worker_wake[i] : 0;
        done_to_recv[i]    = t_client_recv[i] > t_worker_done[i] ? t_client_recv[i] - t_worker_done[i] : 0;
    }

    printf("\n============================================================================================================\n");
    printf("  SUITE B LATENCY BREAKDOWN (MICROSECOND GRANULARITY)\n");
    printf("============================================================================================================\n");
    print_percentiles("Stage 1+2: Client Send -> Worker Wake (sem_wait)", send_to_wake, ITERS);
    print_percentiles("Stage 3:   Worker Dispatch Execution",               wake_to_done, ITERS);
    print_percentiles("Stage 4+5: Worker Done -> Reactor -> Client Recv",    done_to_recv, ITERS);
    printf("────────────────────────────────────────────────────────────────────────────────────────────────────────────\n");
    print_percentiles("TOTAL ROUNDTRIP LATENCY (End-to-End)",               total_roundtrip, ITERS);
    printf("============================================================================================================\n");

    close(client_fd);
    oomaya_reactor_stop(&reactor);
    pthread_join(r_tid, NULL);
    oomaya_reactor_destroy(&reactor);
    oomaya_worker_pool_shutdown(&workers);
    close(listen_fd);
    unlink(sock_path);

    return 0;
}
