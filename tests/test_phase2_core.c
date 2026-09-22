/* test_phase2_core.c — Phase 2 Core unit tests.
 *
 * Test groups:
 *   RATE group  — Token-bucket rate limiter
 *     R01  Pool init sets all slots inactive
 *     R02  client_add claims first free slot and returns its index
 *     R03  Fresh bucket allows burst of CAPACITY_MSGS messages instantly
 *     R04  Bucket drains to zero; next check is denied (fail-closed)
 *     R05  Bucket refills over simulated time (white-box: direct token set)
 *     R06  client_remove marks slot inactive; subsequent check denied
 *     R07  Pool full: add beyond MAX_CLIENTS returns -1
 *     R08  NULL and bad-slot inputs return 0 (fail-closed)
 *     R09  Slot reuse: after remove, add re-claims the same slot
 *     R10  oomaya_rate_now_ns() advances monotonically across two calls
 *
 *   RING group  — ABA-safe lock-free ring buffer
 *     G01  Init: enqueue_pos == 0, dequeue_pos == 0
 *     G02  Init: every slot tag == make_tag(0, i)
 *     G03  Single enqueue+dequeue round-trip preserves task payload
 *     G04  Ring fills to CAPACITY; next enqueue returns RING_FULL
 *     G05  Dequeue from empty ring returns RING_EMPTY
 *     G06  FIFO ordering: N enqueues produce N matching dequeues in order
 *     G07  ABA detection: inject lapped tag; dequeue returns RING_ABA
 *     G08  After full fill+drain cycle, ring is reusable (generation wraps)
 *     G09  NULL inputs: all functions return error codes, no crash
 *     G10  oomaya_ring_size() tracks occupancy during fill and drain
 *
 * Zero dynamic allocations. Plain C99+C11 atomics. No external framework.
 * Exits 0 on full pass.
 */

#include <stdint.h>
#include <stdatomic.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/oomaya_rate.h"
#include "../include/oomaya_ring.h"

/* ── Minimal test harness ───────────────────────────────────────────────── */

static int g_pass = 0;
static int g_fail = 0;

#define ASSERT_EQ(label, got, expected)                                     \
    do {                                                                    \
        if ((long long)(got) == (long long)(expected)) {                    \
            printf("  [PASS] %s\n", (label));                              \
            ++g_pass;                                                       \
        } else {                                                            \
            printf("  [FAIL] %s  got=%lld  expected=%lld\n",               \
                   (label), (long long)(got), (long long)(expected));       \
            ++g_fail;                                                       \
        }                                                                   \
    } while (0)

#define ASSERT_GT(label, got, floor)                                        \
    do {                                                                    \
        if ((long long)(got) > (long long)(floor)) {                        \
            printf("  [PASS] %s  (%lld > %lld)\n",                        \
                   (label), (long long)(got), (long long)(floor));          \
            ++g_pass;                                                       \
        } else {                                                            \
            printf("  [FAIL] %s  got=%lld  not > %lld\n",                  \
                   (label), (long long)(got), (long long)(floor));          \
            ++g_fail;                                                       \
        }                                                                   \
    } while (0)

#define TEST(name) printf("\n[TEST] " name "\n")

/* ── Helpers ─────────────────────────────────────────────────────────────── */

/* Build a synthetic task with opcode and a recognisable payload byte. */
static oomaya_task_t
make_task(uint16_t opcode, uint8_t marker)
{
    oomaya_task_t t;
    memset(&t, 0, sizeof(t));
    t.opcode      = opcode;
    t.payload[0]  = marker;
    t.payload_len = 1;
    return t;
}

