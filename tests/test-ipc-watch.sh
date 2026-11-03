#!/usr/bin/env bash
set -euo pipefail

# Real dwm-oomayad and `oomaya-ctl watch` for pure WM event streaming (tag,
# layout, focus). Zero audio subprocesses, no display, no user bus required.

repo=$(
	unset CDPATH
	cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd
)
work=$(mktemp -d)
daemon_pid=
watch_pid=
tag_pid=
cleanup() {
	local pid
	for pid in "$tag_pid" "$watch_pid" "$daemon_pid"; do
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
export XDG_RUNTIME_DIR=$work/runtime
mkdir -p "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"

wait_for_line() {
	local expected=$1
	local file=${2:-$work/watch.out}
	for _ in {1..400}; do
		grep -Fxq -- "$expected" "$file" && return 0
		sleep 0.01
	done
	cat "$file" >&2
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

# Audio filter must be rejected by purified oomaya-ctl.
if "$repo/tools/oomaya-ctl" watch audio >/dev/null 2>&1; then
	fail 'watch audio should be rejected'
fi

# Subscribe to all WM events (tag, layout, focus).
start_watch tag layout focus
sleep 0.05

# Tag mutation emits tag event and suppresses duplicate unchanged masks.
"$repo/tools/oomaya-ctl" view 3 >/dev/null
wait_for_line '{"event":"tag","mask":4}'
"$repo/tools/oomaya-ctl" view 3 >/dev/null
sleep 0.05
[[ $(grep -c '"event":"tag"' "$work/watch.out") -eq 1 ]] || fail 'unchanged tag mask was emitted twice'

# Focus mutation emits focus event.
"$repo/tools/oomaya-ctl" focus 42 >/dev/null
wait_for_line '{"event":"focus","window":42}'

# Filtered watcher only prints requested WM domain.
"$repo/tools/oomaya-ctl" watch tag >"$work/tag.out" 2>&1 &
tag_pid=$!
sleep 0.05
"$repo/tools/oomaya-ctl" focus 99 >/dev/null
"$repo/tools/oomaya-ctl" view 1 >/dev/null
wait_for_line '{"event":"tag","mask":1}' "$work/tag.out"
if grep -q '"event":"focus"' "$work/tag.out"; then
	fail 'tag-only watcher printed focus event'
fi
kill "$tag_pid" 2>/dev/null || true
wait "$tag_pid" 2>/dev/null || true
tag_pid=

# Bare watch (no filter) defaults to WATCH_ALL: tag and focus both arrive.
"$repo/tools/oomaya-ctl" watch >"$work/all.out" 2>&1 &
tag_pid=$!
sleep 0.05
"$repo/tools/oomaya-ctl" view 2 >/dev/null
"$repo/tools/oomaya-ctl" focus 7 >/dev/null
wait_for_line '{"event":"tag","mask":2}' "$work/all.out"
wait_for_line '{"event":"focus","window":7}' "$work/all.out"
kill "$tag_pid" 2>/dev/null || true
wait "$tag_pid" 2>/dev/null || true
tag_pid=

# Stop from idle must be prompt, and watch must report the loss.
kill -TERM "$daemon_pid"
wait "$daemon_pid" 2>/dev/null || true
daemon_pid=
if wait "$watch_pid"; then
	fail 'watch exited 0 when the daemon went away'
fi
watch_pid=

printf 'ipc watch checks passed\n'
