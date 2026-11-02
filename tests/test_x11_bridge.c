/* test_x11_bridge.c - Bridge root-atom semantics against a real X server.
 *
 * A second X connection plays dwm, writing root properties a live window
 * manager owns. It must never reach a real session, so it ignores the
 * inherited DISPLAY: only OOMAYA_TEST_X11_DISPLAY selects a server, and a
 * server advertising an EWMH window manager is refused before anything is
 * written. Exits 77 when no test display is named. Use tests/test-x11-bridge.sh.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "../include/oomaya_state.h"
#include "../include/oomaya_x11_bridge.h"

#define SKIP_STATUS 77
#define REFUSE_STATUS 2
#define TEST_DISPLAY_ENV "OOMAYA_TEST_X11_DISPLAY"

#ifndef OOMAYA_HAVE_X11

int main(void)
{
    printf("SKIP: built without X11\n");
    return SKIP_STATUS;
}

#else

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#define COUNTER_A 1000ul
#define COUNTER_B 1001ul
#define STALE_MASK 0x80u
#define SYNC_MASK 0x55u
#define DESKTOP 2ul
#define DESKTOP_MASK (1u << DESKTOP)
#define PUMP_MS 2000

static int g_pass = 0, g_fail = 0;
#define ASSERT_EQ(lbl, got, exp) \
    do { \
        long long _g=(long long)(got), _e=(long long)(exp); \
        if (_g==_e){printf("  [PASS] %s\n",(lbl));++g_pass;} \
        else{printf("  [FAIL] %s got=%lld exp=%lld\n",(lbl),_g,_e);++g_fail;} \
    } while(0)

static Display *g_wm;
static Window g_root;
static oomaya_wm_state_t g_state;
static unsigned long g_fence = 0x10000ul;

static void
set_cardinal(const char *name, unsigned long value)
{
    XChangeProperty(g_wm, g_root, XInternAtom(g_wm, name, False), XA_CARDINAL,
                    32, PropModeReplace, (unsigned char *)&value, 1);
    XSync(g_wm, False);
}

static long long
get_cardinal(const char *name)
{
    Atom type;
    int fmt;
    unsigned long nitems, after;
    unsigned char *data = NULL;
    long long value = -1;

    if (XGetWindowProperty(g_wm, g_root, XInternAtom(g_wm, name, False), 0L, 1L,
                           False, AnyPropertyType, &type, &fmt, &nitems, &after,
                           &data) == Success && data != NULL) {
        if (nitems > 0)
            value = (long long)*(unsigned long *)data;
        XFree(data);
    }
    return value;
}

static void
sleep_ms(long ms)
{
    struct timespec ts = { .tv_sec = 0, .tv_nsec = ms * 1000000L };

    nanosleep(&ts, NULL);
}

/*
 * Events from one connection arrive in order, so once the bridge sees this
 * fence it has also processed every property written before it. A "value did
 * not change" assertion is meaningless without this.
 */
static int
fence(void)
{
    unsigned long marker = ++g_fence;
    int ms;

    set_cardinal("_NET_ACTIVE_WINDOW", marker);
    for (ms = 0; ms < PUMP_MS; ms++) {
        oomaya_x11_process_events();
        if (oomaya_state_get_focused(&g_state) == (uint32_t)marker)
            return 0;
        sleep_ms(1);
    }
    return -1;
}

static long long
wait_cardinal(const char *name, unsigned long want)
{
    long long got = -1;
    int ms;

    for (ms = 0; ms < PUMP_MS; ms++) {
        got = get_cardinal(name);
        if (got == (long long)want)
            break;
        sleep_ms(1);
    }
    return got;
}

