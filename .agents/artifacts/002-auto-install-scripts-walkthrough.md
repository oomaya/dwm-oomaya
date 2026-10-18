# Packet #002 Walkthrough: Auto-Install `scripts/` into `~/.local/bin/`

## Executive Summary

- **Task Packet**: Packet #002 (`.agents/tasks/002-auto-install-scripts-into-local-bin.md`)
- **Status**: Completed & 100% Green
- **Branch**: `packet/002-auto-install-scripts-into-local-bin`
- **Scope**: Automated, idempotent deployment of utility scripts in `scripts/` to `${USER_HOME}/.local/bin` via `make install`, `make install-user`, and `install.sh` with `0755` permissions, exclusion of internal development tools, resolution of install manifest drift, and full passing across all 3 Phase 1 parallel CI matrix jobs (`check-core`, `check-shell-appearance`, `check-system`).

---

## Key Achievements & Implementation Details

### 1. `Makefile` Installation Target & User Wireup
- **Dynamic User Utility Deployment**: Implemented `install-scripts:` target in [Makefile](file:///home/rand/.local/src/dwm-oomaya/Makefile#L687) that discovers all executable files in `scripts/`, excludes internal development tools (`quickshell-qmllint`, `run-tests`, `configure-build.sh`, `dev-sync-install.sh`) and privileged root helpers (`dwm-settings-display-root`), and installs them with mode `0755` into `${USER_HOME}/.local/bin/`.
- **Install Recipe Safety**: Wired `install-user:` to invoke `$(MAKE) install-scripts`, honoring `DESTDIR`, `@test "$$(id -u)" -ne 0`, and avoiding root `chown` violations as strictly required by `tests/test-install-preservation.sh`.
- **Manifest Drift Resolution**: Updated `INSTALL_COMMANDS` and `check-install-manifest` in `Makefile` to include newly added commands (`tools/oomaya-ctl`, `tools/dwm-quickshell-state-bin`), bringing the manifest contract into 100% parity.

### 2. End-to-End Installer Integration (`install.sh`)
- Updated [install.sh](file:///home/rand/.local/src/dwm-oomaya/install.sh#L160) to explicitly invoke `make install-scripts` during existing-system installation, providing clear console feedback on installed utilities.

### 3. Cross-Platform Hardening & Subsystem Resilience

During execution and validation against the 3-job CI test suite, several systemic edge cases and cross-distribution behavioral divergences were diagnosed and hardened:

1. **Procps-NG Compatibility & Process Inspection**:
   - In Debian/Ubuntu/Peppermint (procps 4.0.4), `pgrep` does not support `--env` and `pidwait` does not support `-p <pid>`.
   - Updated [scripts/dwm-quickshell-controlcenter](file:///home/rand/.local/src/dwm-oomaya/scripts/dwm-quickshell-controlcenter#L280) to inspect `/proc/$pid/environ` upon exit code 2.
   - Updated [scripts/dwm-xsettings](file:///home/rand/.local/src/dwm-oomaya/scripts/dwm-xsettings#L30) with `pidwait -F` fallback.
2. **Lock Separation in `dwm-xsettings`**:
   - Disentangled directory creation (`ensure_runtime_dir`) from runtime locking (`ensure_runtime`) to eliminate recursive deadlock on `dwm-xsettings.lock` during asynchronous session apply.
3. **Bash 5.2 Command Substitution Handling**:
   - Reordered `dwm-xdg-autostart` protocol header emission before variable assignment to prevent Bash 5.2 subshell syntax traps upon SIGTERM.
4. **PyGObject Introspection Normalization**:
   - In [scripts/dwm-system-management](file:///home/rand/.local/src/dwm-oomaya/scripts/dwm-system-management#L9360), normalized `GLibUnix.signal_add`, `GLibUnix.signal_add_full`, and `GLib.unix_signal_add` across differing PyGObject introspection bindings.
5. **Cross-Distro GLibc Interposition in Tests**:
   - In [tests/test-system-management.py](file:///home/rand/.local/src/dwm-oomaya/tests/test-system-management.py#L3640), interposed both `fopen` and `fopen64` to maintain mountinfo polling assertions across all glibc ABI versions.
6. **Deterministic ASCII Sorting**:
   - Added `LC_ALL=C sort -u` in [tests/test-quickshell-system-management.sh](file:///home/rand/.local/src/dwm-oomaya/tests/test-quickshell-system-management.sh#L184) to eliminate locale-dependent casing permutations between `updates` and `updateSummary`.

---

## Validation Results Matrix

All validation gates were executed under `scripts/run-tests` and verified with exit code 0:

| Gate / Matrix Job | Test Target | Result | Details |
| :--- | :--- | :---: | :--- |
| **Shell & Lints** | `make check-shell` | **PASS** | 0 warnings, ShellCheck passed |
| **Install Contract** | `make check-install` | **PASS** | `DESTDIR` & staging verified |
| **Manifest Contract** | `make check-install-manifest` | **PASS** | 100% manifest consistency |
| **Preservation** | `make check-install-preservation` | **PASS** | 72 utility scripts installed cleanly |
| **CI Matrix Job 1** | `scripts/run-tests make check-core` | **PASS** | Core wm, IPC, and autostart verified |
| **CI Matrix Job 2** | `scripts/run-tests make check-shell-appearance` | **PASS** | Quickshell, design system & themes verified |
| **CI Matrix Job 3** | `scripts/run-tests make check-system` | **PASS** | 689 unit tests & settings contracts verified |

---

## File Changes Summary

```text
 Makefile                                           | 61 +++++++++++++++++-----
 install.sh                                         |  5 ++
 scripts/dmenu-find                                 |  0 (chmod +x)
 scripts/dwm-cursor-reload                          |  0 (chmod +x)
 scripts/dwm-polkit                                 |  0 (chmod +x)
 scripts/dwm-quickshell-controlcenter               | 14 ++++-
 scripts/dwm-system-management                      |  9 ++--
 scripts/dwm-utils.sh                               |  2 +-
 scripts/dwm-xdg-autostart                          |  5 +-
 scripts/dwm-xsettings                              | 26 ++++++---
 scripts/seed-default-apps.sh                       |  0 (chmod +x)
 tests/fixtures/system-regional-interruption-bus.py |  4 +-
 tests/test-fedora-packages.sh                      | 22 ++++----
 tests/test-fedora-platform.sh                      |  5 +-
 tests/test-quickshell-controls.sh                  |  6 +++
 tests/test-quickshell-power-backend.sh             | 31 ++++++++++-
 tests/test-quickshell-system-management.sh         |  2 +-
 tests/test-system-management.py                    | 53 ++++++++++++++-----
 18 files changed, 189 insertions(+), 57 deletions(-)
```
