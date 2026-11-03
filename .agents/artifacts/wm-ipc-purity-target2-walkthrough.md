# Walkthrough: Target 2 — WM IPC Purity & C Core Audio Decoupling

**Date**: 2026-11-03 KST  
**Repository**: [`dwm-oomaya-pr1`](file:///home/rand/.local/src/dwm-oomaya-pr1) (`codex/ipc-event-stream`, PR #24)  
**ADR Reference**: [`ipc-bridge-architectural-intent-and-blameless-retrospective.md`](file:///home/rand/.local/src/dwm-oomaya-pr1/.agents/artifacts/ipc-bridge-architectural-intent-and-blameless-retrospective.md)

---

## 1. Executive Summary

Following the canonical ADR and the Four Sovereign IPC Architectural Laws, Target 2 has been completed across the C daemon (`dwm-oomayad`), CLI controller (`oomaya-ctl`), and verification suite:
1. **Zero-Subprocess Epoll Reactor**: [`src/daemon/reactor.c`](file:///home/rand/.local/src/dwm-oomaya-pr1/src/daemon/reactor.c) and [`include/oomaya_reactor_impl.h`](file:///home/rand/.local/src/dwm-oomaya-pr1/include/oomaya_reactor_impl.h) monitor exclusively `listen_fd`, `x11_fd`, `stop_fd`, `workers->eventfd`, and client sockets.
2. **Worker Pool Purification**: [`src/daemon/worker_dispatch.c`](file:///home/rand/.local/src/dwm-oomaya-pr1/src/daemon/worker_dispatch.c) excised `query_audio()`, `set_audio_volume()`, `IPC_CMD_SYS_GET_AUDIO`, and `IPC_CMD_SYS_SET_VOLUME`. Unimplemented worker opcodes now fail-closed (`task->status = -1`).
3. **Pure WM Controller CLI**: [`tools/oomaya-ctl.c`](file:///home/rand/.local/src/dwm-oomaya-pr1/tools/oomaya-ctl.c) removed `audio`, `get-audio`, `volume`, `set-volume`, and `WATCH_AUDIO`, restricting `oomaya-ctl watch` strictly to `[tag|layout|focus]`.
4. **Dead File & Build Cleanup**: Deleted `src/daemon/audio_watch.c` and `include/oomaya_audio_watch.h`, and cleaned [`Makefile.ipc`](file:///home/rand/.local/src/dwm-oomaya-pr1/Makefile.ipc).
5. **Test Suite Alignment**: Refactored [`tests/test-ipc-watch.sh`](file:///home/rand/.local/src/dwm-oomaya-pr1/tests/test-ipc-watch.sh), [`tests/test-audio-consumers.sh`](file:///home/rand/.local/src/dwm-oomaya-pr1/tests/test-audio-consumers.sh), and [`tests/test-ipc-tools-build.sh`](file:///home/rand/.local/src/dwm-oomaya-pr1/tests/test-ipc-tools-build.sh) to verify pure WM event streaming (`tag`, `layout`, `focus`) and confirm rejection of audio commands in `oomaya-ctl`.

---

## 2. Structural Changes Summary

| File | Action | Details |
| :--- | :--- | :--- |
| [`include/oomaya_ipc.h`](file:///home/rand/.local/src/dwm-oomaya-pr1/include/oomaya_ipc.h) | Modified | Removed experimental `IPC_SIG_SYS_AUDIO_CHANGED` (`0x0304`) signal opcode |
| [`tools/oomaya-ctl.c`](file:///home/rand/.local/src/dwm-oomaya-pr1/tools/oomaya-ctl.c) | Modified | Removed `audio`/`volume` commands and `WATCH_AUDIO`; `watch` accepts only `tag`, `layout`, `focus` |
| [`src/daemon/worker_dispatch.c`](file:///home/rand/.local/src/dwm-oomaya-pr1/src/daemon/worker_dispatch.c) | Modified | Removed `popen("wpctl ...")` helpers and audio cases; default branch fails closed (`status = -1`) |
| [`src/daemon/reactor.c`](file:///home/rand/.local/src/dwm-oomaya-pr1/src/daemon/reactor.c) | Modified | Pure epoll loop without `audio_watch` or `timerfd` |
| [`include/oomaya_reactor_impl.h`](file:///home/rand/.local/src/dwm-oomaya-pr1/include/oomaya_reactor_impl.h) | Modified | Removed audio watch state fields |
| `src/daemon/audio_watch.c` | Deleted | Removed experimental `pactl subscribe` child manager |
| `include/oomaya_audio_watch.h` | Deleted | Removed experimental header |
| [`Makefile.ipc`](file:///home/rand/.local/src/dwm-oomaya-pr1/Makefile.ipc) | Modified | Removed `src/daemon/audio_watch.o` reference |
| [`tests/test-ipc-watch.sh`](file:///home/rand/.local/src/dwm-oomaya-pr1/tests/test-ipc-watch.sh) | Refactored | Pure WM `tag`/`focus` event streaming & deduplication verification |
| [`tests/test-audio-consumers.sh`](file:///home/rand/.local/src/dwm-oomaya-pr1/tests/test-audio-consumers.sh) | Updated | Asserts `oomaya-ctl` rejects `audio`/`volume`/`watch audio`; verifies direct `pactl` in `dwm-volume` & `dwm-status` |
| [`tests/test-ipc-tools-build.sh`](file:///home/rand/.local/src/dwm-oomaya-pr1/tests/test-ipc-tools-build.sh) | Updated | Asserts `oomaya-ctl --help` lists `watch` and excludes `audio` |

