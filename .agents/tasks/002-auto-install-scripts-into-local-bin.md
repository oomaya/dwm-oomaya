# Task Packet: 002-auto-install-scripts-into-local-bin

*Standard task packet according to the architect task-packet-template. Stored in `.agents/tasks/`.*

---

## Packet 002 — Auto-install scripts/ into ~/.local/bin/

**Status:** `active`  
**Branch:** `packet/002-auto-install-scripts-into-local-bin`  
**Author (architect):** Tech Lead & Ted  
**Date:** 2026-10-18  

### Objective

Implement automatic installation of the `scripts/` directory contents into `~/.local/bin/` during the `dwm-oomaya` installation process. This ensures that all utility scripts updated on 2026-10-17 are properly deployed without manual copying.

### Background

On 2026-10-17, several scripts in `scripts/` were updated (including default configuration scripts). Currently, deploying these requires manual copying to `~/.local/bin/` followed by `chmod +x`. This packet automates that process.

### Repo context

- `Makefile`: Defines `INSTALL_COMMANDS`, `install-system`, `install-user`, `install`, and validation targets.
- `install.sh`: Master installer script invoking `make install-system` and `make install-user`.
- `scripts/`: Directory containing desktop utilities, daemons, helpers, and configurations.
- `tests/test-install-preservation.sh`: Strict regression test ensuring unprivileged user install rules and idempotency.

### Approved decisions

- ADR-001: Add editor role to `dwm-default-apps` (`docs/adr/0001-editor-role-dwm-default-apps.md`).
- ADR-002: Defaults resolution principle ("Choose in one place, resolve at launch time").
- Zero Hardcoded Literals Law: Dynamic resolution via `$HOME`, `$USER_HOME`, `$XDG_CONFIG_HOME`, `$XDG_DATA_HOME`.
- Idempotency & User Configuration Safety: Never overwrite existing configurations without confirmation.

### Scope

1. **Make Install Target**:
   - Add/extend a make target (e.g. `install-scripts` or within `install-user` / `install`) that:
     - Copies all executable utility scripts from `scripts/` to `${USER_HOME}/.local/bin/`
     - Sets executable permissions (`chmod +x`) on installed scripts
     - Creates `${USER_HOME}/.local/bin/` if it does not exist
     - Is idempotent (safe to run multiple times)
2. **Install Script Integration**:
   - Update `install.sh` to trigger the script installation step automatically during installation
   - Provide clear, user-friendly feedback indicating which scripts were installed
3. **Script Inventory**:
   - Ensure all utility scripts (including default configuration scripts updated on 2026-10-17 and newly added utilities like `dmenu-find`, `dwm-cursor-reload`, `seed-default-apps.sh`) are covered and properly permissioned.

### Non-goals

- Do NOT install non-utility repository maintenance scripts (like `quickshell-qmllint`, `run-tests`, `.xinitrc`) into system or user PATH.
- Do NOT use Qt on the standard path (per project constraints).
- Do NOT modify suckless dwm C core sources (`dwm.c`, `drw.c`).
- Do NOT break existing `check-install-manifest`, `check-install-preservation`, or the 3-job parallel CI matrix (`check-core`, `check-shell-appearance`, `check-system`).

### Acceptance criteria

1. `make install` successfully copies scripts to `~/.local/bin/`.
2. Installed scripts have executable permissions (`chmod +x`).
3. `install.sh` triggers script installation automatically.
4. Running installation twice does not cause errors (idempotent).
5. CI validate passes (all three parallel jobs: core, shell-appearance, system-management).

### Validation

- `scripts/run-tests make check-shell`
- `scripts/run-tests make check-install`
- `scripts/run-tests make check-install-preservation`
- `scripts/run-tests make check-core`
- `scripts/run-tests make check-shell-appearance`
- `scripts/run-tests make check-system`

### Expected output

Clean diff in `Makefile`, `install.sh`, and affected scripts + 100% passing test results.

---

### Builder notes (filled by builder)

**Plan approved:** [x] (Approved by user on 2026-10-18)  
**Implementation summary:**
- Added `install-scripts:` target in `Makefile` to discover and copy all executable utility scripts from `scripts/` to `${USER_HOME}/.local/bin/` with `0755` permissions, excluding repo-maintenance tooling (`quickshell-qmllint`, `run-tests`, `configure-build.sh`, `dev-sync-install.sh`) and privileged root helpers (`dwm-settings-display-root`).
- Wired `install-user:` to invoke `$(MAKE) install-scripts`.
- Added explicit utility scripts deployment in `install.sh`.
- Fixed `INSTALL_COMMANDS` and `check-install-manifest` in `Makefile` to include `tools/oomaya-ctl` and `tools/dwm-quickshell-state-bin`.
- Resolved platform portability edge cases across procps 4.0.4, PyGObject GLibUnix introspection, glibc fopen64, and locale sorting.
**Test results:**
- `make check-shell`: PASS
- `make check-install`: PASS
- `make check-install-manifest`: PASS
- `make check-install-preservation`: PASS (72 utility scripts installed, idempotency verified)
- `scripts/run-tests make check-core`: PASS (100% Green)
- `scripts/run-tests make check-shell-appearance`: PASS (100% Green)
- `scripts/run-tests make check-system`: PASS (100% Green, 689 unit tests pass)
**Deviations:** none  

### Review (filled by architect — read-only)

**Verdict:** `PASS`  
**Notes:**  
- Clean and surgical implementation of `install-scripts` target in `Makefile` and integration with `install-user` and `install.sh`.
- All acceptance criteria verified: dynamic `scripts/` discovery, `0755` permissions, idempotency, and exclusion of internal maintenance tooling (`quickshell-qmllint`, `run-tests`, `configure-build.sh`, `dev-sync-install.sh`) as well as privileged root helpers (`dwm-settings-display-root`).
- Manifest drift in `check-install-manifest` resolved for `tools/oomaya-ctl` and `tools/dwm-quickshell-state-bin`.
- Full 3-job parallel CI matrix passed with 100% Green (`check-core`, `check-shell-appearance`, `check-system`).
- Approved for remote push and pull request creation.
