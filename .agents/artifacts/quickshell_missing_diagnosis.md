# Diagnosis: Quickshell Status on Linux Mint 22.3 (Zena)

## 1. Executive Root-Cause Summary
- **Distribution Constraint**: `mint-vm` is running **Linux Mint 22.3**, which tracks **Ubuntu 24.04 LTS (noble)**.
- **Qt Version Floor**: Quickshell requires **Qt $\ge$ 6.6.0** (`set(QT_MIN_VERSION "6.6.0")` in upstream CMakeLists). Ubuntu 24.04 noble repositories only provide **Qt 6.4.2**.
- **PPA Support**: The `ppa:avengemedia/danklinux` PPA builds Quickshell packages only for newer Ubuntu targets (e.g., `plucky`, `stonking`), skipping `noble`.
- **Fleet Differences**:
  - **Fedora 44**: Quickshell is packaged natively in official Fedora updates repositories (Qt 6.7+).
  - **Arch Linux / CachyOS (`omarchy-gram`)**: Quickshell is available via official CachyOS / AUR packages (Qt 6.8+).
  - **Debian / Ubuntu 24.04**: Quickshell is omitted by design; the installer specifies:  
    `"Quickshell not detected; dmenu-oomaya and standard X11 utilities will be used."`

---

## 2. Compounding Desktop Symptoms
1. **Missing Topbar**: Upstream `dwm-titus` / `dwm-oomaya` disabled the native suckless topbar in C (`drawbar(Monitor *m)` in [dwm.c](file:///home/rand/.local/src/dwm-oomaya/dwm.c#L1353-L1357) has an unconditional early `return;`), delegating 100% of topbar rendering to Quickshell. Without Quickshell running, no bar is drawn.
2. **Missing `dmenu-desktop` / Hotkeys**: During the earlier fleet rectification build, only `sudo make install` of dwm was run. The companion `install_dmenu_ecosystem` was not invoked, leaving `dmenu-desktop`, `dmenu-windows`, `dmenu-hub`, and `dmenu-power` unbuilt on `mint-vm` (`/bin/sh: 1: dmenu-desktop: not found` in `~/.xsession-errors`).
3. **Background Status Active**: `dwm-status` is currently running healthy in the background and populating the root window name (`WM_NAME = "AC | VOL 50%"`), but dwm's bar is not displaying it due to the early return in `drawbar()`.

---

## 3. Recommended Remediation Paths

### Path A: Deploy `dmenu-oomaya` + Native Tokyo Night DWM Bar (Fast, Bloat-Free POSIX)
1. **Clone & Install `dmenu-oomaya`**:
   Build `oomaya/dmenu-oomaya` into `/usr/local/bin`, restoring `<kbd>Super</kbd> + <kbd>D</kbd>` (`dmenu-desktop`), `<kbd>Alt</kbd> + <kbd>P</kbd>` (`dmenu-run`), `<kbd>Alt</kbd> + <kbd>Tab</kbd>` (`dmenu-windows`), and `<kbd>Alt</kbd> + <kbd>X</kbd>` (`dmenu-power`).
2. **Enable Native DWM Bar Fallback in `dwm.c`**:
   Allow `drawbar(Monitor *m)` and `updatebars()` to draw the native status bar with Tokyo Night styling, tags `[1]-[9]`, and `dwm-status` (`WM_NAME`) whenever Quickshell is absent.

### Path B: Standalone Quickshell via Modern Qt Toolchain or Container
- Build Quickshell inside a container/Distrobox or install newer Qt libraries in an isolated prefix (`/opt/qt6`), allowing the full QML topbar and control center to run on Mint.
