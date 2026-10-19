# Implementation Plan: Packet #005 — Quickshell Modularization & CI Streamlining

## Executive Summary

Packet #005 achieves the final performance and architecture milestone of Phase 2 Option B: slashing CI turnaround time from ~18 minutes down to ~2.5 minutes. By decoupling the heavyweight Quickshell Qt6/QML RPM package installation and headless Xvfb rendering jobs into an optional on-demand matrix, core Suckless C and POSIX shell changes can be validated and merged with sub-3-minute turnaround.

---

## Quota & Token Budget Analysis (Go/No-Go Decision Gate)

### Current Allowance Snapshot
- **Gemini Weekly Cap**: 66.3% remaining
- **Gemini 5-Hour Session**: 97.8% remaining
- **Claude / GPT Weekly Cap**: 100% remaining
- **Claude / GPT 5-Hour Session**: 100% remaining

### Projected Consumption
- **Packet #005 Implementation & Verification**: ~6,000–10,000 tokens (<1% quota)
- **Status**: **GO**.

---

## Proposed Changes

### 1. `Makefile` Target Modularization
- Include `$(MAKE) check-settings` in `check-core:` so native desktop settings, control center, and shell mode are always validated.
- Update `check:` to run `check-core` by default (under 2.5 minutes).
- Add `check-quickshell:` target bundling all Quickshell QML tests.
- Add `check-all:` target running both core and Quickshell test suites.
- Update `.PHONY` targets accordingly.

### 2. GitHub Actions CI Optimization (`.github/workflows/c-cpp.yml`)
- Add `pull_request: branches: ["main"]` trigger so PRs get automated status checks.
- Add `workflow_dispatch` input: `test_quickshell` (boolean, default: false).
- Mandatory CI jobs (Always Run):
  - `validate-core`: Compiles dwm and executes `check-core` in container (<2.5m).
  - `clang-build`: Compiles dwm with Clang (<45s).
- Optional CI jobs (Conditional):
  - `validate-shell-appearance`: Only runs if `github.event.inputs.test_quickshell == 'true'`.
  - `validate-system-management`: Only runs if `github.event.inputs.test_quickshell == 'true'`.

### 3. Verification & Metrics
- Run local `make check` and measure execution time (<2.5 minutes).
- Commit, push, open PR #9, and verify all checks complete green in <3 minutes.
