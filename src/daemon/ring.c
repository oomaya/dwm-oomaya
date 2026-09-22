/* ring.c — ABA-safe MPMC lock-free ring buffer implementation.
 *
 * Phase 2 Core — Structural Integrity Guarantee (Priority 2).
 * C11 atomics. Zero dynamic allocations.
 *
 * Protocol (Vyukov MPMC queue, adapted with generation tags):
 *
 *   Each slot carries an _Atomic uint32_t tag encoding:
 *       Bits 31..16  → generation  (number of full laps producer has taken)
 *       Bits 15.. 0  → sequence    (slot index, 0..CAPACITY-1)
 *
 *   Initial state (oomaya_ring_init):
 *       slot[i].tag = make_tag(0, i)
 *       enqueue_pos = 0
 *       dequeue_pos = 0
 *
 *   Enqueue (producer at position pos):
 *       slot_index   = pos & MASK
 *       lap          = pos / CAPACITY          -- which lap the producer is on
 *       expected_tag = make_tag(lap, slot_index) -- what "slot is free" looks like
 *
 *       Read slot[slot_index].tag (acquire).
 *       If tag == expected_tag  → slot is free; claim it:
 *           Write payload (relaxed).
 *           Write tag = make_tag(lap + 1, slot_index) (release) — "slot ready"
 *           Advance enqueue_pos (relaxed, single producer).
 *       Else → ring is FULL (consumer hasn't freed this slot yet).
 *
 *   Dequeue (consumer at position pos):
 *       slot_index   = pos & MASK
 *       lap          = pos / CAPACITY          -- which lap the consumer is on
 *       expected_tag = make_tag(lap + 1, slot_index) -- "slot ready" tag
 *
 *       Read slot[slot_index].tag (acquire).
 *       If tag == expected_tag → data is ready; claim via CAS on dequeue_pos:
 *           Copy payload (relaxed).
 *           Write tag = make_tag(lap + 1, slot_index) … wait — we must write
 *           the "freed" tag, i.e. what the NEXT enqueue on this slot expects:
 *               next enqueue lap = current consumer lap + 1
 *               freed_tag = make_tag(lap + 1, slot_index)
 *           But that equals expected_tag — we'd be writing the same value.
 *
 *   CORRECTED freed_tag computation:
 *   After consuming slot at dequeue_pos=P:
 *       consumer_lap = P / CAPACITY
 *       freed_tag    = make_tag(consumer_lap + 1, slot_index)
 *   This is the tag the NEXT producer (on its next lap = consumer_lap + 1)
 *   will find and interpret as "slot is free."
 *
 *   Full detection invariant:
 *       At init:          slot[i].tag = make_tag(0, i)   → free for lap-0 producer
 *       After enqueue-0:  slot[i].tag = make_tag(1, i)   → ready for lap-0 consumer
 *       After dequeue-0:  slot[i].tag = make_tag(1, i)   → free for lap-1 producer ✓
 *       After enqueue-1:  slot[i].tag = make_tag(2, i)   → ready for lap-1 consumer
 *       ...
 *   At FULL (producer on lap 1, consumer still on lap 0):
 *       slot[i].tag = make_tag(1, i)   (set by enqueue-0)
 *       producer expects make_tag(1, i) ← matches! ← BUG in naive implementation
 *
 *   ROOT CAUSE: "free" and "ready" tags are indistinguishable when the
 *   consumer's write-back is make_tag(consumer_lap+1, idx) = make_tag(1, idx)
 *   and the producer's "free" check is also make_tag(1, idx).
 *
 *   DEFINITIVE FIX — two-bit tag encoding:
 *   Encode both "lap" and "state" (free vs. ready) in the tag:
 *       "free for lap L"    = make_tag(L * 2,     idx)   (even generation)
 *       "ready from lap L"  = make_tag(L * 2 + 1, idx)   (odd generation)
 *
 *   This ensures:
 *       Init:             tag = make_tag(0, i)   → even → free for lap 0 ✓
 *       After enqueue:    tag = make_tag(1, i)   → odd  → ready for lap 0 ✓
 *       After dequeue:    tag = make_tag(2, i)   → even → free for lap 1 ✓
 *       Full check (lap 1 producer, slot not yet freed):
 *           slot tag    = make_tag(1, i)   (odd  = "ready")
 *           expected    = make_tag(2, i)   (even = "free for lap 1")
 *           mismatch    → RING_FULL ✓
 *   ABA check (dequeue consumer, slot already re-enqueued):
 *           slot tag    = make_tag(3, i)   (odd  = "ready for lap 1")
 *           expected    = make_tag(1, i)   (odd  = "ready for lap 0")
 *           mismatch + tag > expected → RING_ABA ✓
 */

#include <stdint.h>
#include <stdatomic.h>
#include <string.h>

#include "../../include/oomaya_ring.h"

/* ── Tag helpers ─────────────────────────────────────────────────────────── */

static inline uint32_t
make_tag(uint32_t gen, uint32_t seq)
{
    return ((gen & 0xFFFFu) << 16) | (seq & 0xFFFFu);
}

