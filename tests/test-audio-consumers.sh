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

"$repo/dwm-oomayad" >"$work/daemon.log" 2>&1 &
daemon_pid=$!
for _ in {1..200}; do
	[[ -S $XDG_RUNTIME_DIR/dwm-oomaya/ipc.sock ]] && break
	sleep 0.01
done
[[ -S $XDG_RUNTIME_DIR/dwm-oomaya/ipc.sock ]] || fail 'daemon socket never appeared'

ctl=$repo/tools/oomaya-ctl

# Audio commands are decoupled from oomaya-ctl and must be rejected.
for command in 'audio' '-j audio' 'volume 50' 'watch audio'; do
	# shellcheck disable=SC2086
	if out=$("$ctl" $command 2>&1); then
		fail "$command should be rejected by purified oomaya-ctl: $out"
	fi
done

# The daemon cannot perform these, so it must refuse them, not report success.
for command in 'toggleview 2' 'setlayout 1'; do
	# shellcheck disable=SC2086
	if out=$("$ctl" $command 2>&1); then
		fail "$command reported success: $out"
	fi
done
for command in 'toggleview 2' 'setlayout 1'; do
	# shellcheck disable=SC2086
	if out=$(PATH="$repo/tools:$PATH" sh "$repo/tools/dwm-msg-compat.sh" $command 2>&1); then
		fail "dwm-msg $command reported success: $out"
	fi
done
expect 'refused layout left state alone' "$("$ctl" -j state | grep -o '"layout_idx":[0-9]*')" '"layout_idx":0'

# A stalled daemon must not hang the caller.
kill -STOP "$daemon_pid"
if timeout 5 "$ctl" -j state >/dev/null 2>&1; then
	kill -CONT "$daemon_pid"
	fail 'stalled daemon answered'
else
	stalled_rc=$?
fi
kill -CONT "$daemon_pid"
[[ $stalled_rc -ne 124 ]] || fail 'oomaya-ctl hung on a stalled daemon'

kill -TERM "$daemon_pid"
wait "$daemon_pid" 2>/dev/null || true
daemon_pid=

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
"watch audio")
	[ -f "$AUDIO_TEST_DIR/ctl.watch" ] || exit 1
	exec tail -n +1 -f "$AUDIO_TEST_DIR/ctl.watch"
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
	rm -f "$work/ctl.audio" "$work/ctl.volume-fails" "$work/ctl.watch"
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

# ── dwm-volume (pure pactl direct, zero daemon calls) ────────────────────────

reset_calls
run_volume up 5%
called pactl 'set-sink-volume @DEFAULT_SINK@ +5%' || fail 'volume up did not call pactl'
if grep -q . "$work/ctl.calls"; then
	fail 'volume up should not call oomaya-ctl'
fi

reset_calls
run_volume status >"$work/out"
expect 'status volume' "$(cat "$work/out")" '30%'
run_volume down 5%
called pactl 'set-sink-volume @DEFAULT_SINK@ -5%' || fail 'volume down did not call pactl'
grep -q 'Volume Muted' "$work/notify.calls" || fail 'mute notification missing'
if grep -q . "$work/ctl.calls"; then
	fail 'volume down should not call oomaya-ctl'
fi

reset_calls
run_volume mute
called pactl 'set-sink-mute @DEFAULT_SINK@ toggle' || fail 'mute did not call pactl'
if grep -q . "$work/ctl.calls"; then
	fail 'mute should not call oomaya-ctl'
fi

# ── dwm-status (pure pactl subscribe, zero daemon calls) ─────────────────────

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
status_once
grep -q 'VOL muted' "$DWM_STATUS_TEST_LOG" || fail 'bar did not show pactl volume'
called pactl 'get-sink-mute @DEFAULT_SINK@' || fail 'bar did not query pactl mute'
called pactl 'get-sink-volume @DEFAULT_SINK@' || fail 'bar did not query pactl volume'
if grep -q . "$work/ctl.calls"; then
	fail 'bar should not call oomaya-ctl'
fi

# ── dwm-keybind-exec ─────────────────────────────────────────────────────────

# dwm-oomayad cannot perform these. They must keep using the real key binding
# until dwm accepts them over IPC.
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
