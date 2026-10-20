# Quickshell & DWM Fleet Update & Rollout Guide

> **Target Fleet**: Arch Linux / CachyOS, Fedora 40+, Debian 12 / Ubuntu 24.04 / Linux Mint 22 (Bare Metal & VM)  
> **Source Repositories**:  
> - `dwm-oomaya`: Sovereign Suckless C core, layout engine, and desktop lifecycle scripts  
> - `quickshell-oomaya`: Qt 6 / QML modular desktop shell (bar, tray, notification daemon, control center, launcher)  
> **Updated Date**: October 2026  
> **Status**: Verified Production Ready (100% CI & `make check` Green)

---

## 1. Executive Architecture & Fleet Compatibility Matrix

`dwm-oomaya` and `quickshell-oomaya` operate under a sovereign dual-shell model. The system provides zero-friction toggling between lightweight native suckless mode (dwm status + dunst) and rich modern QML shell mode (quickshell) via `dwm-shell-mode`.

| Tier | Distributions | Qt Version | Coreutils | Atomic File Mutation | Quickshell Packaging |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Tier 1 (Bleeding Edge)** | Arch Linux, CachyOS, Fedora 40+ | Qt 6.7 / 6.8+ | >= 9.5 | Native `mv --exchange` | Native package / COPR |
| **Tier 2 (LTS / Stable)** | Linux Mint 22, Ubuntu 24.04, Debian 12 | Qt 6.6+ (toolchain/backports) | 9.4 | `renameat2` shim (`tools/mv-shim`) | Custom Qt6 build / trixie-backports |

### Key Breakthroughs Included in This Release:
1. **Qt 6.6+ Signal Collision Immunity**: Eliminated `Connections.onEnabledChanged` signal shadowing bug in `SystemRegionalControls.qml` that crashed the QML runtime on modern Qt versions.
2. **D-Bus Notification Race Elimination**: Added Quickshell IPC readiness polling before `notify-send` in `dwm-shell-mode`, permanently preventing unwanted D-Bus autostart of `/usr/bin/dunst`.
3. **Coreutils < 9.5 Bridge (`renameat2` mv-shim)**: Automated installation of a lightweight C kernel shim (`tools/mv-shim`) for atomic panel configuration exchanges on LTS distros running GNU coreutils 9.4.
4. **Subshell Watchdog Termination**: Enforced `kill -KILL` prior to `wait` in helper scripts (`dwm-quickshell-controls`, `dwm-quickshell-network`, `dwm-quickshell-controlcenter`) to eliminate hang conditions during test suites and shell resets.
5. **VM Compositor Stability**: Adjusted `picom` defaults in `autostart.sh` to `--backend glx --no-use-damage` for zero-flicker hardware acceleration in virtualized environments.

---

## 2. Prerequisites by Distribution Fleet

Ensure each node has the required compilers, headers, and runtime tools before updating.

### Arch Linux / CachyOS Fleet
```bash
sudo pacman -S --needed base-devel git libx11 libxft libxinerama libxrender \
    imlib2 libxcb xcb-util fontconfig freetype2 \
    quickshell qt6-declarative picom feh playerctl networkmanager
```

### Fedora Fleet (40 / 41 / 42 / 44)
```bash
sudo dnf install -y git gcc make pkgconf-pkg-config \
    libX11-devel libXft-devel libXinerama-devel libXrender-devel \
    imlib2-devel libxcb-devel xcb-util-devel fontconfig-devel freetype-devel \
    quickshell qt6-qtdeclarative-devel picom feh playerctl NetworkManager
```

### Linux Mint 22 / Ubuntu 24.04 LTS Fleet
```bash
sudo apt update
sudo apt install -y build-essential git pkg-config \
    libx11-dev libxft-dev libxinerama-dev libxrender-dev \
    libimlib2-dev libxcb1-dev libxcb-util-dev libfontconfig1-dev libfreetype6-dev \
    picom feh playerctl network-manager
```
> [!NOTE]
> On Mint 22 / Ubuntu 24.04, ensure Quickshell is built against Qt 6.6+ and that a binary wrapper exists at `$HOME/.local/bin/quickshell` so that `/proc/$pid/comm` matches `quickshell` natively for process inspection.

---

## 3. Step-by-Step Manual Fleet Rollout

Run these steps on each node in the fleet. All paths dynamically resolve via `$HOME` and `$XDG_*` in compliance with the Zero Hardcoded Literals Law.

### Step 1: Update Source Trees
```bash
# 1. Update dwm-oomaya
cd "${HOME}/.local/src/dwm-oomaya"
git fetch origin main
git pull --rebase origin main

# 2. Update quickshell-oomaya
cd "${HOME}/.local/src/quickshell-oomaya"
git fetch origin main
git pull --rebase origin main
```

### Step 2: Build & Deploy `dwm-oomaya`
```bash
cd "${HOME}/.local/src/dwm-oomaya"

# Compile sovereign C binary
make clean all

# Install binary and desktop session integration
sudo make install

# Deploy updated lifecycle scripts to local user bin
mkdir -p "${HOME}/.local/bin"
install -Dm755 scripts/dwm-shell-mode "${HOME}/.local/bin/dwm-shell-mode"
install -Dm755 scripts/dwm-status "${HOME}/.local/bin/dwm-status"
install -Dm755 scripts/autostart.sh "${HOME}/.local/bin/dwm-autostart"
```

