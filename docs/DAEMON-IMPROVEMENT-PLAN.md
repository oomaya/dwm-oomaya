# Daemon Improvement Plan — dwm-oomayad (Reactor, X11 Bridge, IPC)

Status: planning document. No code changes are included yet.
Scope: `src/daemon/`, `src/ipc/`, `include/`, and the dwm <-> daemon root-atom
contract. Derived from a static review of `src/daemon/reactor.c`,
`src/daemon/x11_bridge.c`, `src/daemon/main.c`, `src/daemon/worker_pool.c`,
`src/ipc/negotiate.c`, and cross-checks against `dwm.c`, `config.def.h`,
and `config.h`.

## Review summary

The daemon architecture is sound overall: an epoll reactor with a fixed
zero-malloc client array, fail-closed rate limiting, an edge-triggered
listener/eventfd path with a level-triggered client path, C11 atomics in the
state cache, and an ABA-safe lock-free ring between reactor and worker pool.
The issues below are boundary-contract defects rather than design defects.

## Findings

### Critical

| ID | Summary | Location |
|----|---------|----------|
| C1 | `layout_symbols[]` order disagrees with `layouts[]` in `config.def.h` / `config.h` (`"><>"` at index 1 vs 13; every entry shifted). Symbol-to-index mapping stores a wrong `layout_idx` for all layouts except tile. | `src/daemon/x11_bridge.c:83-100` |
| C2 | `DWM_TAG_UPDATE` carries an incrementing sequence counter written by dwm (`dwm.c:3413-3420`, `data[0] = ++tagupdatesequence`), but the bridge interprets it as a tag mask and writes it into the state cache. Every client tag change stomps `tag_mask` with a garbage value. | `src/daemon/x11_bridge.c:248-259` |
| C3 | Payload-length contract mismatch: `oomaya_frame_validate()` accepts up to `OOMAYA_MAX_PAYLOAD_LEN` (4096), while client body buffers, worker `task.payload`, ring slots, and the negotiate transformer are all sized by `OOMAYA_FRAME_BODY_MAX` (512). A 513..4096-byte frame passes validation, is truncated, and leaves bytes in the socket buffer that are reinterpreted as the next frame header (permanent stream desync). Silent truncation also occurs on the offload path. | `include/oomaya_ipc.h:99`, `src/ipc/protocol.c:62`, `src/daemon/reactor.c:264,312`, `src/daemon/ring.c:69,73`, `src/ipc/negotiate.c:168` |
| C4 | Worker completions record only a raw fd. The reactor verifies liveness by fd-number equality; if a client disconnects and a new accept reuses the fd number before the completion drains, the response frame (with the wrong `msg_id`) is delivered to an unrelated client. | `src/daemon/reactor.c:345-371`, `include/oomaya_worker.h` |

### Medium

| ID | Summary | Location |
|----|---------|----------|
| M5 | `_DWM_MONITOR_DESKTOPS` handler updates `monitor_tags[]` but never `monitor_count`; `IPC_CMD_WM_GET_STATE` always reports 1. | `src/daemon/x11_bridge.c:261-286` |
| M6 | X11 I/O error sets `g_bridge.active = 0` but leaves the fd in the epoll set. The dead fd stays readable (EOF), `oomaya_x11_process_events()` returns immediately, EPOLLIN remains level-triggered: busy spin at 100% CPU until shutdown. | `src/daemon/x11_bridge.c:70-77`, `src/daemon/reactor.c:497` |
| M7 | Layout symbol property is read with a 16-item limit and passed to `strcmp()` without a NUL guarantee: heap over-read when the symbol is exactly 16 bytes. | `src/daemon/x11_bridge.c:160-166,234-245` |
| M8 | `create_listener()` unconditionally `unlink()`s the socket path. A second daemon instance transparently replaces the first; no lock file or liveness check. | `src/daemon/main.c:120` |
| M9 | Rate-limit rejection responds `ERR_MALFORMED` instead of a dedicated error code, misleading client-side diagnostics. | `src/daemon/reactor.c:235-239` |
| M10 | `recv()` `EINTR` is treated as a hard error and closes the client connection. | `src/daemon/reactor.c:293,317` |
| M11 | `send_response()` return value is ignored everywhere; partial `writev()` writes are neither retried nor detected, and an undeliverable offload response silently corrupts the client stream. | `src/daemon/reactor.c:82-92` |
| M12 | Worker shutdown does not drain `resp_ring`; in-flight completions are dropped without responses. | `src/daemon/worker_pool.c:139-165` |

### Low / design debt

- `oomaya_neg_classify()` receives the opcode smuggled through
  `hdr->msg_id` (`reactor.c:250-252`); documented in `src/ipc/negotiate.c:85-111`
  as Phase 2 debt ("promote opcode into the header in V2").
