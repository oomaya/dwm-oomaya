# DWM-Oomaya Architectural Patch Map & Reference Manual

This document maps all patches, layout functions, and keybinding mechanisms integrated into **dwm-oomaya** to achieve full parity with **chadwm** while building upon **dwm-titus**'s modern base, strictly adhering to the **Zero Hardcoded Literals Law** and the **Native Header-Based Configuration Law**.

---

## 1. Patch Inventory & Layout Matrix

| Patch Name | Primary File(s) | Functions Provided | Keybind / Trigger |
| :--- | :--- | :--- | :--- |
| **Vanity Gaps** | `vanitygaps.c`, `functions.h` | `tile`, `spiral`, `dwindle`, `deck`, `bstack`, `bstackhoriz`, `grid`, `nrowgrid`, `horizgrid`, `gaplessgrid`, `centeredmaster`, `centeredfloatingmaster`, `togglegaps`, `incrgaps`, `incrigaps`, `incrogaps`, `incrihgaps`, `incrivgaps`, `incrohgaps`, `incrovgaps`, `defaultgaps` | `MODKEY + Ctrl + i/d`<br>`MODKEY + Shift + i`<br>`MODKEY + Ctrl + Shift + i`<br>`MODKEY + Ctrl + o`<br>`MODKEY + Ctrl + Shift + o`<br>`MODKEY + Ctrl + 6/7/8/9`<br>`MODKEY + Ctrl + Shift + 6/7/8/9`<br>`MODKEY + Ctrl + Shift + d` |
| **Shiftview** | `shiftview.c`, `functions.h` | `shiftview` | `MODKEY + Left`<br>`MODKEY + Right` |
| **Cyclelayouts** | `dwm.c`, `functions.h` | `cyclelayout` | `MODKEY + Ctrl + comma`<br>`MODKEY + Ctrl + period` |
| **Actual Fullscreen** | `dwm.c`, `functions.h` | `togglefullscr` | `MODKEY + f` |
| **Dynamic Borders** | `dwm.c`, `functions.h` | `setborderpx` | `MODKEY + Shift + minus`<br>`MODKEY + Shift + p`<br>`MODKEY + Shift + w` |
| **Self Restart** | `dwm.c`, `functions.h` | `restart` | `MODKEY + Shift + r` |
| **Window Hide (Awesomebar)** | `dwm.c`, `functions.h` | `hidewin`, `restorewin` | `MODKEY + e`<br>`MODKEY + Shift + e` |
| **Bartabgroups / Tabmode** | `dwm.c`, `functions.h` | `tabmode`, `focuswin` | `MODKEY + Ctrl + w` |
| **Drag Mfact / Cfact** | `dwm.c` | `dragmfact`, `dragcfact` | `Ctrl + Button1`<br>`Ctrl + Button3` |
| **Move or Place** | `dwm.c` | `moveorplace` | `MODKEY + Button1` |
| **Cfacts** | `dwm.c` | `setcfact` | `MODKEY + Ctrl + h/l`<br>`MODKEY + Shift + o` |
| **Movestack** | `dwm.c` | `movestack` | `MODKEY + Shift + j/k` |
| **XF86 Audio & Light** | `<X11/XF86keysym.h>` | `spawn` | `XF86XK_AudioLowerVolume`<br>`XF86XK_AudioRaiseVolume`<br>`XF86XK_AudioMute`<br>`XF86XK_MonBrightnessUp`<br>`XF86XK_MonBrightnessDown` |

---

## 2. Zero Hardcoded Literals Compliance

In accordance with the foundational law from `oomaya/antigravity-skills` and `oomaya/dotfiles`:

1. **No Absolute User Paths**: No paths containing `/home/<user>` appear anywhere in `config.def.h` or source files.
2. **Dynamic PATH Resolution**:
   - `pactl` instead of `/usr/bin/pactl`
   - `light` instead of `/usr/bin/light`
   - Terminal selection defaults cleanly to `ghostty` (primary in dotfiles) or `kitty` (secondary).
   - Screenshots invoke `maim` and `xclip` directly.

---

## 3. Native Header Workflow Verification

To verify that changes in `config.def.h` take effect without needing runtime TOML or `sxhkd`:

```bash
make clean
rm -f config.h
make dwm
```