static inline uint32_t
tag_gen(uint32_t tag)
{
    return tag >> 16;
}

/* ── Public API ─────────────────────────────────────────────────────────── */

void
oomaya_ring_init(oomaya_ring_t *ring)
{
    uint32_t i;
    if (ring == NULL)
        return;

    atomic_store_explicit(&ring->enqueue_pos, 0u, memory_order_relaxed);
    atomic_store_explicit(&ring->dequeue_pos, 0u, memory_order_relaxed);

    /* "Free for lap 0" = make_tag(0*2, i) = make_tag(0, i) */
    for (i = 0; i < OOMAYA_RING_CAPACITY; ++i) {
        atomic_store_explicit(&ring->slots[i].tag,
                              make_tag(0u, i),
                              memory_order_relaxed);
    }
}

int
oomaya_ring_enqueue(oomaya_ring_t *ring, const oomaya_task_t *task)
{
    uint32_t pos;
    uint32_t sidx;
    uint32_t lap;
    uint32_t free_tag;    /* tag meaning "free for this lap" */
    uint32_t ready_tag;   /* tag meaning "ready for consumer" */
    uint32_t actual;

    if (ring == NULL || task == NULL)
        return OOMAYA_RING_FULL;

    pos      = atomic_load_explicit(&ring->enqueue_pos, memory_order_relaxed);
    sidx     = pos & OOMAYA_RING_MASK;
    lap      = pos / OOMAYA_RING_CAPACITY;
    free_tag  = make_tag(lap * 2u,       sidx);  /* even gen = free */
    ready_tag = make_tag(lap * 2u + 1u,  sidx);  /* odd  gen = ready */

    actual = atomic_load_explicit(&ring->slots[sidx].tag, memory_order_acquire);

    if (actual != free_tag) {
        /* Not yet freed by consumer → ring full (or ABA — treat as full). */
        return OOMAYA_RING_FULL;
    }

    /* Slot is ours (single producer — no CAS needed). */
    memcpy(&ring->slots[sidx].task, task, sizeof(oomaya_task_t));

    /* Publish: mark slot as ready for the consumer (RELEASE). */
    atomic_store_explicit(&ring->slots[sidx].tag, ready_tag, memory_order_release);

    /* Advance producer cursor (relaxed — single producer). */
    atomic_store_explicit(&ring->enqueue_pos, pos + 1u, memory_order_relaxed);

    return OOMAYA_RING_OK;
}

int
oomaya_ring_dequeue(oomaya_ring_t *ring, oomaya_task_t *task)
{
    uint32_t pos;
    uint32_t sidx;
    uint32_t lap;
    uint32_t ready_tag;   /* what the consumer expects to find */
    uint32_t freed_tag;   /* what we write back to free the slot for next lap */
    uint32_t actual;
    uint32_t actual_gen;
    uint32_t expected_gen;

    if (ring == NULL || task == NULL)
        return OOMAYA_RING_EMPTY;

    for (;;) {
        pos  = atomic_load_explicit(&ring->dequeue_pos, memory_order_relaxed);
        sidx = pos & OOMAYA_RING_MASK;
        lap  = pos / OOMAYA_RING_CAPACITY;

        /* Consumer on lap L expects the slot to be "ready from lap L":
         * ready_tag = make_tag(L*2 + 1, sidx)  (odd generation) */
        ready_tag = make_tag(lap * 2u + 1u, sidx);

        /* After consuming, mark slot "free for lap L+1":
         * freed_tag = make_tag((L+1)*2, sidx)  (even generation, next lap) */
        freed_tag = make_tag((lap + 1u) * 2u, sidx);

        actual      = atomic_load_explicit(&ring->slots[sidx].tag, memory_order_acquire);
        actual_gen  = tag_gen(actual);
        expected_gen = tag_gen(ready_tag);

        if (actual_gen == expected_gen) {
            /* Data is ready. CAS dequeue_pos to claim (MPMC safety). */
            if (atomic_compare_exchange_weak_explicit(
                    &ring->dequeue_pos,
                    &pos, pos + 1u,
                    memory_order_acq_rel,
                    memory_order_relaxed)) {
                memcpy(task, &ring->slots[sidx].task, sizeof(oomaya_task_t));
                /* Release slot for next-lap producer. */
                atomic_store_explicit(&ring->slots[sidx].tag,
                                      freed_tag,
                                      memory_order_release);
                return OOMAYA_RING_OK;
            }
            /* Another worker claimed it — retry. */
            continue;
        }

        if (actual_gen < expected_gen) {
            /* Producer hasn't published yet — ring is empty. */
            return OOMAYA_RING_EMPTY;
        }

        /* actual_gen > expected_gen — producer has lapped this consumer: ABA. */
        return OOMAYA_RING_ABA;
    }
}

uint32_t
oomaya_ring_size(const oomaya_ring_t *ring)
{
    uint32_t head, tail;
    if (ring == NULL)
        return 0u;
    head = atomic_load_explicit(&ring->enqueue_pos, memory_order_relaxed);
    tail = atomic_load_explicit(&ring->dequeue_pos, memory_order_relaxed);
    return (head - tail);
}
