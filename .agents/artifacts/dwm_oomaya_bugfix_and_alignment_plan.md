# DWM-Oomaya Review Feedback Remediation & Bugfix Plan

## Executive Summary
This plan addresses all seven issues raised in the user review of `dwm_settings_hub_user_manual.md` and live testing on PeppermintOS Debian trixie:
1. **Super + E "Black Hole"**: `MODKEY, XK_e` was mapped to `hidewin`, hiding windows instead of opening the file manager.
2. **Wallpaper Picker Failure**: `~/Pictures/backgrounds` did not exist, causing the picker to exit silently.
3. **Requisite Packages in `install.sh`**: `feh`, `blueman`, `maim`, `slop`, `dunst`, and `xsettingsd` were missing because desktop packages were not installed or lacked `slop`.
4. **Screenshot Tools Inoperative**: `maim` was missing, and `dmenu-scrot` / `dwm-screenshot` lacked fallback to `xfce4-screenshooter`.
5. **Notification System**: `dunst` was missing, causing fallback to un-themed `xfce4-notifyd`.
6. **Theme Switcher Topbar Incomplete**: `dwm.c`'s `load_themes_toml()` only recomputed `SchemeNorm` and `SchemeSel`, completely omitting `SchemeTitle` (index 2). Furthermore, `dwm-settings-appearance` hardcoded `dwm-titus` paths.
7. **MIME Type Status Clarification**: `inode/directory` mapping confusion in default apps.

---

## Quota & Token Budget Analysis (Go/No-Go Decision Gate)

### Current Allowance Snapshot
- **Gemini Models Weekly Limit Remaining**: 80.1% (resets in 3d 14h)
- **Gemini Models Five Hour Limit Remaining**: 55.4% (resets in 2h 59m)
- **Claude and GPT Models Weekly Limit Remaining**: 100.0%
- **Claude and GPT Models Five Hour Limit Remaining**: 100.0%

### Estimated Phase Expenditure
- **Phase 1 (Inspection & Planning)**: ~6,000 tokens (Gemini Flash)
- **Phase 2 (Implementation & Code Editing)**: ~12,000 tokens (Gemini / Claude pairing)
- **Phase 3 (Compilation, Verification & Testing)**: ~5,000 tokens (Local Shell)

### Model Selection & Risk Mitigation Rationale
- **Go/No-Go Verdict**: **GO**. Quotas across both Gemini and Claude are highly abundant (>55% and 100%).
- **Strategy**: Gemini handles surgical edits across C source (`dwm.c`, `config.def.h`, `config.h`) and shell scripts (`dwm-settings-hub`, `dmenu-scrot`, `install.sh`, `dwm-packages.sh`), followed by local compiler verification.

---

## Detailed Root Cause Analysis & Remediation Matrix

| Issue | Root Cause | Target Files | Remediation |
|---|---|---|---|
| **Super + E window disappearance** | `MODKEY, XK_e` bound to `hidewin` in `config.def.h` | `config.def.h`, `config.h` | Bind `MODKEY, XK_e` to `xdg-open ~ \|\| thunar \|\| pcmanfm`. Move `hidewin` / `restorewin` to `MODKEY\|ControlMask, XK_e` and `MODKEY\|ControlMask\|ShiftMask, XK_e`. |
| **Wallpaper picker doesn't open** | `$HOME/Pictures/backgrounds` missing; directory check failed without creation or fallback | `scripts/dwm-settings-hub`, `install.sh` | Auto-create `$HOME/Pictures/backgrounds`, seed default `assets/dwm-oomaya.jpg`, and provide GUI file picker fallback. |
| **Missing packages in `install.sh`** | `debian:screenshot-optional` lacked `slop`; `desktop` profile was not reconciled during update; no explicit runtime pre-flight audit | `scripts/dwm-packages.sh`, `install.sh` | Add `slop` to `screenshot-optional` across distros; add pre-flight & verification checks for `feh`, `blueman`, `maim`, `slop`, `dunst`, `xsettingsd`. |
| **Screenshots broken** | `dmenu-scrot` & `dwm-screenshot` invoked `maim` unconditionally with no fallback | `dmenu-oomaya/scripts/dmenu-scrot`, `dwm-oomaya/scripts/dwm-screenshot`, `config.def.h` | Add graceful fallback to `xfce4-screenshooter` / `scrot`. Map `Print` key (with & without Super) in dwm. |
| **Notifications** | `dunst` not installed, falling back to `xfce4-notifyd` | `scripts/dwm-packages.sh`, `install.sh` | Ensure `dunst` is installed and started by `autostart.sh` using Tokyo Night theme. |
| **Theme title portion stays Tokyo Night** | `dwm.c` line 4073: `for (i = 0; i < 2; i++)` only rebuilt `SchemeNorm` & `SchemeSel`, never `SchemeTitle`; `dwm-settings-appearance` had hardcoded `dwm-titus` paths | `dwm.c`, `scripts/dwm-settings-appearance` | Rebuild `SchemeTitle` in `load_themes_toml()`; patch `dwm-settings-appearance` to dynamically resolve `dwm-oomaya`. |
| **MIME type display** | `dwm-default-apps status` displays raw `inode/directory=pcmanfm.desktop` | `scripts/dwm-settings-hub`, user manual | Add human-readable label "Default File Manager (inode/directory)" in status viewer. |

