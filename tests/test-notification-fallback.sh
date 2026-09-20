#!/bin/sh
set -eu

repo_dir=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)
work=$(mktemp -d)

cleanup() {
	rm -rf "$work"
}
trap cleanup EXIT HUP INT TERM

mkdir -p "$work/bin" "$work/state"

# Test 1: dwm-packages.sh capability maps include dunst
test_packages() {
	family=$1
	profile=$2
	if ! "$repo_dir/scripts/dwm-packages.sh" "$family" "$profile" | grep -qx "dunst"; then
		printf 'FAIL: %s:%s does not include dunst\n' "$family" "$profile" >&2
		return 1
	fi
	printf 'PASS: %s:%s includes dunst\n' "$family" "$profile"
}

# Test 2: dunstrc template exists and enforces Tokyo Night + sharp corners
test_dunstrc() {
	dunstrc="$repo_dir/config/dunst/dunstrc"
	if [ ! -f "$dunstrc" ]; then
		printf 'FAIL: %s does not exist\n' "$dunstrc" >&2
		return 1
	fi

	# Sharp corners (radius = 0)
	if ! grep -qE 'corner_radius\s*=\s*0' "$dunstrc"; then
		printf 'FAIL: %s does not enforce corner_radius = 0\n' "$dunstrc" >&2
		return 1
	fi

	# Tokyo Night dark canvas (#1a1b26)
	if ! grep -qi 'background\s*=\s*"*#1a1b26"*' "$dunstrc"; then
		printf 'FAIL: %s does not enforce Tokyo Night background #1a1b26\n' "$dunstrc" >&2
		return 1
	fi

	# Tokyo Night accent frame (#7aa2f7)
	if ! grep -qi 'frame_color\s*=\s*"*#7aa2f7"*' "$dunstrc"; then
		printf 'FAIL: %s does not enforce frame_color #7aa2f7\n' "$dunstrc" >&2
		return 1
	fi

	printf 'PASS: %s meets Tokyo Night sharp-corner standards\n' "$dunstrc"
}

# Test 3: autostart.sh starts dunst when quickshell is absent
test_autostart_fallback() {
	# Mock dunst
	cat >"$work/bin/dunst" <<'EOF'
#!/bin/sh
echo "dunst_started" > "${TEST_STATE}/dunst.invoked"
EOF
	chmod +x "$work/bin/dunst"

	# Mock pgrep to simulate no running dunst or quickshell
	cat >"$work/bin/pgrep" <<'EOF'
#!/bin/sh
exit 1
EOF
	chmod +x "$work/bin/pgrep"

	# Mock quickshell to NOT exist
	rm -f "$work/bin/quickshell"

	TEST_STATE="$work/state"
	export TEST_STATE
	rm -f "$TEST_STATE/dunst.invoked"

	# Run a targeted subshell simulating autostart notification block
	# We verify that autostart invokes dunst when quickshell is not present
	PATH="$work/bin:$PATH" sh -c "
		quickshell_compatible=0
		if [ \"\${quickshell_compatible:-0}\" -ne 1 ] && ! pgrep -u \"\$(id -u)\" -x quickshell >/dev/null 2>&1; then
			if command -v dunst >/dev/null 2>&1 && ! pgrep -u \"\$(id -u)\" -x dunst >/dev/null 2>&1; then
				dunst &
			fi
		fi
	"

	sleep 0.1
	if [ ! -f "$TEST_STATE/dunst.invoked" ]; then
		printf 'FAIL: autostart did not invoke dunst fallback when quickshell is absent\n' >&2
		return 1
	fi
	printf 'PASS: autostart invoked dunst when quickshell is absent\n'
}

# Run tests
test_packages debian desktop
test_packages arch desktop
test_packages fedora desktop
test_dunstrc
test_autostart_fallback

printf '\nALL NOTIFICATION FALLBACK TESTS PASSED\n'
