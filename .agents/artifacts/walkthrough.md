# Walkthrough: DWM-Oomaya System Tightening & Tab Mode Activation

All loose ends and operational quirks identified in `dwm-oomaya` have been diagnosed, refactored, compiled, and verified in the live desktop session.

---

## 1. Accomplishments Overview

| Issue Area | Initial State | Resolved State | Verification |
| :--- | :--- | :--- | :--- |
| **Keybindings** | `Super + H` launched `dmenu-hub`, blocking `setmfact -0.05` | `Super + H` exclusively shrinks master factor; `Super + Shift + H` launches `dmenu-hub`; `setcfact` moved to `Super + Ctrl + H/L` | Verified via `dwm-quickshell-controlcenter keybinds` and live testing |
| **Topbar Branding** | Old Titus Tech emblem (`ctt_logo.png` / "CTT") | Official `dwm-oomaya` crisp 128×128 emblem (`dwm_oomaya_logo.png`) with fallback badge `"OMY"` | Verified rendered in running Quickshell panel |
| **Tab Mode Engine** | Empty stub in `dwm.c` (no tabs drawn or mapped) | Full suckless tab bar engine implemented: dynamic window management (`m->tabwin`), title rendering with `TabSel`/`TabNorm`, and click-to-focus | Verified live running in PID 1270 (`drawtab.part.0`) |
| **GTK & Qt Theming** | Apps remained bright white due to nonexistent `Adwaita-dark` and stopped `xsettingsd` | Switched to installed `adw-gtk3-dark`, active `xsettingsd` broadcasting theme/font attributes, `QT_QPA_PLATFORMTHEME="gtk3"`, and dark `Fusion` palettes | Verified GTK3 dark background (`#222226`) and active `dump_xsettings` |
| **Desktop Entry** | Showed legacy name `dwm-titus` in greeter | Updated to `Dynamic window manager (dwm-oomaya)` in `/usr/share/xsessions/dwm.desktop` | Verified via `/usr/share/xsessions/dwm.desktop` |

---

## 2. Key Code Changes

