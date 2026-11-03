# dwm-oomaya & quickshell-oomaya Installation Idempotency Verification Report

- **Date**: 2026-11-04 KST
- **Author**: dwm-oomaya Tech Lead Pair
- **Base Commits**:
  - `dwm-oomaya-pr1`: `8cb0cfa` (`main`)
  - `quickshell-oomaya`: `3508c5e` (`main`)
- **Execution Boundaries**:
  - Core: `/home/rand/.local/src/dwm-oomaya-pr1`
  - Quickshell: `/home/rand/.local/src/quickshell-oomaya`
  - Off-Limits: `~/dwm-oomaya`, `~/.local/src/dwm-oomaya` (Protected)

---

## 1. Executive Summary & Verification Matrix

| Target Component | Test Target / Invocations | Local Exit Code | GitHub CI Status | Idempotency / Preservation Verdict |
| :--- | :--- | :--- | :--- | :--- |
| **dwm-oomaya-pr1** | `make check-install check-install-preservation` | **0** | **Green** (Run #196, Commit `8cb0cfa`) | **PASS** (Manifest symmetric, user configs preserved) |
| **quickshell-oomaya** | Repeated `install` / `uninstall` (Isolated `CFG_DIR`, `BIN_DIR`) | **0** | **Green** (Commit `3508c5e`) | **PASS** (100% snapshot parity across repeated installs) |

---

## 2. dwm-oomaya-pr1 Verification Details

### 2.1 Execution Command
```bash
make -C /home/rand/.local/src/dwm-oomaya-pr1 check-install check-install-preservation
```

### 2.2 Empirical Results
1. **Manifest Symmetry & Clean Uninstall (`check-install`)**:
   - Staged installation to isolated `DESTDIR` validated against expected binary and asset manifest.
   - All 66 utility scripts, core binaries (`dwm`, `dwm-oomayad`, `oomaya-ctl`), desktop entry (`dwm.desktop`), and Capitaine cursor theme variants verified.
   - Symmetrical uninstallation confirmed: `cmp "$before" "$after"` exited 0.
2. **Configuration & State Preservation (`check-install-preservation`)**:
   - Direct `root` installation rejected prior to modifying any paths.
   - Stale build inputs and missing binary preflight checks verified.
   - Two consecutive executions of `install-user` performed over existing configurations.
   - User configuration files verified preserved via SHA256 snapshot comparison:
     - `config.h`
     - `.xinitrc`
     - `dwm-oomaya/hotkeys.toml`
     - `dwm-oomaya/themes.toml`
     - `dwm-oomaya/window-rules.toml`
     - `picom/picom.conf`
     - `Thunar/uca.xml`
     - `autostart/picom.desktop`
     - `systemd/user/custom.service`
   - Verdict: `Repeated install preservation: PASS` (Local Exit Code 0).

---

## 3. quickshell-oomaya Verification Details

### 3.1 Execution Command & Test Isolation
Executed isolated multi-pass installation and teardown harness under `/tmp/quickshell_idempotency_test.*` with custom `CFG_DIR` and `BIN_DIR`:
1. **First Pass (`make install`)**:
   - `CFG_DIR/quickshell` created and populated with full QML assets.
   - 8 helper scripts (`dwm-quickshell-launcher`, `dwm-quickshell-controls`, `dwm-quickshell-controlcenter`, `dwm-quickshell-network`, `dwm-quickshell-pointer`, `dwm-quickshell-state`, `dwm-quickshell-version-check`, `quickshell-qmllint`) and `dwm-quickshell-state-bin` installed with `0755` permissions.
   - Pre-existing unrelated configuration (`other_app/settings.conf`) and binaries (`unrelated_tool`) remained completely untouched.
2. **Second Pass (`make install` - Update / Idempotency)**:
   - Re-executed against the existing installation.
   - Full filesystem snapshot diff: **0 differences** (100% idempotent).
3. **Third Pass (`make uninstall`)**:
   - Removed `quickshell` configuration directory cleanly.
   - Removed all 8 managed helper scripts and `dwm-quickshell-state-bin`.
   - Pre-existing files in `CFG_DIR` and `BIN_DIR` remained intact.
4. **Fourth Pass (`make uninstall` - Idempotent Teardown)**:
   - Re-executed against empty/uninstalled state: exited with code 0 without errors.
5. **Fifth Pass (Re-installation)**:
   - Restored full configuration and scripts cleanly, matching first install snapshot 100%.

---

## 4. Architectural Discovery: `mv-shim` Teardown Edge Case

### 4.1 Root Cause Analysis
In `quickshell-oomaya/Makefile`:
```makefile
uninstall:
...
	@if [ -f "$(BIN_DIR)/mv" ]; then \
		if "$(BIN_DIR)/mv" --help 2>&1 | grep -q 'renameat2'; then \
			rm -f "$(BIN_DIR)/mv"; \
		fi; \
	fi
```
- On platforms with GNU Coreutils < 9.5, `make install` installs `tools/mv-shim` to `$(BIN_DIR)/mv`.
- During `make uninstall`, the Makefile checks `mv --help` for the keyword `renameat2` to verify it is deleting the shim rather than a vendor `mv`.
- In `tools/mv-shim.c`, the `--help` string is:
  ```c
  printf("Usage: mv [OPTION]... SOURCE DEST\n"
         "  --exchange    exchange source and destination\n"
         "  --no-copy     do not copy if exchange fails\n"
         "  --update=none-fail fail if destination exists\n"
         "  none-fail     fail if destination exists\n");
  ```
- Because `--help` does **not** contain the word `renameat2` (it only appears in `perror("mv: renameat2")`), `grep -q 'renameat2'` fails. Consequently, `$(BIN_DIR)/mv` is never removed on Coreutils < 9.5 fleet nodes upon uninstallation.
- On Fedora 44 (the workstation), Coreutils >= 9.5 is present, so `mv-shim` is skipped and this issue does not manifest locally.

### 4.2 Resolution (Option A Implemented)
- **Status**: Implemented & Committed (`d1751dc`).
- **Modification**: Updated `tools/mv-shim.c` help output:
  ```c
  printf("Usage: mv (quickshell renameat2 shim) [OPTION]... SOURCE DEST\n"
  ```
- **Validation**:
  - Re-compiled `tools/mv-shim` and tested `--help` (contains `renameat2`).
  - Tested `make uninstall` with `tools/mv-shim` installed as `$(BIN_DIR)/mv` in an isolated stage: cleanly removed.
  - Tested `make uninstall` with simulated vendor `mv`: safely preserved without accidental deletion.
