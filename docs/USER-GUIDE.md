# DWM-Oomaya Comprehensive User Guide & Manual

**DWM-Oomaya** is a sovereign, ultra-fast, and keyboard-driven dynamic window manager and desktop ecosystem built on pure Suckless C (C99/C11) foundations.

This guide provides comprehensive end-to-end instructions from initial installation and keybinding mastery to deep customization, maintenance, and seamless updates.

---

## Table of Contents
1. [Architecture & Philosophy](#1-architecture--philosophy)
2. [Supported Distributions & Requirements](#2-supported-distributions--requirements)
3. [Installation Guide](#3-installation-guide)
4. [Session Launch & Starting DWM](#4-session-launch--starting-dwm)
5. [Complete Keybindings Matrix](#5-complete-keybindings-matrix)
6. [Native Control Center & Settings Hub](#6-native-control-center--settings-hub)
7. [Status Bar & Widget Interactions](#7-status-bar--widget-interactions)
8. [Shell Mode Architecture (Suckless vs. Quickshell)](#8-shell-mode-architecture-suckless-vs-quickshell)
9. [Runtime TOML Customization](#9-runtime-toml-customization)
10. [Maintenance & Seamless Updates](#10-maintenance--seamless-updates)
11. [System Diagnostics & Troubleshooting](#11-system-diagnostics--troubleshooting)

---

## 1. Architecture & Philosophy

DWM-Oomaya is designed around the principle of the **Sovereign Desktop**—a high-performance graphical computing environment independent of heavy runtime frameworks:
- **Pure C Core**: The window manager (`dwm`), the asynchronous event and state daemon (`dwm-oomayad`), and the centered dynamic menu (`dmenu-oomaya`) are compiled in pure C with `-Os` optimization, consuming less than 1MB of memory combined.
- **Zero-Dependency by Default**: By default, the top bar (Status2d), application launcher, control center, settings hub, and audio/brightness OSDs operate with zero Qt6, QML, or Python runtime dependencies.
- **Optional Companion Plugin**: Users desiring rich QML-animated desktop overlays can install the independent companion repository ([`quickshell-oomaya`](https://github.com/oomaya/quickshell-oomaya)) on demand without bloating the base system.

---

## 2. Supported Distributions & Requirements

DWM-Oomaya has validated package mapping and native compatibility across all major Linux families:
- **Fedora**: Fedora 41, 42, 43, 44+ (Workstation, Silverblue/Kinoite, Minimal/Server).
- **Arch Linux & CachyOS**: Arch Linux, CachyOS, EndeavourOS, Manjaro.
- **Debian Family**: Debian 12/13 (Bookworm/Trixie), Ubuntu LTS (22.04/24.04/26.04), Linux Mint, Peppermint OS, Pop!_OS.

---

## 3. Installation Guide

### Method A: Consolidated Automated Installer (Recommended)
Automatically provisions required system development headers, utilities, Nerd fonts, themes, and default configs:

```bash
# Clone the repository
git clone https://github.com/oomaya/dwm-oomaya.git ~/.local/src/dwm-oomaya
cd ~/.local/src/dwm-oomaya

# Run recommended installation
./install.sh --desktop
```

### Method B: Native Distribution Packages
Distribution-standard build specifications are available under `packaging/`:

- **Fedora (RPM)**:
  ```bash
  rpmbuild -ba packaging/rpm/dwm-oomaya.spec
  sudo dnf install ~/rpmbuild/RPMS/x86_64/dwm-oomaya-0.8.0-*.rpm
  ```
- **Arch Linux & CachyOS (PKGBUILD)**:
  ```bash
  cd packaging/arch
  makepkg -si
  ```

### Method C: Manual Source Build
```bash
make clean all

# System-wide installation (/usr/local/bin, /usr/share/xsessions)
sudo make install-system

# User-local installation (~/.local/bin)
make install-user
```

---

## 4. Session Launch & Starting DWM

### Display Managers (GDM, LightDM, SDDM)
Upon installation, a session file (`dwm.desktop`) is registered in `/usr/share/xsessions/`. Select **DWM** or **dwm-oomaya** from the login screen menu.

### Console Login (xinit / startx)
Add the following line to the end of `~/.xinitrc`:
```bash
exec dwm-session-launch
```

---

## 5. Complete Keybindings Matrix

> **Default Modifiers**: `Super` = Windows/Command key, `Alt` = Option key

### 1) Core Applications & Launchers
| Action | Keybinding | Command / Description |
| :--- | :--- | :--- |
| **Primary Terminal** | `Super + Shift + Enter` | Ghostty / Kitty / Alacritty |
| **Floating Scratchpad** | `Super + s` | `dwm-scratchpad` (Centred popup terminal) |
| **File Manager** | `Super + e` | Thunar / xdg-open |
| **Dmenu App Launcher** | `Super + d` | `dmenu-desktop` |
| **Direct Command Runner** | `Alt + p` | `dmenu-run` |
| **Window Switcher** | `Alt + Tab` | `dmenu-windows` |
| **Keybindings Palette** | `Super + /` | `dwm-keybinds` (Searchable cheatsheet) |

### 2) Control Center, Settings & Power
| Action | Keybinding | Description |
| :--- | :--- | :--- |
| **Native Control Center** | `Super + F1` or `Super + c` | `dwm-controlcenter` (Wi-Fi, Bluetooth, Audio, Power) |
| **System Settings Hub** | `Super + Shift + s` | `dwm-settings-hub` (Appearance, Displays, Fonts, Input) |
| **Dmenu Tools Hub** | `Super + Shift + h` | `dmenu-hub` |
| **Power Menu** | `Alt + x` or `Super + Ctrl + q` | `dmenu-power` (Lock, Logout, Suspend, Reboot, Shutdown) |
| **Toggle Shell Mode** | `Super + Ctrl + r` | `dwm-shell-mode toggle` (Suckless ↔ Quickshell) |
| **In-Place DWM Restart** | `Super + Shift + r` | Atomic restart in RAM preserving all running windows |

### 3) Window Management & Tiling
| Action | Keybinding | Description |
| :--- | :--- | :--- |
| **Close Focused Window** | `Super + q` | Close active client gracefully |
| **Focus Next / Prev Window** | `Super + j` / `Super + k` | Traverse window stack |
| **Move Window Down / Up** | `Super + Shift + j` / `k` | Swap active window position in stack |
| **Master Area Size** | `Super + h` / `Super + l` | Shrink / expand master pane ratio |
| **Promote to Master** | `Super + Enter` | Zoom focused window into master position |
| **Master Count (+ / -)** | `Super + i` / `Super + Shift + i` | Increase / decrease number of master windows |
| **Toggle Fullscreen** | `Super + f` | Borderless true fullscreen toggle |
| **Toggle Floating** | `Super + Shift + Space` | Switch window between tiling and floating |
| **Cycle Layouts** | `Super + Tab` | Cycle Tile ↔ Monocle ↔ Grid ↔ Spiral |
| **Toggle Topbar** | `Super + b` | Show or hide the top status bar |

### 4) Vanitygaps (Pixel Padding)
| Action | Keybinding | Description |
| :--- | :--- | :--- |
| **Toggle All Gaps** | `Super + Ctrl + t` | Enable or disable window gaps |
| **Adjust Gap Size** | `Super + Ctrl + i` / `d` | Increase (+5px) / decrease (-5px) gaps |
| **Reset Default Gaps** | `Super + Alt + Shift + =` | Restore baseline gap geometry |

### 5) Workspaces (Tags) & Multi-Monitor
| Action | Keybinding | Description |
| :--- | :--- | :--- |
| **Switch Workspace** | `Super + 1 ~ 9` | Focus workspace tag 1 through 9 |
| **Move Window to Tag** | `Super + Shift + 1 ~ 9` | Move focused window to tag 1 through 9 |
| **Cycle Workspaces** | `Super + Left` / `Right` | Shift view to previous / next active tag |
| **Focus Monitor** | `Super + ,` / `Super + .` | Switch focus to previous / next physical monitor |
| **Send Window to Monitor** | `Super + Shift + ,` / `.` | Move focused window to previous / next monitor |

### 6) Media, Brightness & Screenshots
| Action | Keybinding | Description |
| :--- | :--- | :--- |
| **Volume Up / Down** | `XF86AudioRaiseVolume` / `Lower` | Adjust volume by 5% with Dunst OSD notification |
| **Mute Audio** | `XF86AudioMute` | Toggle audio mute |
| **Brightness Up / Down** | `XF86MonBrightnessUp` / `Down` | Adjust screen brightness by 10% |
| **Screenshot Menu** | `Print` or `Super + Print` | `dmenu-scrot` (Fullscreen, Selected area, Active window) |
| **Crop Area to Clipboard** | `Super + u` or `Shift + Print` | Select screen region and copy directly to clipboard |

---

## 6. Native Control Center & Settings Hub

DWM-Oomaya provides a keyboard-centric, distraction-free control environment:

- **Ultra-light Control Center (`dwm-controlcenter`)**:
  - Activated via `Super + F1` or `Super + c`.
  - Check network status, connect to Wi-Fi, pair Bluetooth devices, switch audio devices, and toggle power profiles (Performance, Balanced, Power-saver).
  - All background actions include sub-second (0.8s) timeout guards, guaranteeing the UI never hangs during network latency.
- **Consolidated Settings Hub (`dwm-settings-hub`)**:
  - Activated via `Super + Shift + s`.
  - Configure display profiles, Tokyo Night themes, system fonts, input devices, and default applications without touching raw configuration files manually.

---

## 7. Status Bar & Widget Interactions

The top status bar provides click and scroll actions:
- **Workspace Tags (1~9)**: Left-click to switch workspaces.
- **Weather Widget**: Left-click for summary; right-click for extended forecast.
- **Volume Widget**:
  - Scroll wheel: Volume up / down.
  - Left-click: Launch `pavucontrol` mixer.
  - Right-click: Toggle mute.
- **Clock Widget**: Left-click to switch between 12-hour and 24-hour display format.

---

## 8. Shell Mode Architecture (Suckless vs. Quickshell)

DWM-Oomaya supports two distinct shell experiences that can be toggled dynamically:

1. **Suckless Mode (Default, Recommended Sovereign Core)**:
   - Pure C `status2d` status bar, `dunst` notification daemon, and `dmenu` control surfaces.
   - Minimal memory footprint (<80MB idle), instant responsiveness, zero external dependencies.
   - Activate: `dwm-shell-mode suckless`

2. **Quickshell Mode (Optional Companion Plugin)**:
   - Rich QML-animated floating bar, interactive control center, and media widgets.
   - Activate: `dwm-shell-mode quickshell` (Requires companion repository).
   - Fast toggle shortcut: `Super + Ctrl + r`

To install the optional Quickshell companion:
```bash
git clone https://github.com/oomaya/quickshell-oomaya.git ~/.config/quickshell
```

---

## 9. Runtime TOML Customization

All user configuration files reside in `~/.config/dwm-oomaya/config/` and reload dynamically:
- **Keybindings**: `~/.config/dwm-oomaya/config/hotkeys.toml`
  - Define custom shortcuts and shell commands. Apply changes instantly with `Super + Shift + r`.
- **Themes & Colors**: `~/.config/dwm-oomaya/config/themes.toml`
  - Choose between Tokyo Night, Catppuccin, Gruvbox, and Nordic palettes.
- **Window Rules**: `~/.config/dwm-oomaya/config/window-rules.toml`
  - Automatically float specific applications or route them to designated workspace tags.

---

## 10. Maintenance & Seamless Updates

DWM-Oomaya guarantees non-destructive updates that never overwrite customized user configuration files:

```bash
cd ~/.local/src/dwm-oomaya
git pull origin main
make install-user
```
After installation completes, press **`Super + Shift + r`** to perform an in-place atomic restart under the same PID—your workspace state and all running applications will remain intact.

---

## 11. System Diagnostics & Troubleshooting

### One-Click Diagnostics
If you experience any issues, run the diagnostic suite:
```bash
dwm-diagnostics
```
This utility audits X11 server state, compositor health (`picom`), audio servers (`pipewire` / `pulseaudio`), and notification daemon readiness.

### Common Solutions
1. **Window shadow or border artifacts**:
   - Restart the compositor: `killall picom && picom -b &`
2. **Missing audio notifications or volume keys inactive**:
   - Inspect active sink: `dwm-volume status`
3. **VMware / VirtualBox 3D Acceleration Glitches**:
   - Run in VM-safe display mode: `dwm-display-setup --vm-safe`
