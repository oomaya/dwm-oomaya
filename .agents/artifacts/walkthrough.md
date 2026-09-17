# Consolidated DWM-oomaya & Dmenu Installation Pipeline Walkthrough

We have unified the previously separate `dwm-oomaya` and `dmenu-oomaya` installation streams into a consolidated pipeline, eliminated the GitHub `insteadOf` rewrite loop and SSH/HTTPS authentication trap, and introduced a remote one-liner bootstrapper with TTY reattachment.

---

## Changes Made

### 1. Smart Git Transport Engine & Isolation Shield
- **File**: [`scripts/dwm-git-helper.sh`](file:///home/rand/dwm-oomaya/scripts/dwm-git-helper.sh)
- **`dwm_git_probe_ssh()`**: Probes GitHub SSH authentication non-interactively in sub-second time using `ssh -o BatchMode=yes -o ConnectTimeout=3 -o StrictHostKeyChecking=accept-new -T git@github.com`.
- **`dwm_git_resolve_url()`**: Automatically routes `oomaya/*` repositories to SSH (`git@github.com:...`) when authenticated (preserving instant `git push` access for you) or falls back cleanly to HTTPS when unauthenticated.
- **`dwm_git_safe_clone()`**: Implements an **Isolation Shield** using `GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null GIT_CONFIG_NOSYSTEM=1`. This completely bypasses any global `url.<base>.insteadOf` rules in `~/.gitconfig`, permanently neutralizing circular infinite loops (`fatal: infinite loop in insteadOf substitution detected!`) and public-key auth failures.
- **`dwm_git_clone()`**: Unified clone dispatcher supporting `--git-protocol={auto,ssh,https}`.

---

### 2. Consolidated Installation Script
- **File**: [`install.sh`](file:///home/rand/dwm-oomaya/install.sh)
- **Unified Ecosystem Build (`install_dmenu_ecosystem`)**:
  - Automatically discovers `dmenu-oomaya` across custom `--dmenu-dir`, adjacent developer checkouts (`../dmenu-oomaya`), or canonical checkout (`~/.local/src/dmenu-oomaya`).
  - Automatically clones `oomaya/dmenu-oomaya` if not present using the Smart Git Transport.
  - Compiles and deploys `dmenu` and its companion POSIX suite (`dmenu-desktop`, `dmenu-run`, `dmenu-power`, `dmenu-windows`, `dmenu-hub`, `dmenu-scrot`, `dmenu-clip`) both system-wide (`/usr/local/bin`) and user-locally (`~/.local/bin`) using `install -Dm755` (`ETXTBSY` immune).
- **New CLI Flags**:
  - `--skip-dmenu`: Skip dmenu installation if managed independently.
  - `--dmenu-dir=PATH`: Explicit path to dmenu source checkout.
  - `--git-protocol={auto,ssh,https}`: Transport protocol selection.
- **Third-Party Asset Immunity**:
  - Nordic GTK theme and Nord wallpaper downloads now use `dwm_git_safe_clone`, preventing SSH rewriting on public repositories.

---

### 3. Remote Zero-Friction Bootstrapper
- **File**: [`bootstrap.sh`](file:///home/rand/dwm-oomaya/bootstrap.sh)
- **Single-Line Remote Execution**:
  ```bash
  curl -fsSL https://raw.githubusercontent.com/oomaya/dwm-oomaya/main/bootstrap.sh | bash
  ```
- **Piped TTY Preservation**: Detects if stdin is connected to a pipe (`! -t 0`) and reattaches stdin to `/dev/tty`. This guarantees that `sudo` password prompts and configuration dialogs remain interactive without consuming subsequent bash instructions.
- **Prerequisite Bootstrapping**: Validates and installs missing build tools (`git`, `curl`, `make`, `gcc`).
- **Canonical Setup**: Clones and synchronizes both `dwm-oomaya` and `dmenu-oomaya` into canonical `~/.local/src/` before executing the consolidated installer.

---

### 4. Build System & Test Suite Parity
- **File**: [`Makefile`](file:///home/rand/dwm-oomaya/Makefile)
  - Added target `install-dmenu` for manual developer builds.
  - Fixed manifest verification parity in `check-install-manifest` (`usr/libexec/dwm-oomaya/dwm-settings-display-root`, `capitaine-cursors` licenses, and `dwm-oomaya` symlink).
- **File**: [`tests/test-install-preservation.sh`](file:///home/rand/dwm-oomaya/tests/test-install-preservation.sh)
  - Updated expected data path to `dwm-oomaya`.
- **File**: [`README.md`](file:///home/rand/dwm-oomaya/README.md)
  - Updated Quickstart to document the remote one-liner, canonical source paths, Smart Git Transport options, and consolidated build instructions.

---

## Validation & Test Results

All test suites executed with 100% success:

```bash
# 1. Smart Git Transport & insteadOf immunity under hostile circular configs
/home/rand/dwm-oomaya/tests/test-smart-git.sh
# ==> All Smart Git Transport tests passed successfully! ✓

# 2. Consolidated installer options & dry-run validation
/home/rand/dwm-oomaya/tests/test-consolidated-install.sh
# ==> Consolidated installation test suite passed! ✓

# 3. System install manifest and uninstall symmetry
make check-install
# ==> Install manifest and uninstall symmetry validated.

# 4. Repeated install and user file preservation
/home/rand/dwm-oomaya/tests/test-install-preservation.sh
# ==> Repeated install preservation: PASS

# 5. Shellcheck across all 90 scripts
make check-shell && make check-build-config
# ==> Build configuration generation and preservation: PASS (0 errors, 0 warnings)
```
