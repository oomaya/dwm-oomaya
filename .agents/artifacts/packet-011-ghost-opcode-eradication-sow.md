# SOW & Implementation Packet #011: Ghost OpCode Eradication & Protocol Hardening

**Date**: 2026-11-03 KST  
**Author**: Antigravity Tech Lead Pair  
**Repository**: `/home/rand/.local/src/dwm-oomaya-pr1` (Branch: `codex/ipc-event-stream`, PR #24)  
**Base Commit**: [`1fce114`](https://github.com/oomaya/dwm-oomaya/commit/1fce114b874d4eb090e58b22bb5ea5e6f0a3d974)  
**Subject**: Eradicate ghost audio opcodes from `negotiate.c`, harden worker error code semantics, and polish test coverage.

---

## 1. Context & Architectural Motivation

In PR #24 Target 2, audio processing was completely excised from `worker_dispatch.c` and `reactor.c`. However, forensic audit ([`wm-ipc-purity-antagonistic-review.md`](file:///home/rand/.local/src/dwm-oomaya-pr1/.agents/artifacts/wm-ipc-purity-antagonistic-review.md)) revealed:
1. **Ghost OpCode Classification**: `src/ipc/negotiate.c` still classifies `0x0203` (`IPC_CMD_SYS_GET_AUDIO`) and `0x0204` (`IPC_CMD_SYS_SET_VOLUME`) as valid worker-pool offload opcodes (`return 1`). When called, the reactor enqueues a task to the worker ring buffer, signals `eventfd`, wakes worker pthreads, and context-switches only for `worker_dispatch.c` to fail in `default:`.
2. **Fast-Path Rejection Violation**: Retired opcodes must be rejected immediately on the reactor fast-path (<25 µs) with `OOMAYA_IPC_ERR_OPCODE` without involving worker threads.
3. **Worker Error Distortion**: `worker_dispatch.c` default branch sets `task->resp_len = sizeof(int32_t)`, which distorts `payload_len` in error frames.
4. **Test Polish**: `tests/test-ipc-watch.sh` lacks test coverage for bare `oomaya-ctl watch` (zero arguments, defaulting to `WATCH_ALL`).

---

## 2. Work Breakdown & Action Items

### Task 1: Eradicate Ghost OpCodes from Protocol Layer
1. **`src/ipc/negotiate.c`**:
   - In `oomaya_neg_classify()`, remove `case IPC_CMD_SYS_GET_AUDIO:` and `case IPC_CMD_SYS_SET_VOLUME:`.
   - Any packet with opcode `0x0203` or `0x0204` will now fall into `default:` and return `-1` (unrecognized opcode).
   - In `reactor.c:dispatch_frame()`, this immediately triggers the fast-path error response `OOMAYA_IPC_ERR_OPCODE` (-3) with zero worker pool involvement.
2. **`include/oomaya_ipc.h`**:
   - Remove `IPC_CMD_SYS_GET_AUDIO = 0x0203` and `IPC_CMD_SYS_SET_VOLUME = 0x0204` from `oomaya_opcode_t` (or comment as retired).
3. **`tests/test_ipc_protocol.c`**:
   - In `t09_worker_pool_opcodes()`: Remove assertions expecting `GET_AUDIO` and `SET_VOLUME` to classify as `1`.
   - In `t10_unknown_opcode()`: Add assertion that `IPC_CMD_SYS_GET_AUDIO` (`0x0203`) and `IPC_CMD_SYS_SET_VOLUME` (`0x0204`) classify as `-1`.

### Task 2: Harden Error Code Semantics in `worker_dispatch.c`
1. **`src/daemon/worker_dispatch.c`**:
   - In `oomaya_default_dispatch()` `default:` case:
     ```c
     default: {
         task->status   = OOMAYA_IPC_ERR_OPCODE; /* -3 */
         task->resp_len = 0;
         break;
     }
     ```
   - This ensures that if any future unhandled opcode reaches a worker, `reactor.c:handle_eventfd()` constructs a compliant `OOMAYA_MSG_ERROR` frame with error code in `payload_len`.

### Task 3: Polish Test Suite
1. **`tests/test-ipc-watch.sh`**:
   - Add a test case verifying bare `oomaya-ctl watch` (no arguments) subscribes and receives tag and focus signals (validating `WATCH_ALL`).
2. **`tests/measure-ipc-consumers.sh`**:
   - Add a header notice indicating that audio is directly handled by `pactl` without daemon proxying, making `legacy` and `daemon` modes equivalent for volume.

---

## 3. Verification Gates

Execute in private sandbox / Xvfb:
```bash
cd /home/rand/.local/src/dwm-oomaya-pr1
make -f Makefile.ipc clean test_all
scripts/run-tests make check-ipc check-shell check-format check-ipc-watch
```

---

## 4. Git Delivery

```bash
git -C /home/rand/.local/src/dwm-oomaya-pr1 add -u
git -C /home/rand/.local/src/dwm-oomaya-pr1 commit -m "refactor(ipc): eradicate ghost audio opcodes and harden fast-path error rejection"
git -C /home/rand/.local/src/dwm-oomaya-pr1 push origin codex/ipc-event-stream
```
