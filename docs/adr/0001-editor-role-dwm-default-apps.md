# ADR-001: Add `editor` Role to `dwm-default-apps`

- **Status:** Accepted
- **Date:** 2026-09-29
- **Deciders:** Ted (오빠), DD
- **Context:** Settings parity workstream — "It Just Works: DWM-oomaya"

## Context

`dwm-default-apps` (forked from dwm-titus) supported three roles:
`browser`, `terminal`, `file-manager`. There was no way to set a default
**text editor** through the settings UI.

Ted's product insight: *"초짜는 기본 에디터/터미널을 설정할 줄 모른다"*
(beginners don't know how to configure a default editor). Changing the
default editor from nano to nvim on Fedora 44 took 30+ minutes with AI
assistance — a failure of "It Just Works."

If a feature exists it must be meaningful from beginner to expert, and the
tool should teach: *"아하 이렇게 하는 방법이 있구나"*.

## Decision

Add a fourth role, `editor`, to `dwm-default-apps`, mirroring the existing
`file-manager` (XDG MIME) pattern:

- **Storage:** XDG MIME association for `text/plain` via `xdg-mime`
  (same transactional, lock-guarded `set_xdg_defaults` path as other roles).
- **Candidate matching:** `TextEditor` desktop category OR `text/plain` in
  `MimeType` (see `parsed_desktop_matches_role`).
- **Command mapping:** `editor_command_for_id` / `editor_id_for_command`
  cover nvim, vim, vi, nano, code, codium, gedit, kate, mousepad, neovide.
- **Surfaces:** `set-role`, `reset-role`, `snapshot` (role + candidates +
  recovery), `status` (`text/plain=` line), usage text.

Scope note: this sets the *desktop* default (what opens when a text file is
double-clicked). The `$EDITOR` shell variable is a separate concern and
intentionally out of scope for v1.

## Consequences

- `dwm-settings-hub` "Defaults" menu can now offer Change Default Editor.
- `seed-default-apps.sh` (parity item) should seed a sane editor default on
  first run.
- Deviates from upstream dwm-titus (which has no editor role) — this is an
  intentional oomaya-only improvement, documented here.

## Verification

- `bash -n` clean.
- `./dwm-default-apps snapshot` emits `role  editor  available  vim.desktop`.
- `./dwm-default-apps status` includes `text/plain=vim.desktop`.
- Invalid role / invalid desktop-id paths return proper errors (exit 2 / 1).
- Commit: `524a1a2e` on `oomaya/dwm-oomaya`.
