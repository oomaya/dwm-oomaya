# Implementation Plan: DWM-Oomaya Keybindings, Layouts, Vanity Gaps, Flow Cockpit & README Overhaul

Harmonize the native header (`config.def.h`/`config.h`), TOML runtime configuration (`config/hotkeys.toml`), and documentation (`README.md`) for all requested keybindings, layout switching, vanity gap controls, the Sovereign Flow Cockpit (`agy` + `nvim`), compressed apps installation guide, and sandboxed verification.

---

## 1. Keybinding Architecture & Conflict Resolution

### Analysis of `Super + A`, `Super + C`, and `Super + D`
1. **`Super + A` $\rightarrow$ Antigravity IDE (GUI)**:
   - Previously mapped to `alacritty`. Unbinding Alacritty frees `Super + A` with **zero conflicts**.
   - Highly intuitive mnemonic: `Super + A` = **A**ntigravity IDE.
   - Deploys an executable launcher `~/.local/bin/antigravity` (and `/usr/local/bin/antigravity`) dynamically resolving the executable without hardcoded usernames.

2. **`Super + C` / `Super + Shift + C` $\rightarrow$ Sovereign Flow Cockpit (`dwm-flow`: agy + nvim)**:
   - Mirrors the proven Omarchy dual-pane workflow: spawns `agy` CLI on the left (40% master) and `nvim` (LazyVim) on the right (60% stack).
   - Driven by `scripts/dwm-flow`, which ensures:
     - Left pane: `ghostty --class=org.omarchy.flow.dispatcher -e agy`
     - Right pane: `ghostty --class=org.omarchy.flow.studio -e nvim` (pointing to artifacts or current project)
   - Honors **Rule 7** (`"whenever spawned via CLI, shortcut (Super+Shift+C), or omarchy-flow"`).

3. **`Super + D` $\rightarrow$ Primary Desktop App Launcher (`dmenu-desktop`)**:
   - In standard tiling window managers (i3, sway, bspwm, rofi), `Super + D` is the standard desktop application launcher.
   - Decouples the dual-binding conflict in suckless dwm where `Super + D` also ran `incnmaster -1`.
   - Moves `incnmaster -1` to `<kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>I</kbd>` (paired with `<kbd>Super</kbd> + <kbd>I</kbd>` for `incnmaster +1`), permanently preventing the accidental collapse of Tile layout into horizontal bands!

---

## 2. Comprehensive Keybinding Matrix

| Category | Action | Keybinding | Target / Function | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Launchers** | **Antigravity IDE** | <kbd>Super</kbd> + <kbd>A</kbd> | `antigravity` | GUI IDE; zero conflict. |
| | **Sovereign Flow Cockpit** | <kbd>Super</kbd> + <kbd>C</kbd> / <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>C</kbd> | `dwm-flow` | Spawns `agy` (left) + `nvim` (right) dual pane. |
| | **App Launcher** | <kbd>Super</kbd> + <kbd>D</kbd> | `dmenu-desktop` | Clean, dedicated app launcher. |
| | **Primary Terminal** | <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>Enter</kbd> | `ghostty \|\| kitty` | Unchanged. |
| | **Command Runner** | <kbd>Alt</kbd> + <kbd>P</kbd> | `dmenu-run` | 3-tier self-healing dmenu-run. |
| | **Window Switcher** | <kbd>Alt</kbd> + <kbd>Tab</kbd> | `dmenu-windows` | Centered window switcher. |
| | **Master Action Hub** | <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>H</kbd> | `dmenu-hub` | Master action palette. |
| **Layouts** | **Cycle Layouts Forward** | <kbd>Super</kbd> + <kbd>Tab</kbd> | `cyclelayout (+1)` | Replaces tag toggle `view {0}`. |
| | **Cycle Layouts Reverse** | <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>Tab</kbd> | `cyclelayout (-1)` | Symmetric reverse cycling. |
| | **Switch to Tile Layout** | <kbd>Super</kbd> + <kbd>T</kbd> | `setlayout(&layouts[0])` (`[]=`) | Verified reliable; `nmaster` protected. |
| | **Switch to Monocle Layout**| <kbd>Super</kbd> + <kbd>M</kbd> | `setlayout(&layouts[1])` (`[M]`) | Fixed index from 2 (spiral) to 1 (monocle). |
| **Window State** | **Hide Window** | <kbd>Super</kbd> + <kbd>E</kbd> | `hidewin` | Fixed documentation error in README. |
| | **Restore Hidden Window** | <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>E</kbd> | `restorewin` | Fixed documentation error in README. |
| | **Master Shrink / Grow** | <kbd>Super</kbd> + <kbd>H</kbd> / <kbd>Super</kbd> + <kbd>L</kbd> | `setmfact (-0.05 / +0.05)` | Documented clearly. |
| | **Master Count Add / Sub** | <kbd>Super</kbd> + <kbd>I</kbd> / <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>I</kbd> | `incnmaster (+1 / -1)` | Decoupled from `Super + D`. |
| **Vanity Gaps** | **Toggle All Gaps** | <kbd>Super</kbd> + <kbd>Alt</kbd> + <kbd>0</kbd> | `togglegaps` | Wired to upstream suckless standard. |
| | **Gaps Increase / Decrease**| <kbd>Super</kbd> + <kbd>Alt</kbd> + <kbd>=</kbd> / <kbd>-</kbd> | `incrgaps (+5 / -5)` | Visually noticeable 5px increments. |
| | **Reset Default Gaps** | <kbd>Super</kbd> + <kbd>Alt</kbd> + <kbd>Shift</kbd> + <kbd>=</kbd> | `defaultgaps` | Resets to default padding (10px). |

