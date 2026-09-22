/* bench_ipc_roundtrip.c — High-precision IPC round-trip latency benchmark.
 *
 * Measures:
 *   Suite A: Base Table Fast-Path (IPC_CMD_WM_GET_STATE) — 100,000 iterations
 *   Suite B: Derived Join Worker Offload (IPC_CMD_SYS_GET_POWER) — 20,000 iterations
 *
 * Metrics:
 *   Min, P50 (median), P95, P99, Max, Mean, Standard Deviation (in microseconds).
 *   Uses CLOCK_MONOTONIC_RAW for clock drift / NTP-free nanosecond accuracy.
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
#include <sys/uio.h>
#include <sys/un.h>
#include <errno.h>

#include "../include/oomaya_ipc.h"
#include "../include/oomaya_state.h"
#include "../include/oomaya_worker.h"
#include "../include/oomaya_reactor.h"
#include "../include/oomaya_reactor_impl.h"

#define SUITE_A_ITERS 100000u
#define SUITE_B_ITERS  20000u
#define WARMUP_ITERS    1000u

static uint64_t g_latencies[SUITE_A_ITERS];

static int cmp_u64(const void *a, const void *b)
{
    uint64_t va = *(const uint64_t *)a;
    uint64_t vb = *(const uint64_t *)b;
    return (va > vb) - (va < vb);
}

static inline uint64_t get_time_raw_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

/* Worker handler simulating a derived join (sysfs / hardware query) */
static void bench_worker_dispatch(oomaya_task_t *task, oomaya_wm_state_t *state)
{
    /* Simulate derived join aggregation across state + foreign telemetry */
    uint32_t mask = oomaya_state_get_tag_mask(state);
    int32_t val = (int32_t)(mask ^ 0x5A5A);
    task->status = 0;
    task->resp_len = sizeof(val);
    memcpy(task->resp_data, &val, sizeof(val));
}

static void *reactor_runner(void *arg)
{
    struct oomaya_reactor *r = (struct oomaya_reactor *)arg;
    oomaya_reactor_run(r);
    return NULL;
}

static void print_stats(const char *name, uint64_t *lats, size_t n)
{
    qsort(lats, n, sizeof(uint64_t), cmp_u64);

    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) sum += (double)lats[i];
    double mean = sum / (double)n;

    double var_sum = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double diff = (double)lats[i] - mean;
        var_sum += diff * diff;
    }
    double stddev = sqrt(var_sum / (double)n);

    double min_us  = (double)lats[0] / 1000.0;
    double p50_us  = (double)lats[n * 50 / 100] / 1000.0;
    double p95_us  = (double)lats[n * 95 / 100] / 1000.0;
    double p99_us  = (double)lats[n * 99 / 100] / 1000.0;
    double max_us  = (double)lats[n - 1] / 1000.0;
    double mean_us = mean / 1000.0;
    double std_us  = stddev / 1000.0;

    printf("\n======================================================================\n");
    printf("  BENCHMARK RESULTS: %s (%zu samples)\n", name, n);
    printf("======================================================================\n");
    printf("  Metric         Latency (microseconds)\n");
    printf("  ──────         ──────────────────────\n");
    printf("  Min:           %8.2f µs\n", min_us);
    printf("  P50 (Median):  %8.2f µs\n", p50_us);
    printf("  P95:           %8.2f µs   <-- Key Target (<50 µs)\n", p95_us);
    printf("  P99:           %8.2f µs\n", p99_us);
    printf("  Max:           %8.2f µs\n", max_us);
    printf("  Mean:          %8.2f µs  (± %6.2f µs)\n", mean_us, std_us);
    printf("  Throughput:    %8.0f ops/sec\n", 1000000.0 / mean_us);
    printf("======================================================================\n");
}

