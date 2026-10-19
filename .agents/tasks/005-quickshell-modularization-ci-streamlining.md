# Task Packet: 005-quickshell-modularization-ci-streamlining

*Standard task packet according to the architect task-packet-template. Stored in `.agents/tasks/`.*

---

## Packet 005 — Quickshell Modularization & CI Streamlining

**Status:** `active`  
**Branch:** `packet/005-quickshell-ci-streamlining`  
**Author (architect):** Tech Lead & Ted  
**Date:** 2026-10-19  

### Objective

Decouple optional Quickshell QML GUI tests from the mandatory CI gate, slashing CI turnaround time from ~18 minutes down to ~2.5 minutes. Organize `Makefile` so that `make check` executes the sovereign core Suckless test suite in under 2.5 minutes, while providing `make check-quickshell` and `make check-all` for optional deep QML verification.

### Background

Forensic analysis in Phase 2 demonstrated that Quickshell accounts for 84% of the codebase (39,782 LOC) and over 85% of CI turnaround time (installing >2GB of Qt6/QML RPMs and executing headless Xvfb GUI renders). Now that Packet #003 and Packet #004 have elevated native Suckless C / dmenu tools to 100% parity and established pure Suckless as the sovereign default, mandatory CI no longer needs to wait on Qt6 scene graphs for core commits.

### Repo context

- `.github/workflows/c-cpp.yml`: Matrix currently includes `validate-shell-appearance` (11m) and `validate-system-management` (18m).
- `Makefile`: `check-core` validates core C and desktop scripts. `check:` runs everything sequentially.
- `Makefile`: `check-settings:` validates native settings, control center, and shell mode.

### Scope

1. **`Makefile` Target Streamlining**:
   - Move `$(MAKE) check-settings` into `check-core:` so native desktop settings, control center, and shell-mode tests are always part of core validation.
   - Update `check:` to run `check-core` by default (<2.5 minutes).
   - Add `check-quickshell:` for testing Quickshell QML components (`check-shell-appearance` + `check-quickshell-system-management`).
   - Add `check-all:` for comprehensive full-matrix testing.

2. **`.github/workflows/c-cpp.yml` CI Optimization**:
   - Add `pull_request: branches: ["main"]` trigger.
   - Streamline mandatory CI gate to `validate-core` and `clang-build` (<3 minutes total!).
   - Isolate `validate-shell-appearance` and `validate-system-management` to `workflow_dispatch` with input flag `test_quickshell: true` (default: false).

3. **Documentation & Validation**:
   - Record living handoff and implementation walkthrough.
   - Validate local `make check` in <2.5 minutes.
   - Push and verify CI green in <3 minutes on PR.

### Non-goals

- Do NOT delete Quickshell QML code (it remains fully available as an optional add-on).
- Do NOT disable any unit tests for native tools.
