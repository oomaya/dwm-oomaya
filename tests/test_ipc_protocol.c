/* test_ipc_protocol.c — Phase 1 unit tests for oomaya IPC protocol.
 *
 * Covers:
 *   T01  Header size is exactly 16 bytes (compile-time + runtime guard)
 *   T02  Magic validation: accept OOMB, reject any other value
 *   T03  Version validation: accept known versions, reject unknown
 *   T04  Payload length cap enforcement
 *   T05  oomaya_frame_init_request() field population
 *   T06  oomaya_frame_init_response() echoes msg_id and target_ver
 *   T07  oomaya_frame_init_error() encodes error code in payload_len
 *   T08  Fast-path opcode classification
 *   T09  Worker-pool opcode classification
 *   T10  Unknown opcode returns -1 from oomaya_neg_classify()
 *   T11  v1_to_v2_req: identity round-trip with exact byte comparison
 *   T12  v2_to_v1_resp: identity round-trip with exact byte comparison
 *   T13  NULL pointer safety for all public API functions
 *   T14  oomaya_frame_validate() NULL pointer returns MALFORMED
 *
 * Zero dynamic allocations. All buffers on the stack.
 * No external test framework — plain C99, exits with 0 on pass.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/oomaya_ipc.h"

/* ── Minimal test harness ───────────────────────────────────────────────── */

static int g_pass = 0;
static int g_fail = 0;

#define ASSERT_EQ(label, got, expected)                                        \
    do {                                                                       \
        if ((got) == (expected)) {                                             \
            printf("  [PASS] %s\n", (label));                                 \
            ++g_pass;                                                          \
        } else {                                                               \
            printf("  [FAIL] %s  got=%lld  expected=%lld\n",                  \
                   (label),                                                    \
                   (long long)(got),                                           \
                   (long long)(expected));                                     \
            ++g_fail;                                                          \
        }                                                                      \
    } while (0)

#define ASSERT_NEQ(label, got, expected)                                       \
    do {                                                                       \
        if ((got) != (expected)) {                                             \
            printf("  [PASS] %s\n", (label));                                 \
            ++g_pass;                                                          \
        } else {                                                               \
            printf("  [FAIL] %s  (values unexpectedly equal: %lld)\n",        \
                   (label), (long long)(got));                                 \
            ++g_fail;                                                          \
        }                                                                      \
    } while (0)

#define TEST(name) \
    printf("\n[TEST] " name "\n")

/* ── Helpers ─────────────────────────────────────────────────────────────── */

static oomaya_ipc_header_t
make_valid_header(void)
{
    oomaya_ipc_header_t h;
    memset(&h, 0, sizeof(h));
    h.magic       = OOMAYA_IPC_MAGIC;
    h.client_ver  = OOMAYA_IPC_VERSION_1_0;
    h.target_ver  = OOMAYA_IPC_VERSION_1_0;
    h.msg_type    = (uint16_t)OOMAYA_MSG_REQUEST;
    h.msg_id      = 0x0001;
    h.payload_len = 0;
    return h;
}

/* ── Tests ──────────────────────────────────────────────────────────────── */

static void
t01_header_size(void)
{
    TEST("T01: Header is exactly 16 bytes");
    ASSERT_EQ("sizeof(oomaya_ipc_header_t) == 16",
              (int)sizeof(oomaya_ipc_header_t), 16);
}

static void
t02_magic_validation(void)
{
    oomaya_ipc_header_t h;
    TEST("T02: Magic validation");

    h = make_valid_header();
    ASSERT_EQ("Valid OOMB magic",
              oomaya_frame_validate(&h), OOMAYA_IPC_OK);

    h.magic = 0xDEADBEEFu;
    ASSERT_EQ("Bad magic → MALFORMED",
              oomaya_frame_validate(&h), OOMAYA_IPC_ERR_MALFORMED);

    h.magic = 0x00000000u;
    ASSERT_EQ("Zero magic → MALFORMED",
              oomaya_frame_validate(&h), OOMAYA_IPC_ERR_MALFORMED);
}

