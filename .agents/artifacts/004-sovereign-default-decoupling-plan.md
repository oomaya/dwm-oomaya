# Implementation Plan: Packet #004 — Sovereign Default Decoupling

## Executive Summary

Packet #004 executes the core decoupling of `dwm-oomaya` under Phase 2 Option B (Sovereign Default & Modular Decoupling). With native Suckless tools (`dwm-controlcenter`, `dwm-settings-hub`, `dwm-settings`) elevated to 100% feature parity in Packet #003, Packet #004 decouples the session initialization and keybindings so that Pure Suckless C mode (native `status2d` bar + `dwm-status` + `dunst` + `dwm-controlcenter` + `dwm-settings-hub`) boots directly out of the box with zero Quickshell or Qt6 dependencies, while providing a seamless switcher (`dwm-shell-mode`) for users who want Quickshell.

---

## Quota & Token Budget Analysis (Go/No-Go Decision Gate)

### Current Allowance Snapshot
- **Gemini Weekly Cap**: 66.3% remaining (resets in 5 days, 3 hours)
- **Gemini 5-Hour Session**: 97.8% remaining (resets in 4 hours, 35 minutes)
- **Claude / GPT Weekly Cap**: 100% remaining
- **Claude / GPT 5-Hour Session**: 100% remaining

### Projected Phase Consumption
- **Packet #004 Planning & Execution**: ~8,000–12,000 tokens
- **Testing & Verification**: ~4,000–6,000 tokens
- **Total Projected**: ~12,000–18,000 tokens (<2% of 5-hour quota)

### Go/No-Go Decision
- **Status**: **GO**. Quota allowances are exceptionally healthy across both Gemini and Claude engines. Gemini will lead implementation and testing.

---

## Proposed Changes

### 1. `scripts/dwm-shell-mode` (New Utility)
- Manages and queries desktop shell mode: `suckless` (default) vs `quickshell`.
- Commands:
  - `suckless`: Persists `suckless`, stops Quickshell, ensures `dunst` is running, native `status2d` bar takes over.
  - `quickshell`: Persists `quickshell`, verifies Quickshell availability, stops `dunst`, launches Quickshell.
  - `toggle`: Toggles between the two modes.
  - `status`: Displays current configured mode and active process status.
  - `current`: Prints `suckless` or `quickshell`.
  - `--list` / `--dry-run`: Headless inspection for menus and test automation.
- State file: `${XDG_CONFIG_HOME:-$HOME/.config}/dwm-oomaya/shell-mode`.
- Adheres strictly to Rule 3 (Zero Hardcoded Literals) and POSIX `sh`.

### 2. `scripts/autostart.sh` (Sovereign Startup Decoupling)
- Reconciles shell mode from `shell-mode` file or `DWM_SHELL_MODE` environment variable.
- Defaults to `suckless`.
- If `suckless`:
  - Skips Quickshell startup.
  - Starts `dunst` for Tokyo Night notifications.
  - `dwm-status` runs in background feeding `status2d` on the native C bar.
- If `quickshell`:
  - Starts managed Quickshell if installed and compatible, with graceful fallback.

### 3. `scripts/dwm-settings-hub` (Shell Mode Menu Integration)
- Adds `🖥️  Desktop Shell Mode [Suckless C / Quickshell]` entry.
- Invoking it presents a dmenu picker with the current mode marked with `(Active)`.

### 4. `config/hotkeys.toml` & `config.def.h` (Native Keybinding Decoupling)
- Update `Super + F1` to invoke `dwm-controlcenter` (which already forwards to Quickshell IPC if Quickshell is active, or native dmenu if Quickshell is offline).
- Update `Super + slash` to invoke `dwm-keybinds` directly.
- Add `Super + Shift + S` for `dwm-settings-hub`.

### 5. `Makefile` Integration
- Add `scripts/dwm-shell-mode` to `INSTALL_COMMANDS`, `check-shell`, and `check-format`.
- Add `tests/test-dwm-shell-mode.sh` to `check-settings:`.

### 6. Test Suite (`tests/test-dwm-shell-mode.sh`)
- Validate `--list`, `--dry-run`, `current`, `suckless`, `quickshell`, `toggle`, and `status`.
- Validate process termination and notification daemon launch under mock environments.
- Ensure regression tests (`test-autostart.sh`, `test-settings.sh`, `test-notification-fallback.sh`) pass 100%.

---

## Verification Plan

### Automated Local Verification
1. `./tests/test-dwm-shell-mode.sh`: Verify all 7 shell mode unit tests pass.
2. `./tests/test-autostart.sh`: Verify autostart tests pass.
3. `./tests/test-dwm-controlcenter.sh`: Verify control center tests pass.
4. `make check-shell`: Verify 0 shellcheck warnings.
5. `make check-format`: Verify shfmt formatting passes.
6. `make check-install-manifest check-install-preservation`: Verify install targets.
7. `make check-core`: Verify core tests pass.

### CI Verification
- Push branch `packet/004-sovereign-default-decoupling` and trigger `c-cpp.yml` workflow via GitHub Actions.
- Ensure all 4 matrix jobs are 100% Green before merging to `main`.
