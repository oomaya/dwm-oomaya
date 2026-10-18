# Packet #003 Walkthrough: Native Suckless Control Center & Settings Parity

## Executive Summary

- **Task Packet**: Packet #003 (`.agents/tasks/003-native-suckless-control-center-parity.md`)
- **Status**: Completed & Verified
- **Branch**: `packet/003-native-suckless-control-center-parity`
- **Scope**: Elevated native Suckless POSIX/dmenu desktop tools (`dwm-controlcenter`, `dwm-settings-hub`, `dwm-settings`) to 100% standalone feature parity with Quickshell, eliminated all hardcoded `/home/*` literals, added audio sink switching and system update triggers, and added unit tests in `tests/test-dwm-controlcenter.sh`.

---

## 1. Key Implementation Details

### 1.1 Native Suckless Control Center (`scripts/dwm-controlcenter`)
Previously, `dwm-controlcenter` was an 11-line stub that unconditionally attempted `quickshell ipc ...`, failing immediately if Quickshell was not installed or running.

It has been upgraded to a resilient **Hybrid Control Center**:
1. **Introspection & Testing Flags**:
   - `--list`: Prints all control center menu options with live status badges and exits 0.
   - `--dry-run`: Exits 0 in non-interactive / headless test environments.
   - `--native`: Force-invokes the native `dmenu` interface even if Quickshell is running.
2. **Quickshell Backward Compatibility**:
   - If Quickshell is active and `--native` is omitted, it seamlessly proxies to `quickshell ipc --path "$config" call controlcenter toggle`.
3. **Pure Suckless POSIX Fallback**:
   - When Quickshell is offline, launches an interactive `dmenu` popup:
     - 📶 **Wi-Fi**: Shows active SSID; opens network picker, toggle on/off, or rescan via `nmcli`.
     - 󰂯 **Bluetooth**: Shows power state / connected device; toggles power or connects via `bluetoothctl`.
     - 🔊 **Volume**: Shows current percentage or `[MUTED]`; allows volume stepping, mute toggle, sink switching, or mixer launch.
     - 󰃠 **Brightness**: Shows current percentage; allows setting brightness levels via `brightnessctl`.
     - ⚡ **Power Profile**: Shows active profile; allows selecting `performance`, `balanced`, or `power-saver` via `powerprofilesctl`.
     - 󰏘 **Appearance & Wallpapers**: Quick jump into `dwm-settings-hub`.
     - ⚙ **Full Settings Hub**: Launches `dwm-settings-hub`.
4. **Sub-second Concurrency Guards**:
   - Wrapped external inspection calls (`bluetoothctl`, `nmcli`, `wpctl`, `brightnessctl`, `powerprofilesctl`) in `timeout 0.5s` to prevent system daemon lockups in headless VMs.

### 1.2 Settings Hub Parity & Zero Hardcoded Literals (`scripts/dwm-settings-hub`)
1. **Rule 3 Compliance (Zero Hardcoded Literals Law)**:
   - Replaced hardcoded `/home/rand` wallpaper copy fallback with dynamic asset resolution searching `${XDG_DATA_HOME:-$HOME/.local/share}/dwm-oomaya/assets/`, `/usr/share/backgrounds/`, and relative repo paths.
2. **Audio & Sound Devices Sub-menu**:
   - Added `🔊  Audio & Sound Devices` option to `get_menu_options()`:
     - Volume Up (+5%), Volume Down (-5%), Toggle Mute.
     - Switch Audio Output Sink: dynamically lists active sinks via `pactl list short sinks` / `wpctl status` and sets the default sink.
     - Open Audio Mixer: launches `pavucontrol` or terminal `alsamixer`.
3. **System Update Action**:
   - Added `📦  Check & Apply System Updates` under `󰍛  System Diagnostics`:
     - Spawns `dwm-terminal` with distribution-native package upgrades (`dnf upgrade`, `pacman -Syu`, or `apt upgrade`).

### 1.3 Settings Dispatcher (`scripts/dwm-settings`)
- Added `--list` and `--dry-run` passthrough to `dwm-settings-hub`.
- Reordered helper resolution to prioritize repository siblings during development before checking installed `~/.local/bin/`.

### 1.4 Test Infrastructure & Race Hardening
1. **New Test Suite (`tests/test-dwm-controlcenter.sh`)**:
   - Validates `--list` output.
   - Validates `--dry-run` exit code 0.
   - Validates Quickshell IPC forwarding using mock binaries.
   - Validates native fallback when Quickshell is offline.
   - Validates `--native` flag bypass.
   - Integrated into `Makefile` target `check-settings:`.
2. **Harden `test-dwm-status.sh`**:
   - Initialized `: >"$work/status.log"` before daemon spawn and relaxed status assertion poll loops from 10 to 30 iterations (1.5s) to eliminate false-positive grep errors under heavy parallel CPU load.

---

## 2. Verification Results Matrix

| Test Suite / Target | Command | Result | Details |
| :--- | :--- | :---: | :--- |
| **Control Center Tests** | `./tests/test-dwm-controlcenter.sh` | **PASS** | All 5 unit tests passed |
| **Status Bar Fallback** | `./tests/test-suckless-bar-fallback.sh` | **PASS** | 8 native bar fallback invariants verified |
| **Hardcoded Literals Check**| `grep -rn "/home/" scripts/` | **PASS** | 0 occurrences across all scripts |
| **Settings Suite** | `make check-settings` | **PASS** | Settings provider, input discovery & control center passed |
| **ShellCheck** | `make check-shell` | **PASS** | 0 warnings across all repository scripts |
| **Install Manifest** | `make check-install-manifest check-install-preservation` | **PASS** | 100% manifest consistency & 72 utility scripts installed |
| **Core C & Desktop** | `make check-core` | **PASS** | dwm, dwm-oomayad, hotkeys, status bar, and Xvfb verified |

---

## 3. Next Steps (Packet #004)

With 100% feature parity achieved in native Suckless POSIX/dmenu tools, dwm-oomaya is ready for **Packet #004: Sovereign Default Decoupling**:
1. Configure `scripts/dwm-autostart` and XDG autostart to run pure Suckless C mode (`dwm-status` + native bar) as the primary default desktop.
2. Provide a clean `dwm-shell-mode [suckless|quickshell]` toggle utility.
3. Update keybindings in `config/hotkeys.toml` so shortcuts invoke native tools directly without Quickshell dependency.
