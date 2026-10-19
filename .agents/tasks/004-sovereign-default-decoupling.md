# Task Packet: 004-sovereign-default-decoupling

*Standard task packet according to the architect task-packet-template. Stored in `.agents/tasks/`.*

---

## Packet 004 — Sovereign Default Decoupling

**Status:** `active`  
**Branch:** `packet/004-sovereign-default-decoupling`  
**Author (architect):** Tech Lead & Ted  
**Date:** 2026-10-19  

### Objective

Decouple dwm-oomaya from its previous hardcoded assumption that Quickshell must always run. Establish Pure Suckless C mode (native `status2d` bar + `dwm-status` + `dunst` + `dwm-controlcenter` + `dwm-settings-hub`) as the sovereign default out-of-the-box experience, while providing a seamless bidirectional toggle (`dwm-shell-mode`) for users who wish to run Quickshell.

### Background

Under Phase 2 Option B (Sovereign Default & Modular Decoupling), Packet #003 achieved 100% native feature parity for the control center and settings hub. With native tools now having full feature parity, Packet #004 changes the runtime autostart and keybinding dispatching so that:
1. Pure Suckless C mode boots by default without spawning Qt6/QML processes.
2. Users have zero Qt6 build or runtime dependencies for a complete desktop workflow.
3. Quickshell is preserved as an optional, non-blocking modular shell that can be enabled or toggled on demand via `dwm-shell-mode`.

### Repo context

- `scripts/autostart.sh`: Currently starts Quickshell unconditionally whenever `shell.qml` exists.
- `scripts/dwm-shell-mode`: New utility to inspect, persist, and toggle between `suckless` and `quickshell` shell modes.
- `scripts/dwm-settings-hub`: Dmenu settings hub; will expose the shell mode switcher.
- `config/hotkeys.toml`: Keybindings palette; will use `dwm-controlcenter` and `dwm-keybinds` natively, and bind `Super + Shift + S` to `dwm-settings-hub`.
- `Makefile`: Registers `dwm-shell-mode` in `INSTALL_COMMANDS` and validation targets.
- `tests/test-dwm-shell-mode.sh`: Unit and integration test suite for shell mode switching.

### Scope

1. **`scripts/dwm-shell-mode`**:
   - Commands: `suckless`, `quickshell`, `toggle`, `status`, `current`, `--list`, `--dry-run`.
   - Persists selection to `${XDG_CONFIG_HOME:-$HOME/.config}/dwm-oomaya/shell-mode`.
   - Default is `suckless`.
   - When switching to `suckless`:
     - Stops Quickshell cleanly (`pkill -x quickshell`).
     - Ensures `dunst` notification daemon is running.
     - Dwm automatically restores the native `status2d` bar upon altbar unmapping.
   - When switching to `quickshell`:
     - Validates Quickshell binary and config availability.
     - Stops `dunst` to avoid notification conflicts.
     - Launches Quickshell in background (`quickshell --path ...`).
     - Dwm automatically unmaps native bar when Quickshell docks.
   - Supports `DWM_SHELL_MODE` environment variable override.

2. **`scripts/autostart.sh`**:
   - Reads configured shell mode from `${XDG_CONFIG_HOME:-$HOME/.config}/dwm-oomaya/shell-mode` (defaulting to `suckless`).
   - If mode is `suckless`, skips Quickshell startup entirely and activates `dunst` notification daemon.
   - If mode is `quickshell` and `shell.qml` exists, launches Quickshell as before.
   - `dwm-status` continues publishing root-window status badges for native `status2d` bar.

3. **`scripts/dwm-settings-hub`**:
   - Adds `🖥️  Desktop Shell Mode [Suckless C / Quickshell]` option allowing instant switching from the dmenu GUI.

4. **`config/hotkeys.toml`**:
   - Updates `Super + F1` to invoke `dwm-controlcenter`.
   - Updates `Super + slash` to invoke `dwm-keybinds`.
   - Adds `Super + Shift + S` for `dwm-settings-hub`.

5. **`Makefile` & Testing**:
   - Wire `dwm-shell-mode` into `INSTALL_COMMANDS`, `check-shell`, and `check-format`.
   - Add unit test `tests/test-dwm-shell-mode.sh` and wire into `check-settings:`.
   - Ensure `tests/test-autostart.sh` passes 100%.

### Non-goals

- Do NOT delete Quickshell QML source trees (Quickshell is preserved as an optional module).
- Do NOT alter native dwm C event loop or introduce external binding daemons.
- Strictly adhere to POSIX `sh` / Bash without heavy Python/Node runtimes.
