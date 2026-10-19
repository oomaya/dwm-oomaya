# Task Packet #007: Multi-Distro Packaging (RPM & Arch PKGBUILD)

## Status: IN_PROGRESS
- **Owner**: Antigravity Pair
- **Branch**: `packet/007-multi-distro-packaging`
- **Related PRs**: TBD

## Description
Provide official, clean, native distribution packaging for `dwm-oomaya` (pure C sovereign desktop, zero Qt6 runtime dependencies) across Fedora/COPR and Arch Linux/CachyOS.

## Checklist
- [x] 1. Create Git branch `packet/007-multi-distro-packaging`.
- [x] 2. Create `packaging/rpm/dwm-oomaya.spec` targeting Fedora 41–44 (clean C99 build, X11/Xft/fontconfig deps, no Qt6).
- [x] 3. Create `packaging/arch/PKGBUILD` for Arch Linux & CachyOS.
- [x] 4. Add `check-packaging` validation target in `Makefile` (integrated into `check-core`).
- [x] 5. Add `tests/test-packaging.sh` validating version alignment, syntax, and %files manifest completeness.
- [x] 6. Verify local test suite (`make check-core`) passes 100%.
- [ ] 7. Commit, push branch, open PR, and verify CI.
- [ ] 8. Merge PR to `main` and report walkthrough.
