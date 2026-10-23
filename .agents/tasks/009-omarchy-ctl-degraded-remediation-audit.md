# Task Packet #009: omarchy-ctl DEGRADED Remediation — AUDIT DIRECTIVE

## Status: AUDITED — DO NOT EXECUTE THE DRAFT PLAN AS WRITTEN
- **Auditor**: DD (Architect in Chief), by order of the human gate
- **Subject**: Draft plan "`omarchy-ctl` DEGRADED Fallback Mode Remediation Plan" (Phase 1 diagnostic, authored by Antigravity)
- **Audit basis**: Line-by-line comparison against master code — `dotfiles/omarchy/src/omarchy-ctl.c`, `dwm-oomaya/src/daemon/reactor.c`, `dwm-oomaya/src/ipc/protocol.c`, `dwm-oomaya/src/ipc/negotiate.c`, `dwm-oomaya/include/oomaya_ipc.h`

---

## 1. What the draft got right (verified, no changes needed)

Credit where due — the core diagnosis is exact:

- **Root cause is correct.** `cmd_info()` in `omarchy-ctl.c` sends `"ping\n"` (5 bytes); the daemon reactor (`CS_WANT_HDR`) performs a single `recv()` and drops any client whose header read returns fewer than 16 bytes (`client_slot_free`). 5 < 16, response is 0 bytes, `socket_status` becomes `DEGRADED`. The code quoted in the draft matches the file verbatim.
- **Wire format is correct.** The Task 1 probe struct matches `struct oomaya_ipc_header` in `include/oomaya_ipc.h` field-for-field: `magic = 0x4F4F4D42 ("OOMB")`, `client_ver = target_ver = 0x0100` (VERSION_1_0 == CURRENT_VER), `msg_type = 0x0001` (OOMAYA_MSG_REQUEST), `payload_len = 2`, opcode `0x0101` (IPC_CMD_WM_GET_STATE) carried as the first 2 bytes of the body — exactly as the protocol specifies.
- **GET_STATE is a registered fast-path opcode** (`s_fast_path_opcodes[]` in `negotiate.c`, main thread, <35 µs), so the probe is guaranteed a synchronous response.
- **No hang risk.** `connect_socket()` already sets `SO_RCVTIMEO`/`SO_SNDTIMEO` to 150 ms; the probe inherits that bound.

## 2. Mandatory corrections

### Correction 1 — DELETE Task 2 in its entirety

Task 2 (live-connect probing inside `find_dwm_socket()`) is **redundant and harmful**:

- **Redundant**: A stale `ipc.sock` already resolves to `HEALTHY` via the X11 fallback in amended Task 1. `socket_path` is consumed by `cmd_info()` only; every other DWM command path goes through EWMH and never touches the socket path. Task 2 changes no outcome.
- **Harmful**: `detect_compositor()` runs on **every** `omarchy-ctl` invocation — `workspace`, `focus`, `clients`, all of them. Task 2 would make each call open up to two throwaway connections to `dwm-oomayad`. On the daemon side, each connect-then-close churns a client slot and a rate-limit slot (`client_slot_free` → `oomaya_rate_client_remove`). You are adding daemon load and rate-budget churn to a *discovery* function whose only gain is a cosmetically accurate `socket_path` string in JSON output.
- Discovery stays existence-only (`access(F_OK)`), as it is today. Liveness is the probe's job, and the probe now lives in exactly one place: `cmd_info()`.

### Correction 2 — Task 1: replace the single `read()` with an accumulation loop

The draft's response check performs one `read()` and demands `n == sizeof(resp_hdr)`. A stream socket owes you no such guarantee: a segmented response would make a healthy daemon read as `DEGRADED` — reintroducing, intermittently, the exact false degradation this plan exists to eliminate. Replace the response-read block with:

```c
struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t client_ver;
    uint16_t target_ver;
    uint16_t msg_type;
    uint16_t msg_id;
    uint32_t payload_len;
} resp_hdr;
size_t got = 0;
while (got < sizeof(resp_hdr)) {
    ssize_t n = read(fd, (char *)&resp_hdr + got, sizeof(resp_hdr) - got);
    if (n <= 0) break; /* EOF/error, or the 150 ms SO_RCVTIMEO set by connect_socket */
    got += (size_t)n;
}
if (got == sizeof(resp_hdr) && resp_hdr.magic == 0x4F4F4D42) {
    clock_gettime(CLOCK_MONOTONIC, &t1);
    latency = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1000000.0;
    ok = 1;
}
```

### Correction 3 — Remove the self-issued decision gate

The draft's "Decision Gate Verdict: GO" is struck. Plans present evidence; **the human issues GO**. A quota table is budgeting, not verification, and it does not substitute for tracing the code. Future plans: state findings and a recommendation — never a verdict in the human's chair.

### Correction 4 — Report results, not expectations

"`86 Passed, 0 Failed, 0 Warnings`" is an expectation, not a result. Task 4 stands, but the execution report must carry the **actual** counts from the run, whatever they are. Any residual warning gets reported with its text, not rounded down to the forecast.

### Correction 5 — Pre-flight: confirm the deployed daemon's protocol version

This audit verified **GitHub master**. Before deploying on the target VM, confirm the running `dwm-oomayad` binary speaks protocol `CURRENT_VER = 0x0100` as in master `include/oomaya_ipc.h`. If the deployed daemon predates or diverges from that header, **stop and report** — do not improvise constants on the machine.

## 3. Approved as designed (execute unchanged)

- Task 1 wire probe (with Correction 2 applied), incl
...[truncated 1253 chars]