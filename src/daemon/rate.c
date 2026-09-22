/* SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L
/* rate.c — Per-client token-bucket rate limiter implementation.
 *
 * Phase 2 Core — Operational Shield (Priority 1).
 * C99. Zero dynamic allocations. No locks — reactor is single-threaded.
 *
 * Token arithmetic:
 *   All token counts are stored as (real_tokens × 1000) to gain three
 *   decimal places of precision without floating point. This allows the
 *   refill calculation to accumulate fractional tokens across many small
 *   time deltas without systematic loss.
 *
 *   Example (REFILL_PER_SEC = 50, NS_PER_TOKEN = 20_000_000):
 *     elapsed = 5_000_000 ns (5 ms)
 *     new_tokens_x1000 = (5_000_000 × 1000) / 20_000_000 = 250
 *     i.e. 0.250 real tokens accumulated — stored as 250 (×1000)
 *     Only when tokens_x1000 >= 1000 can a message be consumed.
 */

#include <stdint.h>
#include <string.h>
#include <time.h>

#include "../../include/oomaya_rate.h"

/* ── Internal helpers ───────────────────────────────────────────────────── */

/* Full-scale capacity in ×1000 units. */
#define CAPACITY_X1000 \
    ((uint64_t)(OOMAYA_RATE_CAPACITY_MSGS) * UINT64_C(1000))

/* One consumable token in ×1000 units. */
#define ONE_TOKEN_X1000  UINT64_C(1000)

/**
 * refill_bucket() — Compute and apply token accumulation since last refill.
 *
 * Called lazily on every oomaya_rate_check() — no background timer needed.
 * Clamps accumulated tokens at CAPACITY_X1000 to model burst capacity.
 */
static void
refill_bucket(oomaya_rate_bucket_t *b, uint64_t now_ns)
{
    uint64_t elapsed_ns;
    uint64_t new_tokens_x1000;

    /* Guard against clock non-monotonicity (e.g. vDSO quirks). */
    if (now_ns <= b->last_refill_ns)
        return;

    elapsed_ns = now_ns - b->last_refill_ns;

    /*
     * new_tokens_x1000 = (elapsed_ns × 1000) / NS_PER_TOKEN
     *
     * Integer division truncates sub-token remainders; those remainders
     * are implicitly absorbed into the next refill cycle because we only
     * advance last_refill_ns by the consumed quantum, not by elapsed_ns.
     *
     * Overflow analysis:
     *   elapsed_ns max ≈ 1 second = 1e9 ns
     *   1e9 × 1000 = 1e12  < UINT64_MAX (1.8e19) — safe.
     */
    new_tokens_x1000 = (elapsed_ns * UINT64_C(1000)) / OOMAYA_RATE_NS_PER_TOKEN;

    if (new_tokens_x1000 == 0)
        return; /* No full sub-token accumulated yet — skip timestamp update */

    if (b->tokens_x1000 + new_tokens_x1000 >= CAPACITY_X1000) {
        b->tokens_x1000 = CAPACITY_X1000;
        b->last_refill_ns = now_ns;
    } else {
        b->tokens_x1000 += new_tokens_x1000;
        b->last_refill_ns += new_tokens_x1000 * OOMAYA_RATE_NS_PER_TOKEN
                             / UINT64_C(1000);
    }
}

/* ── Public API ─────────────────────────────────────────────────────────── */

uint64_t
oomaya_rate_now_ns(void)
{
    struct timespec ts;
    /* CLOCK_MONOTONIC never goes backwards and is unaffected by NTP jumps. */
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

void
oomaya_rate_pool_init(oomaya_rate_pool_t *pool)
{
    int i;
    if (pool == NULL)
        return;
    for (i = 0; i < OOMAYA_RATE_MAX_CLIENTS; ++i) {
        pool->buckets[i].client_fd      = -1;
        pool->buckets[i].tokens_x1000   = 0;
        pool->buckets[i].last_refill_ns = 0;
    }
    pool->count = 0;
}

int
oomaya_rate_client_add(oomaya_rate_pool_t *pool, int client_fd)
{
    int i;
    if (pool == NULL || client_fd < 0)
        return -1;

    for (i = 0; i < OOMAYA_RATE_MAX_CLIENTS; ++i) {
        if (pool->buckets[i].client_fd == -1) {
            pool->buckets[i].client_fd      = client_fd;
            /* Start at full capacity — new client gets full burst budget. */
            pool->buckets[i].tokens_x1000   = CAPACITY_X1000;
            pool->buckets[i].last_refill_ns = oomaya_rate_now_ns();
            ++pool->count;
            return i;
        }
    }
    return -1; /* pool full */
}

void
oomaya_rate_client_remove(oomaya_rate_pool_t *pool, int slot)
{
    if (pool == NULL || slot < 0 || slot >= OOMAYA_RATE_MAX_CLIENTS)
        return;
    if (pool->buckets[slot].client_fd == -1)
        return; /* already free */

    pool->buckets[slot].client_fd      = -1;
    pool->buckets[slot].tokens_x1000   = 0;
    pool->buckets[slot].last_refill_ns = 0;
    if (pool->count > 0)
        --pool->count;
}

int
oomaya_rate_check(oomaya_rate_pool_t *pool, int slot)
{
    oomaya_rate_bucket_t *b;
    uint64_t              now;

    if (pool == NULL || slot < 0 || slot >= OOMAYA_RATE_MAX_CLIENTS)
        return 0; /* deny on bad input — fail closed */

    b = &pool->buckets[slot];
    if (b->client_fd == -1)
        return 0; /* inactive slot — deny */

    now = oomaya_rate_now_ns();
    refill_bucket(b, now);

    if (b->tokens_x1000 >= ONE_TOKEN_X1000) {
        b->tokens_x1000 -= ONE_TOKEN_X1000;
        return 1; /* allow */
    }
    return 0; /* deny — bucket empty */
}
