/* state_cache.c — Atomic WM state cache implementation.
 *
 * Phase 3 Core. C11. Zero dynamic allocations.
 * Read path: relaxed loads — no fence, minimal latency.
 * Write path: release stores — establishes happens-before for consumers
 *             that subsequently load with acquire.
 */
#include <stdint.h>
#include <stdatomic.h>
#include <string.h>
#include "../../include/oomaya_state.h"

void
oomaya_state_init(oomaya_wm_state_t *s)
{
    uint32_t i;
    if (s == NULL) return;
    atomic_store_explicit(&s->tag_mask,      0u, memory_order_relaxed);
    atomic_store_explicit(&s->layout_idx,    0u, memory_order_relaxed);
    atomic_store_explicit(&s->focused_win,   0u, memory_order_relaxed);
    atomic_store_explicit(&s->monitor_count, 1u, memory_order_relaxed);
    for (i = 0; i < OOMAYA_STATE_MAX_MONITORS; ++i)
        atomic_store_explicit(&s->monitor_tags[i], 0u, memory_order_relaxed);
}

uint32_t oomaya_state_get_tag_mask(const oomaya_wm_state_t *s)
{ return atomic_load_explicit(&s->tag_mask, memory_order_relaxed); }

void oomaya_state_set_tag_mask(oomaya_wm_state_t *s, uint32_t mask)
{ atomic_store_explicit(&s->tag_mask, mask, memory_order_release); }

uint32_t oomaya_state_get_layout(const oomaya_wm_state_t *s)
{ return atomic_load_explicit(&s->layout_idx, memory_order_relaxed); }

void oomaya_state_set_layout(oomaya_wm_state_t *s, uint32_t idx)
{ atomic_store_explicit(&s->layout_idx, idx, memory_order_release); }

uint32_t oomaya_state_get_focused(const oomaya_wm_state_t *s)
{ return atomic_load_explicit(&s->focused_win, memory_order_relaxed); }

void oomaya_state_set_focused(oomaya_wm_state_t *s, uint32_t xid)
{ atomic_store_explicit(&s->focused_win, xid, memory_order_release); }

uint32_t oomaya_state_get_monitor_count(const oomaya_wm_state_t *s)
{ return atomic_load_explicit(&s->monitor_count, memory_order_relaxed); }

void oomaya_state_set_monitor_count(oomaya_wm_state_t *s, uint32_t n)
{ atomic_store_explicit(&s->monitor_count, n, memory_order_release); }

uint32_t
oomaya_state_get_monitor_tag(const oomaya_wm_state_t *s, uint32_t mon)
{
    if (mon >= OOMAYA_STATE_MAX_MONITORS) return 0u;
    return atomic_load_explicit(&s->monitor_tags[mon], memory_order_relaxed);
}

void
oomaya_state_set_monitor_tag(oomaya_wm_state_t *s, uint32_t mon, uint32_t mask)
{
    if (mon >= OOMAYA_STATE_MAX_MONITORS) return;
    atomic_store_explicit(&s->monitor_tags[mon], mask, memory_order_release);
}