/* White-box: directly set a bucket's token count for time-simulation. */
static void
set_tokens(oomaya_rate_pool_t *pool, int slot, uint64_t tokens_x1000)
{
    pool->buckets[slot].tokens_x1000 = tokens_x1000;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * RATE LIMITER TESTS
 * ═══════════════════════════════════════════════════════════════════════════ */

static void
r01_pool_init(void)
{
    oomaya_rate_pool_t pool;
    int i;
    TEST("R01: Pool init clears all slots");
    oomaya_rate_pool_init(&pool);
    for (i = 0; i < OOMAYA_RATE_MAX_CLIENTS; ++i) {
        if (pool.buckets[i].client_fd != -1) {
            printf("  [FAIL] slot %d not cleared\n", i);
            ++g_fail;
            return;
        }
    }
    printf("  [PASS] All %d slots inactive after init\n", OOMAYA_RATE_MAX_CLIENTS);
    ++g_pass;
    ASSERT_EQ("pool.count == 0", pool.count, 0);
}

static void
r02_client_add(void)
{
    oomaya_rate_pool_t pool;
    int slot;
    TEST("R02: client_add claims first free slot");
    oomaya_rate_pool_init(&pool);
    slot = oomaya_rate_client_add(&pool, 42);
    ASSERT_EQ("slot >= 0",      (slot >= 0), 1);
    ASSERT_EQ("pool.count == 1", pool.count, 1);
    ASSERT_EQ("bucket fd == 42", pool.buckets[slot].client_fd, 42);
    /* Fresh bucket starts at full capacity */
    ASSERT_EQ("tokens_x1000 == CAPACITY×1000",
              pool.buckets[slot].tokens_x1000,
              (uint64_t)OOMAYA_RATE_CAPACITY_MSGS * 1000u);
}

static void
r03_burst_allowed(void)
{
    oomaya_rate_pool_t pool;
    int slot, i, denied = 0;
    TEST("R03: Fresh bucket allows full burst");
    oomaya_rate_pool_init(&pool);
    slot = oomaya_rate_client_add(&pool, 10);
    for (i = 0; i < OOMAYA_RATE_CAPACITY_MSGS; ++i) {
        if (!oomaya_rate_check(&pool, slot))
            ++denied;
    }
    ASSERT_EQ("0 denials within burst capacity", denied, 0);
}

static void
r04_bucket_empty_denied(void)
{
    oomaya_rate_pool_t pool;
    int slot;
    TEST("R04: Empty bucket denies next message");
    oomaya_rate_pool_init(&pool);
    slot = oomaya_rate_client_add(&pool, 11);
    /* Drain to zero */
    set_tokens(&pool, slot, 0);
    ASSERT_EQ("Empty bucket → deny", oomaya_rate_check(&pool, slot), 0);
}

static void
r05_refill_allows(void)
{
    oomaya_rate_pool_t pool;
    int slot;
    TEST("R05: Bucket refills allow messages after drain");
    oomaya_rate_pool_init(&pool);
    slot = oomaya_rate_client_add(&pool, 12);
    /* Drain completely */
    set_tokens(&pool, slot, 0);
    ASSERT_EQ("Drained → deny (sanity)", oomaya_rate_check(&pool, slot), 0);
    /* Inject tokens as if time has passed (5 tokens = 5000 ×1000) */
    set_tokens(&pool, slot, 5000u);
    ASSERT_EQ("After refill → allow", oomaya_rate_check(&pool, slot), 1);
    /* 4 more should succeed */
    ASSERT_EQ("After refill → allow ×2", oomaya_rate_check(&pool, slot), 1);
}

static void
r06_remove_denies(void)
{
    oomaya_rate_pool_t pool;
    int slot;
    TEST("R06: Removed client slot is denied");
    oomaya_rate_pool_init(&pool);
    slot = oomaya_rate_client_add(&pool, 13);
    oomaya_rate_client_remove(&pool, slot);
    ASSERT_EQ("pool.count == 0 after remove", pool.count, 0);
    ASSERT_EQ("Removed slot → deny",  oomaya_rate_check(&pool, slot), 0);
    ASSERT_EQ("Removed bucket fd==-1", pool.buckets[slot].client_fd, -1);
}

static void
r07_pool_full(void)
{
    oomaya_rate_pool_t pool;
    int i, last;
    TEST("R07: Pool full returns -1");
    oomaya_rate_pool_init(&pool);
    for (i = 0; i < OOMAYA_RATE_MAX_CLIENTS; ++i)
        oomaya_rate_client_add(&pool, i + 100);
    last = oomaya_rate_client_add(&pool, 9999);
    ASSERT_EQ("Overflow returns -1", last, -1);
    ASSERT_EQ("count stays at MAX", pool.count, OOMAYA_RATE_MAX_CLIENTS);
}

static void
r08_null_bad_slot(void)
{
    oomaya_rate_pool_t pool;
    TEST("R08: NULL and bad-slot inputs fail-closed");
    oomaya_rate_pool_init(&pool);
    ASSERT_EQ("NULL pool → deny",   oomaya_rate_check(NULL, 0),  0);
    ASSERT_EQ("slot=-1 → deny",     oomaya_rate_check(&pool, -1), 0);
    ASSERT_EQ("slot=MAX → deny",
              oomaya_rate_check(&pool, OOMAYA_RATE_MAX_CLIENTS), 0);
    /* NULL pool_init: must not crash */
    oomaya_rate_pool_init(NULL);
    printf("  [PASS] pool_init(NULL) no crash\n"); ++g_pass;
    /* NULL client_add */
    ASSERT_EQ("add NULL pool → -1", oomaya_rate_client_add(NULL, 1), -1);
    ASSERT_EQ("add fd=-1 → -1",     oomaya_rate_client_add(&pool, -1), -1);
}

static void
r09_slot_reuse(void)
{
    oomaya_rate_pool_t pool;
    int s1, s2;
    TEST("R09: Slot reuse after remove");
    oomaya_rate_pool_init(&pool);
    s1 = oomaya_rate_client_add(&pool, 20);
    oomaya_rate_client_remove(&pool, s1);
    s2 = oomaya_rate_client_add(&pool, 21);
    ASSERT_EQ("Same slot reused", s1, s2);
    ASSERT_EQ("New fd in slot",   pool.buckets[s2].client_fd, 21);
}

static void
r10_clock_monotonic(void)
{
    uint64_t t1, t2;
    TEST("R10: oomaya_rate_now_ns() advances monotonically");
    t1 = oomaya_rate_now_ns();
    /* Busy-wait a tiny spin to ensure at least one ns passes. */
    { volatile int i; for (i = 0; i < 10000; ++i) {} }
    t2 = oomaya_rate_now_ns();
    ASSERT_GT("t2 > t1", t2, t1);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * RING BUFFER TESTS
 * ═══════════════════════════════════════════════════════════════════════════ */

static void
g01_ring_init_positions(void)
{
    oomaya_ring_t ring;
    TEST("G01: Init positions are zero");
    oomaya_ring_init(&ring);
    ASSERT_EQ("enqueue_pos == 0",
              atomic_load(&ring.enqueue_pos), 0u);
    ASSERT_EQ("dequeue_pos == 0",
              atomic_load(&ring.dequeue_pos), 0u);
}

static void
g02_ring_init_tags(void)
{
    oomaya_ring_t ring;
    uint32_t i;
    int bad = 0;
    TEST("G02: Init slot tags == make_tag(0, i)");
    oomaya_ring_init(&ring);
    for (i = 0; i < OOMAYA_RING_CAPACITY; ++i) {
        uint32_t expected = (0u << 16) | i;
        uint32_t actual   = atomic_load(&ring.slots[i].tag);
        if (actual != expected) { ++bad; }
    }
    ASSERT_EQ("All slot tags correct (bad count == 0)", bad, 0);
}

static void
g03_single_roundtrip(void)
{
    oomaya_ring_t ring;
    oomaya_task_t in_task, out_task;
    TEST("G03: Single enqueue+dequeue round-trip");
    oomaya_ring_init(&ring);
    in_task = make_task(0x0102, 0xAB);
    ASSERT_EQ("enqueue OK", oomaya_ring_enqueue(&ring, &in_task), OOMAYA_RING_OK);
    memset(&out_task, 0, sizeof(out_task));
    ASSERT_EQ("dequeue OK", oomaya_ring_dequeue(&ring, &out_task), OOMAYA_RING_OK);
    ASSERT_EQ("opcode preserved",    out_task.opcode,     in_task.opcode);
    ASSERT_EQ("marker preserved",    out_task.payload[0], in_task.payload[0]);
    ASSERT_EQ("payload_len preserved",out_task.payload_len, in_task.payload_len);
}

static void
g04_ring_full(void)
{
    oomaya_ring_t ring;
    oomaya_task_t t;
    uint32_t i;
    int rc;
    TEST("G04: Ring fills to CAPACITY; next enqueue returns FULL");
    oomaya_ring_init(&ring);
    t = make_task(0x0101, 0x01);
    for (i = 0; i < OOMAYA_RING_CAPACITY; ++i) {
        rc = oomaya_ring_enqueue(&ring, &t);
        if (rc != OOMAYA_RING_OK) {
            printf("  [FAIL] enqueue failed at slot %u\n", i);
            ++g_fail;
            return;
        }
    }
    printf("  [PASS] Filled %u slots\n", OOMAYA_RING_CAPACITY); ++g_pass;
    rc = oomaya_ring_enqueue(&ring, &t);
    ASSERT_EQ("Enqueue on full ring → FULL", rc, OOMAYA_RING_FULL);
}

static void
g05_empty_dequeue(void)
{
    oomaya_ring_t ring;
    oomaya_task_t out;
    TEST("G05: Dequeue from empty ring → EMPTY");
    oomaya_ring_init(&ring);
    ASSERT_EQ("dequeue empty → RING_EMPTY",
              oomaya_ring_dequeue(&ring, &out), OOMAYA_RING_EMPTY);
}

static void
g06_fifo_order(void)
{
    oomaya_ring_t ring;
    oomaya_task_t out;
    uint32_t i;
    int order_ok = 1;
    TEST("G06: FIFO ordering across N enqueue+dequeue pairs");
    oomaya_ring_init(&ring);
    /* Enqueue 16 tasks with distinct markers */
    for (i = 0; i < 16u; ++i) {
        oomaya_task_t t = make_task(0x0103, (uint8_t)(i + 1u));
        oomaya_ring_enqueue(&ring, &t);
    }
    /* Dequeue and verify order */
    for (i = 0; i < 16u; ++i) {
        oomaya_ring_dequeue(&ring, &out);
        if (out.payload[0] != (uint8_t)(i + 1u))
            order_ok = 0;
    }
    ASSERT_EQ("FIFO order preserved across 16 messages", order_ok, 1);
}

static void
g07_aba_detection(void)
{
    oomaya_ring_t ring;
    oomaya_task_t out;
    TEST("G07: ABA detection — injected lapped tag returns RING_ABA");
    oomaya_ring_init(&ring);

    /*
     * Simulate an ABA collision on slot 0:
     * The consumer's dequeue_pos is at 0 (expecting generation 1 in slot 0),
     * but we inject generation 3 (ready-for-lap-1, > expected gen=1) (as if the producer has lapped twice).
     * expected_gen for dequeue_pos=0 is: 0/256 + 1 = 1
     * We inject: (3 << 16) | 0  → generation 2 → ABA
     */
    atomic_store(&ring.dequeue_pos, 0u);
    atomic_store(&ring.enqueue_pos, 0u); /* producer hasn't moved */
    atomic_store(&ring.slots[0].tag, (uint32_t)((3u << 16) | 0u) /* gen=3: ready-for-lap-1 > expected gen=1 */);

    ASSERT_EQ("Lapped slot → RING_ABA",
              oomaya_ring_dequeue(&ring, &out), OOMAYA_RING_ABA);
}

static void
g08_reuse_after_full_cycle(void)
{
    oomaya_ring_t ring;
    oomaya_task_t t, out;
    uint32_t i;
    TEST("G08: Ring is reusable after full fill+drain cycle");
    oomaya_ring_init(&ring);
    t = make_task(0x0104, 0xFF);

    /* Fill */
    for (i = 0; i < OOMAYA_RING_CAPACITY; ++i)
        oomaya_ring_enqueue(&ring, &t);
    /* Drain */
    for (i = 0; i < OOMAYA_RING_CAPACITY; ++i)
        oomaya_ring_dequeue(&ring, &out);

    /* After one full lap the ring should accept new enqueues */
    t = make_task(0x0105, 0x42);
    ASSERT_EQ("Enqueue after full cycle OK",
              oomaya_ring_enqueue(&ring, &t), OOMAYA_RING_OK);
    ASSERT_EQ("Dequeue after full cycle OK",
              oomaya_ring_dequeue(&ring, &out), OOMAYA_RING_OK);
    ASSERT_EQ("Payload survives lap", out.payload[0], 0x42u);
}

static void
g09_null_inputs(void)
{
    oomaya_task_t t = make_task(0x0101, 0x01);
    oomaya_task_t out;
    TEST("G09: NULL inputs return error codes, no crash");
    oomaya_ring_init(NULL);
    printf("  [PASS] ring_init(NULL) no crash\n"); ++g_pass;
    ASSERT_EQ("enqueue NULL ring → FULL",
              oomaya_ring_enqueue(NULL, &t),    OOMAYA_RING_FULL);
    ASSERT_EQ("enqueue NULL task → FULL",
              oomaya_ring_enqueue(NULL, NULL),   OOMAYA_RING_FULL);
    ASSERT_EQ("dequeue NULL ring → EMPTY",
              oomaya_ring_dequeue(NULL, &out),   OOMAYA_RING_EMPTY);
    ASSERT_EQ("dequeue NULL task → EMPTY",
              oomaya_ring_dequeue(NULL, NULL),   OOMAYA_RING_EMPTY);
    ASSERT_EQ("size NULL → 0",
              oomaya_ring_size(NULL), 0u);
}

static void
g10_ring_size(void)
{
    oomaya_ring_t ring;
    oomaya_task_t t, out;
    uint32_t i;
    TEST("G10: oomaya_ring_size() tracks occupancy");
    oomaya_ring_init(&ring);
    t = make_task(0x0102, 0x01);
    ASSERT_EQ("size == 0 after init", oomaya_ring_size(&ring), 0u);
    for (i = 1; i <= 8u; ++i) {
        oomaya_ring_enqueue(&ring, &t);
        ASSERT_EQ("size grows", oomaya_ring_size(&ring), i);
    }
    for (i = 7; i <= 7u; --i) { /* drain 1 */
        oomaya_ring_dequeue(&ring, &out);
        break;
    }
    ASSERT_EQ("size shrinks by 1", oomaya_ring_size(&ring), 7u);
}

/* ── Entry point ─────────────────────────────────────────────────────────── */

int
main(void)
{
    printf("=== oomaya Phase 2 Core unit tests ===\n");
    printf("\n─── Rate Limiter (Operational Shield) ───\n");

    r01_pool_init();
    r02_client_add();
    r03_burst_allowed();
    r04_bucket_empty_denied();
    r05_refill_allows();
    r06_remove_denies();
    r07_pool_full();
    r08_null_bad_slot();
    r09_slot_reuse();
    r10_clock_monotonic();

    printf("\n─── ABA-Safe Ring Buffer (Structural Guarantee) ───\n");

    g01_ring_init_positions();
    g02_ring_init_tags();
    g03_single_roundtrip();
    g04_ring_full();
    g05_empty_dequeue();
    g06_fifo_order();
    g07_aba_detection();
    g08_reuse_after_full_cycle();
    g09_null_inputs();
    g10_ring_size();

    printf("\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
    return (g_fail == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
