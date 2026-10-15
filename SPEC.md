# SPEC.md: DWM-Oomaya System Specification

## 1. Product Vision & Architectural Identity

DWM-Oomaya is a sovereign, high-performance X11 desktop environment engineered around a heavily patched suckless dwm core, paired with an adaptive dual-shell architecture (managed Quickshell or ultra-lean ChadDWM C-status bar) and the brutalist, sharp-corner design grammar of Omarchy / OMACOM.

The environment guarantees 100% native desktop parity across Fedora, Arch Linux, and Debian—deployable onto minimal server netinstalls, bare-metal workstations, or layered cleanly over existing desktop environments without display-manager collision.

---

## 2. Platform Support Matrix & Tier Contracts

The platform contract dynamically resolves target environments via `/etc/os-release` and native package discovery (`dnf`, `pacman`, `apt`). Systems must never be rejected with hard fatal aborts if a functional C99 toolchain and X11 development headers exist.

| Tier | Distributions | Target Scenarios | Supported Shell Stacks | Core Validation Requirement |
| :--- | :--- | :--- | :--- | :--- |
| **Tier 1 (Flagship)** | **Fedora**, **Arch Linux** | Modern kernels ($\ge$ 6.x), bleeding-edge Qt6/QML toolchains, full compositor acceleration | Quickshell (Primary), ChadDWM Bar (Fallback) | Automated headless container + live nested X11 session |
| **Tier 2 (Sovereign Lean)** | **Debian** (Stable/Testing), **Ubuntu** (LTS) | Production server netinstalls, conservative toolchains, strict minimal dependency baselines | ChadDWM Bar (Primary), Quickshell (Conditional) | Clean netinst build gate without Qt6/QML bloat |
| **Tier 3 (Fleet / Embedded)** | **MX Linux**, Void, Generic Minimal X11 | Low-spec nodes, legacy kernels (< 5.15), systems lacking Qt6/QML runtime | ChadDWM Bar (Strict) | Zero-QML suckless C compilation and sub-2ms status loop |

### Invariants:
1. Package name definitions must remain consolidated in `scripts/check-deps.sh` and `install.sh`. Never scatter ad-hoc package lists into individual utility scripts.
2. The installation scripts must respect `$CC`, `$CFLAGS`, `$PREFIX`, `$DESTDIR`, `$XDG_CONFIG_HOME`, and dynamic `$HOME` resolution. Hardcoded paths (such as `/usr/X11R6` or literal `/home/*`) are strictly forbidden.

---

## 3. Adaptive Dual-Shell Architecture (Quickshell vs. ChadDWM Bar)

The environment strictly decouples the C window manager event loop from user-facing status surfaces and launchers. The runtime session auto-negotiates the active shell profile at startup without requiring re-compilation of the `dwm` binary.

```text
                    ┌───────────────────────────────┐
                    │  dwm core (C99, ~150KB, IPC)  │
                    └───────────────┬───────────────┘
                                    │
               ┌────────────────────┴────────────────────┐
               ▼                                         ▼
   [Profile: Modern / Tier 1]                [Profile: Lean / Legacy / Tier 2/3]
   Quickshell Managed Shell                  ChadDWM Status2d Engine
   - Qt6 / QML Event-Driven Engine           - Microsecond POSIX status emitter
   - Control Center & OSD                    - Sub-2ms C status2d rendering
   - IPC: Atom `_DWM_CURRENT_LAYOUT`         - Zero Qt/C++ runtime dependency
```

### 3.1 Shell Capability Detection Contract
Before initializing the visual status bar, session startup (`scripts/dwm-session` or `autostart.sh`) must execute the shell probe protocol:

1. **Qt6 / Quickshell Runtime Verification**:
   - Check binary availability: `command -v quickshell >/dev/null 2>&1`
   - Check dynamic linkage: Execute non-interactive version probe (`quickshell --version >/dev/null 2>&1`).
   - Check driver compatibility: Verify that running hardware/virtualization drivers do not trip known QML scene-graph aborts.
2. **Deterministic Resolution**:
   - **Modern Profile**: If the probe passes and `DWM_FORCE_LEAN_SHELL=1` is not set, launch Quickshell via `dwm-quickshell-controlcenter action restart-quickshell`.
   - **Lean Profile**: If the probe fails or `DWM_FORCE_LEAN_SHELL=1` is asserted, fallback cleanly to the ChadDWM status2d bar (`scripts/dwm-bar.sh`).
   - Record resolved shell tier into `${XDG_STATE_HOME:-$HOME/.local/state}/dwm/session.log`.