---

## Step-by-Step Implementation Steps

### Phase 1: Window Manager Keybindings & Core Theme Fix (`dwm-oomaya`)
1. **`config.def.h` & `config.h`**:
   - Rebind `MODKEY, XK_e` to file manager.
   - Relocate `hidewin` to `MODKEY|ControlMask, XK_e` and `restorewin` to `MODKEY|ControlMask|ShiftMask, XK_e`.
   - Add native `Print` keysym bindings for `dmenu-scrot` with fallback.
2. **`dwm.c`**:
   - In `load_themes_toml()`, expand scheme reconstruction loop to include `SchemeTitle`:
     ```c
     const char *new_clrs[3][3] = {
         [SchemeNorm]  = { c_normfg, c_normbg, c_normborder },
         [SchemeSel]   = { c_selfg,  c_selbg,  c_selborder  },
         [SchemeTitle] = { c_selfg,  c_normbg, c_normbg     }
     };
     for (i = 0; i < 3; i++) { ... }
     ```
   - Recompile `dwm` via `make` and verify zero compiler warnings.

### Phase 2: Dwm Settings Hub & Appearance Path Resolution
1. **`scripts/dwm-settings-appearance`**:
   - Resolve `$dwm_cfg_name` and `$dwm_data_name` preferring `dwm-oomaya` over `dwm-titus`.
   - Validate `dwm-settings-appearance snapshot` returns code 0 and valid theme table.
2. **`scripts/dwm-settings-hub`**:
   - Ensure `$HOME/Pictures/backgrounds` is created and seeded if missing.
   - Add fallback to open GUI file picker (`xdg-open "$bg_dir" || thunar "$bg_dir" || pcmanfm "$bg_dir"`).
   - Format `dwm-default-apps status` output with friendly labels.

### Phase 3: Screenshot Resilience & Fallbacks
1. **`dmenu-oomaya/scripts/dmenu-scrot`**:
   - Implement automatic engine detection: `maim` $\rightarrow$ `xfce4-screenshooter` $\rightarrow$ `scrot`.
2. **`dwm-oomaya/scripts/dwm-screenshot`**:
   - Add `xfce4-screenshooter` fallback when `maim` is missing.

### Phase 4: Installer & Package Map Hardening
1. **`scripts/dwm-packages.sh`**:
   - Add `slop` alongside `maim` in `screenshot-optional` for Debian, Fedora, and Arch.
2. **`install.sh`**:
   - Ensure wallpapers directory `$HOME/Pictures/backgrounds` is created unconditionally.
   - Add a runtime package audit check reporting the presence of `feh`, `blueman-manager`, `maim`, `slop`, `dunst`, `xsettingsd`.
   - Ensure `desktop` and `screenshot-optional` profiles are cleanly installable.

---

## Verification & Validation Suite
1. Run `make` in `dwm-oomaya` and verify clean build.
2. Run `tests/test-dwm-settings-theme.sh` and `tests/test-dwm-settings-appearance.sh`.
3. Test `dwm-settings-hub --list` and dry-run execution.
4. Test `dmenu-scrot` execution with `xfce4-screenshooter` fallback.
5. Verify `dwm-settings-appearance snapshot` succeeds without environment variable overrides.
