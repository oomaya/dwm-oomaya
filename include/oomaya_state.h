/* oomaya_state.h — Atomic WM state cache.
 *
 * Phase 3 Core. C11 _Atomic. Zero allocations.
 * All reads: memory_order_relaxed  (hot path; stale-by-one-frame is fine).
 * All writes: memory_order_release (after X11 mutation commits).
 *
 * Workers access this struct read-only. Only the reactor/main thread writes.
 */
#ifndef OOMAYA_STATE_H
#define OOMAYA_STATE_H

#include <stdint.h>
#include <stdatomic.h>

#define OOMAYA_STATE_MAX_MONITORS 4u

typedef struct {
    _Atomic uint32_t tag_mask;                              /* active tag bitmask */
    _Atomic uint32_t layout_idx;                            /* layout enum index  */
    _Atomic uint32_t focused_win;                           /* XID focused client */
    _Atomic uint32_t monitor_count;                         /* active monitors    */
    _Atomic uint32_t monitor_tags[OOMAYA_STATE_MAX_MONITORS]; /* per-monitor tags */
} oomaya_wm_state_t;

void     oomaya_state_init(oomaya_wm_state_t *s);

uint32_t oomaya_state_get_tag_mask(const oomaya_wm_state_t *s);
void     oomaya_state_set_tag_mask(oomaya_wm_state_t *s, uint32_t mask);

uint32_t oomaya_state_get_layout(const oomaya_wm_state_t *s);
void     oomaya_state_set_layout(oomaya_wm_state_t *s, uint32_t idx);

uint32_t oomaya_state_get_focused(const oomaya_wm_state_t *s);
void     oomaya_state_set_focused(oomaya_wm_state_t *s, uint32_t xid);

uint32_t oomaya_state_get_monitor_count(const oomaya_wm_state_t *s);
void     oomaya_state_set_monitor_count(oomaya_wm_state_t *s, uint32_t n);

uint32_t oomaya_state_get_monitor_tag(const oomaya_wm_state_t *s, uint32_t mon);
void     oomaya_state_set_monitor_tag(oomaya_wm_state_t *s, uint32_t mon, uint32_t mask);

#endif /* OOMAYA_STATE_H */
