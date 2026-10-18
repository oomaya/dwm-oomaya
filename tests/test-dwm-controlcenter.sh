#!/bin/sh
set -eu

repo=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
controlcenter=$repo/scripts/dwm-controlcenter

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM

printf '==> Running native dwm-controlcenter tests\n'

# Test 1: Non-interactive introspection --list
printf 'Test 1: dwm-controlcenter --list outputs options... '
list_out=$("$controlcenter" --list)
if printf '%s\n' "$list_out" | grep -Fq "Wi-Fi" && \
   printf '%s\n' "$list_out" | grep -Fq "Bluetooth" && \
   printf '%s\n' "$list_out" | grep -Fq "Volume" && \
   printf '%s\n' "$list_out" | grep -Fq "Full Settings Hub"; then
	printf 'PASS\n'
else
	printf 'FAIL: --list output missing expected control center items\n' >&2
	printf '%s\n' "$list_out" >&2
	exit 1
fi

# Test 2: Non-interactive introspection --dry-run
printf 'Test 2: dwm-controlcenter --dry-run exits 0... '
if "$controlcenter" --dry-run >/dev/null 2>&1; then
	printf 'PASS\n'
else
	printf 'FAIL: --dry-run returned non-zero\n' >&2
	exit 1
fi

# Test 3: Quickshell IPC forwarding when Quickshell is running
printf 'Test 3: Quickshell IPC forwarding when quickshell is running... '
mock_bin=$work/mock-bin
mkdir -p "$mock_bin" "$work/config/quickshell"
touch "$work/config/quickshell/shell.qml"

cat >"$mock_bin/quickshell" <<'EOF'
#!/bin/sh
printf '%s\n' "$@" >"${DWM_CC_TEST_LOG:?}"
EOF
chmod +x "$mock_bin/quickshell"

cat >"$mock_bin/pgrep" <<'EOF'
#!/bin/sh
exit 0
EOF
chmod +x "$mock_bin/pgrep"

PATH="$mock_bin:$PATH" XDG_CONFIG_HOME="$work/config" \
	DWM_CC_TEST_LOG="$work/quickshell.log" "$controlcenter"

if grep -Fqx 'ipc' "$work/quickshell.log" && \
   grep -Fqx 'controlcenter' "$work/quickshell.log" && \
   grep -Fqx 'toggle' "$work/quickshell.log"; then
	printf 'PASS\n'
else
	printf 'FAIL: quickshell IPC invocation mismatch\n' >&2
	cat "$work/quickshell.log" >&2
	exit 1
fi

# Test 4: Native fallback when Quickshell is not running
printf 'Test 4: Native fallback when quickshell is offline... '
cat >"$mock_bin/pgrep" <<'EOF'
#!/bin/sh
exit 1
EOF
chmod +x "$mock_bin/pgrep"

cat >"$mock_bin/dmenu" <<'EOF'
#!/bin/sh
cat >/dev/null
exit 0
EOF
chmod +x "$mock_bin/dmenu"

PATH="$mock_bin:$PATH" XDG_CONFIG_HOME="$work/config" "$controlcenter"
printf 'PASS\n'

# Test 5: Explicit --native flag bypasses Quickshell even if running
printf 'Test 5: Explicit --native flag uses native dmenu... '
cat >"$mock_bin/pgrep" <<'EOF'
#!/bin/sh
exit 0
EOF
chmod +x "$mock_bin/pgrep"

rm -f "$work/quickshell.log"
PATH="$mock_bin:$PATH" XDG_CONFIG_HOME="$work/config" "$controlcenter" --native
if [ -f "$work/quickshell.log" ]; then
	printf 'FAIL: --native invoked quickshell instead of dmenu\n' >&2
	exit 1
fi
printf 'PASS\n'

printf 'ALL DWM-CONTROLCENTER TESTS PASSED\n'
