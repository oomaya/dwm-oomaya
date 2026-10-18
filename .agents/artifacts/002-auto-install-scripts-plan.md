# Architectural Implementation Plan: Packet #002 Auto-install scripts/ into ~/.local/bin/

- **Packet ID**: `Packet #002`
- **Topic**: Auto-install `scripts/` into `~/.local/bin/`
- **Status**: Ready for Human Review & Approval
- **Branch**: `packet/002-auto-install-scripts-into-local-bin`
- **Author**: Tech Lead (Antigravity Pair)

---

## 1. Executive Summary

On 2026-10-17, multiple configuration and utility scripts in `scripts/` were added or updated (such as `dwm-settings-hub`, `dwm-default-apps`, `dmenu-find`, `dwm-cursor-reload`, `seed-default-apps.sh`). Currently, deploying changes requires manual copying and `chmod +x`, or running the full distribution installer. 

This plan implements an automated, idempotent `make install-scripts` target (and integrates it into `make install-user`, `make install`, and `install.sh`), updates file permissions and script inventory, ensures user PATH deployment with clear feedback, and repairs a pre-existing manifest mismatch in `check-install-manifest`.

---

## 2. Quota & Token Budget Analysis (Go/No-Go Decision Gate)

- **Gemini Weekly**: 0.21% used (~99.79% remaining, resets 2026-10-24)
- **Gemini 5h Window**: 0.02% used (~99.98% remaining)
- **Claude Weekly**: 0.0% used (100% remaining)
- **Projected Token Burn**: ~15k tokens across planning, surgical edits, and test validation.
- **Decision Gate**: **GO (Green Light)**. Model pairing: Gemini 3.8 Flash for techlead coordination and surgical edits.

---

## 3. Deep-Dive Problem & Context Analysis

### 3.1 Current Architecture & Friction Points
1. **Developer Friction**:
   - `make install` attempts `install-system` followed by `install-user`. When run as an unprivileged user, `install-system` attempts to write to `/usr/local/bin` and fails with permission denied.
   - Developers updating scripts in `scripts/` have no dedicated, lightweight target to refresh `~/.local/bin/` without rebuilding everything or running as root.
2. **Missing Script Inventory**:
   - `INSTALL_COMMANDS` in [Makefile](file:///home/rand/.local/src/dwm-oomaya/Makefile) lists 65 specific commands, omitting recently introduced utilities like `scripts/dmenu-find`, `scripts/dwm-cursor-reload`, and `scripts/seed-default-apps.sh`.
   - In git, `scripts/dmenu-find`, `scripts/dwm-cursor-reload`, `scripts/dwm-polkit`, and `scripts/seed-default-apps.sh` are set to `100644` (non-executable).
3. **Lack of User Feedback**:
   - `install-user` installs scripts in a silent loop (`@for f in ${INSTALL_COMMANDS}; do ... done`), giving no feedback on what scripts were deployed.
4. **Pre-Existing Manifest Bug in `check-install-manifest`**:
   - `dwm-oomayad`, `oomaya-ctl`, `dwm-msg-compat`, `dwm-msg`, and `dwm-oomayad.service` were added to `install-system` and `uninstall`, but omitted from `check-install-manifest`'s expected list, causing `make check-install-manifest` to fail on `main`.

---

## 4. Proposed Solution & Architecture

### 4.1 Makefile Target: `install-scripts`
Add a dedicated, unprivileged, idempotent target `install-scripts` in [Makefile](file:///home/rand/.local/src/dwm-oomaya/Makefile):
- Validates `${USER_HOME}` and ensures `id -u != 0`.
- Creates `${USER_HOME}/.local/bin` if missing (`mkdir -p`).
- Scans `scripts/*`, excluding non-command files (`.xinitrc`, backup/editor files) and privileged helpers (`dwm-settings-display-root`).
- Installs executable scripts with `install -Dm755` into `${USER_HOME}/.local/bin/<basename>`.
- Prints clear feedback for each installed utility script along with a summary total count.
- Integrate `install-scripts` into `install-user` so standard `make install` and `make install-user` invoke it automatically.
- Add `install-scripts` to `.PHONY`.

### 4.2 Script Inventory & Git Mode Fixes
- Add missing utility commands to `INSTALL_COMMANDS` in `Makefile`:
  - `scripts/dmenu-find`
  - `scripts/dwm-cursor-reload`
  - `scripts/seed-default-apps.sh`
- Ensure file executable bits are set in git:
  `chmod +x scripts/dmenu-find scripts/dwm-cursor-reload scripts/dwm-polkit scripts/seed-default-apps.sh`

### 4.3 `install.sh` Integration
- In [install.sh](file:///home/rand/.local/src/dwm-oomaya/install.sh), add an explicit script installation verification / trigger step around the build & install block:
  ```bash
  info "Deploying desktop utility scripts to ~/.local/bin..."
  make install-scripts USER_HOME="$HOME" OWNER="$(id -un)"
  ok "Desktop utility scripts deployed to ~/.local/bin."
  ```
- Because `make install-user` also invokes `make install-scripts`, this is completely idempotent and safe.

### 4.4 Fix `check-install-manifest`
- Update `check-install-manifest` in `Makefile` to include the expected outputs of `dwm-oomayad`, `oomaya-ctl`, `dwm-msg`, `dwm-msg-compat`, and `dwm-oomayad.service`.

---

## 5. Risk Assessment & Mitigations

| Risk | Impact | Mitigation |
| :--- | :--- | :--- |
| `test-install-preservation.sh` regression | High | Keep `install-user:` recipe clean; preserve `@test "$$(id -u)" -ne 0`, zero `chown`, and `--no-preserve=ownership`. |
| Overwriting privileged helper `dwm-settings-display-root` into user bin | Critical (Security) | Explicitly exclude `dwm-settings-display-root` from `install-scripts` loop. |
| Non-command dotfiles in `~/.local/bin` | Medium | Explicitly exclude `.xinitrc`. |
| Rebuilding user sources as root | High | Obey `AGENTS.md` and `Makefile` user isolation contract. |

---

## 6. Verification & Validation Steps

1. **Syntax & Style Gates**:
   - `scripts/run-tests make check-shell`
   - `shfmt -d install.sh scripts/*.sh tests/*.sh` (where shfmt is available)
2. **Install & Preservation Gates**:
   - `scripts/run-tests make check-install-manifest`
   - `scripts/run-tests make check-install-preservation`
   - `scripts/run-tests make check-dev-sync-install`
3. **Execution & Idempotency Check**:
   - Run `make install-scripts` once $\rightarrow$ verify all scripts installed in `~/.local/bin/` with 0755.
   - Run `make install-scripts` a second time $\rightarrow$ verify 0 errors, idempotency satisfied.
4. **Full Phase 1 Parallel Matrix Gates**:
   - `scripts/run-tests make check-core`
   - `scripts/run-tests make check-shell-appearance`
   - `scripts/run-tests make check-system`

---

## 7. Tollgate Decision Points for User

1. **Maintenance scripts exclusion**:
   - Repo maintenance scripts (`quickshell-qmllint`, `run-tests`, `configure-build.sh`, `dev-sync-install.sh`) should be excluded from `~/.local/bin/` to avoid polluting the user's interactive shell command list.
   *(Recommendation: Exclude them from user `~/.local/bin/`, installing only end-user desktop utilities).*
2. **Fixing `check-install-manifest`**:
   - Include the missing manifest entries for `tools/oomaya-ctl` and `tools/dwm-quickshell-state-bin` in `check-install-manifest` so the Phase 1 `check-core` gate passes 100% cleanly.
