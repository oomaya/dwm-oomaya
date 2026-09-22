# Phase 4: Upstream Tree Consolidation, Desktop Debloating & Lifecycle Hardening

## Overview
Following the successful implementation, benchmarking, and initial Quickshell integration of `dwm-oomayad` (<4ms state queries, 0 subprocess forks, 0.0% idle CPU), this next phase consolidates the IPC subsystem into the official upstream repository (`/home/rand/.local/src/dwm-oomaya`), debloats remaining desktop scripts (`dwm-keybind-exec`, `dwm-volume`), hardens systemd daemon lifecycle management, and establishes drop-in `dwm-msg` compatibility.

---

## User Review Required

> [!IMPORTANT]
> **Upstream Repository Integration**: All IPC daemon sources, headers, tests, and build tooling currently staged in `~/Documents/artifacts/dwm-oomaya` will be unified into `/home/rand/.local/src/dwm-oomaya`. The master `Makefile` will be updated so standard `make`, `make install`, and `make test` manage the IPC binaries alongside `dwm`.

> [!NOTE]
> **Preserving Non-Breaking Fallbacks**: All modified scripts retain robust fallbacks to legacy tools (`xprop`, `pactl`, `wpctl`, `xdotool`) if `dwm-oomayad` or `oomaya-ctl` is ever unavailable.

---

## Open Questions

1. **Systemd User Unit vs. autostart.sh**: Should `dwm-oomayad` be managed primarily as a systemd user service (`~/.config/systemd/user/dwm-oomayad.service`) or stay purely within `scripts/autostart.sh`? *(Recommended: Dual support—provide the systemd user unit, with `autostart.sh` falling back to detached execution).*
2. **`dwm-msg` Compatibility Symlink**: Should `tools/dwm-msg-compat.sh` be installed directly as `~/.local/bin/dwm-msg` to capture legacy scripts? *(Recommended: Yes, ensuring zero breakage for external callers).*

---

## Proposed Changes

### 1. Upstream Source Consolidation

Consolidate the complete IPC engine from the working artifacts staging directory into the official Git tree.

#### [NEW] [Headers](file:///home/rand/.local/src/dwm-oomaya/include)
- `include/oomaya_ipc.h`
- `include/oomaya_ring.h`
- `include/oomaya_rate.h`
- `include/oomaya_state.h`
- `include/oomaya_worker.h`
- `include/oomaya_reactor.h`
- `include/oomaya_reactor_impl.h`
- `include/oomaya_dispatch.h`
- `include/oomaya_x11_bridge.h`

#### [NEW] [Daemon & Library Sources](file:///home/rand/.local/src/dwm-oomaya/src)
- `src/ipc/protocol.c`
- `src/ipc/negotiate.c`
- `src/daemon/ring.c`
- `src/daemon/rate.c`
- `src/daemon/state_cache.c`
- `src/daemon/worker_pool.c`
- `src/daemon/worker_dispatch.c`
- `src/daemon/reactor.c`
- `src/daemon/x11_bridge.c`
- `src/daemon/main.c`

#### [NEW] [Tools & Tests](file:///home/rand/.local/src/dwm-oomaya)
- `tools/oomaya-ctl.c`
- `tools/dwm-quickshell-state.c`
- `tools/dwm-msg-compat.sh`
- `tests/test_ipc_protocol.c`
- `tests/test_phase2_core.c`
- `tests/test_daemon_core.c`
- `tests/test_integration_live.c`
- `tests/bench_ipc_roundtrip.c`
- `tests/profile_suite_b.c`
- `Makefile.ipc`

#### [MODIFY] [Makefile](file:///home/rand/.local/src/dwm-oomaya/Makefile)
- Add targets `ipc`, `daemon`, `tools`, and `test-ipc` invoking `Makefile.ipc`.
- Add `dwm-oomayad`, `oomaya-ctl`, and `dwm-quickshell-state-bin` to `make install` and `make uninstall`.

---

### 2. Desktop Keybind & Audio Debloating

#### [MODIFY] [scripts/dwm-keybind-exec](file:///home/rand/.local/src/dwm-oomaya/scripts/dwm-keybind-exec)
- Replace synthetic `xdotool` key faking (`sleep 0.05 && xdotool key ...`) with instant `oomaya-ctl` commands for standard window management actions:
  - `view <tag>` -> `oomaya-ctl view <tag>`
  - `focusstack` / `focus` -> direct focus IPC
  - `killclient` -> `oomaya-ctl kill`
- Fallback to `xdotool` only for non-IPC actions.

#### [MODIFY] [scripts/dwm-volume](file:///home/rand/.local/src/dwm-oomaya/scripts/dwm-volume)
- Update `get_volume` and `status` to query `oomaya-ctl -j audio` first, eliminating `pactl`/`wpctl` subshell pipes on volume key presses.
- Update volume set to invoke `oomaya-ctl volume <level>`.

---

### 3. Lifecycle Hardening

#### [NEW] [systemd/dwm-oomayad.service](file:///home/rand/.local/src/dwm-oomaya/systemd/dwm-oomayad.service)
- Define standard systemd user unit for `dwm-oomayad` with socket cleanup and restart policy.

#### [MODIFY] [scripts/autostart.sh](file:///home/rand/.local/src/dwm-oomaya/scripts/autostart.sh)
- Check `systemctl --user is-active dwm-oomayad` before falling back to manual `start_detached_once`.

---

## Verification Plan

### Automated Tests
1. **Unit & Concurrency Tests**:
   ```bash
   make -C /home/rand/.local/src/dwm-oomaya -f Makefile.ipc test_all
   ```
2. **IPC Roundtrip Latency Check**:
   ```bash
   /home/rand/.local/src/dwm-oomaya/tests/bench_ipc_roundtrip
   ```
3. **Repository Regression Suite**:
   ```bash
   cd /home/rand/.local/src/dwm-oomaya && ./scripts/run-tests
   ```

### Manual & Interactive Verification
1. **Desktop Keybind Responsiveness**:
   - Test keybind triggers via `dwm-keybind-exec`. Verify zero `xdotool` process spawning.
2. **Audio Volume Control**:
   - Execute `dwm-volume status` and `dwm-volume up 5%`. Confirm Dunst notification displays updated volume immediately.
3. **Living Handoff Logging**:
   - Run `antigravity-handoff create` to checkpoint state across federated workstations.
