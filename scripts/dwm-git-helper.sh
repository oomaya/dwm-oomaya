#!/usr/bin/env bash
# ==============================================================================
# dwm-git-helper.sh — Smart Git Transport Engine & Isolation Shield
# ==============================================================================
# Protects against GitHub insteadOf infinite substitution loops, host-key
# prompt deadlocks, and public-key auth failures during automated installations.
# ==============================================================================

# Fast, non-interactive SSH authentication probe against GitHub
dwm_git_probe_ssh() {
	if ! command -v ssh >/dev/null 2>&1; then
		return 1
	fi
	local probe_output
	probe_output=$(ssh -n -o BatchMode=yes \
		-o ConnectTimeout=3 \
		-o StrictHostKeyChecking=accept-new \
		-T git@github.com 2>&1 || true)

	if printf '%s\n' "$probe_output" | grep -qi "successfully authenticated"; then
		return 0
	fi
	return 1
}

# Resolve canonical clone URL for an oomaya repository
# Usage: dwm_git_resolve_url <repo_slug> [protocol: auto|ssh|https]
# Example: dwm_git_resolve_url "oomaya/dmenu-oomaya" "auto"
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

# Safe, isolated HTTPS clone that completely bypasses user and system git configs.
# Guarantees immunity against:
#   1. url.<base>.insteadOf infinite loops
#   2. Forced SSH rewriting on fresh machines lacking SSH keys
#   3. Host key verification interactive prompts in headless/pipe sessions
# Usage: dwm_git_safe_clone [git_clone_options...] <url> <dest>
dwm_git_safe_clone() {
	env GIT_CONFIG_GLOBAL=/dev/null \
		GIT_CONFIG_SYSTEM=/dev/null \
		GIT_CONFIG_NOSYSTEM=1 \
		git clone "$@" < /dev/null
}

# Smart clone for ecosystem repositories (e.g. dwm-oomaya, dmenu-oomaya)
# Supports auto-detection, explicit protocols, shallow depths, and branch tracking.
# Usage: dwm_git_clone <repo_slug> <target_dir> [protocol: auto|ssh|https] [extra_git_clone_args...]
dwm_git_clone() {
	local repo_slug="$1"
	local target_dir="$2"
	local protocol="${3:-auto}"
	shift 3 || true
	local extra_args=("$@")

	local resolved_url
	resolved_url=$(dwm_git_resolve_url "$repo_slug" "$protocol")

	if [[ "$resolved_url" =~ ^git@ ]]; then
		# SSH clone: use user environment (ssh-agent, ~/.ssh/config)
		git clone "${extra_args[@]}" "$resolved_url" "$target_dir" < /dev/null
	else
		# HTTPS clone: use isolation shield to eliminate insteadOf traps
		dwm_git_safe_clone "${extra_args[@]}" "$resolved_url" "$target_dir"
	fi
}
