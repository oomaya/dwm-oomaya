#!/usr/bin/env bash
set -euo pipefail

repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
pkg_script="$repo/scripts/dwm-packages.sh"

# shellcheck source=scripts/dwm-packages.sh
source "$pkg_script"

fail() {
	printf 'FAIL: %s\n' "$1" >&2
	exit 1
}

pass() {
	printf 'PASS: %s\n' "$1"
}

# 1. Test supported families and core profiles
for family in fedora arch debian; do
	for profile in build x11 runtime-required required desktop recommended full; do
		packages=$(dwm_packages "$family" "$profile") || fail "dwm_packages $family $profile failed with exit code $?"
		[[ -n $packages ]] || fail "dwm_packages $family $profile returned empty output"
	done
	pass "All core profiles for $family returned valid non-empty package sets"
done

# 2. Arch-specific regression guards
arch_runtime=$(dwm_packages arch runtime-required)
if printf '%s\n' "$arch_runtime" | grep -Fqx "xprop"; then
	fail "Found bare 'xprop' in arch:runtime-required (must be 'xorg-xprop')"
fi
printf '%s\n' "$arch_runtime" | grep -Fqx "xorg-xprop" || fail "Missing 'xorg-xprop' in arch:runtime-required"
pass "Arch uses xorg-xprop instead of bare xprop"

arch_x11=$(dwm_packages arch x11)
if printf '%s\n' "$arch_x11" | grep -Fqx "xkbset"; then
	fail "Found AUR-only 'xkbset' in arch:x11"
fi
pass "Arch x11 profile does not contain AUR-only xkbset"

arch_recommended=$(dwm_packages arch recommended)
if printf '%s\n' "$arch_recommended" | grep -Eqx "(cups|packagekit|system-config-printer)"; then
	fail "Found server/distro bloat (cups/packagekit) in arch:recommended"
fi
pass "Arch recommended profile is clean of distro bloat (cups, packagekit)"

# 3. Debian-specific regression guards
debian_build=$(dwm_packages debian build)
for pkg in build-essential pkg-config libx11-dev libxft-dev libxinerama-dev libimlib2-dev libxcb1-dev libfontconfig1-dev libfreetype-dev; do
	printf '%s\n' "$debian_build" | grep -Fqx "$pkg" || fail "Missing '$pkg' in debian:build"
done
pass "Debian build profile contains all required C development packages"

debian_x11=$(dwm_packages debian x11)
for pkg in xorg x11-xserver-utils x11-utils x11-xkb-utils xinput; do
	printf '%s\n' "$debian_x11" | grep -Fqx "$pkg" || fail "Missing '$pkg' in debian:x11"
done
pass "Debian x11 profile contains standard X11 toolchain"

debian_recommended=$(dwm_packages debian recommended)
if printf '%s\n' "$debian_recommended" | grep -Eqx "(cups|packagekit|system-config-printer)"; then
	fail "Found server/distro bloat (cups/packagekit) in debian:recommended"
fi
pass "Debian recommended profile is clean of distro bloat (cups, packagekit)"

# 4. Error handling for unknown profiles
if dwm_packages unknown_distro required >/dev/null 2>&1; then
	fail "dwm_packages unexpectedly succeeded for unknown_distro"
fi
if dwm_packages fedora nonexistent_profile >/dev/null 2>&1; then
	fail "dwm_packages unexpectedly succeeded for nonexistent_profile"
fi
pass "dwm_packages rejects unknown distributions and profiles"

echo ""
echo "All package map validation tests passed successfully!"
