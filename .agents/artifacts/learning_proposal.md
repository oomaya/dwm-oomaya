# Learning Proposal: Suckless Multi-Distro Resilience, Shell Traps & Xft Font Invariants

## 1. Classification & Scope
- **Target File**: [`/home/rand/.gemini/config/skills/dwm-craft/SKILL.md`](file:///home/rand/.gemini/config/skills/dwm-craft/SKILL.md)
- **Classification**: **Skill Update** (Actionable multi-step engineering patterns, system boundary checks, and cheatsheets for `dwm`, `dmenu`, and suckless desktop tooling).
- **Scope**: Fleet-wide (`oomaya/antigravity-skills` and local `.gemini/config/skills/dwm-craft/`). Reusable across all Debian, Ubuntu, Linux Mint, Fedora, and Arch/CachyOS nodes.

---

## 2. Root Causes & Core Discoveries from Today's Session

1. **Multi-Distro Shell Boundary (Quickshell vs. Suckless Fallback)**:
   - Upstream Quickshell strictly enforces **Qt $\ge$ 6.6.0** (`set(QT_MIN_VERSION "6.6.0")`).
   - Modern rolling/fast distributions (Fedora 44, Arch, CachyOS) ship Qt 6.7–6.8+ and package Quickshell natively.
   - Stable/LTS distributions (Ubuntu 24.04 noble, Linux Mint 22.x) ship Qt 6.4.2 in universe repositories; launchpad PPAs do not target `noble`.
   - **Invariant**: The window manager and desktop shell must cleanly detect Quickshell availability and fall back to `dmenu-oomaya` + native suckless topbar without crashing, dropping hotkeys, or leaving the user with a blank desktop void.

2. **The `[ -x "$basename" ]` PATH Resolution Trap**:
   - In POSIX `sh` / `bash`, `test -x "$cmd"` checks if a file exists and is executable in the **current working directory (`$PWD`)**, NOT in `$PATH`.
   - When a helper script like `dwm-keybinds` tests `if [ -x "$control_helper" ]; then` with a bare basename (`"dwm-quickshell-controlcenter"`), it silently evaluates to `false` unless `$PWD` happens to be `/usr/local/bin/`.
   - **Invariant**: Always resolve command names through `command -v "$cmd"` to an absolute path first before asserting `-x`, or use `command -v "$cmd" >/dev/null 2>&1`.

3. **Suckless Xft / Fontconfig Nerd Font PUA Fallback Architecture**:
   - `drw_fontset_create()` loads fonts in reverse order; the first font in `fonts[]` acts as the primary font and the template pattern for Fontconfig fallback substitutions.
   - Classic system monospace fonts (`DejaVu Sans Mono`, `Nimbus Mono`) lack Nerd Font Private Use Area (PUA) glyphs (`0xF000..0xF8FF`, `0xF0000..0xFFFFF`).
   - If fontconfig defaults `monospace` to a non-Nerd font, Xft character matching falls back to system fonts lacking icon coverage, causing garbled boxed X's (`` / U+FFFD).
   - **Invariant**:
     1. In `dmenu` and `dwm` C headers, compile explicit multi-font arrays covering both JetBrains, Meslo, and Color Emoji:
        `static const char *fonts[] = { "JetBrainsMono Nerd Font:size=16", "MesloLGS Nerd Font Mono:size=16", "Noto Color Emoji:size=14", "monospace:size=16" };`
     2. In user fontconfig (`~/.config/fontconfig/conf.d/10-nerd-font-default.conf`), explicitly prioritize `JetBrainsMono Nerd Font` and `MesloLGS Nerd Font Mono` as preferred families for `monospace`.

---

## 3. Proposed Additions to `dwm-craft/SKILL.md`

```markdown
--- a/dwm-craft/SKILL.md
+++ b/dwm-craft/SKILL.md
@@ -136,3 +136,49 @@
 Never use `cp -f` to replace active window manager or runner binaries on disk. Always use `install -Dm755` to safely unlink the active inode and write a new file, allowing hot-reloads in RAM without session crashes.
 
+---
+
+## 7. Multi-Distro Shell Resilience & Fallback Architecture
+
+### A. Quickshell Qt Version Invariant (Qt >= 6.6)
+- **Distribution Boundary**: Upstream Quickshell mandates Qt $\ge$ 6.6.0 (`set(QT_MIN_VERSION "6.6.0")`).
+  - **Tier 1 (Supported native packages)**: Fedora 44 (`quickshell` in official updates), Arch / CachyOS (`quickshell` in CachyOS / AUR).
+  - **Tier 2 (LTS / Debian / Mint 22.x)**: Ubuntu 24.04 noble only packages Qt 6.4.2; launchpad PPAs do not publish for noble.
+- **Suckless Fallback Law**: On nodes where Quickshell is absent, never leave the desktop void of menus or status. `dwm-oomaya` must decouple its altbar requirement and gracefully fall back to `dmenu-oomaya` (`dmenu-desktop`, `dmenu-run`, `dmenu-windows`, `dmenu-power`, `dmenu-hub`) and native `dwm` topbar rendering.
+
+### B. Shell Script `[ -x "$cmd" ]` PATH Resolution Trap
+- In POSIX `sh` / `bash`, `[ -x "$cmd" ]` checks if `$cmd` exists in the **current working directory (`$PWD`)**, not `$PATH`.
+- In launcher or helper wrappers, testing `if [ -x "some-helper" ]; then` fails silently unless the terminal happens to be located in `/usr/local/bin/`.
+- **The Rule**: Always resolve via `command -v` before checking permissions:
+  ```sh
+  helper=$(command -v some-helper 2>/dev/null || true)
+  if [ -n "$helper" ] && [ -x "$helper" ]; then
+      exec "$helper"
+  fi
+  ```
+
+---
+
+## 8. Suckless Xft & Fontconfig Nerd Font Invariants
+
+### A. Compound Fontset C Header Array
+In `dmenu` and `dwm`, Xft character lookup iterates through `drw->fonts` linked list. When icons (Private Use Area: `0xF000..0xF8FF`, `0xF0000..0xFFFFF`) are drawn, having only a single font or relying on generic `monospace` causes missing-character rectangles (boxed X's / ``) if the system monospace is non-Nerd (e.g. `DejaVu Sans Mono`).
+- **Compilation Standard**: Always define a compound fontset array with primary Nerd Font, secondary Nerd Font, and Color Emoji:
+  ```c
+  static const char *fonts[] = {
+      "JetBrainsMono Nerd Font:size=16",
+      "MesloLGS Nerd Font Mono:size=16",
+      "Noto Color Emoji:size=14",
+      "monospace:size=16"
+  };
+  ```
+
+### B. User Fontconfig Default Monospace Invariant
+To prevent any suckless or X11 application from inheriting legacy non-Nerd monospace fallbacks, ensure user fontconfig establishes Nerd Fonts as preferred:
+```xml
+<!-- ~/.config/fontconfig/conf.d/10-nerd-font-default.conf -->
+<alias>
+  <family>monospace</family>
+  <prefer>
+    <family>JetBrainsMono Nerd Font</family>
+    <family>MesloLGS Nerd Font Mono</family>
+    <family>DejaVu Sans Mono</family>
+  </prefer>
+</alias>
+```
```
