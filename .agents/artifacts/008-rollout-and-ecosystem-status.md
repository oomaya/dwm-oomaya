# DWM-Oomaya Status Report & Multi-VM Rollout Guide

## Executive Summary

This report covers:
1. Documentation & Repository status in `dwm-oomaya` (pure English standard, Quickshell companion decoupling, integration tests).
2. Pending roadmap actions (PR #12 merge, v0.8.0 release).
3. End-to-end multi-distro rollout instructions for secondary VMs (Fedora, Arch Linux, Debian/Ubuntu).

---

## 1. Documentation & Architecture Status

### A. Repository Documentation Modernization (100% English Standard)
All core documents in [`oomaya/dwm-oomaya`](https://github.com/oomaya/dwm-oomaya) have been updated to reflect the Sovereign C Core architecture:
- **`README.md`**: Complete overhaul featuring the pure C sovereign core, dynamic TOML live-reload, and the decoupled `quickshell-oomaya` companion architecture.
- **`docs/USER-GUIDE.md`**: Comprehensive 250-line end-user guide detailing installation, keybindings cheat-sheet, runtime config, and multi-distro setup.
- **`docs/SMOKE-TEST.md`**: 10-step host verification checklist covering status bar, launchers, control center, and window operations.
- **`ROADMAP.md` & `SPEC.md`**: Updated to Phase 3 milestones.
- **`config/hotkeys.toml` & `config.def.h`**: Synchronized with `Super + c` (Control Center), `Super + Shift + s` (Settings Hub), `Super + Shift + c` (Sovereign Flow Cockpit), and host-safe `Alt` aliases.

### B. `oomaya/quickshell-oomaya` Companion Decoupling & Verification
- **Companion Repo Published**: Fully isolated at [`https://github.com/oomaya/quickshell-oomaya`](https://github.com/oomaya/quickshell-oomaya).
- **Core Decoupling**: 40,956 lines of QML and heavy Qt6 CI fixtures were excised from `dwm-oomaya`.
- **Integration Test Status**:
  - `tests/test-dwm-shell-mode.sh`: Fully automated test verifying bidirectional switching between `suckless` and `quickshell` modes passes with 100% success.
  - `dwm-shell-mode` runtime: If Quickshell is not installed, it gracefully alerts the user while maintaining pure Suckless status. If installed, `dwm-controlcenter` automatically delegates to Quickshell via IPC.

---

## 2. Outstanding & Recommended Next Actions

| Task | Priority | Status | Action Required |
| :--- | :--- | :--- | :--- |
| **PR #12 Merge** | High | Green (`2/2 checks pass`) | Squash merge `packet/008-user-manual-and-docs` into `main` |
| **Release v0.8.0 Tag** | Medium | Pending PR #12 | Tag `v0.8.0` on `main` and publish release notes |
| **Package Upstream** | Low | Packaged locally | Submit `packaging/rpm/dwm-oomaya.spec` to COPR / `packaging/arch/PKGBUILD` to AUR |

---

## 3. Multi-VM Rollout Guide

Follow these standardized steps to roll out `dwm-oomaya` across other VMs (Fedora, Arch, Debian/Ubuntu).

### Method 1: Git User-Space Deployment (Fastest, No Root Required)

```bash
# 1. Clone repository
git clone https://github.com/oomaya/dwm-oomaya.git ~/.local/src/dwm-oomaya
cd ~/.local/src/dwm-oomaya

# 2. Check and install dependencies automatically
./scripts/check-deps.sh

# If manual package installation is needed:
# - Fedora:   sudo dnf install gcc make libX11-devel libXinerama-devel libXft-devel freetype-devel
# - Arch:     sudo pacman -S base-devel libx11 libxinerama libxft freetype2
# - Debian:   sudo apt install build-essential libx11-dev libxinerama-dev libxft-dev libfreetype6-dev

# 3. User-space build & installation
make install-user

# 4. (Optional) System-wide Display Manager session (.desktop entry for SDDM/GDM/LightDM)
sudo make install
```

### Method 2: Native Package Managers

- **Fedora / RHEL**:
  ```bash
  cd ~/.local/src/dwm-oomaya
  rpmbuild -ba packaging/rpm/dwm-oomaya.spec
  sudo dnf install ~/rpmbuild/RPMS/x86_64/dwm-oomaya-0.8.0-*.rpm
  ```
- **Arch Linux / CachyOS**:
  ```bash
  cd ~/.local/src/dwm-oomaya/packaging/arch
  makepkg -si
  ```

### Optional Companion: Quickshell Addon Rollout
If a target VM desires the modern Qt6 widget companion:
```bash
git clone https://github.com/oomaya/quickshell-oomaya.git ~/.local/src/quickshell-oomaya
cd ~/.local/src/quickshell-oomaya
make install
dwm-shell-mode quickshell
```
