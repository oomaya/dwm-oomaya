#!/usr/bin/env bash
set -euo pipefail

repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)

rpm_spec="$repo/packaging/rpm/dwm-oomaya.spec"
arch_pkg="$repo/packaging/arch/PKGBUILD"

test -f "$rpm_spec" || { echo "RPM spec missing: $rpm_spec" >&2; exit 1; }
test -f "$arch_pkg" || { echo "PKGBUILD missing: $arch_pkg" >&2; exit 1; }

# 1. Version alignment
config_mk_version=$(grep -E '^VERSION[[:space:]]*=' "$repo/config.mk" | cut -d= -f2 | tr -d '[:space:]')
base_version="${config_mk_version%%-*}"

spec_version=$(grep -E '^Version:[[:space:]]*' "$rpm_spec" | awk '{print $2}')
arch_version=$(grep -E '^pkgver=' "$arch_pkg" | cut -d= -f2)

if [[ "$spec_version" != "$base_version" ]]; then
	echo "RPM spec version ($spec_version) does not match config.mk ($base_version)" >&2
	exit 1
fi

if [[ "$arch_version" != "$base_version" ]]; then
	echo "Arch PKGBUILD version ($arch_version) does not match config.mk ($base_version)" >&2
	exit 1
fi

# 2. Syntax validation
bash -n "$arch_pkg"

# 3. Ensure spec files cover all installed commands
mapfile -t commands < <(sed -n '/^INSTALL_COMMANDS = \\/,/^[A-Z_]/p' "$repo/Makefile" | grep -E '^\s*scripts/' | awk '{print $1}')
for cmd in "${commands[@]}"; do
	cmd_name=$(basename "$cmd")
	if ! grep -Fq "%{_bindir}/$cmd_name" "$rpm_spec"; then
		echo "RPM spec %files missing command: $cmd_name" >&2
		exit 1
	fi
done

# 4. Ensure no quickshell files in pure dwm-oomaya package
if grep -Eq 'quickshell.*\.qml' "$rpm_spec"; then
	echo "RPM spec still contains QML files!" >&2
	exit 1
fi

if grep -Eq 'quickshell.*\.qml' "$arch_pkg"; then
	echo "Arch PKGBUILD still contains QML files!" >&2
	exit 1
fi

echo "Multi-distro packaging validation: PASS"
