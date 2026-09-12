---
name: dwm-craft
description: Use when modifying, patching, compiling, or auditing suckless dwm, dwm-titus, or dwm-oomaya source code, layout engines, and native header-based keybindings.
---

# DWM Craft & Patching Playbook (dwm-oomaya)

This skill provides expert procedures, architectural laws, and troubleshooting guides for developing, patching, and maintaining `dwm` (specifically `dwm-oomaya` based on `dwm-titus` with `chadwm` layout and keybinding parity).

---

## 1. Core Architectural Laws

### A. Zero Hardcoded Literals Law (Absolute Top Priority)
- **Never hardcode usernames, `/home/*` paths, or strict binary paths** (e.g. avoid `/usr/bin/pactl`, `/usr/bin/light`, `/home/ted/...`).
- In `config.def.h` / `config.h`, invoke binaries directly via `PATH` lookup: `"pactl"`, `"light"`, `"maim"`, `"rofi"`, `"ghostty"`, `"kitty"`.
- If scripts or helper binaries are needed, resolve them dynamically using `$HOME` (`getenv("HOME")`) or `~/.local/bin/`.

### B. Sharp-Corner Aesthetic Law
- Window borders and client frames must default to crisp, square corners (`borderpx = 1` or `2`, no synthetic border smoothing or rounding).
- Ensure color palettes match the system aesthetic (e.g. TokyoNight, Catppuccin, Gruvchad).

### C. Native Header-Based Configuration Law (No SXHKD)
- dwm is inherently designed to be configured through C headers (`config.def.h` -> `config.h`) and compiled into a single binary.
- Do not introduce external keybinding daemons like `sxhkd` for window manager functions or multimedia keys.
- All keybindings (multimedia, brightness, application spawns, gap controls, layout toggles) must be registered in the `keys[]` array using `<X11/XF86keysym.h>` and handled natively via `XGrabKey` / `keypress()`.

---

## 2. Layout Matrix & Vanitygaps Architecture

`dwm-oomaya` provides 14 layouts integrated via `vanitygaps.c` and `functions.h`:

| Layout | Arrange Function | Source | Gap Aware |
| :--- | :--- | :--- | :--- |
| `[]=` | `tile` | Vanitygaps / Cfacts | Yes (Inner & Outer) |
| `[M]` | `monocle` | Core dwm | N/A |
| `[@]` | `spiral` | Fibonacci | Yes |
| `[\\]`| `dwindle` | Fibonacci | Yes |
| `H[]` | `deck` | Deck | Yes |
| `TTT` | `bstack` | Bottomstack | Yes |
| `===` | `bstackhoriz`| Bottomstack | Yes |
| `HHH` | `grid` | Gridmode | Yes |
| `###` | `nrowgrid` | Nrowgrid | Yes |
| `---` | `horizgrid` | Horizgrid | Yes |
| `:::` | `gaplessgrid`| Gaplessgrid | Yes |
| `\|M\|`| `centeredmaster`| Centeredmaster | Yes |
| `>M>` | `centeredfloatingmaster` | Centeredmaster | Yes |
| `><>` | `NULL` (Floating) | Core dwm | N/A |

### Monitor Gap Fields
All vanitygap layouts query the `Monitor` struct for:
- `m->gappih`: Inner horizontal gap between clients
- `m->gappiv`: Inner vertical gap between clients
- `m->gappoh`: Outer horizontal gap between clients and monitor edge
- `m->gappov`: Outer vertical gap between clients and monitor edge

---

## 3. Required Patches & Function Declarations

When adding or synchronizing functionality between `chadwm` and `dwm-titus`:

1. **`functions.h`**: Forward-declares layout functions and gap helpers before `config.h` is parsed. Must be guarded with `#define FORCE_VSPLIT 1`.
2. **`shiftview.c`**: Implements tag navigation (`shiftview`) with circular bit shifting.
3. **`cyclelayouts`**: Implements `cyclelayout(const Arg *arg)` to cycle through `layouts[]`.
4. **`actualfullscreen`**: Implements `togglefullscr(const Arg *arg)` using `setfullscreen(selmon->sel, !selmon->sel->isfullscreen)`.
5. **`setborderpx`**: Dynamically adjusts client border pixels (`selmon->borderpx`) and triggers `arrange(selmon)`.
6. **`restart`**: Sets `running = 0` to trigger a clean reload in wrapper loops without terminating X.
7. **`winhide`**: Provides `hidewin` and `restorewin` client stacking using `c->ishidden`.
8. **`bartabgroups`**: Provides `tabmode` and `focuswin`.
9. **`dragmfact` & `dragcfact`**: Mouse-driven resizing of master and client factors.

---

## 4. Verification & Validation Workflow

Always verify builds using a strict compilation gate:

```bash
# 1. Clean previous build artifacts
make clean

# 2. Re-generate config.h from config.def.h
rm -f config.h && cp config.def.h config.h

# 3. Compile with strict error detection
make dwm

# 4. Check for undefined symbols
nm -u dwm | grep -E 'vanity|shiftview|togglefullscr' || true
```

---

## 5. Agent Triad Protocol for DWM Changes

- **🎨 Systems Architect**: Plans header definitions, patches, and suckless C structs without bloat.
- **🔍 SRE & Security Auditor**: Enforces Zero Hardcoded Literals, validates memory allocations (`ecalloc`), pointer null-checks (`if (!selmon->sel) return;`), and signals.
- **⚖️ Desktop UX Arbiter**: Guarantees mnemonic keybindings, sharp corners, and smooth layout switching.
