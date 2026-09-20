#!/bin/sh
set -eu

repo_dir=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)

printf '==> Running Suckless Bar Fallback Invariant Tests\n'

# Test 1: Struct Monitor must include nativebarwin and isaltbar
printf 'Test 1: Struct Monitor tracks nativebarwin and isaltbar... '
if grep -q "Window nativebarwin;" "$repo_dir/dwm.c" && grep -q "int isaltbar;" "$repo_dir/dwm.c"; then
	printf 'PASS\n'
else
	printf 'FAIL: struct Monitor is missing nativebarwin or isaltbar\n' >&2
	exit 1
fi

# Test 2: drawbar() must NOT contain unconditional return
printf 'Test 2: drawbar() is un-stubbed (no unconditional return)... '
if grep -A 4 "drawbar(Monitor \*m)" "$repo_dir/dwm.c" | grep -qv "return;"; then
	# Verify that drawbar actually contains active rendering code
	if grep -A 20 "drawbar(Monitor \*m)" "$repo_dir/dwm.c" | grep -q "drw_setscheme"; then
		printf 'PASS\n'
	else
		printf 'FAIL: drawbar() does not execute rendering code\n' >&2
		exit 1
	fi
else
	printf 'FAIL: drawbar() has unconditional return\n' >&2
	exit 1
fi

# Test 3: setup() calculates native default_bh from font height, not hardcoded 0
printf 'Test 3: setup() calculates native default_bh... '
if grep -q "default_bh = drw->fonts->h" "$repo_dir/dwm.c" && ! grep -q "bh = 0; /\* Quickshell" "$repo_dir/dwm.c"; then
	printf 'PASS\n'
else
	printf 'FAIL: setup() still hardcodes bh = 0\n' >&2
	exit 1
fi

# Test 4: updatebars() creates native bar window when !m->isaltbar
printf 'Test 4: updatebars() creates native bar window... '
if grep -A 35 "updatebars(void)" "$repo_dir/dwm.c" | grep -q "nativebarwin"; then
	printf 'PASS\n'
else
	printf 'FAIL: updatebars() does not create nativebarwin\n' >&2
	exit 1
fi

# Test 5: updatealtbar() unmaps nativebarwin when altbar (Quickshell) docks
printf 'Test 5: updatealtbar() unmaps native bar on altbar docking... '
if grep -A 25 "updatealtbar(Monitor \*m," "$repo_dir/dwm.c" | grep -q "XUnmapWindow(dpy, m->nativebarwin)"; then
	printf 'PASS\n'
else
	printf 'FAIL: updatealtbar() does not unmap nativebarwin\n' >&2
	exit 1
fi

# Test 6: unmanagealtbar() restores native bar when altbar (Quickshell) exits
printf 'Test 6: unmanagealtbar() restores native bar on altbar exit... '
if grep -A 25 "unmanagealtbar(Window w)" "$repo_dir/dwm.c" | grep -q "m->isaltbar = 0"; then
	printf 'PASS\n'
else
	printf 'FAIL: unmanagealtbar() does not restore native bar state\n' >&2
	exit 1
fi

# Test 7: config.def.h includes JetBrainsMono Nerd Font
printf 'Test 7: config.def.h includes JetBrainsMono Nerd Font in fonts[]... '
if grep -A 2 "static const char \*fonts\[\]" "$repo_dir/config.def.h" | grep -q "JetBrainsMono Nerd Font"; then
	printf 'PASS\n'
else
	printf 'FAIL: config.def.h does not include JetBrainsMono Nerd Font\n' >&2
	exit 1
fi

printf '\nALL SUCKLESS BAR FALLBACK INVARIANT TESTS PASSED\n'