int main(void)
{
    char sock_path[108];
    int listen_fd, client_fd;
    struct sockaddr_un addr;
    oomaya_wm_state_t state;
    oomaya_worker_pool_t workers;
    struct oomaya_reactor reactor;
    pthread_t tid;
    ssize_t rw;

    snprintf(sock_path, sizeof(sock_path), "/tmp/bench_oomaya_%d.sock", (int)getpid());
    unlink(sock_path);

    listen_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sock_path);
    bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(listen_fd, 8);

    oomaya_state_init(&state);
    oomaya_state_set_tag_mask(&state, 0xBEEF);

    if (oomaya_worker_pool_init(&workers, 4, &state, bench_worker_dispatch) < 0) {
        fprintf(stderr, "Worker pool init failed\n");
        return 1;
    }
    if (oomaya_reactor_init(&reactor, listen_fd, &workers, &state) < 0) {
        fprintf(stderr, "Reactor init failed\n");
        return 1;
    }

    pthread_create(&tid, NULL, reactor_runner, &reactor);

    /* Connect benchmark client */
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (connect(client_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }

    /* ── Warm-up ─────────────────────────────────────────────────────────── */
    printf("Warming up (%u iterations)...\n", WARMUP_ITERS);
    for (uint32_t i = 0; i < WARMUP_ITERS; ++i) {
        oomaya_ipc_header_t req, resp;
        uint16_t op = IPC_CMD_WM_GET_STATE;
        oomaya_frame_init_request(&req, op, (uint16_t)i, 2);
        rw = write(client_fd, &req, sizeof(req)); (void)rw;
        rw = write(client_fd, &op, sizeof(op)); (void)rw;
        rw = read(client_fd, &resp, sizeof(resp)); (void)rw;
        if (resp.msg_type == (uint16_t)OOMAYA_MSG_RESPONSE && resp.payload_len > 0) {
            uint8_t dummy[64];
            uint32_t want = resp.payload_len > sizeof(dummy) ? sizeof(dummy) : resp.payload_len;
            rw = read(client_fd, dummy, want); (void)rw;
        }
    }

    /* ── Suite A: Base Table Fast-Path (100,000 iterations) ──────────────── */
    printf("Running Suite A: Base Table Fast-Path (100k requests)...\n");
    for (uint32_t i = 0; i < SUITE_A_ITERS; ++i) {
        oomaya_ipc_header_t req, resp;
        uint16_t op = IPC_CMD_WM_GET_STATE;
        uint8_t body[64];
        oomaya_frame_init_request(&req, op, (uint16_t)i, 2);

        struct iovec iov[2];
        iov[0].iov_base = &req; iov[0].iov_len = sizeof(req);
        iov[1].iov_base = &op;  iov[1].iov_len = sizeof(op);

        uint64_t t0 = get_time_raw_ns();
        rw = writev(client_fd, iov, 2); (void)rw;
        rw = read(client_fd, &resp, sizeof(resp)); (void)rw;
        if (resp.msg_type == (uint16_t)OOMAYA_MSG_RESPONSE && resp.payload_len > 0) {
            uint32_t want = resp.payload_len > sizeof(body) ? sizeof(body) : resp.payload_len;
            rw = read(client_fd, body, want); (void)rw;
        }
        uint64_t t1 = get_time_raw_ns();
        g_latencies[i] = t1 - t0;
    }
    print_stats("Suite A: Base Table Fast-Path (<10µs direct in-memory read)",
                g_latencies, SUITE_A_ITERS);

    /* ── Suite B: Derived Join Worker Offload (20,000 iterations) ────────── */
    printf("Running Suite B: Derived Join Worker Offload (20k requests)...\n");
    for (uint32_t i = 0; i < SUITE_B_ITERS; ++i) {
        oomaya_ipc_header_t req, resp;
        uint16_t op = IPC_CMD_SYS_GET_POWER;
        int32_t val = 0;
        oomaya_frame_init_request(&req, op, (uint16_t)i, 2);

        struct iovec iov[2];
        iov[0].iov_base = &req; iov[0].iov_len = sizeof(req);
        iov[1].iov_base = &op;  iov[1].iov_len = sizeof(op);

        uint64_t t0 = get_time_raw_ns();
        rw = writev(client_fd, iov, 2); (void)rw;
        rw = read(client_fd, &resp, sizeof(resp)); (void)rw;
        if (resp.msg_type == (uint16_t)OOMAYA_MSG_RESPONSE && resp.payload_len > 0) {
            uint32_t want = resp.payload_len > sizeof(val) ? sizeof(val) : resp.payload_len;
            rw = read(client_fd, &val, want); (void)rw;
        }
        uint64_t t1 = get_time_raw_ns();
        g_latencies[i] = t1 - t0;
    }
    print_stats("Suite B: Derived Join Worker Offload (lock-free ring + eventfd)",
                g_latencies, SUITE_B_ITERS);

    /* Cleanup */
    close(client_fd);
    oomaya_reactor_stop(&reactor);
    pthread_join(tid, NULL);
    oomaya_reactor_destroy(&reactor);
    oomaya_worker_pool_shutdown(&workers);
    close(listen_fd);
    unlink(sock_path);

    return 0;
}
