# Walkthrough: IPC Bridge Consumer Adoption (M0 to M2)

**Date**: 2026-11-03 KST
**Author**: Claude Code (Tech Lead)
**Subject**: Connecting `dwm-oomayad` to real consumers, and what the measurements say about it.
**Follow-up**: `.agents/tasks/010-ipc-bridge-g2-antigravity-handoff.md`
**Background**: `.agents/artifacts/ipc_bridge_inspection_and_fleet_opportunities.md`

---

## 1. Summary

The daemon had almost no traffic because `oomaya-ctl` was never built. Three milestones addressed that:

- **M0** published the payload contract, tag semantics, and test isolation work.
- **M1** connected the status bar and volume keys, and it was installed and verified on the live session.
- **M2** added an event stream. It is built and passes the gate, but one review finding is open and it is not published as a PR.

The main result is a measurement. **The daemon path is still about 20 ms slower than the plain `pactl` path.** M2 improved the daemon path by about 7% and removed its idle wakeups, but did not close that gap. This is the input to gate G2, which a human must decide.

## 2. State of the Work

| PR | Branch | Head | Base | State |
|---|---|---|---|---|
| #19 | `codex/payload-contract` | 78d5edc | `codex/daemon-improvement-plan` | Open, not merged |
| #20 | `codex/tag-update-semantics` | e4b3d62 | #19 | Open, not merged |
| #21 | `codex/test-isolation` | d6b8c72 | #20 | Open, not merged |
| #22 | `codex/ipc-first-consumers` | 405246b | #21 | Open, not merged |
| #23 | `codex/status-weather-wait` | b71fa4a | #22 | Open, not merged |
| None | `codex/ipc-event-stream` | This branch | #23 | No PR yet, see section 7 |

## 3. M0: Cleanup

- **#20** makes the daemon treat `DWM_TAG_UPDATE` as a change signal and accept only a `CARDINAL` `_NET_CURRENT_DESKTOP`.
- **#21** stops the test gate from reaching the live session. `test-autostop` ran a real `pkill` that restarted the live daemon, and `test-install-preservation` reloaded user systemd. `scripts/run-tests` now clears session variables and uses a private runtime directory.

One incident is disclosed in #20: a host run of `test_x11_bridge` wrote to the real `:0` before the test required a disposable display.

## 4. M1: First Real Consumers

- **Build:** `tools` matched a directory name, so `make all` skipped `oomaya-ctl`. It is now phony, and installs fail without the binary.
- **Audio:** `IPC_CMD_SYS_GET_AUDIO` returns `[volume:4][muted:4]`. A client built before the change still works; this was run, not assumed. A failed read is an error, not volume 0.
- **Consumers:** `dwm-status` and `dwm-volume` use the daemon and fall back to `pactl` or `wpctl`.
- **Client:** `oomaya-ctl` gives up after 1 second on a stalled daemon.

### What building `oomaya-ctl` woke up

The binary activated dormant `dwm-keybind-exec` paths. Against real dwm in a container, `oomaya-ctl toggleview` and `setlayout` returned rc 0 and did nothing. The daemon cannot do either: EWMH carries one desktop index, and layout has no inbound path.

The palette now keeps `toggleview`, `setlayout`, and `focusmon` on their key bindings, and the daemon refuses the first two with `ERR_OPCODE`. The real fix needs `dwm.c`, and the human split it from M1.

### Live install

These files in `~/.local/bin` were replaced with approval. `install.sh` was not used, because `make install-user` also touches configuration.

| File | Version |
|---|---|
| `dwm-oomayad`, `oomaya-ctl`, `dwm-status`, `dwm-volume`, `dwm-keybind-exec` | 405246b |
| `dwm-weather` | b71fa4a |

Backups are in `~/.local/state/dwm-oomaya/pre-m1-20261103`. `oomaya-ctl` was new, so it has none. The `dwm` binary and all configuration are unchanged. **The M2 build is not installed.**

## 5. The Bar Latency Bug

Live key-to-bar latency measured about 350 ms on every path, against about 25 ms with stubs. The cause was `dwm-weather`, not audio.

