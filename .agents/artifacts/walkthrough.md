# Walkthrough: DWM-Oomaya Keybindings, Layouts, Flow Cockpit & Documentation

We addressed all reported keybinding conflicts, layout anomalies, vanity gap controls, and README instructions across `dwm-oomaya`.

---

## Changes Implemented

### 1. Keybindings & Launchers
- **<kbd>Super</kbd> + <kbd>A</kbd> $\rightarrow$ Antigravity IDE (GUI)**:
  - Unbound from `alacritty` (zero conflict).
  - Deployed dynamic launcher [antigravity](file:///home/rand/.local/bin/antigravity) resolving local IDE installs without hardcoded usernames.
- **<kbd>Super</kbd> + <kbd>C</kbd> / <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>C</kbd> $\rightarrow$ Sovereign Flow Cockpit**:
  - Implemented [scripts/dwm-flow](file:///home/rand/dwm-oomaya/scripts/dwm-flow) and installed to `~/.local/bin/dwm-flow`.
  - Spawns `agy -c` on the left pane (master) and `nvim` (LazyVim) on the right pane (stack) in Tile layout, reproducing your Omarchy flow cockpit.
- **<kbd>Super</kbd> + <kbd>D</kbd> $\rightarrow$ Desktop App Launcher (`dmenu-desktop`)**:
  - Decoupled `incnmaster -1` collision (which previously dropped `nmaster` to 0 and caused the horizontal strip collapse).
  - Moved `incnmaster -1` to <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>I</kbd> (symmetrically paired with <kbd>Super</kbd> + <kbd>I</kbd> for `incnmaster +1`).

### 2. Layouts & Gap Controls
- **Cycle Layouts (<kbd>Super</kbd> + <kbd>Tab</kbd> / <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>Tab</kbd>)**:
  - Replaced tag toggle `view {0}` with `cyclelayout (+1)` and `cyclelayout (-1)` for bidirectional layout navigation.
- **Switch to Monocle (<kbd>Super</kbd> + <kbd>M</kbd>)**:
  - Corrected layout index from `2` (which pointed to `spiral`) to `1` (`monocle`).
  - Updated layout symbol button 3 click to `&layouts[1]`.
- **Switch to Tile (<kbd>Super</kbd> + <kbd>T</kbd>)**:
  - Verified `layout_idx = 0` (`[]=`). The previous "horizontal grid" appearance was diagnosed as `nmaster` having been decremented to 0 via the old `Super + D` dual-binding.
- **Vanity Gaps**:
  - Wired upstream suckless bindings: <kbd>Super</kbd> + <kbd>Alt</kbd> + <kbd>0</kbd> (toggle), <kbd>Super</kbd> + <kbd>Alt</kbd> + <kbd>=</kbd> (+5px), <kbd>Super</kbd> + <kbd>Alt</kbd> + <kbd>-</kbd> (-5px), and <kbd>Super</kbd> + <kbd>Alt</kbd> + <kbd>Shift</kbd> + <kbd>=</kbd> (reset default 10px).
  - Updated `Super + Ctrl` aliases to visually noticeable 5px increments.
- **Hide / Restore Window**:
  - Documented <kbd>Super</kbd> + <kbd>E</kbd> (`hidewin`) and <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>E</kbd> (`restorewin`) in `README.md`.
  - Documented <kbd>Super</kbd> + <kbd>H</kbd> / <kbd>Super</kbd> + <kbd>L</kbd> for master resizing.

### 3. Systems Craftsmanship Runbook
- Authored [docs/runbooks/compressed_apps_linux_guide.md](file:///home/rand/dwm-oomaya/docs/runbooks/compressed_apps_linux_guide.md) (mirrored to `~/Documents/artifacts/guides/`):
  - Standard Unix FHS hierarchy (`/opt/` vs `~/.local/share/`).
  - JetBrains suites, VMware Workstation (`.bundle`, services, kernel modules), and AppImages.
  - XDG desktop entry integration and `/usr/local/bin` trampoline symlinks.

### 4. Documentation & Installation
- Overhauled [README.md](file:///home/rand/dwm-oomaya/README.md):
  - Updated Keyboard Mastery reference table.
  - Section 2 (Build & Install): Dual-path deployment (`make install-local` and `sudo make install-system`) to guarantee display manager parity.
  - Section 3 (Install Dmenu Ecosystem): Updated `dmenu-oomaya` build & install with atomic `install -Dm755`.

---

## Verification Results

### 1. Automated Build Gates
- `make clean && make -j$(nproc) dwm`: **PASS** (Zero warnings, clean compilation).
- `tests/test-configure-build.sh`: **PASS** (Configuration generator and header preservation validated).

### 2. Live Runtime Validation (via `xdotool` & `xprop`)
- `xdotool key Super_L+m`: `_DWM_CURRENT_LAYOUT` switched to `"[M]"` (Monocle) **PASS**.
- `xdotool key Super_L+t`: `_DWM_CURRENT_LAYOUT` switched to `"[]="` (Tile) **PASS**.
- `xdotool key Super_L+Tab`: Cycled forward across layouts (`"[]="` $\rightarrow$ `"[M]"` $\rightarrow$ `"[@]"`) **PASS**.
- `xdotool key Super_L+Shift_L+Tab`: Cycled reverse across layouts **PASS**.
- `kill -USR1 $(pidof dwm)`: Reloaded runtime TOML configuration cleanly **PASS**.

### 3. Sandboxed Build & Install Testing
- Tested in isolated `mktemp -d` sandbox:
  - `DESTDIR=$SANDBOX make install-system` (verified `dwm`, `dwm-flow`, desktop session, man pages) **PASS**.
  - `DESTDIR=$SANDBOX ... make install-user` (verified user tree, TOML configs, autostart overrides) **PASS**.
  - `DESTDIR=$SANDBOX make install` in `dmenu-oomaya` (verified binaries and all 7 scripts) **PASS**.
