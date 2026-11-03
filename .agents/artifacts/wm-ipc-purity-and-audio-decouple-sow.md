# SOW & Implementation Plan: `dwm-oomaya` WM IPC Purity & Audio Decoupling

**Date**: 2026-11-03 KST  
**Author**: Antigravity Tech Lead Pair  
**Repository**: `/home/rand/.local/src/dwm-oomaya-pr1` (worktree of `/home/rand/.local/src/dwm-oomaya`)  
**Branch**: `codex/ipc-event-stream` (PR #24, stacked on `codex/status-weather-wait` #23)  
**Subject**: Pure Window Manager IPC restoration, Zero-Subprocess C daemon enforcement, and full audio decoupling across dwm and Quickshell.

---

## 1. Executive Summary & Consensus Architecture

Following rigorous empirical review and historical audit between User and Antigravity:
1. **Core Purpose**: `dwm-oomayad` (`reactor.c`) and `oomaya-ctl` were created exclusively as a **sub-2ms sovereign Window Manager IPC bridge** across X11 (`dwm-oomaya`) and Wayland (`omarchy-ctl` over Hyprland/Niri).
2. **Audio Decoupling**: Audio belongs exclusively to the **PipeWire / WirePlumber ecosystem** (`pactl`, `wpctl`). Quickshell (`quickshell-oomaya`) already communicates natively with PipeWire via in-memory Qt/C++ bindings (`Quickshell.Services.Pipewire`), requiring zero subprocesses or IPC daemon proxies.
3. **PR1 State**: The streaming audio daemon code (`audio_watch.c`, `pactl subscribe` in `reactor.c`) was an experimental feature isolated in PR1 that introduced a +20ms regression. It never existed in `origin/main`.
4. **Target**: Completely excise all audio code from the C core and CLI tools, establishing `dwm-oomayad` as a pure, zero-subprocess X11 window manager state streaming daemon.

---

## 2. Progress & Current State

- [x] **Target 1: Shell Consumers Decoupled & Verified (GREEN)**:
  - `scripts/dwm-volume`: Decoupled to direct `pactl`/`wpctl`. (Done & shfmt clean)
  - `scripts/dwm-status`: Decoupled to direct `pactl subscribe`. (Done & shfmt clean)
  - `tests/test-audio-consumers.sh`: Updated assertions and verified via `scripts/run-tests`. (PASS)
- [x] **Target 2: C Core & CLI Purified as Pure WM IPC (COMPLETE)**:
  - `src/daemon/reactor.c`: Audio watch and timerfd excised. Zero-subprocess epoll verified.
  - `src/daemon/worker_dispatch.c`: Excised `IPC_CMD_SYS_GET_AUDIO` and `IPC_CMD_SYS_SET_VOLUME`.
  - `tools/oomaya-ctl.c`: Purified CLI by removing audio/volume commands and narrowing `watch` to `[tag|layout|focus]`.
  - `tests/test-ipc-watch.sh`: Refactored to test tag, layout, and focus streams strictly without audio stubs.
  - Removed obsolete `src/daemon/audio_watch.c` and `include/oomaya_audio_watch.h`.

---

## 3. Work Packet Breakdown (Target 2 Implementation)

### Task 1: Purify `tools/oomaya-ctl.c`
1. **Usage & Help**:
   - Remove `audio`, `get-audio`, `volume`, `set-volume` from usage output and comments.
   - Update description: pure window manager control and event watching.
2. **Watch Filter Parsing (`parse_watch_filter`)**:
   - Remove `WATCH_AUDIO` (`1u << 0`).
   - Define `WATCH_ALL = (WATCH_TAG | WATCH_LAYOUT | WATCH_FOCUS)`.
   - Disallow `audio` filter keyword; return error if passed.
3. **Event Stream Loop (`run_watch`)**:
   - Remove `IPC_SIG_SYS_AUDIO_CHANGED` switch case.
   - Retain and verify `IPC_SIG_WM_TAG_CHANGED`, `IPC_SIG_WM_LAYOUT_CHANGED`, `IPC_SIG_WM_FOCUS_CHANGED`.
4. **Command Dispatch & Response Handling**:
   - Remove `strcmp(cmd, "audio")` and `strcmp(cmd, "volume")` branches.
   - Remove `IPC_CMD_SYS_GET_AUDIO` payload formatting block.

### Task 2: Purify `src/daemon/worker_dispatch.c` & `reactor.c`
1. **`worker_dispatch.c`**:
   - Remove `case IPC_CMD_SYS_GET_AUDIO:` and `case IPC_CMD_SYS_SET_VOLUME:`.
   - Remove `query_audio()` and `set_audio_volume()` helper functions (which used `popen("wpctl ...")`).
2. **`src/daemon/reactor.c` & `include/oomaya_reactor_impl.h`**:
   - Confirm complete removal of `audio_watch.h`, `sys/timerfd.h`, and `audio_*` state fields.
   - Confirm epoll loop monitors exclusively: `listen_fd`, `x11_fd`, `stop_fd`, and `workers->eventfd`. Zero subprocesses spawned.

### Task 3: Obsolete File Removal & Build System
1. Delete `src/daemon/audio_watch.c` and `include/oomaya_audio_watch.h`.
2. Verify `Makefile.ipc` contains no references to `audio_watch.o`.

### Task 4: Test Suite Refactoring
1. **`tests/test-ipc-watch.sh`**:
   - Remove audio stub scripts (`wpctl`, `pactl`, `events`).
   - Refactor test flow to launch `dwm-oomayad`, subscribe to `oomaya-ctl watch tag layout focus`, trigger WM mutations via `oomaya-ctl view <N>`, and assert JSON lines arrive instantaneously (<2ms).
2. **`tests/test-ipc-tools-build.sh`**:
   - Verify `oomaya-ctl` compiles cleanly without audio symbols.

### Task 5: Gate Verification & Push
Execute in private sandbox / Xvfb:
```bash
cd /home/rand/.local/src/dwm-oomaya-pr1
make -f Makefile.ipc clean
scripts/run-tests make clean all
scripts/run-tests make check
scripts/run-tests make test-ipc
make check-format
```
Commit and push:
```bash
git add -u
git rm src/daemon/audio_watch.c include/oomaya_audio_watch.h
git commit -m "refactor(ipc): dedicate dwm-oomayad C core to pure WM event streaming"
git push origin codex/ipc-event-stream
```

---

## 4. Key References & File Links

- Plan & Spec: [`wm-ipc-purity-and-audio-decouple-sow.md`](file:///home/rand/.local/src/dwm-oomaya-pr1/.agents/artifacts/wm-ipc-purity-and-audio-decouple-sow.md)
- Worktree Root: [`/home/rand/.local/src/dwm-oomaya-pr1`](file:///home/rand/.local/src/dwm-oomaya-pr1)
- Daemon Reactor: [`src/daemon/reactor.c`](file:///home/rand/.local/src/dwm-oomaya-pr1/src/daemon/reactor.c)
- Worker Dispatch: [`src/daemon/worker_dispatch.c`](file:///home/rand/.local/src/dwm-oomaya-pr1/src/daemon/worker_dispatch.c)
- IPC Controller CLI: [`tools/oomaya-ctl.c`](file:///home/rand/.local/src/dwm-oomaya-pr1/tools/oomaya-ctl.c)
- IPC Watch Test: [`tests/test-ipc-watch.sh`](file:///home/rand/.local/src/dwm-oomaya-pr1/tests/test-ipc-watch.sh)
- Decoupled Scripts: [`scripts/dwm-volume`](file:///home/rand/.local/src/dwm-oomaya-pr1/scripts/dwm-volume) & [`scripts/dwm-status`](file:///home/rand/.local/src/dwm-oomaya-pr1/scripts/dwm-status)
- Quickshell Controls Model: [`quickshell/controls/ControlsModel.qml`](file:///home/rand/.local/src/quickshell-oomaya/quickshell/controls/ControlsModel.qml#L39)
- Active PR: [PR #24 on GitHub](https://github.com/oomaya/dwm-oomaya/pull/24)