- The bridge subscribes to `DWM_TAG_UPDATE`, a property it also writes itself
  (`oomaya_x11_sync_state()`), so every outbound state mirror triggers a
  redundant self-echo PropertyNotify round-trip.
- Wire byte order (little-endian assumed by `memcpy`) is implemented but not
  documented.
- `CS_READY` is declared but never used.
- `oomaya_reactor_destroy()` depends on the caller for `listen_fd`, eventfd,
  and X11 fd ownership; the ownership boundary is undocumented.

## Improvement plan

Each item ships as an independent PR with its own test, per the AGENTS.md
review-boundary rules.

### Phase A - correctness fixes (highest priority)

| Item | Fixes | Approach | Validation |
|------|-------|----------|------------|
| A1 | C1 | Reorder `layout_symbols[]` to match `layouts[]`; longer-term, extract the layout order into a shared header consumed by both dwm and the daemon so the drift class itself disappears. | New unit test comparing symbol-to-index mapping across the full table. |
| A2 | C2 | Treat `DWM_TAG_UPDATE` as a change signal only: on notification, re-read `_NET_CURRENT_DESKTOP` instead of interpreting the payload as a mask (or drop the subscription if it becomes redundant). Decide the intended bidirectional semantics with dwm first. | Unit test with a counter-valued property asserting `tag_mask` is preserved. |
| A3 | C3 | Unify the payload contract on a single constant: make `OOMAYA_MAX_PAYLOAD_LEN` equal `OOMAYA_FRAME_BODY_MAX` (512), referenced by `frame_validate`, reactor receive logic, task payload, ring slots, and the negotiate transformer. | Boundary unit tests for validate (511/512/513) plus a pipeline desync integration test. |
| A4 | C4 | Add `slot_idx` + per-slot `generation` to the task/completion and validate completions at slot granularity instead of fd equality; increment the generation in `client_slot_free()`. | Daemon integration stress test: repeated connect/disconnect cycles with pending offload tasks. |

Phase A sub-items (small, may piggyback on A4):

- M5: set `monitor_count = nmons` in the `_DWM_MONITOR_DESKTOPS` handler
  (five-minute fix with user-visible impact).
- M7: copy the layout symbol into a bounded, NUL-terminated local buffer
  before `strcmp()`.

### Phase B - robustness

1. M6: add `oomaya_reactor_remove_x11()` and invoke it from the I/O error
   path (or have the reactor drop the X11 fd itself on failure).
2. M8: guard listener creation with `flock(LOCK_EX | LOCK_NB)` on a lock
   file (or abstract-socket liveness detection); second instance logs and
   exits cleanly instead of silently replacing the first.
3. M9: define and use a dedicated rate-limit error code.
4. M10: retry on `EINTR` instead of closing the connection.
5. M11: detect partial `writev()`; define a policy (single retry, then close
   the offending connection) so the stream can never slide.
6. M12: drain `resp_ring` during worker pool shutdown, emitting whatever
   responses are still deliverable.

### Phase C - structural debt (IPC v2)

1. Promote the opcode into the wire header per the existing TODO in
   `src/ipc/negotiate.c`, removing the `msg_id` repurposing hack.
2. Introduce a shared contract header (`include/` or generated from
   `config.def.h`) covering atom names, layout ordering, payload size
   constants, and the `DWM_TAG_UPDATE` semantics, consumed by both dwm and
   the daemon. This eliminates the drift class behind C1 and C2.
3. Document the wire format (byte order, frame layout, socket lifecycle,
   single-writer rule for root atoms) in `SPEC.md`.
4. Test hardening: slow-reader backpressure scenario, fd-reuse race test,
   and a reactor integration test with a scripted client.

## Validation strategy

Per AGENTS.md required validation, each phase runs the managed workspace
gate:

```sh
scripts/run-tests make clean all
scripts/run-tests
```

Runtime behavior that cannot be proven statically (A2, M5, M6, X11 bridge
changes) must be verified in a real or nested X11 session before being
declared fixed; anything not tested in a given environment is stated
explicitly in the corresponding PR.

## Open questions

1. A2 semantics: was `DWM_TAG_UPDATE` intended as a dwm -> daemon change
   signal, or is `sync_state()` writing it for external observers only?
   dwm.c never reads it, so defense currently has two writers with two
   different meanings to one atom. Proposal: dwm keeps it as a signal;
   the daemon publishes its own mirror on a distinct atom (or reuses
   `_NET_CURRENT_DESKTOP` consumers directly).
2. C3 target: is there a reason to keep the 4096 limit (future large
   frames arriving in v2), or should 512 become authoritative everywhere?
3. A4: adding `generation` grows the completion record; confirm the ring
   slot size (currently 512B payload + fixed fields) accommodates two more
   32-bit fields without enlarging slots.
