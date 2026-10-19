#!/bin/sh
set -eu

repo_dir=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)
work=$(mktemp -d)

cleanup() {
	rm -rf "$work"
}
trap cleanup EXIT HUP INT TERM

printf '==> Running native dwm-shell-mode tests\n'

mkdir -p "$work/bin" "$work/config/dwm-oomaya" "$work/config/quickshell" "$work/state"
export PATH="$work/bin:$PATH"
export XDG_CONFIG_HOME="$work/config"
export HOME="$work"

# Mock quickshell binary
cat >"$work/bin/quickshell" <<'EOF'
#!/bin/sh
echo "quickshell $*" >> "${TEST_STATE:-/tmp}/quickshell.log"
EOF
chmod +x "$work/bin/quickshell"

# Mock dunst binary
cat >"$work/bin/dunst" <<'EOF'
#!/bin/sh
echo "dunst $*" >> "${TEST_STATE:-/tmp}/dunst.log"
EOF
chmod +x "$work/bin/dunst"

# Mock dwm-status binary
cat >"$work/bin/dwm-status" <<'EOF'
#!/bin/sh
echo "dwm-status $*" >> "${TEST_STATE:-/tmp}/status.log"
EOF
chmod +x "$work/bin/dwm-status"

# Mock notify-send binary
cat >"$work/bin/notify-send" <<'EOF'
#!/bin/sh
echo "notify-send $*" >> "${TEST_STATE:-/tmp}/notify.log"
EOF
chmod +x "$work/bin/notify-send"

export TEST_STATE="$work/state"

# Test 1: dwm-shell-mode --dry-run exits 0
printf 'Test 1: dwm-shell-mode --dry-run exits 0... '
if "$repo_dir/scripts/dwm-shell-mode" --dry-run; then
	printf 'PASS\n'
else
	printf 'FAIL: --dry-run returned non-zero\n' >&2
	exit 1
fi

# Test 2: dwm-shell-mode --list outputs suckless and quickshell options
printf 'Test 2: dwm-shell-mode --list outputs options... '
list_out=$("$repo_dir/scripts/dwm-shell-mode" --list)
if printf '%s\n' "$list_out" | grep -q 'suckless' && printf '%s\n' "$list_out" | grep -q 'quickshell'; then
	printf 'PASS\n'
else
	printf 'FAIL: --list output unexpected: %s\n' "$list_out" >&2
	exit 1
fi

# Test 3: dwm-shell-mode current defaults to suckless when unconfigured
printf 'Test 3: dwm-shell-mode current defaults to suckless... '
current_out=$("$repo_dir/scripts/dwm-shell-mode" current)
if [ "$current_out" = "suckless" ]; then
	printf 'PASS\n'
else
	printf 'FAIL: default mode is not suckless (%s)\n' "$current_out" >&2
	exit 1
fi

# Test 4: dwm-shell-mode current respects DWM_SHELL_MODE environment override
printf 'Test 4: dwm-shell-mode current respects DWM_SHELL_MODE... '
env_out=$(DWM_SHELL_MODE=quickshell "$repo_dir/scripts/dwm-shell-mode" current)
if [ "$env_out" = "quickshell" ]; then
	printf 'PASS\n'
else
	printf 'FAIL: environment override failed (%s)\n' "$env_out" >&2
	exit 1
fi

# Test 5: dwm-shell-mode suckless persists suckless to config
printf 'Test 5: dwm-shell-mode suckless persists suckless... '
"$repo_dir/scripts/dwm-shell-mode" suckless >/dev/null
if [ -f "$work/config/dwm-oomaya/shell-mode" ] && [ "$(cat "$work/config/dwm-oomaya/shell-mode")" = "suckless" ]; then
	printf 'PASS\n'
else
	printf 'FAIL: failed to persist suckless mode\n' >&2
	exit 1
fi

# Test 6: dwm-shell-mode quickshell persists quickshell when config exists
printf 'Test 6: dwm-shell-mode quickshell persists quickshell... '
: >"$work/config/quickshell/shell.qml"
"$repo_dir/scripts/dwm-shell-mode" quickshell >/dev/null
if [ "$(cat "$work/config/dwm-oomaya/shell-mode")" = "quickshell" ]; then
	printf 'PASS\n'
else
	printf 'FAIL: failed to persist quickshell mode\n' >&2
	exit 1
fi

# Test 7: dwm-shell-mode toggle switches between modes
printf 'Test 7: dwm-shell-mode toggle switches modes... '
"$repo_dir/scripts/dwm-shell-mode" toggle >/dev/null
if [ "$("$repo_dir/scripts/dwm-shell-mode" current)" = "suckless" ]; then
	"$repo_dir/scripts/dwm-shell-mode" toggle >/dev/null
	if [ "$("$repo_dir/scripts/dwm-shell-mode" current)" = "quickshell" ]; then
		printf 'PASS\n'
	else
		printf 'FAIL: second toggle did not switch to quickshell\n' >&2
		exit 1
	fi
else
	printf 'FAIL: first toggle did not switch to suckless\n' >&2
	exit 1
fi

# Test 8: dwm-shell-mode status produces valid diagnostic output
printf 'Test 8: dwm-shell-mode status reports components... '
status_out=$("$repo_dir/scripts/dwm-shell-mode" status)
if printf '%s\n' "$status_out" | grep -q 'Configured Mode' && printf '%s\n' "$status_out" | grep -q 'Quickshell Proc'; then
	printf 'PASS\n'
else
	printf 'FAIL: status output missing expected fields: %s\n' "$status_out" >&2
	exit 1
fi

printf 'ALL DWM-SHELL-MODE TESTS PASSED\n'
