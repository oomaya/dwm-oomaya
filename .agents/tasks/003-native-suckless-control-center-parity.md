# Task Packet: 003-native-suckless-control-center-parity

*Standard task packet according to the architect task-packet-template. Stored in `.agents/tasks/`.*

---

## Packet 003 — Native Suckless Control Center & Settings Parity

**Status:** `active`  
**Branch:** `packet/003-native-suckless-control-center-parity`  
**Author (architect):** Tech Lead & Ted  
**Date:** 2026-10-19  

### Objective

Elevate native Suckless POSIX/dmenu desktop tools (`dwm-controlcenter`, `dwm-settings-hub`, `dwm-settings`) to 100% standalone feature parity so that `dwm-oomaya` operates completely autonomously with rich functionality even when Quickshell is not installed or running.

### Background

Phase 2 architectural audit revealed that Quickshell accounts for ~40,000 lines of code (84% of the repository) and 85% of CI turnaround time. The user selected **Option B (Sovereign Default & Modular Decoupling)**. The first required milestone is ensuring that the native Suckless C/POSIX tools lack zero features compared to Quickshell before decoupling defaults in Packet #004.

### Repo context

- `scripts/dwm-controlcenter`: Currently runs `quickshell ipc ...` without standalone fallback.
- `scripts/dwm-settings-hub`: Native dmenu settings hub; has a hardcoded path (`/home/rand` at line 48) and lacks audio sink selection and package upgrade actions.
- `scripts/dwm-settings`: CLI dispatcher between Quickshell and `dwm-settings-hub`.
- `tests/test-settings.sh`: Regression tests for settings dispatching and menu options.

### Scope

1. **`scripts/dwm-controlcenter`**:
   - Add graceful fallback: If Quickshell is running, call `quickshell ipc ...` (maintaining 100% backward compatibility).
   - If Quickshell is offline (or when invoked with `--native` / `--list`), display a sleek, native `dmenu` Control Center popup:
     - Wi-Fi Quick Toggles / Connect (via `nmcli dev wifi`)
     - Bluetooth Power / Connect (via `bluetoothctl`)
     - Volume / Audio Output Sink (via `wpctl` / `pactl`)
     - Brightness Level (via `brightnessctl`)
     - Night Light Toggle (via `gammastep` / `redshift`)
     - Power Profile (Performance / Balanced / Saver via `powerprofilesctl`)
     - Link to `dwm-settings-hub`
   - Support non-interactive `--list` and `--dry-run` modes for headless automated testing.

2. **`scripts/dwm-settings-hub`**:
   - Fix hardcoded `/home/rand` literal on line 48 per Rule 3 (Zero Hardcoded Literals Law); resolve wallpaper assets dynamically via `${XDG_DATA_HOME:-$HOME/.local/share}/dwm-oomaya/assets/`, `/usr/share/backgrounds/`, or relative repo paths.
   - Add "🔊 Audio & Sound Devices" sub-menu allowing default audio output sink switching (`wpctl status` / `pactl list short sinks`).
   - Add "📦 Check & Apply System Updates" under Diagnostics launching `dwm-terminal` with package manager upgrade (`dnf upgrade` / `pacman -Syu`).

3. **`scripts/dwm-settings`**:
   - Fall back to `dwm-settings-hub` gracefully when `action` is `toggle` or `open` and Quickshell is offline.

4. **Regression & Unit Testing**:
   - Add comprehensive tests in `tests/test-dwm-controlcenter.sh` validating `--list`, `--dry-run`, and fallback execution under mock environments.
   - Run `make check-shell`, `make check-core`, `make check-shell-appearance`, and `make check-system` to guarantee 100% Green CI.

### Non-goals

- Do NOT delete Quickshell QML files in this packet (that occurs in Packet #005 or downstream).
- Do NOT alter native dwm C window manager core sources (`dwm.c`, `drw.c`).
- Do NOT introduce heavy Python or Node.js runtimes for menu helpers (strictly POSIX `sh` / Bash per Rule 3).

### Acceptance criteria

1. Zero `/home/*` hardcoded literals across all scripts in `scripts/`.
2. `dwm-controlcenter` launches native dmenu popup when Quickshell is offline without throwing IPC connection errors.
3. `dwm-controlcenter --list` and `dwm-settings-hub --list` return valid menus in non-interactive environments.
4. `dwm-settings toggle` opens `dwm-settings-hub` when Quickshell is offline.
5. `make check-shell` passes with 0 ShellCheck warnings.
6. All test suites pass 100% Green.
