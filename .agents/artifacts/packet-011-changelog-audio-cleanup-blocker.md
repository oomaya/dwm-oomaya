# BLOCKER — Packet #011: Stale audio entries in `CHANGELOG.md` `[Unreleased]`

- **Status**: RESOLVED (Option A adopted by Tech Lead on 2026-11-03)
- **Resolved**: 2026-11-03 KST (Packet #011 cleanup)
- **Branch**: `codex/ipc-event-stream` (PR #24)
- **Resolution**: Stale unreleased audio daemon entries removed; `oomaya-ctl watch` entry scoped strictly to pure WM domains (`[tag|layout|focus]`).

---

## 1. Problem

Commit `1fce114` removed all audio handling from `dwm-oomayad` and `oomaya-ctl`. `dwm-status` and `dwm-volume` now call `pactl` directly. The `[Unreleased]` section of [`CHANGELOG.md`](file:///home/rand/.local/src/dwm-oomaya-pr1/CHANGELOG.md#L25-L36) still describes the audio features that were removed. If released as is, these notes would describe features that do not exist.

| Lines | Entry (abridged) | Reality after `1fce114` + Packet #011 |
| :--- | :--- | :--- |
| [L25–L28](file:///home/rand/.local/src/dwm-oomaya-pr1/CHANGELOG.md#L25-L28) | `oomaya-ctl -j audio` reports `{"volume":N,"muted":…}`; `IPC_CMD_SYS_GET_AUDIO` keeps volume in first four bytes | `oomaya-ctl audio` removed; `IPC_CMD_SYS_GET_AUDIO` deleted from the header; `0x0203` now gets `ERR_OPCODE` |
| [L29–L30](file:///home/rand/.local/src/dwm-oomaya-pr1/CHANGELOG.md#L29-L30) | `dwm-status` and `dwm-volume` read volume/mute from the daemon, fall back to `pactl`/`wpctl` | They use `pactl` directly; the daemon is never asked |
| [L34–L36](file:///home/rand/.local/src/dwm-oomaya-pr1/CHANGELOG.md#L34-L36) | `oomaya-ctl watch [audio\|tag\|layout\|focus]…`; `dwm-status` uses it for audio | `watch audio` is rejected (asserted in `tests/test-ipc-watch.sh`); only `tag\|layout\|focus` remain |

These entries were never in a tagged release; they appear only in `[Unreleased]` (first tagged section is `[0.7.0]` at L50). No user ever got these features in a release.

## 2. Options

### A. Rewrite to the net change (recommended)
Delete L25–L30, and change L34–L36 to the surface that actually ships:
```markdown
- `oomaya-ctl watch [tag|layout|focus]...` prints daemon WM events as JSON
  lines. With no filter it prints all three.
```
Optionally add one line saying audio stays outside the daemon (`dwm-status`/`dwm-volume` keep using `pactl`).
- **Pro**: Keep a Changelog describes the change from the last release. Since audio-in-daemon was never released, net zero is the honest record.
- **Con**: The PR1 detour is no longer in the changelog. It is still in git history and the ADR retrospective.

### B. Keep and annotate
Leave the entries and add a "Removed" bullet saying daemon audio was withdrawn before release.
- **Pro**: Nothing is deleted.
- **Con**: The release notes would describe a feature as both added and removed in the same release, which confuses users.

### C. Defer
Leave `CHANGELOG.md` alone in PR #24 and fix it in a separate docs PR before tagging.
- **Pro**: Keeps PR #24 to code only.
- **Con**: Easy to forget before release.

## 3. Decision Needed

- [ ] Option A / B / C?
- [ ] If A or B: same commit as Packet #011, or a separate `docs(changelog):` commit on PR #24?

## 4. Current State

Packet #011 adds one new `[Unreleased]` entry (retired opcodes `0x0203`/`0x0204` get `ERR_OPCODE` on the fast path; worker errors carry the error code in `payload_len` with no body). The stale entries above were left untouched until this is decided.
