# DWM-Oomaya Live Smoke Test Checklist

[English](SMOKE-TEST.md) | [한국어](SMOKE-TEST.ko.md)

A structured 10-step verification matrix designed to validate the live host X11 session and desktop ergonomics of `dwm-oomaya`.

---

## 1. Session Launch & Status Bar
- [ ] **Status Bar Visibility**: Confirm the ChadDWM/Status2d top bar is clearly visible at the top of the screen.
- [ ] **Tag Indicators (1~9)**: Workspace numbers and active window indicators are rendered with Tokyo Night colors.
- [ ] **Status Widgets**: Weather, CPU/RAM utilization, Volume, and Clock are displayed on the right side of the bar.
- [ ] **Status Bar Mouse Interactions**:
  - Left-click Clock: Toggles between 12-hour and 24-hour time formats.
  - Scroll wheel on Volume: Adjusts sound level (`dwm-volume up/down`).
  - Right-click Volume: Toggles mute (`dwm-volume mute`).

---

## 2. Terminal & Core Applications
- [ ] `Super + Shift + Enter`: Launches primary terminal (Ghostty, Kitty, or Alacritty) immediately.
- [ ] `Super + s`: Toggles centered floating scratchpad terminal.
- [ ] `Super + e`: Opens primary file manager (Thunar or xdg-open).

---

## 3. Application Search & Dmenu Launchers
- [ ] `Super + d`: Opens centered `dmenu-desktop` application launcher with fuzzy filtering.
- [ ] `Alt + p`: Opens command runner (`dmenu-run`).
- [ ] `Alt + Tab`: Opens interactive window switcher (`dmenu-windows`) and shifts focus instantly.

---

## 4. Control Center & Settings Hub
- [ ] `Super + F1` or `Super + c`: Invokes the native lightweight control center (`dwm-controlcenter` / `dwm-flow`).
  - Verify that Wi-Fi, Bluetooth, Audio devices, and Power Profile submenus are interactive and responsive.
- [ ] `Super + Shift + s`: Invokes the system settings hub (`dwm-settings-hub`).
  - Verify that Display, Appearance, Fonts, and Input submodules open cleanly.

---

## 5. Keybindings Help Palette
- [ ] `Super + /`: Pops up the categorized searchable keybindings palette (`dwm-keybinds`).
  - Verify typing filters shortcuts in real-time.

---

## 6. Window Tiling & Layout Manipulation
- [ ] `Super + j` / `Super + k`: Traverses focus down / up the window stack.
- [ ] `Super + Shift + j` / `Super + Shift + k`: Swaps window position within the stack.
- [ ] `Super + h` / `Super + l`: Shrinks / expands master area ratio.
- [ ] `Super + Tab`: Cycles tiling layouts (Master-Stack -> Monocle -> Grid -> Spiral).
- [ ] `Super + f`: Toggles true borderless fullscreen.
- [ ] `Super + Shift + Space`: Toggles floating mode on active window.
- [ ] `Super + q`: Gracefully closes the focused window.

---

## 7. Vanitygaps (Pixel Padding)
- [ ] `Super + Ctrl + t`: Toggles all window gaps on/off.
- [ ] `Super + Ctrl + i` / `Super + Ctrl + d`: Increases / decreases gap size (+5px / -5px).
- [ ] `Super + Alt + Shift + =`: Restores default gap geometry.

---

## 8. Workspaces (Tags) & Multi-Monitor
- [ ] `Super + 1~9`: Switches directly to the specified workspace tag.
- [ ] `Super + Shift + 1~9`: Sends the focused window to the target tag.
- [ ] `Super + Left` / `Super + Right`: Shifts view to previous / next active tag.
- [ ] `Super + ,` / `Super + .`: Shifts focus across physical monitors.
- [ ] `Super + Shift + ,` / `Super + Shift + .`: Sends active window to target monitor.

---

## 9. Hardware Keys, Media & Screenshots
- [ ] Volume Keys (`XF86AudioRaiseVolume`, `XF86AudioLowerVolume`, `XF86AudioMute`): Adjusts volume and triggers Dunst OSD popup.
- [ ] Brightness Keys (`XF86MonBrightnessUp`, `XF86MonBrightnessDown`): Adjusts display backlight.
- [ ] `Print`: Launches screenshot menu (`dmenu-scrot`).
- [ ] `Super + u` or `Shift + Print`: Crops screen area directly to clipboard.

---

## 10. Shell Mode & Power Management
- [ ] `Super + Ctrl + r`: Toggles shell mode (`dwm-shell-mode toggle`).
  - If Quickshell companion is not installed, alerts user cleanly while maintaining Suckless mode.
- [ ] `Alt + x` or `Super + Ctrl + q`: Opens power management menu (`dmenu-power`).
- [ ] `Super + Shift + r`: In-place hot restart of DWM (refreshes binary without closing client windows).
