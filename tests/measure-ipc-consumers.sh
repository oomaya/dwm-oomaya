#!/usr/bin/env bash
set -euo pipefail

# Compare the volume path with and without dwm-oomayad. Audio tools are stubs
# and the X server is a private Xvfb, so nothing reaches the live session or
# the real sink. The numbers compare process cost and event handling. They do
# not include PipeWire's own latency.
#
# Historical: dwm-volume and dwm-status now call pactl directly and the daemon
# carries no audio, so legacy and daemon modes run the same volume path. Kept
# to record the PR #24 baseline, not to compare the two.
#
# Usage: tests/measure-ipc-consumers.sh legacy|daemon [trials] [idle-seconds]
# Run through scripts/run-tests. It needs a built dwm-oomayad and tools/oomaya-ctl.

mode=${1:-}
trials=${2:-20}
idle_seconds=${3:-10}
case $mode in
legacy | daemon) ;;
*)
	printf 'usage: %s legacy|daemon [trials] [idle-seconds]\n' "$0" >&2
	exit 2
	;;
esac

repo=$(
	unset CDPATH
	cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd
)
for tool in Xvfb xprop xsetroot strace; do
	if ! command -v "$tool" >/dev/null 2>&1; then
		printf 'measure: %s is required\n' "$tool" >&2
		exit 77
	fi
done
if [[ $mode == daemon && (! -x $repo/dwm-oomayad || ! -x $repo/tools/oomaya-ctl) ]]; then
	printf 'measure: build dwm-oomayad and tools/oomaya-ctl first\n' >&2
	exit 2
fi

work=$(mktemp -d)
pids=()
cleanup() {
	local pid
	for pid in "${pids[@]}"; do
		kill -TERM "$pid" 2>/dev/null || true
	done
	for pid in "${pids[@]}"; do
		wait "$pid" 2>/dev/null || true
	done
	rm -rf "$work"
}
trap cleanup EXIT

mkdir -p "$work/bin" "$work/runtime" "$work/power"
chmod 700 "$work/runtime"
printf '50 0\n' >"$work/audio.state"
: >"$work/audio.events"
: >"$work/audio.calls"

# One stub for both audio CLIs. It uses shell builtins only, so a call costs one
# exec, the same as the real tool.
cat >"$work/bin/audio-stub" <<'SH'
#!/bin/sh
name=${0##*/}
printf '%s %s\n' "$name" "$*" >>"$MEASURE_DIR/audio.calls"
read -r vol muted <"$MEASURE_DIR/audio.state"
store() {
	printf '%s %s\n' "$1" "$2" >"$MEASURE_DIR/audio.state"
	printf "Event 'change' on sink #1\n" >>"$MEASURE_DIR/audio.events"
}
case $name:$1 in
pactl:subscribe) exec tail -n 0 -f "$MEASURE_DIR/audio.events" ;;
pactl:get-sink-mute)
	if [ "$muted" = 1 ]; then echo 'Mute: yes'; else echo 'Mute: no'; fi
	;;
pactl:get-sink-volume) echo "Volume: front-left: 0 / $vol% / 0 dB" ;;
pactl:set-sink-volume)
	case $3 in
	+*) step=${3#+} ;;
	-*) step=-${3#-} ;;
	esac
	store "$((vol + ${step%\%}))" "$muted"
	;;
wpctl:get-volume)
	frac=$vol
	[ "$vol" -ge 10 ] || frac=0$vol
	if [ "$muted" = 1 ]; then
		echo "Volume: $((vol / 100)).${frac#"${frac%??}"} [MUTED]"
	else
		echo "Volume: $((vol / 100)).${frac#"${frac%??}"}"
	fi
	;;
wpctl:set-volume)
	target=${3%\%}
	store "$target" "$muted"
	;;
esac
SH
chmod +x "$work/bin/audio-stub"
ln -s audio-stub "$work/bin/pactl"
ln -s audio-stub "$work/bin/wpctl"
printf '#!/bin/sh\nexit 0\n' >"$work/bin/notify-send"
printf '#!/bin/sh\nexec sleep 3600\n' >"$work/bin/udevadm"
chmod +x "$work/bin/notify-send" "$work/bin/udevadm"
if [[ $mode == daemon ]]; then
	ln -s "$repo/tools/oomaya-ctl" "$work/bin/oomaya-ctl"
fi

export PATH="$work/bin:/usr/bin:/bin"
export MEASURE_DIR=$work
export XDG_RUNTIME_DIR=$work/runtime
export DWM_STATUS_POWER_SUPPLY_DIR=$work/power

Xvfb -displayfd 9 -screen 0 640x480x24 -nolisten tcp 9>"$work/display" >/dev/null 2>&1 &
pids+=("$!")
for _ in {1..500}; do
	[[ -s $work/display ]] && break
	sleep 0.01
done
display_number=$(<"$work/display")
export DISPLAY=":$display_number"

