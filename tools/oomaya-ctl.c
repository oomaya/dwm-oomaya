/* oomaya-ctl.c — Production high-performance CLI client for dwm-oomayad.
 *
 * Spec:  claude_sonnet_sow_ipc_bridge.md Phase 4
 * C99.   Zero dynamic memory allocations on command dispatch.
 *        Sub-millisecond execution for hot-path desktop scripts & Quickshell.
 *
 * Usage:
 *   oomaya-ctl [--json|-j] [--socket <path>] <command> [args...]
 *
 * Commands:
 *   state, get-state            Print current WM state (tags, layout, focus)
 *   view <tag_mask>             Set active tag mask
 *   toggle-view <tag_mask>      Toggle tag mask bit
 *   set-layout <layout_idx>     Set tiling layout index
 *   focus <window_id>           Focus client window by XID
 *   kill                        Close focused client window
 *   cycle-monitor               Cycle active monitor
 *   power, get-power            Query system power status
 *   audio, get-audio            Query audio volume level
 *   volume <0-100>              Set audio volume level
 *   help, --help, -h            Show this help message
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>

#include "../include/oomaya_ipc.h"

/* Fast-path state response body layout (matches reactor.c) */
typedef struct __attribute__((packed)) {
    int32_t  status;
    uint32_t tag_mask;
    uint32_t layout_idx;
    uint32_t focused_win;
    uint32_t monitor_count;
} oomaya_state_resp_t;

static void print_usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [OPTIONS] <COMMAND> [ARGS...]\n\n"
        "High-performance IPC client for dwm-oomaya desktop environment.\n\n"
        "Options:\n"
        "  -j, --json              Output machine-parseable JSON format\n"
        "  -s, --socket <path>     Explicit UNIX socket path\n"
        "  -h, --help              Show this help message and exit\n\n"
        "Commands:\n"
        "  state, get-state            Query WM state (tags, layout, focus, monitors)\n"
        "  view <mask|1-9>             Set active tag mask (hex/int or tag number)\n"
        "  toggle-view <mask|1-9>      Toggle tag mask bit\n"
        "  set-layout <index>          Set geometry tile layout index\n"
        "  focus <window_xid>          Set input focus to client window XID\n"
        "  kill                        Close currently focused window\n"
        "  cycle-monitor               Switch focus to next physical monitor\n"
        "  power, get-power            Query power / battery profile\n"
        "  audio, get-audio            Query system audio volume\n"
        "  volume <0-100>              Set system audio volume\n",
        prog);
}

static int resolve_socket_path(const char *custom, char *buf, size_t buflen)
{
    if (custom != NULL && custom[0] != '\0') {
        int n = snprintf(buf, buflen, "%s", custom);
        return (n > 0 && (size_t)n < buflen) ? 0 : -1;
    }

    const char *xdg = getenv("XDG_RUNTIME_DIR");
    int n;
    if (xdg != NULL && xdg[0] != '\0') {
        n = snprintf(buf, buflen, "%s/dwm-oomaya/ipc.sock", xdg);
    } else {
        n = snprintf(buf, buflen, "/tmp/dwm-oomaya-%u/ipc.sock", (unsigned)getuid());
    }
    return (n > 0 && (size_t)n < buflen) ? 0 : -1;
}

static int parse_tag_arg(const char *arg, uint32_t *out_mask)
{
    char *endptr = NULL;
    unsigned long val = strtoul(arg, &endptr, 0);
    if (*endptr != '\0') return -1;

    /* If user passed single digit 1..9, convert to bitmask 1 << (val - 1) */
    if (val >= 1 && val <= 9 && strncmp(arg, "0x", 2) != 0) {
        *out_mask = (1u << (val - 1));
    } else {
        *out_mask = (uint32_t)val;
    }
    return 0;
}