static void
t03_version_validation(void)
{
    oomaya_ipc_header_t h;
    TEST("T03: Version validation");

    h = make_valid_header();

    h.client_ver = OOMAYA_IPC_VERSION_1_0;
    h.target_ver = OOMAYA_IPC_VERSION_1_0;
    ASSERT_EQ("V1/V1 accepted", oomaya_frame_validate(&h), OOMAYA_IPC_OK);

    h.client_ver = OOMAYA_IPC_VERSION_2_0;
    h.target_ver = OOMAYA_IPC_VERSION_1_0;
    ASSERT_EQ("V2/V1 accepted", oomaya_frame_validate(&h), OOMAYA_IPC_OK);

    h.client_ver = OOMAYA_IPC_VERSION_1_0;
    h.target_ver = OOMAYA_IPC_VERSION_2_0;
    ASSERT_EQ("V1/V2 accepted", oomaya_frame_validate(&h), OOMAYA_IPC_OK);

    h.client_ver = 0x0300u;  /* unknown future version */
    h.target_ver = OOMAYA_IPC_VERSION_1_0;
    ASSERT_EQ("Unknown client_ver → VERSION",
              oomaya_frame_validate(&h), OOMAYA_IPC_ERR_VERSION);

    h.client_ver = OOMAYA_IPC_VERSION_1_0;
    h.target_ver = 0xFFFFu;
    ASSERT_EQ("Unknown target_ver → VERSION",
              oomaya_frame_validate(&h), OOMAYA_IPC_ERR_VERSION);
}

static void
t04_payload_len_cap(void)
{
    oomaya_ipc_header_t h;
    TEST("T04: Payload length cap");

    h = make_valid_header();

    h.payload_len = OOMAYA_MAX_PAYLOAD_LEN;
    ASSERT_EQ("payload_len == MAX accepted",
              oomaya_frame_validate(&h), OOMAYA_IPC_OK);

    h.payload_len = OOMAYA_MAX_PAYLOAD_LEN + 1;
    ASSERT_EQ("payload_len > MAX → PAYLOAD",
              oomaya_frame_validate(&h), OOMAYA_IPC_ERR_PAYLOAD);

    h.payload_len = 0xFFFFFFFFu;
    ASSERT_EQ("payload_len = UINT32_MAX → PAYLOAD",
              oomaya_frame_validate(&h), OOMAYA_IPC_ERR_PAYLOAD);
}

static void
t05_init_request(void)
{
    oomaya_ipc_header_t h;
    TEST("T05: oomaya_frame_init_request()");

    memset(&h, 0xAA, sizeof(h));  /* poison memory */
    oomaya_frame_init_request(&h,
                              (uint16_t)IPC_CMD_WM_VIEW_TAG,
                              0x0042u,
                              64u);

    ASSERT_EQ("magic set",    (uint32_t)h.magic, (uint32_t)OOMAYA_IPC_MAGIC);
    ASSERT_EQ("client_ver",   h.client_ver, OOMAYA_IPC_CURRENT_VER);
    ASSERT_EQ("target_ver",   h.target_ver, OOMAYA_IPC_CURRENT_VER);
    ASSERT_EQ("msg_type",     h.msg_type,   (uint16_t)OOMAYA_MSG_REQUEST);
    ASSERT_EQ("msg_id",       h.msg_id,     0x0042u);
    ASSERT_EQ("payload_len",  h.payload_len, 64u);
}

static void
t06_init_response(void)
{
    oomaya_ipc_header_t req, resp;
    TEST("T06: oomaya_frame_init_response()");

    req = make_valid_header();
    req.msg_id     = 0x00ABu;
    req.target_ver = OOMAYA_IPC_VERSION_1_0;

    memset(&resp, 0xBB, sizeof(resp));
    oomaya_frame_init_response(&resp, &req, 128u);

    ASSERT_EQ("magic",        (uint32_t)resp.magic, (uint32_t)OOMAYA_IPC_MAGIC);
    ASSERT_EQ("msg_type",     resp.msg_type, (uint16_t)OOMAYA_MSG_RESPONSE);
    ASSERT_EQ("msg_id echo",  resp.msg_id,   req.msg_id);
    ASSERT_EQ("target_ver echo", resp.target_ver, req.target_ver);
    ASSERT_EQ("payload_len",  resp.payload_len, 128u);
}

static void
t07_init_error(void)
{
    oomaya_ipc_header_t req, err_hdr;
    TEST("T07: oomaya_frame_init_error()");

    req = make_valid_header();
    req.msg_id = 0x00CDu;

    memset(&err_hdr, 0xCC, sizeof(err_hdr));
    oomaya_frame_init_error(&err_hdr, &req, OOMAYA_IPC_ERR_MALFORMED);

    ASSERT_EQ("magic",      (uint32_t)err_hdr.magic, (uint32_t)OOMAYA_IPC_MAGIC);
    ASSERT_EQ("msg_type",   err_hdr.msg_type, (uint16_t)OOMAYA_MSG_ERROR);
    ASSERT_EQ("msg_id",     err_hdr.msg_id,   req.msg_id);
    ASSERT_EQ("err in payload_len",
              (int32_t)err_hdr.payload_len, (int32_t)OOMAYA_IPC_ERR_MALFORMED);

    /* NULL req: pre-parse error, msg_id must be 0 */
    oomaya_frame_init_error(&err_hdr, NULL, OOMAYA_IPC_ERR_OPCODE);
    ASSERT_EQ("NULL req → msg_id=0", err_hdr.msg_id, 0u);
}