---

## 3. Proposed Code & Configuration Changes

### Component A: Flow Cockpit & Launcher Scripts
#### [NEW] [dwm-flow](file:///home/rand/dwm-oomaya/scripts/dwm-flow)
- Orchestrates the Sovereign Flow Cockpit in dwm:
  - Ensures layout is Tile (`[]=`) or switches to it.
  - Spawns Ghostty terminal running `agy -c` (Master, left).
  - Spawns Ghostty terminal running `nvim` on workspace/artifacts (Stack, right).
  - Installs to `~/.local/bin/dwm-flow` and `/usr/local/bin/dwm-flow`.

#### [NEW] [antigravity](file:///home/rand/.local/bin/antigravity)
- Portable POSIX wrapper in `~/.local/bin/antigravity` (and `/usr/local/bin/antigravity`):
  Resolves `$HOME/Downloads/Antigravity IDE/antigravity-ide` or `/opt/antigravity-ide/antigravity-ide` without hardcoded paths.

#### [NEW] [compressed_apps_linux_guide.md](file:///home/rand/dwm-oomaya/docs/runbooks/compressed_apps_linux_guide.md)
- Complete engineering guide for installing compressed application downloads (JetBrains suites, VMware Workstation, AppImages, tarballs) cleanly into Linux:
  - FHS hierarchy (`/opt/` vs `~/.local/share/`).
  - Systemd service wiring and kernel modules (VMware).
  - Desktop integration (`.desktop` specs, icon handling, `update-desktop-database`).
  - Binary trampoline wrappers in `/usr/local/bin` and `~/.local/bin`.

---

### Component B: DWM Native Header Configuration
#### [MODIFY] [config.def.h](file:///home/rand/dwm-oomaya/config.def.h) & [config.h](file:///home/rand/dwm-oomaya/config.h)
- Replace `Super + A` with `antigravity`:
  `{ MODKEY, XK_a, spawn, SHCMD("antigravity") },`
- Set `Super + C` and `Super + Shift + C` to `dwm-flow`:
  `{ MODKEY, XK_c, spawn, SHCMD("dwm-flow") },`
  `{ MODKEY|ShiftMask, XK_c, spawn, SHCMD("dwm-flow") },`
- Set `Super + D` cleanly to `dmenu-desktop`:
  `{ MODKEY, XK_d, spawn, SHCMD("dmenu-desktop") },`
- Move `incnmaster -1` from `Super + D` to `Super + Shift + I`:
  `{ MODKEY|ShiftMask, XK_i, incnmaster, {.i = -1 } },`
- Wire `Super + Tab` and `Super + Shift + Tab` to `cyclelayout`:
  `{ MODKEY, XK_Tab, cyclelayout, {.i = +1 } },`
  `{ MODKEY|ShiftMask, XK_Tab, cyclelayout, {.i = -1 } },`
- Fix `Super + M` to `&layouts[1]` (Monocle):
  `{ MODKEY, XK_m, setlayout, {.v = &layouts[1]} },`