static const char *err_str(int32_t err)
{
    switch ((oomaya_ipc_err_t)err) {
    case OOMAYA_IPC_OK:            return "success";
    case OOMAYA_IPC_ERR_MALFORMED: return "malformed packet or rate limit exceeded";
    case OOMAYA_IPC_ERR_VERSION:   return "unsupported protocol version";
    case OOMAYA_IPC_ERR_OPCODE:    return "unknown opcode";
    case OOMAYA_IPC_ERR_PAYLOAD:   return "payload length exceeded limit";
    case OOMAYA_IPC_ERR_INTERNAL:  return "daemon internal error";
    default:                       return "unknown error";
    }
}

int main(int argc, char **argv)
{
    const char *custom_sock = NULL;
    bool json_mode = false;
    char sock_path[256];
    int argi = 1;

    /* Parse global options */
    while (argi < argc && argv[argi][0] == '-') {
        if (strcmp(argv[argi], "--json") == 0 || strcmp(argv[argi], "-j") == 0) {
            json_mode = true;
            argi++;
        } else if (strcmp(argv[argi], "--socket") == 0 || strcmp(argv[argi], "-s") == 0) {
            if (argi + 1 >= argc) {
                fprintf(stderr, "oomaya-ctl: --socket requires a path argument\n");
                return EXIT_FAILURE;
            }
            custom_sock = argv[++argi];
            argi++;
        } else if (strcmp(argv[argi], "--help") == 0 || strcmp(argv[argi], "-h") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        } else {
            fprintf(stderr, "oomaya-ctl: unrecognized option '%s'\n", argv[argi]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (argi >= argc) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *cmd = argv[argi++];

    if (strcmp(cmd, "help") == 0) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    /* Scan trailing args for -j / --json */
    for (int ti = argi; ti < argc; ti++) {
        if (strcmp(argv[ti], "-j") == 0 || strcmp(argv[ti], "--json") == 0) {
            json_mode = true;
        }
    }

    /* Resolve socket path */
    if (resolve_socket_path(custom_sock, sock_path, sizeof(sock_path)) < 0) {
        fprintf(stderr, "oomaya-ctl: socket path resolution failed\n");
        return EXIT_FAILURE;
    }

    /* Connect to daemon */
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("oomaya-ctl: socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sock_path) >= (int)sizeof(addr.sun_path)) {
        fprintf(stderr, "oomaya-ctl: socket path too long: %s\n", sock_path);
        close(sock);
        return EXIT_FAILURE;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        if (json_mode) {
            fprintf(stdout, "{\"success\":false,\"error\":\"Cannot connect to dwm-oomayad (%s): %s\"}\n",
                    sock_path, strerror(errno));
        } else {
            fprintf(stderr, "oomaya-ctl: cannot connect to dwm-oomayad at %s: %s\n",
                    sock_path, strerror(errno));
        }
        close(sock);
        return EXIT_FAILURE;
    }

    /* Prepare command payload */
    uint16_t opcode = 0;
    uint8_t payload[64];
    uint32_t payload_len = 0;

    if (strcmp(cmd, "state") == 0 || strcmp(cmd, "get-state") == 0) {
        opcode = IPC_CMD_WM_GET_STATE;
        memcpy(payload, &opcode, 2);
        payload_len = 2;
    } else if (strcmp(cmd, "view") == 0) {
        if (argi >= argc) {
            fprintf(stderr, "oomaya-ctl: 'view' requires a tag mask argument\n");
            close(sock); return EXIT_FAILURE;
        }
        uint32_t mask = 0;
        if (parse_tag_arg(argv[argi], &mask) < 0) {
            fprintf(stderr, "oomaya-ctl: invalid tag mask '%s'\n", argv[argi]);
            close(sock); return EXIT_FAILURE;
        }
        opcode = IPC_CMD_WM_VIEW_TAG;
        memcpy(payload, &opcode, 2);
        memcpy(payload + 2, &mask, 4);
        payload_len = 6;
    } else if (strcmp(cmd, "toggle-view") == 0 || strcmp(cmd, "toggleview") == 0) {
        if (argi >= argc) {
            fprintf(stderr, "oomaya-ctl: 'toggle-view' requires a tag mask argument\n");
            close(sock); return EXIT_FAILURE;
        }
        uint32_t mask = 0;
        if (parse_tag_arg(argv[argi], &mask) < 0) {
            fprintf(stderr, "oomaya-ctl: invalid tag mask '%s'\n", argv[argi]);
            close(sock); return EXIT_FAILURE;
        }
        opcode = IPC_CMD_WM_TOGGLE_TAG;
        memcpy(payload, &opcode, 2);
        memcpy(payload + 2, &mask, 4);
        payload_len = 6;
    } else if (strcmp(cmd, "set-layout") == 0 || strcmp(cmd, "setlayout") == 0) {
        if (argi >= argc) {
            fprintf(stderr, "oomaya-ctl: 'set-layout' requires a layout index argument\n");
            close(sock); return EXIT_FAILURE;
        }
        uint32_t idx = (uint32_t)strtoul(argv[argi], NULL, 0);
        opcode = IPC_CMD_WM_SET_LAYOUT;
        memcpy(payload, &opcode, 2);
        memcpy(payload + 2, &idx, 4);
        payload_len = 6;
    } else if (strcmp(cmd, "focus") == 0 || strcmp(cmd, "focus-client") == 0) {
        if (argi >= argc) {
            fprintf(stderr, "oomaya-ctl: 'focus' requires a window XID argument\n");
            close(sock); return EXIT_FAILURE;
        }
        uint32_t xid = (uint32_t)strtoul(argv[argi], NULL, 0);
        opcode = IPC_CMD_WM_FOCUS_CLIENT;
        memcpy(payload, &opcode, 2);
        memcpy(payload + 2, &xid, 4);
        payload_len = 6;
    } else if (strcmp(cmd, "kill") == 0 || strcmp(cmd, "kill-client") == 0) {
        opcode = IPC_CMD_WM_KILL_CLIENT;
        memcpy(payload, &opcode, 2);
        payload_len = 2;
    } else if (strcmp(cmd, "cycle-monitor") == 0) {
        opcode = IPC_CMD_WM_CYCLE_MONITOR;
        memcpy(payload, &opcode, 2);
        payload_len = 2;
    } else if (strcmp(cmd, "power") == 0 || strcmp(cmd, "get-power") == 0) {
        opcode = IPC_CMD_SYS_GET_POWER;
        memcpy(payload, &opcode, 2);
        payload_len = 2;
    } else if (strcmp(cmd, "audio") == 0 || strcmp(cmd, "get-audio") == 0) {
        opcode = IPC_CMD_SYS_GET_AUDIO;
        memcpy(payload, &opcode, 2);
        payload_len = 2;
    } else if (strcmp(cmd, "volume") == 0 || strcmp(cmd, "set-volume") == 0) {
        if (argi >= argc) {
            fprintf(stderr, "oomaya-ctl: 'volume' requires a level (0-100)\n");
            close(sock); return EXIT_FAILURE;
        }
        int32_t vol = (int32_t)strtol(argv[argi], NULL, 0);
        opcode = IPC_CMD_SYS_SET_VOLUME;
        memcpy(payload, &opcode, 2);
        memcpy(payload + 2, &vol, 4);
        payload_len = 6;
    } else {
        fprintf(stderr, "oomaya-ctl: unknown command '%s'\n", cmd);
        print_usage(argv[0]);
        close(sock);
        return EXIT_FAILURE;
    }

    /* Format 16-byte wire header */
    oomaya_ipc_header_t hdr;
    oomaya_frame_init_request(&hdr, opcode, 0x4242u, payload_len);

    /* Atomic writev transmission */
    struct iovec iov[2];
    iov[0].iov_base = &hdr;
    iov[0].iov_len  = sizeof(hdr);
    iov[1].iov_base = payload;
    iov[1].iov_len  = (size_t)payload_len;

    if (writev(sock, iov, 2) < 0) {
        perror("oomaya-ctl: writev");
        close(sock);
        return EXIT_FAILURE;
    }

    /* Read response header */
    oomaya_ipc_header_t resp_hdr;
    ssize_t n = read(sock, &resp_hdr, sizeof(resp_hdr));
    if (n < (ssize_t)sizeof(resp_hdr)) {
        fprintf(stderr, "oomaya-ctl: incomplete response from daemon\n");
        close(sock);
        return EXIT_FAILURE;
    }

    if (resp_hdr.magic != OOMAYA_IPC_MAGIC) {
        fprintf(stderr, "oomaya-ctl: invalid response magic 0x%08X\n", resp_hdr.magic);
        close(sock);
        return EXIT_FAILURE;
    }

    /* Check error frames */
    if (resp_hdr.msg_type == (uint16_t)OOMAYA_MSG_ERROR) {
        int32_t err_code = (int32_t)resp_hdr.payload_len;
        if (json_mode) {
            fprintf(stdout, "{\"success\":false,\"error\":\"%s\",\"code\":%d}\n",
                    err_str(err_code), (int)err_code);
        } else {
            fprintf(stderr, "oomaya-ctl error: %s (code %d)\n",
                    err_str(err_code), (int)err_code);
        }
        close(sock);
        return EXIT_FAILURE;
    }

    /* Read response payload */
    uint8_t resp_buf[512];
    uint32_t to_read = resp_hdr.payload_len;
    if (to_read > sizeof(resp_buf)) to_read = sizeof(resp_buf);

    uint32_t read_accum = 0;
    while (read_accum < to_read) {
        ssize_t rn = read(sock, resp_buf + read_accum, to_read - read_accum);
        if (rn <= 0) break;
        read_accum += (uint32_t)rn;
    }

    close(sock);

    /* Output formatting */
    if (opcode == IPC_CMD_WM_GET_STATE) {
        if (read_accum >= sizeof(oomaya_state_resp_t)) {
            oomaya_state_resp_t st;
            memcpy(&st, resp_buf, sizeof(st));
            if (json_mode) {
                fprintf(stdout, "{\"tag_mask\":%u,\"layout_idx\":%u,\"focused_win\":%u,\"monitor_count\":%u}\n",
                        st.tag_mask, st.layout_idx, st.focused_win, st.monitor_count);
            } else {
                fprintf(stdout, "Tags:      0x%04X (mask %u)\n", st.tag_mask, st.tag_mask);
                fprintf(stdout, "Layout:    %u\n", st.layout_idx);
                fprintf(stdout, "Focus:     0x%X\n", st.focused_win);
                fprintf(stdout, "Monitors:  %u\n", st.monitor_count);
            }
        }
    } else if (opcode == IPC_CMD_SYS_GET_POWER) {
        int32_t pwr = 0;
        if (read_accum >= sizeof(pwr)) memcpy(&pwr, resp_buf, sizeof(pwr));
        if (json_mode) {
            fprintf(stdout, "{\"power\":%d}\n", (int)pwr);
        } else {
            fprintf(stdout, "Power: %d%%\n", (int)pwr);
        }
    } else if (opcode == IPC_CMD_SYS_GET_AUDIO) {
        int32_t aud = 0;
        if (read_accum >= sizeof(aud)) memcpy(&aud, resp_buf, sizeof(aud));
        if (json_mode) {
            fprintf(stdout, "{\"volume\":%d}\n", (int)aud);
        } else {
            fprintf(stdout, "Volume: %d%%\n", (int)aud);
        }
    } else {
        /* Generic mutation success */
        if (json_mode) {
            fprintf(stdout, "{\"success\":true,\"command\":\"%s\"}\n", cmd);
        } else {
            fprintf(stdout, "OK\n");
        }
    }

    return EXIT_SUCCESS;
}
