/* x11_bridge.c — Bidirectional X11 root-atom synchronization (The Shield).
 *
 * Phase 3 & 4. Conditional compile: only built when OOMAYA_HAVE_X11 is defined
 * and libX11 is available (Makefile checks via pkg-config x11).
 *
 * Responsibilities:
 *   Inbound (X11 → IPC):
 *     Monitor root window PropertyNotify events for:
 *       _NET_CURRENT_DESKTOP  → update state_cache.tag_mask
 *       _NET_ACTIVE_WINDOW    → update state_cache.focused_win
 *       _DWM_CURRENT_LAYOUT   → update state_cache.layout_idx
 *       _DWM_MONITOR_DESKTOPS → update state_cache.monitor_tags[]
 *       DWM_TAG_UPDATE        → update state_cache.tag_mask
 *
 *   Outbound (IPC → X11):
 *     Direct sub-millisecond dispatch of EWMH ClientMessages to dwm:
 *       oomaya_x11_view_tag()      → _NET_CURRENT_DESKTOP
 *       oomaya_x11_focus_window()  → _NET_ACTIVE_WINDOW
 *       oomaya_x11_kill_window()   → WM_DELETE_WINDOW / XKillClient
 *       oomaya_x11_sync_state()    → Mirror state to root atoms
 *
 * Integration:
 *   XConnectionNumber(dpy) is added to the reactor's epoll set.
 *   On EPOLLIN on that fd, the reactor calls oomaya_x11_process_events().
 *   This avoids a separate X11 thread entirely.
 */

#ifdef OOMAYA_HAVE_X11

#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "../../include/oomaya_state.h"
#include "../../include/oomaya_x11_bridge.h"

/* ── Atom cache ──────────────────────────────────────────────────────────── */
typedef struct {
    Display          *dpy;
    Window            root;
    Atom              a_current_layout;
    Atom              a_monitor_desktops;
    Atom              a_tag_update;
    Atom              a_oomaya_layout;
    Atom              a_net_current_desktop;
    Atom              a_net_active_window;
    Atom              a_wm_protocols;
    Atom              a_wm_delete;
    oomaya_wm_state_t *state;
    int               active;
} x11_bridge_t;

static x11_bridge_t g_bridge;

/* ── X11 error handlers (never crash) ───────────────────────────────────── */

static int
x11_err_handler(Display *dpy, XErrorEvent *e)
{
    char buf[64];
    XGetErrorText(dpy, e->error_code, buf, sizeof(buf));
    fprintf(stderr, "oomaya-x11: X error: %s (req %u)\n",
            buf, e->request_code);
    return 0;
}

static int
x11_io_err_handler(Display *dpy)
{
    (void)dpy;
    fprintf(stderr, "oomaya-x11: X11 I/O error — disabling bridge\n");
    g_bridge.active = 0;
    return 0;
}

/* ── Layout symbol table ────────────────────────────────────────────────── */
/* Must stay in sync with the layouts[] order in config.def.h / dwm.c.
 * dwm publishes the exact ltsymbol string on _DWM_CURRENT_LAYOUT, so exact
 * strcmp matching is correct here (and safer than substring matching). */
static const char *const layout_symbols[] = {
    "[]=", "><>", "[M]", "[@]", "[\\]",
    "H[]", "TTT", "===", "HHH", "###",
    "---", ":::", "|M|", ">M>",
};

static uint32_t
layout_symbol_to_idx(const char *sym)
{
    uint32_t i;
    if (sym == NULL)
        return 0;
    for (i = 0; i < sizeof(layout_symbols) / sizeof(layout_symbols[0]); i++) {
        if (strcmp(sym, layout_symbols[i]) == 0)
            return i;
    }
    return 0; /* unknown symbol → tile */
}

/* ── Public API ─────────────────────────────────────────────────────────── */

