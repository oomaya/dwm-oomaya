# Implementation Plan: Lean Desktop Package Architecture & Cross-Distro Support

Execute the user-approved **Option A (Lean Desktop)** architecture across `dwm-oomaya` and `dmenu-oomaya`. Purge distro-installer bloat from default flows, fix Arch package names (`xorg-xprop`, purge AUR-only `xkbset`), introduce comprehensive Debian/Ubuntu/Pop!_OS package maps, install JetBrains Mono Nerd Font alongside Meslo, and secure display manager handling.

---

## User Review Required

> [!IMPORTANT]
> **Default Profile Change:**
> The default install profile (`DWM_INSTALL_PROFILE`) will change from `full` to `recommended`.
> - **`recommended` (Default):** Builds `dwm-oomaya` and `dmenu-oomaya`, installs X11 development/runtime essentials, `picom`, `feh`, Meslo + JetBrains Mono Nerd Fonts, and user-level desktop tools. CUPS, PackageKit, LightDM, Steam, Gamescope, and extra GTK themes are completely bypassed.
> - **`full` (Opt-in only):** Retained for users provisioning a completely bare-metal server from scratch (`./install.sh --profile=full`).

> [!NOTE]
> **Existing Display Manager Preservation:**
> `detect_display_manager` is expanded to recognize `greetd`, `ly`, `lxdm`, and active systemd units. Under `recommended`, `install.sh` will **never** attempt to install or enable `lightdm.service`, keeping CachyOS Niri, Wayland greeters, and existing desktop managers completely safe.

---

## Open Questions

None. The user has reviewed the audit artifact and selected **Option A** with explicit directives for:
1. Full Debian / Ubuntu / Pop!_OS capability.
2. JetBrains Mono Nerd Font integration as an imperative alongside Meslo.
3. Verification across existing DEs and barebone server environments.

---

## Proposed Changes

### Packaging & Distribution Layer

#### [MODIFY] [scripts/dwm-packages.sh](file:///home/rand/dwm-oomaya/scripts/dwm-packages.sh)
- **Arch Corrections:**
  - In `arch:runtime-required`: Replace `xprop` with `xorg-xprop`.
  - In `arch:x11`: Ensure standard official packages only (`xorg-xrandr`, `xorg-xset`, `xorg-xsetroot`, `xorg-xinput`, `xorg-setxkbmap`).
  - In `arch:desktop`: Streamline to lean desktop (`picom`, `feh`, `dex`, `inotify-tools`, `jq`, `alsa-utils`, `brightnessctl`, `libnotify`, `playerctl`). Move `power-profiles-daemon`, `bluez`, `blueman`, `pipewire`, `wireplumber` to optional/full to prevent conflicting with existing desktop audio/power managers.
- **Debian / Ubuntu / Pop!_OS Definitions:**
  - Add `debian:build`: `build-essential pkg-config libx11-dev libxft-dev libxinerama-dev libxrender-dev libimlib2-dev libxcb1-dev libxcb-res0-dev libxcb-util-dev libfontconfig1-dev libfreetype-dev`.
  - Add `debian:x11`: `xorg x11-xserver-utils x11-utils x11-xkb-utils xinput`.
  - Add `debian:runtime-required`: `dbus-x11 curl git procps psmisc unzip util-linux xclip xdotool xdg-utils`.
  - Add `debian:desktop`: `picom feh dex inotify-tools jq alsa-utils brightnessctl libnotify-bin pulseaudio-utils playerctl`.
  - Add `debian:fonts`: `fonts-noto-core fonts-noto-color-emoji`.
  - Add `debian:theme`, `debian:theme-gtk`, `debian:theme-optional`, `debian:terminal`, `debian:terminal-primary`, `debian:screenshot-optional`, `debian:lightdm`, `debian:system-management`, `debian:desktop-optional`.
  - Add `debian:required`, `debian:recommended`, `debian:optional`, `debian:full`.
- **Fedora Adjustments:**
  - Move `system-management` (`cups`, `PackageKit`, `system-config-printer`) strictly out of `recommended` and into `full`.

---

### Installer & Font Subsystem

#### [MODIFY] [install.sh](file:///home/rand/dwm-oomaya/install.sh)
- **Default Profile:** Change default `INSTALL_PROFILE="${DWM_INSTALL_PROFILE:-recommended}"`.
- **System Management Decoupling:** Move `dwm_install_package_profile system-management` from `install_recommended_profile` into `install_optional_profile`.
- **Display Manager Guard:** Enhance `detect_display_manager` to check `greetd`, `ly`, `lxdm`, and active systemd units. Never install `lightdm` unless `--profile=full` is passed AND no display manager is active.
- **JetBrains Mono Nerd Font Support:**
  - Define `JETBRAINS_VERSION="3.4.0"`, `JETBRAINS_URL`, and SHA-256 `76f05ff3ace48a464a6ca57977998784ff7bdbb65a6d915d7e401cd3927c493c`.
  - Add `install_jetbrains_mono_nerd_font()` with checksum verification and font caching.
  - Call both `install_meslo_nerd_font` and `install_jetbrains_mono_nerd_font` during the font installation phase.

---

### Test Suite & Multi-Distro Validation

#### [NEW] [tests/test-package-maps.sh](file:///home/rand/dwm-oomaya/tests/test-package-maps.sh)
- Cross-distro package syntax and capability validator.
- Tests that `dwm_packages` returns valid non-empty package sets for `fedora`, `arch`, and `debian` across `required`, `recommended`, `desktop`, and `build`.
- Verifies that known trap packages (`xprop` without prefix, `xkbset`, missing dev headers) do not appear in Arch or Debian profiles.

#### [MODIFY] [tests/test-consolidated-install.sh](file:///home/rand/dwm-oomaya/tests/test-consolidated-install.sh)
- Update mock test assertions to reflect `recommended` as the default profile and ensure `system-management` is skipped in default test passes.

---

## Verification Plan

### Automated Tests
1. **Package Map Matrix Test:**
   ```bash
   bash tests/test-package-maps.sh
   ```
2. **Consolidated Installer Test:**
   ```bash
   bash tests/test-consolidated-install.sh
   ```
3. **Smart Git Transport & Lockstep Dmenu Build:**
   ```bash
   bash tests/test-smart-git.sh
   ```
4. **ShellCheck Linting:**
   ```bash
   shellcheck scripts/dwm-packages.sh install.sh
   ```

### Manual & Target Verification
1. Verify `install.sh --dry-run` output on local Fedora host: confirm profile resolves to `recommended`, system-management is skipped, and dmenu lockstep build is active.
2. Verify simulated Arch package set with mock pacman: confirm `xorg-xprop` is resolved and `xkbset` is absent.
3. Test font installation verification logic in a temporary clean environment.
