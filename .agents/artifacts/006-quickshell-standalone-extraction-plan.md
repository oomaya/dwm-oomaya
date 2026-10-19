# Implementation Plan: Packet #006 - Quickshell Standalone Extraction & Radical Codebase Diet

## Quota & Token Budget Analysis (Go/No-Go Decision Gate)

| Metric | Current Status | Safe Threshold | Decision Gate |
| :--- | :--- | :--- | :--- |
| **Gemini Weekly Quota** | **63.1% Remaining** (36.9% used) | > 15.0% | **GO (Green)** |
| **Gemini 5-Hour Quota** | **89.2% Remaining** (10.8% used) | > 20.0% | **GO (Green)** |
| **Claude Weekly Quota** | **100.0% Remaining** (0.0% used) | > 15.0% | **GO (Green)** |
| **Projected Phase Tokens** | ~25,000 – 35,000 tokens | Low footprint | **GO (Green)** |
| **Model Selection** | Gemini 2.5 Flash / Pair | Flow Preservation | Gemini plans & executes with direct verification |

**Decision**: Quotas are clear across all windows. Proceed directly with execution.

---

## 1. Problem Statement & Architecture Goals
Following Phase 2 (Option B: Sovereign Default & Modular Decoupling), `dwm-oomaya` boots into pure ChadDWM C mode by default. However, the repository still contains:
- `config/quickshell/` (99 files, ~1.8 MB, 39,782 lines of Qt6/QML code)
- 8 Quickshell helper scripts (`scripts/dwm-quickshell-*`, `scripts/quickshell-qmllint`)
- 1 Quickshell C IPC bridge (`tools/dwm-quickshell-state.c`)
- 26 Quickshell test scripts (`tests/test-quickshell*.sh`)

This bloats the repository size, adds dead weight to build targets, and complicates packaging for minimal systems.

### Goals of Packet #006 (Track 1)
1. **Preserve Complete Quickshell Ecosystem**: Extract all Quickshell components cleanly into a dedicated companion repository: `oomaya/quickshell-oomaya` (with its own standalone Makefile, tests, and installer).
2. **Radical Diet for `dwm-oomaya`**: Purge all 39,782 lines of QML and 26 test scripts from `dwm-oomaya`.
3. **Clean Decoupling in Core**:
   - `scripts/dwm-shell-mode quickshell` dynamically detects if `~/.config/quickshell` is installed; if not, offers to clone and install `oomaya/quickshell-oomaya`.
   - `Makefile` removes all Quickshell-specific build, install, and check targets.
   - `.github/workflows/c-cpp.yml` drops all heavy QML workflow triggers, leaving only fast C validation and clang builds (< 2 minutes).
4. **100% Green CI**: All tests pass cleanly without broken dependencies.

---

## 2. Proposed Changes

### Component 1: Standalone `oomaya/quickshell-oomaya` Companion Repository
- Location: `$HOME/.local/src/quickshell-oomaya`
- Contents:
  - `config/quickshell/` contents moved to root or `shell/`
  - `scripts/dwm-quickshell-*` moved to `bin/` or `scripts/`
  - `tools/dwm-quickshell-state.c` moved to `tools/`
  - `tests/test-quickshell*.sh` moved to `tests/`
  - Standalone `Makefile` (`make install`, `make check`, `make check-xvfb`)
  - `README.md` explaining how to install as an optional addon for `dwm-oomaya`.
- GitHub: Initialize repository and publish to `https://github.com/oomaya/quickshell-oomaya`.

### Component 2: `dwm-oomaya` Codebase Diet
- **File Deletions**:
  - `rm -rf config/quickshell/`
  - `rm -f scripts/dwm-quickshell-* scripts/quickshell-qmllint`
  - `rm -f tools/dwm-quickshell-state.c`
  - `rm -f tests/test-quickshell*.sh`
- **File Modifications**:
  - `Makefile`:
    - Purge `scripts/dwm-quickshell-*` from `INSTALL_COMMANDS`.
    - Purge `tools/dwm-quickshell-state-bin` compilation, installation, and clean targets.
    - Purge `install-quickshell` from `install-user` and `install`.
    - Purge `check-quickshell*` targets and remove them from `.PHONY`.
    - Update `check-shell` and `check-format` lists.
  - `scripts/dwm-shell-mode`:
    - Keep `suckless` as sovereign default.
    - If user runs `dwm-shell-mode quickshell`, check for `~/.config/quickshell/shell.qml` or `quickshell` command.
    - If absent, display an informative message/dialog explaining how to install `oomaya/quickshell-oomaya`.
  - `.github/workflows/c-cpp.yml`:
    - Remove the optional `validate-quickshell` job and heavy workflow dispatch.
    - Keep lean parallel matrix: `validate-core` and `clang-build`.
  - `tests/test-dwm-shell-mode.sh`:
    - Ensure tests assert the graceful missing-plugin fallback behavior.

---

## 3. Verification Plan

1. **Compilation & Local Checks**:
   ```bash
   make clean all
   make check
   ```
2. **Shell Script Validation**:
   ```bash
   make check-shell check-format
   ```
3. **DWM Shell Mode Test**:
   ```bash
   tests/test-dwm-shell-mode.sh
   tests/test-autostart.sh
   ```
4. **Git Verification**:
   - `git diff --stat` confirms ~40,000 lines removed.
   - Zero broken paths or references in `grep -rn "config/quickshell" .`.
5. **CI Workflow Verification**:
   - Push to `packet/006-quickshell-standalone-extraction`.
   - Open PR, watch GitHub Actions run to 100% green in < 2 minutes.