static void
t08_fast_path_opcodes(void)
{
    TEST("T08: Fast-path opcode classification");
    ASSERT_EQ("WM_GET_STATE   fast", oomaya_neg_opcode_is_fast_path(IPC_CMD_WM_GET_STATE),    1);
    ASSERT_EQ("WM_VIEW_TAG    fast", oomaya_neg_opcode_is_fast_path(IPC_CMD_WM_VIEW_TAG),     1);
    ASSERT_EQ("WM_TOGGLE_TAG  fast", oomaya_neg_opcode_is_fast_path(IPC_CMD_WM_TOGGLE_TAG),   1);
    ASSERT_EQ("WM_SET_LAYOUT  fast", oomaya_neg_opcode_is_fast_path(IPC_CMD_WM_SET_LAYOUT),   1);
    ASSERT_EQ("WM_FOCUS_CLIENT fast",oomaya_neg_opcode_is_fast_path(IPC_CMD_WM_FOCUS_CLIENT), 1);
    ASSERT_EQ("WM_KILL_CLIENT fast", oomaya_neg_opcode_is_fast_path(IPC_CMD_WM_KILL_CLIENT),  1);
    /* Cycle monitor is NOT fast-path despite being WM namespace */
    ASSERT_EQ("WM_CYCLE_MONITOR NOT fast",
              oomaya_neg_opcode_is_fast_path(IPC_CMD_WM_CYCLE_MONITOR), 0);
}

static void
t09_worker_pool_opcodes(void)
{
    oomaya_ipc_header_t h;
    TEST("T09: Worker-pool opcode classification via oomaya_neg_classify()");

    h = make_valid_header();

    h.msg_id = IPC_CMD_WM_CYCLE_MONITOR;
    ASSERT_EQ("CYCLE_MONITOR → offload",   oomaya_neg_classify(&h), 1);

    h.msg_id = IPC_CMD_SYS_GET_POWER;
    ASSERT_EQ("SYS_GET_POWER → offload",   oomaya_neg_classify(&h), 1);

    h.msg_id = IPC_CMD_SYS_SET_POWER;
    ASSERT_EQ("SYS_SET_POWER → offload",   oomaya_neg_classify(&h), 1);

    h.msg_id = IPC_CMD_SYS_GET_AUDIO;
    ASSERT_EQ("SYS_GET_AUDIO → offload",   oomaya_neg_classify(&h), 1);

    h.msg_id = IPC_CMD_SYS_SET_VOLUME;
    ASSERT_EQ("SYS_SET_VOLUME → offload",  oomaya_neg_classify(&h), 1);

    /* Fast-path ops must return 0 */
    h.msg_id = IPC_CMD_WM_VIEW_TAG;
    ASSERT_EQ("WM_VIEW_TAG → fast (0)",    oomaya_neg_classify(&h), 0);
}

static void
t10_unknown_opcode(void)
{
    oomaya_ipc_header_t h;
    TEST("T10: Unknown opcode → classify returns -1");

    h = make_valid_header();

    h.msg_id = 0x0999u;
    ASSERT_EQ("unknown 0x0999 → -1", oomaya_neg_classify(&h), -1);

    h.msg_id = 0xFFFFu;
    ASSERT_EQ("unknown 0xFFFF → -1", oomaya_neg_classify(&h), -1);

    h.msg_id = 0x0000u;
    ASSERT_EQ("unknown 0x0000 → -1", oomaya_neg_classify(&h), -1);
}

static void
t11_v1_to_v2_req(void)
{
    uint8_t  src[8]  = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint8_t  dst[OOMAYA_FRAME_BODY_MAX];
    uint32_t dst_len = 0;
    int      rc;

    TEST("T11: v1_to_v2_req identity transform");

    memset(dst, 0xFF, sizeof(dst));
    rc = oomaya_neg_v1_to_v2_req(src, 8u, dst, &dst_len);

    ASSERT_EQ("return OK",     rc, OOMAYA_IPC_OK);
    ASSERT_EQ("dst_len == 8",  (int)dst_len, 8);
    ASSERT_EQ("byte[0] match", dst[0], 0x01u);
    ASSERT_EQ("byte[7] match", dst[7], 0x08u);

    /* Empty payload */
    rc = oomaya_neg_v1_to_v2_req(src, 0u, dst, &dst_len);
    ASSERT_EQ("empty payload OK",  rc, OOMAYA_IPC_OK);
    ASSERT_EQ("empty dst_len==0",  (int)dst_len, 0);

    /* Oversized src */
    rc = oomaya_neg_v1_to_v2_req(src, OOMAYA_FRAME_BODY_MAX + 1u, dst, &dst_len);
    ASSERT_EQ("oversized → PAYLOAD", rc, OOMAYA_IPC_ERR_PAYLOAD);
}

