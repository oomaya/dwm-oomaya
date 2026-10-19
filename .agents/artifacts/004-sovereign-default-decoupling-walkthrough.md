# Walkthrough: Packet #004 — Sovereign Default Decoupling

## Overview

Packet #004 executes the sovereign default decoupling milestone in `dwm-oomaya` under Phase 2 Option B (Sovereign Default & Modular Decoupling). dwm-oomaya now boots directly into Pure Suckless C mode (native `status2d` bar + `dwm-status` + `dunst` + `dwm-controlcenter` + `dwm-settings-hub`) with zero Quickshell or Qt6 dependencies, while providing a seamless bidirectional switcher (`dwm-shell-mode`) for users who choose to run Quickshell.

---

## Changes Implemented

### 1. `scripts/dwm-shell-mode` (New Desktop Shell Switcher)
- Created a standalone POSIX utility to inspect, persist, and toggle desktop shell modes (`suckless` vs `quickshell`).
- Supports subcommands:
  - `suckless`: Persists `suckless` mode to `${XDG_CONFIG_HOME:-$HOME/.config}/dwm-oomaya/shell-mode`, stops Quickshell cleanly (dwm restores native C `status2d` bar), and ensures `dunst` and `dwm-status` are active.
  - `quickshell`: Verifies Quickshell availability and config, persists `quickshell`, stops `dunst`, and launches Quickshell (dwm automatically unmaps native bar).
  - `toggle`: Toggles between the two modes seamlessly.
  - `current`: Prints `suckless` or `quickshell`.
  - `status`: Displays configured mode and live process states (`quickshell`, `dunst`, `dwm-status`).
  - `--list` / `--dry-run`: Headless inspection and test flags.
- Interactive dmenu mode when invoked without arguments.

### 2. `scripts/autostart.sh` (Sovereign Default Autostart)
- Reads configured shell mode from `shell-mode` config file or `DWM_SHELL_MODE` environment variable.
- Defaults to `suckless`.
- In `suckless` mode, Quickshell is skipped entirely and `dunst` is activated for Tokyo Night notifications.
- `dwm-status` continues publishing root-window status badges for native `status2d` bar.

### 3. `scripts/dwm-settings-hub` (Shell Mode Menu Integration)
- Added `🖥️  Desktop Shell Mode` to `get_menu_options()`.
- Dispatches to `dwm-shell-mode` to allow changing shell mode directly from the native settings hub.

### 4. `config/hotkeys.toml` & `config.def.h` (Native Keybinding Decoupling)
- Updated `Super + F1` to invoke `dwm-controlcenter` (which forwards to Quickshell IPC if Quickshell is running, or native dmenu if Quickshell is offline or `--native`).
- Updated `Super + slash` to invoke `dwm-keybinds` directly.
- Added `Super + Shift + S` for `dwm-settings-hub`.
- Added `Super + Ctrl + R` for `dwm-shell-mode toggle`.

### 5. `Makefile` Integration & Testing
- Added `scripts/dwm-shell-mode` to `INSTALL_COMMANDS`, `check-shell`, and `check-format`.
- Added `tests/test-dwm-shell-mode.sh` (8 unit tests) and wired into `check-settings:`.
- Updated `tests/test-autostart.sh` to include `run_suckless_default_case()`, proving Quickshell is not started in default mode while keeping regression coverage intact.

---

## Verification Results

| Test Suite / Target | Result | Details |
| :--- | :---: | :--- |
| `tests/test-dwm-shell-mode.sh` | **PASS** | 8/8 unit tests passed |
| `tests/test-dwm-controlcenter.sh` | **PASS** | 5/5 unit tests passed |
| `tests/test-autostart.sh` | **PASS** | Default suckless + quickshell lifecycle passed |
| `tests/test-notification-fallback.sh` | **PASS** | Dunst fallback passed |
| `tests/test-suckless-bar-fallback.sh` | **PASS** | 8/8 invariant tests passed |
| `make check-settings` | **PASS** | 13 test suites passed |
| `make check-shell` | **PASS** | 0 ShellCheck warnings |
| `make check-format` (shfmt) | **PASS** | 0 formatting mismatches |
| `make check-install-manifest` | **PASS** | 73 utility scripts installed |
| `make check-core` | **PASS** | Core C builds and unit tests passed |
| `scripts/validate-hotkeys` | **PASS** | 46/46 keybindings valid, 0 TUI traps |
