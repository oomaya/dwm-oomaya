#!/usr/bin/env bash
set -euo pipefail

# Real dwm-oomayad and oomaya-ctl with a stub wpctl, then the scripts that use
# them with a stub oomaya-ctl. No display, no user bus, no real audio.

repo=$(
	unset CDPATH
	cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd
)
work=$(mktemp -d)
daemon_pid=
status_pid=
cleanup() {
	local pid
	for pid in "$status_pid" "$daemon_pid"; do
		[[ -n $pid ]] || continue
		kill -TERM "$pid" 2>/dev/null || true
		wait "$pid" 2>/dev/null || true
	done
	rm -rf "$work"
}
trap cleanup EXIT

fail() {
	printf 'FAIL: %s\n' "$1" >&2
	exit 1
}

expect() {
	[[ $2 == "$3" ]] || fail "$1: expected '$3', got '$2'"
}

unset DISPLAY
export XDG_RUNTIME_DIR=$work/runtime
export AUDIO_TEST_DIR=$work
export DWM_STATUS_TEST_LOG=$work/status.log
export DWM_STATUS_POWER_SUPPLY_DIR=$work/power
mkdir -p "$work/daemon-bin" "$work/bin" "$XDG_RUNTIME_DIR" "$work/power"
chmod 700 "$XDG_RUNTIME_DIR"

# ── Daemon and client ────────────────────────────────────────────────────────

cat >"$work/daemon-bin/wpctl" <<'SH'
#!/bin/sh
[ -f "$AUDIO_TEST_DIR/wpctl.output" ] || exit 1
cat "$AUDIO_TEST_DIR/wpctl.output"
SH
chmod +x "$work/daemon-bin/wpctl"

PATH="$work/daemon-bin:$PATH" "$repo/dwm-oomayad" >"$work/daemon.log" 2>&1 &
daemon_pid=$!
for _ in {1..200}; do
	[[ -S $XDG_RUNTIME_DIR/dwm-oomaya/ipc.sock ]] && break
	sleep 0.01
done
[[ -S $XDG_RUNTIME_DIR/dwm-oomaya/ipc.sock ]] || fail 'daemon socket never appeared'

ctl=$repo/tools/oomaya-ctl

printf 'Volume: 0.42\n' >"$work/wpctl.output"
expect 'unmuted json' "$("$ctl" -j audio)" '{"volume":42,"muted":false}'
expect 'unmuted text' "$("$ctl" audio)" 'Volume: 42%'

printf 'Volume: 0.42 [MUTED]\n' >"$work/wpctl.output"
expect 'muted json' "$("$ctl" -j audio)" '{"volume":42,"muted":true}'
expect 'muted text' "$("$ctl" audio)" 'Volume: 42% (muted)'

# A stalled daemon must not hang the caller.
kill -STOP "$daemon_pid"
if timeout 5 "$ctl" -j audio >/dev/null 2>&1; then
	kill -CONT "$daemon_pid"
	fail 'stalled daemon answered'
else
	stalled_rc=$?
fi
kill -CONT "$daemon_pid"
[[ $stalled_rc -ne 124 ]] || fail 'oomaya-ctl hung on a stalled daemon'

# No audio server must read as a failure, never as volume 0.
rm "$work/wpctl.output"
if out=$("$ctl" -j audio 2>/dev/null); then
	fail "audio without wpctl succeeded: $out"
fi
[[ $out != *'"volume"'* ]] || fail "failure reported a volume: $out"

kill -TERM "$daemon_pid"
wait "$daemon_pid" 2>/dev/null || true
daemon_pid=

# A response without a body must fail, so that scripts fall back.
if command -v python3 >/dev/null 2>&1; then
	mkdir -p "$work/fake"
	python3 -I - "$work/fake/ipc.sock" >"$work/fake.ready" <<'PY' &
import socket
import sys