### 3.2 Shell Resilience Invariants
- **Zero Shell Deadlock**: A crash or absence of Quickshell must never prevent `dwm` from launching core client terminals (<kbd>Super</kbd>+<kbd>Shift</kbd>+<kbd>Enter</kbd>) or `dmenu-oomaya` launchers (<kbd>Super</kbd>+<kbd>D</kbd>).
- **IPC Atom Compliance**: The C window manager core must unconditionally broadcast layout changes and workspace state via X11 atoms (`_DWM_CURRENT_LAYOUT`) regardless of active shell backend.
- **Aesthetic Floor**: Both shell implementations must adhere strictly to the **Tokyo Night Sharp-Corner Aesthetic Law** (2px square borders, `#7aa2f7` primary blue, `#24283b` background, 16pt font typography floor).

---

## 4. Window Manager Core & Runtime Configuration

### 4.1 C Window Manager Core
- Core executable remains pure C99 compiled with size/speed optimization (`-Os`).
- Keep core binary small (~150KB) and dependency-light (Xlib, Xft, Xinerama).
- Never introduce blocking operations, long timers, or external system daemon calls into the X event loop.

### 4.2 Seamless In-Place Self-Restart
- <kbd>Super</kbd>+<kbd>Shift</kbd>+<kbd>R</kbd> executes an atomic in-place restart via `execvp("dwm", argv)`.
- dwm unmanages clients, closes display sockets cleanly, and replaces its executable in memory under the exact same PID without dropping existing X client windows or killing running applications.

### 4.3 Runtime TOML Configuration
Personal settings live in standard XDG paths (`${XDG_CONFIG_HOME:-$HOME/.config}/dwm-oomaya/`):
- `hotkeys.toml`: Keybindings and launcher assignments (hot-reloaded on file modification).
- `themes.toml`: Tokyo Night color roles, border widths, and layout gaps.
- `window-rules.toml`: Application workspace tagging, floating rules, and monitor routing.
- The installation/update script must never overwrite existing user TOML files without generating a timestamped backup.

---

## 5. Layout Engine (14 Vanitygaps Matrix)

The window manager core integrates 14 dynamic tiling layouts with pixel-perfect inner and outer gaps (`gappih`, `gappiv`, `gappoh`, `gappov`):
- Standard: Tile (`[]=`), Monocle (`[M]`), Floating (`><>`).
- Fibonacci: Spiral (`[@]`), Dwindle (`[\\]`).
- Stacks & Grids: Deck (`H[]`), Bottomstack (`TTT`), Bottomstack Horiz (`===`), Grid (`HHH`), Nrowgrid (`###`), Horizgrid (`---`), Gaplessgrid (`:::`).
- Master Centric: Centered Master (`|M|`), Centered Floating Master (`>M>`).

The active layout symbol must be mirrored directly to the root window atom `_DWM_CURRENT_LAYOUT` on every state change.

---

## 6. Centered Dmenu & Helper Suite

The window manager pairs with `dmenu-oomaya` located in canonical source paths (`~/.local/src/dmenu-oomaya/` or system `/usr/local/bin/dmenu`):
- All interactive menus (`dmenu-desktop`, `dmenu-windows`, `dmenu-power`, `dmenu-run`, `dmenu-scrot`, `dmenu-hub`) run centered with 2px Tokyo Night borders and a 16pt typography baseline.
- `dmenu-windows` integrates with EWMH `_NET_ACTIVE_WINDOW` to smoothly raise, unhide, and warp focus to target windows across workspaces.

---

## 7. Installation, Display Manager & System Invariants

1. **Display Manager Immunity**: Default installation must never clobber, disable, or force reconfiguration of active display managers (`sddm`, `gdm`, `greetd`, `ly`, `lxdm`, `lightdm`). The dwm session is exposed cleanly via `/usr/share/xsessions/dwm.desktop`.
2. **Minimal Seeding**: Unconditionally ensure target asset directories exist:
   - `$HOME/Pictures/Screenshots`
   - `$HOME/Pictures/backgrounds`
   - `$HOME/.local/share/fonts` (JetBrains Mono & Meslo Nerd Font)
3. **Idempotency**: Running `install.sh` multiple times must produce identical state without appending duplicate lines or corrupting existing system configs.
4. **Elevation Boundaries**: Privilege escalation (`sudo`) is restricted strictly to package installation and system binary placement. Never run the entire installer as root.

---

## 8. Shell and Code Hygiene Standards

- Scripts: Use Bash with `set -euo pipefail` only when Bash-specific arrays/syntax are required; otherwise use strict POSIX `sh` (`set -eu`).
- Linting Gates:
  ```sh
  shellcheck install.sh bootstrap.sh scripts/*.sh
  shfmt -d install.sh bootstrap.sh scripts/*.sh
  ```
- Validation Gate:
  ```sh
  make clean all
  scripts/run-tests
  ```
- Any Quickshell QML modifications must be linted via `qmllint` with explicit Qt import paths before declaring tasks ready for review.
