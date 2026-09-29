# ADR-002: "Choose in One Place, Resolve at Launch Time" — the Defaults Resolution Principle

- **Status:** Accepted
- **Date:** 2026-09-29
- **Deciders:** Ted (오빠), DD
- **Context:** File-manager debugging session on Peppermint OS (dwm-oomaya-core) — "It Just Works: DWM-oomaya"

## Context

Setting the default file manager via `dwm-settings-hub` appeared to do nothing. Investigation revealed the setting *was* applied (`inode/directory=pcmanfm.desktop`), but three separate launch paths ignored it:

1. `Super+E` was bound to `xdg-open ~ || thunar || pcmanfm` — a hardcoded fallback chain. If `xdg-open` stumbled, Thunar opened regardless of the configured default.
2. `Super+D` (`dmenu-desktop`) lists individual `.desktop` entries ("Thunar File Manager", "PCManFM File Manager") — there was no generic "File Manager" entry bound to the default.
3. `dwm-default-apps` could *set* role defaults but had no `open <role>` primitive to *launch* them (browser was the only exception).

Ted's framing: *"Super+D에서 File Manager를 선택하면 그게 default가 열려야 하는게 예상되는 행동이에요."* (Selecting File Manager from the launcher should open the default — that's the expected behavior.)

The deeper lesson: choices baked into launch paths rot. Runtime resolution stays fresh.

## Decision

**"선택은 한 곳에서, 해결은 실행 시점에."** (Choose in one place, resolve at launch time.)

- **Choose** — defaults are declared in exactly one place: `dwm-default-apps` (via `set-role`, surfaced in the settings hub). No other component stores or duplicates the choice.
- **Resolve** — every launch path (keybinding, launcher, script) resolves the *current* default at launch time through `dwm-default-apps open <role>`. Never bake the choice into the launch path.

Corollaries (the three disciplines):

1. **No hardcoded app names in launch paths.** `xdg-open ~ || thunar || pcmanfm` is the anti-pattern; `dwm-default-apps open file-manager` is the pattern.
2. **Verify territory, not the map.** A `.desktop` file existing is the map; the binary being launchable is the territory. The resolver must check the binary (cf. the phantom `pcmanfm.desktop` incident, 2026-09-29).
3. **Failure teaches.** If no default is set (or the default is stale), don't silently open something arbitrary — tell the user where to set it ("It Teaches Too").

Concretely, this ADR blesses:

- A new `dwm-default-apps open <role>` subcommand (the single resolution primitive).
- Generic `.desktop` shims (e.g. `Name=File Manager`, `Exec=dwm-default-apps open file-manager`) so any launcher — dmenu, rofi, app grids — can offer the default without knowing it. Shims carry no app names and must not advertise themselves as role candidates (no `inode/directory` MimeType) to avoid self-scanning loops.
- Keybindings calling the primitive directly (`Super+E` → `dwm-default-apps open file-manager`), replacing hardcoded chains.

## Consequences

- `config.def.h` `Super+E` binding becomes a config change + recompile item (tracked separately).
- `dwm-default-apps` gains launch responsibility, not just bookkeeping — its correctness is now load-bearing for every launch path. Hardening (territory checks, stale-default fallback) is mandatory, not optional.
- Launcher-agnostic by construction: the design assumes nothing about dmenu vs rofi (the `.desktop` spec is the common denominator) — matters for the multi-distro public release goal (ubuntu/fedora/debian/arch).
- Review razor for the pipeline: any packet that puts an app name in a launch path fails review as an ADR-002 violation.

## Verification

- Principle derived from, and validated against, the 2026-09-29 file-manager session: it diagnoses all three broken launch paths above with one test.
- ADR text reviewed and accepted by Ted (오빠), 2026-09-29.
