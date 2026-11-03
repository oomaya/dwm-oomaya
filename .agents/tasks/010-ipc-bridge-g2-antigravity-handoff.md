# Task Packet #010: IPC Bridge M2 Completion and G2 Hand-off

## Status: OPEN, handed to Antigravity as Tech Lead (2026-11-03)
- **From**: Claude Code (outgoing Tech Lead)
- **To**: Antigravity
- **Human gate**: oomaya
- **Read first**: `.agents/artifacts/ipc_bridge_consumer_adoption_walkthrough.md`, then `AGENTS.md` and `.agents/rules/development-protocol.md`
- **Branch**: `codex/ipc-event-stream`, stacked on #23

---

## 1. Goal

Make `dwm-oomayad` earn its place. If nothing uses it, it should not exist. The order is M2, gate G2, then M3. The next step after this packet depends on a human decision at G2.

## 2. Hard Stops

Do not do these without explicit human approval:

- Merge a PR, release, or deploy.
- Install on the live session. The M1 approval does not cover M2. Never run `install.sh` locally.
- Run destructive git or file operations, or expose secrets.
- Run anything that writes to `:0`. Tests use a private Xvfb or a container, through `scripts/run-tests`.
- Start M3 before G2 is decided.

If a package is missing, ask the human to install it and name the package. Do not stop at reporting it.

## 3. Task 1: Fix the Split Event Bug

`oomaya_audio_watch_read` in `src/daemon/audio_watch.c` matches each `read()` alone, so `" on si"` then `"nk #1"` is missed.

1. **Test first.** In `tests/test-ipc-watch.sh`, before the burst case, add this and confirm it fails:

   ```sh
   printf 'Volume: 0.41\n' >"$work/wpctl.output"
   printf "Event 'change' on si" >>"$work/events"
   sleep 0.2
   printf 'nk #1\n' >>"$work/events"
   wait_for_line '{"event":"audio","volume":41,"muted":false}'
   ```

2. **Fix.** Keep the unfinished end of the last line between reads:
   - Add `#define OOMAYA_AUDIO_WATCH_CARRY 16` and change the signature to `oomaya_audio_watch_read(int fd, char *carry)`.
   - Prepend `carry` to each read, then match.
   - Save only text after the last newline, at most 15 bytes, so a finished event is not counted twice.
   - Add `char audio_carry[OOMAYA_AUDIO_WATCH_CARRY]` to the reactor, and clear it in `audio_source_start`.
   - `oomaya_reactor_impl.h` must then include `oomaya_audio_watch.h`.

## 4. Task 2: Gate and Publish M2

1. Run the gate:

   ```sh
   scripts/run-tests make clean all
   scripts/run-tests make check
   scripts/run-tests make test-ipc
   make check-format
   ```

   Expected: 67/36/95, X11 bridge 16/16, and the audio consumer and ipc watch checks pass. Plain `make -f Makefile.ipc clean` now removes daemon objects; use it after a header change.

2. Run `codex review` once over `codex/status-weather-wait..HEAD`. Give it an evidence file on a short path. In the last run it mangled a long scratch path and never read the file.
3. Open a ready PR against `codex/status-weather-wait`. Record the measurements, the missing power signal, and section 7 below.
4. Verify the remote head and inspect review threads.

## 5. Task 3: Present G2 and Stop

Rerun `.agents/artifacts/ipc_g2_compare.sh`, because Task 1 changes the build. It needs a tree with built binaries. It moves the real sink by 1% and restores it, so confirm the human is fine with that first. It hardcodes `/run/user/1000` and needs `$HOME/tmp`.

```sh
bash .agents/artifacts/ipc_g2_compare.sh "$PWD" on m2-on 2>/dev/null | grep ' median='
bash .agents/artifacts/ipc_g2_compare.sh "$PWD" off pactl-off 2>/dev/null | grep ' median='
```

Then put this decision to the human. The baseline is M2 at about 64 ms, and `pactl` at about 44 ms.

