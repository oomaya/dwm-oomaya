/* oomaya_ipc.h — Public wire-protocol header for dwm-oomaya IPC bridge.
 *
 * Spec:  dwm_oomaya_ipc_bridge_design.md §4.1
 * Lang:  C99. No dynamic allocations. No X11/POSIX deps in this header.
 *
 * Wire layout (16 bytes, packed):
 *
 *   Offset  Size  Field
 *   ──────  ────  ──────────────────────────────────────────────────
 *     0       4   magic       (0x4F4F4D42 — ASCII "OOMB")
 *     4       2   client_ver  (sender payload version)
 *     6       2   target_ver  (response version requested by client)
 *     8       2   msg_type    (oomaya_msg_type_t)
 *    10       2   msg_id      (opaque transaction ID)
 *    12       4   payload_len (byte length of following payload body)
 *   ──────  ────
 *   Total  16 bytes
 */

#ifndef OOMAYA_IPC_H
#define OOMAYA_IPC_H

#include <stdint.h>
#include <stddef.h>

/* ── Magic ──────────────────────────────────────────────────────────────── */
#define OOMAYA_IPC_MAGIC          UINT32_C(0x4F4F4D42)  /* "OOMB" */

/* ── Protocol versions ──────────────────────────────────────────────────── */
#define OOMAYA_IPC_VERSION_1_0    UINT16_C(0x0100)
#define OOMAYA_IPC_VERSION_2_0    UINT16_C(0x0200)
#define OOMAYA_IPC_CURRENT_VER    OOMAYA_IPC_VERSION_1_0

/* ── Message types ──────────────────────────────────────────────────────── */
typedef enum {
    OOMAYA_MSG_REQUEST  = 0x0001,
    OOMAYA_MSG_RESPONSE = 0x0002,
    OOMAYA_MSG_SIGNAL   = 0x0003,
    OOMAYA_MSG_ERROR    = 0x00FF
} oomaya_msg_type_t;

/* ── 16-byte packed wire header ─────────────────────────────────────────── */
struct __attribute__((packed)) oomaya_ipc_header {
    uint32_t magic;         /* 4 B: 0x4F4F4D42 ("OOMB") */
    uint16_t client_ver;    /* 2 B: payload version sent by the client */
    uint16_t target_ver;    /* 2 B: response version requested by client */
    uint16_t msg_type;      /* 2 B: oomaya_msg_type_t */
    uint16_t msg_id;        /* 2 B: opaque transaction ID */
    uint32_t payload_len;   /* 4 B: byte length of payload body */
};                          /* Total: 16 bytes */

typedef struct oomaya_ipc_header oomaya_ipc_header_t;

/* Compile-time size guard — build error if struct drifts from 16 B. */
typedef char _oomaya_header_size_check[
    (sizeof(oomaya_ipc_header_t) == 16) ? 1 : -1
];

/* ── OpCodes ─────────────────────────────────────────────────────────────
 *
 * High byte = namespace:
 *   0x01  = WM  (window manager — synchronous fast-path, main thread)
 *   0x02  = SYS (system/hardware — offload to worker pool)
 *   0x03  = SIG (server-initiated signals / events)
 */
typedef enum {
    /* WM namespace — fast-path (<35 µs, main thread) */
    IPC_CMD_WM_GET_STATE      = 0x0101,
    IPC_CMD_WM_VIEW_TAG       = 0x0102,
    IPC_CMD_WM_TOGGLE_TAG     = 0x0103,
    IPC_CMD_WM_SET_LAYOUT     = 0x0104,
    IPC_CMD_WM_FOCUS_CLIENT   = 0x0105,
    IPC_CMD_WM_KILL_CLIENT    = 0x0106,
    IPC_CMD_WM_CYCLE_MONITOR  = 0x0107,  /* RandR enum — worker offload */

    /* SYS namespace — worker-pool offload (1–15 ms) */
    IPC_CMD_SYS_GET_POWER     = 0x0201,
    IPC_CMD_SYS_SET_POWER     = 0x0202,
    IPC_CMD_SYS_GET_AUDIO     = 0x0203,
    IPC_CMD_SYS_SET_VOLUME    = 0x0204,

    /* Signals emitted by daemon to subscribers */
    IPC_SIG_WM_LAYOUT_CHANGED = 0x0301,
    IPC_SIG_WM_TAG_CHANGED    = 0x0302,
    IPC_SIG_WM_FOCUS_CHANGED  = 0x0303
} oomaya_opcode_t;

/* ── Error codes ─────────────────────────────────────────────────────────── */
typedef enum {
    OOMAYA_IPC_OK             =  0,
    OOMAYA_IPC_ERR_MALFORMED  = -1,  /* bad magic or truncated header */
    OOMAYA_IPC_ERR_VERSION    = -2,  /* unsupported version pair */
    OOMAYA_IPC_ERR_OPCODE     = -3,  /* unknown opcode */
    OOMAYA_IPC_ERR_PAYLOAD    = -4,  /* payload_len exceeds maximum */
    OOMAYA_IPC_ERR_INTERNAL   = -5   /* daemon-side internal fault */
} oomaya_ipc_err_t;

/* ── Payload size limits ─────────────────────────────────────────────────── */
#define OOMAYA_MAX_PAYLOAD_LEN  UINT32_C(4096)

/* Stack-allocated in-band frame (header + bounded body). */
#define OOMAYA_FRAME_BODY_MAX   512u

typedef struct {
    oomaya_ipc_header_t hdr;
    uint8_t             body[OOMAYA_FRAME_BODY_MAX];
} oomaya_frame_t;

/* ── Protocol function prototypes (src/ipc/protocol.c) ──────────────────── */

int  oomaya_frame_validate(const oomaya_ipc_header_t *hdr);

void oomaya_frame_init_request(oomaya_ipc_header_t *hdr,
                               uint16_t             opcode,
                               uint16_t             msg_id,
                               uint32_t             payload_len);

void oomaya_frame_init_response(oomaya_ipc_header_t       *hdr,
                                const oomaya_ipc_header_t *req,
                                uint32_t                   payload_len);

void oomaya_frame_init_error(oomaya_ipc_header_t       *hdr,
                             const oomaya_ipc_header_t *req,
                             oomaya_ipc_err_t           err);

/* ── Negotiation prototypes (src/ipc/negotiate.c) ────────────────────────── */

int oomaya_neg_opcode_is_fast_path(uint16_t opcode);

int oomaya_neg_classify(const oomaya_ipc_header_t *hdr);

int oomaya_neg_v1_to_v2_req(const uint8_t *src_body,
                             uint32_t       src_len,
                             uint8_t       *dst_body,
                             uint32_t      *dst_len);

int oomaya_neg_v2_to_v1_resp(const uint8_t *src_body,
                              uint32_t       src_len,
                              uint8_t       *dst_body,
                              uint32_t      *dst_len);

#endif /* OOMAYA_IPC_H */
