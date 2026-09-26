# dwm-oomaya Settings Hub & Master Launcher User Manual

Welcome to the **dwm-oomaya Settings Hub** user guide. This environment delivers a complete, lightweight, keyboard-driven desktop experience built on Suckless principles. It replaces heavy background daemons and bloated desktop control centers with high-performance, native `dmenu` interfaces backed by robust POSIX utility engines.

---

## 1. Quick Access Keybindings

| Shortcut | Action | Description |
| :--- | :--- | :--- |
| **`Super+Shift+h`** | **DWM Master Hub** | Unified access point for apps, settings, windows, and system power. |
| **`Alt+Tab`** | **Window Switcher** | Interactive client switcher with workspace tag indicators. |
| **`Super+s`** | **Settings Hub** | Opens the Settings Hub directly (`dwm-settings`). |
| **`Super+/`** | **Keybindings Palette** | Searchable popup viewer of all configured hotkeys. |
| **`Alt+p`** | **Run Command** | Native dmenu command prompt. |
| **`Super+d`** | **Desktop Launcher** | Filtered `.desktop` application menu. |
| **`Alt+x`** | **Power Menu** | Lock, logout, reboot, or shut down. |
| **`Super+Shift+r`**| **Reload DWM** | Seamlessly restart window manager without dropping running apps. |

---

## 2. Settings Hub Modules (`dwm-settings-hub`)

When you open `⚙ Settings Hub` from the Master Hub (or press `Super+s`), you have 9 unified modules:

### 󰏘 1. Wallpaper & Background
- **🎲 Randomize Wallpaper**: Cycles through wallpapers in `~/Pictures/backgrounds` and applies one randomly via `dwm-settings-wallpaper randomize`.
- **📁 Select from ~/Pictures/backgrounds**: Interactive dmenu image picker. Selecting any `.jpg`, `.png`, or `.webp` file applies it instantly.
- **↺ Reset to Default**: Restores the bundled distribution default wallpaper.
> [!NOTE]
> Powered by `feh`. If `feh` is not installed, install it with `sudo apt install feh`.

### 🎨 2. Theme & Palette
- **Available Themes**: Tokyo Night (default), Catppuccin Mocha, Nord, and Gruvbox Dark.
- **Mechanism**: Seamlessly reconfigures your active GTK theme, cursor palette, and desktop color tokens via `dwm-settings-theme`.

###  3. Font & Typography
- **Presets**:
  - `JetBrainsMono Nerd Font (100% scale)` — Clean, modern coding font.
  - `JetBrainsMono Nerd Font (125% scale)` — Optimized for high-DPI (2K/4K) monitors.
  - `MesloLGS Nerd Font (100% scale)` — Tight glyph terminal typography.
  - `Reset Font Defaults` — Restores default system fontconfig metrics.

### 󰖲 4. Default Applications
- **🌐 Default Browser**: Detects installed browsers (`brave`, `firefox`, `chromium`, etc.) and sets your system-wide `x-scheme-handler/http(s)` association.
- **📁 Default File Manager**: Select between `thunar.desktop`, `nautilus.desktop`, `pcmanfm.desktop`, etc.
- **📋 View Defaults Status**: Inspects current XDG mime types and defaults.

### 󰐥 5. Display & Monitors
- **󰍹 Interactive Setup Wizard (Terminal)**: Launches `dwm-display-setup wizard` in a Ghostty terminal. It guides you interactively through resolution selection, refresh rates, monitor positioning (left/right/above/below), primary output selection, and anti-tearing (`TearFree`). Changes are verified with a 15-second timed confirmation rollback.
- **📊 Current Display Status**: Displays connected outputs, current modes, and managed Xorg configs in dmenu.
- **↺ Auto-Detect Displays**: Executes `xrandr --auto` to quickly recover or reset display geometry after docking/undocking.
- **󰍺 Select Display Profile**: Apply pre-configured multi-monitor profiles.

###  6. Autostart Services
- **How It Works**: Queries the system XDG autostart repository via `dwm-xdg-autostart snapshot`.
- **Display**: Lists services with clear status tags:
  ```
  [✓ Enabled ] NetworkManager Applet (nm-applet.desktop)
  [✓ Enabled ] Fcitx 5               (org.fcitx.Fcitx5.desktop)
  [✗ Disabled] PolicyKit Agent       (polkit-mate...desktop)
  ```
- **Actions**: Selecting any service lets you:
  1. **Toggle State**: Enable or disable the service for your next login.
  2. **Reset to Default**: Revert user overrides back to vendor defaults.

### 󰌌 7. Input Devices
- **🖱 Mouse Acceleration & Sensitivity**: Choose between Slow (`1/2 4`), Normal (`1 2`), Fast (`2 2`), or Very Fast (`3 1`) via `xset m`.
- **⌨ Keyboard Layout**: Rapidly toggle layouts between US English (`us`), Korean (`kr`), Japanese (`jp`), or enter a custom layout (e.g. `de`, `fr`, `es`) using `setxkbmap`.
- **📋 List Connected Devices**: Displays connected mice, keyboards, touchpads, and virtual pointers.
- **↺ Reset Input Defaults**: Resets keyboard to US and mouse acceleration to system defaults.

### 󰂯 8. Bluetooth Manager
- **GUI Manager**: Launches `blueman-manager` for device pairing, connecting Bluetooth headphones, and audio profiles.
- **Topbar Integration**: Follows our Signal-to-Noise principle. When no Bluetooth device is connected, the indicator takes 0 pixels. When a device (such as headphones) is connected, it displays `BT <count>`.

### 󰍛 9. System Diagnostics
- **📋 Quick Diagnostics Summary**: Opens a dmenu popup summarizing build tools, X11 libraries, and optional desktop packages.
- **🖥 Run Full Diagnostics**: Launches `dwm-diagnostics` in your terminal for deep dependency auditing.

---

## 3. Master Action Hub (`Super+Shift+h`) Reference

The Master Hub is your primary launcher:

1. ** Applications (`dmenu-desktop`)**: Full `.desktop` application menu with categorization.
2. ** Run Command (`dmenu-run`)**: Execute any binary from `$PATH`.
3. ** Window Switcher (`dmenu-windows`)**: Fast client switcher with icon detection and workspace tag numbering (`[Tag 1]`, `[Tag 2]`, etc.).
4. **⚙ Settings Hub (`dwm-settings`)**: Opens the 9-module configuration center.
5. **🦄 Omacorn DPI Bypass (`dmenu-omacorn`)**: DPI scaling toggles.
6. **󱉥 Clipboard History (`dmenu-clip`)**: Interactive clipboard history manager.
7. ** Keybindings Palette (`dwm-keybinds`)**: Instant cheat sheet for all hotkeys.
8. ** Screenshot Tool (`dmenu-scrot`)**: Area selection or full screen screenshot capture.
9. ** Power Menu (`dmenu-power`)**: System lock, logout, restart, and power off.
