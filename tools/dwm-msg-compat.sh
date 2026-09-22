#!/bin/sh
# dwm-msg-compat — Drop-in replacement for dwm-msg.
#
# If dwm-oomayad is running (socket present), delegates to oomaya-ctl (or raw UDS socket).
# If the daemon is offline, falls back to the original dwm-msg path.
#
# Usage:
#   dwm-msg run_command <command> [args...]
#   dwm-msg <command> [args...]
#
# POSIX sh. No hardcoded paths. No external runtimes.
set -eu

# ── Resolve socket path (mirrors daemon logic) ────────────────────────────
resolve_sock() {
    if [ -n "${XDG_RUNTIME_DIR:-}" ]; then
        printf '%s/dwm-oomaya/ipc.sock' "$XDG_RUNTIME_DIR"
    else
        printf '/tmp/dwm-oomaya-%s/ipc.sock' "$(id -u)"
    fi
}

SOCK="$(resolve_sock)"

# ── Check daemon availability ─────────────────────────────────────────────
daemon_up() {
    [ -S "$SOCK" ]
}

# ── Parse arguments ───────────────────────────────────────────────────────
if [ "$#" -eq 0 ]; then
    printf 'usage: %s [run_command] <command> [args...]\n' "$0" >&2
    exit 1
fi

CMD=$1
shift

if [ "$CMD" = "run_command" ]; then
    if [ "$#" -eq 0 ]; then
        printf 'dwm-msg-compat: run_command requires a command name\n' >&2
        exit 1
    fi
    CMD=$1
    shift
fi

ARG="${1:-0}"

# ── Fast path: delegate to oomaya-ctl ─────────────────────────────────────
dispatch_oomaya_ctl() {
    if ! command -v oomaya-ctl >/dev/null 2>&1; then
        return 1
    fi

    case "$CMD" in
        view)
            oomaya-ctl view "$ARG"
            ;;
        toggleview)
            oomaya-ctl toggleview "$ARG"
            ;;
        setlayout)
            oomaya-ctl setlayout "$ARG"
            ;;
        focusclient|focus)
            oomaya-ctl focus "$ARG"
            ;;
        killclient|kill)
            oomaya-ctl kill
            ;;
        cycle-monitor|focusmon)
            oomaya-ctl cycle-monitor
            ;;
        get_state|getstate|state|get_monitors|get_layouts)
            oomaya-ctl state -j
            ;;
        *)
            return 1
            ;;
    esac
    return 0
}

# ── Raw IPC frame fallback (socat / nc) ───────────────────────────────────
# shellcheck disable=SC2059
send_raw_ipc() {
    _opcode="$1"
    _arg="${2:-0}"

    _hdr='\x42\x4D\x4F\x4F\x00\x01\x00\x01\x01\x00\x01\x00\x06\x00\x00\x00'

    _op_lo=$(printf '%02x' $(( _opcode & 0xFF )) )
    _op_hi=$(printf '%02x' $(( (_opcode >> 8) & 0xFF )) )
    _a0=$(printf '%02x' $(( _arg & 0xFF )) )
    _a1=$(printf '%02x' $(( (_arg >> 8) & 0xFF )) )
    _a2=$(printf '%02x' $(( (_arg >> 16) & 0xFF )) )
    _a3=$(printf '%02x' $(( (_arg >> 24) & 0xFF )) )

    if command -v socat >/dev/null 2>&1; then
        printf "${_hdr}\\x${_op_lo}\\x${_op_hi}\\x${_a0}\\x${_a1}\\x${_a2}\\x${_a3}" \
            | socat - "UNIX-CONNECT:${SOCK}" 2>/dev/null
    else
        printf "${_hdr}\\x${_op_lo}\\x${_op_hi}\\x${_a0}\\x${_a1}\\x${_a2}\\x${_a3}" \
            | nc -U -q1 "$SOCK" 2>/dev/null || true
    fi
}

if daemon_up; then
    if dispatch_oomaya_ctl; then
        exit 0
    fi

    case "$CMD" in
        view)         send_raw_ipc 0x0102 "$ARG" ;;
        toggleview)   send_raw_ipc 0x0103 "$ARG" ;;
        setlayout)    send_raw_ipc 0x0104 "$ARG" ;;
        focusclient)  send_raw_ipc 0x0105 "$ARG" ;;
        killclient)   send_raw_ipc 0x0106 0       ;;
        getstate)     send_raw_ipc 0x0101 0       ;;
        *)
            printf 'dwm-msg-compat: unknown command: %s\n' "$CMD" >&2
            exit 1
            ;;
    esac
else
    # Daemon offline — fall back to original dwm-msg if present
    if command -v /usr/bin/dwm-msg >/dev/null 2>&1; then
        exec /usr/bin/dwm-msg "$CMD" "$@"
    elif command -v /usr/local/bin/dwm-msg >/dev/null 2>&1; then
        exec /usr/local/bin/dwm-msg "$CMD" "$@"
    else
        printf 'dwm-msg-compat: daemon offline and system dwm-msg not found\n' >&2
        exit 1
    fi
fi
