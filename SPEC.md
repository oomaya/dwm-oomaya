# dwm-oomaya Project Specification

> Rewritten 2026-09-29. This document replaces the verbatim dwm-titus
> specification that previously occupied this file (it named "dwm-titus",
> declared Fedora-only support, and never mentioned oomaya). Everything below
> describes dwm-oomaya as it actually exists: a multi-distribution X11 desktop
> built from suckless dwm, ChadDWM, and dwm-titus lineage.

## 1. Product Definition

dwm-oomaya is a multi-distribution X11 desktop environment: a small,
maintained fork of suckless dwm with runtime-configurable hotkeys, themes,
and window rules, a managed Quickshell shell layer, and a companion
centered dmenu build (dmenu-oomaya).

Lineage, in order:

1. **suckless dwm** — the C core, event loop, and configuration model.
2. **ChadDWM** — the vanitygaps layout engine (14 layouts), cfacts,
   and ergonomic tiling behavior.
3. **dwm-titus** — TOML runtime configuration with live reload, the
   Quickshell panel/launcher, and installer engineering.
4. **Omarchy** — the design grammar (Tokyo Night palette, brutalist
   restraint, sharp 2px borders, no rounded corners).

dwm-oomaya synthesizes these into one product that installs cleanly on
bare-metal minimal servers and minimal base installs across three
distribution families. It is MIT licensed.

## 2. Goals

- Install a complete, daily-usable X11 desktop from a minimal base on
  Fedora, Arch-family, and Debian-family systems.
- Provide a one-command bootstrap that works the same on all three
  families: `curl -fsSL <bootstrap-url> | bash`.
- Ship a 14-layout vanitygaps tiling engine with per-tag layout state.
- Load hotkeys, themes, and window rules from TOML at runtime and reload
  them live, without recompiling.
- Restart the window manager in place (`Super+Shift+R`) without closing a
  single client window.
- Expose window-manager state to external tools through X11 atoms using
  pure event-driven IPC: zero polling daemons.
- Preserve suckless virtues: a ~150KB binary, a sub-180MB idle system
  (Xorg + dwm + panel + services), and sub-2ms event-to-screen latency.
- Keep user configuration under standard XDG paths and preserve it across
  upgrades.
- Ship dmenu-oomaya as the companion launcher: centered, Tokyo Night,
  fuzzy, sub-15ms.

## 3. Non-Goals

- A Wayland compositor or Wayland-native session.
- Support contracts for distributions outside the three families. Other
  distributions may work; they are not covered by the validation contract.
- Automatic installation of proprietary GPU drivers outside an explicitly
  opted-in flow.
- Turning the shell layer into a general-purpose root shell, service
  editor, firewall editor, or partition manager.
- Pixel-identical behavior across every theme, driver, display manager, or
  third-party desktop utility.
- Supporting distribution releases after they reach end of life.

## 4. Distribution Support Contract

The supported families and their toolchains:

| Family | Examples | Package tool | Build toolchain packages |
| --- | --- | --- | --- |
| Fedora | Fedora 40+ | `dnf` | `gcc`, `make`, `pkgconf-pkg-config` |
| Arch | Arch, CachyOS, Omarchy | `pacman` | `base-devel` |
| Debian | Debian, Ubuntu, Mint | `apt` | `build-essential` |

The contract is defined by three artifacts that must agree:

1. `bootstrap.sh` detects the family and installs the toolchain.
2. `scripts/dwm-packages.sh` maps every capability to per-family package
   names (`fedora:`, `arch:`, `debian:` entries).
3. `install.sh` reads `/etc/os-release`, accepts the three families, and
   rejects anything else **before** making changes, with a clear message.

Derivatives (CachyOS, Mint, Omarchy) ride their family's mapping. The
primary architecture is x86_64.

## 5. Functional Requirements

### 5.1 Window Manager

The installed session must provide:

- The 14 vanitygaps layouts, with per-tag layout state and sizing:

