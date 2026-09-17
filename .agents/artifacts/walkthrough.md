# Lean Desktop Architecture & Multi-Distro Hardening Walkthrough

We have completed the architectural overhaul of `dwm-oomaya`'s dependency pipeline, resolving the consecutive package target errors on Arch/CachyOS, implementing full Debian/Ubuntu/Pop!_OS support, adding JetBrains Mono Nerd Font, and establishing Priority 0 display manager safety.

---

## Changes Made

### 1. Architectural Default: Lean Desktop (`recommended`)
- **Files**: [`install.sh`](file:///home/rand/dwm-oomaya/install.sh), [`scripts/dwm-packages.sh`](file:///home/rand/dwm-oomaya/scripts/dwm-packages.sh)
- Changed default `INSTALL_PROFILE` from `full` to `recommended`.
- Decoupled `system-management` (`cups`, `PackageKit`, `system-config-printer`) from the default installation path, moving it strictly into `--profile=full` for bare-metal ISO kickstarts.
- Running `./install.sh` on an existing desktop now completes cleanly in seconds without attempting to install 50+ unwanted packages or background daemons.

---

### 2. Arch Linux Package Hardening & Target Corrections
- **File**: [`scripts/dwm-packages.sh`](file:///home/rand/dwm-oomaya/scripts/dwm-packages.sh)
- **Target Resolution**: Replaced bare `xprop` with `xorg-xprop` in `arch:runtime-required`, resolving the `target not found: xprop` failure.
- **AUR Dependency Purge**: Removed `xkbset` (AUR-only) from official package manager transactions.
- **Lean Desktop Streamlining**: Streamlined `arch:desktop` to core desktop utilities (`picom`, `feh`, `dex`, `inotify-tools`, `jq`, `alsa-utils`, `brightnessctl`, `libnotify`, `playerctl`), offloading redundant audio/power daemons to `arch:desktop-optional`.

---

### 3. Native Debian / Ubuntu / Pop!_OS Capability
- **File**: [`scripts/dwm-packages.sh`](file:///home/rand/dwm-oomaya/scripts/dwm-packages.sh)
- Added complete, verified Debian/Ubuntu package mappings:
  - `debian:build`: `build-essential`, `pkg-config`, `libx11-dev`, `libxft-dev`, `libxinerama-dev`, `libxrender-dev`, `libimlib2-dev`, `libxcb1-dev`, `libxcb-res0-dev`, `libxcb-util-dev`, `libfontconfig1-dev`, `libfreetype-dev`.
  - `debian:x11`: `xorg`, `x11-xserver-utils`, `x11-utils`, `x11-xkb-utils`, `xinput`.
  - `debian:runtime-required`: `dbus-x11`, `curl`, `git`, `procps`, `psmisc`, `unzip`, `util-linux`, `xclip`, `xdotool`, `xdg-utils`.
  - `debian:desktop`: `picom`, `feh`, `dex`, `inotify-tools`, `jq`, `alsa-utils`, `brightnessctl`, `libnotify-bin`, `pulseaudio-utils`, `playerctl`.
  - `debian:fonts`, `debian:theme`, `debian:theme-gtk`, `debian:theme-optional`, `debian:terminal`, `debian:screenshot-optional`, `debian:lightdm`, `debian:system-management`.
  - Supported profiles: `required`, `recommended`, `optional`, `full`.

---

### 4. JetBrains Mono Nerd Font Integration (Imperative Standard)
- **File**: [`install.sh`](file:///home/rand/dwm-oomaya/install.sh)
- Added `install_jetbrains_mono_nerd_font()` alongside `install_meslo_nerd_font()`.
- Uses official Nerd Fonts release v3.4.0 with SHA-256 integrity verification:
  - Archive: `JetBrainsMono.zip`
  - SHA-256: `76f05ff3ace48a464a6ca57977998784ff7bdbb65a6d915d7e401cd3927c493c`
  - Target: `~/.local/share/fonts/JetBrainsMono/`
- Enforces our 16pt typography floor and sharp-corner Tokyo Night terminal rendering without glyph clipping.

---

### 5. Priority 0: Display Manager Preservation
- **File**: [`install.sh`](file:///home/rand/dwm-oomaya/install.sh)
- Enhanced `detect_display_manager()` to actively inspect systemd units and check:
  `lightdm`, `gdm`, `sddm`, `greetd`, `ly`, `lxdm`.
- Under `recommended` (default), the installer **never touches or enables LightDM**, preventing crashes or display manager collisions on systems running CachyOS Niri, Greetd, or SDDM.

---

### 6. Automated Multi-Distro Regression Test Suite
- **File**: [`tests/test-package-maps.sh`](file:///home/rand/dwm-oomaya/tests/test-package-maps.sh)
- Validates all package profiles for `fedora`, `arch`, and `debian`.
- Enforces regression guards:
  - Rejects bare `xprop` on Arch (must be `xorg-xprop`).
  - Rejects AUR-only `xkbset` in standard Arch transactions.
  - Rejects server bloat (`cups`, `packagekit`) in `recommended` profiles for Arch and Debian.

---

## Verification Results

### 1. Multi-Distro Package Map Suite
```
$ bash tests/test-package-maps.sh
PASS: All core profiles for fedora returned valid non-empty package sets
PASS: All core profiles for arch returned valid non-empty package sets
PASS: All core profiles for debian returned valid non-empty package sets
PASS: Arch uses xorg-xprop instead of bare xprop
PASS: Arch x11 profile does not contain AUR-only xkbset
PASS: Arch recommended profile is clean of distro bloat (cups, packagekit)
PASS: Debian build profile contains all required C development packages
PASS: Debian x11 profile contains standard X11 toolchain
PASS: Debian recommended profile is clean of distro bloat (cups, packagekit)
PASS: dwm_packages rejects unknown distributions and profiles

All package map validation tests passed successfully!
```

### 2. Consolidated Installation Dry Run (Fedora Host)
```
$ ./install.sh --dry-run
Installation summary:
  Distribution: Fedora Linux 44 (Forty Four)
  Family: fedora
  Profile: recommended
  Mode: interactive
  Gear Lever: user-scoped Flathub install (it.mijorus.gearlever)
  Optional extras: skipped
  dmenu-oomaya ecosystem: consolidated build & deploy (transport: auto)

[OK] Dry run complete; no changes were made.
```

### 3. Simulated Dry Run (Arch Linux / CachyOS)
```
Profile: recommended
Package manager: sudo pacman -S --needed --noconfirm
Required: ... xorg-xsetroot xorg-xinput xorg-setxkbmap ... xorg-xprop ...
Recommended: picom feh dex inotify-tools jq alsa-utils brightnessctl libnotify playerctl maim dconf arc-gtk-theme noto-fonts
[OK] Dry run complete; no changes were made.
```

### 4. Simulated Dry Run (Debian / Ubuntu / Pop!_OS)
```
Profile: recommended
Package manager: sudo apt-get install -y
Required: build-essential pkg-config libx11-dev ... xorg x11-xserver-utils x11-utils x11-xkb-utils xinput ...
Recommended: picom feh dex inotify-tools jq alsa-utils brightnessctl libnotify-bin pulseaudio-utils playerctl maim dconf-cli arc-theme
[OK] Dry run complete; no changes were made.
```

### 5. ShellCheck & Regression Suite
- `shellcheck -e SC1091 scripts/dwm-packages.sh install.sh`: Clean (exit code 0).
- `tests/test-consolidated-install.sh`: All 5 test cases passed.
- `tests/test-smart-git.sh`: All smart git transport tests passed.
- `tests/test-fedora-packages.sh`: Passed (73 packages verified).
- `tests/test-install-preservation.sh`: Passed.