`dwm-status` runs `dwm-weather status` inside `$(...)`. While the cache was stale, the background `curl` inherited that pipe, so every bar update waited for it. The fetch keeps failing on this machine, so the cache stayed stale. #23 detaches the fetch.

| Live bar median | Before #23 | After #23 |
|---|---|---|
| Daemon on | 360.4 ms | 56.2 ms |
| Daemon stopped | 346.8 ms | 45.7 ms |

This went wrong before it went right, and the failure is worth keeping: a direct timing with stdout on `/dev/null` showed 6 ms, because that hides the pipe. The test must match how the caller reads the output.

## 6. M2: Event Stream

Three code commits on this branch (2d5ec31, c1fa7d8, 463d43d), on top of #23:

- **Delivery (C4, M10, M11):** completions now name a client slot and generation, not an fd number. Before this, a reused fd received another client's reply in 15 of 300 test cycles. A client that stops reading is dropped, and `EINTR` no longer closes a connection.
- **Events:** `IPC_CMD_SUBSCRIBE`, the `IPC_SIG_*` signals, and `oomaya-ctl watch [audio|tag|layout|focus]...`, which prints JSON lines.
- **Audio source:** one `pactl subscribe` child, alive only while someone subscribes. It restarts after 2 seconds, and bursts collapse into one sink read.
- **Idle:** the reactor sleeps instead of waking every 200 ms.
- **Bar:** `dwm-status` uses `watch` when the daemon can read the sink, and `pactl subscribe` otherwise.
- **Build:** `Makefile.ipc` now has header dependencies. A stale object with the old task size hung the integration test.

### Deviation from the plan

The power signal was not built. `dwm-status` keeps `udevadm`, and no other consumer needs it.

## 7. Open Finding

Codex found a real bug that is **not fixed**. `oomaya_audio_watch_read` matches each read separately, so an event split across two pipe reads is lost and the bar stays stale until the next event. A fix was drafted, and the work stopped before it was applied. Task 1 of the follow-up packet describes it.

The same review could not read its evidence file, because Codex mangled the path. Its conclusions did not use that evidence.

## 8. G2 Measurements

Script: `.agents/artifacts/ipc_g2_compare.sh`. It uses real PipeWire with a private Xvfb and runtime directory, so the live bar and installed daemon are untouched. Two rounds of 15 key presses each, with 10 seconds idle.

| Path | Median, round 1 | Median, round 2 | Daemon idle wakeups in 10 s |
|---|---|---|---|
| #23 daemon | 66.1 ms | 69.8 ms | 50 |
| M2 daemon | 64.4 ms | 62.9 ms | 0 |
| No daemon, `pactl` | 43.8 ms | 44.0 ms | Not applicable |

- Daemon idle CPU was 0 ticks in every run, so removing the wakeups saves little in practice.
- The script's process count is dominated by its own `wpctl` and `grep` calls. It read about 105, 101, and 91 per key, which supports only the ordering.
- Earlier, volume key execs measured 6 with the daemon and 11 without. That counts the key script alone, not the daemon's own `sh` and `wpctl` children.

### Why the daemon path is slower

`pactl` reports the change either way. The daemon path then adds a `sh` and `wpctl` run, a socket hop, and `oomaya-ctl`, before reaching the same bar parser. The bar's own publish work is the same on both.

## 9. Corrections to Earlier Claims

These appeared in chat or in PR text during the work:

- **Bluetooth was wrong.** A 150 ms Bluetooth wait was reported as a cause. This machine has no `/sys/class/bluetooth`, so `bluetooth_text` returns immediately. It was removed from #22.
- **dunst was wrong.** It was reported as stopped, and possibly killed by the old `pkill`. It was not installed. The human installed it, and it now runs.
- **"Fixed" needs the process, not the exit code.** `setsid -f dunst` returned 0 with no `dunst` on the system.

## 10. Validation at the Branch Head

Commit 463d43d, before these documents were added: build 0, `make check` 0, format 0, IPC 67/36/95, X11 bridge 16/16, and the watch and audio consumer checks pass. No M2 code was installed or run on the live session. The G2 numbers above used the real sink through a private Xvfb.
