/* protocol.c — IPC frame framing, validation, and header construction.
 *
 * Spec:  dwm_oomaya_ipc_bridge_design.md §4
 * C99.   Zero dynamic allocations on the fast path.
 *        All operations are safe to call from the epoll reactor hot loop.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "../../include/oomaya_ipc.h"

/* ── Internal helpers ───────────────────────────────────────────────────── */

/**
 * version_is_known() — Return 1 if the version word is a recognised protocol
 * version; 0 otherwise.  Extend this table as new versions are ratified.
 */
static int
version_is_known(uint16_t ver)
{
    switch (ver) {
    case OOMAYA_IPC_VERSION_1_0:
    case OOMAYA_IPC_VERSION_2_0:
        return 1;
    default:
        return 0;
    }
}

/* ── Public API ─────────────────────────────────────────────────────────── */

/**
 * oomaya_frame_validate() — Validate a fully-received 16-byte wire header.
 *
 * Checks (in order):
 *   1. Magic word == "OOMB"
 *   2. client_ver and target_ver are both known versions
 *   3. payload_len does not exceed OOMAYA_MAX_PAYLOAD_LEN
 *
 * @hdr   Pointer to the header in host byte order.
 * @return OOMAYA_IPC_OK on success, or a negative oomaya_ipc_err_t.
 */
int
oomaya_frame_validate(const oomaya_ipc_header_t *hdr)
{
    if (hdr == NULL)
        return OOMAYA_IPC_ERR_MALFORMED;

    /* 1. Magic check */
    if (hdr->magic != OOMAYA_IPC_MAGIC)
        return OOMAYA_IPC_ERR_MALFORMED;

    /* 2. Version sanity: both fields must be known versions */
    if (!version_is_known(hdr->client_ver) ||
        !version_is_known(hdr->target_ver))
        return OOMAYA_IPC_ERR_VERSION;

    /* 3. Payload length cap prevents heap exhaustion / buffer overruns (errors carry err in payload_len) */
    if (hdr->msg_type != (uint16_t)OOMAYA_MSG_ERROR &&
        hdr->payload_len > OOMAYA_MAX_PAYLOAD_LEN)
        return OOMAYA_IPC_ERR_PAYLOAD;

    return OOMAYA_IPC_OK;
}

/**
 * oomaya_frame_init_request() — Populate a header for an outbound request.
 *
 * client_ver and target_ver are both set to the daemon's current version.
 * Callers that need to simulate an older client_ver (e.g. test harnesses)
 * may overwrite those fields after this call.
 */
void
oomaya_frame_init_request(oomaya_ipc_header_t *hdr,
                          uint16_t             opcode,
                          uint16_t             msg_id,
                          uint32_t             payload_len)
{
    if (hdr == NULL)
        return;

    hdr->magic       = OOMAYA_IPC_MAGIC;
    hdr->client_ver  = OOMAYA_IPC_CURRENT_VER;
    hdr->target_ver  = OOMAYA_IPC_CURRENT_VER;
    hdr->msg_type    = (uint16_t)OOMAYA_MSG_REQUEST;
    hdr->msg_id      = msg_id;
    hdr->payload_len = payload_len;
    (void)opcode; /* opcode carried in payload, not in base header */
}

/**
 * oomaya_frame_init_response() — Populate a header for an outbound response.
 *
 * Echoes the client's msg_id and honours target_ver so that the negotiator
 * can downgrade the response payload when target_ver < CURRENT.
 */
void
oomaya_frame_init_response(oomaya_ipc_header_t       *hdr,
                           const oomaya_ipc_header_t *req,
                           uint32_t                   payload_len)
{
    if (hdr == NULL)
        return;

    hdr->magic       = OOMAYA_IPC_MAGIC;
    hdr->client_ver  = OOMAYA_IPC_CURRENT_VER;

    /* Honour the version the client asked for — negotiator will downgrade */
    hdr->target_ver  = (req != NULL) ? req->target_ver : OOMAYA_IPC_CURRENT_VER;

    hdr->msg_type    = (uint16_t)OOMAYA_MSG_RESPONSE;
    hdr->msg_id      = (req != NULL) ? req->msg_id : 0;
    hdr->payload_len = payload_len;
}

/**
 * oomaya_frame_init_error() — Populate an error-response header.
 *
 * The error code is placed in payload_len as a 4-byte signed value by
 * convention; the caller writes the actual 4-byte error body separately.
 * If req is NULL (e.g. a pre-parse error), msg_id is set to 0 and
 * target_ver defaults to CURRENT.
 */
void
oomaya_frame_init_error(oomaya_ipc_header_t       *hdr,
                        const oomaya_ipc_header_t *req,
                        oomaya_ipc_err_t           err)
{
    if (hdr == NULL)
        return;

    hdr->magic       = OOMAYA_IPC_MAGIC;
    hdr->client_ver  = OOMAYA_IPC_CURRENT_VER;
    hdr->target_ver  = (req != NULL) ? req->target_ver : OOMAYA_IPC_CURRENT_VER;
    hdr->msg_type    = (uint16_t)OOMAYA_MSG_ERROR;
    hdr->msg_id      = (req != NULL) ? req->msg_id : 0;

    /*
     * payload_len encodes the error code (4-byte signed integer).
     * The reactor writes a matching 4-byte body after the header.
     * Cast via uint32_t to avoid implementation-defined sign extension.
     */
    hdr->payload_len = (uint32_t)(int32_t)err;
}
