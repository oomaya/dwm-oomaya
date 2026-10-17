# Plan: DWM-Oomaya Codebase Diet, Agent-First Parity & CI Optimization

## Executive Summary
Following user feedback and architectural review, this plan refines DWM-Oomaya's focus around its core identity: **Suckless philosophy, ChadDWM brutalist aesthetics, and an Agent-First sovereign desktop environment with secondary gaming capabilities**.

Key revisions based on user directions:
1. **Herdr Workspace Evaluation**: **RETAINED & ELEVATED**. Herdr (`herdr.dev`) is a Rust-based terminal multiplexer engineered specifically for AI coding agents (Claude Code, Codex, Antigravity) with persistent sessions and agent state tracking. It directly fulfills Oomaya's "Agent-First WM" raison d'être.
2. **Gaming Support Re-evaluation**: **RETAINED & POLISHED**. Gaming is an explicit secondary purpose. `scripts/protonrestart` and GPU helpers (`nvidia-gpu`, `nvidia-temp`) are preserved to prevent session restarts when Proton/Wine games hang.
3. **Quickshell Necessity & Pure Suckless Parity**: In-depth audit shows that DWM-Oomaya's dynamic TOML parsing (`hotkeys.toml`, `themes.toml`) is **100% native C (`tomlparser.c`)** and completely independent of Quickshell. Furthermore, `scripts/dwm-settings-hub` already implements a pure dmenu/POSIX frontend for 90% of settings. Full parity without Quickshell requires only ~12–18 engineering hours.
4. **Fedora ISO Builder Purge**: Complete deletion of the custom Fedora netinstall ISO builder, Anaconda installer branding, and Kickstart files (`dwm-fedora*.ks`).
5. **CI Tri-Job Parallelization**: Slashes CI turnaround from **30 minutes to under 10 minutes**.

---

## Quota & Token Budget Analysis (Go/No-Go Decision Gate)

| Metric | Current Status | Safe Threshold | Decision Gate |
| :--- | :--- | :--- | :--- |
| **Gemini Weekly Quota** | **84.35% Remaining** (15.65% used) | > 15.0% | **GO (Green)** |
| **Gemini 5-Hour Quota** | **98.84% Remaining** (1.16% used) | > 20.0% | **GO (Green)** |
| **Claude Weekly Quota** | **100.0% Remaining** (0.0% used) | > 15.0% | **GO (Green)** |
| **Projected Phase Tokens** | ~30,000 – 40,000 tokens | Low footprint | **GO (Green)** |
| **Model Selection** | Gemini 2.5 Flash / Architect | Flow Preservation | Gemini handles plan & execution directly |

**Decision**: Quotas are abundantly clear (>84% remaining across all windows). No throttling required. Proceed with execution upon user confirmation.

---

## 1. Deep Dive: Agent-First WM & Gaming Alignment

### 1.1 Herdr Evaluation: Core "Agent-First WM" Synergy
- **What is Herdr?** Developed in Rust, Herdr is a terminal multiplexer designed specifically as a runtime and workspace for AI coding agents (Codex, Claude Code, Antigravity, OpenCode).
- **Synergy with DWM-Oomaya**:
  - **Background Persistence**: Preserves terminal panes and agent subprocesses even across X11 restarts, SSH reconnects, or screen lock.
  - **Agent State Tracking**: Automatically monitors and categorizes agent panes as `working`, `blocked` (waiting for user input), `idle`, or `done`.
  - **DWM Flow Cockpit Integration**: Pairs naturally with `scripts/dwm-flow` (`Super+C` sovereign cockpit: Agy CLI + Neovim studio).
- **Verdict**: **100% aligned with Oomaya's core mission**. Keep `scripts/install-herdr` and `tests/test-install-herdr.sh`.

### 1.2 Gaming Support: `protonrestart` & GPU Helpers
- **Role in Oomaya**: When running Steam/Proton/Lutris games on Linux, game crashes often leave hung background processes (`wineserver`, `services.exe`, `pv-bwrap`, `Battle.net`) that prevent games from relaunching without rebooting the X session.
- **`scripts/protonrestart`**: Automatically terminates lingering Wine/Proton prefixes and CEF processes cleanly in one keybinding.
- **GPU Helpers**: `scripts/nvidia-gpu` and `scripts/nvidia-temp` provide instantaneous, lightweight metrics without launching heavy GUI monitors.
- **Verdict**: **RETAIN**. Keep `protonrestart`, `nvidia-gpu`, and `nvidia-temp`.