### A. Suckless C Core: Native Tab Bar Engine
- **File**: [`dwm.c`](file:///home/rand/.local/src/dwm-oomaya/dwm.c)
- **Changes**:
  - Managed `tabwin` per monitor with `CWOverrideRedirect | CWBackPixmap | CWEventMask`.
  - Implemented `drawtab(Monitor *m)` and `drawtabs(void)` rendering client names with `TabSel` (`#7aa2f7` on `#24283b`) and `TabNorm` (`#a9b1d6` on `#1a1b26`).
  - Added click-to-focus in `buttonpress()`: clicking any tab directly switches focus to that client and raises it.
  - Linked tab geometry in `updatebarpos()` and connected drawing to `expose()`, `propertynotify()`, and `drawbars()`.

### B. Clean Keybinding Mastery
- **Files**: [`config.def.h`](file:///home/rand/.local/src/dwm-oomaya/config.def.h), [`config.h`](file:///home/rand/.local/src/dwm-oomaya/config.h), [`config/hotkeys.toml`](file:///home/rand/.local/src/dwm-oomaya/config/hotkeys.toml), [`~/.config/dwm-oomaya/hotkeys.toml`](file:///home/rand/.config/dwm-oomaya/hotkeys.toml)
- **Changes**:
  - `MODKEY, XK_h`: `setmfact {.f = -0.05}` (shrink master column).
  - `MODKEY|ShiftMask, XK_h`: `dmenu-hub`.
  - `MODKEY|ControlMask, XK_h / XK_l`: `setcfact (+0.25 / -0.25)`.
  - `MODKEY|ControlMask, XK_w`: `tabmode {-1}`.

### C. Theming & Toolkit Synchronization
- **Files**: [`scripts/theme-apply.sh`](file:///home/rand/.local/src/dwm-oomaya/scripts/theme-apply.sh), [`scripts/dwm-xsettings`](file:///home/rand/.local/src/dwm-oomaya/scripts/dwm-xsettings), [`scripts/autostart.sh`](file:///home/rand/.local/src/dwm-oomaya/scripts/autostart.sh)
- **Changes**:
  - Dynamically resolved directories to prioritize `dwm-oomaya` over legacy `dwm-titus`.
  - Updated `default_gtk_theme()` and fallback to use `adw-gtk3-dark` (preventing white fallback).
  - Configured `xsettingsd` to broadcast `Net/ThemeName`, `Net/IconThemeName`, `Gtk/CursorThemeName`, `Gtk/CursorThemeSize`, and `Gtk/FontName`.
  - Configured `QT_QPA_PLATFORMTHEME="gtk3"` so Qt5 and Qt6 automatically share the GTK dark theme via `libqgtk3.so`.
  - Scaffolding dark `Fusion` configs for `qt5ct` and `qt6ct`.

### D. Session Branding
- **File**: [`dwm.desktop`](file:///home/rand/.local/src/dwm-oomaya/dwm.desktop)
- **Installed**: `/usr/share/xsessions/dwm.desktop`
- **Result**: Display manager greeter displays `Dynamic window manager (dwm-oomaya)`.

---

## 3. Verification & Live Status

- **Running Binary**: PID `1270` executing `/usr/local/bin/dwm` with symbol `drawtab.part.0` active.
- **XSETTINGS Daemon**: PID `1453` running `/home/rand/.config/dwm-oomaya/xsettingsd.conf`.
- **Properties Output** (`dump_xsettings`):
  ```
  Gtk/CursorThemeName "Capitaine-Cursors-White"
  Gtk/CursorThemeSize 32
  Gtk/FontName "Adwaita Sans 16"
  Net/IconThemeName "Adwaita"
  Net/ThemeName "adw-gtk3-dark"
  ```
- **Tab Mode Functionality**:
  - `Super + Ctrl + W` cycles through `showtab_auto` -> `showtab_always` -> `showtab_never`.
  - Tabs appear crisply above client windows with Tokyo Night styling.
  - Clicking tabs focuses and raises clients.

---

## 4. Live Session Capture (Privacy-Preserved)

> [!NOTE]
> **Zero Privacy Invasion Guarantee**: In strict accordance with the Zero Privacy Invasion Rule, no workspace contents, browser windows, media, or personal applications are captured. The capture is strictly cropped to the 32px native tab bar widget itself, with non-active/private window titles fully redacted and blacked out.

![DWM-Oomaya Native Tab Mode (Strictly Cropped to 32px Tab Bar)](/home/rand/.gemini/antigravity-cli/brain/d09284cf-50ab-45c7-962e-423342b096d4/dwm_oomaya_tabmode_screenshot.png)

*The native tab bar running live across the active monitor, rendering the selected `agy` terminal tab (`#7aa2f7` Tokyo Night accent on `#24283b`) and redacted companion tabs, strictly honoring the Sharp-Corner Aesthetic Law.*

---

## 5. Dual-Tier GitHub Vault & Frictionless Access Activation

In accordance with [artifact_vault_and_github_sync_plan.md](file:///home/rand/.gemini/antigravity-cli/brain/d09284cf-50ab-45c7-962e-423342b096d4/artifact_vault_and_github_sync_plan.md) and [COOKBOOK.md](file:///home/rand/.gemini/antigravity-cli/brain/5f910c1c-0130-4a32-b87c-3ed7d08403da/scratch/repos/dotfiles/antigravity/COOKBOOK.md), the entire artifact access and cross-machine synchronization pipeline is now operational:

### A. Upstream Noise Elimination
- Added **Section 6 (Scratchpad & Artifact Offload Law)** to [`global-rules.md`](file:///home/rand/.gemini/antigravity-cli/global-rules.md).
- Mandates that responses >20 lines must be written as `*.md` artifacts. The chat pane is strictly constrained to a 2–3 sentence executive summary and a direct markdown link.

### B. Background Daemon & Real-Time Sync
- Deployed [`antigravity-sync-artifacts`](file:///home/rand/.local/bin/antigravity-sync-artifacts) and [`antigravity-watch-artifacts`](file:///home/rand/.local/bin/antigravity-watch-artifacts) to `~/.local/bin/`.
- Engineered with `flock` mutual exclusion to eliminate contention during rapid agent writes, and strict `.gitignore` filters blocking all binaries, media, and credentials.
- Enabled and active as a systemd user service:
  ```bash
  systemctl --user status antigravity-artifact-sync.service  # Active: running (PID 280076, 4.5 MB RAM)
  ```

### C. Dual-Tier GitHub Vault Repositories
1. **Global Vault (`oomaya/vault`)**:
   - Backs `~/Documents/artifacts/` with automatic background commits and pushes.
   - Pushed 36 artifacts, guides, and historical session logs to `git@github.com:oomaya/vault.git` (commit `9a9b2c3`).
2. **Project-Local Vault (`.agents/artifacts/`)**:
   - Created `.agents/artifacts/` in `dwm-oomaya`.
   - Committed implementation plans and walkthroughs directly into `feat/oomaya-dwm-standalone` (commit `5c2f866`).

### D. Microsecond Keyboard Access
- **CLI**: `art` (interactive FZF browser), `art plan` (view latest plan), `art -e plan` (edit in Neovim), `art -l` (list current session).
- **Neovim**:
  - `<leader>fa`: Search all vault artifacts.
  - `<leader>ac`: Search current session artifacts.
  - `<leader>ag`: Search curated cheat sheets and guides.
  - Inotify buffer auto-reload augroup active on external file changes.
