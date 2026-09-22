/* negotiate.c — Active version negotiation and payload transformation.
 *
 * Spec:  dwm_oomaya_ipc_bridge_design.md §4.2
 * C99.   Zero dynamic allocations. All transforms operate on caller-supplied
 *        stack or pool buffers.
 *
 * Execution-path classification matrix (from spec §2):
 *
 *   Opcode range  Namespace  Execution target
 *   ────────────  ─────────  ──────────────────────────────────────────────
 *   0x0101–0x0106  WM        Main thread fast-path  (<35 µs)
 *   0x0107         WM/RandR  Worker pool offload     (1–5 ms)
 *   0x0201–0x0204  SYS       Worker pool offload     (1–15 ms)
 *   0x0301–0x0303  SIG       N/A (server-emitted signals)
 *
 * V1 <-> V2 transformer notes (Phase 1 — single-version system):
 *
 *   With only V1 deployed today, the transformers are identity-preserving
 *   memcpy stubs with full bound checks.  They are structured as real
 *   transformers so that Phase 2 V2 additions are drop-in replacements —
 *   no call-site changes needed in the reactor.
 *
 *   Future V2 additions MUST:
 *     • Extend oomaya_neg_v1_to_v2_req()  to zero-fill new V2 fields.
 *     • Extend oomaya_neg_v2_to_v1_resp() to truncate/synthesise V1 fields.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "../../include/oomaya_ipc.h"

/* ── Fast-path opcode table ──────────────────────────────────────────────
 *
 * Opcodes listed here execute synchronously on the main thread.
 * All others are routed to the worker pool.
 */
static const uint16_t s_fast_path_opcodes[] = {
    IPC_CMD_WM_GET_STATE,
    IPC_CMD_WM_VIEW_TAG,
    IPC_CMD_WM_TOGGLE_TAG,
    IPC_CMD_WM_SET_LAYOUT,
    IPC_CMD_WM_FOCUS_CLIENT,
    IPC_CMD_WM_KILL_CLIENT,
};

#define FAST_PATH_COUNT \
    (sizeof(s_fast_path_opcodes) / sizeof(s_fast_path_opcodes[0]))

/* ── Public API ─────────────────────────────────────────────────────────── */

/**
 * oomaya_neg_opcode_is_fast_path() — 1 if opcode executes on the main thread;
 * 0 if it must be dispatched to the worker pool.
 *
 * O(N) linear scan over a small constant table — N=6, always fits in a single
 * cache line, and avoids branchy switch statements that inhibit inlining.
 */
int
oomaya_neg_opcode_is_fast_path(uint16_t opcode)
{
    size_t i;
    for (i = 0; i < FAST_PATH_COUNT; ++i) {
        if (s_fast_path_opcodes[i] == opcode)
            return 1;
    }
    return 0;
}

/**
 * oomaya_neg_classify() — Map a validated header to an execution path.
 *
 * @return  0  = fast-path (main thread, synchronous)
 *          1  = worker-pool offload (async, eventfd completion)
 *         -1  = unknown / unrecognised opcode (caller must send error)
 *
 * The reactor calls this once per inbound frame, after
 * oomaya_frame_validate() returns OK.  The opcode is carried in the
 * payload body; this function inspects only the header's msg_type and
 * expects the caller to pass the opcode extracted from the first two
 * bytes of the payload.  As a convenience, for OOMAYA_MSG_SIGNAL frames
 * (server-emitted) the classify result is always 0 (no dispatch needed).
 *
 * NOTE: Until Phase 2 adds opcode-in-header support, the reactor must
 * extract the opcode from payload[0..1] before calling this function and
 * pass it via the cast below.  The header's msg_type field is used only
 * as a sanity gate.
 */
