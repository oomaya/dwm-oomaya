# DWM-Oomaya Bugfix, Bare-Server Deployment & Post-Mortem Report

## 1. Technical Post-Mortem

### A. Theme Switcher Topbar Incomplete (`SchemeTitle` Omisssion)
- **Symptom**: Applying themes (Nord, Catppuccin, Gruvbox) changed the GTK theme and partially colored dwm tags, but the central window title portion of the topbar remained stuck in Tokyo Night colors.
- **Root Cause in C Core (`dwm.c`)**:
  In `dwm.c`, `load_themes_toml()` (line 4073) parsed `themes.toml` but only allocated and replaced two schemes:
  ```c
  const char *new_clrs[2][3] = {
      { c_normfg, c_normbg, c_normborder },
      { c_selfg,  c_selbg,  c_selborder  }
  };
  for (i = 0; i < 2; i++) {
      Clr *newscm = drw_scm_create(drw, new_clrs[i], 3);
      if (newscm) {
          drw_scm_free(drw, scheme[i], 3);
          scheme[i] = newscm;
      }
  }
  ```
  However, in `dwm.c`, `drawbar()` draws the active window title using `SchemeTitle`:
  ```c
  drw_setscheme(drw, scheme[m == selmon ? SchemeTitle : SchemeNorm]);
  ```
  In the scheme enum (`enum { SchemeNorm, SchemeSel, SchemeTitle, TabSel, TabNorm... }`), `SchemeTitle` is index `2`. Because the reload loop only updated indices `0` and `1`, `scheme[SchemeTitle]` was never updated and remained permanently locked to compile-time Tokyo Night colors (`themes/tokyonight.h`).
- **Path Resolution Defect**:
  In `scripts/dwm-settings-appearance`, `themes_file` was hardcoded to `$config_home/dwm-titus/themes.toml`. When `dwm-settings-theme` queried `dwm-settings-appearance snapshot`, it exited with error code 3 (`No readable user or managed themes.toml file is available`).
- **Remediation**:
  1. Expanded `load_themes_toml()` in `dwm.c` to rebuild `SchemeTitle` (`index 2`) dynamically with `{ c_titlefg, c_titlebg, c_titleborder }`, supporting `titlefgcolor` / `titlebgcolor` overrides with fallback to `{ c_selfg, c_normbg, c_normborder }`.
  2. Updated `scripts/dwm-settings-appearance` and `scripts/dwm-settings-font` to dynamically resolve `$dwm_cfg_name` (`dwm-oomaya` before `dwm-titus`), enabling clean `snapshot` execution.

---

### B. What Caused the MIME Type Confusion?
- **Symptom**: User saw `"inode/notification is set as pcmanfm, even though I set pcmanfm as my default file picker."`
- **Root Cause**:
  Under the XDG MIME standard, directories/folders are identified by the MIME type string `inode/directory`.
  When `dwm-default-apps status` was executed in the Hub, it dumped the raw XDG key:
  ```
  inode/directory=pcmanfm.desktop
  ```
  Because the word `inode/` looks like an internal filesystem descriptor rather than "File Manager", it caused natural confusion with system notifications or unexpected settings.
- **Remediation**:
  Updated `dwm-settings-hub` to transform the raw MIME key into a human-readable title:
  ```
  Default File Manager (inode/directory)=pcmanfm.desktop
  ```

---

### C. Super + E "Black Hole" Keybinding
- **Symptom**: Pressing `Super + E` caused active windows to vanish into thin air.
- **Root Cause**:
  `config.def.h` mapped `{ MODKEY, XK_e, hidewin, {0} }`. In standard Linux/Windows desktop ergonomics, `Super + E` is the universal keybinding for opening the file manager. In dwm, `hidewin` unmaps the client window and pushes it onto the hidden window stack.
- **Remediation**:
  - Bound `MODKEY, XK_e` to `spawn, SHCMD("xdg-open ~ || thunar || pcmanfm")`.
  - Moved `hidewin` to `MODKEY|ControlMask, XK_e` and `restorewin` to `MODKEY|ControlMask|ShiftMask, XK_e`.
  - Recompiled and installed dwm to `~/.local/bin/dwm`.

---

### D. Bare-Server Deployment Hardening
- **Objective**: Ensure a completely unattended, working desktop on minimal bare-server Linux installs (Debian netinst, Arch minimal, Fedora Server).
- **Remediations**:
  1. **File Manager Provisioning**: Added `thunar` and `gvfs` to `debian:desktop`, `fedora:desktop`, and `arch:desktop` (previously buried in `desktop-optional`).
  2. **Interactive Screenshots**: Added `slop` alongside `maim` in `screenshot-optional` across Debian, Fedora, and Arch.
  3. **Screenshot Engine Fallbacks**: Updated `dmenu-scrot` and `dwm-screenshot` with resilient fallbacks: `maim` $\rightarrow$ `xfce4-screenshooter` $\rightarrow$ `scrot`.
  4. **Wallpaper & Screenshots Directories**: `install.sh` now unconditionally provisions `$HOME/Pictures/backgrounds` and `$HOME/Pictures/Screenshots`, seeding `assets/dwm-oomaya.jpg`.
  5. **Dependency Audit Gate**: Embedded `$REPO_DIR/scripts/check-deps.sh` directly into the conclusion of `install.sh` to audit all desktop runtime binaries (`dunst`, `picom`, `feh`, `thunar`, `maim`, `slop`, `blueman-manager`, `xsettingsd`).

---

## 2. Verification Results

| Component | Test Command | Outcome |
|---|---|---|
| **dwm C Core** | `make clean && make dwm` | Exit 0, 0 compiler warnings |
| **Appearance Engine** | `dwm-settings-appearance snapshot` | Exit 0, active theme `tokyonight`, all 15 palettes resolved |
| **Settings Hub** | `dwm-settings-hub --list` | Exit 0, all 9 mnemonic modules available |
| **Package Map (Debian Desktop)** | `dwm-packages.sh debian desktop` | Exit 0, contains `thunar`, `gvfs`, `dunst`, `feh`, `blueman` |
| **Package Map (Screenshots)** | `dwm-packages.sh debian screenshot-optional` | Exit 0, contains `maim` AND `slop` |
| **Installed Binaries** | `install -Dm755` to `~/.local/bin` | `dwm`, `dwm-settings-appearance`, `dwm-settings-font`, `dwm-settings-hub`, `dmenu-scrot`, `dwm-screenshot` |
