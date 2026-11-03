#!/usr/bin/env bash
set -euo pipefail

# Real dwm-oomayad and `oomaya-ctl watch` with stub audio tools. No display, no
# user bus, no real audio.

repo=$(
	unset CDPATH
	cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd
)
work=$(mktemp -d)
daemon_pid=
watch_pid=
cleanup() {
	local pid
	for pid in "$watch_pid" "$daemon_pid"; do
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

unset DISPLAY
export XDG_RUNTIME_DIR=$work/runtime WATCH_TEST_DIR=$work
mkdir -p "$work/bin" "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"
: >"$work/events"
: >"$work/wpctl.calls"
printf 'Volume: 0.50\n' >"$work/wpctl.output"

cat >"$work/bin/wpctl" <<'SH'
#!/bin/sh
echo call >>"$WATCH_TEST_DIR/wpctl.calls"
cat "$WATCH_TEST_DIR/wpctl.output"
SH
cat >"$work/bin/pactl" <<'SH'
#!/bin/sh
[ "$1" = subscribe ] || exit 1
echo "$$" >"$WATCH_TEST_DIR/pactl.pid"
[ ! -f "$WATCH_TEST_DIR/pactl.fails" ] || exit 1
exec tail -n 0 -f "$WATCH_TEST_DIR/events"
SH
chmod +x "$work"/bin/*
export PATH="$work/bin:$PATH"

emit() {
	printf "Event '%s' on %s #1\n" "$1" "$2" >>"$work/events"
}

wait_for_line() {
	local expected=$1
	for _ in {1..400}; do
		grep -Fxq -- "$expected" "$work/watch.out" && return 0
		sleep 0.01
	done
	cat "$work/watch.out" >&2
	fail "watch never printed $expected"
}

start_watch() {
	: >"$work/watch.out"
	"$repo/tools/oomaya-ctl" watch "$@" >"$work/watch.out" 2>"$work/watch.err" &
	watch_pid=$!
}

"$repo/dwm-oomayad" >"$work/daemon.log" 2>&1 &
daemon_pid=$!
for _ in {1..200}; do
	[[ -S $XDG_RUNTIME_DIR/dwm-oomaya/ipc.sock ]] && break
	sleep 0.01
done
[[ -S $XDG_RUNTIME_DIR/dwm-oomaya/ipc.sock ]] || fail 'daemon socket never appeared'

# No subscriber means no audio child.
[[ ! -e $work/pactl.pid ]] || fail 'audio source ran without a subscriber'

start_watch audio
wait_for_line '{"event":"audio","volume":50,"muted":false}'
for _ in {1..200}; do
	[[ -s $work/pactl.pid ]] && break
	sleep 0.01
done
[[ -s $work/pactl.pid ]] || fail 'audio source did not start'

printf 'Volume: 0.42 [MUTED]\n' >"$work/wpctl.output"
emit change sink
wait_for_line '{"event":"audio","volume":42,"muted":true}'

# A non-audio event costs nothing, and an unchanged value prints nothing.
calls=$(wc -l <"$work/wpctl.calls")
emit change client
sleep 0.2
[[ $(wc -l <"$work/wpctl.calls") -eq $calls ]] || fail 'client event read the sink'
emit change sink
for _ in {1..200}; do
	[[ $(wc -l <"$work/wpctl.calls") -gt $calls ]] && break
	sleep 0.01
done
sleep 0.1
[[ $(wc -l <"$work/watch.out") -eq 2 ]] || fail 'unchanged audio was reported'

printf 'Volume: 0.41\n' >"$work/wpctl.output"
printf "Event 'change' on si" >>"$work/events"
sleep 0.2
printf 'nk #1\n' >>"$work/events"
wait_for_line '{"event":"audio","volume":41,"muted":false}'

# A burst collapses instead of queueing one read per event.
calls=$(wc -l <"$work/wpctl.calls")
printf 'Volume: 0.43\n' >"$work/wpctl.output"
for _ in {1..50}; do
	emit change sink
done
wait_for_line '{"event":"audio","volume":43,"muted":false}'
sleep 0.2
burst=$(($(wc -l <"$work/wpctl.calls") - calls))
((burst < 25)) || fail "burst of 50 ran $burst reads"

# A later subscriber starts from the current state, and tag filtering applies.
"$repo/tools/oomaya-ctl" watch tag >"$work/tag.out" 2>&1 &
tag_pid=$!
sleep 0.1
"$repo/tools/oomaya-ctl" view 3 >/dev/null
for _ in {1..200}; do
	grep -Fxq '{"event":"tag","mask":4}' "$work/tag.out" && break
	sleep 0.01
done
grep -Fxq '{"event":"tag","mask":4}' "$work/tag.out" || fail 'tag event missing'
if grep -q audio "$work/tag.out"; then
	fail 'tag watcher printed audio'
fi
kill "$tag_pid"
wait "$tag_pid" 2>/dev/null || true

# The audio server going away restarts the source and catches up.
pactl_pid=$(cat "$work/pactl.pid")
: >"$work/pactl.pid"
printf 'Volume: 0.44\n' >"$work/wpctl.output"
kill "$pactl_pid"
wait_for_line '{"event":"audio","volume":44,"muted":false}'

# The last subscriber leaving stops the child.
pactl_pid=$(cat "$work/pactl.pid")
kill "$watch_pid"
wait "$watch_pid" 2>/dev/null || true
watch_pid=
for _ in {1..200}; do
	kill -0 "$pactl_pid" 2>/dev/null || break
	sleep 0.01
done
if kill -0 "$pactl_pid" 2>/dev/null; then
	fail 'audio source outlived its subscribers'
fi

# A missing audio server must not stop WM events or hurt the daemon.
: >"$work/pactl.fails"
start_watch tag
sleep 0.1
"$repo/tools/oomaya-ctl" view 1 >/dev/null
wait_for_line '{"event":"tag","mask":1}'

# Stop from idle must be prompt, and watch must report the loss.
kill -TERM "$daemon_pid"
wait "$daemon_pid" 2>/dev/null || true
daemon_pid=
if wait "$watch_pid"; then
	fail 'watch exited 0 when the daemon went away'
fi
watch_pid=

printf 'ipc watch checks passed\n'