- Fix `Button3` click on layout symbol:
  `{ ClkLtSymbol, 0, Button3, setlayout, {.v = &layouts[1]} },`
- Add vanity gap bindings matching README with 5px steps:
  - `{ MODKEY|Mod1Mask, XK_0, togglegaps, {0} },`
  - `{ MODKEY|Mod1Mask, XK_equal, incrgaps, {.i = +5} },`
  - `{ MODKEY|Mod1Mask, XK_minus, incrgaps, {.i = -5} },`
  - `{ MODKEY|Mod1Mask|ShiftMask, XK_equal, defaultgaps, {0} },`

---

### Component C: Runtime Hotkeys Configuration
#### [MODIFY] [config/hotkeys.toml](file:///home/rand/dwm-oomaya/config/hotkeys.toml) & `~/.config/dwm-oomaya/hotkeys.toml`
- Reflect exact keybindings:
  - `key="a"`, `func="spawn"`, `cmd="antigravity"`, `desc="Antigravity IDE"`
  - `key="c"`, `func="spawn"`, `cmd="dwm-flow"`, `desc="Sovereign Flow Cockpit (agy + nvim)"`
  - `mod="SUPER SHIFT"`, `key="c"`, `func="spawn"`, `cmd="dwm-flow"`, `desc="Sovereign Flow Cockpit (alias)"`
  - `key="d"`, `func="spawn"`, `cmd="dmenu-desktop"`, `desc="App Launcher"`
  - `mod="SUPER SHIFT"`, `key="i"`, `func="incnmaster"`, `i=-1`, `desc="Remove from master"`
  - `key="Tab"`, `func="cyclelayout"`, `i=1`, `desc="Cycle layouts forward"`
  - `mod="SUPER SHIFT"`, `key="Tab"`, `func="cyclelayout"`, `i=-1`, `desc="Cycle layouts reverse"`
  - `key="m"`, `func="setlayout"`, `layout_idx=1`, `desc="Monocle layout"`
  - Add `SUPER ALT` gap controls with `i=5` / `i=-5`.

---

### Component D: README Overhaul
#### [MODIFY] [README.md](file:///home/rand/dwm-oomaya/README.md)
- Fully update Keyboard Mastery tables (Launchers, Window Management, Layouts & Gaps).
- Overhaul **2. Build & Install**:
  - Clear, step-by-step instructions for `make -j$(nproc) dwm`.
  - Binary deployment explaining user installation (`install-local` / `install -Dm755 dwm ~/.local/bin/dwm`) and system-wide installation (`sudo make install-system`).
  - Explain dual-path parity for display managers (LightDM/SDDM) and user shells.
- Overhaul **3. Install the Dmenu Ecosystem**:
  - Streamlined `dmenu-oomaya` build & install (`sudo make install` to `/usr/local/bin` and user sync to `~/.local/bin`).
  - Highlight the 7 dmenu scripts and launcher tools.

---

## 4. Verification Plan

### Automated Compilation & Tests
1. **Compilation Gate**:
   ```bash
   cd /home/rand/dwm-oomaya
   make clean
   rm -f config.h && cp config.def.h config.h
   make -j$(nproc) dwm
   ```
2. **Build Configuration Generation Test**:
   ```bash
   tests/test-configure-build.sh
   ```
3. **Headless Xvfb Runtime Test**:
   ```bash
   tests/test-xvfb-runtime.sh
   ```

### Sandboxed Build & Install Testing
1. Create temporary sandbox `SANDBOX=$(mktemp -d)`.
2. Test `DESTDIR=$SANDBOX make install-system` in `dwm-oomaya`.
3. Test `USER_HOME=$SANDBOX/home make install-user` in `dwm-oomaya`.
4. Test `DESTDIR=$SANDBOX make install` in `dmenu-oomaya`.
5. Verify checksum parity and binary file permissions across all staging trees.

### Live Session Validation
1. Reload runtime TOML configuration via `kill -USR1 $(pidof dwm)`.
2. Verify via `xdotool` / `xprop`:
   - `Super + M` sets layout to `[M]`.
   - `Super + T` sets layout to `[]=`.
   - `Super + Tab` cycles through layouts.
   - `Super + Alt + =` / `-` adjusts gaps.