---

## 2. Quickshell vs. Pure Suckless Architecture: Parity & Effort Analysis

The user asked: *Is Quickshell itself needed in DWM-OOMAYA? Does TOML instant reloading depend on it? Can we achieve parity without Quickshell, and what would be the engineering effort?*

### 2.1 The TOML Instant Configuration Truth
- **Key Finding**: Quickshell has **zero** involvement in TOML live-reloading.
- In `dwm.c`, `tomlparser.c` natively parses `~/.config/dwm-oomaya/hotkeys.toml` and updates internal C structs upon receiving `SIGUSR1`. dwm updates its keybindings, layout definitions, and rules in under 1 millisecond with zero external dependencies.

### 2.2 Feature Parity Audit (Quickshell vs. Pure Suckless/dmenu)

| Feature Area | Current Quickshell (Qt6/QML) | Native Suckless / Omarchy Equivalent | Current Parity Status | Additional Effort to Replace Quickshell |
| :--- | :--- | :--- | :---: | :--- |
| **Status Bar** | `config/quickshell/panel` (QML topbar) | ChadDWM C `status2d` topbar (`dwm-bar.sh` / `dwm-status`) | **100% Exists** | **0 hours** (native in `dwm.c`) |
| **App Launcher** | `config/quickshell/launcher` | `dmenu-oomaya` (`dmenu-run`, `dmenu-desktop`) | **100% Exists** | **0 hours** (native in repo) |
| **Settings Hub** | `config/quickshell/settings` (Qt6 GUI) | `scripts/dwm-settings-hub` via `dmenu` | **90% Exists** | **~4 hours** (add any missing toggles) |
| **Keybindings Menu** | `quickshell call controlcenter openKeybinds` | `scripts/dwm-keybinds` via `dmenu` | **100% Exists** | **0 hours** (native in repo) |
| **Notifications & OSD** | `config/quickshell/notifications` | `dunst` (`~/.config/dunst/dunstrc`) + `dwm-volume` OSD | **100% Exists** | **0 hours** (standard Unix daemon) |
| **Power Menu** | `config/quickshell/power` | `dmenu` session menu (`dwm-session-launch`) | **100% Exists** | **0 hours** (native script) |
| **Control Center Toggles** | `config/quickshell/controlcenter` (Wi-Fi, BT, Audio, Brightness) | `scripts/dwm-controlcenter` via `dmenu` / POSIX helpers | **80% Exists** | **~6–8 hours** (polish cohesive dmenu quick-toggle menu) |
| **System Info & Updates** | `dwm-system-management` + QML UI | Terminal wrapper (`dnf upgrade` / `pacman -Syu` in floating terminal) | **70% Exists** | **~2–4 hours** (lightweight status indicator) |

### 2.3 Quickshell Overhead vs. Pure Suckless Engineering Cost
- **Quickshell Cost**:
  - Adds ~17 minutes to CI execution (Xvfb QML scenes, mock D-Bus).
  - Requires heavy Qt6/QML and GPU scene-graph support (fails in minimal headless/server environments).
  - High maintenance surface (~10,000 lines of QML and Phase 6 Python code).
- **Engineering Effort to Fully Decouple/Replace Quickshell**:
  - **Total Effort: ~12 – 18 engineering hours**.
  - `dwm-settings-hub` already drives wallpaper, themes, fonts, displays, autostart, and default apps natively via dmenu.
  - Adding a polished dmenu/TUI control center and terminal update wrapper completes 100% parity.

---

## 3. Forensic Analysis: Why CI Green Takes ~30 Minutes

In CI Run `#39045926102` (`validate` total: **29m 34s**):
- **Bottleneck 1: `make check` inside `validate` takes 28m 10s (95.3% of total time)**.
- **Top 4 Time Sinks (84.7% of test execution)**:
  1. `check-quickshell-system-management` (`tests/test-quickshell-system-management-xvfb.sh`): **9m 23s (33.3%)**
  2. `check-system-management` (`tests/test-system-management.py`): **6m 40s (23.7%)**
  3. `check-appearance` (theme, wallpaper, font suites): **4m 25s (15.7%)**
  4. `check-quickshell-settings-xvfb`: **3m 22s (12.0%)**
