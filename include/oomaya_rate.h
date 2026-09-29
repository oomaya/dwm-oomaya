#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
/* oomaya_rate.h — Per-client token-bucket rate limiter.
 *
 * Phase 2 Core — Operational Shield (Priority 1).
 *
 * Design:
 *   • One fixed-size pool of OOMAYA_RATE_MAX_CLIENTS buckets.
 *   • Zero dynamic allocation — all state lives in the pool array.
 *   • Clock source: CLOCK_MONOTONIC (ns resolution).
 *   • Token arithmetic in integer nanoseconds — no floating point.
 *   • Thread safety: each bucket is owned exclusively by the reactor
 *     thread (main loop). Workers never touch rate state.
 *
 * Token-bucket parameters (tunable at compile time):
 *   OOMAYA_RATE_CAPACITY_MSGS  — burst capacity (max token accumulation)
 *   OOMAYA_RATE_REFILL_PER_SEC — steady-state refill rate (msgs/sec)
 *
 * Usage:
 *   oomaya_rate_pool_t pool;
 *   oomaya_rate_pool_init(&pool);
 *
 *   // On new client connect:
 *   int slot = oomaya_rate_client_add(&pool, client_fd);
 *
 *   // On each incoming frame:
 *   if (!oomaya_rate_check(&pool, slot)) { // deny + send ERR }
 *
 *   // On client disconnect:
 *   oomaya_rate_client_remove(&pool, slot);
 */

#ifndef OOMAYA_RATE_H
#define OOMAYA_RATE_H

#include <stdint.h>
#include <time.h>   /* struct timespec, clock_gettime */

/* ── Tunable parameters ──────────────────────────────────────────────────── */

/* Maximum simultaneous connected clients. */
#ifndef OOMAYA_RATE_MAX_CLIENTS
#define OOMAYA_RATE_MAX_CLIENTS   64
#endif

/* Burst capacity: max tokens a fresh or idle client may accumulate. */
#ifndef OOMAYA_RATE_CAPACITY_MSGS
#define OOMAYA_RATE_CAPACITY_MSGS 100
#endif

/* Steady-state refill rate in messages per second. */
#ifndef OOMAYA_RATE_REFILL_PER_SEC
#define OOMAYA_RATE_REFILL_PER_SEC 50
#endif

/* ── Internal constants (derived — do not touch) ─────────────────────────── */

/* Nanoseconds per token (integer arithmetic — avoids fp). */
#define OOMAYA_RATE_NS_PER_TOKEN \
    (UINT64_C(1000000000) / OOMAYA_RATE_REFILL_PER_SEC)

/* ── Per-client token bucket ─────────────────────────────────────────────── */
typedef struct {
    uint64_t tokens_x1000;    /* token count × 1000 (sub-token precision) */
    uint64_t last_refill_ns;  /* CLOCK_MONOTONIC timestamp of last refill */
    int      client_fd;       /* owning socket fd (-1 = free slot) */
} oomaya_rate_bucket_t;

/* ── Pool of buckets (stack-allocatable) ─────────────────────────────────── */
typedef struct {
    oomaya_rate_bucket_t buckets[OOMAYA_RATE_MAX_CLIENTS];
    int                  count; /* number of active clients */
} oomaya_rate_pool_t;

/* ── API ─────────────────────────────────────────────────────────────────── */

/**
 * oomaya_rate_pool_init() — Zero-initialise the pool.
 * Must be called once before any other rate function.
 */
void oomaya_rate_pool_init(oomaya_rate_pool_t *pool);

/**
 * oomaya_rate_client_add() — Claim a free bucket for a new client.
 *
 * @pool       The pool.
 * @client_fd  The accepted socket file descriptor.
 * @return     Slot index [0, OOMAYA_RATE_MAX_CLIENTS), or -1 if pool full.
 */
int oomaya_rate_client_add(oomaya_rate_pool_t *pool, int client_fd);

/**
 * oomaya_rate_client_remove() — Release a client's bucket back to the pool.
 *
 * @pool  The pool.
 * @slot  Slot index returned by oomaya_rate_client_add().
 */
void oomaya_rate_client_remove(oomaya_rate_pool_t *pool, int slot);

/**
 * oomaya_rate_check() — Attempt to consume one token from a client's bucket.
 *
 * Performs a lazy refill based on elapsed monotonic time, then consumes
 * one token if available.
 *
 * @pool  The pool.
 * @slot  Slot index returned by oomaya_rate_client_add().
 * @return 1 if the message is allowed; 0 if the client is over-rate and the
 *         frame must be rejected with OOMAYA_IPC_ERR_MALFORMED (or a
 *         dedicated RATE_LIMIT error that Phase 3 will add).
 *
 * Zero allocations. O(1). Safe to call on every reactor frame.
 */
int oomaya_rate_check(oomaya_rate_pool_t *pool, int slot);

/**
 * oomaya_rate_now_ns() — Return CLOCK_MONOTONIC in nanoseconds.
 * Exposed for testability — tests may call this to verify timestamps.
 */
uint64_t oomaya_rate_now_ns(void);

#endif /* OOMAYA_RATE_H */
