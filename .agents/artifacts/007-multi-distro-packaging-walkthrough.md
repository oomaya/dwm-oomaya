# Walkthrough: Packet #007 - Multi-Distro Packaging (RPM & Arch PKGBUILD)

## Overview & Architecture

Following the extraction of `quickshell-oomaya` into an independent companion repository in Packet #006, `dwm-oomaya` is established as a pure C sovereign desktop core. Packet #007 delivers native, distribution-standard packaging across Fedora (RPM) and Arch Linux / CachyOS (PKGBUILD) with zero Qt6 runtime dependencies in the base install.

---

## Changes Implemented

### 1. Fedora RPM Specification (`packaging/rpm/dwm-oomaya.spec`)
- **Target Distributions**: Fedora 41, 42, 43, 44+ / COPR.
- **Pure C Build**: Standard `gcc`, `make`, `pkgconfig(x11)`, `pkgconfig(xft)`, `pkgconfig(imlib2)`, `pkgconfig(fontconfig)`.
- **Zero Heavy Runtime Bloat**: Does not require Qt6 or Python modules at runtime.
- **Config Seeding**: Installs system-wide default configuration templates to `%{_datadir}/dwm-oomaya/config/` without overwriting user configs.
- **Companion Decoupling**: Declares `Recommends: quickshell-oomaya` and `Recommends: dmenu-oomaya`, allowing users to install the optional overlay without forcing it on pure suckless users.

### 2. Arch Linux & CachyOS PKGBUILD (`packaging/arch/PKGBUILD`)
- **Target Distributions**: Arch Linux, CachyOS, Omarchy.
- **Standard Layout**: Full `build()` and `package()` lifecycle using `make install-system`.
- **Optional Dependencies (`optdepends`)**:
  - `quickshell-oomaya: Optional Qt6/QML desktop shell and widgets companion`
  - `dmenu-oomaya: Fast sovereign dynamic menu`
  - Audio, power, and bluetooth utilities.

### 3. Packaging Test Suite (`tests/test-packaging.sh` & `make check-packaging`)
- Automated validation of:
  - Version alignment between `config.mk`, `dwm-oomaya.spec`, and `PKGBUILD`.
  - Shell syntax of `PKGBUILD` via `bash -n`.
  - 100% binary manifest coverage: verifies that every script listed under `INSTALL_COMMANDS` in `Makefile` is tracked in `%files` of the RPM spec.
  - Strict absence of Qt6/QML assets in the core package.
- Integrated into `make check-core` and GitHub Actions CI.

---

## Verification Results

| Test Target | Scope | Result |
| :--- | :--- | :--- |
| `make check-packaging` | Version matching, bash syntax, `%files` completeness | **PASS** |
| `make check-shell check-format` | Shell script syntax and formatting | **PASS** |
| `make check-core` | Full daemon, X11 mocks, installation, and packaging suite | **PASS** |
| GitHub Actions CI | Pull Request #11 automated workflow | **PASS** |

---

## Related Pull Requests & Repositories
- PR: [oomaya/dwm-oomaya#11](https://github.com/oomaya/dwm-oomaya/pull/11)
- Companion Repo: [oomaya/quickshell-oomaya](https://github.com/oomaya/quickshell-oomaya)