static void
t12_v2_to_v1_resp(void)
{
    uint8_t  src[16];
    uint8_t  dst[OOMAYA_FRAME_BODY_MAX];
    uint32_t dst_len = 0;
    int      rc;
    size_t   i;

    TEST("T12: v2_to_v1_resp identity transform");

    for (i = 0; i < sizeof(src); ++i)
        src[i] = (uint8_t)(i + 0xA0u);

    memset(dst, 0x00, sizeof(dst));
    rc = oomaya_neg_v2_to_v1_resp(src, 16u, dst, &dst_len);

    ASSERT_EQ("return OK",      rc, OOMAYA_IPC_OK);
    ASSERT_EQ("dst_len == 16",  (int)dst_len, 16);
    ASSERT_EQ("byte[0] match",  dst[0],  0xA0u);
    ASSERT_EQ("byte[15] match", dst[15], 0xAFu);

    /* Oversized src is clamped, not an error */
    rc = oomaya_neg_v2_to_v1_resp(src, OOMAYA_FRAME_BODY_MAX + 1u, dst, &dst_len);
    ASSERT_EQ("oversized clamped OK",  rc, OOMAYA_IPC_OK);
    ASSERT_EQ("dst_len clamped",       (int)dst_len, (int)OOMAYA_FRAME_BODY_MAX);
}

static void
t13_null_safety(void)
{
    oomaya_ipc_header_t h = make_valid_header();
    uint8_t  buf[32];
    uint32_t len = 0;

    TEST("T13: NULL pointer safety");

    /* These must not crash or segfault */
    oomaya_frame_init_request(NULL, 0, 0, 0);
    ASSERT_EQ("init_request NULL no crash", 1, 1);

    oomaya_frame_init_response(NULL, &h, 0);
    ASSERT_EQ("init_response NULL hdr no crash", 1, 1);

    oomaya_frame_init_response(&h, NULL, 0);
    ASSERT_EQ("init_response NULL req no crash", 1, 1);

    oomaya_frame_init_error(NULL, &h, OOMAYA_IPC_ERR_OPCODE);
    ASSERT_EQ("init_error NULL hdr no crash", 1, 1);

    ASSERT_EQ("neg_classify NULL → -1",
              oomaya_neg_classify(NULL), -1);

    ASSERT_EQ("v1_to_v2 NULL src → MALFORMED",
              oomaya_neg_v1_to_v2_req(NULL, 0, buf, &len),
              OOMAYA_IPC_ERR_MALFORMED);

    ASSERT_EQ("v1_to_v2 NULL dst → MALFORMED",
              oomaya_neg_v1_to_v2_req(buf, 0, NULL, &len),
              OOMAYA_IPC_ERR_MALFORMED);

    ASSERT_EQ("v2_to_v1 NULL src → MALFORMED",
              oomaya_neg_v2_to_v1_resp(NULL, 0, buf, &len),
              OOMAYA_IPC_ERR_MALFORMED);

    ASSERT_EQ("v2_to_v1 NULL dst → MALFORMED",
              oomaya_neg_v2_to_v1_resp(buf, 0, NULL, &len),
              OOMAYA_IPC_ERR_MALFORMED);
}

static void
t14_validate_null(void)
{
    TEST("T14: oomaya_frame_validate(NULL) → MALFORMED");
    ASSERT_EQ("validate NULL → MALFORMED",
              oomaya_frame_validate(NULL), OOMAYA_IPC_ERR_MALFORMED);
}

/* ── Entry point ─────────────────────────────────────────────────────────── */

int
main(void)
{
    printf("=== oomaya IPC protocol — Phase 1 unit tests ===\n");

    t01_header_size();
    t02_magic_validation();
    t03_version_validation();
    t04_payload_len_cap();
    t05_init_request();
    t06_init_response();
    t07_init_error();
    t08_fast_path_opcodes();
    t09_worker_pool_opcodes();
    t10_unknown_opcode();
    t11_v1_to_v2_req();
    t12_v2_to_v1_resp();
    t13_null_safety();
    t14_validate_null();

    printf("\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);

    return (g_fail == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