- **Bottleneck 2: Monolithic Sequential Execution**: Everything runs sequentially on one CPU core inside a single container.
- **Bottleneck 3: Unused ISO Packages**: Installing `squashfs-tools-ng`, `xorriso`, `isomd5sum`, `pykickstart` in container setup.

---

## 4. Proposed Phased Execution Roadmap

```mermaid
graph TD
    subgraph "Phase 1: Immediate Diet & CI Optimization (Current Task)"
        P1A[Purge Fedora ISO Builder & Anaconda Branding]
        P1B[Parallelize CI into 3 Concurrent Jobs]
        P1C[Retain Herdr & Gaming Scripts]
        P1A --> P1D[CI Green Turnaround < 10 mins]
        P1B --> P1D
        P1C --> P1D
    end

    subgraph "Phase 2: Pure Suckless Transition (Next Milestone)"
        P2A[Enhance dwm-settings-hub & Control Center]
        P2B[Make Quickshell Fully Optional / Decoupled]
        P2C[Pure Suckless C + POSIX Desktop]
        P2A --> P2C
        P2B --> P2C
    end
```

### Phase 1: Immediate Diet & CI Parallelization (Scope of this task)

#### 1. Purge Fedora ISO Builder & Anaconda Branding
- `[DELETE]` `scripts/build-dwm-fedora-installer-iso.sh`
- `[DELETE]` `scripts/generate-sidebar-logo.py`
- `[DELETE]` `dwm-fedora.ks`
- `[DELETE]` `dwm-fedora-nvidia.ks`
- `[DELETE]` `branding/anaconda/` (entire directory)
- `[DELETE]` `tests/test-fedora-iso-builder.sh`
- `[DELETE]` `tests/test-kickstart-variants.sh`
- `[MODIFY]` `Makefile`: Remove `check-kickstart` and `check-fedora-iso-builder` from `check:` and `.PHONY`.
- `[MODIFY]` `scripts/dwm-packages.sh`: Remove `image-build` package list.

#### 2. Retain & Clean Gaming / App Scripts
- `[KEEP]` `scripts/install-herdr` & `tests/test-install-herdr.sh` (Agent-first workspace tool).
- `[KEEP]` `scripts/protonrestart`, `scripts/nvidia-gpu`, `scripts/nvidia-temp` (Gaming support).
- `[DELETE]` `scripts/install-gearlever` & `tests/test-install-gearlever.sh` (Oomaya uses native `dwm-app-install`).
- `[MODIFY]` `install.sh`: Remove Flathub GearLever forced auto-install.

#### 3. Tri-Job Parallel CI Matrix (`.github/workflows/c-cpp.yml`)
Split the monolithic 28-minute `validate` job into 3 parallel jobs:

```yaml
jobs:
  validate-core:
    runs-on: ubuntu-latest
    timeout-minutes: 10
    container: fedora:44
    steps:
      - C compilation (dwm, dwm-oomayad)
      - check-format, check-shell, check-hotkeys, check-status
      - check-terminal, check-lock, check-xvfb-runtime
      # Runs in ~3-4 minutes

  validate-shell-appearance:
    runs-on: ubuntu-latest
    timeout-minutes: 15
    container: fedora:44
    steps:
      - Quickshell QML lint
      - check-appearance (theme, wallpaper, font)
      - check-quickshell-launcher, controls, controlcenter, power
      # Runs in ~5-6 minutes

  validate-system-management:
    runs-on: ubuntu-latest
    timeout-minutes: 15
    container: fedora:44
    steps:
      - check-system-management (Python unit tests)
      - check-quickshell-system-management (Xvfb)
      - check-settings-display-security
      # Runs in ~9-10 minutes
```

**Net Impact**: Total CI Green wall-clock time drops from **29m 34s to ~9m 30s (68% faster)** immediately.

---

## 5. Verification Plan

### Automated Tests
```bash
# 1. Clean build verification
make clean all

# 2. ShellCheck & formatting on remaining scripts
make check-shell check-format

# 3. Fast core validation
make check-build-config check-hotkeys check-status check-terminal check-lock check-xvfb-runtime

# 4. CI Workflow verification on origin/main
gh run watch <run-id>
```

### Success Criteria
1. `git grep -i "fedora-iso"` returns 0 results.
2. `make check` executes cleanly without broken Kickstart/ISO dependencies.
3. Herdr, Protonrestart, and native App installer remain 100% operational.
4. GitHub Actions CI completes in under 10 minutes with 100% green checkmarks across all parallel jobs.
