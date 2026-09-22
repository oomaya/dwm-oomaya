/* oomaya_x11_bridge.h — X11 root-atom bidirectional bridge API (The Shield). */
#ifndef OOMAYA_X11_BRIDGE_H
#define OOMAYA_X11_BRIDGE_H

#include <stdint.h>
#include "oomaya_state.h"

int  oomaya_x11_bridge_init(oomaya_wm_state_t *state, int *x11_fd_out);
void oomaya_x11_process_events(void);
void oomaya_x11_sync_state(void);
int  oomaya_x11_view_tag(uint32_t tag_mask);
int  oomaya_x11_focus_window(uint32_t xid);
int  oomaya_x11_kill_window(uint32_t xid);
void oomaya_x11_bridge_destroy(void);

#endif /* OOMAYA_X11_BRIDGE_H */