server = socket.socket(socket.AF_UNIX)
server.bind(sys.argv[1])
server.listen(1)
print("ready", flush=True)
client, _ = server.accept()
request = client.recv(64)
client.sendall(request[:12] + bytes(4))
client.close()
PY
	daemon_pid=$!
	for _ in {1..200}; do
		[[ -s $work/fake.ready ]] && break
		sleep 0.01
	done
	if out=$("$ctl" --socket "$work/fake/ipc.sock" -j audio 2>/dev/null); then
		fail "empty audio response succeeded: $out"
	fi
	wait "$daemon_pid" 2>/dev/null || true
	daemon_pid=
fi

# ── Stubs for script consumers ───────────────────────────────────────────────

cat >"$work/bin/oomaya-ctl" <<'SH'
#!/bin/sh
printf '%s\n' "$*" >>"$AUDIO_TEST_DIR/ctl.calls"
case "$*" in
"-j audio")
	[ -f "$AUDIO_TEST_DIR/ctl.audio" ] || exit 1
	cat "$AUDIO_TEST_DIR/ctl.audio"
	;;
volume\ *)
	[ ! -f "$AUDIO_TEST_DIR/ctl.volume-fails" ] || exit 1
	echo OK
	;;
esac
SH
cat >"$work/bin/pactl" <<'SH'
#!/bin/sh
printf '%s\n' "$*" >>"$AUDIO_TEST_DIR/pactl.calls"
case $1 in
get-sink-mute) echo 'Mute: yes' ;;
get-sink-volume) echo 'Volume: front-left: 0 / 30% / 0 dB' ;;
subscribe) exec tail -f /dev/null ;;
esac
SH
cat >"$work/bin/notify-send" <<'SH'
#!/bin/sh
printf '%s\n' "$*" >>"$AUDIO_TEST_DIR/notify.calls"
SH
cat >"$work/bin/xsetroot" <<'SH'
#!/bin/sh
printf '%s\n' "$2" >>"$DWM_STATUS_TEST_LOG"
SH
cat >"$work/bin/xdotool" <<'SH'
#!/bin/sh
printf '%s\n' "$*" >>"$AUDIO_TEST_DIR/xdotool.calls"
SH
printf '#!/bin/sh\nexec sleep 60\n' >"$work/bin/udevadm"
chmod +x "$work"/bin/*

reset_calls() {
	rm -f "$work/ctl.audio" "$work/ctl.volume-fails"
	: >"$work/ctl.calls"
	: >"$work/pactl.calls"
	: >"$work/notify.calls"
	: >"$work/xdotool.calls"
}

run_volume() {
	PATH="$work/bin:/usr/bin:/bin" "$repo/scripts/dwm-volume" "$@"
}

called() {
	grep -Fxq -- "$2" "$work/$1.calls"
}

# ── dwm-volume ───────────────────────────────────────────────────────────────

reset_calls
printf '{"volume":50,"muted":false}\n' >"$work/ctl.audio"
run_volume up 5%
called ctl 'volume 55' || fail 'volume up did not use the daemon'
if grep -q set-sink-volume "$work/pactl.calls"; then
	fail 'volume up changed the sink twice'
fi
if grep -q . "$work/pactl.calls"; then
	fail 'daemon path should not call pactl'
fi
grep -q 'Volume: 50%' "$work/notify.calls" || fail 'notification missing'

reset_calls
printf '{"volume":50,"muted":true}\n' >"$work/ctl.audio"
run_volume status >"$work/out"
expect 'status volume' "$(cat "$work/out")" '50%'
run_volume down 5%
called ctl 'volume 45' || fail 'volume down did not use the daemon'
grep -q 'Volume Muted' "$work/notify.calls" || fail 'mute did not come from the daemon'
if grep -q . "$work/pactl.calls"; then
	fail 'muted daemon path should not call pactl'
fi

# Daemon stopped: oomaya-ctl exists but cannot answer.
reset_calls
run_volume up 5%
called pactl 'set-sink-volume @DEFAULT_SINK@ +5%' || fail 'no fallback with daemon down'

# The daemon answers but cannot set the sink.
reset_calls
printf '{"volume":50,"muted":false}\n' >"$work/ctl.audio"
: >"$work/ctl.volume-fails"
run_volume up 5%
called pactl 'set-sink-volume @DEFAULT_SINK@ +5%' || fail 'no fallback when set fails'

# A sink above 100% must not be pulled down by the daemon's 0-100 range.
reset_calls
printf '{"volume":120,"muted":false}\n' >"$work/ctl.audio"
run_volume up 5%
called pactl 'set-sink-volume @DEFAULT_SINK@ +5%' || fail 'over-100 sink was clamped'
if grep -q '^volume ' "$work/ctl.calls"; then
	fail 'over-100 sink used the daemon'
fi

# An older daemon reports no mute state.
reset_calls
printf '{"volume":50}\n' >"$work/ctl.audio"
run_volume down 5%
called ctl 'volume 45' || fail 'older daemon volume path broke'
grep -q 'Volume Muted' "$work/notify.calls" || fail 'older daemon lost pactl mute state'

# ── dwm-status ───────────────────────────────────────────────────────────────

status_once() {
	: >"$DWM_STATUS_TEST_LOG"
	DISPLAY=:199 PATH="$work/bin:/usr/bin:/bin" "$repo/scripts/dwm-status" &
	status_pid=$!
	for _ in {1..200}; do
		grep -q 'VOL' "$DWM_STATUS_TEST_LOG" && break
		sleep 0.01
	done
	kill -TERM "$status_pid" 2>/dev/null || true
	wait "$status_pid" 2>/dev/null || true
	status_pid=
}

reset_calls
printf '{"volume":64,"muted":false}\n' >"$work/ctl.audio"
status_once
grep -q 'VOL 64%' "$DWM_STATUS_TEST_LOG" || fail 'bar did not show daemon volume'
if grep -q 'get-sink' "$work/pactl.calls"; then
	fail 'bar called pactl with a working daemon'
fi

reset_calls
printf '{"volume":64,"muted":true}\n' >"$work/ctl.audio"
status_once
grep -q 'VOL muted' "$DWM_STATUS_TEST_LOG" || fail 'bar lost daemon mute state'
if grep -q 'get-sink' "$work/pactl.calls"; then
	fail 'bar called pactl for mute with a working daemon'
fi

# Older daemon: no mute field, so pactl decides. This stub reports muted.
reset_calls
printf '{"volume":64}\n' >"$work/ctl.audio"
status_once
grep -q 'VOL muted' "$DWM_STATUS_TEST_LOG" || fail 'bar lost mute with an older daemon'

# Daemon stopped.
reset_calls
status_once
grep -q 'VOL muted' "$DWM_STATUS_TEST_LOG" || fail 'bar did not fall back to pactl'

# ── dwm-keybind-exec ─────────────────────────────────────────────────────────

# dwm-oomayad cannot perform these, and it reports success anyway. They must
# keep using the real key binding until dwm accepts them over IPC.
for spec in 'toggleview:tag:2:super+ctrl+2' 'setlayout:i:1:super+m' 'focusmon:i:-1:super+comma'; do
	func=${spec%%:*}
	rest=${spec#*:}
	keys=${rest##*:}
	action=${rest%:*}
	reset_calls
	PATH="$work/bin:/usr/bin:/bin" "$repo/scripts/dwm-keybind-exec" "$func" "$action" "$keys"
	for _ in {1..100}; do
		grep -q . "$work/xdotool.calls" && break
		sleep 0.01
	done
	called xdotool "key --clearmodifiers $keys" || fail "$func did not use its key binding"
	if grep -q . "$work/ctl.calls"; then
		fail "$func was sent to the daemon"
	fi
done

printf 'audio consumer checks passed\n'
