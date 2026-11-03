#!/bin/sh
set -eu

# oomaya-ctl must not be skipped silently. The name "tools" matches a directory,
# so without .PHONY make all considers it up to date, and every script that
# checks for oomaya-ctl then falls back without saying so.

repo=$(
	unset CDPATH
	cd -- "$(dirname -- "$0")/.." && pwd
)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

tree=$work/tree
mkdir "$tree"
cd "$repo"
cp -- *.c *.h Makefile Makefile.ipc config.mk "$tree/"
cp -R tools src include "$tree/"
cp config.def.h "$tree/config.h"
rm -f "$tree/tools/oomaya-ctl" "$tree"/src/*/*.o
cd "$tree"

make tools >"$work/tools.log" 2>&1 </dev/null || {
	cat "$work/tools.log" >&2
	printf '%s\n' 'make tools failed' >&2
	exit 1
}
[ -x tools/oomaya-ctl ] || {
	printf '%s\n' 'make tools did not build tools/oomaya-ctl' >&2
	exit 1
}
tools/oomaya-ctl --help >"$work/help.log" 2>&1 </dev/null || :
grep -Fq 'audio' "$work/help.log"

make clean >/dev/null 2>&1 </dev/null
[ ! -e tools/oomaya-ctl ] || {
	printf '%s\n' 'make clean left tools/oomaya-ctl behind' >&2
	exit 1
}

# A package or user install without the binary must fail, not skip it.
for source in *.c; do
	: >"${source%.c}.o"
done
: >dwm
chmod +x dwm
touch -d '+1 hour' dwm
for target in install-system install-user; do
	if make "$target" DESTDIR="$work/stage" PREFIX=/usr \
		USER_HOME="$work/home" OWNER=tester >"$work/$target.log" 2>&1 </dev/null; then
		printf '%s succeeded without oomaya-ctl\n' "$target" >&2
		exit 1
	fi
	grep -Fq 'oomaya-ctl is not built' "$work/$target.log" || {
		cat "$work/$target.log" >&2
		printf '%s failed for another reason\n' "$target" >&2
		exit 1
	}
done
