# PR-1 Payload Contract Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `OOMAYA_MAX_PAYLOAD_LEN` (512) the single authoritative v1 IPC payload limit, and make the reactor treat a frame whose header violates the frame contract (oversize payload, bad magic/version) as a connection-ending protocol violation so the stream can never desync.

**Architecture:** One shared constant in `include/oomaya_ipc.h`; `include/oomaya_ring.h` includes it and drops its hardcoded `512` literals; `src/daemon/reactor.c` validates the header as soon as it is fully received and closes the connection on violation (v1 cannot trust stream alignment after a rejected frame, because bytes of the rejected body may remain in the socket).

**Tech Stack:** C99/C11, existing makefile `Makefile.ipc`, existing unit harness `tests/test_daemon_core.c`, existing integration harness `tests/test_integration_live.c` (full reactor in-process over UDS; requires X11 headers at link time via `x11_bridge.o` but no running X server, since bridge init is never called).

**Spec:** `docs/DAEMON-IMPROVEMENT-PLAN.md` (PR-1 row + OQ2 resolved decision; C3 finding). The plan argues from that document; executors read both.

## Global Constraints

- C99 style, compile with warnings enabled (AGENTS.md "Build and Code Standards").
- No new library dependencies.
- Branch: `codex/payload-contract`, created from the current `codex/daemon-improvement-plan` branch (stacked, per AGENTS.md; this repo's `main` is not yet updated with the plan doc this PR depends on). PR initially targets `codex/daemon-improvement-plan`, retargeted to `main` after it merges.
- Worktree isolation via `superpowers:using-git-worktrees` at execution time.
- Run `git status --short` before each editing task; preserve unrelated changes.
- Never run `install.sh` locally; everything through the managed test workspace.
- Final gates: `scripts/run-tests make clean all` then `scripts/run-tests`.
- ASCII punctuation in source and docs.
- Deferred on purpose: `SPEC.md` wire-format documentation (Phase C item 3); rate-limit error code (PR-6). A 513-byte frame changing behavior from "silent truncation" to "error + disconnect" is intentional and must be stated in the PR body as a migration note.

## Review Focus

1. A frame of exactly 512 payload bytes must succeed end-to-end (accept, not off-by-one reject). Pinned by F01/T01.
2. A frame of 513 payload bytes must get `ERR_PAYLOAD` and a closed connection, and a subsequent reconnect must work cleanly (no desync inherited). Pinned by F02/T02/T03.
3. The `oomaya_task_t` struct layout must not change size (both literals replaced by a same-valued constant only). Pinned by G01 grep check + `test3` still passing.
4. A fast-path unknown-opcode frame must still get `ERR_OPCODE` with the connection left open (only frame-contract violations close). Not covered by existing tests - pinned by T04, added to `test_integration_live.c`.
5. The existing validate-then-dispatch double-check must keep rejecting malformed bodies identically after the early check lands (no behavior drift for bad-magic frames other than the added disconnect). Pinned by T02 asserting the error header echo (msg_id).

---

### Task 0: Branch setup in worktree

**Files:**
- Create: none (process task)

**Interfaces:**
- Produces: a worktree checked out at branch `codex/payload-contract`.

- [ ] **Step 1: Create a git worktree from the stacked base**

Run: `git worktree add ../dwm-oomaya-pr1 -b codex/payload-contract codex/daemon-improvement-plan` (or the superpowers worktree tool). All later tasks run inside it.

- [ ] **Step 2: Verify clean state**

Run: `git status --short` (empty), `git log --oneline -1` (shows codex/daemon-improvement-plan head `e41b7cc`).

---

### Task 1: Failing boundary tests (unit)

**Files:**
- Modify: `tests/test_daemon_core.c` (new test group; reuses the file's existing `TEST`/`ASSERT_EQ` macros and its existing header-construction helper pattern from the REACT group)

**Interfaces:**
- Consumes: `oomaya_frame_validate(const oomaya_ipc_header_t *hdr)` -> int, `errno`-style codes `OOMAYA_IPC_OK (0)`, `OOMAYA_IPC_ERR_PAYLOAD (-4)` from `src/ipc/protocol.c`.

- [ ] **Step 1: Write the failing tests**

Add a new group `FRAME F01-F04` to `tests/test_daemon_core.c`, using the existing header-construction helper already in that file (do not invent a new one):

- F01 `frame_validate_512_ok`: header with valid magic/version and `payload_len == OOMAYA_FRAME_BODY_MAX` -> `ASSERT_EQ(..., oomaya_frame_validate(&hdr), OOMAYA_IPC_OK)`
- F02 `frame_validate_513_rejected`: same header with `payload_len == OOMAYA_FRAME_BODY_MAX + 1` -> returns `OOMAYA_IPC_ERR_PAYLOAD`
- F03 `frame_validate_4096_rejected`: same header with `payload_len == 4096` -> returns `OOMAYA_IPC_ERR_PAYLOAD`
- F04 `negotiate_transformer_513_rejected`: `oomaya_neg_v1_to_v2_req(src, 513, dst, &dst_len)` -> returns `OOMAYA_IPC_ERR_PAYLOAD` (pins the transformer bound; passes before and after, included as a regression pin)

If `oomaya_frame_validate` currently returns a code other than `OOMAYA_IPC_ERR_PAYLOAD` on the oversize branch, the task also includes correcting `src/ipc/protocol.c`'s oversize branch to `OOMAYA_IPC_ERR_PAYLOAD` - the acceptance boundary (512) is decided; the error code for it must be the one the enum documents as "payload_len exceeds maximum".

- [ ] **Step 2: Run to verify only F02/F03 fail**

Run: `make -f Makefile.ipc test3`
Expected: F02 and F03 FAIL (current limit is 4096 so 513/4096 are accepted), F01 and F04 PASS; daemon worker tests keep passing too.

- [ ] **Step 3: Commit**

```bash
git add tests/test_daemon_core.c src/ipc/protocol.c
git commit -m "test(ipc): assert frame payload boundary rejects 513 and 4096"
```

---

### Task 2: Unify the payload constant

**Files:**
- Modify: `include/oomaya_ipc.h:99-102` (reorder definitions, single literal)
- Modify: `include/oomaya_ring.h:69,73` (literal `512` -> `OOMAYA_FRAME_BODY_MAX`, add `#include "oomaya_ipc.h"`)

**Interfaces:**
- Consumes: definitions from Task 1 tests.
- Produces: `OOMAYA_MAX_PAYLOAD_LEN` == `OOMAYA_FRAME_BODY_MAX` == `512u` as the only payload limit; `oomaya_task_t.payload`/`resp_data` size derived from it.

- [ ] **Step 1: Reorder and reduce `oomaya_ipc.h`**

`OOMAYA_FRAME_BODY_MAX` (`512u`) is defined first, then `#define OOMAYA_MAX_PAYLOAD_LEN OOMAYA_FRAME_BODY_MAX`, with a one-line comment: "v1 wire limit; a frame larger than the in-band body buffer is a contract violation and disconnects the client (see docs/DAEMON-IMPROVEMENT-PLAN.md PR-1)."

- [ ] **Step 2: Replace ring literals**

In `include/oomaya_ring.h`, `payload[512]` -> `payload[OOMAYA_FRAME_BODY_MAX]`, `resp_data[512]` -> `resp_data[OOMAYA_FRAME_BODY_MAX]`; add the include directive with the file's existing include style.

- [ ] **Step 3: G01 - no stray literals remain**

Run: `grep -n '512' include/oomaya_ipc.h include/oomaya_ring.h include/oomaya_reactor_impl.h src/ipc/negotiate.c src/ipc/protocol.c src/daemon/reactor.c`
Expected: only the single `512u` literal in `oomaya_ipc.h` (and non-size uses such as `16` header constants elsewhere are untouched).

- [ ] **Step 4: Run tests to verify green**

Run: `make -f Makefile.ipc test3`
Expected: all FRAME/STATE/WORKER/REACT/PATH groups PASS.

- [ ] **Step 5: Commit**

```bash
git add include/oomaya_ipc.h include/oomaya_ring.h
git commit -m "fix(ipc): unify v1 payload limit on OOMAYA_FRAME_BODY_MAX (512)"
```

---

### Task 3: Reactor closes on frame-contract violation

**Files:**
- Modify: `src/daemon/reactor.c` (`handle_client_readable`, CS_WANT_HDR branch, after the full 16-byte header is received; `dispatch_frame` keeps its existing validate as defense in depth)

**Interfaces:**
- Consumes: `oomaya_frame_validate`, `oomaya_frame_init_error(dst, src, code)`, `send_response`, `client_slot_free` all already in this file.
- Produces: behavior contract - after a complete header fails validation, the daemon sends the error frame once and then closes the connection. No other error paths change.

- [ ] **Step 1: Add the early contract gate**

In `handle_client_readable`, immediately after the full header is consumed and before `payload_len == 0` dispatch logic: call `oomaya_frame_validate(&c->hdr)`; if it returns non-OK, build the error reply with the returned code (not hardcoded `MALFORMED`), `send_response(c->fd, &err, NULL, 0)`, then `client_slot_free(r, c)` and return. Add a comment: "v1 disconnects clients whose header violates the frame contract; rejected-frame body bytes may still be in flight and would desync the stream."

- [ ] **Step 2: Compile**

Run: `make -f Makefile.ipc daemon` (and `test3` links untouched files) - expected: no warnings, no errors.

- [ ] **Step 3: Commit**

```bash
git add src/daemon/reactor.c
git commit -m "fix(daemon): disconnect clients on frame-contract violations"
```

---

### Task 4: Integration desync tests (red -> already green after Tasks 2-3)

**Files:**
- Modify: `tests/test_integration_live.c` (new tests, reusing its UDS client pattern and `ASSERT_EQ` harness)

**Interfaces:**
- Consumes: in-process reactor/client socket plumbing already present in the file.
- Produces: T01-T04 assertions described below.

- [ ] **Step 1: Write T02/T03 first (the desync pair), then T01/T04 alignment**

- T01 `getstate_512_boundary_ok`: send a valid `WM_GET_STATE` frame carrying `payload_len == OOMAYA_FRAME_BODY_MAX` with a 512-byte body whose first two bytes encode a valid fast-path opcode; expect a RESPONSE header back.
- T02 `oversize_frame_gets_error_and_disconnect`: send a well-formed header with `payload_len == 513` (valid magic so only the payload rule trips); expect a received error-frame header with `msg_type == OOMAYA_MSG_ERROR`, echoed `msg_id`, then `recv` returns 0 (disconnect).
- T03 `reconnect_after_oversize_is_healthy`: reconnect and send `WM_GET_STATE`; expect the correct response body (tag mask `0x1234` per the harness setup) - proves no desync survived.
- T04 `unknown_opcode_stays_connected`: send a fast-path-class header with an unknown opcode; expect `OOMAYA_MSG_ERROR` response and the connection still usable (one more `WM_GET_STATE` succeeds).

- [ ] **Step 2: Run to verify green**

Run: `make -f Makefile.ipc test_live`
Expected: all PASS (Tasks 2-3 already implement the behavior; this task exists so the fix has coverage that would have failed without it - if any of T02/T03 still fail, go back and diagnose with superpowers:systematic-debugging before proceeding).

- [ ] **Step 3: Commit**

```bash
git add tests/test_integration_live.c
git commit -m "test(ipc): oversize frame disconnects cleanly and reconnect recovers"
```

---

### Task 5: Full gates, push, open PR

**Files:**
- Modify: none (finalization)

- [ ] **Step 1: Full managed gates**

Run: `scripts/run-tests make clean all`
Expected: build clean.
Run: `scripts/run-tests`
Expected: full suite passes. Anything that fails is diagnosed with superpowers:systematic-debugging before continuing; anything that cannot run is stated exactly in the PR.

- [ ] **Step 2: Cleanliness check**

Run: `git status --short` (empty), `git log --oneline codex/daemon-improvement-plan..HEAD` (exactly the 4 commits from Tasks 1-4).

- [ ] **Step 3: Push and open PR**

```bash
git push -u origin codex/payload-contract
gh pr create --title "fix(daemon): unify v1 payload contract on 512 and disconnect on frame violations" --body "Fixes C3 from docs/DAEMON-IMPROVEMENT-PLAN.md (PR-1). ... migration note: frames with payload_len > 512 now get ERR_PAYLOAD and a disconnect instead of silent truncation; boundary tests F01-F04 and integration tests T01-T04 included; gates: scripts/run-tests make clean all, scripts/run-tests."
```

Target branch: `codex/daemon-improvement-plan` (stacked), retarget to `main` after that merges.
