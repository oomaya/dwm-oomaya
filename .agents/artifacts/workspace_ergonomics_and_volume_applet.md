# Workspace Ergonomics, Dunst Heartbeat & Volume Control Integration

## 1. The Togglebar (<kbd>Super</kbd> + <kbd>B</kbd>) Root Cause & Fix

### Root Cause Diagnosis
When <kbd>Super</kbd> + <kbd>B</kbd> unmapped `selmon->barwin`, the X server sent an `UnmapNotify` event to the root window. In `dwm.c`, `unmapnotify()` caught this event and unconditionally called `unmanagealtbar(ev->window)`. 

Because `unmanagealtbar` had an `else if (m->barwin == w)` branch originally intended for altbar docks, it wiped `m->barwin = 0; m->by = 0; m->bh = 0;`. When <kbd>Super</kbd> + <kbd>B</kbd> was pressed a second time, `selmon->barwin` was NULL, permanently preventing the native bar from re-mapping until dwm was restarted.

### Surgical Resolution (Committed `3703a30`)
1. **Altbar Guard**: `unmapnotify()` and `destroynotify()` now strictly verify `m->isaltbar` before invoking `unmanagealtbar()`.
2. **Native Protection**: `unmanagealtbar()` will never zero out native bar geometry.
3. **Live Verified**: Tested live on `DISPLAY=:0`. <kbd>Super</kbd> + <kbd>B</kbd> hides the bar offscreen (`+0+-30`), and a second press restores it immediately (`+0+0`).

---

## 2. Workspace Catch-22 Resolved (Option A: Classic Chadwm Ticker)

### Architecture
- **Visible Horizon**: Restored all tags `1 2 3 4 5 6 7 8 9` on the top bar.
- **Active Workspace**: Renders as a Tokyo Night blue pill (`#7aa2f7`).
- **Occupied Workspaces**: Render with a sharp corner ticker dot (`drw_rect` in Tokyo Night Accent).
- **Vacant Workspaces**: Render in subtle dim gray (`#414868`), fully clickable by mouse at all times.
- **Hit Testing Synchronization**: Removed the vacant tag filter in `buttonpress()` so mouse click hit calculations match `drawbar()` 1:1.

### Live Verification
```bash
# Mouse click at x=45 (Tag 2 center) switched desktop:
_NET_CURRENT_DESKTOP = 1
# Mouse click at x=15 (Tag 1 center) switched desktop:
_NET_CURRENT_DESKTOP = 0
```

---

## 3. Dunst Notification Heartbeat & DBus Conflict Resolution

### Root Causes of Silence
1. **DBus Service Conflict**: Linux Mint ships `/usr/share/dbus-1/services/org.freedesktop.mate.Notifications.service`, which intercepted DBus activation and conflicted with Dunst.
2. **Dunst 1.9.2 Configuration Syntax**: The Dunst config contained `height = (60, 300)`. Dunst 1.9.2 on Ubuntu 24.04 noble only accepts a single integer `height = 300` and failed parsing with `WARNING: '(60, 300)': No digits found`.

### Surgical Fix
1. **DBus User Service Override**: Added overrides in `~/.local/share/dbus-1/services/` routing notification activation directly to `/usr/bin/dunst`.
2. **Configuration Normalization**: Updated `config/dunst/dunstrc` and `~/.config/dunst/dunstrc` to `height = 300`.
3. **Daemon Activation**: Started and enabled `dunst.service` via `systemctl --user`. Updated `scripts/autostart.sh` to prioritize `systemctl --user start dunst.service`.
4. **Heartbeat Verified**: Sent test notification and inspected `dunstctl history`, confirming successful event delivery.

---

## 4. Volume Control UX: Mouse Wheel Scroll + Dunst OSD

### Implemented Stack
- **Audio Helper Script**: [`scripts/dwm-volume`](file:///home/rand/.local/src/dwm-oomaya/scripts/dwm-volume) installed to `~/.local/bin/dwm-volume`.
  - Conforms to **Zero Hardcoded Literals Law** (dynamically uses `pactl` or `wpctl`).
  - Actions: `up [step]`, `down [step]`, `mute`, `mixer`, `status`.
  - Flashes a sleek Tokyo Night Dunst notification with a native progress bar on volume change (`-h string:x-dunst-stack-tag:volume -h int:value:$vol`).
- **Hardware Multimedia Keys**: Bound `XF86AudioRaiseVolume`, `XF86AudioLowerVolume`, and `XF86AudioMute` to `dwm-volume` in [`config/hotkeys.toml`](file:///home/rand/.local/src/dwm-oomaya/config/hotkeys.toml).
- **Interactive Top Bar (`ClkStatusText`)**:
  - **Scroll Up (Button 4)**: Volume Up + Dunst OSD.
  - **Scroll Down (Button 5)**: Volume Down + Dunst OSD.
  - **Right Click (Button 3)**: Mute Toggle + Dunst OSD.
  - **Left Click (Button 1)**: Launches mixer (`pavucontrol` / `alsamixer`).

---

## 5. Critical Discovery & Fix: TOML Parser Capacity (`TOML_MAX_ENTRIES`)

### The Hidden Trap
During testing of `ClkStatusText` mouse scroll events, volume remained unchanged despite correct configuration. Investigation revealed:
- `tomlparser.h` defined `#define TOML_MAX_ENTRIES 512`.
- As `hotkeys.toml` grew, the document reached **557 entries** (94 keys $\times$ 4 entries + tag_keys + buttons).
- At entry 512, the parser silently dropped all remaining entries.
- Dropped entries included `ClkTabBar`, all 4 `ClkStatusText` volume controls, and `ClkClientWin` window dragging/resizing bindings!

### The Fix (Committed `9fe6b3d`)
1. **Capacity Expansion**: Expanded `TOML_MAX_ENTRIES` from 512 to **1024** in [`tomlparser.h`](file:///home/rand/.local/src/dwm-oomaya/tomlparser.h).
2. **Verification**: Parser now loads all 557 entries, including all 18 mouse button actions.
3. **Live Verified**: Mouse wheel scroll over the top bar status text now smoothly raises/lowers volume, triggering the Dunst OSD in real-time.
