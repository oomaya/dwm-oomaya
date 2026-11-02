#!/usr/bin/env bash
set -euo pipefail

# dwm-status reads the weather through a command substitution, so a background
# fetch that keeps stdout open makes the whole bar wait for curl.

repo=$(
	unset CDPATH
	cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd
)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

mkdir "$work/bin" "$work/runtime"
cat >"$work/bin/curl" <<'SH'
#!/bin/sh
sleep 1
exit 7
SH
chmod +x "$work/bin/curl"

export PATH="$work/bin:$PATH" XDG_RUNTIME_DIR=$work/runtime

check() {
	local label=$1 expected=$2 start end output elapsed
	start=$EPOCHREALTIME
	output=$("$repo/scripts/dwm-weather" status)
	end=$EPOCHREALTIME
	elapsed=$(awk -v a="$start" -v b="$end" 'BEGIN { printf "%d", (b - a) * 1000 }')
	if [[ $output != "$expected" ]]; then
		printf 'FAIL: %s: expected %s, got %s\n' "$label" "$expected" "$output" >&2
		exit 1
	fi
	if ((elapsed >= 500)); then
		printf 'FAIL: %s: status waited %s ms for the fetch\n' "$label" "$elapsed" >&2
		exit 1
	fi
}

printf 'cached\n' >"$work/runtime/dwm-status-weather.cache"
touch -d '2 hours ago' "$work/runtime/dwm-status-weather.cache"
check 'stale cache' cached

rm "$work/runtime/dwm-status-weather.cache"
check 'no cache' $'\342\230\201 --'

# The background fetches must finish before the workspace goes away.
sleep 1.2
printf 'dwm-weather checks passed\n'