int
oomaya_neg_classify(const oomaya_ipc_header_t *hdr)
{
    uint16_t ns;

    if (hdr == NULL)
        return -1;

    /* Signals are not dispatched — they are emitted outbound only. */
    if (hdr->msg_type == (uint16_t)OOMAYA_MSG_SIGNAL)
        return 0;

    /*
     * Opcode namespace classification:
     *   0x01xx → WM fast-path or cycle-monitor offload
     *   0x02xx → SYS worker-pool offload
     *   Other  → unknown
     *
     * This function uses the namespace embedded in msg_id as a stand-in
     * until the opcode is promoted into the header in V2.  In the reactor
     * the caller passes the opcode as the msg_id during classification.
     */
    ns = (uint16_t)(hdr->msg_id & 0xFF00u);

    switch (ns) {
    case 0x0100:
        /* WM namespace: cycle-monitor (0x0107) is offloaded, rest fast */
        if (hdr->msg_id == (uint16_t)IPC_CMD_WM_CYCLE_MONITOR)
            return 1;
        return oomaya_neg_opcode_is_fast_path(hdr->msg_id) ? 0 : -1;

    case 0x0200:
        /* SYS namespace: always worker-pool */
        switch (hdr->msg_id) {
        case IPC_CMD_SYS_GET_POWER:
        case IPC_CMD_SYS_SET_POWER:
        case IPC_CMD_SYS_GET_AUDIO:
        case IPC_CMD_SYS_SET_VOLUME:
            return 1;
        default:
            return -1;
        }

    case 0x0300:
        /* Signal namespace — not dispatched */
        return 0;

    default:
        return -1;
    }
}

/* ── V1 ↔ V2 payload transformers ──────────────────────────────────────────
 *
 * Phase 1: identity transformers with full bound checking.
 *
 * V1 and V2 payloads share the same layout for all currently defined opcodes.
 * The transformer copies bytes verbatim and sets *dst_len = src_len.
 *
 * When V2 introduces new trailing fields (Phase 2+):
 *   • v1_to_v2_req must zero-fill the extra bytes after the V1 body.
 *   • v2_to_v1_resp must drop / synthesise any V2-only fields.
 *
 * All error handling is explicit — no assert(), no abort().
 */

/**
 * oomaya_neg_v1_to_v2_req() — Promote a V1 request payload to V2 layout.
 */
int
oomaya_neg_v1_to_v2_req(const uint8_t *src_body,
                         uint32_t       src_len,
                         uint8_t       *dst_body,
                         uint32_t      *dst_len)
{
    if (src_body == NULL || dst_body == NULL || dst_len == NULL)
        return OOMAYA_IPC_ERR_MALFORMED;

    if (src_len > OOMAYA_FRAME_BODY_MAX)
        return OOMAYA_IPC_ERR_PAYLOAD;

    /*
     * Phase 1: identity copy.
     * Phase 2 extension point: memset(dst_body + src_len, 0, V2_EXTRA_BYTES)
     * and set *dst_len = src_len + V2_EXTRA_BYTES.
     */
    if (src_len > 0)
        memcpy(dst_body, src_body, (size_t)src_len);

    *dst_len = src_len;
    return OOMAYA_IPC_OK;
}

/**
 * oomaya_neg_v2_to_v1_resp() — Demote a V2 response payload to V1 layout.
 */
int
oomaya_neg_v2_to_v1_resp(const uint8_t *src_body,
                          uint32_t       src_len,
                          uint8_t       *dst_body,
                          uint32_t      *dst_len)
{
    uint32_t copy_len;

    if (src_body == NULL || dst_body == NULL || dst_len == NULL)
        return OOMAYA_IPC_ERR_MALFORMED;

    /*
     * Phase 1: identity copy, clamped to FRAME_BODY_MAX.
     * Phase 2: copy_len = V1_PAYLOAD_STRUCT_SIZE for each opcode;
     * truncate any V2-only trailing fields.
     */
    copy_len = (src_len > OOMAYA_FRAME_BODY_MAX)
               ? OOMAYA_FRAME_BODY_MAX
               : src_len;

    if (copy_len > 0)
        memcpy(dst_body, src_body, (size_t)copy_len);

    *dst_len = copy_len;
    return OOMAYA_IPC_OK;
}