int main(void)
{
    int x11_fd = -1;
    const char *display = getenv(TEST_DISPLAY_ENV);

    if (display == NULL || display[0] == '\0') {
        printf("SKIP: %s is not set\n", TEST_DISPLAY_ENV);
        return SKIP_STATUS;
    }
    g_wm = XOpenDisplay(display);
    if (g_wm == NULL) {
        printf("FAIL: cannot open test display %s\n", display);
        return REFUSE_STATUS;
    }
    g_root = DefaultRootWindow(g_wm);
    if (get_cardinal("_NET_SUPPORTING_WM_CHECK") != -1) {
        printf("REFUSED: a window manager is running on %s\n", display);
        XCloseDisplay(g_wm);
        return REFUSE_STATUS;
    }
    /* The bridge opens its own connection through DISPLAY. */
    if (setenv("DISPLAY", display, 1) != 0) {
        XCloseDisplay(g_wm);
        return REFUSE_STATUS;
    }
    oomaya_state_init(&g_state);

    /* A value left on a reused server must not satisfy X04. */
    XDeleteProperty(g_wm, g_root, XInternAtom(g_wm, "_OOMAYA_TAG_MASK", False));
    set_cardinal("DWM_TAG_UPDATE", COUNTER_A - 1);
    set_cardinal("_NET_CURRENT_DESKTOP", DESKTOP);
    ASSERT_EQ("X01 bridge init", oomaya_x11_bridge_init(&g_state, &x11_fd), 0);
    ASSERT_EQ("X01 init reads _NET_CURRENT_DESKTOP",
              oomaya_state_get_tag_mask(&g_state), DESKTOP_MASK);

    /* dwm's counter is a change signal, never a mask (C2). */
    set_cardinal("DWM_TAG_UPDATE", COUNTER_A);
    ASSERT_EQ("X02 fence", fence(), 0);
    ASSERT_EQ("X02 counter does not stomp tag_mask",
              oomaya_state_get_tag_mask(&g_state), DESKTOP_MASK);

    /* The signal must re-read _NET_CURRENT_DESKTOP, not merely be ignored. */
    oomaya_state_set_tag_mask(&g_state, STALE_MASK);
    set_cardinal("DWM_TAG_UPDATE", COUNTER_B);
    ASSERT_EQ("X03 fence", fence(), 0);
    ASSERT_EQ("X03 signal re-reads _NET_CURRENT_DESKTOP",
              oomaya_state_get_tag_mask(&g_state), DESKTOP_MASK);

    /* One writer per atom: the mirror uses a private atom. */
    oomaya_state_set_tag_mask(&g_state, SYNC_MASK);
    oomaya_x11_sync_state();
    ASSERT_EQ("X04 mirror publishes _OOMAYA_TAG_MASK",
              wait_cardinal("_OOMAYA_TAG_MASK", SYNC_MASK), SYNC_MASK);
    ASSERT_EQ("X05 mirror leaves DWM_TAG_UPDATE alone",
              get_cardinal("DWM_TAG_UPDATE"), COUNTER_B);
    ASSERT_EQ("X06 fence", fence(), 0);
    ASSERT_EQ("X06 mirror causes no self-echo",
              oomaya_state_get_tag_mask(&g_state), SYNC_MASK);

    /* A normal desktop switch still lands. */
    set_cardinal("_NET_CURRENT_DESKTOP", DESKTOP + 1);
    ASSERT_EQ("X07 fence", fence(), 0);
    ASSERT_EQ("X07 desktop switch updates tag_mask",
              oomaya_state_get_tag_mask(&g_state), 1u << (DESKTOP + 1));

    /*
     * Any client can write root properties, and a short format is not a long.
     * Four bytes fill what a 1-long read returns. An unchecked read also takes
     * uninitialized bytes after them, which are zero on default glibc, so this
     * is a best-effort regression pin: a missing check can still pass elsewhere.
     */
    {
        unsigned char bytes[4] = { 5, 0, 0, 0 };

        XChangeProperty(g_wm, g_root,
                        XInternAtom(g_wm, "_NET_CURRENT_DESKTOP", False),
                        XA_CARDINAL, 8, PropModeReplace, bytes, 4);
        XSync(g_wm, False);
    }
    ASSERT_EQ("X08 fence", fence(), 0);
    ASSERT_EQ("X08 format-8 property is ignored",
              oomaya_state_get_tag_mask(&g_state), 1u << (DESKTOP + 1));

    oomaya_x11_bridge_destroy();
    XCloseDisplay(g_wm);

    printf("\n=== X11 Bridge: %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}

#endif /* OOMAYA_HAVE_X11 */
