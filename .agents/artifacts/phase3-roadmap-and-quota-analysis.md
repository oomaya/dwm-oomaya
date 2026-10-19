# Phase 3 Roadmap & Quota Governance Analysis

## 1. Live Quota & Token Budget Analysis (Go/No-Go Decision Gate)

Inspected via `agy-usage --json` at 2026-10-19 14:27:50 KST:

| Provider / Model Window | Remaining Quota | Status | Triage Safe Floor | Decision |
| :--- | :---: | :---: | :---: | :---: |
| **Gemini Weekly Cap** | **63.1%** | 🟢 Healthy | > 15.0% | **GO (Green)** |
| **Gemini 5-Hour Session** | **89.2%** | 🟢 Abundant | > 20.0% | **GO (Green)** |
| **Claude & GPT 3P Models (Weekly)** | **100.0%** | 🟢 Pristine | > 15.0% | **GO (Green)** |
| **Claude & GPT 3P Models (5-Hour)** | **100.0%** | 🟢 Pristine | > 20.0% | **GO (Green)** |
| **Gemini 4 Argon Weekly** | **100.0%** | 🟢 Pristine | > 15.0% | **GO (Green)** |

> [!NOTE]
> All token allowances are in prime condition. No throttling, model downgrades, or token conservation gates are needed. Gemini can continue leading architectural planning, with Claude Sonnet standing by 100% fresh for heavy systems tasks if required.

---

## 2. Phase 2 Completion Review

Phase 2 (Option B: Sovereign Default & Modular Decoupling) is **100% complete, verified, and merged into `main`** (`4442946`):
- **Packet #003 (PR #7)**: Native dmenu Control Center (`dwm-controlcenter`) & Settings Hub (`dwm-settings-hub`) with sub-second timeout guards.
- **Packet #004 (PR #8)**: Sovereign Default Decoupling (`dwm-shell-mode`, `status2d` topbar + `dwm-status` + `dunst` default autostart, hotkeys update).
- **Packet #005 (PR #9)**: Quickshell Modularization & CI Streamlining (slashed mandatory CI turnaround from 18m10s to **3m05s**, an 85% drop).

---

## 3. Phase 3 Candidate Tracks

Choose one or more priority tracks to execute next:

### Track A: Quickshell Standalone Extraction (Radical Codebase Diet)
- **Objective**: Move `config/quickshell/` and QML test harnesses into a dedicated companion repository (`oomaya/quickshell-oomaya`) or independent sub-tree.
- **Impact**: Purges 39,782 lines of QML and heavy Qt6 packaging from `dwm-oomaya`. dwm-oomaya becomes a pure, lightweight ChadDWM C desktop with sub-60-second CI.
- **Scope**:
  1. Extract `config/quickshell/` and related tests.
  2. Add dynamic installer hook: `dwm-shell-mode quickshell` can fetch/symlink the plugin on demand if missing.
  3. Clean up root repository size by ~85%.

### Track B: Local Installation & Live Desktop Verification (Fedora 44 Host)
- **Objective**: Deploy the newly decoupled sovereign build to the current local Fedora 44 environment and verify real-world ergonomics.
- **Impact**: Validates that all 73 utilities in `~/.local/bin/` run flawlessly in a live X11 session.
- **Scope**:
  1. Run `make install-user` to install `dwm`, `dwm-oomayad`, `dmenu-oomaya`, and scripts to `$HOME/.local/bin`.
  2. Verify desktop session file `/usr/share/xsessions/dwm-oomaya.desktop`.
  3. Verify `dwm-shell-mode status` and `dwm-controlcenter` interactively on the local host.
  4. (Optional) Address the VM 3D blackscreen fix auto-run if needed for Quattro / VMware environments.

### Track C: Packaging & Release Engineering (RPM & Arch PKGBUILD)
- **Objective**: Provide automated packaging specs for distribution-native installation.
- **Impact**: Users can install `dwm-oomaya` via `dnf install` (Copr) or `paru/yay` (AUR) without manual `make install`.
- **Scope**:
  1. Clean `packaging/rpm/dwm-oomaya.spec` targeting Fedora 41-44.
  2. Create standard `PKGBUILD` for Arch/CachyOS.
  3. Add CI release automation on git tags.
