#!/usr/bin/env bash
# ==============================================================================
# test-consolidated-install.sh — Verify Consolidated DWM & Dmenu Installation
# ==============================================================================
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "==> Testing install.sh dry-run with default consolidated dmenu..."
out_default="$("$REPO_DIR/install.sh" --dry-run)"
echo "$out_default" | grep -Fq "dmenu-oomaya ecosystem: " || {
	echo "FAIL: Expected dmenu-oomaya in default install summary" >&2
	exit 1
}

echo "==> Testing install.sh --skip-dmenu flag..."
out_skip="$("$REPO_DIR/install.sh" --dry-run --skip-dmenu)"
echo "$out_skip" | grep -Fq "dmenu-oomaya ecosystem: skipped (--skip-dmenu)" || {
	echo "FAIL: Expected skipped dmenu in summary" >&2
	exit 1
}

echo "==> Testing install.sh --dmenu-dir flag..."
out_custom_dir="$("$REPO_DIR/install.sh" --dry-run --dmenu-dir="/tmp/custom-dmenu")"
echo "$out_custom_dir" | grep -Fq "dmenu-oomaya ecosystem: local source (/tmp/custom-dmenu)" || {
	echo "FAIL: Expected custom dmenu dir in summary" >&2
	exit 1
}

echo "==> Testing install.sh --git-protocol flag..."
out_ssh="$("$REPO_DIR/install.sh" --dry-run --git-protocol=ssh)"
echo "$out_ssh" | grep -Fq "transport: ssh" || {
	echo "FAIL: Expected ssh transport in summary" >&2
	exit 1
}

echo "==> Testing bootstrap.sh dry-run handoff..."
out_bootstrap="$("$REPO_DIR/bootstrap.sh" --dry-run)"
echo "$out_bootstrap" | grep -Fq "Transferring control to consolidated install.sh..." || {
	echo "FAIL: Expected bootstrap handoff output" >&2
	exit 1
}
echo "$out_bootstrap" | grep -Fq "Dry run complete; no changes were made." || {
	echo "FAIL: Expected install.sh completion under bootstrap.sh" >&2
	exit 1
}

echo "==> Consolidated installation test suite passed! ✓"
