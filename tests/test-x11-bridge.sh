#!/bin/sh
# Run the X11 bridge test on a private Xvfb. Exit 77 when Xvfb is unavailable.

set -eu

repo_dir=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)

if ! command -v Xvfb >/dev/null 2>&1; then
	printf '%s\n' 'SKIP: X11 bridge test needs Xvfb' >&2
	exit 77
fi

work=$(mktemp -d "${TMPDIR:-/tmp}/oomaya-x11.XXXXXX")
xvfb_pid=
cleanup() {
	if [ -n "$xvfb_pid" ]; then
		kill "$xvfb_pid" 2>/dev/null || true
		i=0
		while kill -0 "$xvfb_pid" 2>/dev/null && [ "$i" -lt 100 ]; do
			i=$((i + 1))
			sleep 0.05
		done
		kill -KILL "$xvfb_pid" 2>/dev/null || true
		wait "$xvfb_pid" 2>/dev/null || true
	fi
	rm -rf -- "$work"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

# Never use -displayfd here: it skips the lock check and can take over a live
# display, deleting its socket on exit. A plain Xvfb :N honors the lock, and
# the lock and socket checks skip any display that is already claimed.
start_xvfb() {
	Xvfb ":$1" -noreset -screen 0 640x480x24 -nolisten tcp \
		>"$work/xvfb.log" 2>&1 &
	xvfb_pid=$!
	i=0
	while [ "$i" -lt 200 ]; do
		[ -S "/tmp/.X11-unix/X$1" ] && [ -e "/tmp/.X$1-lock" ] && return 0
		kill -0 "$xvfb_pid" 2>/dev/null || break
		i=$((i + 1))
		sleep 0.05
	done
	wait "$xvfb_pid" 2>/dev/null || true
	xvfb_pid=
	return 1
}

display=
n=700
while [ "$n" -lt 730 ]; do
	if [ ! -e "/tmp/.X$n-lock" ] && [ ! -e "/tmp/.X11-unix/X$n" ] &&
		start_xvfb "$n"; then
		display=":$n"
		break
	fi
	n=$((n + 1))
done
if [ -z "$display" ]; then
	printf '%s\n' 'Xvfb did not become ready' >&2
	exit 1
fi

cd "$repo_dir"
OOMAYA_TEST_X11_DISPLAY=$display make -f Makefile.ipc test_x11