### Step 3: Build & Deploy `quickshell-oomaya`
```bash
cd "${HOME}/.local/src/quickshell-oomaya"

# Compile helper binary (dwm-quickshell-state-bin) and mv-shim
make clean all

# Run integration tests (Must return 0)
make check

# Install QML configuration, helper scripts, and conditional mv-shim
make install
```
> [!TIP]
> `make install` automatically inspects `mv --help`. If the host lacks `--exchange` (coreutils < 9.5), it automatically compiles and installs `tools/mv-shim` to `${HOME}/.local/bin/mv`.

### Step 4: Reload Shell & Verify
```bash
# Verify current shell status
dwm-shell-mode status

# Restart or activate Quickshell mode
dwm-shell-mode quickshell

# Check process tree and IPC
pgrep -a quickshell
quickshell ipc --path "${HOME}/.config/quickshell/shell.qml" call tray count
```

---

## 4. Automated Fleet Rollout Script (`fleet-update.sh`)

Save and run this script on any target machine to perform an atomic, guarded update:

```bash
#!/bin/sh
set -eu

SRC_DIR="${XDG_DATA_HOME:-${HOME}/.local/src}"
BIN_DIR="${HOME}/.local/bin"
CFG_DIR="${XDG_CONFIG_HOME:-${HOME}/.config}"

printf '==> [1/5] Updating dwm-oomaya repository...\n'
cd "${SRC_DIR}/dwm-oomaya"
git fetch origin main
git pull --rebase origin main

printf '==> [2/5] Building and installing dwm-oomaya...\n'
make clean all
if [ "$(id -u)" -eq 0 ]; then
    make install
elif command -v sudo >/dev/null 2>&1; then
    sudo make install
else
    install -Dm755 dwm "${BIN_DIR}/dwm"
fi
install -Dm755 scripts/dwm-shell-mode "${BIN_DIR}/dwm-shell-mode"
install -Dm755 scripts/dwm-status "${BIN_DIR}/dwm-status"

printf '==> [3/5] Updating quickshell-oomaya repository...\n'
cd "${SRC_DIR}/quickshell-oomaya"
git fetch origin main
git pull --rebase origin main

printf '==> [4/5] Building, verifying, and deploying quickshell-oomaya...\n'
make clean all
make check
make install

printf '==> [5/5] Re-synchronizing desktop shell mode...\n'
current_mode=$("${BIN_DIR}/dwm-shell-mode" current 2>/dev/null || printf 'quickshell')
if [ "$current_mode" = "quickshell" ]; then
    "${BIN_DIR}/dwm-shell-mode" quickshell
else
    "${BIN_DIR}/dwm-shell-mode" suckless
fi

printf '\n\033[1;32m✓ Fleet update completed successfully on $(hostname)!\033[0m\n'
"${BIN_DIR}/dwm-shell-mode" status
```

---

## 5. Post-Deployment Verification Checklist

After running the update, perform these checks:

| Check Item | Command / Action | Expected Result |
| :--- | :--- | :--- |
| **Shell Mode Status** | `dwm-shell-mode status` | `Configured Mode: quickshell`, `Quickshell Proc: RUNNING`, `Dunst Daemon: STOPPED` |
| **Dunst Conflict** | `pgrep -x dunst` | Return 1 (no process). Dunst must not autostart via D-Bus |
| **Panel Visibility** | Visual check top screen | 32px Tokyo Night panel showing active tags, window title, clock, tray |
| **Control Center** | Run `dwm-controlcenter` (`Super+c`) | Sliders (volume, brightness), theme selector, and power tiles open instantly |
| **Atomic Theme Apply** | `dwm-quickshell-controlcenter theme-set tokyonight` | Returns 0, themes apply atomically without file corruption |
| **IPC Readiness** | `quickshell ipc --path ~/.config/quickshell/shell.qml call tray count` | Returns integer count of active system tray icons |
| **Clean Mode Toggle** | `dwm-shell-mode suckless && sleep 1 && dwm-shell-mode quickshell` | Clean transition between classic bar/dunst and QML shell |

---

## 6. Distro-Specific Gotchas & Troubleshooting

### 1. `mv: --exchange requires exactly two operands` or missing option error
- **Cause**: Coreutils < 9.5 without `mv-shim`.
- **Fix**: Ensure `${HOME}/.local/bin` is at the head of `$PATH` in `~/.bashrc` or `~/.profile`:
  ```sh
  export PATH="${HOME}/.local/bin:${PATH}"
  ```
  Verify with `mv --help | grep -F -- '--exchange'`.

### 2. VMware / VirtualBox Compositor Tearing or Freezes
- **Cause**: Default `xrender` backend or unthrottled damage tracking in virtual display adapters.
- **Fix**: Verify `scripts/autostart.sh` uses `picom --backend glx --no-use-damage`. Run `picom -b --backend glx --no-use-damage` if needed.

### 3. Missing Tray Icons (Fcitx5 / Blueman)
- **Cause**: StatusNotifierItem / SNI D-Bus watcher race condition.
- **Fix**: Restart the notification applet or run `killall -9 quickshell && dwm-shell-mode quickshell`. Quickshell's `TrayArea.qml` will automatically reclaim the SNI watcher interface.

### 4. Custom Local Qt 6 Toolchain
- **Cause**: Mint/Ubuntu systems where Quickshell is built against a Qt installed under `$HOME/Qt/6.x.x/gcc_64`.
- **Fix**: `bin/quickshell-qmllint` automatically searches `$HOME/Qt/*/gcc_64/bin/qmllint`. Ensure `LD_LIBRARY_PATH` includes `$HOME/Qt/6.x.x/gcc_64/lib` in your `~/.profile` or `/etc/environment` if the system Qt runtime is too old.
