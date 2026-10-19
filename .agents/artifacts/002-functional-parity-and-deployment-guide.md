# dwm-oomaya: Functional Parity, CI Status & VM Deployment Guide

## 1. CI Status Overview

### Local Validation Matrix (100% Green)
All three Phase 1 parallel CI matrix jobs and install safety gates have passed locally:

| Test Target / Job | Scope | Status | Notes |
| :--- | :--- | :---: | :--- |
| `make check-shell` | ShellCheck script linting | **PASS** | 0 warnings across all shell scripts |
| `make check-format` | `shfmt` POSIX/Bash formatting | **PASS** | Formatted and verified with `shfmt -d` |
| `make check-install` | Staged install manifest & uninstall | **PASS** | Verified with temporary staging roots |
| `make check-install-manifest` | Binary & script install manifest contract | **PASS** | 100% symmetry across installed tools |
| `make check-install-preservation` | Idempotency & user config preservation | **PASS** | 72 utility scripts installed cleanly |
| **Matrix Job 1 (`check-core`)** | Window manager C core, IPC, autostart | **PASS** | Exit code 0 |
| **Matrix Job 2 (`check-shell-appearance`)** | Quickshell UI, design tokens, themes | **PASS** | Exit code 0 |
| **Matrix Job 3 (`check-system`)** | System management, 689 unit tests, settings | **PASS** | Exit code 0 (all 689 tests pass) |

### Hosted GitHub Actions CI Status
- **Prior Run (`39174980325`)**: Failed at `check-core` due to a 1-character formatting discrepancy in `scripts/dwm-utils.sh` (`shfmt` whitespace around parentheses).
- **Resolution**: `shfmt -w scripts/dwm-utils.sh` executed, verified with `make check-format`, committed (`9800d77`), and pushed to `main`.
- **Current Run**: Actively executing on `main` via GitHub Actions (`c-cpp.yml`).

---

## 2. Functional Parity: Today's Additions & Changes

### A. Automatic Script Deployment (`install-scripts`)
- **What Changed**: Previously, deploying updated utilities from `scripts/` required manual copying and `chmod +x`. Now, `make install-scripts` dynamically inspects `scripts/`, filters out repository-internal development tooling, and deploys 72 executable utilities to `${USER_HOME}/.local/bin/` with mode `0755`.
- **Integrated Paths**:
  - `make install-scripts` (standalone utility deployment).
  - `make install-user` (automatically calls `install-scripts`).
  - `install.sh` (existing-system installer automatically invokes `install-scripts`).
- **Excluded Tools**: Internal maintenance tools (`quickshell-qmllint`, `run-tests`, `configure-build.sh`, `dev-sync-install.sh`) and privileged root helpers (`dwm-settings-display-root`) are strictly excluded from the user's interactive command search path.

### B. Install Manifest Parity
- **What Changed**: Added entries for `tools/oomaya-ctl` and `tools/dwm-quickshell-state-bin` to `INSTALL_COMMANDS` and `check-install-manifest`. Both tools are now first-class citizens in the install lifecycle.

### C. Cross-Distribution Runtime Parity (Debian / Peppermint OS / Non-Fedora)
Today's testing uncovered and resolved several architectural divergences between Fedora (CI container) and Debian-derived environments (Peppermint OS):

1. **Procps 4.0.4 Process Inspection**:
   - `pgrep` on Debian does not support `--env "DISPLAY=..."` and `pidwait` does not support `-p <pid>`.
   - Updated `scripts/dwm-quickshell-controlcenter` and `scripts/dwm-xsettings` to gracefully fall back to `/proc/$pid/environ` inspection and `pidwait -F <pidfile>`, preventing hangs during locker and settings daemon detection.
2. **Lock Deadlock Prevention in `dwm-xsettings`**:
   - Separated runtime directory initialization (`ensure_runtime_dir`) from exclusive file locking (`ensure_runtime`), preventing recursive deadlocks during `dwm-xsettings session-apply`.
3. **Bash 5.2 Command Substitution Traps**:
   - Reordered `dwm-xdg-autostart` protocol emission before subshell variable assignments to avoid Bash 5.2 subshell syntax traps upon SIGTERM.
4. **PyGObject Introspection Normalization**:
   - In `scripts/dwm-system-management`, added dynamic fallback across `GLibUnix.signal_add`, `GLibUnix.signal_add_full`, and `GLib.unix_signal_add` to support older and newer PyGObject introspection bindings.
5. **Glibc `fopen` vs `fopen64` Interposition**:
   - In `tests/test-system-management.py`, interposed both `fopen` and `fopen64` to support mountinfo polling tests across glibc 2.38+ variants.
6. **Deterministic Test Sorting**:
   - Enforced `LC_ALL=C sort -u` in `tests/test-quickshell-system-management.sh` to prevent failure on UTF-8 locales where uppercase and lowercase strings sort differently.

---

## 3. How to Test the Changes

### Test 1: Verify Script Installation & Permissions
Run the user-level installation:
```bash
cd ~/.local/src/dwm-oomaya
make install-scripts
```
**Expected Output**:
```text
==> Installing utility scripts to /home/rand/.local/bin...
  [installed] active-audio -> /home/rand/.local/bin/active-audio
  ...
==> Installed 72 utility scripts to /home/rand/.local/bin.
```
Verify an installed script:
```bash
which dwm-controlcenter dmenu-find dwm-volume dwm-cursor-reload
ls -l ~/.local/bin/dwm-controlcenter
```
**Expected**: All paths point to `/home/rand/.local/bin/...` and permissions are `-rwxr-xr-x` (`0755`).

### Test 2: Verify `oomaya-ctl` and State Bin
Run:
```bash
oomaya-ctl --help || ~/.local/bin/oomaya-ctl --help
```
**Expected**: Displays usage instructions without command-not-found errors.

### Test 3: Test Quickshell Control Center & Process Inspection
Test the control center helper with the procps 4.0.4 fallback:
```bash
dwm-quickshell-controlcenter status
```
**Expected**: Returns JSON/status without throwing `pgrep: unrecognized option '--env'` errors.

### Test 4: Test XSettings Session Apply
Trigger XSettings session application:
```bash
dwm-xsettings session-apply
```
**Expected**: Completes immediately without hanging or locking up.

---

## 4. Applying the New dwm-oomaya to Current VM

Follow these steps to fully deploy and activate the updated `dwm-oomaya` desktop on your Peppermint OS VM:

### Step 1: Compile and Deploy User Environment
```bash
cd ~/.local/src/dwm-oomaya

# 1. Clean build dwm and daemons
make clean all

# 2. Deploy binaries, utility scripts, and Quickshell configs to ~/.local/
make install-user
```

> [!NOTE]
> `make install-user` preserves all your custom configurations under `~/.config/dwm-oomaya/` (`hotkeys.toml`, `themes.toml`, `window-rules.toml`). It will not overwrite your existing personal dotfiles.

### Step 2: Refresh Shell Hash Table
To ensure your interactive shell immediately resolves new binaries in `~/.local/bin`:
```bash
hash -r
```

### Step 3: Reload Desktop & Quickshell
Depending on what you want to reload:

- **Option A (Restart Quickshell UI only)**:
  ```bash
  dwm-quickshell-controlcenter action restart-quickshell
  ```
- **Option B (In-place dwm restart without closing applications)**:
  Press `Super + Shift + R` (default dwm restart keybinding).
- **Option C (Full Clean Session Reload - Recommended)**:
  Log out from the current graphical session (`Super + Shift + Q` or via system menu) and log back in from the display manager.