| Option | Change | Cost |
|---|---|---|
| A (recommended) | Bar and volume keys go back to `pactl` directly. Keep the daemon for WM events and SYS requests. | M1 audio wiring is reverted. Needs a real consumer for WM events, likely Quickshell. |
| B | Keep the daemon for audio as built. | Every key and bar update stays about 20 ms slower and uses more processes. |
| C | Retire the daemon. | Script fallbacks make this safe, but `omarchy-ctl` loses its dwm backend. |

**Why A:** the audio path cannot beat `pactl`, because it still depends on `pactl` and adds steps. Linking an audio library would remove the `wpctl` fork, but it adds a dependency to a daemon that has no audio need of its own. The plan said to propose reduction if there is no improvement. There is a small improvement over #23, but not over having no daemon, so state both numbers.

## 6. M3, After G2

Only if the daemon survives. `omarchy-ctl` (`~/dotfiles/omarchy/src/omarchy-ctl.c`) is the fleet API, so do not build a new fleet CLI. Align it with `watch` and audio, add a protocol compatibility test, confirm SYS works with no X11, add `oomaya-ctl health`, and connect it to `~/dotfiles/verify.sh`. Roll out to the canary first, with approval. Wayland value must be measured, not assumed. Its live state is unverified: `omarchy-ctl state` printed nothing, so check the real subcommands.

## 7. Known Gaps

- **Real `toggleview` and `setlayout`.** These need `dwm.c` to accept a tag mask and a layout, and to publish its tagset. Split from M1. Report the scope before touching `dwm.c`.
- **Stale epoll events.** A client freed during a batch can leave an event that lands on a slot reused in the same batch. This is older than M2, but broadcasts free clients more often.
- **`dwm-status` stays on `pactl` after a daemon restart** if its `oomaya-ctl -j audio` probe fails when the source restarts. It returns to the daemon only after a bar restart.
- **A stale bar if the daemon's sink read fails later.** The watch stays up and sends nothing.
- **`dwm-keybind-exec` routes `view`, `killclient`, and `focus` to the daemon.** Both `view` and `kill` worked through `oomaya-ctl` in a container, but the palette's real action strings were not traced.
- **`dwm-weather` fetch keeps failing here,** and each bar update starts a background `curl`. There is no backoff.
- **Older audit items:** inbound `msg_type` not enforced, split headers dropped, 64 half-frame clients can exhaust slots, `_NET_ACTIVE_WINDOW` not type-checked.

- **Unexplained volume change.** The live sink moved 100%, then 60%, then 70% during this work. The measurements restore the value they started from, so they do not explain it. The cause is unverified. Check the volume before and after any measurement.

## 8. Decisions Waiting on the Human

1. Merge order and timing for #19 to #23. They are stacked, so they go in order.
2. G2, above.
3. Live install of M2, if G2 keeps it.

## 9. Working Notes

- Earlier evidence is git-ignored and exists only in this worktree: `.superpowers/sdd/2026-11-01-pr1-payload-contract/`. It holds `progress.md` (append-only ledger, rulings TL-1 to TL-10), both earlier hand-offs, and `m1-evidence/`. The approved plan is `/home/rand/.claude/plans/modular-sauteeing-treasure.md`.
- Test through `scripts/run-tests`, and give scripts their stdin. New behavior gets a test, and a bug fix gets one that fails without the fix.
- Measure the same way the caller runs. Section 5 of the walkthrough shows how this went wrong.
- Confirm a process exists before calling it started.
- `pgrep -x dwm-status` does not match, because the process name is `bash`. Use `pgrep -f '^bash .*/dwm-status$'`.
- If `gh pr edit` fails with a GraphQL error, use `gh api repos/oomaya/dwm-oomaya/pulls/N -X PATCH -F body=@file`.
- Commits and PRs use `codex/<topic>` branches. Publish after the local gates pass. Merging remains a human decision.
