#!/bin/bash
# G2 comparison on real PipeWire. Private Xvfb and runtime dir, so the live bar
# and installed daemon are untouched. Moves the sink 1% and restores it.
# Usage: ipc_g2_compare.sh <tree> <on|off> <label> [trials]
set -u
tree=$1 mode=$2 label=$3 trials=${4:-15}
T=$(mktemp -d "$HOME/tmp/g2.XXXXXX"); mkdir "$T/bin" "$T/rt"; chmod 700 "$T/rt"
export PIPEWIRE_RUNTIME_DIR=/run/user/1000 PULSE_RUNTIME_PATH=/run/user/1000/pulse XDG_RUNTIME_DIR=$T/rt
unset DBUS_SESSION_BUS_ADDRESS DISPLAY XAUTHORITY
printf '#!/bin/sh\nexit 0\n' >"$T/bin/notify-send"; printf '#!/bin/sh\nexec sleep 3600\n' >"$T/bin/udevadm"; chmod +x "$T"/bin/*
printf 'sun +10C\n' >"$T/rt/dwm-status-weather.cache"
[ "$mode" = on ] && ln -s "$tree/tools/oomaya-ctl" "$T/bin/oomaya-ctl"
export PATH="$T/bin:/usr/bin:/bin"
before=$(wpctl get-volume @DEFAULT_AUDIO_SINK@ | awk '{print $2}')
pids=()
Xvfb -displayfd 9 -screen 0 320x200x24 -nolisten tcp 9>"$T/display" >/dev/null 2>&1 & pids+=($!)
for _ in {1..300}; do [ -s "$T/display" ] && break; sleep 0.01; done
export DISPLAY=":$(cat "$T/display")"
dpid=
if [ "$mode" = on ]; then "$tree/dwm-oomayad" >/dev/null 2>&1 & dpid=$!; pids+=($dpid); for _ in {1..200}; do [ -S "$T/rt/dwm-oomaya/ipc.sock" ] && break; sleep 0.01; done; fi
"$tree/scripts/dwm-status" & spid=$!; pids+=($spid)
log=$T/spy; : >"$log"
( exec xprop -root -spy WM_NAME ) | while IFS= read -r l; do printf '%s %s\n' "$EPOCHREALTIME" "$l" >>"$log"; done & pids+=($!)
finish() { wpctl set-volume @DEFAULT_AUDIO_SINK@ "$before"; for p in "${pids[@]}"; do kill "$p" 2>/dev/null; done; sleep 0.2; pkill -f "^xprop -root -spy" -P $$ 2>/dev/null; rm -rf "$T"; }
trap finish EXIT
sleep 1.2
pivot=$(awk -v v="$before" 'BEGIN{print int(v*100+0.5)}')
ms=()
bg0=$(awk '/^processes/{print $2}' /proc/stat); sleep 2; bg1=$(awk '/^processes/{print $2}' /proc/stat); bg=$(( (bg1-bg0) ))
p0=$(awk '/^processes/{print $2}' /proc/stat); t0=$EPOCHREALTIME
for ((i=0;i<trials;i++)); do
	cur=$(wpctl get-volume @DEFAULT_AUDIO_SINK@ | awk '{print int($2*100+0.5)}')
	if ((cur>=pivot)); then act=down exp=$((cur-1)); else act=up exp=$((cur+1)); fi
	: >"$log"; s=$EPOCHREALTIME
	"$tree/scripts/dwm-volume" "$act" 1% >/dev/null 2>&1
	for _ in {1..400}; do grep -q "VOL $exp%" "$log" && break; sleep 0.002; done
	e=$(grep -m1 "VOL $exp%" "$log" | cut -d' ' -f1)
	[ -n "$e" ] && ms+=("$(awk -v a="$s" -v b="$e" 'BEGIN{printf "%.1f",(b-a)*1000}')")
	sleep 0.1
done
p1=$(awk '/^processes/{print $2}' /proc/stat); t1=$EPOCHREALTIME
procs=$(awk -v d=$((p1-p0)) -v n="$trials" -v bg="$bg" -v a="$t0" -v b="$t1" 'BEGIN{printf "%.1f",(d-bg*(b-a)/2)/n - 2}')
st() { awk '{l=$0;sub(/^.*\) /,"",l);split(l,f," ");print f[12]+f[13]+f[14]+f[15]}' /proc/$1/stat; }
cs() { awk '/ctxt_switches/{n+=$2}END{print n+0}' /proc/$1/status; }
sleep 1; a=$(cs $spid); ta=$(st $spid); [ -n "$dpid" ] && { b=$(cs $dpid); tb=$(st $dpid); }
sleep 10
ia=$(($(cs $spid)-a)); ita=$(($(st $spid)-ta)); ib=0 itb=0; [ -n "$dpid" ] && { ib=$(($(cs $dpid)-b)); itb=$(($(st $dpid)-tb)); }
printf '%s\n' "${ms[@]}" | sort -n | awk -v l="$label" -v p="$procs" -v ia=$ia -v ib=$ib -v ita=$ita -v itb=$itb '{v[NR]=$1}END{printf "%-14s n=%d median=%s min=%s max=%s procs_per_key=%s idle10s bar_ctx=%d bar_ticks=%d daemon_ctx=%d daemon_ticks=%d\n",l,NR,v[int((NR+1)/2)],v[1],v[NR],p,ia,ita,ib,itb}'
