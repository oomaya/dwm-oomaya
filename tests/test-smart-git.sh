#!/usr/bin/env bash
# ==============================================================================
# test-smart-git.sh — Test Smart Git Transport Engine & insteadOf Immunity Shield
# ==============================================================================
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=scripts/dwm-git-helper.sh
source "$REPO_DIR/scripts/dwm-git-helper.sh"

TEST_TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TEST_TMPDIR"' EXIT

echo "==> Testing dwm_git_resolve_url protocol resolution..."
ssh_url="$(dwm_git_resolve_url "oomaya/dmenu-oomaya" "ssh")"
[[ "$ssh_url" == "git@github.com:oomaya/dmenu-oomaya.git" ]] || {
	echo "FAIL: Expected SSH URL, got $ssh_url" >&2
	exit 1
}

https_url="$(dwm_git_resolve_url "oomaya/dmenu-oomaya" "https")"
[[ "$https_url" == "https://github.com/oomaya/dmenu-oomaya.git" ]] || {
	echo "FAIL: Expected HTTPS URL, got $https_url" >&2
	exit 1
}

auto_url="$(dwm_git_resolve_url "oomaya/dmenu-oomaya" "auto")"
echo "  Resolved auto URL: $auto_url"
[[ -n "$auto_url" ]] || {
	echo "FAIL: Auto URL resolution returned empty" >&2
	exit 1
}

echo "==> Testing dwm_git_safe_clone immunity under hostile insteadOf configuration..."
# Create a local fixture repository to avoid heavy network downloads
FIXTURE_SRC="$TEST_TMPDIR/fixture-src"
mkdir -p "$FIXTURE_SRC"
git -C "$FIXTURE_SRC" init -b main >/dev/null 2>&1
git -C "$FIXTURE_SRC" config user.email "test@example.com"
git -C "$FIXTURE_SRC" config user.name "Test User"
echo "test fixture" >"$FIXTURE_SRC/README.md"
git -C "$FIXTURE_SRC" add README.md
git -C "$FIXTURE_SRC" commit -m "initial commit" >/dev/null 2>&1

FIXTURE_BARE="$TEST_TMPDIR/fixture-bare.git"
git clone --bare "$FIXTURE_SRC" "$FIXTURE_BARE" >/dev/null 2>&1

# Hostile git config with circular insteadOf rules targeting GitHub and custom URLs
HOSTILE_CONFIG_DIR="$TEST_TMPDIR/hostile-git"
mkdir -p "$HOSTILE_CONFIG_DIR"
cat >"$HOSTILE_CONFIG_DIR/config" <<EOF
[url "git@github.com:"]
	insteadOf = https://github.com/
[url "https://github.com/"]
	insteadOf = git@github.com:
[url "hostile-loop:"]
	insteadOf = file://$FIXTURE_BARE
[url "file://$FIXTURE_BARE"]
	insteadOf = hostile-loop:
EOF

export GIT_CONFIG_GLOBAL="$HOSTILE_CONFIG_DIR/config"

echo "  Testing standard git failure under hostile insteadOf configuration..."
standard_git_output=$(git clone "file://$FIXTURE_BARE" "$TEST_TMPDIR/fail-test" 2>&1 || true)
if ! echo "$standard_git_output" | grep -qi "infinite loop in insteadOf"; then
	echo "Note: Output was: $standard_git_output"
else
	echo "  Standard git failed as expected with circular insteadOf substitution."
fi

echo "  Testing dwm_git_safe_clone immunity against circular rules..."
TARGET_CLONE="$TEST_TMPDIR/fixture-safe"
dwm_git_safe_clone "file://$FIXTURE_BARE" "$TARGET_CLONE" >/dev/null 2>&1

[[ -f "$TARGET_CLONE/README.md" ]] || {
	echo "FAIL: Safe clone did not produce expected files" >&2
	exit 1
}
echo "  Safe clone succeeded cleanly without being trapped by hostile insteadOf rules."

echo "==> All Smart Git Transport tests passed successfully! ✓"
