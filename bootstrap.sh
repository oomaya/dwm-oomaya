#!/usr/bin/env bash
# ==============================================================================
# bootstrap.sh — Sovereign Remote Entrypoint for DWM-oomaya & Dmenu
# ==============================================================================
# Single-line curl installer:
#   curl -fsSL https://raw.githubusercontent.com/oomaya/dwm-oomaya/main/bootstrap.sh | bash
#
# Features:
#   • Automatic /dev/tty reattachment (safe interactive prompts in curl | bash)
#   • Minimal prerequisite bootstrapping (git, curl, base toolchain)
#   • Smart Git Transport (SSH auto-probe + isolated HTTPS insteadOf shield)
#   • Seamless dual-stream checkout to canonical ~/.local/src/
#   • Direct execution handoff to consolidated install.sh
# ==============================================================================
set -euo pipefail

# Reattach stdin to /dev/tty if running from a pipe (curl ... | bash)
if [[ ! -t 0 ]] && [[ -r /dev/tty ]]; then
	exec 0</dev/tty
fi

BOLD='\033[1m'
CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

b_info() { printf "${CYAN}[BOOTSTRAP]${NC} %s\n" "$1"; }
b_ok() { printf "${GREEN}[BOOTSTRAP]${NC} %s\n" "$1"; }
b_warn() { printf "${YELLOW}[BOOTSTRAP]${NC} %s\n" "$1"; }
b_err() { printf "${RED}[BOOTSTRAP ERROR]${NC} %s\n" "$1" >&2; }

if [[ $EUID -eq 0 ]]; then
	b_err "Do not run bootstrap.sh as root or with sudo."
	b_err "The installer operates in user-space and escalates via sudo only when required."
	exit 1
fi

SRC_ROOT="${XDG_SRC_HOME:-$HOME/.local/src}"
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" 2>/dev/null && pwd || echo "")"

if [[ -n "$REPO_DIR" && -f "$REPO_DIR/install.sh" && -f "$REPO_DIR/dwm.c" ]]; then
	DWM_DIR="${DWM_DIR:-$REPO_DIR}"
else
	DWM_DIR="${DWM_DIR:-$SRC_ROOT/dwm-oomaya}"
fi

if [[ -d "$DWM_DIR/../dmenu-oomaya" && -f "$DWM_DIR/../dmenu-oomaya/Makefile" ]]; then
	DMENU_DIR="${DMENU_DIR:-$(cd "$DWM_DIR/../dmenu-oomaya" && pwd)}"
else
	DMENU_DIR="${DMENU_DIR:-$SRC_ROOT/dmenu-oomaya}"
fi

GIT_PROTOCOL="${DWM_GIT_PROTOCOL:-auto}"
IS_DRY_RUN=false

# Parse any bootstrap-level flags before handoff
FORWARD_ARGS=()
while (($# > 0)); do
	case "$1" in
	--dry-run)
		IS_DRY_RUN=true
		FORWARD_ARGS+=("$1")
		shift
		;;
	--git-protocol=*)
		GIT_PROTOCOL="${1#*=}"
		FORWARD_ARGS+=("$1")
		shift
		;;
	--git-protocol)
		GIT_PROTOCOL="$2"
		FORWARD_ARGS+=("$1" "$2")
		shift 2
		;;
	*)
		FORWARD_ARGS+=("$1")
		shift
		;;
	esac
done

echo ""
printf "${BOLD}${CYAN}╔═══════════════════════════════════════════════════════╗${NC}\n"
printf "${BOLD}${CYAN}║     DWM-oomaya & Dmenu Sovereign Remote Bootstrapper  ║${NC}\n"
printf "${BOLD}${CYAN}╚═══════════════════════════════════════════════════════╝${NC}\n"
echo ""

