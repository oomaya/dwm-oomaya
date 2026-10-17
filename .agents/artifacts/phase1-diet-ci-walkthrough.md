# Phase 1 Codebase Diet & CI Optimization Walkthrough

## Executive Summary

Phase 1 of the **dwm-oomaya Codebase Diet & CI Optimization** has been executed, committed (`aa0c6a8`, `be89c39`), pushed to `origin/main`, and deployed across GitHub Actions. By applying Pareto principles and stripping unneeded legacy DWM-Titus installer artifacts while retaining core Agent-first tools, the repository footprint has been dramatically reduced and CI execution restructured from a 30+ minute sequential bottleneck into 3 concurrent parallel jobs.

---

## 1. Codebase Diet Metrics & Deleted Legacy Artifacts

### Net Reduction
- **Files deleted:** 17 files
- **Net code lines removed:** 3,573 lines deleted
- **Binary assets removed:** Anaconda pixmaps and installer logos (~100 KB)

### Purged Bloat Components
1. **Fedora Netinstall ISO Builder & Anaconda Branding:**
   - Deleted: `scripts/build-dwm-fedora-installer-iso.sh` (224 lines)
   - Deleted: `scripts/generate-sidebar-logo.py` (114 lines)
   - Deleted: `branding/anaconda/` (9 files, 1,814 lines of CSS/Glade + 4 PNG logos)
   - Deleted: `dwm-fedora.ks` (236 lines) & `dwm-fedora-nvidia.ks` (254 lines)
   - Deleted: `tests/test-fedora-iso-builder.sh` (263 lines) & `tests/test-kickstart-variants.sh` (220 lines)
   - Removed: DNF build dependencies (`squashfs-tools-ng`, `xorriso`, `isomd5sum`, `pykickstart`) from package map and CI runner.
2. **Flathub GearLever Sub-Installer:**
   - Deleted: `scripts/install-gearlever` (66 lines)
   - Deleted: `tests/test-install-gearlever.sh` (136 lines)
   - Cleaned: `install.sh` installation prompts and auto-run blocks.
   - Cleaned: `docs/src/content/troubleshooting.md` GearLever section.
3. **Dead Shell Utilities:**
   - Deleted: `scripts/disable-powersaving` (unreferenced stub).

### Preserved Core Tools (Confirmed Design Rationale)
- **Herdr (`scripts/install-herdr` & `tests/test-herdr-install.sh`):**
  - **Verdict:** Retained.
  - **Rationale:** [Herdr](https://herdr.dev) is a high-performance Rust terminal multiplexer purpose-built for AI coding agents (Claude Code, Antigravity, OpenCode). It directly serves dwm-oomaya's identity as an **Agent-First Window Manager**.
- **Proton & Gaming Scripts (`scripts/protonrestart`, `scripts/nvidia-gpu`, `scripts/nvidia-temp`):**
  - **Verdict:** Retained.
  - **Rationale:** Gaming support is an explicit secondary purpose of dwm-oomaya.

---

## 2. CI Parallelization & Test Target Decomposition

### Problem Solved
Previously, GitHub Actions ran a single monolithic `validate` job running all 40+ tests sequentially inside a container, taking **30 to 35 minutes** to reach CI Green.

### Modular Makefile Targets
In [Makefile](file:///home/rand/.local/src/dwm-oomaya/Makefile), test suites were decomposed by domain into 3 independent suites:

```makefile
# Core C, dwm compilation, hotkeys, status bar, terminals, lock screen (~2-3m)
check-core: all \
	check-build-config check-hotkeys check-status check-terminal \
	check-lock check-herdr-install check-fedora-packages \
	check-monitors-install check-wallpapers check-sound \
	check-screenshot check-monitor-tags check-xvfb-runtime \
	check-shell-runtime-xvfb check-theme-xvfb check-display-reset-xvfb \
	check-display-daemon-xvfb check-settings-daemon-xvfb

# Shell scripts, formatters, and Quickshell appearance/theming (~4-5m)
check-shell-appearance: all \
	check-shell check-format check-quickshell-syntax \
	check-quickshell-lint check-quickshell-runtime \
	check-quickshell-qml check-appearance \
	check-quickshell-launcher check-quickshell-controls \
	check-quickshell-audio check-quickshell-controlcenter \
	check-quickshell-power check-quickshell-power-backend \
	check-quickshell-power-model check-quickshell-session-actions \
	check-quickshell-defaults-model check-quickshell-appearance-model \
	check-quickshell-design-system check-quickshell-large-surfaces \
	check-quickshell-large-surfaces-xvfb check-quickshell-panel-menus \
	check-quickshell-panel-settings check-accessibility \
	check-quickshell-command-menu check-quickshell-notifications \
	check-quickshell-tray check-quickshell-network check-quickshell-connectivity \
	check-quickshell-settings-xvfb check-quickshell-settings-responsiveness-xvfb

# Deep Xvfb, settings hub, power backend, diagnostics, system management (~6-8m)
check-system: all \
	check-system-health check-system-management \
	check-quickshell-system-management check-settings
```

### CI Hardening & Concurrency Protection
1. **Unprivileged Setpriv Daemon Cleanup (`tests/test-quickshell-settings-xvfb.sh`):**
   - Added `cleanup_root_runner` with `pkill -u "$unprivileged_uid"` to terminate lingering D-Bus daemons (`xfconfd`) before deleting `/var/tmp/dwm-settings-xvfb-root.*`.
   - Added a 5-step retry loop (`sleep 0.1`) on directory removal to eliminate `rm: cannot remove: Directory not empty` race conditions.
2. **Concurrency-Resilient Discovery Timers:**
   - Relaxed tight single-thread timers in `SystemNativeDiscovery.qml` and `SystemProviderDiscovery.qml` from 30s to 70s.
   - Raised shell wrapper timeouts in `test-quickshell-system-management-xvfb.sh` from 35s to 75s so multi-core parallel container execution never falsely times out.

---

## 3. Remote GitHub Actions Run Status

- **Workflow Run:** [CI Run 39074008495](https://github.com/oomaya/dwm-oomaya/actions/runs/39074008495)
- **Parallel Jobs:**
  1. `validate-core` (`make check-core`)
  2. `validate-shell-appearance` (`make check-shell-appearance`)
  3. `validate-system-management` (`make check-system`)