| # | Symbol | Arrange function | Behavior |
| --- | --- | --- | --- |
| 1 | `[]=` | `tile` | Master left, vertical stack right |
| 2 | `"><>"` | `NULL` | Free-floating (no arrange function) |
| 3 | `[M]` | `monocle` | Full-screen single client |
| 4 | `[@]` | `spiral` | Fibonacci spiral |
| 5 | `[\]` | `dwindle` | Fibonacci dwindle |
| 6 | `H[]` | `deck` | Master left, stacked cards right |
| 7 | `TTT` | `bstack` | Bottom stack |
| 8 | `===` | `bstackhoriz` | Bottom stack, horizontal |
| 9 | `HHH` | `grid` | Symmetrical grid |
| 10 | `###` | `nrowgrid` | N-row adaptive grid |
| 11 | `---` | `horizgrid` | Horizontal partition grid |
| 12 | `:::` | `gaplessgrid` | Edge-to-edge, no gaps |
| 13 | `\|M\|` | `centeredmaster` | Focused master, flanked stacks |
| 14 | `>M>` | `centeredfloatingmaster` | Centered floating master |

- Dynamic inner/outer gap geometry per monitor (`gappih`, `gappiv`,
  `gappoh`, `gappov`), subtracted before every `XMoveResizeWindow`.
- EWMH integration: `_NET_ACTIVE_WINDOW` focus requests must be honored
  (the `focusonnetactive` patch) — a request for a window on another tag
  switches to that tag, unhides the client if needed, focuses it, and
  warps the cursor to its center. `_NET_CURRENT_DESKTOP` must stay
  accurate for external bars and inspection tools.
- Xinerama multi-monitor support.
- Window swallowing, per-client size factors (cfacts), stack reordering,
  and real plus fake fullscreen behavior.
- Sharp 2px borders with no rounding; border colors follow the active
  theme (Tokyo Night `#7aa2f7` accent by default).
- Stable handling of applications that omit optional X properties.

### 5.2 Runtime State IPC: `_DWM_CURRENT_LAYOUT`

The window manager must publish its layout symbol on the
`_DWM_CURRENT_LAYOUT` root-window atom (`UTF8_STRING`) on every layout
switch, tag view change, and monitor focus change.

Consumers (the Quickshell panel, `tools/dwm-quickshell-state.c`,
`src/daemon/x11_bridge.c`) must observe it with `xprop -spy` or an
equivalent X11 event subscription. Polling the atom on a timer is not
acceptable: state propagation is event-driven and must cost 0.0% CPU
while idle.

### 5.3 Atomic In-Place Restart

`Super+Shift+R` must trigger `restart()`: `cleanup()` (detach clients,
restore borders, free resources — without killing any client window),
`XCloseDisplay()`, then `execvp("dwm", argv)` to replace the process
image under the same PID. The new process re-manages existing top-level
windows and resumes in under 10ms. Recompiling configuration must never
require terminating the X session.

### 5.4 Runtime Configuration

User configuration loads from:

```text
${XDG_CONFIG_HOME:-$HOME/.config}/dwm-oomaya/
```

The supported runtime files are `hotkeys.toml`, `themes.toml`, and
`window-rules.toml` (seeded from the tracked `config/` directory).

- Changes must reload via the inotify hot-reload path without restarting
  dwm. Invalid TOML must produce an actionable error and retain the last
  valid state.
- Legacy dwm-titus config paths remain supported as a fallback for
  migrating users, but new installations use the dwm-oomaya paths.
- Compile-time defaults remain in `config.def.h`. An existing `config.h`
  is user-owned and must never be overwritten by install or upgrade.

### 5.5 Session Startup

The project must support:

- A display-manager session installed as `dwm.desktop`.
- A `startx` flow whose `.xinitrc` launches dwm in a D-Bus session.
- Startup without Picom, a wallpaper, or a polkit agent. Missing
  optional components must be logged or skipped without terminating dwm.
- Startup helpers that do not create duplicate long-running processes
  when the session is restarted.

### 5.6 Companion: dmenu-oomaya

dmenu-oomaya is a separate repository (`github.com/oomaya/dmenu-oomaya`,
MIT) and the project's designated launcher engine. It is a maintained
fork of Tony Banters' dmenu. It must provide:

