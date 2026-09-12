#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────
# dwm-utils.sh — Shared utility library for dwm-oomaya
# Source this file from other scripts:
#   source "$(dirname "$0")/dwm-utils.sh"
# ─────────────────────────────────────────────────────────

# ── Distribution and package manager ────────────────────
DISTRO_ID="unknown"
DISTRO_NAME="Unknown Linux"
DISTRO_FAMILY="unknown"
DWM_PACKAGE_COMMAND=()
OS_RELEASE_FILE="/etc/os-release"
if [[ ${DWM_TEST_MODE:-0} == 1 && -n ${DWM_OS_RELEASE:-} ]]; then
	OS_RELEASE_FILE=$DWM_OS_RELEASE
fi

if [[ -r $OS_RELEASE_FILE ]]; then
	# shellcheck disable=SC1090
	source "$OS_RELEASE_FILE"
	DISTRO_ID="${ID:-unknown}"
	DISTRO_NAME="${PRETTY_NAME:-${NAME:-Unknown Linux}}"
fi

if [[ $DISTRO_ID =~ ^(arch|cachyos|manjaro|endeavouros)$ || ${ID_LIKE:-} =~ arch ]]; then
	DISTRO_FAMILY="arch"
elif [[ $DISTRO_ID =~ ^fedora$ || ${ID_LIKE:-} =~ fedora ]]; then
	DISTRO_FAMILY="fedora"
elif [[ $DISTRO_ID =~ ^(debian|ubuntu|pop|linuxmint)$ || ${ID_LIKE:-} =~ (debian|ubuntu) ]]; then
	DISTRO_FAMILY="debian"
fi

case "$DISTRO_FAMILY" in
fedora)
	if ((EUID == 0)); then
		DWM_PACKAGE_COMMAND=(dnf install -y)
	else
		DWM_PACKAGE_COMMAND=(sudo dnf install -y)
	fi
	PKG_CMD="${DWM_PACKAGE_COMMAND[*]}"
	;;
arch)
	if ((EUID == 0)); then
		DWM_PACKAGE_COMMAND=(pacman -S --needed --noconfirm)
	else
		DWM_PACKAGE_COMMAND=(sudo pacman -S --needed --noconfirm)
	fi
	PKG_CMD="${DWM_PACKAGE_COMMAND[*]}"
	;;
debian)
	if ((EUID == 0)); then
		DWM_PACKAGE_COMMAND=(apt-get install -y)
	else
		DWM_PACKAGE_COMMAND=(sudo apt-get install -y)
	fi
	PKG_CMD="${DWM_PACKAGE_COMMAND[*]}"
	;;
*)
	PKG_CMD="unavailable"
	;;
esac
export PKG_CMD

install_packages() {
	case "$DISTRO_FAMILY" in
	fedora|arch|debian)
		"${DWM_PACKAGE_COMMAND[@]}" "$@"
		;;
	*)
		printf 'Unsupported distribution: %s\n' "$DISTRO_NAME" >&2
		return 1
		;;
	esac
}

package_available() {
	local package_arch

	case "$DISTRO_FAMILY" in
	fedora)
		package_arch=${1##*.}
		case "$package_arch" in
		i686 | x86_64 | aarch64)
			dnf -q repoquery --available --qf '%{name}.%{arch}' "$1" 2>/dev/null |
				command grep -Fxq "$1"
			;;
		*)
			dnf -q repoquery --available --qf '%{name}' "$1" 2>/dev/null |
				command grep -Fxq "$1"
			;;
		esac
		;;
	arch)
		pacman -Si "$1" &>/dev/null
		;;
	debian)
		apt-cache show "$1" &>/dev/null
		;;
	*)
		return 1
		;;
	esac
}

install_optional_package() {
	local package=$1

	if package_available "$package"; then
		install_packages "$package"
		return
	fi

	printf 'Optional package is unavailable in enabled repositories: %s\n' "$package" >&2
	return 1
}

# ── Hardware Detection ──────────────────────────────────

# Detect GPU type: nvidia, amd, intel, or unknown
detect_gpu() {
	if command -v lspci &>/dev/null; then
		local vga
		vga=$(lspci 2>/dev/null | command grep -i 'vga\|3d\|display' || true)
		if echo "$vga" | command grep -qi nvidia; then
			echo "nvidia"
		elif echo "$vga" | command grep -qi 'amd\|radeon'; then
			echo "amd"
		elif echo "$vga" | command grep -qi intel; then
			echo "intel"
		else
			echo "unknown"
		fi
	else
		echo "unknown"
	fi
}

# Detect battery device name (e.g., BAT0, BAT1)
detect_battery() {
	command ls /sys/class/power_supply/ 2>/dev/null | command grep -E '^BAT[0-9]' | head -1
}

# Detect AC adapter name (e.g., ACAD, AC0, ADP1)
detect_adapter() {
	command ls /sys/class/power_supply/ 2>/dev/null | command grep -Ev '^BAT' | head -1
}

# Detect if running on a laptop (has battery)
is_laptop() {
	[ -n "$(detect_battery)" ]
}

# Detect first available terminal emulator
detect_terminal() {
	for t in dwmterm alacritty kitty ghostty foot st warp-terminal xterm; do
		if command -v "$t" &>/dev/null; then
			echo "$t"
			return
		fi
	done
	echo "xterm"
}