daemon_pid=
if [[ $mode == daemon ]]; then
	"$repo/dwm-oomayad" >"$work/daemon.log" 2>&1 &
	daemon_pid=$!
	pids+=("$daemon_pid")
	for _ in {1..100}; do
		[[ -S $work/runtime/dwm-oomaya/ipc.sock ]] && break
		sleep 0.02
	done
	[[ -S $work/runtime/dwm-oomaya/ipc.sock ]]
fi

"$repo/scripts/dwm-status" &
status_pid=$!
pids+=("$status_pid")

: >"$work/spy.log"
(
	exec xprop -root -spy WM_NAME
) | while IFS= read -r line; do
	printf '%s %s\n' "$EPOCHREALTIME" "$line" >>"$work/spy.log"
done &
pids+=("$!")

wait_for_volume() {
	local expected=$1 attempts=${2:-300}
	while ((attempts-- > 0)); do
		if grep -Fq "VOL $expected%" "$work/spy.log"; then
			return 0
		fi
		sleep 0.01
	done
	return 1
}

# The first publish can land before the spy starts, so nudge until it shows.
ready=0
for _ in {1..50}; do
	printf "Event 'change' on sink #1\n" >>"$work/audio.events"
	if wait_for_volume 50 10; then
		ready=1
		break
	fi
done
((ready == 1))

# Cost of one key press. Audio calls also count the daemon and the bar refresh
# that the press causes, so let the refresh finish before counting.
: >"$work/audio.calls"
strace -f -qq -e trace=execve,fork,vfork,clone,clone3 -o "$work/strace.log" \
	"$repo/scripts/dwm-volume" up 5% >/dev/null 2>&1
sleep 0.5
execs=$(grep -c 'execve(.* = 0$' "$work/strace.log" || true)
forks=$(grep -cE '(fork|clone3?)\(.* = [1-9]' "$work/strace.log" || true)
audio_calls=$(wc -l <"$work/audio.calls")

# Key press to bar update, measured on the X property.
latencies=()
for ((i = 0; i < trials; i++)); do
	read -r current _ <"$work/audio.state"
	if ((i % 2 == 0)); then
		action=down expected=$((current - 5))
	else
		action=up expected=$((current + 5))
	fi
	: >"$work/spy.log"
	start=$EPOCHREALTIME
	"$repo/scripts/dwm-volume" "$action" 5% >/dev/null 2>&1
	if ! wait_for_volume "$expected"; then
		printf 'measure: bar never showed VOL %s%%\n' "$expected" >&2
		exit 1
	fi
	end=$(grep -F "VOL $expected%" "$work/spy.log" | head -n 1 | cut -d' ' -f1)
	latencies+=("$(awk -v a="$start" -v b="$end" 'BEGIN { printf "%.1f", (b - a) * 1000 }')")
done
sorted=$(printf '%s\n' "${latencies[@]}" | sort -n)
median=$(printf '%s\n' "$sorted" | awk '{ v[NR] = $1 } END { print v[int((NR + 1) / 2)] }')
p95=$(printf '%s\n' "$sorted" | awk '{ v[NR] = $1 } END { i = int(NR * 0.95 + 0.5); print v[i < 1 ? 1 : i] }')

# Idle cost. Fields 14-17 include reaped children.
cpu_ticks() {
	awk '{ line = $0; sub(/^.*\) /, "", line); split(line, f, " "); print f[12] + f[13] + f[14] + f[15] }' "/proc/$1/stat"
}
ctx_switches() {
	awk '/ctxt_switches/ { n += $2 } END { print n + 0 }' "/proc/$1/status"
}
status_ticks=$(cpu_ticks "$status_pid")
status_ctx=$(ctx_switches "$status_pid")
if [[ -n $daemon_pid ]]; then
	daemon_ticks=$(cpu_ticks "$daemon_pid")
	daemon_ctx=$(ctx_switches "$daemon_pid")
fi
sleep "$idle_seconds"
printf 'mode=%s\n' "$mode"
printf 'volume_up_execs=%s\n' "$execs"
printf 'volume_up_forks=%s\n' "$forks"
printf 'volume_up_audio_tool_calls=%s\n' "$audio_calls"
printf 'bar_latency_ms_median=%s\n' "$median"
printf 'bar_latency_ms_p95=%s\n' "$p95"
printf 'bar_latency_trials=%s\n' "$trials"
printf 'idle_seconds=%s\n' "$idle_seconds"
printf 'idle_status_cpu_ticks=%s\n' "$(($(cpu_ticks "$status_pid") - status_ticks))"
printf 'idle_status_ctx_switches=%s\n' "$(($(ctx_switches "$status_pid") - status_ctx))"
if [[ -n $daemon_pid ]]; then
	printf 'idle_daemon_cpu_ticks=%s\n' "$(($(cpu_ticks "$daemon_pid") - daemon_ticks))"
	printf 'idle_daemon_ctx_switches=%s\n' "$(($(ctx_switches "$daemon_pid") - daemon_ctx))"
fi
