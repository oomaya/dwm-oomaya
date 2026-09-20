# Suckless Topbar & Dunst Notification Fallback Restoration

## Executive Summary

The native Chadwm C top bar and status bar have been fully un-stubbed and restored in `dwm-oomaya`. On machines where modern Quickshell is unavailable (e.g., Linux Mint 22.3 / Ubuntu 24.04 noble due to Qt 6.4 limits), `dwm` now dynamically initializes its native Tokyo Night top bar (`default_bh = drw->fonts->h + 4`), rendering workspace tags, layout symbol, active window title, and `dwm-status` text. On rolling nodes where Quickshell is running, `dwm` automatically yields the bar geometry to Quickshell without conflict.

The update was applied, compiled, and reloaded live in-place via <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>R</kbd> without requiring an OS reboot or closing running applications.

---

## Architectural Changes in `dwm.c`

### 1. Dynamic Coexistence State Machine

`struct Monitor` was augmented to distinguish between native X11 bar windows and external dock windows:

```c
struct Monitor {
    ...
    Window barwin;          /* Active bar window handle */
    Window nativebarwin;    /* Native X11 window created by dwm */
    int isaltbar;           /* 1 if managed by external dock (Quickshell), 0 if native */
    ...
};
```

| Lifecycle Event | Action Taken |
| :--- | :--- |
| **dwm Startup (`setup()`)** | Sets `default_bh = drw->fonts->h + 4`. Monitors initialize `m->bh = default_bh`. |
| **Bar Creation (`updatebars()`)** | If `!m->isaltbar`, creates `m->nativebarwin`, maps it, and sets `m->barwin = m->nativebarwin`. |
| **Altbar Discovery (`updatealtbar()`)** | If Quickshell docks, `m->nativebarwin` is unmapped, `m->isaltbar = 1`, and `m->barwin = quickshell_win`. |
| **Altbar Termination (`unmanagealtbar()`)** | If Quickshell closes or crashes, `m->isaltbar = 0`, `m->bh = default_bh`, `m->nativebarwin` is re-mapped, and `drawbar(m)` renders immediately. |
| **dwm Cleanup (`cleanupmon()`)** | Cleans up and destroys `m->nativebarwin` safely on window manager shutdown. |

### 2. Un-stubbed `drawbar(Monitor *m)`

Replaced the upstream hardcoded `return;` with full Chadwm Tokyo Night rendering:
- **Guard**: `if (!m || !m->barwin || m->isaltbar || !m->showbar) return;`
- **Right Status**: Status text from root `WM_NAME` (`dwm-status` setting `"AC | VOL 50%"`) right-aligned.
- **Left Tags**: Active and occupied tags rendered sequentially.
- **Layout Symbol**: Pill rendered immediately adjacent to tags.
- **Active Title**: Truncated window title rendered in the remaining canvas with `SchemeTitle` (`#a9b1d6` on `#1a1b26`).
- **Buffer Flush**: Flushed to `m->barwin` via `drw_map()`.

### 3. Fontconfig & Compound Nerd Font Hardening

Added `JetBrainsMono Nerd Font` alongside `MesloLGS Nerd Font Mono` to `fonts[]` in `config.def.h`:
```c
static const char *fonts[] = {
    "JetBrainsMono Nerd Font:size=14:antialias=true:autohint=true",
    "MesloLGS Nerd Font Mono:size=14:antialias=true:autohint=true",
    "NotoColorEmoji:pixelsize=14:antialias=true:autohint=true"
};
```

---

## Verification & Test Results

### 1. Invariant Test Suite (`tests/test-suckless-bar-fallback.sh`)
```
==> Running Suckless Bar Fallback Invariant Tests
Test 1: Struct Monitor tracks nativebarwin and isaltbar... PASS
Test 2: drawbar() is un-stubbed (no unconditional return)... PASS
Test 3: setup() calculates native default_bh... PASS
Test 4: updatebars() creates native bar window... PASS
Test 5: updatealtbar() unmaps native bar on altbar docking... PASS
Test 6: unmanagealtbar() restores native bar on altbar exit... PASS
Test 7: config.def.h includes JetBrainsMono Nerd Font in fonts[]... PASS

ALL SUCKLESS BAR FALLBACK INVARIANT TESTS PASSED
```

### 2. Live Window Verification on `DISPLAY=:0`
```
xwininfo -id 0x40001f
  Absolute upper-left X:  0
  Absolute upper-left Y:  0
  Width: 2558
  Height: 30
  Map State: IsViewable
  Class: ("dwm" "dwm")
```
Window area for tiled clients automatically shifted to accommodate the 30px bar without overlap.

---

## Git Operations & Federation

All commits pushed to GitHub upstream:
1. `dwm-oomaya`: Commit `8a46b66` pushed to `origin/main`.
2. `dmenu-oomaya`: Commit `2773f93` pushed to `origin/master`.
3. `antigravity-skills`: Commit `65ebdac` pushed to `origin/main`.
4. Living Handoff: `20260920_143958_mint-vm_suckless_topbar_and_dunst_notification_fallback_restoration.md` created.