- Centering on the active monitor via Xinerama geometry.
- A 2px Tokyo Night border (`#7aa2f7`).
- Configurable line height for large typography and Nerd Font glyphs.
- Fuzzy subsequence matching with exact matches ranked first.
- Tokyo Night color roles configured in `config.def.h`/`config.h`.

The POSIX `dmenu-*` script suite (installed to `~/.local/bin`) must
provide:

- `dmenu-desktop`: an AWK parser over freedesktop `.desktop` files that
  correctly ignores `[Desktop Action]` blocks (a naive parser lets a
  trailing action block's `Exec=` overwrite the main entry), wraps
  `Terminal=true` entries with the detected terminal emulator, prepends
  Nerd Font glyph categories, and completes in under 15ms.
- `dmenu-windows`: a cross-tag window switcher built on `wmctrl -l`
  feeding window IDs back through `wmctrl -ia`, which the
  `focusonnetactive` patch turns into tag-switching focus jumps.
- `dmenu-power` (lock/suspend/restart/reboot/poweroff via
  `loginctl`/`systemctl`), `dmenu-scrot` (`maim` + `xclip` capture),
  `dmenu-clip` (clipboard history), and `dmenu-hub` (master menu).

All scripts are pure POSIX `sh` with `set -eu`, depend on no daemons,
and resolve every path dynamically from XDG variables — never a
hardcoded `/home/username` (the zero-hardcoded-literals law).

### 5.7 Installer

A two-stage flow:

**Stage 1 — `bootstrap.sh`** (the documented `curl | bash` entrypoint):

1. Detect `dnf`, `pacman`, or `apt-get`; refuse anything else before
   making changes.
2. Install only the core toolchain: `git`, `curl`, `make`, `gcc` (or
   `build-essential`). It must not claim to install X11 development
   headers — that is `install.sh`'s job.
3. Clone the repository to `~/.local/src/dwm-oomaya` (or reuse an
   existing checkout) and hand off to `install.sh`.

**Stage 2 — `install.sh`:**

1. Accept `--profile core|recommended|full` (default `recommended`).
2. Resolve every capability through `scripts/dwm-packages.sh`
   (`fedora:`/`arch:`/`debian:` entries), show required and optional
   packages before installing, and install only missing required
   packages unless a broader profile was requested.
3. Create a missing `config.h` from guided questions or documented
   unattended defaults. Never overwrite an existing one.
4. Build dwm with the system compiler and `pkg-config`-discovered X11
   flags.
5. Install the binary, man page, X session file, scripts, and default
   configuration; seed missing user configuration while preserving
   existing files; set ownership to the invoking user.
6. Be idempotent: repeated runs cause no destructive side effects.
7. Print a summary, skipped optional features, and actionable next
   steps — readable in both interactive terminals and logs.

**Live-update rule.** A source-checkout update of a live installation
must use the complete supported install path: build the checked-out
revision, then refresh the binary, helpers, session scripts, managed
Quickshell tree, and user data copy from that same revision. Updating
only the `dwm` executable is not a supported upgrade.

**`ETXTBSY` rule.** Never `cp` over a running executable (`Text file
busy`). Always `install -Dm755 <binary> <destination>`: `unlink()`
before write, so the running process keeps its old inode until it
exits.

### 5.8 Build System

The build must:

- Use a C99-capable compiler and `make`.
- Honor `CC`, `CFLAGS`, `CPPFLAGS`, `LDFLAGS`, `PREFIX`, and `DESTDIR`.
- Discover compiler and linker flags with `pkg-config` (no mandatory
  `/usr/X11R6`, `/usr/lib`, or `/usr/lib64` assumptions).
- Produce a working `dwm` binary from a clean checkout.
- Support staged, unprivileged installation through `DESTDIR` without
  writing to the invoking user's home.

Required native modules (per `config.mk`, via `pkg-config`):

```text
x11 xft xinerama xrender imlib2 x11-xcb xcb xcb-res fontconfig freetype2
```

### 5.9 Quickshell Shell Layer

Inherited from the dwm-titus lineage and kept as a first-class
component:

- The managed shell lives in the tracked `config/quickshell/` tree and
  is deployed to the user's Quickshell config location on install.
- One `PanelWindow` per active screen; per-screen state providers are
  shared, not duplicated.
- All state updates are event-driven (atoms, IPC, subscriptions).
  Polling timers are acceptable only for inherently sampled values
  (clock, CPU load) or as documented fallbacks.
- The shell must sit near idle with the launcher closed: no continuous
  filtering or rendering of hidden UI, no overlapping timer-triggered
  helpers, exactly one shell provider per session.
- The launcher opens/closes/toggles through Quickshell IPC so dwm
  keybindings control it without Wayland-only APIs.
- The minimum Quickshell version is enforced by
  `scripts/dwm-quickshell-version-check`; install proceeds only when
  the check passes.
- QML development tooling (`qmllint` with explicit Qt/Quickshell import
  roots, and the Quickshell-aware language server) follows the
  repository's documented editor setup.

### 5.10 Dependency Mapping

The maintained tri-distro capability map (source of truth:
`scripts/dwm-packages.sh`):

| Capability | Fedora (`dnf`) | Arch (`pacman`) | Debian (`apt`) |
| --- | --- | --- | --- |
| Compiler and make | `gcc`, `make`, `pkgconf-pkg-config` | `base-devel` | `build-essential` |
| Xlib development | `libX11-devel` | `libx11` | `libx11-dev` |
| Xft and fonts | `libXft-devel`, `fontconfig-devel`, `freetype-devel` | `libxft`, `fontconfig` | `libxft-dev`, `libfontconfig1-dev` |
| Xinerama | `libXinerama-devel` | `libxinerama` | `libxinerama-dev` |
| Xrender | `libXrender-devel` | `libxrender` | `libxrender-dev` |
| Imlib2 | `imlib2-devel` | `imlib2` | `libimlib2-dev` |
| XCB | `libxcb-devel`, `xcb-util-devel` | `libxcb`, `xcb-util` | `libxcb1-dev`, `libxcb-util-dev` |
| Window tools | `wmctrl`, `maim`, `xclip` | `wmctrl`, `maim`, `xclip` | `wmctrl`, `maim`, `xclip` |

These are capability mappings, not immutable lists: availability must
be validated against the targeted release of each family.

Runtime dependencies are classified as core (X11 session, D-Bus, one
usable terminal, tools required by configured core keybindings),
recommended desktop (Quickshell, compositor, wallpaper, notifications,
polkit agent, audio controls, screenshot tooling, Nerd/emoji fonts),
and optional (file manager, network tray, theme utilities, greeter
customization, wallpapers, hardware-specific helpers).

## 6. Filesystem and Installation Contract

Default system locations:

```text
${PREFIX}/bin/dwm
${PREFIX}/share/man/man1/dwm.1
/usr/share/xsessions/dwm.desktop
```

Default user locations:

```text
~/.local/src/dwm-oomaya/        # window-manager source (this repository)
~/.local/src/dmenu-oomaya/      # launcher source (companion repository)
~/.local/bin/                   # dwm, dmenu binaries and dmenu-* scripts ($PATH)
~/.config/dwm-oomaya/           # runtime TOML (hotkeys, themes, window rules)
~/.local/share/dwm-oomaya/      # user data copy
```

System paths must be overridable for packaging and staged installs.
User data must not be written during a package build using `DESTDIR`.
User-scoped installation runs as the target user; project-managed
directories must be owned by that user, and installation must not
recursively change ownership outside them. Uninstall must remove only
files owned by this project and must preserve user configuration by
default.

## 7. User Experience Requirements

- The default session must remain usable when optional visual
  components fail.
- Visual identity: sharp 2px borders, no rounded corners; Tokyo Night
  palette; a 16pt typography floor so glyphs and Nerd Font icons never
  clip.
- Keybindings must be discoverable from the TOML config and the README's
  keybinding table. `Super+Shift+R` (in-place restart) is a headline
  binding, not a hidden trick.
- Error messages must identify the missing command, library, package
  capability, or file, and state the next action.
- Defaults must work at 1080p and remain usable at lower and higher
  resolutions.
- Multi-monitor setup must expose EWMH tags correctly to the Quickshell
  panel and to EWMH inspection tools.

## 8. Security and Safety Requirements

- The only remote code executed during installation is the project's own
  documented bootstrap entrypoint. No third-party remote scripts, no
  unverified binaries; pinned external assets must match
  repository-pinned checksums before installation.
- Do not run user configuration or desktop helpers as root.
- Quote paths and arguments that may contain whitespace.
- Prevent command injection through distribution metadata,
  configuration values, filenames, and environment variables.
- Avoid broad recursive ownership or permission changes outside
  project-owned directories.
- Preserve existing user files, or create explicit backups before
  replacement.
- Keep the shell/Settings frontend unprivileged; require explicit
  confirmation for privileged or destructive actions; prefer polkit or
  an equivalently narrow mechanism over broad passwordless sudo.
- Continue showing readable state when authorization is denied,
  cancelled, or unavailable.

## 9. Validation and Acceptance Criteria

A release is ready only when all of the following pass.

### 9.1 Static Validation

- Clean C build with warnings enabled.
- `shellcheck` and `shfmt` clean on every changed shell script, with
  documented justified exceptions.
- No generated build artifacts committed.
- `make test-ipc` and the `tests/` shell suite pass; the C unit tests
  (rate limiter, ring buffer) pass — currently 57/57.

### 9.2 Per-Distribution Validation

On each supported family:

- `/etc/os-release` detection accepts the family and rejects
  unsupported systems before any mutation.
- The `dwm-packages.sh` capability map resolves to installable
  packages.
- A clean checkout builds successfully.
- `make install DESTDIR=<staging-dir>` installs the expected system
  files without writing to the test user's home.
- The real installer preserves pre-existing user configuration.
- `config.mk`'s `check-build-deps` target reports missing `pkg-config`
  modules accurately (it is the diagnostic of record when a minimal
  environment lacks freetype headers or similar).

### 9.3 Runtime Validation

In a real or nested X11 session:

- dwm starts and can launch a terminal.
- Tiling, floating, tags, focus, and close-window actions work across
  all 14 layouts.
- Runtime TOML configuration loads and hot-reloads.
- `Super+Shift+R` restarts dwm in place: same PID family behavior, all
  client windows survive.
- `_DWM_CURRENT_LAYOUT` is observable changing via `xprop -spy` on
  layout/tag/monitor switches, with no polling process running.
- A `_NET_ACTIVE_WINDOW` request for a window on another tag switches
  to that tag and focuses the window.
- A display-manager session and a `startx` path both launch.
- Missing optional desktop processes do not terminate the session.
- Each active monitor has one managed Quickshell panel; with the
  launcher closed the shell process stays near idle in a short CPU
  sample.

## 10. Current Gaps

Honest inventory as of the 2026-09-29 rewrite:

- `docs/` still carries dwm-titus-era artifacts: the Astro documentation
  site and the P3–P7 evidence/qualification documents. They describe
  titus qualification on Fedora, not oomaya — treat them as historical
  until re-qualified against this specification.
- Titus naming residue remains in the tree: `scripts/dwm-titus-release`,
  the `dwm-titus-tests.*` test workspace, the ISO builder's "dwm-titus
  Kickstart" branding, and `dwm-fedora.ks` / `dwm-fedora-nvidia.ks`
  kickstarts that have not been re-validated under oomaya. The
  Quickshell version check still references a Fedora 44 snapshot
  string.
- The Quickshell Settings platform phases from the old specification
  (system health dashboard, desktop settings) exist in the tree as
  inherited code but have not been re-specified against the
  multi-distribution contract in section 4.
- No oomaya release image has been built or validated; the Fedora image
  contract from the old specification does not transfer automatically.
- The dwm binary cannot compile in a minimal container/VM that lacks
  freetype headers — an environment limitation, correctly diagnosed by
  `check-build-deps`, not a source defect.

## 11. Definition of Done

A feature or roadmap item is complete when:

- Its behavior meets the section 5 requirement, or the change is
  explicitly scoped as preparatory work.
- It is implemented and runtime-validated on at least one contracted
  distribution family, or the limitation is stated precisely.
- Installation attempts on unsupported distributions fail clearly
  before making changes.
- Relevant automated and manual validation is recorded.
- User-facing installation and troubleshooting documentation is
  updated.
- No existing user configuration is overwritten.
- Known limitations and untested platforms are stated precisely.
