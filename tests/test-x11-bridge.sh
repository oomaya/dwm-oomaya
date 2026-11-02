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
		wait "$xvfb_pid" 2>/dev/null || true
	fi
	rm -rf -- "$work"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

# -displayfd picks a free display, so a live session can never be selected.
Xvfb -displayfd 3 -noreset -screen 0 640x480x24 -nolisten tcp \
	3>"$work/display" >/dev/null 2>&1 &
xvfb_pid=$!

i=0
while [ ! -s "$work/display" ]; do
	i=$((i + 1))
	if [ "$i" -ge 200 ] || ! kill -0 "$xvfb_pid" 2>/dev/null; then
		printf '%s\n' 'Xvfb did not become ready' >&2
		exit 1
	fi
	sleep 0.05
done

cd "$repo_dir"
OOMAYA_TEST_X11_DISPLAY=":$(cat "$work/display")" make -f Makefile.ipc test_x11