# ── 1. Ensure minimal prerequisites ──────────────────────────────
ensure_prerequisites() {
	local missing=()
	for cmd in git curl make gcc; do
		command -v "$cmd" >/dev/null 2>&1 || missing+=("$cmd")
	done

	if ((${#missing[@]} == 0)); then
		return 0
	fi

	b_info "Missing minimal prerequisites: ${missing[*]}"
	if [[ "$IS_DRY_RUN" == true ]]; then
		b_warn "Dry-run mode active; skipping prerequisite installation."
		return 0
	fi
	b_info "Attempting to install required base tools via system package manager..."

	if command -v dnf >/dev/null 2>&1; then
		sudo dnf install -y git curl make gcc
	elif command -v pacman >/dev/null 2>&1; then
		sudo pacman -Sy --needed --noconfirm git curl make gcc
	elif command -v apt-get >/dev/null 2>&1; then
		sudo apt-get update && sudo apt-get install -y git curl make gcc build-essential
	else
		b_err "Unknown package manager. Please install git, curl, make, and gcc manually."
		exit 1
	fi
}

ensure_prerequisites

# ── 2. Smart Git Transport Engine & Isolation Shield ─────────────
dwm_git_probe_ssh() {
	if ! command -v ssh >/dev/null 2>&1; then
		return 1
	fi
	local probe_output
	probe_output=$(ssh -o BatchMode=yes \
		-o ConnectTimeout=3 \
		-o StrictHostKeyChecking=accept-new \
		-T git@github.com 2>&1 || true)

	if printf '%s\n' "$probe_output" | grep -qi "successfully authenticated"; then
		return 0
	fi
	return 1
}

dwm_git_resolve_url() {
	local repo_slug="$1"
	local protocol="${2:-auto}"

	case "$protocol" in
	ssh)
		echo "git@github.com:${repo_slug}.git"
		;;
	https)
		echo "https://github.com/${repo_slug}.git"
		;;
	auto)
		if dwm_git_probe_ssh; then
			echo "git@github.com:${repo_slug}.git"
		else
			echo "https://github.com/${repo_slug}.git"
		fi
		;;
	*)
		echo "https://github.com/${repo_slug}.git"
		;;
	esac
}

dwm_git_safe_clone() {
	env GIT_CONFIG_GLOBAL=/dev/null \
		GIT_CONFIG_SYSTEM=/dev/null \
		GIT_CONFIG_NOSYSTEM=1 \
		git clone "$@"
}

dwm_git_checkout() {
	local repo_slug="$1"
	local target_dir="$2"
	local protocol="${3:-auto}"

	local resolved_url
	resolved_url=$(dwm_git_resolve_url "$repo_slug" "$protocol")

	if [[ -d "$target_dir/.git" ]]; then
		b_ok "$repo_slug already present at $target_dir"
		b_info "Fetching latest remote changes..."
		git -C "$target_dir" pull --ff-only 2>/dev/null || b_warn "Local modifications present; keeping existing checkout."
		return 0
	fi

	mkdir -p "$(dirname "$target_dir")"
	b_info "Cloning $repo_slug into $target_dir using $resolved_url..."

	if [[ "$resolved_url" =~ ^git@ ]]; then
		git clone "$resolved_url" "$target_dir"
	else
		dwm_git_safe_clone "$resolved_url" "$target_dir"
	fi
	b_ok "$repo_slug synchronized to $target_dir"
}

# ── 3. Synchronize canonical repositories ─────────────────────────
mkdir -p "$SRC_ROOT"

b_info "Synchronizing dwm-oomaya and dmenu-oomaya to canonical paths..."
dwm_git_checkout "oomaya/dwm-oomaya" "$DWM_DIR" "$GIT_PROTOCOL"
dwm_git_checkout "oomaya/dmenu-oomaya" "$DMENU_DIR" "$GIT_PROTOCOL"

# ── 4. Transfer control to consolidated install.sh ───────────────
b_info "Transferring control to consolidated install.sh..."
cd "$DWM_DIR"
exec ./install.sh --dmenu-dir="$DMENU_DIR" --git-protocol="$GIT_PROTOCOL" "${FORWARD_ARGS[@]}"