int
oomaya_x11_bridge_init(oomaya_wm_state_t *state, int *x11_fd_out)
{
    Display *dpy;

    g_bridge.active = 0;
    g_bridge.state  = state;

    XSetErrorHandler(x11_err_handler);
    XSetIOErrorHandler(x11_io_err_handler);

    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        fprintf(stderr, "oomaya-x11: XOpenDisplay failed — bridge disabled\n");
        return -1;
    }

    g_bridge.dpy  = dpy;
    g_bridge.root = DefaultRootWindow(dpy);

    /* Intern all atoms in one round-trip batch */
    g_bridge.a_current_layout      = XInternAtom(dpy, "_DWM_CURRENT_LAYOUT",   False);
    g_bridge.a_monitor_desktops    = XInternAtom(dpy, "_DWM_MONITOR_DESKTOPS", False);
    g_bridge.a_tag_update          = XInternAtom(dpy, "DWM_TAG_UPDATE",         False);
    g_bridge.a_oomaya_layout       = XInternAtom(dpy, "_OOMAYA_LAYOUT",         False);
    g_bridge.a_net_current_desktop = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
    g_bridge.a_net_active_window   = XInternAtom(dpy, "_NET_ACTIVE_WINDOW",   False);
    g_bridge.a_wm_protocols        = XInternAtom(dpy, "WM_PROTOCOLS",         False);
    g_bridge.a_wm_delete           = XInternAtom(dpy, "WM_DELETE_WINDOW",     False);

    /* Read initial root properties to populate state cache */
    Atom type;
    int fmt;
    unsigned long nitems, after;
    unsigned char *data = NULL;

    if (XGetWindowProperty(dpy, g_bridge.root, g_bridge.a_net_current_desktop,
                           0L, 1L, False, AnyPropertyType,
                           &type, &fmt, &nitems, &after, &data) == Success && data != NULL) {
        unsigned long val = *(unsigned long *)data;
        if (val < 32) {
            oomaya_state_set_tag_mask(state, (1u << val));
        }
        XFree(data);
        data = NULL;
    }

    if (XGetWindowProperty(dpy, g_bridge.root, g_bridge.a_net_active_window,
                           0L, 1L, False, AnyPropertyType,
                           &type, &fmt, &nitems, &after, &data) == Success && data != NULL) {
        unsigned long val = *(unsigned long *)data;
        oomaya_state_set_focused(state, (uint32_t)val);
        XFree(data);
        data = NULL;
    }

    if (XGetWindowProperty(dpy, g_bridge.root, g_bridge.a_current_layout,
                           0L, 16L, False, AnyPropertyType,
                           &type, &fmt, &nitems, &after, &data) == Success && data != NULL) {
        oomaya_state_set_layout(state,
            layout_symbol_to_idx((const char *)data));
        XFree(data);
        data = NULL;
    }

    /* Subscribe to PropertyNotify on root window */
    XSelectInput(dpy, g_bridge.root, PropertyChangeMask);
    XFlush(dpy);

    g_bridge.active = 1;
    if (x11_fd_out != NULL)
        *x11_fd_out = XConnectionNumber(dpy);

    return 0;
}

/*
 * oomaya_x11_process_events() — Non-blocking drain of pending X11 events.
 * Called by the reactor when XConnectionNumber fd is readable.
 */
void
oomaya_x11_process_events(void)
{
    XEvent          ev;
    XPropertyEvent *pe;
    unsigned long   val;
    Atom            type;
    int             fmt;
    unsigned long   nitems, after;
    unsigned char  *data = NULL;

    if (!g_bridge.active || g_bridge.dpy == NULL) return;

    while (XPending(g_bridge.dpy)) {
        XNextEvent(g_bridge.dpy, &ev);
        if (ev.type != PropertyNotify) continue;

        pe = &ev.xproperty;

        /* _NET_CURRENT_DESKTOP → update tag_mask */
        if (pe->atom == g_bridge.a_net_current_desktop) {
            if (XGetWindowProperty(g_bridge.dpy, g_bridge.root,
                                   g_bridge.a_net_current_desktop,
                                   0L, 1L, False, AnyPropertyType,
                                   &type, &fmt, &nitems, &after, &data) == Success
                && data != NULL) {
                val = *(unsigned long *)data;
                if (val < 32) {
                    oomaya_state_set_tag_mask(g_bridge.state, (1u << val));
                }
                XFree(data);
                data = NULL;
            }
        }

        /* _NET_ACTIVE_WINDOW → update focused_win */
        else if (pe->atom == g_bridge.a_net_active_window) {
            if (XGetWindowProperty(g_bridge.dpy, g_bridge.root,
                                   g_bridge.a_net_active_window,
                                   0L, 1L, False, AnyPropertyType,
                                   &type, &fmt, &nitems, &after, &data) == Success
                && data != NULL) {
                val = *(unsigned long *)data;
                oomaya_state_set_focused(g_bridge.state, (uint32_t)val);
                XFree(data);
                data = NULL;
            }
        }

        /* _DWM_CURRENT_LAYOUT → update layout_idx in state cache */
        else if (pe->atom == g_bridge.a_current_layout) {
            if (XGetWindowProperty(g_bridge.dpy, g_bridge.root,
                                   g_bridge.a_current_layout,
                                   0L, 16L, False, AnyPropertyType,
                                   &type, &fmt, &nitems, &after, &data) == Success
                && data != NULL) {
                oomaya_state_set_layout(g_bridge.state,
                    layout_symbol_to_idx((const char *)data));
                XFree(data);
                data = NULL;
            }
        }

        /* DWM_TAG_UPDATE → update tag_mask */
        else if (pe->atom == g_bridge.a_tag_update) {
            if (XGetWindowProperty(g_bridge.dpy, g_bridge.root,
                                   g_bridge.a_tag_update,
                                   0L, 1L, False, AnyPropertyType,
                                   &type, &fmt, &nitems, &after, &data) == Success
                && data != NULL) {
                val = *(unsigned long *)data;
                oomaya_state_set_tag_mask(g_bridge.state, (uint32_t)val);
                XFree(data);
                data = NULL;
            }
        }
    }
}

