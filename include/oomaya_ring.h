/* oomaya_ring.h — ABA-safe SPSC/MPMC lock-free ring buffer.
 *
 * Phase 2 Core — Structural Integrity Guarantee (Priority 2).
 *
 * ABA Problem Statement:
 *   A naive ring uses a bare uint32_t head/tail index that wraps at 2³².
 *   With 256 slots, a producer can lap a slow consumer in exactly 256
 *   enqueue operations.  On wrap, the producer overwrites slot[0] while
 *   the consumer still holds a stale pointer to slot[0] from the previous
 *   lap — an ABA collision.  The consumer reads corrupted task data
 *   silently, with no assertion or crash to signal the fault.
 *
 * Solution — Generation-Tagged Slots:
 *   Each ring slot carries an _Atomic uint32_t tag.
 *   The tag encodes:
 *       high 16 bits → generation number (lap count)
 *       low  16 bits → slot sequence index (position in ring)
 *
 *   On enqueue: producer writes its generation+1 into the slot tag after
 *               writing task data (release store).
 *   On dequeue: consumer reads the slot tag (acquire load) and compares
 *               it to the expected generation.  A mismatch means the slot
 *               has been lapped — consumer retries or reports overflow.
 *
 *   This makes lapping detectable at slot granularity without any mutex.
 *
 * Concurrency model:
 *   SPSC (single-producer single-consumer): reactor thread enqueues,
 *   one worker dequeues.  Trivially extends to MPMC with a CAS on
 *   dequeue_pos, which ring.c implements.
 *
 * Memory model:
 *   All _Atomic accesses use explicit memory orders:
 *     enqueue: relaxed payload write → release tag store
 *     dequeue: acquire tag load      → relaxed payload read → release pos advance
 *
 * Zero dynamic allocations. The ring struct is stack- or BSS-allocatable.
 */

#ifndef OOMAYA_RING_H
#define OOMAYA_RING_H

#include <stdint.h>
#include <stdatomic.h>

/* ── Ring geometry ──────────────────────────────────────────────────────── */

/* Must be a power of two. Wrap via bitwise AND, not modulo. */
#ifndef OOMAYA_RING_CAPACITY
#define OOMAYA_RING_CAPACITY   256u
#endif

#define OOMAYA_RING_MASK  (OOMAYA_RING_CAPACITY - 1u)

/* Compile-time power-of-two check. */
typedef char _oomaya_ring_pow2_check[
    ((OOMAYA_RING_CAPACITY & (OOMAYA_RING_CAPACITY - 1u)) == 0u) ? 1 : -1
];

/* ── Task descriptor (same struct as spec §3.1) ─────────────────────────── */
#ifndef OOMAYA_TASK_DEFINED
#define OOMAYA_TASK_DEFINED
typedef struct {
    uint32_t client_fd;
    uint16_t msg_id;
    uint16_t target_ver;
    uint16_t opcode;
    uint32_t payload_len;
    uint8_t  payload[512];
    /* Result container written by worker */
    int32_t  status;
    uint32_t resp_len;
    uint8_t  resp_data[512];
} oomaya_task_t;
#endif /* OOMAYA_TASK_DEFINED */

/* ── Generation-tagged slot ─────────────────────────────────────────────── */
typedef struct {
    _Atomic uint32_t tag;  /* high16=generation, low16=sequence */
    oomaya_task_t    task; /* payload — written before tag is published */
} oomaya_ring_slot_t;

/* ── Ring descriptor ─────────────────────────────────────────────────────
 *
 * Pad enqueue_pos and dequeue_pos onto separate cache lines (64 B apart)
 * to eliminate false sharing between the reactor (producer) and workers
 * (consumers) on multi-core hardware.
 */
typedef struct {
    _Atomic uint32_t    enqueue_pos;          /* producer writes here */
    uint8_t             _pad0[60];            /* cache-line pad */
    _Atomic uint32_t    dequeue_pos;          /* consumers CAS here */
    uint8_t             _pad1[60];            /* cache-line pad */
    oomaya_ring_slot_t  slots[OOMAYA_RING_CAPACITY];
} oomaya_ring_t;

/* ── Return codes ────────────────────────────────────────────────────────── */
typedef enum {
    OOMAYA_RING_OK       =  0,
    OOMAYA_RING_FULL     = -1,  /* ring at capacity — producer must back off */
    OOMAYA_RING_EMPTY    = -2,  /* no work available — consumer should wait */
    OOMAYA_RING_ABA      = -3   /* ABA collision detected — slot lapped */
} oomaya_ring_err_t;

/* ── API ─────────────────────────────────────────────────────────────────── */

/**
 * oomaya_ring_init() — Initialise all slot tags to their sequence position.
 *
 * Each slot tag is pre-loaded with its index so that the generation check
 * on the first enqueue of each slot succeeds immediately (generation 0
 * expects tag == slot_index).
 */
void oomaya_ring_init(oomaya_ring_t *ring);

/**
 * oomaya_ring_enqueue() — Attempt to enqueue one task (producer / SPSC).
 *
 * Non-blocking. Returns OOMAYA_RING_FULL if no slot is available.
 * The caller (reactor) must handle backpressure — e.g. send ERR to client.
 *
 * @ring  The ring.
 * @task  Task to copy into the ring slot (shallow copy of struct).
 * @return OOMAYA_RING_OK or OOMAYA_RING_FULL.
 */
int oomaya_ring_enqueue(oomaya_ring_t *ring, const oomaya_task_t *task);

/**
 * oomaya_ring_dequeue() — Attempt to dequeue one task (consumer / MPMC-safe).
 *
 * Non-blocking. Returns OOMAYA_RING_EMPTY if no work is available.
 * ABA detection: returns OOMAYA_RING_ABA if a slot generation mismatch is
 * detected, indicating that the producer has lapped this consumer.
 *
 * @ring  The ring.
 * @task  Output: populated with the dequeued task on success.
 * @return OOMAYA_RING_OK, OOMAYA_RING_EMPTY, or OOMAYA_RING_ABA.
 */
int oomaya_ring_dequeue(oomaya_ring_t *ring, oomaya_task_t *task);

/**
 * oomaya_ring_size() — Approximate occupancy (not linearisable under MPMC).
 * Use only for diagnostics / metrics, never for correctness decisions.
 */
uint32_t oomaya_ring_size(const oomaya_ring_t *ring);

#endif /* OOMAYA_RING_H */