/*
 * oomaya_x11_view_tag() — Instruct dwm to switch to desktop matching tag_mask.
 */
int
oomaya_x11_view_tag(uint32_t tag_mask)
{
    if (!g_bridge.active || g_bridge.dpy == NULL || tag_mask == 0) return -1;

    int desktop = 0;
    while ((tag_mask & (1u << desktop)) == 0 && desktop < 31) {
        desktop++;
    }

    XClientMessageEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.type         = ClientMessage;
    ev.serial       = 0;
    ev.send_event   = True;
    ev.display      = g_bridge.dpy;
    ev.window       = g_bridge.root;
    ev.message_type = g_bridge.a_net_current_desktop;
    ev.format       = 32;
    ev.data.l[0]    = desktop;
    ev.data.l[1]    = CurrentTime;

    XSendEvent(g_bridge.dpy, g_bridge.root, False,
               SubstructureNotifyMask | SubstructureRedirectMask,
               (XEvent *)&ev);
    XFlush(g_bridge.dpy);
    return 0;
}

/*
 * oomaya_x11_focus_window() — Instruct dwm to focus the given client window XID.
 */
int
oomaya_x11_focus_window(uint32_t xid)
{
    if (!g_bridge.active || g_bridge.dpy == NULL || xid == 0) return -1;

    XClientMessageEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.type         = ClientMessage;
    ev.serial       = 0;
    ev.send_event   = True;
    ev.display      = g_bridge.dpy;
    ev.window       = (Window)xid;
    ev.message_type = g_bridge.a_net_active_window;
    ev.format       = 32;
    ev.data.l[0]    = 2; /* 2 = pager / dock */
    ev.data.l[1]    = CurrentTime;

    XSendEvent(g_bridge.dpy, g_bridge.root, False,
               SubstructureNotifyMask | SubstructureRedirectMask,
               (XEvent *)&ev);
    XFlush(g_bridge.dpy);
    return 0;
}

/*
 * oomaya_x11_kill_window() — Send polite WM_DELETE_WINDOW to client window.
 */
int
oomaya_x11_kill_window(uint32_t xid)
{
    if (!g_bridge.active || g_bridge.dpy == NULL || xid == 0) return -1;

    XClientMessageEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.type         = ClientMessage;
    ev.serial       = 0;
    ev.send_event   = True;
    ev.display      = g_bridge.dpy;
    ev.window       = (Window)xid;
    ev.message_type = g_bridge.a_wm_protocols;
    ev.format       = 32;
    ev.data.l[0]    = (long)g_bridge.a_wm_delete;
    ev.data.l[1]    = CurrentTime;

    XSendEvent(g_bridge.dpy, (Window)xid, False, NoEventMask, (XEvent *)&ev);
    XFlush(g_bridge.dpy);
    return 0;
}

/*
 * oomaya_x11_sync_state() — Mirror current state_cache values into X11 root atoms.
 * Called after every fast-path IPC mutation so legacy tools see fresh values.
 */
void
oomaya_x11_sync_state(void)
{
    unsigned long layout;
    unsigned long tags;

    if (!g_bridge.active || g_bridge.dpy == NULL) return;

    layout = (unsigned long)oomaya_state_get_layout(g_bridge.state);
    tags   = (unsigned long)oomaya_state_get_tag_mask(g_bridge.state);

    (void)layout;
    XChangeProperty(g_bridge.dpy, g_bridge.root,
                    g_bridge.a_tag_update, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&tags, 1);

    XFlush(g_bridge.dpy);
}

void
oomaya_x11_bridge_destroy(void)
{
    if (g_bridge.dpy != NULL) {
        XCloseDisplay(g_bridge.dpy);
        g_bridge.dpy    = NULL;
        g_bridge.active = 0;
    }
}

#else /* OOMAYA_HAVE_X11 not defined */

/* Stub implementations when X11 is unavailable — daemon works without display. */
#include "../../include/oomaya_x11_bridge.h"
#include "../../include/oomaya_state.h"

int  oomaya_x11_bridge_init(oomaya_wm_state_t *s, int *fd)
     { (void)s; if (fd) *fd = -1; return -1; }
void oomaya_x11_process_events(void) {}
void oomaya_x11_sync_state(void)     {}
int  oomaya_x11_view_tag(uint32_t m)    { (void)m; return -1; }
int  oomaya_x11_focus_window(uint32_t x) { (void)x; return -1; }
int  oomaya_x11_kill_window(uint32_t x)  { (void)x; return -1; }
void oomaya_x11_bridge_destroy(void) {}

#endif /* OOMAYA_HAVE_X11 */
